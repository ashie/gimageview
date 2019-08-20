/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * XV thumbnail support plugin for GImageView
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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "gimv_image_saver.h"
#include "gimv_plugin.h"
#include "gimv_thumb_cache.h"
#include "utils_file.h"
#include "utils_file_gtk.h"


#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

#define X_MAX_SIZE 80
#define Y_MAX_SIZE 60

/* under image directory */
#define XV_THUMNAIL_DIRECTORY ".xvpics"

static GimvImage *load_thumb (const gchar *filename,
                              const gchar *cache_type,
                              GimvImageInfo *info);
static GimvImage *save_thumb (const gchar *filename,
                              const gchar *cache_type,
                              GimvImage   *image,
                              GimvImageInfo *info);
static gchar     *get_path   (const gchar *filename,
                              const gchar *cache_type);
static gboolean   get_size   (gint         width,
                              gint         height,
                              const gchar *cache_type,
                              gint        *width_ret,
                              gint        *height_ret);


static GimvThumbCacheLoader plugin_impl[] =
{
   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("XV thumbnail"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL, NULL, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_THUMB_CACHE)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("XV thumbnail support"),
   version:       "0.5.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


gchar *
g_module_check_init (GModule *module)
{
   return NULL;
}


void
g_module_unload (GModule *module)
{
   return;
}


static GimvImage *
load_thumb (const gchar *filename, const gchar *cache_type,
            GimvImageInfo *info)
{
   GimvImage *image;
   gchar *thumb_file;
   const gchar *orig_width = NULL, *orig_height = NULL, *orig_size = NULL;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);

   g_return_val_if_fail (thumb_file, NULL);

   image = gimv_image_load_file (thumb_file, FALSE);
   if (!image) goto ERROR;

   orig_width  = gimv_image_get_comment (image, "OriginalWidth");
   orig_height = gimv_image_get_comment (image, "OriginalHeight");
   orig_size   = gimv_image_get_comment (image, "OriginalSize");

   if (info && orig_width && orig_height && orig_size) {
      gimv_image_info_set_size (info,
                                atoi (orig_width),
                                atoi (orig_height));
   }

ERROR:
   g_free (thumb_file);
   return image;
}


static gchar *
get_path (const gchar *filename, const gchar *cache_type)
{
   const gchar *image_name;
   gchar *abspath, *image_dir;
   gchar buf[MAX_PATH_LEN];

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

   abspath = relpath2abs (filename);

   /* get filename */
   image_name = g_path_get_basename (abspath);
   if (!image_name) goto ERROR;

   /* get dir name */
   image_dir = g_dirname (abspath);
   if (!image_dir) goto ERROR;

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s",
               image_dir, XV_THUMNAIL_DIRECTORY, image_name);

   g_free (abspath);
   g_free (image_dir);
   return g_strdup (buf);

ERROR:
   g_free (abspath);
   return NULL;
}


static GimvImage *
save_thumb (const gchar *filename, const gchar *cache_type,
            GimvImage *image, GimvImageInfo *info)
{
   GimvImage *imcache;
   gchar *thumb_file;
   gint im_width = -1, im_height = -1, width = -1, height = -1;
   gboolean success;

   /* check args */
   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);
   g_return_val_if_fail (thumb_file, NULL);

   /* get image width & height */
   gimv_image_get_size (image, &im_width, &im_height);
   if (im_width < 1 || im_height < 1) return NULL;

   /* get thumnail width & height */
   success = get_size (im_width, im_height, cache_type, &width, &height);
   if (!success || width < 1 || height < 1) return NULL;

   /* create cache directory if not found */
   success = mkdirs (thumb_file);
   if (!success) return NULL;

   /* scale image */
   imcache = gimv_image_scale (image, width, height);

   /* save cache to disk */
   if (imcache) {
      GimvImageSaver *saver;

      saver = gimv_image_saver_new_with_attr (imcache, thumb_file, "xvpics");
      if (info)
         gimv_image_saver_set_image_info (saver, info);
      gimv_image_saver_save (saver);
      g_object_unref (G_OBJECT (saver));
   }

   g_free (thumb_file);

   return imcache;
}


static gboolean
get_size (gint width, gint height, const gchar *cache_type,
          gint *width_ret, gint *height_ret)
{
   gfloat aspect_ratio = 1.0;
   gfloat thumb_aspect_ratio = (gfloat) Y_MAX_SIZE / (gfloat) Y_MAX_SIZE;

   /* check args */
   g_return_val_if_fail (width_ret && height_ret, FALSE);
   *width_ret = *height_ret = -1;

   g_return_val_if_fail (cache_type, FALSE);

   if (width < 1 || height < 1)
      return FALSE;

   aspect_ratio = (gfloat) width / (gfloat) height;

   if (aspect_ratio > thumb_aspect_ratio) {
      *width_ret  = X_MAX_SIZE;
      *height_ret = (gfloat) height * (gfloat) X_MAX_SIZE / (gfloat) width;
      if (*height_ret > Y_MAX_SIZE)
         *height_ret = Y_MAX_SIZE;
   } else {
      *width_ret  = (gfloat) width  * (gfloat) Y_MAX_SIZE / (gfloat) height;
      *height_ret = Y_MAX_SIZE;
      if (*width_ret > X_MAX_SIZE)
         *width_ret = X_MAX_SIZE;
   }

   return TRUE;
}
