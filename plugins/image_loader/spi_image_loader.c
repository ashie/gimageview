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

#include "spi_image_loader.h"

#ifdef ENABLE_SPI

#include <string.h>

#include "spi.h"
#include "gimv_plugin.h"
#include "prefs_spi.h"

static GimvImageLoaderPlugin gimv_spi_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "SPI",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_LOW,
      check_type:    NULL,
      get_info:      NULL,
      loader:        spi_load_image,
   }
};

GIMV_PLUGIN_GET_IMPL(gimv_spi_loader, GIMV_PLUGIN_IMAGE_LOADER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Susie Plugin Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_prefs_ui:  gimv_prefs_ui_spi_get_page,
};
static GimvPluginInfo *this;


gchar *
g_module_check_init (GModule *module)
{
   g_module_symbol (module, "gimv_plugin_info", (gpointer) &this);
   return NULL;
}

GimvPluginInfo *
gimv_spi_plugin_get_info (void)
{
   return this;
}


#if 1

#warning FIXME!! should be integrated to Bitmap loader.

#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3

#define ToL(buffer, off)  (buffer[(off)] \
                           | buffer[(off) + 1] << 8 \
                           | buffer[(off) + 2] << 16 \
                           | buffer[(off) + 3] << 24)

typedef struct BmpBitFields_Tag
{
   gint r_mask, r_shift, r_bits;
   gint g_mask, g_shift, g_bits;
   gint b_mask, b_shift, b_bits;
} BmpBitFields;


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


