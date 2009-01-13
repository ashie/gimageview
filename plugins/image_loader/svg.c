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

#include "svg.h"

#ifdef ENABLE_SVG

#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <librsvg/rsvg.h>
#include "gimv_plugin.h"
#include "utils_file.h"


static GimvImageLoaderPlugin gimv_rsvg_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "SVG",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CANNOT_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        svg_load_image,
   }
};

static const gchar *svg_extensions[] =
{
   "svg", "svgz",
};

static GimvMimeTypeEntry svg_mime_types[] =
{
   {
      mime_type:      "image/svg",
      description:    "Scalable Vector Graphics",
      extensions:     svg_extensions,
      extensions_len: sizeof (svg_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/svg+xml",
      description:    "Scalable Vector Graphics",
      extensions:     svg_extensions,
      extensions_len: sizeof (svg_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_rsvg_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(svg_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("SVG Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};

GimvImage *
svg_load_image (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   gfloat w_scale, h_scale;
   GimvImage *image = NULL;
   GdkPixbuf *pixbuf;

   g_return_val_if_fail (loader, NULL);

#warning FIXME!!
   if (loader->info) {
      if (gimv_image_info_is_movie (loader->info) ||
          gimv_image_info_is_audio (loader->info))
      {
         return NULL;
      }
   }

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

   if (!gimv_image_loader_get_scale (loader, &w_scale, &h_scale)) {
      w_scale = 1.0;
      h_scale = 1.0;
   }

   {
      /*GError *error = NULL;*/
      pixbuf = rsvg_pixbuf_from_file_at_zoom (filename,
                                              w_scale,
                                              h_scale,
                                              NULL);
   }

   if (pixbuf) {
      image = gimv_image_new ();
      image->image = pixbuf;
      image->flags |= GIMV_IMAGE_VECTOR_FLAGS;
      return image;
   } else {
      return NULL;
   }
}
#endif /* ENABLE_SVG */
