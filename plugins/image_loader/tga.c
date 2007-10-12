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

/*
 *  These codes are mostly taken from Gimp.
 *  Gimp Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *  Targa plugin code Copyright (C) 1997 Raphael FRANCOIS and Gordon Matzigkeit
 */

/*
 *  TGA File Format Specification, Version 2.0:
 *     <URL:ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/>
 */

#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gimv_image.h"
#include "intl.h"
#include "tga.h"
#include "gimv_plugin.h"

static gint       rle_read       (GimvImageLoader *loader,
                                  guchar          *buffer,
                                  tga_info        *info);
static void       flip_line      (guchar          *buffer,
                                  tga_info        *info);
static void       upsample       (guchar          *dest,
                                  guchar          *src,
                                  guint            width,
                                  guint            bytes);
static void       bgr2rgb        (guchar          *dest,
                                  guchar          *src,
                                  guint            width,
                                  guint            bytes,
                                  guint            alpha);
static void       read_line      (GimvImageLoader *loader,
                                  guchar          *row,
                                  guchar          *buffer,
                                  tga_info        *info,
                                  guint            bytes);
static GimvImage *tga_read_image (GimvImageLoader *loader,
                                  tga_info        *info);


static GimvImageLoaderPlugin gimv_tga_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "TGA",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        tga_load,
   }
};

static const gchar *tga_extensions[] =
{
   "tga",
};

