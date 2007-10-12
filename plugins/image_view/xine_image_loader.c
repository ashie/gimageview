/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2004 Takuro Ashie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include "xine_image_loader.h"

#ifdef ENABLE_XINE

/*#include "gimv_xine.h"*/
#include "gimv_plugin.h"
#include "gimv_xine_priv.h"
#include "prefs_xine.h"

static GimvImageLoaderPlugin gimv_xine_image_loader =
{
   if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
   id:            "XINE",
   priority_hint: GIMV_IMAGE_LOADER_PRIORITY_HIGH,
   check_type:    NULL,
   get_info:      NULL,
   loader:        gimv_xine_image_loader_load_file,
};


void
gimv_xine_image_loader_get_impl (gpointer *impl, guint *size)
{
   *impl = &gimv_xine_image_loader;
   *size = sizeof(gimv_xine_image_loader);
}


GimvImage *
gimv_xine_image_loader_load_file (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GimvImage *image = NULL;
   GimvXinePrivImage *pimage = NULL;
   xine_t *xine;
   xine_stream_t *stream;
   xine_vo_driver_t *vo_driver;
   xine_ao_driver_t *ao_driver;
   guchar *rgb = NULL;
   gint width, height;
   gint ret = 0;
   gfloat pos, ppos;
   gint pos_stream, pos_time, length_time;

   /* FIXME */
   if (!gimv_prefs_xine_get_thumb_enable()) return NULL;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

#warning FIXME
   if (!gimv_image_info_is_movie (loader->info)
       && !gimv_mime_types_extension_is (filename, "rm"))
   {
      return NULL;
   }

   /* FIXME! use exist xine object */
   xine = xine_new();
   xine_init(xine);

   vo_driver = xine_open_video_driver (xine,
                                       "none", XINE_VISUAL_TYPE_NONE,
                                       NULL);
   ao_driver = xine_open_audio_driver (xine,
                                       "none",
                                       NULL);
   stream = xine_stream_new (xine, ao_driver, vo_driver);

   ret = xine_open (stream, filename);
   if (!ret) goto ERROR;

   xine_get_pos_length (stream, &pos_stream, &pos_time, &length_time);
   pos = gimv_prefs_xine_get_thumb_pos();
   ppos = (gfloat) pos * (gfloat) length_time / 100.0;
   xine_play (stream, 0, ppos);

   width  = xine_get_stream_info (stream, XINE_STREAM_INFO_VIDEO_WIDTH);
   height = xine_get_stream_info (stream, XINE_STREAM_INFO_VIDEO_HEIGHT);

   pimage = gimv_xine_priv_image_new (sizeof (guchar) * width * height * 2);

   ret = xine_get_current_frame(stream,
                                &pimage->width, &pimage->height,
                                &pimage->ratio_code,
                                &pimage->format,
                                pimage->img);
   if (!ret) goto ERROR;
   if (!pimage->img) goto ERROR;

   rgb = gimv_xine_priv_yuv2rgb (pimage);
   width = pimage->width;
   height = pimage->height;

   if (rgb)
      image = gimv_image_create_from_data (rgb, width, height, FALSE);

   /* clean */
ERROR:
   if (pimage)
      gimv_xine_priv_image_delete (pimage);

   xine_stop (stream);

   xine_close (stream);
   xine_dispose (stream);

   xine_close_audio_driver(xine, ao_driver);
   xine_close_video_driver(xine, vo_driver);

   /* FIXME! use exist xine object */
   xine_exit (xine);

   return image;
}

#endif /* ENABLE_XINE */
