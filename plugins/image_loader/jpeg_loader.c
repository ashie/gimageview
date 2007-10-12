/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * gnome-thumbnail-pixbuf-utils.c: Utilities for handling pixbufs when thumbnailing
 *
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

/*
 *  2003-05-14 Takuro Ashie <ashie@homa.ne.jp>
 *
 *      fetched from of libgnomeui-2.2.0.1
 *      (libgnomeui/gnome-thumbnail-pixbuf-utils.c) and adapt to GImageView
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */


#ifdef ENABLE_JPEG

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <setjmp.h>
#include <stdio.h>

#include <jpeglib.h>

/*
 * Workaround broken libjpeg defining these that may
 * collide w/ the ones in config.h
 */
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#include "gimv_image_loader.h"
#include "gimv_mime_types.h"
#include "gimv_plugin.h"

#define BUFFER_SIZE 16384

typedef struct
{
   struct jpeg_source_mgr pub;	/* public fields */
   GimvIO *gio;
   JOCTET buffer[BUFFER_SIZE];
   guint bytes_read;
} Source;


typedef struct
{
   struct jpeg_error_mgr pub;
   jmp_buf setjmp_buffer;
} ErrorHandlerData;


static GimvImage *jpeg_loader_load (GimvImageLoader *loader,
                                    gpointer         data);

static GimvImageLoaderPlugin gimv_jpeg_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "JPEG",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        jpeg_loader_load,
   }
};

static const gchar *jpeg_extensions[] =
{
   "jpg", "jpeg", "jpg",
};

