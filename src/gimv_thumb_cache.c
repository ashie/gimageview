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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include "gimageview.h"
#include "gimv_image.h"
#include "gimv_image_info.h"
#include "gimv_thumb_cache.h"
#include "prefs.h"
#include "utils_gtk.h"
#include "utils_file.h"


GHashTable *thumbnail_loaders = NULL;


/*******************************************************************************
 *
 *  Private functions
 *
 ******************************************************************************/
static void
store_loader_label (gpointer key, gpointer value, gpointer data)
{
   GimvThumbCacheLoader *loader = value;
   GList **list = data;

   g_return_if_fail (key);
   g_return_if_fail (loader);
   g_return_if_fail (data);

   *list = g_list_append (*list, loader->label);
}


static void
store_saver_label (gpointer key, gpointer value, gpointer data)
{
   GimvThumbCacheLoader *loader = value;
   GList **list = data;

   g_return_if_fail (key);
   g_return_if_fail (loader);
   g_return_if_fail (data);

   if (loader->save)
      *list = g_list_append (*list, loader->label);
}


/*******************************************************************************
 *
 *  Public functions
 *
 ******************************************************************************/
gboolean
gimv_thumb_cache_plugin_regist (const gchar *plugin_name,
                                const gchar *module_name,
                                gpointer impl,
                                gint     size)
{
   GimvThumbCacheLoader *loader = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (loader, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (loader->if_version == GIMV_THUMB_CACHE_LOADER_IF_VERSION, FALSE);
   g_return_val_if_fail (loader->load && loader->get_path, FALSE);

   if (!thumbnail_loaders)
      thumbnail_loaders = g_hash_table_new (g_str_hash, g_str_equal);

   g_hash_table_insert (thumbnail_loaders, loader->label, loader);

   return TRUE;
}


GimvImage *
gimv_thumb_cache_load (const gchar *filename,
                       const gchar *type,
                       GimvImageInfo *info)
{
   GimvThumbCacheLoader *loader;
   GimvImage *retval;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (type, NULL);

   if (!thumbnail_loaders) return NULL;

   if (!g_ascii_strcasecmp (type, "none")) return FALSE;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return NULL;

   retval = loader->load (filename, type, info);

   return retval;
}


GimvImage *
gimv_thumb_cache_save (const gchar *filename,
                       const gchar *type,
                       GimvImage *image,
                       GimvImageInfo *info)
{
   GimvThumbCacheLoader *loader;
   GimvImage *retval;

   g_return_val_if_fail (filename, FALSE);
   g_return_val_if_fail (type, FALSE);

   if (!thumbnail_loaders) return FALSE;

   if (!g_ascii_strcasecmp (type, "none")) return FALSE;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return FALSE;

   retval = loader->save (filename, type, image, info);

   return retval;
}


gchar *
gimv_thumb_cache_get_path (const gchar *filename, const gchar *type)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (type, NULL);

   if (!thumbnail_loaders) return NULL;

   if (!g_ascii_strcasecmp (type, "none")) return NULL;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return NULL;

   return loader->get_path (filename, type);
}


gboolean
gimv_thumb_cache_exists (const gchar *filename, const gchar *type)
{
   gchar *cache = gimv_thumb_cache_get_path (filename, type);
   gboolean exist;

   if (cache) {
      exist = file_exists (cache);
      g_free (cache);
      return exist;
   }

   return FALSE;
}


gboolean
gimv_thumb_cache_get_size (gint width, gint height, const gchar *type,
                           gint *width_ret, gint *height_ret)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (type, FALSE);

   if (!thumbnail_loaders) return FALSE;

   if (!g_ascii_strcasecmp (type, "none")) return FALSE;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return FALSE;

   if (!loader->get_size) return FALSE;

   return loader->get_size (width, height, type, width_ret, height_ret);
}


gboolean
gimv_thumb_cache_has_load_prefs (const gchar *type)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (type, FALSE);

   if (!thumbnail_loaders) return FALSE;

   if (!g_ascii_strcasecmp (type, "none")) return FALSE;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return FALSE;

   if (!loader->prefs_load)
      return FALSE;
   else
      return TRUE;
}


gboolean
gimv_thumb_cache_has_save_prefs (const gchar *type)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (type, FALSE);

   if (!thumbnail_loaders) return FALSE;

   if (!g_ascii_strcasecmp (type, "none")) return FALSE;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return FALSE;

   if (!loader->prefs_save)
      return FALSE;
   else
      return TRUE;
}


GtkWidget *
gimv_thumb_cache_get_load_prefs (const gchar *type, gpointer data)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (type, NULL);

   if (!thumbnail_loaders) return NULL;

   if (!g_ascii_strcasecmp (type, "none")) return NULL;

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return NULL;

   if (!loader->prefs_load) return NULL;

   return loader->prefs_load (data);
}


GtkWidget *
gimv_thumb_cache_get_save_prefs (const gchar *type, gpointer data)
{
   GimvThumbCacheLoader *loader;

   if (!g_ascii_strcasecmp (type, "none")) return NULL;

   if (!thumbnail_loaders) return NULL;

   g_return_val_if_fail (type, NULL);

   loader = g_hash_table_lookup (thumbnail_loaders, type);
   if (!loader) return NULL;

   if (!loader->prefs_save) return NULL;

   return loader->prefs_save (data);
}


GimvThumbCacheClearStatus
gimv_thumb_cache_clear (const gchar *path,
                        const gchar *cache_type,
                        GimvThumbCacheClearFlags flags,
                        gpointer unused_yet)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (cache_type && *cache_type,
                         GIMV_THUMB_CACHE_CLEAR_STATUS_UNKNOWN_ERROR);

   loader = g_hash_table_lookup (thumbnail_loaders, cache_type);
   if (!loader) return GIMV_THUMB_CACHE_CLEAR_STATUS_UNKNOWN_ERROR;
   if (!loader->clear) return GIMV_THUMB_CACHE_CLEAR_STATUS_NORMAL;

   return loader->clear(path, cache_type, flags, unused_yet);
}


gboolean
gimv_thumb_cache_can_clear (const gchar *path, const gchar *cache_type)
{
   GimvThumbCacheLoader *loader;

   g_return_val_if_fail (cache_type && *cache_type,
                         GIMV_THUMB_CACHE_CLEAR_STATUS_UNKNOWN_ERROR);

   loader = g_hash_table_lookup (thumbnail_loaders, cache_type);
   if (!loader) return FALSE;
   if (!loader->clear) return FALSE;

   return TRUE;
}


GList *
gimv_thumb_cache_get_loader_list (void)
{
   GList *list = NULL;

   if (thumbnail_loaders)
      g_hash_table_foreach (thumbnail_loaders, (GHFunc) store_loader_label,
                            &list);

   if (list)
      list = g_list_sort (list, gtkutil_comp_spel);

   return list;
}


GList *
gimv_thumb_cache_get_saver_list (void)
{
   GList *list = NULL;

   if (thumbnail_loaders)
      g_hash_table_foreach (thumbnail_loaders, (GHFunc) store_saver_label,
                            &list);

   if (list)
      list = g_list_sort (list, gtkutil_comp_spel);

   list = g_list_prepend (list, "NONE");

   return list;
}
