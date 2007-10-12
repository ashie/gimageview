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
 *  These codes are mostly taken from GTK See.
 *  Bitfields compression, 32bit, and OS2 BMP support was added.
 *  GTK See Copyright (C) 1998 Hotaru Lee <jkhotaru@mail.sti.com.cn> <hotaru@163.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "bmp.h"
#include "gimv_plugin.h"

#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))
#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define ToL(buffer, off)  (buffer[(off)] \
                           | buffer[(off) + 1] << 8 \
                           | buffer[(off) + 2] << 16 \
                           | buffer[(off) + 3] << 24)
#define ToS(buffer, off)  (buffer[(off)] \
                           | buffer[(off) + 1] << 8)


typedef struct BmpBitFields_Tag
{
   gint r_mask, r_shift, r_bits;
   gint g_mask, g_shift, g_bits;
   gint b_mask, b_shift, b_bits;
} BmpBitFields;


gboolean bmp_read_color_map (GimvIO *gio,
                             guchar buffer[256][3],
                             gint number,
                             gint size,
                             gint *grey);
gboolean bmp_read_bitmasks  (GimvIO *gio,
                             BmpBitFields *bf);
guchar  *bmp_read_image     (GimvImageLoader *loader,
                             gint len,
                             gint height,
                             guchar cmap[256][3],
                             BmpBitFields *bf,
                             gint ncols,
                             gint bpp,
                             gint compression,
                             gint spzeile,
                             gint grey);


static GimvImageLoaderPlugin gimv_bmp_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "BMP",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        bmp_load,
   }
};

static const gchar *bmp_extensions[] =
{
   "bmp",
};

