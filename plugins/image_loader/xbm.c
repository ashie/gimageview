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

/* These codes are mostly taken from:
 * X10 and X11 bitmap (XBM) loading and saving file filter for the GIMP.
 * XBM code Copyright (C) 1998 Gordon Matzigkeit
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "xbm.h"
#include "gimv_plugin.h"

#define XBM_BLACK (0x00)
#define XBM_WHITE (0xff)

gint		xbm_getval	(gint c, gint base);
gint		xbm_fgetc	(FILE *fp);
gboolean	xbm_match	(FILE *fp, guchar *s);
gboolean	xbm_get_int	(FILE *fp, gint *val);


static GimvImageLoaderPlugin gimv_xbm_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "XBM",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        xbm_load,
   }
};

static const gchar *xbm_extensions[] =
{
   "xbm",
};

static GimvMimeTypeEntry xbm_mime_types[] =
{
   {
      mime_type:      "image/x-xbm",
      description:    "X Bitmap Image",
      extensions:     xbm_extensions,
      extensions_len: sizeof (xbm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-xbitmap",
      description:    "X Bitmap Image",
      extensions:     xbm_extensions,
      extensions_len: sizeof (xbm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_xbm_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(xbm_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("XBM Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


gboolean
xbm_get_header(const gchar *filename, xbm_info *info)
{
   FILE *fp;
   gint width, height, intbits;
   gint c;

   if ((fp = fopen(filename, "r")) == NULL) return FALSE;

   intbits = height = width = 0;
   c = ' ';
   do {
      if (isspace(c)) {
         if(xbm_match(fp, "char")) {
            c = fgetc(fp);
            if (isspace(c)) {
               intbits = 8;
               continue;
            }
         } else if (xbm_match(fp, "short")) {
            c = fgetc(fp);
            if (isspace(c)) {
               intbits = 16;
               continue;
            }
         }
      }

      if (c == '_') {
         if (xbm_match(fp, "width")) {
            c = fgetc(fp);
            if (isspace(c)) {
               if (!xbm_get_int(fp, &width)) break;
               continue;
            }
         } else if (xbm_match(fp, "height")) {
            c = fgetc(fp);
            if (isspace (c)) {
               if (!xbm_get_int(fp, &height)) break;
               continue;
            }
         }
      }
		
      c = xbm_fgetc (fp);
   } while (c != '{' && c != EOF);
	
   fclose(fp);
	
   if (c == EOF || width == 0 || height == 0 || intbits == 0) {
      return FALSE;
   }
	
   info->width = width;
   info->height = height;
	
   return TRUE;
}

GimvImage *
xbm_load(GimvImageLoader *loader, gpointer user_data)
{
   const gchar *filename;
   GimvImage *image;
   FILE *fp;
   gint width, height, intbits;
   gint c, i, j, ptr;
   guchar *data;
   gint bytes_count = 0, bytes_count_tmp = 0, step = 65536;
   glong pos;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

   if ((fp = fopen(filename, "r")) == NULL) return NULL;

   intbits = height = width = 0;
   c = ' ';
   do {
      if (isspace(c)) {
         if(xbm_match(fp, "char")) {
            c = fgetc(fp);
            if (isspace(c)) {
               intbits = 8;
               continue;
            }
         } else if (xbm_match(fp, "short")) {
            c = fgetc(fp);
            if (isspace(c)) {
               intbits = 16;
               continue;
            }
         }
      }

      if (c == '_') {
         if (xbm_match(fp, "width")) {
            c = fgetc(fp);
            if (isspace(c)) {
               if (!xbm_get_int(fp, &width)) break;
               continue;
            }
         } else if (xbm_match(fp, "height")) {
            c = fgetc(fp);
            if (isspace (c)) {
               if (!xbm_get_int(fp, &height)) break;
               continue;
            }
         }
      }
		
      c = xbm_fgetc (fp);
   } while (c != '{' && c != EOF);
	
   if (c == EOF || width == 0 || height == 0 || intbits == 0) goto ERROR0;
   if (!gimv_image_loader_progress_update (loader)) goto ERROR0;

   data = g_new0 (guchar, width * height * 3);
   ptr = 0;

   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         if (j % intbits == 0) {
            if (!xbm_get_int(fp, &c)) goto ERROR1;
         }
         data[ptr++] = (c & 1) ? XBM_BLACK : XBM_WHITE;
         data[ptr++] = (c & 1) ? XBM_BLACK : XBM_WHITE;
         data[ptr++] = (c & 1) ? XBM_BLACK : XBM_WHITE;
         c >>= 1;

         pos = ftell (fp);
         bytes_count_tmp = pos / step;
         if (bytes_count_tmp > bytes_count) {
            bytes_count = bytes_count_tmp;
            if (!gimv_image_loader_progress_update (loader)) goto ERROR1;
         }
      }
   }
	
   fclose(fp);
   image = gimv_image_create_from_data (data, width, height, FALSE);
   return image;

ERROR1:
   g_free (data);
ERROR0:
   fclose(fp);
   return NULL;
}

gint
xbm_getval (gint c, gint base)
{
   static guchar *digits = "0123456789abcdefABCDEF";
   gint val;

   if (base == 16) base = 22;

   for (val = 0; val < base; val ++) {
      if (c == digits[val])
         return (val < 16) ? val : (val - 6);
   }
   return -1;
}

/* Same as fgetc, but skip C-style comments and insert whitespace. */
gint
xbm_fgetc(FILE *fp)
{
   gint comment, c;

   /* FIXME: insert whitespace as advertised. */
   comment = 0;
   do {
      c = fgetc (fp);
      if (comment) {
         if (c == '*')
            comment = 1;
         else if (comment == 1 && c == '/')
            comment = 0;
         else
            comment = 2;
      } else {
         if (c == '/') {
            c = fgetc (fp);
            if (c == '*') {
               comment = 2;
            } else {
               ungetc (c, fp);
               c = '/';
            }
         }
      }
   } while (comment && c != EOF);
	
   return c;
}

/* Match a string with a file. */
gboolean
xbm_match(FILE *fp, guchar *s)
{
   gint c;

   do {
      c = fgetc (fp);
      if (c == *s)
         s++;
      else
         break;
   } while (c != EOF && *s);

   if (!*s) return TRUE;

   if (c != EOF) ungetc (c, fp);
   return FALSE;
}

/* Read the next integer from the file, skipping all non-integers. */
gboolean
xbm_get_int(FILE *fp, gint *val)
{
   gint digval, base, c;

   do {
      c = xbm_fgetc(fp);
   } while (c != EOF && !isdigit (c));

   if (c == EOF) return FALSE;

   if (c == '0') {
      c = fgetc (fp);
      if (c == 'x' || c == 'X') {
         c = fgetc (fp);
         base = 16;
      } else if (isdigit (c)) {
         base = 8;
      } else {
         ungetc (c, fp);
         return FALSE;
      }
   } else {
      base = 10;
   }

   *val = 0;
   for (;;) {
      digval = xbm_getval(c, base);
      if (digval < 0) {
         ungetc (c, fp);
         break;
      }
      *val *= base;
      *val += digval;
      c = fgetc (fp);
   }
	
   return TRUE;
}
