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

#ifndef __GIMV_IMAGE_LOADER_H__
#define __GIMV_IMAGE_LOADER_H__

#include <glib-object.h>

#include "gimv_image.h"
#include "gimv_image_info.h"
#include "gimv_io.h"

#define GIMV_TYPE_IMAGE_LOADER            (gimv_image_loader_get_type ())
#define GIMV_IMAGE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_IMAGE_LOADER, GimvImageLoader))
#define GIMV_IMAGE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_IMAGE_LOADER, GimvImageLoaderClass))
#define GIMV_IS_IMAGE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_IMAGE_LOADER))
#define GIMV_IS_IMAGE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_IMAGE_LOADER))
#define GIMV_IMAGE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_IMAGE_LOADER, GimvImageLoaderClass))

typedef struct GimvImageLoader_Tag      GimvImageLoader;
typedef struct GimvImageLoaderPriv_Tag  GimvImageLoaderPriv;
typedef struct GimvImageLoaderClass_Tag GimvImageLoaderClass;

typedef enum {
   GIMV_IMAGE_LOADER_LOAD_NORMAL,
   GIMV_IMAGE_LOADER_LOAD_THUMBNAIL
} GimvImageLoaderLoadType;

struct GimvImageLoader_Tag
{
   GObject parent;

   GimvImageInfo *info;
   GTimer        *timer;

   GimvImageLoaderPriv *priv;
};

struct GimvImageLoaderClass_Tag
{
   GObjectClass parent;

   void (*load_start)        (GimvImageLoader *loader);
   void (*progress_update)   (GimvImageLoader *loader);
   void (*load_end)          (GimvImageLoader *loader);
};


#define GIMV_IMAGE_LOADER_IF_VERSION 3

/*
 *  for plugin
 */
typedef enum {
   GIMV_IMAGE_LOADER_PRIORITY_HIGH          = -255,
   GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL    = -127,
   GIMV_IMAGE_LOADER_PRIORITY_DEFAULT       = 0,
   GIMV_IMAGE_LOADER_PRIORITY_CANNOT_CANCEL = 127,
   GIMV_IMAGE_LOADER_PRIORITY_LOW           = 255
} GimvImageLoaderPriority;

typedef struct GimvImageLoaderPlugin_Tag
{
   const guint32           if_version; /* plugin interface version */
   const gchar * const     id;
   GimvImageLoaderPriority priority_hint;

   const gchar       *(*check_type)    (GimvImageLoader *loader,
                                        gpointer         data);
   gboolean          *(*get_info)      (GimvImageLoader *loader,
                                        gpointer         data);
   GimvImage         *(*loader)        (GimvImageLoader *loader,
                                        gpointer         data);
} GimvImageLoaderPlugin;


/*
 *  used by client
 */
GType        gimv_image_loader_get_type          (void);
GimvImageLoader
            *gimv_image_loader_new               (void);
GimvImageLoader
            *gimv_image_loader_new_with_image_info (GimvImageInfo *info);
GimvImageLoader
            *gimv_image_loader_new_with_file_name(const gchar *filename);
void         gimv_image_loader_set_image_info    (GimvImageLoader *loader,
                                                  GimvImageInfo   *info);


void         gimv_image_loader_set_gio           (GimvImageLoader *loader,
                                                  GimvIO          *gio);
gboolean     gimv_image_loader_set_as_animation  (GimvImageLoader *loader,
                                                  gboolean         animation);
gboolean     gimv_image_loader_set_load_type     (GimvImageLoader *loader,
                                                  GimvImageLoaderLoadType type);
gboolean     gimv_image_loader_set_scale         (GimvImageLoader *loader,
                                                  gfloat           w_scale,
                                                  gfloat           h_scale);
/*
 *  Loaders are not always follow this request strictly.
 *  This request is for decreasing load of loading if possible.
 */
gboolean     gimv_image_loader_set_size_request  (GimvImageLoader *loader,
                                                  gint             max_width,
                                                  gint             max_height,
                                                  gboolean         keep_aspect);
void         gimv_image_loader_load              (GimvImageLoader *loader);
void         gimv_image_loader_load_start        (GimvImageLoader *loader);
void         gimv_image_loader_load_stop         (GimvImageLoader *loader);
gboolean     gimv_image_loader_is_loading        (GimvImageLoader *loader);
GimvImage   *gimv_image_loader_get_image         (GimvImageLoader *loader);
void         gimv_image_loader_unref_image       (GimvImageLoader *loader);


/*
 *  used by loader module
 */
GimvIO      *gimv_image_loader_get_gio           (GimvImageLoader *loader);
const gchar *gimv_image_loader_get_path          (GimvImageLoader *loader);
GimvImageLoaderLoadType
             gimv_image_loader_get_load_type     (GimvImageLoader *loader);
gboolean     gimv_image_loader_load_as_animation (GimvImageLoader *loader);
gboolean     gimv_image_loader_get_scale         (GimvImageLoader *loader,
                                                  gfloat *width_scale_ret,
                                                  gfloat *height_scale_ret);
gboolean     gimv_image_loader_get_size_request  (GimvImageLoader *loader,
                                                  gint            *max_width,
                                                  gint            *max_height,
                                                  gboolean        *keep_aspect);
gboolean     gimv_image_loader_progress_update   (GimvImageLoader *loader);

/*
 *  for plugin loader
 */
gboolean     gimv_image_loader_plugin_regist     (const gchar *plugin_name,
                                                  const gchar *module_name,
                                                  gpointer     impl,
                                                  gint         size);

#endif /* __GIMV_GIMV_IMAGE_LOADER_H__ */
