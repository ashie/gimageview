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

#ifndef __GIMV_THUMB_CACHE_H__
#define __GIMV_THUMB_CACHE_H__

#include "gimv_image.h"
#include <sys/types.h>


/* not completed yet */
typedef enum {
   GIMV_THUMB_CACHE_CLEAR_STATUS_NORMAL,
   GIMV_THUMB_CACHE_CLEAR_STATUS_UNKNOWN_ERROR
} GimvThumbCacheClearStatus;


/* this feature is not implemented yet */
typedef enum {
   GIMV_THUMB_CACHE_CLEAR_TEST       = 1 << 0, /* confirm whether the plugin can do it or not */
   GIMV_THUMB_CACHE_CLEAR_ALL        = 1 << 1, /* clear thumbnails under specified directory */
                                               /* other flags will be ignored except
                                                  THUMB_CACHE_CLEAR_TEST */
   GIMV_THUMB_CACHE_CLEAR_OLD        = 1 << 2, /* clear old thumbnails */
   GIMV_THUMB_CACHE_CLEAR_GARBAGE    = 1 << 3, /* clear thumbnails for not existing images */
   GIMV_THUMB_CACHE_CLEAR_RECURSIVE  = 1 << 4, /* also clear all sub directories */
   GIMV_THUMB_CACHE_CLEAR_ASYNC      = 1 << 5
} GimvThumbCacheClearFlags;


#define GIMV_THUMB_CACHE_LOADER_IF_VERSION 2


typedef struct GimvThumbCacheLoader_Tag
{
   const guint32          if_version; /* plugin interface version */
   gchar                 *label;

   GimvImage             *(*load)             (const gchar   *filename,
                                               const gchar   *cache_type,
                                               GimvImageInfo *info);
   GimvImage             *(*save)             (const gchar   *filename,
                                               const gchar   *cache_type,
                                               GimvImage     *image,
                                               GimvImageInfo *info);
   gchar                 *(*get_path)         (const gchar   *filename,
                                               const gchar   *cache_type);
   gboolean               (*get_size)         (gint           width,
                                               gint           height,
                                               const gchar   *cache_type,
                                               gint          *width_ret,
                                               gint          *height_ret);

   GimvImageInfo         *(*get_info)         (const gchar   *filename,
                                               const gchar   *cache_type);
   gboolean               (*put_info)         (const gchar   *filename,
                                               const gchar   *cache_type,
                                               GimvImageInfo *info);

   GtkWidget             *(*prefs_load)       (gpointer data);
   GtkWidget             *(*prefs_save)       (gpointer data);

   /* if path is file, all flags will be ignored */
   GimvThumbCacheClearStatus  (*clear)        (const gchar   *path,
                                               const gchar   *cache_type,
                                               GimvThumbCacheClearFlags flags,
                                               gpointer       unused_yet);
} GimvThumbCacheLoader;


gchar     *gimv_thumb_cache_get_path          (const gchar   *filename,
                                               const gchar   *type);
gboolean   gimv_thumb_cache_exists            (const gchar   *filename,
                                               const gchar   *type);
GimvImage *gimv_thumb_cache_load              (const gchar   *filename,
                                               const gchar   *type,
                                               GimvImageInfo *info);
GimvImage *gimv_thumb_cache_save              (const gchar   *filename,
                                               const gchar   *type,
                                               GimvImage     *image,
                                               GimvImageInfo *info);
gboolean   gimv_thumb_cache_get_size          (gint           width,
                                               gint           height,
                                               const gchar   *type,
                                               gint          *width_ret,
                                               gint          *height_ret);
gboolean   gimv_thumb_cache_has_load_prefs    (const gchar   *type);
gboolean   gimv_thumb_cache_has_save_prefs    (const gchar   *type);
GtkWidget *gimv_thumb_cache_get_load_prefs    (const gchar   *type,
                                               gpointer       data);
GtkWidget *gimv_thumb_cache_get_save_prefs    (const gchar   *type,
                                               gpointer       data);

GList     *gimv_thumb_cache_get_loader_list   (void);
GList     *gimv_thumb_cache_get_saver_list    (void);

/* currentry, only a directory will be accepted,
   and all flags will be ignored */
GimvThumbCacheClearStatus
           gimv_thumb_cache_clear             (const gchar   *path,
                                               const gchar   *cache_type,
                                               GimvThumbCacheClearFlags flags,
                                               gpointer       unused_yet);
gboolean   gimv_thumb_cache_can_clear         (const gchar   *path,
                                               const gchar   *cache_type);

/* for plugin loader */
gboolean   gimv_thumb_cache_plugin_regist     (const gchar   *plugin_name,
                                               const gchar   *module_name,
                                               gpointer       impl,
                                               gint           size);

#endif /* __GIMV_THUMB_CACHE_H__ */
