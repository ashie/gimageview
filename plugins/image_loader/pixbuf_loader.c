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
 */

#include "pixbuf_loader.h"

#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "pixbuf_anim.h"
#include "gimv_plugin.h"
#include "utils_file.h"


static GimvImageLoaderPlugin gimv_pixbuf_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "GdkPixbufLoader",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_DEFAULT,
      check_type:    NULL,
      get_info:      NULL,
      loader:        pixbuf_load,
   },
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "GdkPixbufFile",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CANNOT_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        pixbuf_load_file,
   },
};

/* should be fetched from GdkPixbufModule */
static const gchar *ani_extensions[] = {
   "ani",
};
static const gchar *bmp_extensions[] = {
   "bmp",
};
static const gchar *gif_extensions[] = {
   "gif",
};
static const gchar *icon_extensions[] = {
   "ico",
   "cur",
};
static const gchar *jpeg_extensions[] = {
   "jpg",
   "jpeg",
   "jpe",
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
static const gchar *ras_extensions[] = {
   "ras",
};
static const gchar *tga_extensions[] = {
   "tga",
   "targa",
};
static const gchar *tiff_extensions[] = {
   "tiff",
   "tif",
};
static const gchar *wbmp_extensions[] = {
   "wbmp",
};
static const gchar *xbm_extensions[] = {
   "xbm",
};
static const gchar *xpm_extensions[] = {
   "xpm",
};

static GimvMimeTypeEntry pixbuf_mime_types[] = {
   {
      mime_type:      "application/x-navi-animation",
      description:    N_("The ANI image format"),
      extensions:     ani_extensions,
      extensions_len: sizeof (ani_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
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
      mime_type:      "image/x-icon",
      description:    N_("The ICO image format"),
      extensions:     icon_extensions,
      extensions_len: sizeof (icon_extensions) / sizeof (gchar *),
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
      mime_type:      "image/x-cmu-raster",
      description:    N_("The Sun raster image format"),
      extensions:     ras_extensions,
      extensions_len: sizeof (ras_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-sun-raster",
      description:    N_("The Sun raster image format"),
      extensions:     ras_extensions,
      extensions_len: sizeof (ras_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-tga",
      description:    N_("The Targa image format"),
      extensions:     tga_extensions,
      extensions_len: sizeof (tga_extensions) / sizeof (gchar *),
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
      mime_type:      "image/vnd.wap.wbmp",
      description:    N_("The WBMP image format"),
      extensions:     wbmp_extensions,
      extensions_len: sizeof (wbmp_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-xbitmap",
      description:    N_("The XBM image format"),
      extensions:     xbm_extensions,
      extensions_len: sizeof (xbm_extensions) / sizeof (gchar *),
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

GIMV_PLUGIN_GET_IMPL(gimv_pixbuf_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(pixbuf_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("GdkPixbuf Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


static void
cb_area_prepared (GdkPixbufLoader *loader,
                  gboolean *ret)
{
   g_return_if_fail (ret);

   *ret = TRUE;
}


static void
cb_area_updated (GdkPixbufLoader *loader,
                 gint arg1,
                 gint arg2,
                 gint arg3,
                 gint arg4,
                 gboolean *ret)
{
   g_return_if_fail (ret);

   *ret = TRUE;
}


GimvImage *
pixbuf_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO *gio;
   GimvImage *image = NULL;
   GdkPixbufLoader *pixbuf_loader;
   guchar buf[512];
   gint buf_size = sizeof (buf) / sizeof (gchar);
   gint i;
   guint bytes;
   gboolean prepared = FALSE, updated = FALSE, frame_done = FALSE;
   gboolean cancel = FALSE;

   g_return_val_if_fail (loader, NULL);

#warning FIXME!!
   if (loader->info) {
      if (gimv_image_info_is_movie (loader->info) ||
          gimv_image_info_is_audio (loader->info))
      {
         return NULL;
      }
   }

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   pixbuf_loader = gdk_pixbuf_loader_new ();
   g_return_val_if_fail (pixbuf_loader, NULL);

   /* set signals */
   g_signal_connect (G_OBJECT (pixbuf_loader), "area-prepared",
                     G_CALLBACK (cb_area_prepared),
                     &prepared);
   g_signal_connect (G_OBJECT (pixbuf_loader), "area-updated",
                     G_CALLBACK (cb_area_updated),
                     &updated);

   /* load */
   for (i = 0;; i++) {
      gimv_io_read (gio, buf, buf_size, &bytes);

      if ((gint) bytes > 0) {
         gdk_pixbuf_loader_write (pixbuf_loader, buf, bytes, NULL);
      } else {
         break;
      }

      cancel = !gimv_image_loader_progress_update (loader);

      if (cancel) goto FUNC_END;
      if (!gimv_image_loader_load_as_animation(loader) && frame_done) break;
   }

   if (!prepared) goto FUNC_END;

   if (gimv_image_loader_load_as_animation(loader)) {
      GdkPixbufAnimation *anim;
      anim = gdk_pixbuf_loader_get_animation (pixbuf_loader);
      if (anim) {
         image = gimv_anim_new_from_gdk_pixbuf_animation (anim);
      }
   }

   if (!image) {
      image = gimv_image_new ();
      image->image = gdk_pixbuf_loader_get_pixbuf (pixbuf_loader);
      if (!image->image) {
         g_object_unref (G_OBJECT (image));
         image = NULL;
      } else {
         g_object_ref (image->image);
      }
   }

 FUNC_END:
   gdk_pixbuf_loader_close (pixbuf_loader, NULL);
   g_object_unref (G_OBJECT (pixbuf_loader));

   return image;
}


GimvImage *
pixbuf_load_file (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GimvImage *image = NULL;

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

   if (!file_exists (filename))
      return NULL;

   if (gimv_image_loader_load_as_animation(loader)) {
      GdkPixbufAnimation *anim;
      anim = gdk_pixbuf_animation_new_from_file (filename, NULL);
      if (anim) {
         image = gimv_anim_new_from_gdk_pixbuf_animation (anim);
         g_object_unref (anim);
      }
   } else {
      image = gimv_image_new ();
      image->image = gdk_pixbuf_new_from_file (filename, NULL);
   }

   if (image && !image->image) {
      g_object_unref (G_OBJECT (image));
      image = NULL;
   }

   return image;
}