static GimvMimeTypeEntry bmp_mime_types[] =
{
   {
      mime_type:      "image/bmp",
      description:    N_("Windows Bitmap Image"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-bmp",
      description:    N_("Windows Bitmap Image"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-MS-bmp",
      description:    N_("Windows Bitmap Image"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL (gimv_bmp_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(bmp_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Windows Bitmap Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


gboolean
bmp_get_header(GimvIO   *gio,
               bmp_info *info)
{
   guchar buffer[50];
   gulong bfSize, reserverd, bfOffs, biSize;
   GimvIOStatus status;
   guint bytes_read;
	
   g_return_val_if_fail (gio, FALSE);

   /*
    *   ID (2byte)
    *      BM - Windows
    *      BA - OS2
    */
   status = gimv_io_read (gio, buffer, 2, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return FALSE;
   if (bytes_read != 2) return FALSE;

   if (!(!strncmp(buffer, "BM", 2) || !strncmp(buffer, "BA", 2)))
   {
      return FALSE;
   }
	
   status = gimv_io_read (gio, buffer, 16, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return FALSE;
   if (bytes_read != 16) return FALSE;

   bfSize    = ToL(buffer, 0);           /* File Size          (1dword) */
                                         /*    Windows      bfSize = 40 */
                                         /*    OS2          bfSize = 12 */
   reserverd = ToL(buffer, 4);           /* Reserved           (1dword) */
   bfOffs    = ToL(buffer, 8);           /* Bitmap Data Offset (1dword) */
   biSize    = ToL(buffer, 12);          /* Bitmap Header Size (1dword) */

   if (!(biSize == 40 || biSize == 12)) return FALSE;
	
   status = gimv_io_read (gio, buffer, 36, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return FALSE;
   if (bytes_read != 36) return FALSE;

   /* both Windows and OS2 */
   info -> sizeHeader = biSize;
   if (biSize == 40) {
      info -> width   = ToL(buffer, 0x00);  /* width    (Windows - 1dword) */
      info -> height  = ToL(buffer, 0x04);  /* height   (Windows - 1dword) */
   } else if (biSize == 12) {
      info -> width  = ToS(buffer, 0x00);  /*          (OS2     - 1 word) */
      info -> height = ToS(buffer, 0x04);  /*          (OS2     - 1 word) */
   }
   info -> planes  = ToS(buffer, 0x08);     /* Planes             (1 word) */
   info -> bitCnt  = ToS(buffer, 0x0A);     /* Bits Per Pixels    (1 word) */

   /* Windows only */
   if (biSize == 40) {
      info -> compr   = ToL(buffer, 0x0C);  /* Compression        (1dword) */
                                            /*    0 - RGB
                                             *    1 - RLE8bit
                                             *    2 - RLE4bit
                                             *    3 - bitfields
                                             */
      info -> sizeIm  = ToL(buffer, 0x10);  /* Bitmap Data Size   (1dword) */
      info -> xPels   = ToL(buffer, 0x14);  /* HResolution        (1dword) */
      info -> yPels   = ToL(buffer, 0x18);  /* VResolution        (1dword) */
      info -> clrUsed = ToL(buffer, 0x1C);  /* Colors             (1dword) */
      info -> clrImp  = ToL(buffer, 0x20);  /* Important Colors   (1dword) */
                                            /* Palette Table      (Palette num * 4byte) */
                                            /* Bitmap Data ... */
   }else {
      info->compr = 0;
   }

   if (info -> bitCnt > 24) return FALSE;

   return TRUE;
}

GimvImage *
bmp_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO *gio;
   GimvImage *image;
   guchar buffer[50];
   gulong bfSize, reserverd, bfOffs, biSize;
   gint ColormapSize, SpeicherZeile, Maps, Grey;
   guchar ColorMap[256][3];
   guchar *dest;
   bmp_info cinfo;
   BmpBitFields bf;
   GimvIOStatus status;
   guint bytes_read;
   gboolean has_alpha = FALSE;

   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   status = gimv_io_read (gio, buffer, 2, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return NULL;
   if (bytes_read != 2) return NULL;

   if (!(!strncmp (buffer, "BM", 2) || !strncmp (buffer, "BA", 2)))
      return NULL;

   status = gimv_io_read (gio, buffer, 16, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return NULL;
   if (!bytes_read != 16) return NULL;

   bfSize    = ToL (buffer, 0);
   reserverd = ToL (buffer, 4);
   bfOffs    = ToL (buffer, 8);
   biSize    = ToL (buffer, 12);

   if (biSize != 40 && biSize != 12) {
      return NULL;
   } else {
      cinfo.sizeHeader = biSize;
   }
	
   status = gimv_io_read (gio, buffer, biSize - 4, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return NULL;
   if (bytes_read != biSize - 4) return NULL;

   if (biSize == 40) {          /* Windows */
      cinfo.width   = ToL (buffer, 0x00);
      cinfo.height  = ToL (buffer, 0x04);
      cinfo.planes  = ToS (buffer, 0x08);
      cinfo.bitCnt  = ToS (buffer, 0x0A);
      cinfo.compr   = ToL (buffer, 0x0C);
      cinfo.sizeIm  = ToL (buffer, 0x10);
      cinfo.xPels   = ToL (buffer, 0x14);
      cinfo.yPels   = ToL (buffer, 0x18);
      cinfo.clrUsed = ToL (buffer, 0x1C);
   } else if (biSize == 12) {   /* OS2 */
      cinfo.width   = ToS (buffer, 0x00);
      cinfo.height  = ToS (buffer, 0x02);
      cinfo.planes  = ToS (buffer, 0x04);
      cinfo.bitCnt  = ToS (buffer, 0x06);
      cinfo.compr   = 0;
   }

   if (cinfo.bitCnt > 32) {
      return NULL;
   }

   Maps = 4;
   ColormapSize = (bfOffs - biSize - 14) / Maps;
   if ((cinfo.clrUsed == 0) && (cinfo.bitCnt < 24))
      cinfo.clrUsed = ColormapSize;
   if (cinfo.bitCnt == 32 || cinfo.bitCnt == 24 || cinfo.bitCnt == 16) {
      SpeicherZeile = ((bfSize - bfOffs) / cinfo.height);
   } else {
      SpeicherZeile = ((bfSize - bfOffs) / cinfo.height) * (8 / cinfo.bitCnt);
   }

   if (cinfo.compr == BI_BITFIELDS) {   /* BitFields compression */
      if (!bmp_read_bitmasks (gio, &bf)) {
         return NULL;
      }
   } else {                            /* Get Colormap (1, 4, 8 bpp) */
      if (!bmp_read_color_map (gio, ColorMap, ColormapSize, Maps, &Grey)) {
         return NULL;
      }
   }

   if (!gimv_image_loader_progress_update (loader)) return NULL;

   /* read image!! */
   dest = bmp_read_image(loader, cinfo.width, cinfo.height, ColorMap, &bf,
                         cinfo.clrUsed, cinfo.bitCnt, cinfo.compr,
                         SpeicherZeile, Grey);
   if (!dest) {
      return NULL;
   }

   if (cinfo.bitCnt == 32 && gimv_image_can_alpha (NULL))
      has_alpha = TRUE;

   image = gimv_image_create_from_data (dest, cinfo.width, cinfo.height, has_alpha);

   return image;
}


gboolean
bmp_read_color_map (GimvIO *gio,
                    guchar buffer[256][3],
                    gint number,
                    gint size,
                    gint *grey)
{
   gint i;
   guchar rgb[4];
   GimvIOStatus status;
   guint bytes_read;

   *grey = (number > 2);
   for (i = 0; i < number ; i++) {
      status = gimv_io_read (gio, rgb, size, &bytes_read);
      if (status != GIMV_IO_STATUS_NORMAL) return FALSE;
      if (bytes_read != (guint) size) return FALSE;

      if (size == 4) {      
         buffer[i][0] = rgb[2];
         buffer[i][1] = rgb[1];
         buffer[i][2] = rgb[0];
      } else {
         buffer[i][0] = rgb[1];
         buffer[i][1] = rgb[0];
         buffer[i][2] = rgb[2];
      }
      *grey = ((*grey) && (rgb[0] == rgb[1]) && (rgb[1] == rgb[2]));
   }
   return TRUE;
}


static void
find_bits (gint n, gint *lowest, gint *n_set)
{
   gint i;

   *n_set = 0;

   for (i = 31; i >= 0; i--) {
      if (n & (1 << i)) {
         *lowest = i;
         (*n_set)++;
      }
   }
}


gboolean
bmp_read_bitmasks (GimvIO *gio,
                   BmpBitFields *bf)
{
   guchar buffer[12];
   GimvIOStatus status;
   guint bytes_read;

   status = gimv_io_read (gio, buffer, 12, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) return FALSE;
   if (bytes_read != 12) return FALSE;

   bf->r_mask = ToL (buffer, 0);
   bf->g_mask = ToL (buffer, 4);
   bf->b_mask = ToL (buffer, 8);

   find_bits (bf->r_mask, &bf->r_shift, &bf->r_bits);
   find_bits (bf->g_mask, &bf->g_shift, &bf->g_bits);
   find_bits (bf->b_mask, &bf->b_shift, &bf->b_bits);

   if (bf->r_bits == 0 || bf->g_bits == 0 || bf->b_bits == 0) {
      bf->r_mask = 0x7c00;
      bf->r_shift = 10;
      bf->g_mask = 0x03e0;
      bf->g_shift = 5;
      bf->b_mask = 0x001f;
      bf->b_shift = 0;

      bf->r_bits = bf->g_bits = bf->b_bits = 5;
   }

   return TRUE;
}


/* FIXME!! Too long ;-P */
guchar *
bmp_read_image (GimvImageLoader *loader,
                gint len,
                gint height,
                guchar cmap[256][3],
                BmpBitFields *bf,
                gint ncols,
                gint bpp,
                gint compression,
                gint spzeile,
                gint grey)
{
   GimvIO *gio;
   guchar v, wieviel;
   guchar buf[16];
   gint xpos = 0, ypos = 0;
   guchar *dest, *temp;
   gint i, j, pix, bytes_count = 0, bytes_count_tmp = 0, bytes = 3;
   GimvIOStatus status;
   guint bytes_read, step = 65536;  /* 64kB */
   gulong total_bytes_read = 0;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, NULL);

   ypos = height - 1;	/* Bitmaps begin in the lower left corner */
                        /* FIXME!! If height is negative value, it will not work correctly. */

   if (bpp == 32 && gimv_image_can_alpha (NULL))
      bytes = 4;

   dest = g_malloc (sizeof (guchar) * len * height * bytes);

   if (bpp == 32 && compression == BI_BITFIELDS) {
      gint r_lshift, r_rshift;
      gint g_lshift, g_rshift;
      gint b_lshift, b_rshift;

      r_lshift = 8 - bf->r_bits;
      g_lshift = 8 - bf->g_bits;
      b_lshift = 8 - bf->b_bits;

      r_rshift = bf->r_bits - r_lshift;
      g_rshift = bf->g_bits - g_lshift;
      b_rshift = bf->b_bits - b_lshift;

      while (gimv_io_read (gio, buf, 4, &bytes_read) == GIMV_IO_STATUS_NORMAL) {
         gint v, r, g, b;

         if (bytes_read != 4) break;

         temp = dest + (xpos++ * bytes) + (len * ypos * bytes);

         v = buf[0] | (buf[1] << 8) | (buf[2] << 16);

         r = (v & bf->r_mask) >> bf->r_shift;
         g = (v & bf->g_mask) >> bf->g_shift;
         b = (v & bf->b_mask) >> bf->b_shift;

         temp[0] = (r << r_lshift) | (r >> r_rshift);
         temp[1] = (g << g_lshift) | (g >> g_rshift);
         temp[2] = (b << b_lshift) | (b >> b_rshift);
         if (bytes == 4)
            temp[3] = buf[3];

         if (xpos == len) {
            if (spzeile > len * 4) {
               gimv_io_seek (gio, spzeile - (len * 4), SEEK_CUR);
            }
            ypos--;
            xpos = 0;
         }

         total_bytes_read += bytes_read;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR;
         }

         if (ypos < 0) break;
      }

   } else if (bpp == 32) {
      while (gimv_io_read (gio, buf, 4, &bytes_read) == GIMV_IO_STATUS_NORMAL) {
         temp = dest + (xpos++ * bytes) + (len * ypos * bytes);

         if (bytes_read != 4) break;

         temp[0] = buf[2];
         temp[1] = buf[1];
         temp[2] = buf[0];
         if (bytes == 4)
            temp[3] = buf[3];

         if (xpos == len) {
            if (spzeile > len * 4) {
               gimv_io_seek(gio, spzeile - (len * 4), SEEK_CUR);
            }
            ypos--;
            xpos = 0;
         }

         total_bytes_read += bytes_read;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR;
         }

         if (ypos < 0) break;
      }

   } else if (bpp == 24) {
      while (gimv_io_read (gio, buf, 3, &bytes_read) == GIMV_IO_STATUS_NORMAL) {

         if (bytes_read != 3) break;

         temp = dest + (xpos++ * bytes) + (len * ypos * bytes);
         temp[0] = buf[2];
         temp[1] = buf[1];
         temp[2] = buf[0];
         if (xpos == len) {
            if (spzeile > len * 3) {
               gimv_io_seek (gio, spzeile - (len * 3), SEEK_CUR);
            }
            ypos--;
            xpos = 0;
         }

         total_bytes_read += bytes_read;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR;
         }

         if (ypos < 0) break;
      }

   } else if (bpp == 16 && compression == BI_BITFIELDS) {
      gint r_lshift, r_rshift;
      gint g_lshift, g_rshift;
      gint b_lshift, b_rshift;

      r_lshift = 8 - bf->r_bits;
      g_lshift = 8 - bf->g_bits;
      b_lshift = 8 - bf->b_bits;

      r_rshift = bf->r_bits - r_lshift;
      g_rshift = bf->g_bits - g_lshift;
      b_rshift = bf->b_bits - b_lshift;

      while (gimv_io_read (gio, buf, 2, &bytes_read) == GIMV_IO_STATUS_NORMAL) {
         gint v, r, g, b;

         if (bytes_read != 2) break;

         temp = dest + (xpos++ * bytes) + (len * ypos * bytes);

         v = (int) buf[0] | ((int) buf[1] << 8);

         r = (v & bf->r_mask) >> bf->r_shift;
         g = (v & bf->g_mask) >> bf->g_shift;
         b = (v & bf->b_mask) >> bf->b_shift;

         temp[0] = (r << r_lshift) | (r >> r_rshift);
         temp[1] = (g << g_lshift) | (g >> g_rshift);
         temp[2] = (b << b_lshift) | (b >> b_rshift);

         if (xpos == len) {
            ypos--;
            xpos = 0;
         }
 
         total_bytes_read += bytes_read;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR;
         }

         if (ypos < 0) break;
      }

   } else if (bpp == 16) {
      while (gimv_io_read (gio, buf, 2, &bytes_read) == GIMV_IO_STATUS_NORMAL) {

         if (bytes_read != 2) break;

         temp = dest + (xpos++ * bytes) + (len * ypos * bytes);

         temp[0] = (buf[1] << 1) & 0xf8;
         temp[1] = ((buf[1] >> 5) & 0x04) | ((buf[1] << 6) & 0xc0) | ((buf[0] >> 2) & 0x38);
         temp[2] = (buf[0] << 3) & 0xf8;

         if (xpos == len) {
            ypos--;
            xpos = 0;
         }

         total_bytes_read += bytes_read;
         bytes_count_tmp = total_bytes_read / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader))
               goto ERROR;
         }

         if (ypos < 0) break;
      }

   } else {
      switch(compression) {
      case BI_RGB:  	/* uncompressed */
         while (gimv_io_read (gio, &v, 1, &bytes_read) == GIMV_IO_STATUS_NORMAL) {

            if (bytes_read != 1) break;

            for (i = 1; (i <= (8 / bpp)) && (xpos < len); i++) {
               temp = dest + (xpos * bytes) + (len * ypos * bytes);
               pix = (v & (((1 << bpp) - 1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
               temp[0] = cmap[pix][0];
               temp[1] = cmap[pix][1];
               temp[2] = cmap[pix][2];
               xpos++;
            }
            if (xpos == len) {
               if (spzeile > len) {
                  gimv_io_seek (gio, (spzeile-len) / (8 / bpp), SEEK_CUR);
               }
               ypos--;
               xpos = 0;
            }

            total_bytes_read += bytes_read;
            bytes_count_tmp = total_bytes_read / step;
            if (bytes_count_tmp > bytes_count) {
               bytes_count = bytes_count_tmp;
               if (!gimv_image_loader_progress_update (loader))
                  goto ERROR;
            }

            if (ypos < 0) break;
         }
         break;

      default:	/* Compressed images */
         while (TRUE) {
            status = gimv_io_read (gio, buf, 2, &bytes_read);

            if (bytes_read != 2) break;

            total_bytes_read += bytes_read;
            bytes_count_tmp = total_bytes_read / step;
            if (bytes_count_tmp > bytes_count) {
               bytes_count = bytes_count_tmp;
               if (!gimv_image_loader_progress_update (loader))
                  goto ERROR;
            }

            if ((guchar) buf[0] != 0) {
               /* Count + Color - record */
               for (j = 0; ((guchar) j < (guchar) buf[0]) && (xpos < len);) {
                  for (i = 1;
                       ((i <= (8 / bpp)) && (xpos < len) && ((guchar) j < (guchar) buf[0]));
                       i++, j++)
                  {
                     temp = dest + (xpos * bytes) + (len * ypos * bytes);
                     pix = (buf[1] & (((1 << bpp) - 1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
                     temp[0] = cmap[pix][0];
                     temp[1] = cmap[pix][1];
                     temp[2] = cmap[pix][2];
                     xpos++;
                  }
               }
            }
            if (((guchar) buf[0] == 0) && ((guchar) buf[1] > 2)) {
               /* uncompressed record */
               wieviel = buf[1];
               for (j = 0; j < wieviel; j += (8 / bpp)) {
                  status = gimv_io_read (gio, &v, 1, &bytes_read);

                  if (bytes_read != 1) break;

                  total_bytes_read += bytes_read;
                  bytes_count_tmp = total_bytes_read / step;
                  if (bytes_count_tmp > bytes_count) {
                     bytes_count = bytes_count_tmp;
                     if (!gimv_image_loader_progress_update (loader))
                        goto ERROR;
                  }

                  i=1;
                  while ((i <= (8 / bpp)) && (xpos < len)) {
                     temp = dest + (xpos * bytes) + (len * ypos * bytes);
                     pix = (v & (((1 << bpp) - 1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
                     temp[0] = cmap[pix][0];
                     temp[1] = cmap[pix][1];
                     temp[2] = cmap[pix][2];
                     i++;
                     xpos++;
                  }
               }
               if ((wieviel / (8 / bpp)) % 2)
                  status = gimv_io_read (gio, &v, 1, &bytes_read);
               /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
               if (bytes_read != 1) break;

               total_bytes_read += bytes_read;
               bytes_count_tmp = total_bytes_read / step;
               if (bytes_count_tmp > bytes_count) {
                  bytes_count = bytes_count_tmp;
                  if (!gimv_image_loader_progress_update (loader))
                     goto ERROR;
               }
            }
            if (((guchar) buf[0] == 0) && ((guchar) buf[1] == 0)) {   /* Zeilenende */
               ypos--;
               xpos = 0;
            }
            if (((guchar) buf[0] == 0) && ((guchar) buf[1] == 1)) {   /* Bitmapende */
               break;
            }
            if (((guchar) buf[0] == 0) && ((guchar) buf[1] == 2)) {   /* Deltarecord */
               xpos += (guchar) buf[2];
               ypos += (guchar) buf[3];
            }
         }
         break;
      }
   }
   
   return dest;

ERROR:
   g_free(dest);
   return NULL;
}
