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
 *  These codes are mostly taken from Enfle.
 *  Enfle Copyright (C) 1998 Hiroshi Takekawa
 *
 *  See also http://www.jisyo.com/viewer/faq/mag_tech.htm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "mag.h"
#include "gimv_plugin.h"

#define LINE200  0x01
#define COLOR8   0x02
#define DIGITAL  0x04
#define COLOR256 0xf0

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))
#define ToL(buffer, off)  (buffer[(off)] | buffer[(off)+1]<<8 | buffer[(off)+2]<<16 | buffer[(off)+3]<<24)
#define ToS(buffer, off)  (buffer[(off)] | buffer[(off)+1]<<8);

static guchar *mag_decode_image (GimvImageLoader *loader,
                                 mag_info        *minfo,
                                 guchar           colormap[256][3]);


static GimvImageLoaderPlugin gimv_mag_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "MAG",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        mag_load,
   }
};

static const gchar *mag_extensions[] =
{
   "mag",
};

static GimvMimeTypeEntry mag_mime_types[] =
{
   {
      mime_type:      "image/x-mag",
      description:    "MAG Image",
      extensions:     mag_extensions,
      extensions_len: sizeof (mag_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_mag_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(mag_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("MAG Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


GimvImage *
mag_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO *gio;
   GimvImage *image = NULL;
   mag_info minfo;
   guchar buffer[32], comment[BUF_SIZE], colormap[256][3], *dest;
   guint i;
   guint bytes_read;
   GimvIOStatus status;

   g_return_val_if_fail (loader, NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   status = gimv_io_read (gio, buffer, 8, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL
       || !(!strncmp (buffer, "MAKI02  ", 8))) /* check data: 8byte */
   {
      return NULL;
   }

   status = gimv_io_read (gio, buffer, 4 + 18 + 2, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) {
      /* machine code:  4byte
         user name:    18byte
         unused:        2byte */
      return NULL;
   }

   i = 0;
   do {                                        /* comment: unlimited */
      status = gimv_io_read (gio, buffer, 1, &bytes_read);
      if (status != GIMV_IO_STATUS_NORMAL) return NULL;
      comment[i++] = buffer[0];
   } while (buffer[0] != '\0' && i < 4096);    /* start of header */

   gimv_io_tell (gio, &minfo.offset);
   minfo.offset--;

   status = gimv_io_read (gio, buffer, 31, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) {      /* header: 32byte (include end of comment) */
      return NULL;
   }

   minfo.mcode  = buffer[0];
   minfo.dcode  = buffer[1];
   minfo.screen = buffer[2];
   minfo.lx     = ToS (buffer, 3);
   minfo.uy     = ToS (buffer, 5);
   minfo.rx     = ToS (buffer, 7);
   minfo.dy     = ToS (buffer, 9);
   minfo.off_a  = ToL (buffer, 11);
   minfo.off_b  = ToL (buffer, 15);
   minfo.size_b = ToL (buffer, 19);
   minfo.off_p  = ToL (buffer, 23);
   minfo.size_p = ToL (buffer, 27);

   minfo.ncolors = (minfo.screen & COLOR256) ? 256 : 16;

   minfo.width  = minfo.rx - minfo.lx + 1;
   minfo.height = minfo.dy - minfo.uy + 1;
   minfo.lpad   = minfo.lx & 7;
   minfo.rpad   = 7 - (minfo.rx & 7);
   minfo.lx    -= minfo.lpad;
   minfo.rx    += minfo.rpad;


   /* read palette */
   for (i = 0; i < minfo.ncolors; i++) {
      /* ORDER: GRB */
      gimv_io_read (gio, buffer, 3, &bytes_read);
      colormap[i][1] = buffer[0]; /* G */
      colormap[i][0] = buffer[1]; /* R */
      colormap[i][2] = buffer[2]; /* B */
   }

   minfo.flagperline = minfo.width >> ((minfo.screen & COLOR256) ? 2 : 3);

   if (!gimv_image_loader_progress_update (loader))
      return NULL;

   dest = mag_decode_image (loader, &minfo, colormap);

   if (dest)
      image = gimv_image_create_from_data (dest,
                                           minfo.width, minfo.height,
                                           FALSE);

   return image;
}


static guchar *
mag_decode_image (GimvImageLoader *loader, mag_info *minfo, guchar colormap[256][3])
{
   GimvIO *gio;
   guint bytes_read;
   guint i, j, k, sa;
   gint pr, f, t, width, height;
   guchar *flaga, *flagb, *fa, *fb, *pix, *d, *dest, *src;
   guchar bitmask[] = {
      128, 64, 32, 16, 8, 4, 2, 1
   };
   gint offset[][2] = {
      { 0,  0},    {-1,  0},    {-2,  0},    {-4,  0},
      { 0, -1},    {-1, -1},    { 0, -2},    {-1, -2},
      {-2, -2},    { 0, -4},    {-1, -4},    {-2, -4},
      { 0, -8},    {-1, -8},    {-2, -8},    { 0, -16}
   };
   GimvIOStatus status;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, NULL);

   g_return_val_if_fail (minfo, NULL);

   width  = minfo->width;
   height = minfo->height;

   /* memory allocate for flag */
   sa = minfo->off_b - minfo->off_a;
   fa = flaga = g_malloc(sa);
   if (!fa)
      return NULL;
   fb = flagb = g_malloc(sa << 3);
   if (!fb) {
      free(flaga);
      return NULL;
   }

   /* decode flag */
   gimv_io_seek (gio, minfo->offset + minfo->off_a, 0);
   gimv_io_read (gio, flaga, sa, &bytes_read);
   gimv_io_seek (gio, minfo->offset + minfo->off_b, 0);
   for (i = 0; i < sa; i++) {
      f = *fa++;
      for (j = 0; j < 8; j++) {
         guchar c;
         if (f & bitmask[j]) {
            gimv_io_read (gio, &c, 1, &bytes_read);
            *fb++ = c;
         } else {
            *fb++ = 0;
         }
      }
   }
   free(flaga);

   /* do xor */
   fb = flagb + minfo->flagperline;
   for (i = 0; i < minfo->flagperline * (height - 1); i++, fb++)
      *fb ^= *(fb - minfo->flagperline);

   if (!gimv_image_loader_progress_update (loader))
      goto ERROR0;

   /* memory allocate for data  */
   dest = d = g_malloc0 (sizeof(guchar) * width * height * 3);
   if (!dest) goto ERROR0;
			 
   /* read pixel */
   gimv_io_seek(gio, minfo->offset + minfo->off_p, SEEK_SET);
   pix = g_malloc0 (minfo->size_p);
   if (!pix) goto ERROR1;

   status = gimv_io_read (gio, pix, minfo->size_p, &bytes_read);
   if (status != GIMV_IO_STATUS_NORMAL) {
      fprintf(stderr, "Premature MAG file\n");
      goto ERROR2;
   }

   if (!gimv_image_loader_progress_update (loader))
      goto ERROR2;

   /* put pixels */
   pr = 0;
   fb = flagb;
   for (i = 0; i < minfo->flagperline * height; i++) {
      f = *fb++;
      j = f >> 4;
      if (j) {   /* copy from specified pixel */
         if (minfo->screen & COLOR256) {
            src = d + (offset[j][0] * 2 + offset[j][1] * width) * 3;
            for (k = 0; k < 6; k++)
               *d++ = *src++;
         } else {
            src = d + (offset[j][0] * 4 + offset[j][1] * width) * 3;
            for (k = 0; k < 12; k++)
               *d++ = *src++;
         }
      } else {  /* put pixel directly */
         if (minfo->screen & COLOR256) {
            for (k = 0; k < 2; k++) {
               t = pix[pr++];
               *d++ = colormap[t][0];
               *d++ = colormap[t][1];
               *d++ = colormap[t][2];
            }
         } else {
            for (k = 0; k < 2; k++) {
               t = pix[pr++];
               *d++ = colormap[t >> 4][0];
               *d++ = colormap[t >> 4][1];
               *d++ = colormap[t >> 4][2];
               *d++ = colormap[t & 15][0];
               *d++ = colormap[t & 15][1];
               *d++ = colormap[t & 15][2];
            }
         }
      }
      j = f & 15;
      if (j) {
         if (minfo->screen & COLOR256) {
            src = d + (offset[j][0] * 2 + offset[j][1] * width) * 3;
            for (k = 0; k < 6; k++)
               *d++ = *src++;
         } else {
            src = d + (offset[j][0] * 4 + offset[j][1] * width) * 3;
            for (k = 0; k < 12; k++)
               *d++ = *src++;
         }
      } else {
         if (minfo->screen & COLOR256) {
            for (k = 0; k < 2; k++) {
               t = pix[pr++];
               *d++ = colormap[t][0];
               *d++ = colormap[t][1];
               *d++ = colormap[t][2];
            }
         } else {
            for (k = 0; k < 2; k++) {
               t = pix[pr++];
               *d++ = colormap[t >> 4][0];
               *d++ = colormap[t >> 4][1];
               *d++ = colormap[t >> 4][2];
               *d++ = colormap[t & 15][0];
               *d++ = colormap[t & 15][1];
               *d++ = colormap[t & 15][2];
            }
         }
      }
   }

   free(flagb);
   free(pix);

   return dest;

ERROR2:
   free (pix);
ERROR1:
   free (dest);
ERROR0:
   free (flagb);
   return NULL;
}
