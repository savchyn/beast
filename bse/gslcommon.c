/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik and Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gslcommon.h"

#include "gsldatacache.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>


/* --- variables --- */
volatile guint64     gsl_externvar_tick_stamp = 0;
static guint64	     tick_stamp_system_time = 0;
static guint         global_tick_stamp_leaps = 0;
static GslDebugFlags gsl_debug_flags = 0;


/* --- tick stamps --- */
static SfiMutex     global_tick_stamp_mutex = { 0, };
/**
 * gsl_tick_stamp
 * @RETURNS: GSL's execution tick stamp as unsigned 64bit integer
 *
 * Retrieve the global GSL tick counter stamp.
 * GSL increments its global tick stamp at certain intervals,
 * by specific amounts (refer to gsl_engine_init() for further
 * details). The tick stamp is a non-wrapping, unsigned 64bit
 * integer greater than 0. Threads can schedule sleep interruptions
 * at certain tick stamps with sfi_thread_awake_after() and
 * sfi_thread_awake_before(). Tick stamp updating occours at
 * GSL engine block processing boundaries, so code that can
 * guarantee to not run across those boundaries (for instance
 * GslProcessFunc() functions) may use the macro %GSL_TICK_STAMP
 * to retrive the current tick in a faster manner (not involving
 * mutex locking). See also gsl_module_tick_stamp().
 * This function is MT-safe and may be called from any thread.
 */
guint64
gsl_tick_stamp (void)
{
  guint64 stamp;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  stamp = gsl_externvar_tick_stamp;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  return stamp;
}

void
_gsl_tick_stamp_set_leap (guint ticks)
{
  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  global_tick_stamp_leaps = ticks;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);
}

/**
 * gsl_tick_stamp_last
 * @RETURNS: Current tick stamp and system time in micro seconds
 *
 * Get the system time of the last GSL global tick stamp update.
 * This function is MT-safe and may be called from any thread.
 */
GslTickStampUpdate
gsl_tick_stamp_last (void)
{
  GslTickStampUpdate ustamp;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  ustamp.tick_stamp = gsl_externvar_tick_stamp;
  ustamp.system_time = tick_stamp_system_time;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  return ustamp;
}

void
_gsl_tick_stamp_inc (void)
{
  volatile guint64 newstamp;
  guint64 systime;

  g_return_if_fail (global_tick_stamp_leaps > 0);

  systime = sfi_time_system ();
  newstamp = gsl_externvar_tick_stamp + global_tick_stamp_leaps;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  gsl_externvar_tick_stamp = newstamp;
  tick_stamp_system_time = systime;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  sfi_thread_emit_wakeups (newstamp);
}

/**
 * gsl_thread_awake_before
 * @tick_stamp: tick stamp update to trigger wakeup
 * Wakeup the currently running thread upon the last global tick stamp
 * update (see gsl_tick_stamp()) that happens prior to updating the
 * global tick stamp to @tick_stamp.
 * (If the moment of wakeup has already passed by, the thread is
 * woken up at the next global tick stamp update.)
 */
void
gsl_thread_awake_before (guint64 tick_stamp)
{
  g_return_if_fail (tick_stamp > 0);

  if (tick_stamp > global_tick_stamp_leaps)
    sfi_thread_awake_after (tick_stamp - global_tick_stamp_leaps);
  else
    sfi_thread_awake_after (tick_stamp);
}


/* --- GslMessage --- */
const gchar*
gsl_strerror (GslErrorType error)
{
  switch (error)
    {
    case GSL_ERROR_NONE:		return "Everything went well";
    case GSL_ERROR_INTERNAL:		return "Internal error (please report)";
    case GSL_ERROR_UNKNOWN:		return "Unknown error";
    case GSL_ERROR_IO:			return "I/O error";
    case GSL_ERROR_PERMS:		return "Insufficient permission";
    case GSL_ERROR_BUSY:		return "Resource currently busy";
    case GSL_ERROR_EXISTS:		return "Resource exists already";
    case GSL_ERROR_TEMP:		return "Temporary error";
    case GSL_ERROR_EOF:			return "File empty or premature EOF";
    case GSL_ERROR_NOT_FOUND:		return "Resource not found";
    case GSL_ERROR_OPEN_FAILED:		return "Open failed";
    case GSL_ERROR_SEEK_FAILED:		return "Seek failed";
    case GSL_ERROR_READ_FAILED:		return "Read failed";
    case GSL_ERROR_WRITE_FAILED:	return "Write failed";
    case GSL_ERROR_FORMAT_INVALID:	return "Invalid format";
    case GSL_ERROR_FORMAT_UNKNOWN:	return "Unknown format";
    case GSL_ERROR_DATA_CORRUPT:        return "Data corrupt";
    case GSL_ERROR_CONTENT_GLITCH:      return "Data glitch (junk) detected";
    case GSL_ERROR_NO_RESOURCE:		return "Out of memory, disk space or similar resource";
    case GSL_ERROR_CODEC_FAILURE:	return "CODEC failure";
    default:				return NULL;
    }
}

