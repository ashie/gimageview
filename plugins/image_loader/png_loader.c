/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2002 Takuro Ashie
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
#include <stdlib.h>
#include <string.h>
#include <png.h>
#ifdef PNG_SETJMP_SUPPORTED
#include <setjmp.h>
#endif /* PNG_SETJMP_SUPPORTED */

#include "png_loader.h"
#include "gimv_plugin.h"


typedef struct MyPngReadContext_Tag
{
   GimvIO *gio;
   gulong  bytes_read;
} MyPngReadContext;


static GimvImageLoaderPlugin gimv_png_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "PNG",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        gimv_png_load,
   }
};

static const gchar *png_extensions[] = {
   "png",
};

static GimvMimeTypeEntry png_mime_types[] =
{
   {
      mime_type:      "image/png",
      description:    "PNG Image",
      extensions:     png_extensions,
      extensions_len: sizeof (png_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_png_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(png_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("PNG Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};

static void
my_png_read (png_structp png_ptr, png_bytep data, png_uint_32 len)
{
   guint bytes_read;
   MyPngReadContext *context = png_get_io_ptr(png_ptr);

   gimv_io_read (context->gio, data, len, &bytes_read);
   context->bytes_read += bytes_read;
}


static gboolean
gimv_png_check_type (GimvIO *gio)
{
   guchar buf[16];
   size_t bytes_read;
   long pos;

   g_return_val_if_fail (gio, FALSE);

   gimv_io_tell (gio, &pos);

   gimv_io_read (gio, buf, 8, &bytes_read);
   if (bytes_read != 8) return FALSE;

   if (!(buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G'))
      return FALSE;

   if (buf[4] != 0x0d || buf[5] != 0x0a || buf[6] != 0x1a || buf[7] != 0x0a)
      return FALSE;

   gimv_io_seek (gio, pos, SEEK_SET);

   return TRUE;
}



static gboolean
setup_png_transformations(png_structp png_read_ptr,
                          png_infop png_info_ptr,
                          gint *width_p,
                          gint *height_p,
                          gint *color_type_p,
                          gint *num_passes)
{
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type, compression_type, filter_type;
   int channels;
 
   g_return_val_if_fail (png_read_ptr && png_info_ptr, FALSE);
   g_return_val_if_fail (width_p && height_p, FALSE);
   g_return_val_if_fail (num_passes, FALSE);

   png_get_IHDR (png_read_ptr, png_info_ptr,
                 &width, &height,
                 &bit_depth,
                 &color_type,
                 &interlace_type,
                 &compression_type,
                 &filter_type);

   *num_passes = 1;

   if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8) {
      /* Convert indexed images to RGB */
      png_set_expand (png_read_ptr);

   } else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
      /* Convert grayscale to RGB */
      png_set_expand (png_read_ptr);

   } else if (png_get_valid (png_read_ptr, 
                             png_info_ptr, PNG_INFO_tRNS)) {
      /* If we have transparency header, convert it to alpha channel */
      png_set_expand(png_read_ptr);

   } else if (bit_depth < 8) {
      /* If we have < 8 scale it up to 8 */
      png_set_expand(png_read_ptr);
   }

   /* If we are 16-bit, convert to 8-bit */
   if (bit_depth == 16) {
      png_set_strip_16(png_read_ptr);
   }

   /* If gray scale, convert to RGB */
   if (color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
   {
      png_set_gray_to_rgb(png_read_ptr);
   }

   /* If interlaced, handle that */
   if (interlace_type != PNG_INTERLACE_NONE) {
      *num_passes = png_set_interlace_handling(png_read_ptr);
   }

   /* If alpha channel is not supported by GImageView, strip it. */
   if (!gimv_image_can_alpha(NULL) && (color_type & PNG_COLOR_MASK_ALPHA)) {
      png_set_strip_alpha(png_read_ptr);
   }

   /* Update the info the reflect our transformations */
   png_read_update_info(png_read_ptr, png_info_ptr);

   png_get_IHDR (png_read_ptr, png_info_ptr,
                 &width, &height,
                 &bit_depth,
                 &color_type,
                 &interlace_type,
                 &compression_type,
                 &filter_type);

   *width_p      = (gint) width;
   *height_p     = (gint) height;
   *color_type_p = color_type;

   if (bit_depth != 8) {
      g_warning("Bits per channel of transformed PNG is %d, not 8.",
                bit_depth);
      return FALSE;
   }

   if (!(color_type == PNG_COLOR_TYPE_RGB ||
         color_type == PNG_COLOR_TYPE_RGB_ALPHA) )
   {
      g_warning("Transformed PNG not RGB or RGBA.");
      return FALSE;
   }

   channels = png_get_channels(png_read_ptr, png_info_ptr);
   if (!(channels == 3 || channels == 4) ) {
      g_warning("Transformed PNG has %d channels, must be 3 or 4.", channels);
      return FALSE;
   }

   return TRUE;
}


GimvImage *
gimv_png_load (GimvImageLoader *loader, gpointer data)
{
   MyPngReadContext context;
   GimvIO *gio;
   GimvImage *image;
   png_structp png_ptr;
   png_infop png_info;
   png_text *text;
   guchar *pixels;
   gboolean success, alpha;
   gint width, height, ctype, bpp, num_passes, i;
   gint bytes_count = 0, bytes_count_tmp = 0, step = 65536;
   gint n_text = 0;

   /* checks */
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   if (!gimv_png_check_type (gio)) return NULL;

   /* setup libpng */
   png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (!png_ptr) return NULL;

   png_info = png_create_info_struct (png_ptr);
   if (!png_info) {
      png_destroy_read_struct (&png_ptr, NULL, NULL);
      return NULL;
   }

#ifdef PNG_SETJMP_SUPPORTED
   if (setjmp (png_jmpbuf (png_ptr))) goto ERROR;
#endif /* PNG_SETJMP_SUPPORTED */

   context.gio = gio;
   context.bytes_read = 0;
   png_set_read_fn (png_ptr, &context, (png_rw_ptr) my_png_read);

   /* read header */
   png_read_info (png_ptr, png_info);
   success = setup_png_transformations (png_ptr, png_info,
                                        &width, &height, &ctype,
                                        &num_passes);
   if (!success) goto ERROR;
   if (!gimv_image_loader_progress_update (loader)) goto ERROR;

   alpha = ctype & PNG_COLOR_MASK_ALPHA;
   bpp = alpha ? 4:3;

   /* read image */
   pixels = g_malloc0 (width * height * bpp);
   for (i = 0; i < num_passes; i++) {
      guchar *buf = pixels;
      gint j;

      for (j = 0; j < height; j++) {
         png_read_row (png_ptr, buf, NULL);
         buf += width * bpp;

         bytes_count_tmp = context.bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR1;
         }
      }
   }
   image = gimv_image_create_from_data (pixels, width, height, alpha);

   /* get comments */
   png_get_text (png_ptr, png_info, &text, &n_text);
   for (i = 0; i < n_text; i++) {
      gimv_image_add_comment (image, text[i].key, text[i].text);
   }

   png_destroy_read_struct (&png_ptr, &png_info, NULL);
   return image;

ERROR1:
   g_free (pixels);
ERROR:
   png_destroy_read_struct (&png_ptr, &png_info, NULL);
   return NULL;
}
