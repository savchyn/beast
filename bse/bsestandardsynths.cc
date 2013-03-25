// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsestandardsynths.hh"
#include "bsesnet.hh"
#include "bsestandardosc.hh"
#include <zlib.h>
#include <string.h>
/* --- typedef & structures --- */
typedef struct {
  const gchar  *name;
  guint         text_size;
  const guint8 *cdata;
  guint         clength;
} BseZFile;
/* --- generated ZFiles --- */
#include "bse/zintern/bse-zfile.cc"	/* bse_zfiles */
/* --- variables --- */
static GSList	*zfile_names = NULL;
/* --- functions --- */
static gchar*
bse_zfile_uncompress (const BseZFile *zfile,
		      guint          *text_len)
{
  uLongf dlen = zfile->text_size;
  guint len = dlen + 1;
  guint8 *text = (guint8*) g_malloc (len);
  gint result;
  const gchar *err;
  if (zfile->clength)	/* indicates compresssion */
    result = uncompress (text, &dlen, zfile->cdata, zfile->clength);
  else
    {
      memcpy (text, zfile->cdata, dlen);
      result = Z_OK;
    }
  switch (result)
    {
    case Z_OK:
      if (dlen == zfile->text_size)
	{
	  err = NULL;
	  break;
	}
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    g_error ("while decompressing \"%s\": %s", zfile->name, err);
  text[dlen] = 0;
  if (text_len)
    *text_len = dlen;
  return (char*) text;
}
gchar*
bse_standard_synth_inflate (const gchar *synth_name,
			    guint       *text_len)
{
  guint i;
  g_return_val_if_fail (synth_name != NULL, NULL);
  for (i = 0; i < G_N_ELEMENTS (bse_zfiles); i++)
    if (strcmp (synth_name, bse_zfiles[i].name) == 0)
      return bse_zfile_uncompress (bse_zfiles + i, text_len);
  g_warning ("unknown standard synth: %s", synth_name);
  return NULL;
}
GSList*
bse_standard_synth_get_list (void)
{
  guint i;
  if (!zfile_names)
    for (i = 0; i < G_N_ELEMENTS (bse_zfiles); i++)
      zfile_names = g_slist_prepend (zfile_names, (gchar*) bse_zfiles[i].name);
  return zfile_names;
}