/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * XVPICS image saver plugin for GImageView
 * Copyright (C) 2001 Takuro Ashie
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
 */

#include <stdio.h>
#include <png.h>

#include "gimv_image.h"

#include "dither.h"
#include "gimv_plugin.h"
#include "gimv_image_saver.h"

static gboolean save_xvpics (GimvImageSaver *saver,
                             GimvImage *image,
                             const gchar *filename,
                             const gchar *format);

static GimvImageSaverPlugin plugin_impl[] =
{
   {GIMV_IMAGE_SAVER_IF_VERSION, "xvpic",  save_xvpics, NULL},
   {GIMV_IMAGE_SAVER_IF_VERSION, "xvpics", save_xvpics, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_IMAGE_SAVER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("XVPICS Image Saver"),
   version:       "0.2.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


static gboolean
save_xvpics (GimvImageSaver *saver,
             GimvImage *image,
             const gchar *filename,
             const gchar *format)
{
   GimvImageInfo *info;
   FILE *handle;
   gint width, height, rowstride;
   guchar *pixels, *buf;
   gboolean success;
   gint orig_width = -1, orig_height = -1, orig_size = -1;
   gint y;

   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (filename, FALSE);
   g_return_val_if_fail (filename[0] != '\0', FALSE);

   handle = fopen(filename, "wb");
   if (!handle)
      return FALSE;

   width  = gimv_image_width (image);
   height = gimv_image_height (image);
   pixels = gimv_image_get_pixels (image);
   rowstride = gimv_image_rowstride (image);

   info = gimv_image_saver_get_image_info (saver);
   if (info) {
      gimv_image_info_get_image_size (info, &orig_width, &orig_height);
      orig_size = info->st.st_size;
   }
   success = fprintf(handle,
                     "P7 332\n"
                     "#IMGINFO:%dx%d RGB (%d bytes)\n"
                     "#END_OF_COMMENTS\n"
                     "%d %d 255\n",
                     orig_width, orig_height, orig_size,
                     width, height);
   if (!success) {
      fclose(handle);
      return FALSE;
   }

   buf = ditherinit (width);
   for (y = 0; y < height; y++) {
      ditherline(pixels + y * rowstride, y, width);
      fwrite(buf, 1, width, handle);
   }
   ditherfinish();

   fclose(handle);
   return TRUE;
}
