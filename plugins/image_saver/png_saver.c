/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * PNG image saver plugin for GImageView
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
#include <png.h>

#include "gimv_image.h"

#include "gimv_plugin.h"
#include "gimv_image_saver.h"

static gboolean save_png (GimvImageSaver *saver,
                          GimvImage *image,
                          const gchar *filename,
                          const gchar *format);

static GimvImageSaverPlugin plugin_impl[] =
{
   {GIMV_IMAGE_SAVER_IF_VERSION, "png", save_png, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_IMAGE_SAVER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("PNG Image Saver"),
   version:       "0.2.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


static png_text *
create_png_text (GimvImageSaver *saver,
                 const gchar *filename,
                 gint *n_text)
{
   png_text *text;
   gint i;

   g_return_val_if_fail (n_text, NULL);

   *n_text = gimv_image_saver_get_n_comments (saver) + 2;

   text = g_new0 (png_text, *n_text);

   text[0].key         = "Title";
   text[0].text        = (gchar *) filename;
   text[0].compression = PNG_TEXT_COMPRESSION_NONE;
   text[1].key         = "Software";
   text[1].text        = GIMV_PROG_NAME;
   text[1].compression = PNG_TEXT_COMPRESSION_NONE;

   for (i = 2; i < *n_text; i++) {
      const gchar *key, *value;
      if (gimv_image_saver_get_comment (saver, i - 2, &key, &value)) {
         text[i - 2].key         = (gchar *) key;
         text[i - 2].text        = (gchar *) value;
         text[i - 2].compression = PNG_TEXT_COMPRESSION_NONE;      
      } else {
         g_warning ("invalid saver comment length!");
         *n_text = i;
         break;
      }
   }

   return text;
}


static gboolean
save_png (GimvImageSaver *saver,
          GimvImage      *image,
          const gchar    *filename,
          const gchar    *format)
{
   FILE *handle;
   gchar *buffer;
   gboolean has_alpha;
   gint width, height, depth, rowstride;
   guchar *pixels;
   png_structp png_ptr;
   png_infop info_ptr;
   png_text *text = NULL;
   gint i, n_text;

   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), FALSE);
   g_return_val_if_fail (image != NULL, FALSE);
   g_return_val_if_fail (filename != NULL, FALSE);
   g_return_val_if_fail (filename[0] != '\0', FALSE);

   handle = fopen (filename, "wb");
   if (handle == NULL)
      return FALSE;

   png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      fclose (handle);
      return FALSE;
   }

   info_ptr = png_create_info_struct (png_ptr);
   if (info_ptr == NULL) {
      png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
      fclose (handle);
      return FALSE;
   }

   if (setjmp (png_ptr->jmpbuf)) {
      png_destroy_write_struct (&png_ptr, &info_ptr);
      fclose (handle);
      return FALSE;
   }

   png_init_io (png_ptr, handle);

   has_alpha = gimv_image_has_alpha (image);
   width     = gimv_image_width (image);
   height    = gimv_image_height (image);
   depth     = gimv_image_depth (image);
   pixels    = gimv_image_get_pixels (image);
   rowstride = gimv_image_rowstride (image);

   png_set_IHDR (png_ptr, info_ptr, width, height,
                 depth, PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

   /* Some text to go with the png image */
   text = create_png_text (saver, filename, &n_text);
   if (text)
      png_set_text (png_ptr, info_ptr, text, n_text);

   /* Write header data */
   png_write_info (png_ptr, info_ptr);

   /* if there is no alpha in the data, allocate buffer to expand into */
   if (has_alpha)
      buffer = NULL;
   else
      buffer = g_malloc(4 * width);

   /* pump the raster data into libpng, one scan line at a time */	
   for (i = 0; i < height; i++) {
      if (has_alpha) {
         png_bytep row_pointer = pixels;
         png_write_row (png_ptr, row_pointer);
      } else {
         /* expand RGB to RGBA using an opaque alpha value */
         gint x;
         gchar *buffer_ptr = buffer;
         gchar *source_ptr = pixels;
         for (x = 0; x < width; x++) {
            *buffer_ptr++ = *source_ptr++;
            *buffer_ptr++ = *source_ptr++;
            *buffer_ptr++ = *source_ptr++;
            *buffer_ptr++ = 255;
         }
         png_write_row (png_ptr, (png_bytep) buffer);
      }
      pixels += rowstride;
   }

   png_write_end (png_ptr, info_ptr);
   png_destroy_write_struct (&png_ptr, &info_ptr);

   g_free (text);
   g_free (buffer);

   fclose (handle);

   return TRUE;
}
