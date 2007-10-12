/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView thumbnail support plugin
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "gimageview.h"

#include "fileutil.h"
#include "gfileutil.h"
#include "gimv_image_saver.h"
#include "gimv_plugin.h"
#include "gimv_thumb_cache.h"


#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

#define CACHE_GIMV_DEFAULT_SIZE "96"

#ifndef CACHE_GIMV_MIN_SIZE
#define CACHE_GIMV_MIN_SIZE 4
#endif

#ifndef CACHE_GIMV_MAX_SIZE
#define CACHE_GIMV_MAX_SIZE 640
#endif

/* under home directory */
#define GIMV_THUMNAIL_DIRECTORY ".gimv/thumbnail"


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
static GtkWidget *prefs_save (gpointer     data);
static GimvThumbCacheClearStatus clear_cache (const gchar *path,
                                              const gchar *cache_type,
                                              GimvThumbCacheClearFlags flags,
                                              gpointer     unused_yet);

/* plugin implement definition */
static GimvThumbCacheLoader plugin_impl[] =
{
   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("GImageView"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL,
    prefs_save, clear_cache},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_THUMB_CACHE)

/* plugin definition */
GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("GImageView thumbnail support"),
   version:       "0.5.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};
static GimvPluginInfo *this;


gchar *
g_module_check_init (GModule *module)
{
   g_module_symbol (module, "gimv_plugin_info", (gpointer) &this);
   return NULL;
}


#if 0
void
g_module_unload (GModule *module)
{
   return;
}
#endif



/******************************************************************************
 *
 *   Private functions.
 *
 ******************************************************************************/
static void
set_save_comment (GimvImageSaver *saver, GimvImageInfo *info)
{
   gchar buf[32];
   gint buflen = sizeof (buf) / sizeof (gchar);
   gint width, height;

   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));

   if (!info) return;

   gimv_image_info_get_image_size (info, &width, &height);

   g_snprintf (buf, buflen, "%d", width);
   gimv_image_saver_set_comment (saver, "OriginalWidth", buf);
   g_snprintf (buf, buflen, "%d", width);
   gimv_image_saver_set_comment (saver, "OriginalHeight", buf);
}


static gint
get_thumb_size_from_config (void)
{
   gint size = atoi (CACHE_GIMV_DEFAULT_SIZE);
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_THUMB_CACHE,
                                           "thumbnail_image_size",
                                           GIMV_PLUGIN_PREFS_INT,
                                           (gpointer) &size);
   if (!success) {
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_THUMB_CACHE,
                                    "thumbnail_image_size",
                                    CACHE_GIMV_DEFAULT_SIZE);
   }

   return size;
}


static void
cb_gimvthumb_get_data_from_adjustment_by_int (GtkWidget *widget, gint *data)
{
   gint size;
   gchar buf[8];

   size = GTK_ADJUSTMENT(widget)->value;
   g_snprintf (buf, 8, "%d", size);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_THUMB_CACHE,
                                 "thumbnail_image_size",
                                 buf);
}


/******************************************************************************
 *
 *   Plugin implement.
 *
 ******************************************************************************/
static GimvImage *
load_thumb (const gchar *filename, const gchar *cache_type,
            GimvImageInfo *info)
{
   GimvImage *image;
   gchar *thumb_file;
   const gchar *orig_width = NULL, *orig_height = NULL;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   thumb_file = get_path (filename, cache_type);

   g_return_val_if_fail (thumb_file, NULL);

   image = gimv_image_load_file (thumb_file, FALSE);
   if (!image) goto ERROR;

   orig_width  = gimv_image_get_comment (image, "OriginalWidth");
   orig_height = gimv_image_get_comment (image, "OriginalHeight");

   if (info && orig_width && orig_height) {
      gimv_image_info_set_size (info,
                                atoi (orig_width),
                                atoi (orig_height));
   }

ERROR:
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
   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (cache_type, NULL);
   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

   thumb_file = get_path (filename, cache_type);
   g_return_val_if_fail (thumb_file, NULL);

   /* get image width & height */
   gimv_image_get_size (image, &im_width, &im_height);
   if (im_width < 1 || im_height < 1) {
      g_print (N_("image size invalid\n"));
      goto ERROR;
   }

   /* get thumnail width & height */
   success = get_size (im_width, im_height, cache_type, &width, &height);
   if (!success || width < 1 || height < 1) {
      g_print (N_("cache size invalid\n"));
      goto ERROR;
   }

   /* create cache directory if not found */
   success = mkdirs (thumb_file);
   if (!success) {
      g_print (N_("cannot make dir\n"));
      goto ERROR;
   }

   /* scale image */
   imcache = gimv_image_scale (image, width, height);

   /* save cache to disk */
   if (imcache) {
      GimvImageSaver *saver;

      saver = gimv_image_saver_new_with_attr (imcache, thumb_file, "png");
      if (info) {
         gimv_image_saver_set_image_info (saver, info);
         set_save_comment (saver, info);
      }
      gimv_image_saver_save (saver);
      gimv_image_saver_unref (saver);
   }

   g_free (thumb_file);
   return imcache;

ERROR:
   g_free (thumb_file);
   return NULL;
}