static const GDebugKey gsl_static_debug_keys[] = {
  { "notify",         GSL_MSG_NOTIFY },
  { "dcache",         GSL_MSG_DATA_CACHE },
  { "dhandle",        GSL_MSG_DATA_HANDLE },
  { "loader",         GSL_MSG_LOADER },
  { "osc",	      GSL_MSG_OSC },
  { "engine",         GSL_MSG_ENGINE },
  { "jobs",           GSL_MSG_JOBS },
  { "fjobs",          GSL_MSG_FJOBS },
  { "sched",          GSL_MSG_SCHED },
  { "master",         GSL_MSG_MASTER },
  { "slave",          GSL_MSG_SLAVE },
};

static const gchar*
reporter_name (GslDebugFlags reporter)
{
  switch (reporter)
    {
    case GSL_MSG_NOTIFY:	return "Notify";
    case GSL_MSG_DATA_CACHE:	return "DataCache";
    case GSL_MSG_DATA_HANDLE:	return "DataHandle";
    case GSL_MSG_LOADER:	return "Loader";
    case GSL_MSG_OSC:		return "Oscillator";
    case GSL_MSG_ENGINE:	return "Engine";	/* Engine */
    case GSL_MSG_JOBS:		return "Jobs";		/* Engine */
    case GSL_MSG_FJOBS:		return "FlowJobs";	/* Engine */
    case GSL_MSG_SCHED:		return "Sched";		/* Engine */
    case GSL_MSG_MASTER:	return "Master";	/* Engine */
    case GSL_MSG_SLAVE:		return "Slave";		/* Engine */
    default:			return "Custom";
    }
}

const GDebugKey *gsl_debug_keys = gsl_static_debug_keys;
const guint      gsl_n_debug_keys = G_N_ELEMENTS (gsl_static_debug_keys);

void
gsl_message_send (GslDebugFlags reporter,
		  const gchar  *section,
		  GslErrorType  error,
		  const gchar  *messagef,
		  ...)
{
  struct {
    GslDebugFlags reporter;
    gchar         reporter_name[64];
    gchar         section[64];	/* auxillary information about reporter code portion */
    GslErrorType  error;
    const gchar	 *error_str;	/* gsl_strerror() of error */
    gchar	  message[1024];
  } tmsg, *msg = &tmsg;
  gchar *string;
  va_list args;
    
  g_return_if_fail (messagef != NULL);

  /* create message */
  memset (msg, 0, sizeof (*msg));
  msg->reporter = reporter;
  strncpy (msg->reporter_name, reporter_name (msg->reporter), 63);
  if (section)
    strncpy (msg->section, section, 63);
  msg->error = error;
  msg->error_str = error ? gsl_strerror (msg->error) : NULL;

  /* vsnprintf() replacement */
  va_start (args, messagef);
  string = g_strdup_vprintf (messagef, args);
  va_end (args);
  strncpy (msg->message, string, 1023);
  g_free (string);

  /* in current lack of a decent message queue, puke the message to stderr */
  g_printerr ("GSL-%s%s%s: %s%s%s\n",
	      msg->reporter_name,
	      msg->section ? ":" : "",
	      msg->section ? msg->section : "",
	      msg->message,
	      msg->error_str ? ": " : "",
	      msg->error_str ? msg->error_str : "");
}

void
gsl_debug_enable (GslDebugFlags dbg_flags)
{
  gsl_debug_flags |= dbg_flags;
}

