/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * PPM image saver plugin for GImageView
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
 *
 * $Id$
 */

#include <stdio.h>
#include <string.h>

#include "gimv_image.h"

#include "gimv_plugin.h"
#include "gimv_image_saver.h"

static gboolean save_pnm (GimvImageSaver *saver,
                          GimvImage *image,
                          const gchar *filename,
                          const gchar *format);

static GimvImageSaverPlugin plugin_impl[] =
{
   {GIMV_IMAGE_SAVER_IF_VERSION, "pnm", save_pnm, NULL},
   {GIMV_IMAGE_SAVER_IF_VERSION, "ppm", save_pnm, NULL},
   {GIMV_IMAGE_SAVER_IF_VERSION, "pgm", save_pnm, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_IMAGE_SAVER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("PNM Image Saver"),
   version:       "0.2.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


static gboolean
save_pnm (GimvImageSaver *saver,
          GimvImage *image,
          const gchar *filename,
          const gchar *format)
{
   FILE *handle;
   gint width, height, rowstride;
   gint x, y, sum;
   guchar *source_ptr, val, *pixels;

   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (filename, FALSE);
   g_return_val_if_fail (filename[0] != '\0', FALSE);

   handle = fopen(filename, "wb");
   if (!handle)
      return FALSE;

   width     = gimv_image_width (image);
   height    = gimv_image_height (image);
   pixels    = gimv_image_get_pixels (image);
   rowstride = gimv_image_rowstride (image);

   if (!strcmp (format, "pgm")){   /* pgm is gray scale format */
      if (!fprintf(handle, "P5\n# Created by GImageView\n%i %i\n255\n",
                   width, height)) {

         fclose(handle);
         return FALSE;
      }

      for (y = 0; y < height; y++) {
         source_ptr = pixels;
         for (x = 0; x < width; x++) {
            /* convert to gray scale */
            sum  = (int)(*source_ptr++);
            sum += (int)(*source_ptr++);
            sum += (int)(*source_ptr++);
            val  = (guchar)(sum / 3);
            fwrite(&val, 1, 1, handle);
         }
         pixels += rowstride;
      }

      fclose(handle);
      return TRUE;

   } else {
      if (!fprintf(handle, "P6\n# Created by GImageView\n%i %i\n255\n",
                   width, height)) {
         fclose(handle);
         return FALSE;
      }

      for (y = 0; y < height; y++) {
         source_ptr = pixels;
         fwrite(source_ptr, 1, (width * 3), handle);
         pixels += rowstride;
      }

      fclose(handle);
      return TRUE;
   }
}