static gchar *
get_path_private (const gchar *filename, const gchar *cache_type, gboolean add_ext)
{
   gchar *abspath;
   gchar buf[MAX_PATH_LEN];
   const gchar *format_str;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

   abspath = relpath2abs (filename);
   g_return_val_if_fail (abspath, NULL);

   format_str = add_ext ? "%s/%s%s.png" : "%s/%s%s";

   g_snprintf (buf, MAX_PATH_LEN, format_str,
               g_getenv("HOME"), GIMV_THUMNAIL_DIRECTORY, filename);

   g_free (abspath);

   return g_strdup (buf);
}


static gchar *
get_path (const gchar *filename, const gchar *cache_type)
{
   return get_path_private (filename, cache_type, TRUE);
}


static gboolean
get_size (gint width, gint height, const gchar *cache_type,
          gint *width_ret, gint *height_ret)
{
   gint max_size;

   /* check args */
   g_return_val_if_fail (width_ret && height_ret, FALSE);
   *width_ret = *height_ret = -1;

   g_return_val_if_fail (cache_type, FALSE);

   if (width < 1 || height < 1)
      return FALSE;

   /* get config value */
   max_size = get_thumb_size_from_config ();

   /* check config value */
   if (max_size < CACHE_GIMV_MIN_SIZE
       || max_size > CACHE_GIMV_MAX_SIZE)
   {
      return FALSE;
   }

   /* no need to scale */
   if (width < max_size && height < max_size) {
      *width_ret = width;
      *height_ret = height;
      return TRUE;
   }

   /* calculate width & height of thumbnail */
   if (width > height) {
      *width_ret = max_size;
      *height_ret = (gfloat) height * (gfloat) max_size / (gfloat) width;
   } else {
      *width_ret = (gfloat) width * (gfloat) max_size / (gfloat) height;
      *height_ret = max_size;
   }

   return TRUE;
}


GtkWidget *
prefs_save (gpointer data)
{
   GtkWidget *hbox, *label, *spinner;
   GtkAdjustment *adj;
   gint size;

   size = get_thumb_size_from_config ();

   /* GImageView thumbnail size */
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   label = gtk_label_new (_("GImageVIew thumbnail size"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (size,
                                               CACHE_GIMV_MIN_SIZE,
                                               CACHE_GIMV_MAX_SIZE,
                                               1.0, 5.0, 0.0);
   spinner = gtk_spin_button_new (adj, 0, 0);
   gtk_widget_set_usize(spinner, 70, -1);
   gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                       GTK_SIGNAL_FUNC (cb_gimvthumb_get_data_from_adjustment_by_int),
                       NULL);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   return hbox;
}


/* FIXME */
static GimvThumbCacheClearStatus
clear_cache (const gchar *path, const gchar *cache_type,
             GimvThumbCacheClearFlags flags, gpointer unused_yet)
{
   gchar *cache_path = NULL, *tmpstr = NULL;

   if (!path || !*path || !strcmp(path, "/")) {
      cache_path = get_path_private ("/", cache_type, FALSE);
      if (cache_path)
         delete_dir (cache_path, NULL);
   } else {
      /* FIXME: we must accept not only directory but also file */
      cache_path = get_path_private (path, cache_type, FALSE);
      if (cache_path)
         delete_dir (cache_path, NULL);
   }

   g_free (cache_path);
   g_free (tmpstr);

   return GIMV_THUMB_CACHE_CLEAR_STATUS_NORMAL;
}