static GimvMimeTypeEntry tga_mime_types[] =
{
   {
      mime_type:      "image/x-tga",
      description:    "Targa Image",
      extensions:     tga_extensions,
      extensions_len: sizeof (tga_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-targa",
      description:    "Targa Image",
      extensions:     tga_extensions,
      extensions_len: sizeof (tga_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_tga_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(tga_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("TGA Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


/* TRUEVISION-XFILE magic signature string */
static guchar magic[18] =
{
   0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f,
   0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x0
};


GimvImage *
tga_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO      *gio;
   tga_info     info;
   guchar       header[18];
   guchar       footer[26];
   guchar       extension[495];
   GimvImage   *image;
   glong        offset;
   GimvIOStatus status;
   guint        bytes_read;

   g_return_val_if_fail (loader, NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   status = gimv_io_seek (gio, -26L, SEEK_END);
   if (status == GIMV_IO_STATUS_NORMAL) { /* Is file big enough for a footer? */
      status = gimv_io_read (gio, footer, sizeof (footer), &bytes_read);
      if (status != GIMV_IO_STATUS_NORMAL) {
         return NULL;
      } else if (memcmp (footer + 8, magic, sizeof (magic)) == 0) {

         /* Check the signature. */
         offset= footer[0] + (footer[1] * 256) + (footer[2] * 65536)
            + (footer[3] * 16777216);

         status = gimv_io_seek (gio, offset, SEEK_SET);
         if (status != GIMV_IO_STATUS_NORMAL) return NULL;
         status = gimv_io_read (gio, extension, sizeof (extension), &bytes_read);
         if (status != GIMV_IO_STATUS_NORMAL) return NULL;

         /* Eventually actually handle version 2 TGA here */
      }
   }

   status = gimv_io_seek (gio, 0, SEEK_SET);
   if (status != GIMV_IO_STATUS_NORMAL) return NULL;
   status = gimv_io_read (gio, header, sizeof (header), &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return NULL;

   switch (header[2]) {
   case 1:
      info.imageType = TGA_TYPE_MAPPED;
      info.imageCompression = TGA_COMP_NONE;
      break;
   case 2:
      info.imageType = TGA_TYPE_COLOR;
      info.imageCompression = TGA_COMP_NONE;
      break;
   case 3:
      info.imageType = TGA_TYPE_GRAY;
      info.imageCompression = TGA_COMP_NONE;
      break;

   case 9:
      info.imageType = TGA_TYPE_MAPPED;
      info.imageCompression = TGA_COMP_RLE;
      break;
   case 10:
      info.imageType = TGA_TYPE_COLOR;
      info.imageCompression = TGA_COMP_RLE;
      break;
   case 11:
      info.imageType = TGA_TYPE_GRAY;
      info.imageCompression = TGA_COMP_RLE;
      break;

   default:
      info.imageType = 0;
   }

   info.idLength     = header[0];
   info.colorMapType = header[1];

   info.colorMapIndex  = header[3] + header[4] * 256;
   info.colorMapLength = header[5] + header[6] * 256;
   info.colorMapSize   = header[7];

   info.xOrigin = header[8] + header[9] * 256;
   info.yOrigin = header[10] + header[11] * 256;
   info.width   = header[12] + header[13] * 256;
   info.height  = header[14] + header[15] * 256;
  
   info.bpp       = header[16];
   info.bytes     = (info.bpp + 7) / 8;
   info.alphaBits = header[17] & 0x0f; /* Just the low 4 bits */
   info.flipHoriz = (header[17] & 0x10) ? 1 : 0;
   info.flipVert  = (header[17] & 0x20) ? 0 : 1;

   switch (info.imageType) {
   case TGA_TYPE_MAPPED:
      if (info.bpp != 8) return NULL;
      break;

   case TGA_TYPE_COLOR:
      if (info.alphaBits == 0 && info.bpp == 32) {
         info.alphaBits = 8;
      }
      if (info.bpp != 16 && info.bpp != 24
          && (info.alphaBits != 8 || info.bpp != 32))
      {
         return NULL;
      }
      break;

   case TGA_TYPE_GRAY:
      if (info.alphaBits == 0 && info.bpp == 16) {
         info.alphaBits = 8;
      }
      if (info.bpp != 8 && (info.alphaBits != 8 || info.bpp != 16)) {
         return NULL;
      }
      break;

   default:
      return NULL;
   }

   /* Plausible but unhandled formats */
   if (info.bytes * 8 != info.bpp) return NULL;

   /* Check that we have a color map only when we need it. */
   if (info.imageType == TGA_TYPE_MAPPED && info.colorMapType != 1) {
      return NULL;
   } else if (info.imageType != TGA_TYPE_MAPPED && info.colorMapType != 0) {
      return NULL;
   }

   /* Skip the image ID field. */
   if (info.idLength) {
      status = gimv_io_seek (gio, info.idLength, SEEK_CUR);
      if (status != GIMV_IO_STATUS_NORMAL) return NULL;
   }

   image = tga_read_image (loader, &info);

   return image;
}


static gint
rle_read (GimvImageLoader *loader,
          guchar   *buffer,
          tga_info *info)
{
   GimvIO *gio;
   static gint   repeat = 0;
   static gint   direct = 0;
   static guchar sample[4];
   gint head;
   gint x, k;
   guint bytes_read;
   GimvIOStatus status;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, EOF);

   for (x = 0; x < info->width; x++) {
      if (repeat == 0 && direct == 0) {
         head = gimv_io_getc (gio, &status);

         if (head == EOF) {
            return EOF;
         } else if (head >= 128) {
            repeat = head - 127;

            status = gimv_io_read (gio, sample, info->bytes, &bytes_read);
            if (bytes_read < 1)
               return EOF;
         } else {
            direct = head + 1;
         }
      }

      if (repeat > 0) {
         for (k = 0; k < info->bytes; ++k) {
            buffer[k] = sample[k];
         }

         repeat--;
      } else { /* direct > 0 */
         status = gimv_io_read (gio, buffer, info->bytes, &bytes_read);
         if (bytes_read < 1)
            return EOF;

         direct--;
      }

      buffer += info->bytes;
   }

   return 0;
}


static void
flip_line (guchar   *buffer,
           tga_info *info)
{
   guchar  temp;
   guchar *alt;
   gint    x, s;

   alt = buffer + (info->bytes * (info->width - 1));

   for (x = 0; x * 2 <= info->width; x++) {
      for (s = 0; s < info->bytes; ++s) {
         temp = buffer[s];
         buffer[s] = alt[s];
         alt[s] = temp;
      }

      buffer += info->bytes;
      alt -= info->bytes;
   }
}


/* Some people write 16-bit RGB TGA files. The spec would probably
   allow 27-bit RGB too, for what it's worth, but I won't fix that
   unless someone actually provides an existence proof */

static void
upsample (guchar *dest,
          guchar *src,
          guint   width,
          guint   bytes)
{
   guint x;

   for (x = 0; x < width; x++) {
      dest[0] =  ((src[1] << 1) & 0xf8);
      dest[0] += (dest[0] >> 5);

      dest[1] =  ((src[0] & 0xe0) >> 2) + ((src[1] & 0x03) << 6);
      dest[1] += (dest[1] >> 5);

      dest[2] =  ((src[0] << 3) & 0xf8);
      dest[2] += (dest[2] >> 5);

      dest += 3;
      src += bytes;
   }
}


static void
bgr2rgb (guchar *dest,
         guchar *src,
         guint   width,
         guint   bytes,
         guint   alpha)
{
   guint x;

   if (alpha) {
      for (x = 0; x < width; x++) {
         *(dest++) = src[2];
         *(dest++) = src[1];
         *(dest++) = src[0];
         *(dest++) = src[3];

         src += bytes;
      }
   } else {
      for (x = 0; x < width; x++) {
         *(dest++) = src[2];
         *(dest++) = src[1];
         *(dest++) = src[0];

         src += bytes;
      }
   }
}


static void
read_line (GimvImageLoader  *loader,
           guchar       *row,
           guchar       *buffer,
           tga_info     *info,
           guint         bytes)
{
   GimvIO *gio;
   GimvIOStatus status;
   guint bytes_read;

   gio = gimv_image_loader_get_gio (loader);
   g_return_if_fail (gio);

   if (info->imageCompression == TGA_COMP_RLE) {
      rle_read (loader, buffer, info);
   } else {
      status = gimv_io_read (gio, buffer, info->bytes * info->width, &bytes_read);
   }

   if (info->flipHoriz) {
      flip_line (buffer, info);
   }

   if (info->imageType == TGA_TYPE_COLOR) {
      if (info->bpp == 16) {
         upsample (row, buffer, info->width, info->bytes);
      } else {
         bgr2rgb (row, buffer, info->width, info->bytes, info->alphaBits);
      }
   } else {
      memcpy (row, buffer, info->width * bytes);
   }
}


static GimvImage *
tga_read_image (GimvImageLoader *loader, tga_info *info)
{
   GimvIO *gio;
   GimvImage *image;
   guchar *data, *buffer, *row;
   gint i, y;
   gboolean alpha = FALSE;

   gint max_tileheight, tileheight;
   gint bytes;

   guint  cmap_bytes;
   guchar tga_cmap[4 * 256], cmap[3 * 256];

   GimvIOStatus status;
   guint bytes_read;

   gint bytes_count = 0, bytes_count_tmp = 0, step = 65536;
   glong pos;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, NULL);

   /* Handle colormap */
   if (info->colorMapType == 1) {
      cmap_bytes= (info->colorMapSize + 7 ) / 8;

      if (cmap_bytes > 4) return NULL;

      status = gimv_io_read (gio, tga_cmap, info->colorMapLength * cmap_bytes, &bytes_read);

      if (status == GIMV_IO_STATUS_NORMAL) {
         if (info->colorMapSize == 32)
            bgr2rgb(cmap, tga_cmap, info->colorMapLength, cmap_bytes, 1);
         else if (info->colorMapSize == 24)
            bgr2rgb(cmap, tga_cmap, info->colorMapLength, cmap_bytes, 0);
         else if (info->colorMapSize == 16)
            upsample(cmap, tga_cmap, info->colorMapLength, cmap_bytes);

      } else {
         return NULL;
      }
   }

   if (!gimv_image_loader_progress_update (loader)) return NULL;

   /* Allocate the data. */
   /*
    *  FIXME!! When this loader is implement as plugin,
    *  it shuold be supported progressive loading correctly.
    */
   max_tileheight = info->height;
   if (gimv_image_can_alpha (NULL) && info->bytes == 4) {
      bytes = 4;
      alpha = TRUE;
   } else {
      bytes = 3;
   }
   data = (guchar *) g_malloc (info->width * max_tileheight * bytes);
   buffer = (guchar *) g_malloc (info->width * info->bytes);

   if (info->flipVert) {
      for (i = 0; i < info->height; i += tileheight) {
         tileheight = i ? max_tileheight : (info->height % max_tileheight);
         if (tileheight == 0) tileheight = max_tileheight;

         for (y = 1; y <= tileheight; ++y) {
            row = data + (info->width * bytes * (tileheight - y));
            read_line(loader, row, buffer, info, bytes);

            gimv_io_tell (gio, &pos);
            bytes_count_tmp = pos / step;
            if (bytes_count_tmp > bytes_count) {
               bytes_count = bytes_count_tmp;
               if (!gimv_image_loader_progress_update (loader)) goto ERROR;
            }
         }
      }
   } else {
      for (i = 0; i < info->height; i += max_tileheight) {
         tileheight = MIN (max_tileheight, info->height - i);

         for (y = 0; y < tileheight; ++y) {
            row = data + (info->width * bytes * y);
            read_line(loader, row, buffer, info, bytes);

            gimv_io_tell (gio, &pos);
            bytes_count_tmp = pos / step;
            if (bytes_count_tmp > bytes_count) {
               bytes_count = bytes_count_tmp;
               if (!gimv_image_loader_progress_update (loader)) goto ERROR;
            }
         }
      }
   }

   g_free (buffer);

   image = gimv_image_create_from_data (data, info->width, info->height, alpha);

   return image;

ERROR:
   g_free (buffer);
   g_free (data);
   return NULL;
}
