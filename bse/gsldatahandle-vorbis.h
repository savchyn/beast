/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
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
#ifndef __GSL_DATA_HANDLE_VORBIS_H__
#define __GSL_DATA_HANDLE_VORBIS_H__


#include <bse/gslcommon.h>
#include <bse/gsldatahandle.h>

G_BEGIN_DECLS

/* linear-read handle! needs linbuffer handle wrapper
 */
GslDataHandle*	gsl_data_handle_new_ogg_vorbis_muxed    (const gchar	*file_name,
                                                         guint		 lbitstream);
GslDataHandle*	gsl_data_handle_new_ogg_vorbis_zoffset	(const gchar	*file_name,
                                                         GslLong         byte_offset,
                                                         GslLong         byte_size);

G_END_DECLS

#endif /* __GSL_DATA_HANDLE_VORBIS_H__ */