static GimvMimeTypeEntry jpeg_mime_types[] =
{
   {
      mime_type:      "image/jpeg",
      description:    N_("The JPEG image format"),
      extensions:     jpeg_extensions,
      extensions_len: sizeof (jpeg_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL (gimv_jpeg_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(jpeg_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("JPEG Image Loader"),
   version:       "0.0.3",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
   ErrorHandlerData *data;

   data = (ErrorHandlerData *) cinfo->err;
   longjmp (data->setjmp_buffer, 1);
}


static void
output_message_handler (j_common_ptr cinfo)
{
   /* If we don't supply this handler, libjpeg reports errors
    * directly to stderr.
    */
}


static void
init_source (j_decompress_ptr cinfo)
{
}


static gboolean
fill_input_buffer (j_decompress_ptr cinfo)
{
   Source *src;
   GimvIOStatus status;
   guint nbytes;
	
   src = (Source *) cinfo->src;
   status = gimv_io_read (src->gio,
                          src->buffer,
                          sizeof (src->buffer) / sizeof (JOCTET),
                          &nbytes);

   if (status != GIMV_IO_STATUS_NORMAL || nbytes == 0) {
      /* return a fake EOI marker so we will eventually terminate */
      src->buffer[0] = (JOCTET) 0xFF;
      src->buffer[1] = (JOCTET) JPEG_EOI;
      nbytes = 2;
   }

   src->pub.next_input_byte = src->buffer;
   src->pub.bytes_in_buffer = nbytes;
   src->bytes_read += nbytes;

   return TRUE;
}


static void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
   Source *src;

   src = (Source *) cinfo->src;
   if (num_bytes > 0) {
      while (num_bytes > (long) src->pub.bytes_in_buffer) {
         num_bytes -= (long) src->pub.bytes_in_buffer;
         fill_input_buffer (cinfo);
      }
      src->pub.next_input_byte += (size_t) num_bytes;
      src->pub.bytes_in_buffer -= (size_t) num_bytes;
   }
}


static void
term_source (j_decompress_ptr cinfo)
{
}


static void
set_src (j_decompress_ptr cinfo, Source *src, GimvIO *gio)
{
   src->pub.init_source = init_source;
   src->pub.fill_input_buffer = fill_input_buffer;
   src->pub.skip_input_data = skip_input_data;
   src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
   src->pub.term_source = term_source;
   src->gio = gio;
   src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
   src->pub.next_input_byte = NULL; /* until buffer loaded */
   src->bytes_read = 0;
}


static int
calculate_divisor (int width,
                   int height,
                   int target_width,
                   int target_height)
{
   if (target_width < 0 || target_height < 0) {
      return 1;
   }
   if (width / 8 > target_width && height / 8 > target_height) {
      return 8;
   }
   if (width / 4 > target_width && height / 4 > target_height) {
      return 4;
   }
   if (width / 2 > target_width && height / 2 > target_height) {
      return 2;
   }
   return 1;
}


static void
convert_cmyk_to_rgb (struct jpeg_decompress_struct *cinfo,
                     guchar *line) 
{
   guint j;
   guchar *p;

   g_return_if_fail (cinfo != NULL);
   g_return_if_fail (cinfo->output_components == 4);
   g_return_if_fail (cinfo->out_color_space == JCS_CMYK);

   p = line;
   for (j = 0; j < cinfo->output_width; j++) {
      int c, m, y, k;
      c = p[0];
      m = p[1];
      y = p[2];
      k = p[3];
      if (cinfo->saw_Adobe_marker) {
         p[0] = k*c / 255;
         p[1] = k*m / 255;
         p[2] = k*y / 255;
      } else {
         p[0] = (255 - k)*(255 - c) / 255;
         p[1] = (255 - k)*(255 - m) / 255;
         p[2] = (255 - k)*(255 - y) / 255;
      }
      p[3] = 255;
      p += 4;
   }
}


static GimvImage *
jpeg_loader_load (GimvImageLoader *loader,
                  gpointer data)
{
   struct jpeg_decompress_struct cinfo;
   Source src;
   ErrorHandlerData jerr;
   GimvIO *gio;
   unsigned char *lines[1];
   guchar *buffer = NULL;
   guchar *pixels = NULL;
   guchar *ptr;
   int out_n_components;
   gboolean has_alpha;
   int target_width;
   int target_height;
   gboolean keep_aspect;
   gint bytes_count, bytes_count_tmp = 0;
   guint step = 65536;

   static guchar *buffer_prv = NULL;

   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   if (!gimv_image_loader_get_size_request (loader,
                                            &target_width,
                                            &target_height,
                                            &keep_aspect))
   {
      target_width  = -1;
      target_height = -1;
      keep_aspect   = TRUE;
   }

   cinfo.src = NULL;
   cinfo.err = jpeg_std_error (&jerr.pub);
   jerr.pub.error_exit = fatal_error_handler;
   jerr.pub.output_message = output_message_handler;

#warning FIXME!
   if (setjmp (jerr.setjmp_buffer)) {
      /* Handle a JPEG error. */
      jpeg_destroy_decompress (&cinfo);
      if(buffer != buffer_prv){
         buffer_prv = buffer;
         g_free (buffer);
      }
      g_free (pixels);
      return NULL;
   }

   jpeg_create_decompress (&cinfo);

   cinfo.src = &src.pub;
   set_src (&cinfo, &src, gio);

   jpeg_read_header (&cinfo, TRUE);

   /* set image info */
   if (loader->info) {
      GimvImageInfoFlags flags = GIMV_IMAGE_INFO_SYNCED_FLAG;

      gimv_image_info_set_size (loader->info,
                                cinfo.image_width, cinfo.image_height);
      gimv_image_info_set_flags (loader->info, flags);
   }

   cinfo.scale_num = 1;
   cinfo.scale_denom = calculate_divisor (cinfo.image_width,
                                          cinfo.image_height,
                                          target_width,
                                          target_height);

   if (gimv_image_loader_get_load_type (loader)
          == GIMV_IMAGE_LOADER_LOAD_THUMBNAIL)
   {
      cinfo.dct_method = JDCT_FASTEST;
      cinfo.do_fancy_upsampling = FALSE;
   }

   jpeg_calc_output_dimensions(&cinfo);

   if (cinfo.out_color_space != JCS_GRAYSCALE &&
       cinfo.out_color_space != JCS_RGB &&
       cinfo.out_color_space != JCS_CMYK)
   {
      jpeg_destroy_decompress (&cinfo);
      return NULL;
   }

   jpeg_start_decompress (&cinfo);

   if (cinfo.num_components == 1)
      out_n_components = 3;
   else
      out_n_components = cinfo.num_components;

   g_return_val_if_fail (out_n_components <= 3, NULL);

   pixels = g_malloc (cinfo.output_width * cinfo.output_height * out_n_components);

   ptr = pixels;
   if (cinfo.num_components == 1) {
      /* Allocate extra buffer for grayscale data */
      buffer = g_malloc (cinfo.output_width);
      lines[0] = buffer;
   } else {
      lines[0] = pixels;
   }

   bytes_count = 0;

   while (cinfo.output_scanline < cinfo.output_height) {
      guint i;

      jpeg_read_scanlines (&cinfo, lines, 1);

      bytes_count_tmp = ((Source *) cinfo.src)->bytes_read / step;
      if (bytes_count_tmp > bytes_count) {
         bytes_count = bytes_count_tmp;
         if (!gimv_image_loader_progress_update (loader))
            break;
      }

      if (cinfo.num_components == 1) {
         /* Convert grayscale to rgb */
         for (i = 0; i < cinfo.output_width; i++) {
            ptr[i * 3]     = buffer[i];
            ptr[i * 3 + 1] = buffer[i];
            ptr[i * 3 + 2] = buffer[i];
         }
         ptr += cinfo.output_width * 3;
      } else {
         if (cinfo.out_color_space == JCS_CMYK) {
            convert_cmyk_to_rgb (&cinfo, 
                                 lines[0]);
         }
         lines[0] += cinfo.output_width * out_n_components;
      }
   }

   buffer_prv = buffer;
   g_free (buffer);
   buffer = NULL;

   jpeg_finish_decompress (&cinfo);
   jpeg_destroy_decompress (&cinfo);

   has_alpha = cinfo.out_color_components == 4 ? TRUE : FALSE;

   return gimv_image_create_from_data (pixels,
                                       cinfo.output_width,
                                       cinfo.output_height,
                                       has_alpha);
}

#endif /* ENABLE_JPEG */