void
gsl_debug_disable (GslDebugFlags dbg_flags)
{
  gsl_debug_flags &= dbg_flags;
}

gboolean
gsl_debug_check (GslDebugFlags dbg_flags)
{
  return (gsl_debug_flags & dbg_flags) != 0;
}

void
gsl_debug (GslDebugFlags reporter,
	   const gchar  *section,
	   const gchar  *format,
	   ...)
{
  g_return_if_fail (format != NULL);

  if (reporter & gsl_debug_flags)
    {
      va_list args;
      gchar *string;

      va_start (args, format);
      string = g_strdup_vprintf (format, args);
      va_end (args);
      g_printerr ("DEBUG:GSL-%s%s%s: %s\n",
		  reporter_name (reporter),
		  section ? ":" : "",
		  section ? section : "",
		  string);
      g_free (string);
    }
}

void
gsl_auxlog_push (GslDebugFlags reporter,
		 const gchar  *section)
{
  sfi_thread_set_data ("auxlog_reporter", (gpointer) reporter);
  sfi_thread_set_data ("auxlog_section", (char*) section);
}

void
gsl_auxlog_debug (const gchar *format,
		  ...)
{
  GslDebugFlags reporter = (guint) sfi_thread_get_data ("auxlog_reporter");
  const gchar *section = sfi_thread_get_data ("auxlog_section");
  va_list args;
  gchar *string;

  if (!reporter)
    reporter = GSL_MSG_NOTIFY;
  sfi_thread_set_data ("auxlog_reporter", 0);
  sfi_thread_set_data ("auxlog_section", NULL);

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  gsl_debug (reporter, section, "%s", string);
  g_free (string);
}

void
gsl_auxlog_message (GslErrorType error,
		    const gchar *format,
		    ...)
{
  GslDebugFlags reporter = (guint) sfi_thread_get_data ("auxlog_reporter");
  const gchar *section = sfi_thread_get_data ("auxlog_section");
  va_list args;
  gchar *string;

  if (!reporter)
    reporter = GSL_MSG_NOTIFY;
  sfi_thread_set_data ("auxlog_reporter", 0);
  sfi_thread_set_data ("auxlog_section", NULL);

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  gsl_message_send (reporter, section, error, "%s", string);
  g_free (string);
}


/* --- misc --- */
const gchar*
gsl_byte_order_to_string (guint byte_order)
{
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);

  if (byte_order == G_LITTLE_ENDIAN)
    return "little_endian";
  if (byte_order == G_BIG_ENDIAN)
    return "big_endian";

  return NULL;
}

guint
gsl_byte_order_from_string (const gchar *string)
{
  g_return_val_if_fail (string != NULL, 0);

  while (*string == ' ')
    string++;
  if (strncasecmp (string, "little", 6) == 0)
    return G_LITTLE_ENDIAN;
  if (strncasecmp (string, "big", 3) == 0)
    return G_BIG_ENDIAN;
  return 0;
}

GslErrorType
gsl_check_file (const gchar *file_name,
		const gchar *mode)
{
  guint access_mask = 0;
  guint check_file, check_dir, check_link;
  
  if (strchr (mode, 'r'))	/* readable */
    access_mask |= R_OK;
  if (strchr (mode, 'w'))	/* writable */
    access_mask |= W_OK;
  if (strchr (mode, 'x'))	/* executable */
    access_mask |= X_OK;

  if (access_mask && access (file_name, access_mask) < 0)
    goto have_errno;
  
  check_file = strchr (mode, 'f') != NULL;	/* open as file */
  check_dir  = strchr (mode, 'd') != NULL;	/* open as directory */
  check_link = strchr (mode, 'l') != NULL;	/* open as link */

  if (check_file || check_dir || check_link)
    {
      struct stat st;
      
      if (check_link)
	{
	  if (lstat (file_name, &st) < 0)
	    goto have_errno;
	}
      else if (stat (file_name, &st) < 0)
	goto have_errno;

      if ((check_file && !S_ISREG (st.st_mode)) ||
	  (check_dir && !S_ISDIR (st.st_mode)) ||
	  (check_link && !S_ISLNK (st.st_mode)))
	return GSL_ERROR_OPEN_FAILED;
    }

  return GSL_ERROR_NONE;
  
 have_errno:
  return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
}

