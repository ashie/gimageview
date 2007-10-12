/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * Electric Eyes thumbnail support plugin for GImageView
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

#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "fileutil.h"
#include "gfileutil.h"
#include "gimv_plugin.h"
#include "gimv_thumb_cache.h"


#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

/* under image directory */
#define EE_THUMNAIL_DIRECTORY ".ee"


typedef struct EEThumbPrefs_Tag
{
   gint   width;
   gint   height;
   gchar *dir;
} EEThumbPrefs;


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


static EEThumbPrefs ee_thumb_prefs [] =
{
   {180, 180, "large"},
   {256, 64,  "med"},
   {256, 24,  "small"},
};
static gint ee_thumb_prefs_num
= sizeof (ee_thumb_prefs) / sizeof (ee_thumb_prefs[0]);

static GimvThumbCacheLoader plugin_impl[] =
{
   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("Electric Eyes (Preview)"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL, NULL, NULL},

   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("Electric Eyes (Icon)"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL, NULL, NULL},

   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("Electric Eyes (Mini)"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL, NULL, NULL},
};
static gint plugin_impl_num = sizeof (plugin_impl) / sizeof (plugin_impl[0]);

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_THUMB_CACHE)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Electric Eyes thumbnail support"),
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

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);

   g_return_val_if_fail (thumb_file, NULL);

   image = gimv_image_load_file (thumb_file, FALSE);

   g_free (thumb_file);

   return image;
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
   g_return_val_if_fail (filename, FALSE);
   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (cache_type, FALSE);

   thumb_file = get_path (filename, cache_type);
   g_return_val_if_fail (thumb_file, FALSE);

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
      g_print (N_("save cache: %s\n"), thumb_file);
      gimv_image_save_file (imcache, thumb_file, "ppm");
   }

   g_free (thumb_file);

   return imcache;
}


static gchar *
get_path (const gchar *filename, const gchar *cache_type)
{
   gchar *abspath;
   gchar buf[MAX_PATH_LEN];
   gchar *size = NULL;
   gint i;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   for (i = 0; (i < plugin_impl_num) && (i < ee_thumb_prefs_num); i++) {
      if (!strcmp (cache_type, plugin_impl[i].label)) {
         size = ee_thumb_prefs[i].dir;
         break;
      }
   }

   g_return_val_if_fail (size, NULL);

   abspath = relpath2abs (filename);
   g_return_val_if_fail (abspath, NULL);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s%s",
               g_getenv("HOME"), EE_THUMNAIL_DIRECTORY, size, filename);

   g_free (abspath);

   return g_strdup (buf);
}


static gboolean
get_size (gint width, gint height, const gchar *cache_type,
          gint *width_ret, gint *height_ret)
{
   gint max_width = -1, max_height = -1;
   gint i;
   gboolean use_large = FALSE;

   /* check args */
   g_return_val_if_fail (width_ret && height_ret, FALSE);
   *width_ret = *height_ret = -1;

   g_return_val_if_fail (cache_type, FALSE);

   if (width < 1 || height < 1)
      return FALSE;

   /* get max thumbnail size */
   for (i = 0; (i < plugin_impl_num) && (i < ee_thumb_prefs_num); i++) {
      if (!strcmp (cache_type, plugin_impl[i].label)) {
         max_width =  ee_thumb_prefs[i].width;
         max_height = ee_thumb_prefs[i].height;
         if (i == 0)
            use_large = TRUE;
         break;
      }
   }
   g_return_val_if_fail (max_width > 0 && max_height > 0, FALSE);

   /* no need to scale */
   if (width < max_width && height < max_height) {
      *width_ret  = width;
      *height_ret = height;
      return TRUE;
   }

   /* calculate width & height of thumbnail */
   if (width < height || !use_large) {
      *width_ret = (gfloat) width * (gfloat) max_height / (gfloat) height;
      *height_ret = max_height;
      if (!use_large && width > max_width) {
         *width_ret = max_width;
         *height_ret = (gfloat) height * (gfloat) max_width / (gfloat) width;
      }
   } else {
      *width_ret = max_width;
      *height_ret = (gfloat) height * (gfloat) max_width / (gfloat) width;
   }

   return TRUE;
}
