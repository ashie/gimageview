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

#ifndef __GIMV_IMAGE_H__
#define __GIMV_IMAGE_H__

#include "gimageview.h"

#define GIMV_TYPE_IMAGE            (gimv_image_get_type ())
#define GIMV_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_IMAGE, GimvImage))
#define GIMV_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_IMAGE, GimvImageClass))
#define GIMV_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_IMAGE))
#define GIMV_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_IMAGE))
#define GIMV_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_IMAGE, GimvImageClass))

typedef struct GimvImage_Tag      GimvImage;
typedef struct GimvImagePriv_Tag  GimvImagePriv;
typedef struct GimvImageClass_Tag GimvImageClass;

typedef enum {
   GIMV_IMAGE_NORMAL_MODE       = 0,
   GIMV_IMAGE_DISSOLVE_MODE     = 1,
   GIMV_IMAGE_MULTIPLY_MODE     = 3,
   GIMV_IMAGE_SCREEN_MODE       = 4,
   GIMV_IMAGE_OVERLAY_MODE      = 5,
   GIMV_IMAGE_DIFFERENCE_MODE   = 6,
   GIMV_IMAGE_ADDITION_MODE     = 7,
   GIMV_IMAGE_SUBTRACT_MODE     = 8,
   GIMV_IMAGE_DARKEN_ONLY_MODE  = 9,
   GIMV_IMAGE_LIGHTEN_ONLY_MODE = 10,
   GIMV_IMAGE_HUE_MODE          = 11,
   GIMV_IMAGE_SATURATION_MODE   = 12,
   GIMV_IMAGE_COLOR_MODE        = 13,
   GIMV_IMAGE_VALUE_MODE        = 14
} GimvImageLayerMode;

typedef enum {
   GIMV_IMAGE_VECTOR_FLAGS   = 1 << 0
} GimvImageFlags;

typedef enum {
   GIMV_IMAGE_ROTATE_0,
   GIMV_IMAGE_ROTATE_90,
   GIMV_IMAGE_ROTATE_180,
   GIMV_IMAGE_ROTATE_270
} GimvImageAngle;

struct GimvImage_Tag
{
   GObject      parent;

   gpointer       image;   /* library dependent data */
   GimvImageAngle angle;
   GimvImageFlags flags;
   GHashTable    *comments;
   gpointer       additional_data;
};

struct GimvImageClass_Tag
{
   GObjectClass parent_class;
};

GType        gimv_image_get_type             (void);

const gchar *gimv_image_detect_type_by_ext   (const gchar  *str);
GimvImage   *gimv_image_load_file            (const gchar  *filename,
                                              gboolean      animation);
gboolean     gimv_image_save_file            (GimvImage    *image,
                                              const gchar  *filename,
                                              const gchar  *format);
GimvImage   *gimv_image_create_from_data     (guchar       *data,
                                              gint          width,
                                              gint          height,
                                              gboolean      alpha);
GimvImage   *gimv_image_create_from_drawable (GdkDrawable  *drawable,
                                              gint          x,
                                              gint          y,
                                              gint          width,
                                              gint          height);
GimvImage   *gimv_image_rotate_90            (GimvImage    *src_image,
                                              gboolean      counter_clockwise);
GimvImage   *gimv_image_rotate_180           (GimvImage    *src_image);
GimvImage   *gimv_image_rotate               (GimvImage    *src_image,
                                              GimvImageAngle abs_angle);
void         gimv_image_get_pixmap_and_mask  (GimvImage    *image,
                                              GdkPixmap   **pixmap_return,
                                              GdkBitmap   **mask_return);
void         gimv_image_free_pixmap_and_mask (GdkPixmap    *pixmap,
                                              GdkBitmap    *mask);
gboolean     gimv_image_is_scalable          (GimvImage    *image);
GimvImage   *gimv_image_scale                (GimvImage    *src_image,
                                              gint          width,
                                              gint          height);
GimvImage   *gimv_image_scale_down           (GimvImage    *image,
                                              int           dest_width,
                                              int           dest_height);
void         gimv_image_scale_get_pixmap     (GimvImage    *src_image,
                                              gint          width,
                                              gint          height,
                                              GdkPixmap   **pixmap_return,
                                              GdkBitmap   **mask_return);
void         gimv_image_get_size             (GimvImage    *image,
                                              gint         *width,
                                              gint         *height);
gint         gimv_image_width                (GimvImage    *image);
gint         gimv_image_height               (GimvImage    *image);
gint         gimv_image_depth                (GimvImage    *image);
gboolean     gimv_image_has_alpha            (GimvImage    *image);
gboolean     gimv_image_can_alpha            (GimvImage    *image);
gint         gimv_image_rowstride            (GimvImage    *image);
guchar      *gimv_image_get_pixels           (GimvImage    *image);
const gchar *gimv_image_get_comment          (GimvImage    *image,
                                              const gchar  *key);

/* protected */
GimvImage   *gimv_image_new                  (void);
void         gimv_image_add_comment          (GimvImage    *image,
                                              const gchar  *key,
                                              const gchar  *val);
void         gimv_image_free_comments        (GimvImage    *image);


/* FIXME!! */
gboolean     gimv_image_add_layer           (guchar *buffer,
                                             gint width,
                                             gint left,
                                             gint components,
                                             gint pass,
                                             GimvImageLayerMode mode,
                                             guchar *rgbbuffer);
GimvImage   *gimv_image_rgba2rgb            (GimvImage *image,
                                             gint       bg_red,
                                             gint       bg_green,
                                             gint       bg_blue,
                                             gboolean   ignore_alpha);
/* END FIXME!! */

#endif /* __GIMV_IMAGE_H__ */