GslErrorType
gsl_error_from_errno (gint         sys_errno,
		      GslErrorType fallback)
{
  switch (sys_errno)
    {
    case ELOOP:
    case ENAMETOOLONG:
    case ENOTDIR:
    case ENOENT:        return GSL_ERROR_NOT_FOUND;
    case EROFS:
    case EPERM:
    case EACCES:        return GSL_ERROR_PERMS;
    case ENOMEM:
    case ENOSPC:
    case EFBIG:
    case ENFILE:
    case EMFILE:	return GSL_ERROR_NO_RESOURCE;
    case EISDIR:
    case ESPIPE:
    case EIO:           return GSL_ERROR_IO;
    case EEXIST:        return GSL_ERROR_EXISTS;
    case ETXTBSY:
    case EBUSY:         return GSL_ERROR_BUSY;
    case EAGAIN:
    case EINTR:		return GSL_ERROR_TEMP;
    case EINVAL:
    case EFAULT:
    case EBADF:         return GSL_ERROR_INTERNAL;
    default:            return fallback;
    }
}


/* --- global initialization --- */
static guint
get_n_processors (void)
{
#ifdef _SC_NPROCESSORS_ONLN
  {
    gint n = sysconf (_SC_NPROCESSORS_ONLN);

    if (n > 0)
      return n;
  }
#endif
  return 1;
}

static const GslConfig *gsl_config = NULL;

const GslConfig*
gsl_get_config (void)
{
  return gsl_config;
}

#define	ROUND(dblval)	((GslLong) ((dblval) + .5))

void
gsl_init (const GslConfigValue values[])
{
  const GslConfigValue *config = values;
  static GslConfig pconfig = {	/* DEFAULTS */
    1,				/* n_processors */
    2,				/* wave_chunk_padding */
    4,				/* wave_chunk_big_pad */
    512,			/* dcache_block_size */
    1024 * 1024,		/* dcache_cache_memory */
    69,				/* midi_kammer_note */
    440,			/* kammer_freq */
  };

  sfi_init ();	/* ease transition */

  g_return_if_fail (gsl_config == NULL);	/* assert single initialization */

  gsl_externvar_tick_stamp = 1;

  /* configure permanent config record */
  if (config)
    while (config->value_name)
      {
	if (strcmp ("wave_chunk_padding", config->value_name) == 0)
	  pconfig.wave_chunk_padding = ROUND (config->value);
	else if (strcmp ("wave_chunk_big_pad", config->value_name) == 0)
	  pconfig.wave_chunk_big_pad = ROUND (config->value);
	else if (strcmp ("dcache_cache_memory", config->value_name) == 0)
	  pconfig.dcache_cache_memory = ROUND (config->value);
	else if (strcmp ("dcache_block_size", config->value_name) == 0)
	  pconfig.dcache_block_size = ROUND (config->value);
	else if (strcmp ("midi_kammer_note", config->value_name) == 0)
	  pconfig.midi_kammer_note = ROUND (config->value);
	else if (strcmp ("kammer_freq", config->value_name) == 0)
	  pconfig.kammer_freq = config->value;
	config++;
      }
  
  /* constrain (user) config */
  pconfig.wave_chunk_padding = MAX (1, pconfig.wave_chunk_padding);
  pconfig.wave_chunk_big_pad = MAX (2 * pconfig.wave_chunk_padding, pconfig.wave_chunk_big_pad);
  pconfig.dcache_block_size = MAX (2 * pconfig.wave_chunk_big_pad + sizeof (GslDataType), pconfig.dcache_block_size);
  pconfig.dcache_block_size = sfi_alloc_upper_power2 (pconfig.dcache_block_size - 1);
  /* pconfig.dcache_cache_memory = sfi_alloc_upper_power2 (pconfig.dcache_cache_memory); */

  /* non-configurable config updates */
  pconfig.n_processors = get_n_processors ();

  /* export GSL configuration */
  gsl_config = &pconfig;

  /* initialize subsystems */
  sfi_mutex_init (&global_tick_stamp_mutex);
  _gsl_init_signal ();
  _gsl_init_fd_pool ();
  _gsl_init_data_caches ();
  _gsl_init_engine_utils ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
  _gsl_init_loader_mad ();
}
