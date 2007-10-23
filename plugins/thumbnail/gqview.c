/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GQview thumbnail support plugin for GImageView
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


#include "gimageview.h"

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

#define CACHE_GQVIEW_DEFAULT_SIZE "2"

/* under image directory */
#define GQVIEW_THUMNAIL_DIRECTORY ".gqview/thumbnails"

typedef struct GQViewThumbsize_Tag
{
   gint width;
   gint height;
} GQViewThumbSize;


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

GQViewThumbSize gqview_thumb_size [] =
{
   {24, 24},
   {32, 32},
   {48, 48},
   {64, 64},
   {85, 85},
   {100, 100},
   {125, 125},
   {150, 150},
   {175, 175},
   {200, 200},
   {256, 256},
};


static GimvThumbCacheLoader plugin_impl[] =
{
   {GIMV_THUMB_CACHE_LOADER_IF_VERSION,
    N_("GQview"),
    load_thumb, save_thumb,
    get_path, get_size,
    NULL, NULL, NULL,
    prefs_save, NULL},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_THUMB_CACHE)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("GQview thumbnail support"),
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


void
g_module_unload (GModule *module)
{
   return;
}



/******************************************************************************
 *
 *   Private functions.
 *
 ******************************************************************************/
static gint
get_thumb_size_from_config ()
{
   gint size = atoi (CACHE_GQVIEW_DEFAULT_SIZE);
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_THUMB_CACHE,
                                           "thumbnail_image_size_idx",
                                           GIMV_PLUGIN_PREFS_INT,
                                           (gpointer) &size);
   if (!success) {
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_THUMB_CACHE,
                                    "thumbnail_image_size_idx",
                                    CACHE_GQVIEW_DEFAULT_SIZE);
   }

   if (size > (gint) (sizeof (gqview_thumb_size) / sizeof (GQViewThumbSize))
       || size < 0)
   {
      size = atoi (CACHE_GQVIEW_DEFAULT_SIZE);
   }

   return size;
}


static void
cb_get_data_from_menuitem (GtkWidget *widget, gint *data)
{
   gint size_idx;
   gchar buf[8];

   size_idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "num"));
   g_snprintf (buf, 8, "%d", size_idx);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_THUMB_CACHE,
                                 "thumbnail_image_size_idx",
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
   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (cache_type, NULL);
   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

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
      g_print (N_("save cache: %s\n"), thumb_file);
      gimv_image_save_file (imcache, thumb_file, "png");
   }

   g_free (thumb_file);
   return imcache;
}


static gchar *
get_path (const gchar *filename, const gchar *cache_type)
{
   gchar *abspath;
   gchar buf[MAX_PATH_LEN];

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (cache_type, NULL);

   g_return_val_if_fail (!strcmp (cache_type, plugin_impl[0].label), NULL);

   abspath = relpath2abs (filename);
   g_return_val_if_fail (abspath, NULL);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s%s" ".png",
               g_getenv("HOME"), GQVIEW_THUMNAIL_DIRECTORY, filename);

   g_free (abspath);

   return g_strdup (buf);
}


static gboolean
get_size (gint width, gint height, const gchar *cache_type,
          gint *width_ret, gint *height_ret)
{
   gint size_idx, max_size;

   size_idx = get_thumb_size_from_config ();
   max_size = gqview_thumb_size [size_idx].width;

   /* check args */
   g_return_val_if_fail (width_ret && height_ret, FALSE);
   *width_ret = *height_ret = -1;

   g_return_val_if_fail (cache_type, FALSE);

   if (width < 1 || height < 1)
      return FALSE;

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
   GtkWidget *option_menu;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *menu, *menu_item;
   gint i, size_idx;
   gint num = sizeof (gqview_thumb_size) / sizeof (GQViewThumbSize);

   size_idx = get_thumb_size_from_config ();

   /* GQview thumbnail size */
   hbox = gtk_hbox_new (FALSE, 0);
   label = gtk_label_new (_("GQview thumbnail size"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
   option_menu = gtk_option_menu_new();
   menu = gtk_menu_new();
   for (i = 0; i < num; i++) {
      gchar buf [BUF_SIZE];

      g_snprintf (buf, BUF_SIZE, "%d x %d",
                  gqview_thumb_size[i].width, gqview_thumb_size[i].height);
      menu_item = gtk_menu_item_new_with_label (buf);
      g_object_set_data (G_OBJECT (menu_item), "num", GINT_TO_POINTER(i));
      g_signal_connect(G_OBJECT(menu_item), "activate",
                       G_CALLBACK(cb_get_data_from_menuitem),
                       NULL);
      gtk_menu_append (GTK_MENU(menu), menu_item);
      gtk_widget_show (menu_item);
   }
   gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
   gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
                                size_idx);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);

   return hbox;
}