static gboolean
dib_read_bitmasks (guchar *buf,
                   BmpBitFields *bf)
{
   bf->r_mask = ToL (buf, 0);
   bf->g_mask = ToL (buf, 4);
   bf->b_mask = ToL (buf, 8);

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


static gboolean
dib_read_color_map (guchar *rgb,
                    guchar buffer[256][3],
                    gint number,
                    gint size,
                    gint *grey)
{
   gint i;
	
   *grey = (number > 2);
   for (i = 0; i < number ; i++) {
      if (size == 4) {
         buffer[i][0] = rgb[i * size + 2];
         buffer[i][1] = rgb[i * size + 1];
         buffer[i][2] = rgb[i * size];
      } else {
         buffer[i][0] = rgb[i * size + 1];
         buffer[i][1] = rgb[i * size];
         buffer[i][2] = rgb[i * size + 2];
      }
      *grey = ((*grey) && (rgb[0] == rgb[1]) && (rgb[1] == rgb[2]));
   }
   return TRUE;
}


static GimvImage *
dib_read_image (guchar *image,
                BITMAPINFOHEADER *bih,
                guchar cmap[256][3],
                BmpBitFields *bf,
                gint bytes_per_line,
                gint grey)
{
   GimvImage *gimv_image;
   gint ystart = 0;
   guchar *data, *src, *dest;

   ystart = bih->biHeight - 1;

   data = g_malloc (sizeof (guchar) * bih->biWidth * bih->biHeight * 3);

   if (bih->biBitCount == 32) {
      g_print ("32bit DIB image is not supported yet\n");

   } else if (bih->biBitCount == 24) {
      gint i, j;

      for (j = ystart; j >= 0; j--) {
         for (i = 0; i < bih->biWidth; i++) {
            src  = image + (i * 3) + (bytes_per_line * j);
            dest = data  + (i * 3) + (bih->biWidth * 3 * (bih->biHeight - 1 - j));
            dest[0] = src[2];
            dest[1] = src[1];
            dest[2] = src[0];
         }
      }

   } else if (bih->biBitCount == 16) {
      gint xpos = 0, ypos = ystart;

      while (ypos >= 0) {
         src  = image + (xpos * 2) + (bytes_per_line * ypos);
         dest = data + (xpos++ * 3) + (bih->biWidth * ypos * 3);

         dest[0] = (src[1] << 1) & 0xf8;
         dest[1] = ((src[1] >> 5) & 0x04)
            | ((src[1] << 6) & 0xc0)
            | ((src[0] >> 2) & 0x38);
         dest[2] = (src[0] << 3) & 0xf8;

         if (xpos == bih->biWidth) {
            ypos--;
            xpos = 0;
         }
      }

   } else {
      gint xpos = 0, ypos = ystart, i, pix;

      switch (bih->biCompression) {
      case BI_RGB:   /* uncompressed */
         while (ypos >= 0) {
            src  = image + xpos + (bytes_per_line * ypos);
            for (i = 1; (i <= (8 / bih->biBitCount)) && (xpos <= bih->biWidth); i++) {
               dest = data  + (xpos * 3) + (bih->biWidth * 3 * (bih->biHeight - 1 - ypos));
               pix = (*src & (((1 << bih->biBitCount) - 1)
                              << (8 - (i * bih->biBitCount))))
                                  >> (8 - (i * bih->biBitCount));
               dest[0] = cmap[pix][0];
               dest[1] = cmap[pix][1];
               dest[2] = cmap[pix][2];

               xpos++;
            }
            if (xpos >= bih->biWidth) {
               ypos--;
               xpos = 0;
            }
         }
         break;

      default:   /* Compressed images */
         g_print ("RLE compressed DIB image is not supported yet.\n");
         break;
      }
   }

   gimv_image = gimv_image_create_from_data (data,
                                             bih->biWidth, bih->biHeight,
                                             FALSE);

   return gimv_image;
}


static GimvImage *
dib_load(BITMAPINFOHEADER *bih, guchar *image)
{
   gint maps = 4;
   gint cmap_size, bytes_per_line, grey;
   guchar *cmap_ptr, cmap[256][3];
   BmpBitFields bf;
   gboolean success;
   GimvImage *gimv_image;

   if (bih->biBitCount == 4)
      cmap_size = 16;
   else if (bih->biBitCount == 8)
      cmap_size = 256;
   else
      cmap_size = 0;

   if ((bih->biClrUsed == 0) && (bih->biBitCount < 24))
      bih->biClrUsed = cmap_size;

   if (bih->biBitCount == 32 || bih->biBitCount == 24 || bih->biBitCount == 16) {
      bytes_per_line = bih->biSizeImage / bih->biHeight;
   } else {
      bytes_per_line = (bih->biSizeImage / bih->biHeight) * (8 / bih->biBitCount);
   }

   cmap_ptr = (guchar *) bih + sizeof(BITMAPINFOHEADER);

   if (bih->biCompression == BI_BITFIELDS) {
      /* BitFields compression */
      success = dib_read_bitmasks (cmap_ptr, &bf);
      if (!success) return NULL;
   } else {
      /* Get Colormap (1, 4, 8 bpp) */
      success = dib_read_color_map (cmap_ptr, cmap, cmap_size, maps, &grey);
      if (!success) return NULL;
   }

   /* read image!! */
   gimv_image = dib_read_image (image, bih,
                                cmap, &bf,
                                bytes_per_line, grey);

   return gimv_image;
}
#endif


static GimvImage *
spi_image_loader_load (SpiImportFilter *loader, const gchar *filename)
{
   guchar buf[2048], *image;
   gint result;
   FILE *file;
   PictureInfo info;
   BITMAPINFOHEADER *bih;
   gchar *description;
   GimvImage *gimv_image;

   g_return_val_if_fail (loader, NULL);
   g_return_val_if_fail (filename, NULL);

   description = ((SusiePlugin *) loader)->description;

   file = fopen (filename, "rb");
   if (!file) return NULL;

   memset(buf, 0, 2048);
   fread(buf, 2048, 1, file);

#if 0
   cygwin_conv_to_win32_path (filename, filename_w32);
#endif

   g_print ("trying to load image by spi...\n"
            "   File Name: %s\n"
            "   SPI      : %s\n",
            filename, description);
   result = loader->get_pic_info ((char *) filename, 0, 0, &info);
   if (result == SPI_SUCCESS)
      g_print ("This image type is supported by spi: %s\n", description);
   else
      goto ERROR0;

   result = loader->get_pic((char *) filename, 0, 0,
                            (HANDLE) &bih,
                            (HANDLE) &image,
                            NULL,
                            0);

   if (result != SPI_SUCCESS)
      goto ERROR0;

   g_print ("succeeded to load!\n");

   gimv_image = dib_load(bih, image);
   if (gimv_image)
      g_print ("succeeded to translate from DIB to GimvImage.\n\n");
   else
      g_print ("faild to translate from DIB to GimvImage.\n\n");

   g_free (bih);
   g_free (image);
   fclose (file);

   return gimv_image;

ERROR0:
   fclose (file);
   if (result >= 0 && result < spi_errormsg_num)
      g_print ("spi error message: %s\n\n", spi_errormsg[result]);

   return NULL;
}


GimvImage *
spi_load_image (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GList *list, *node;
   GimvImage *image = NULL;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

   list = spi_get_import_filter_list ();

   for (node = list; node; node = g_list_next (node)) {
      SpiImportFilter *loader = node->data;

      if (!loader) continue;

      image = spi_image_loader_load (loader, filename);
      if (image) break;
   }

   return image;
}

#endif
