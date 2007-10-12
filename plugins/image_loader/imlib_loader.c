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

#include "imlib_loader.h"

#ifdef HAVE_GDK_IMLIB

#include <gdk_imlib.h>
#include "gimv_plugin.h"

static GimvImageLoaderPlugin gimv_imlib1_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "Imlib1",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CANNOT_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        gimv_imlib_load_file,
   }
};

/* native support only. do not add ImageMagick fall back */
static const gchar *bmp_extensions[] = {
   "bmp",
};
static const gchar *gif_extensions[] = {
   "gif",
};
static const gchar *jpeg_extensions[] = {
   "jpeg",
   "jpe",
   "jpg",
};
static const gchar *png_extensions[] = {
   "png",
};
static const gchar *ppm_extensions[] = {
   "ppm",
};
static const gchar *pgm_extensions[] = {
   "pgm",
};
static const gchar *pbm_extensions[] = {
   "pbm",
};
static const gchar *pnm_extensions[] = {
   "pnm",
   "pbm",
   "pgm",
   "ppm",
};
static const gchar *tiff_extensions[] = {
   "tiff",
   "tif",
};
static const gchar *xpm_extensions[] = {
   "xpm",
};

static GimvMimeTypeEntry imlib1_mime_types[] = {
   {
      mime_type:      "image/bmp",
      description:    N_("The BMP image format"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-bmp",
      description:    N_("The BMP image format"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-MS-bmp",
      description:    N_("The BMP image format"),
      extensions:     bmp_extensions,
      extensions_len: sizeof (bmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/gif",
      description:    N_("The GIF image format"),
      extensions:     gif_extensions,
      extensions_len: sizeof (gif_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/jpeg",
      description:    N_("The JPEG image format"),
      extensions:     jpeg_extensions,
      extensions_len: sizeof (jpeg_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/png",
      description:    N_("The PNG image format"),
      extensions:     png_extensions,
      extensions_len: sizeof (png_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-portable-anymap",
      description:    N_("Portable Any Map Image"),
      extensions:     pnm_extensions,
      extensions_len: sizeof (pnm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-portable-bitmap",
      description:    N_("The PBM image format"),
      extensions:     pbm_extensions,
      extensions_len: sizeof (pbm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-portable-graymap",
      description:    N_("The PGM image format"),
      extensions:     pgm_extensions,
      extensions_len: sizeof (pgm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-portable-pixmap",
      description:    N_("The PPM image format"),
      extensions:     ppm_extensions,
      extensions_len: sizeof (ppm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/tiff",
      description:    N_("The TIFF image format"),
      extensions:     tiff_extensions,
      extensions_len: sizeof (tiff_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-xpixmap",
      description:    N_("The XPM image format"),
      extensions:     xpm_extensions,
      extensions_len: sizeof (xpm_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_imlib1_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(imlib1_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Imlib1 Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};

GimvImage *
gimv_imlib_load_file (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GimvImage *image = NULL;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

   image = gimv_image_new ();
   image->image = gdk_imlib_load_image ((char *)filename);
   if (image && !image->image) {
      gimv_image_unref (image);
      image = NULL;
   }

   return image;
}
#endif /* HAVE_GDK_IMLIB */
