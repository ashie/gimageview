/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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
 * These codes are mostly taken from libwmf-0.2.8 (src/io-wmf.c)
 *
 * Copyright (C) 2002 Dom Lachowicz
 * Copyright (C) 2002 Francis James Franklin
 *
 * This module is based on AbiWord and wvWare code and ideas done by
 * Francis James Franklin <fjf@alinameridon.com> and io-svg.c in the
 * librsvg module by Matthias Clasen <maclas@gmx.de>
 */

#include "wmf.h"

#ifdef ENABLE_WMF

#include <libwmf/api.h>
#include <libwmf/gd.h>
#include "gimv_plugin.h"


static GimvImageLoaderPlugin gimv_wmf_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "WMF",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        gimv_wmf_load,
   }
};

static const gchar *wmf_extensions[] =
{
   "wmf",
};

static GimvMimeTypeEntry wmf_mime_types[] =
{
   {
      mime_type:      "image/x-wmf",
      description:    "Windows Meta File",
      extensions:     wmf_extensions,
      extensions_len: sizeof (wmf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_wmf_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(wmf_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("WMF Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type:  gimv_plugin_get_mime_type,
};


static guchar *
gd2rgb (int *gd_pixels, gint width, gint height, gboolean alpha)
{
   guchar  *pixels = NULL;
   guchar  *px_ptr = NULL;
   int     *gd_ptr = gd_pixels;
   gint     i = 0;
   gint     j = 0;
   gint     bytes = alpha ? 4 : 3;
   gsize    alloc_size = width * height * sizeof (guchar) * bytes;

   unsigned int pixel;
   guchar       r,g,b,a;

   pixels = (guchar *) g_malloc0 (alloc_size);

   px_ptr = pixels;

   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         pixel = (unsigned int) (*gd_ptr++);

         b = (guchar) (pixel & 0xff);
         pixel >>= 8;
         g = (guchar) (pixel & 0xff);
         pixel >>= 8;
         r = (guchar) (pixel & 0xff);
         pixel >>= 7;
         a = (guchar) (pixel & 0xfe);
         a ^= 0xff;

         *px_ptr++ = r;
         *px_ptr++ = g;
         *px_ptr++ = b;
         if (alpha)
            *px_ptr++ = a;
      }
   }

   return pixels;
}


static gboolean
compare_signature (guchar buf[16], guchar sig[16])
{
   gint i;

   for (i = 0; i < 4; i++) {
      if (buf[i] != sig[i])
         return FALSE;
   }

   return TRUE;
}


static gboolean
gimv_wmf_identify (GimvImageLoader *loader)
{
   guchar buf[16];
   GimvIO *gio;
   gint bytes_read, i;
   guchar *sig[] = {"\xd7\xcd\xc6\x9a", "\x01\x00\x09\x00"};
   gint n_sig = sizeof (sig) / sizeof (guchar*);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return FALSE;

   gimv_io_seek (gio, 0, SEEK_SET);
   gimv_io_read (gio, buf, 16, &bytes_read);
   if (bytes_read != 16) return FALSE;

   for (i = 0; i < n_sig; i++) {
      if (compare_signature (buf, sig[i]))
         return TRUE;
   }

   gimv_io_seek (gio, 0, SEEK_SET);

   return FALSE;
}


static guchar *
gimv_wmf_load_file (GimvImageLoader *loader, gboolean alpha, gint *len)
{
   GimvIO *gio;
   guchar *buf = NULL;
   guint bytes_read;
   gint buflen = 0, step = 65536; /* 64kB */

   g_return_val_if_fail (loader, NULL);
   g_return_val_if_fail (len, NULL);

   *len = 0;

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   buf = g_malloc (step);
   buflen = step;
   gimv_io_seek (gio, 0, SEEK_SET);

   for (;;) {
      if (buflen < *len + step) {
         buflen += step;
         buf = g_realloc (buf, buflen);
      }

      gimv_io_read (gio, buf, step, &bytes_read);
      if (!gimv_image_loader_progress_update (loader)) goto ERROR;

      if (bytes_read == 0) break;
      *len += bytes_read;
   }

   if (len <= 0) {
      g_free (buf);
      return NULL;
   }

   return buf;

 ERROR:
   g_free (buf);
   return NULL;
}


GimvImage *
gimv_wmf_load (GimvImageLoader *loader, gpointer data)
{
   guchar           *wmfdata;
   gint              wmflen;

   guchar           *pixels = NULL;
   GimvImage        *image  = NULL;
   gboolean          alpha  = FALSE;

   /*
    * the bits we need to decode the WMF via libwmf2's GD layer
    */
   wmf_error_t       err;
   unsigned long     flags;        
   wmf_gd_t         *ddata = NULL;        
   wmfAPI           *API   = NULL;        
   wmfAPI_Options    api_options;        
   wmfD_Rect         bbox;

   void             *gd_image = NULL;
   int              *gd_pixels = NULL;

   unsigned int      width, height;
   /* double            units_per_inch; */
   double            resolution_x, resolution_y;

   g_return_val_if_fail (loader, NULL);

   if (!gimv_wmf_identify (loader)) return NULL;

   alpha = gimv_image_can_alpha (NULL);

   wmfdata = gimv_wmf_load_file (loader, alpha, &wmflen);
   if (!wmfdata) return NULL;

   /*
    * TODO: be smarter about getting the resolution from screen 
    */
   resolution_x = resolution_y = 72.0;

   flags = WMF_OPT_IGNORE_NONFATAL | WMF_OPT_FUNCTION;
   api_options.function = wmf_gd_function;

   err = wmf_api_create (&API, flags, &api_options);
   if (err != wmf_E_None) goto ERROR;

   ddata = WMF_GD_GetData (API);
   ddata->type = wmf_gd_image;

   err = wmf_mem_open (API, wmfdata, wmflen);
   if (err != wmf_E_None) goto ERROR;

   err = wmf_scan (API, 0, &bbox);
   if (err != wmf_E_None) goto ERROR;

   err = wmf_display_size (API, &width, &height, resolution_x, resolution_y);
   if (err != wmf_E_None || width <= 0. || height <= 0.) goto ERROR;

   /* FIXME */
   /*
   width  *= x_scale;
   height *= y_scale;
    */

   ddata->bbox   = bbox;        
   ddata->width  = width;
   ddata->height = height;

   err = wmf_play (API, 0, &bbox);
   if (err != wmf_E_None) goto ERROR;

   gd_image = ddata->gd_image;

   wmf_mem_close (API);

   if (gd_image) gd_pixels = wmf_gd_image_pixels (gd_image);
   if (gd_pixels == NULL) goto ERROR;

   pixels = gd2rgb (gd_pixels, width, height, alpha);
   if (pixels == NULL) goto ERROR;

   wmf_api_destroy (API);
   API = NULL;

   image = gimv_image_create_from_data (pixels, width, height, alpha);

 ERROR:
   if (API != NULL) wmf_api_destroy (API);
   g_free (wmfdata);

   return image;
}

#endif /* ENABLE_WMF */
