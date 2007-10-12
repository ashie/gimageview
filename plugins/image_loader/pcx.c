/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "pcx.h"
#include "gimv_plugin.h"

typedef struct {
   guint8 manufacturer;
   guint8 version;
   guint8 compression;
   guint8 bpp;
   gint16 x1, y1;
   gint16 x2, y2;
   gint16 hdpi;
   gint16 vdpi;
   guint8 colormap[48];
   guint8 reserved;
   guint8 planes;
   gint16 bytesperline;
   gint16 color;
   guint8 filler[58];
} pcx_header_struct;

gboolean pcx_readline (GimvImageLoader *loader,
                       guchar          *buffer,
                       gint             bytes,
                       guint8           compression);


static GimvImageLoaderPlugin gimv_pcx_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "PCX",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        pcx_load,
   }
};

static const gchar *pcx_extensions[] =
{
   "pcx",
};

static GimvMimeTypeEntry pcx_mime_types[] =
{
   {
      mime_type:      "image/x-pcx",
      description:    "PCX Image",
      extensions:     pcx_extensions,
      extensions_len: sizeof (pcx_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_pcx_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(pcx_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("PCX Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


gboolean
pcx_get_header (GimvIO *gio, pcx_info *info)
{
   guint bytes_read;
   GimvIOStatus status;

   pcx_header_struct pcx_header;

   g_return_val_if_fail (gio, FALSE);

   status = gimv_io_read (gio, (gchar *) &pcx_header, 128, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return FALSE;

   if(pcx_header.manufacturer != 10) return FALSE;

   switch (pcx_header.version) {
   case 0:
   case 2:
   case 3:
   case 4:
   case 5:
      break;
   default:
      return FALSE;
   }

   if (pcx_header.compression != 1)
      return FALSE;

   switch (pcx_header.bpp) {
   case 1:
   case 2:
   case 4:
   case 8:
      break;
   default:
      return FALSE;
   }

   info->width = pcx_header.x2 - pcx_header.x1 + 1;
   info->height = pcx_header.y2 - pcx_header.y1 + 1;
   if (pcx_header.planes == 1 && pcx_header.bpp == 1) {
      info->ncolors = 1;
   } else if (pcx_header.planes == 4 && pcx_header.bpp == 1) {
      info->ncolors = 4;
   } else if (pcx_header.planes == 1 && pcx_header.bpp == 8) {
      info->ncolors = 8;
   } else if (pcx_header.planes == 3 && pcx_header.bpp == 8) {
      info->ncolors = 24;
   } else {
      return FALSE;
   }
	
   return TRUE;
}

GimvImage *
pcx_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO *gio;
   GimvImage *image;
   pcx_header_struct pcx_header;
   gint width, height, bytes, x, y, c, t1, t2, pix, ptr = 0;
   guchar *dest, *line, *line0, *line1, *line2, *line3;
   guchar cmap[768];
   guint bytes_read;
   GimvIOStatus status;

   gint bytes_count = 0, bytes_count_tmp = 0, step = 65536; /* 64kB */
   gulong total_bytes_read = 0;

   g_return_val_if_fail (loader, NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   status = gimv_io_read (gio, (gchar *) &pcx_header, 128, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL)
      return NULL;

   if(pcx_header.manufacturer != 10) return NULL;

   switch (pcx_header.version) {
   case 0:
   case 2:
   case 3:
   case 4:
   case 5:
      break;
   default:
      return NULL;
   }

   if (pcx_header.compression != 1) return NULL;

   switch (pcx_header.bpp) {
   case 1:
   case 2:
   case 4:
   case 8:
      break;
   default:
      return NULL;
   }

   if (!gimv_image_loader_progress_update (loader))
      return NULL;

   width  = pcx_header.x2 - pcx_header.x1 + 1;
   height = pcx_header.y2 - pcx_header.y1 + 1;
   bytes = pcx_header.bytesperline;

   for (; step < bytes;)
      step *= 2;

   dest = g_new0 (guchar, width * height * 3);

   if (pcx_header.planes == 1 && pcx_header.bpp == 1) {
      /* loading 1-bpp pcx */
      line = (guchar *) g_malloc (bytes * sizeof (guchar));
      for (y = 0; y < height; y++) {
         if (!pcx_readline (loader, line, bytes, pcx_header.compression)) break;

         total_bytes_read += bytes;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader)) {
               g_free (line);
               goto ERROR;
            }
         }

         for (x = 0; x < width; x++) {
            if (line[x / 8] & (128 >> (x % 8))) {
               dest[ptr++]= 0xff;
               dest[ptr++]= 0xff;
               dest[ptr++]= 0xff;
            } else {
               dest[ptr++]= 0;
               dest[ptr++]= 0;
               dest[ptr++]= 0;
            }
         }
      }
      g_free (line);

   } else if (pcx_header.planes == 4 && pcx_header.bpp == 1) {
      /* loading 4-bpp pcx */
      line0 = (guchar *) g_malloc (bytes * sizeof (guchar));
      line1 = (guchar *) g_malloc (bytes * sizeof (guchar));
      line2 = (guchar *) g_malloc (bytes * sizeof (guchar));
      line3 = (guchar *) g_malloc (bytes * sizeof (guchar));
      for (y = 0; y < height; y++) {
         if (!pcx_readline(loader, line0, bytes, pcx_header.compression)) break;
         if (!pcx_readline(loader, line1, bytes, pcx_header.compression)) break;
         if (!pcx_readline(loader, line2, bytes, pcx_header.compression)) break;
         if (!pcx_readline(loader, line3, bytes, pcx_header.compression)) break;

         total_bytes_read += bytes * 4;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader)) {
               g_free (line0);
               g_free (line1);
               g_free (line2);
               g_free (line3);
               goto ERROR;
            }
         }

         for (x = 0; x < width; x++) {
            t1 = x / 8;
            t2 = (128 >> (x % 8));
            pix = (((line0[t1] & t2) == 0) ? 0 : 1) +
               (((line1[t1] & t2) == 0) ? 0 : 2) +
               (((line2[t1] & t2) == 0) ? 0 : 4) +
               (((line3[t1] & t2) == 0) ? 0 : 8);
            pix *= 3;
            dest[ptr++] = pcx_header.colormap[pix];
            dest[ptr++] = pcx_header.colormap[pix + 1];
            dest[ptr++] = pcx_header.colormap[pix + 2];
         }
      }
      g_free (line0);
      g_free (line1);
      g_free (line2);
      g_free (line3);

   } else if (pcx_header.planes == 1 && pcx_header.bpp == 8) {
      /* loading 8-bpp pcx */
      line = (guchar *) g_malloc (bytes * sizeof (guchar));
      gimv_io_seek (gio, -768L, SEEK_END);
      status = gimv_io_read (gio, cmap, 768, &bytes_read);
      if (status != GIMV_IO_STATUS_NORMAL) {
         g_free (line);
         goto ERROR;
      }
      gimv_io_seek(gio, 128, SEEK_SET);
      for (y = 0; y < height; y++) {
         if (!pcx_readline(loader, line, bytes, pcx_header.compression)) break;

         total_bytes_read += bytes;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader)) {
               g_free (line);
               goto ERROR;
            }
         }

         for (x = 0; x < width; x++) {
            pix = line[x] * 3;
            dest[ptr++] = cmap[pix];
            dest[ptr++] = cmap[pix + 1];
            dest[ptr++] = cmap[pix + 2];
         }
      }
      g_free (line);

   } else if (pcx_header.planes == 3 && pcx_header.bpp == 8) {
      /* loading 24-bpp pcx(rgb) */
      line = (guchar *) g_malloc(bytes * 3 * sizeof (guchar));
      for (y = 0; y < height; y++) {
         for (c= 0; c < 3; c++) {
            if (!pcx_readline (loader, line, bytes, pcx_header.compression)) break;

            total_bytes_read += bytes;
            bytes_count_tmp = total_bytes_read / step;
            if (bytes_count_tmp > bytes_count) {
               bytes_count = bytes_count_tmp;
               if (!gimv_image_loader_progress_update (loader)) {
                  g_free (line);
                  goto ERROR;
               }
            }

            for (x= 0; x < width; x++) {
               ptr = x * 3 + y * width * 3 + c;
               dest[ptr] = line[x];
            }
         }
      }
      g_free (line);

   } else {
      goto ERROR;
   }

   image = gimv_image_create_from_data (dest, width, height, FALSE);

   return image;

ERROR:
   g_free (dest);
   return NULL;
}

gboolean
pcx_readline (GimvImageLoader *loader, guchar *buffer,
              gint bytes, guint8 compression)
{
   GimvIO *gio;
   guchar count = 0;
   gint value = 0;
   GimvIOStatus status;
   guint bytes_read;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   if (compression) {
      while (bytes--) {
         if (count == 0) {
            if ((value = gimv_io_getc(gio, NULL)) == EOF) return FALSE;
            if (value < 0xc0) {
               count= 1;
            } else {
               count= value - 0xc0;
               if ((value = gimv_io_getc(gio, NULL)) == EOF) return FALSE;
            }
         }
         count--;
         *(buffer++) = (guchar)value;
      }
   } else {
      status = gimv_io_read (gio, buffer, bytes, &bytes_read);
      if (status != GIMV_IO_STATUS_NORMAL)
         return FALSE;
   }

   return TRUE;
}
