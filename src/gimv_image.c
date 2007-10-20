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

#include "gimageview.h"

#include <stdio.h>
#include <string.h>

#include "gimv_anim.h"
#include "gimv_image.h"
#include "gimv_image_loader.h"
#include "gimv_mime_types.h"
#include "prefs.h"
#include "fileutil.h"
#include "gimv_io.h"
#include "gimv_image_saver.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
typedef GdkPixbuf GimvRawImage;


G_DEFINE_TYPE (GimvImage, gimv_image, G_TYPE_OBJECT)

static void gimv_image_dispose      (GObject *object);
static void gimv_image_backend_init (void);

static GdkPixbuf *pixbuf_copy_rotate_90 (GdkPixbuf *src, 
                                         gboolean counter_clockwise);
static GdkPixbuf *pixbuf_copy_mirror    (GdkPixbuf *src, 
                                         gboolean mirror, 
                                         gboolean flip);

static void
gimv_image_class_init (GimvImageClass *klass)
{
   GObjectClass *gobject_class;

   gimv_image_backend_init ();

   gobject_class = (GObjectClass *) klass;

   gobject_class->dispose = gimv_image_dispose;
}


static void
gimv_image_init (GimvImage *image)
{
   image->image           = NULL;
   image->angle           = GIMV_IMAGE_ROTATE_0;
   image->flags           = 0;
   /* image->ref_count       = 1; */
   image->comments        = NULL;
   image->additional_data = NULL;
}


static void
gimv_image_dispose (GObject *object)
{
   GimvImage *image = GIMV_IMAGE (object);
   GimvRawImage *rawimage;

   g_return_if_fail (image);

   rawimage = (GimvRawImage *) image->image;

   if (rawimage)
      gdk_pixbuf_unref (rawimage);
   image->image = NULL;

   gimv_image_free_comments (image);

   if (G_OBJECT_CLASS (gimv_image_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_image_parent_class)->dispose (object);
}


GimvImage *
gimv_image_new (void)
{
   GimvImage *image = GIMV_IMAGE (g_object_new (GIMV_TYPE_IMAGE, NULL));
   return image;
}


/****************************************************************************
 *
 *  GimvImage methods
 *
 ****************************************************************************/
void
gimv_image_backend_init (void)
{
}


GimvImage *
gimv_image_load_file (const gchar *filename, gboolean animation)
{
   GimvImageLoader *loader;
   GimvImage *image;

   loader = gimv_image_loader_new_with_file_name (filename);
   if (!loader) return NULL;

   gimv_image_loader_set_as_animation (loader, animation);
   gimv_image_loader_load (loader);
   image = gimv_image_loader_get_image (loader);
   if (image)
      g_object_ref (G_OBJECT (image));
   g_object_unref (G_OBJECT (loader));

   return image;
}


gboolean
gimv_image_save_file  (GimvImage   *image,
                       const gchar *filename,
                       const gchar *format)
{
   GimvImageSaver *saver;
   gboolean retval;

   saver = gimv_image_saver_new_with_attr (image, filename, format);
   /* gimv_image_saver_set_param (data, use_specific_data) */
   retval = gimv_image_saver_save (saver);
   g_object_unref (G_OBJECT (saver));

   return retval;
}


static void
free_rgb_buffer (guchar *pixels, gpointer data)
{
   g_free(pixels);
}


GimvImage *
gimv_image_create_from_data (guchar *data, gint width, gint height, gboolean alpha)
{
   GimvImage *image;

   image = gimv_image_new ();

   {
      gint bytes = 3;

      if (alpha)
         bytes = 4;

      image->image = gdk_pixbuf_new_from_data (data,
                                               GDK_COLORSPACE_RGB, alpha, 8,
                                               width, height, bytes * width,
                                               free_rgb_buffer, NULL);
   }

   if (!image->image) {
      g_object_unref (G_OBJECT (image));
      image = NULL;
   }

   return image;
}


GimvImage *
gimv_image_create_from_drawable (GdkDrawable *drawable, gint x, gint y,
                                 gint width, gint height)
{
   GimvImage *image;

   image = gimv_image_new ();

   {
      GdkColormap *cmap = gdk_colormap_get_system ();
      image->image = gdk_pixbuf_get_from_drawable (NULL, drawable, cmap,
                                                   x , y, 0, 0,
                                                   width, height);
   }

   if (!image->image) {
      g_object_unref (G_OBJECT (image));
      image = NULL;
   }

   return image;
}


void
gimv_image_add_comment (GimvImage *image,
                        const gchar *key,
                        const gchar *val)
{
   gchar *orig_key, *old_value;
   gboolean success;

   g_return_if_fail (image);
   g_return_if_fail (key && *key);
   g_return_if_fail (val);

   if (!image->comments)
      image->comments = g_hash_table_new (g_str_hash, g_str_equal);

   success = g_hash_table_lookup_extended (image->comments, key,
                                           (gpointer) &orig_key,
                                           (gpointer) &old_value);
   if (success) {
      g_hash_table_remove (image->comments, key);
      g_free (orig_key);
      g_free (old_value);
   }

   g_hash_table_insert (image->comments, g_strdup (key), g_strdup (val));
}


GimvImage *
gimv_image_rotate_90 (GimvImage *image,
                      gboolean counter_clockwise)
{
   GimvRawImage *src_rawimage, *dest_rawimage;

   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (image->image, NULL);

   src_rawimage = (GimvRawImage *) image->image;

   dest_rawimage = pixbuf_copy_rotate_90 (src_rawimage, counter_clockwise);
   gdk_pixbuf_unref (src_rawimage);

   image->image = dest_rawimage;

   if (counter_clockwise)
      image->angle += GIMV_IMAGE_ROTATE_90;
   else
      image->angle += GIMV_IMAGE_ROTATE_270;

   image->angle %= 4;

   if (image->angle < 0)
      image->angle += 4;

   return image;
}


GimvImage *
gimv_image_rotate_180 (GimvImage *image)
{
   GimvRawImage *src_rawimage, *dest_rawimage;

   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (image->image, NULL);

   src_rawimage = (GimvRawImage *) image->image;

   dest_rawimage = pixbuf_copy_mirror (src_rawimage, TRUE, TRUE);
   gdk_pixbuf_unref (src_rawimage);

   image->image = dest_rawimage;

   image->angle += GIMV_IMAGE_ROTATE_180;
   image->angle %= 4;
   if (image->angle < 0)
      image->angle += 4;

   return image;
}


GimvImage *
gimv_image_rotate (GimvImage *image, GimvImageAngle abs_angle)
{
   gint rotate, angle;

   g_return_val_if_fail (image, NULL);

   if (image->angle < 0 || image->angle > 3) {
      image->angle %= 4;
      if (image->angle < 0)
         image->angle += 4;
   }

   if (abs_angle < 0 || abs_angle > 3) {
      angle = abs_angle % 4;
      if (angle < 0)
         angle += 4;
   } else {
      angle = abs_angle;
   }

   if (image->angle == (GimvImageAngle) angle)
      return image;

   rotate = angle - image->angle;
   if (rotate < 0)
      rotate += 4;

   switch (rotate) {
   case GIMV_IMAGE_ROTATE_90:
      gimv_image_rotate_90 (image, TRUE);
      break;
   case GIMV_IMAGE_ROTATE_180:
      gimv_image_rotate_180 (image);
      break;
   case GIMV_IMAGE_ROTATE_270:
      gimv_image_rotate_90 (image, FALSE);
      break;
   default:
      break;
   }

   return image;
}


void
gimv_image_get_pixmap_and_mask (GimvImage  *image,
                                GdkPixmap **pixmap_return,
                                GdkBitmap **mask_return)
{
   GimvRawImage *rawimage;

   g_return_if_fail (image);
   g_return_if_fail (image->image);
   g_return_if_fail (pixmap_return);
   g_return_if_fail (mask_return);

   rawimage = (GimvRawImage *) image->image;

   gdk_pixbuf_render_pixmap_and_mask (rawimage, pixmap_return, mask_return, 64);
}


void
gimv_image_free_pixmap_and_mask (GdkPixmap *pixmap,
                                 GdkBitmap *mask)
{
   if (pixmap) gdk_pixmap_unref (pixmap);
   if (mask) gdk_bitmap_unref (mask);
}


gboolean
gimv_image_is_scalable (GimvImage *image)
{
   g_return_val_if_fail (image, FALSE);

   return image->flags & GIMV_IMAGE_VECTOR_FLAGS;
}


GimvImage *
gimv_image_scale (GimvImage *image,
                  gint width, gint height)
{
   GimvImage *dest_image;
   GimvRawImage *src_rawimage, *dest_rawimage;

   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (image->image, NULL);

   /* Check me!! */
   if (conf.fast_scale_down
       && width <= gimv_image_width (image)
       && height <= gimv_image_height (image))
   {
      return gimv_image_scale_down (image, width, height);
   }

   src_rawimage = (GimvRawImage *) image->image;

   /*
   if (gimv_image_is_scalable (image)) {
   } else {
   }
   */

   dest_rawimage = gdk_pixbuf_scale_simple (src_rawimage, width, height,
                                            conf.interpolation);

   dest_image = gimv_image_new ();
   dest_image->image = dest_rawimage;

   return dest_image;
}


/*
 *  fetched from of libgnomeui-2.2.0.1
 *  (libgnomeui/gnome-thumbnail-pixbuf-utils.c) and adapt to GImageView
 *
 *  Copyright (C) 2002 Red Hat, Inc.
 *  Author: Alexander Larsson <alexl@redhat.com>
 */
GimvImage *
gimv_image_scale_down (GimvImage *image,
                       int dest_width,
                       int dest_height)
{
   int source_width, source_height;
   int s_x1, s_y1, s_x2, s_y2;
   int s_xfrac, s_yfrac;
   int dx, dx_frac, dy, dy_frac;
   div_t ddx, ddy;
   int x, y;
   int r, g, b, a;
   int n_pixels;
   gboolean has_alpha;
   guchar *dest, *dest_pixels, *src, *xsrc, *src_pixels;
   GimvImage *dest_image;
   int pixel_stride;
   int source_rowstride, dest_rowstride;

   if (dest_width <= 0 || dest_height <= 0) {
      return NULL;
   }

   source_width  = gimv_image_width (image);
   source_height = gimv_image_height (image);

   g_assert (source_width >= dest_width);
   g_assert (source_height >= dest_height);

   ddx = div (source_width, dest_width);
   dx = ddx.quot;
   dx_frac = ddx.rem;

   ddy = div (source_height, dest_height);
   dy = ddy.quot;
   dy_frac = ddy.rem;

   has_alpha = gimv_image_has_alpha (image);
   pixel_stride = has_alpha ? 4 : 3;
   source_rowstride = source_width * pixel_stride;
   src_pixels = gimv_image_get_pixels (image);

   dest_rowstride = dest_width * pixel_stride;
   dest_pixels = g_malloc0 (sizeof (guchar) * dest_rowstride * dest_height);
   dest = dest_pixels;

   s_y1 = 0;
   s_yfrac = -dest_height / 2;
   while (s_y1 < source_height) {
      s_y2 = s_y1 + dy;
      s_yfrac += dy_frac;
      if (s_yfrac > 0) {
         s_y2++;
         s_yfrac -= dest_height;
      }

      s_x1 = 0;
      s_xfrac = -dest_width / 2;
      while (s_x1 < source_width) {
         s_x2 = s_x1 + dx;
         s_xfrac += dx_frac;
         if (s_xfrac > 0) {
            s_x2++;
            s_xfrac -= dest_width;
         }

         /* Average block of [x1,x2[ x [y1,y2[ and store in dest */
         r = g = b = a = 0;
         n_pixels = 0;

         src = src_pixels + s_y1 * source_rowstride + s_x1 * pixel_stride;
         for (y = s_y1; y < s_y2; y++) {
            xsrc = src;
            if (has_alpha) {
               for (x = 0; x < s_x2-s_x1; x++) {
                  n_pixels++;
						
                  r += xsrc[3] * xsrc[0];
                  g += xsrc[3] * xsrc[1];
                  b += xsrc[3] * xsrc[2];
                  a += xsrc[3];
                  xsrc += 4;
               }
            } else {
               for (x = 0; x < s_x2-s_x1; x++) {
                  n_pixels++;
                  r += *xsrc++;
                  g += *xsrc++;
                  b += *xsrc++;
               }
            }
            src += source_rowstride;
         }

         if (has_alpha) {
            if (a != 0) {
               *dest++ = r / a;
               *dest++ = g / a;
               *dest++ = b / a;
               *dest++ = a / n_pixels;
            } else {
               *dest++ = 0;
               *dest++ = 0;
               *dest++ = 0;
               *dest++ = 0;
            }
         } else {
            *dest++ = r / n_pixels;
            *dest++ = g / n_pixels;
            *dest++ = b / n_pixels;
         }

         s_x1 = s_x2;
      }
      s_y1 = s_y2;
      dest += dest_rowstride - dest_width * pixel_stride;
   }

   dest_image = gimv_image_create_from_data (dest_pixels,
                                             dest_width,
                                             dest_height,
                                             has_alpha);

	return dest_image;
}


void
gimv_image_scale_get_pixmap (GimvImage *image,
                             gint width, gint height,
                             GdkPixmap **pixmap_return,
                             GdkBitmap **mask_return)
{
   g_return_if_fail (image);
   g_return_if_fail (image->image);
   g_return_if_fail (pixmap_return);
   g_return_if_fail (mask_return);

   {
      GimvImage *dest_image;
      dest_image = gimv_image_scale (image, width, height);
      gimv_image_get_pixmap_and_mask (dest_image, pixmap_return, mask_return);
      g_object_unref (G_OBJECT (dest_image));
   }
}


void
gimv_image_get_size (GimvImage *image, gint *width, gint *height)
{
   GimvRawImage *rawimage;

   g_return_if_fail (width && height);
   *width = *height = -1;

   g_return_if_fail (image);
   g_return_if_fail (image->image);

   rawimage = (GimvRawImage *) image->image;

   *width  = gdk_pixbuf_get_width  (rawimage);
   *height = gdk_pixbuf_get_height (rawimage);
}


gint
gimv_image_width (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (image->image, -1);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_width  (rawimage);
}


gint
gimv_image_height (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (image->image, -1);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_height (rawimage);
}


gint
gimv_image_depth (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (image->image, -1);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_bits_per_sample (rawimage);
}


gboolean
gimv_image_has_alpha (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (image->image, FALSE);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_has_alpha (rawimage);
}


gboolean
gimv_image_can_alpha (GimvImage *image)
{
   return TRUE;
}


gint
gimv_image_rowstride (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (image->image, -1);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_rowstride (rawimage);
}


guchar *
gimv_image_get_pixels (GimvImage *image)
{
   GimvRawImage *rawimage;

   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (image->image, NULL);

   rawimage = (GimvRawImage *) image->image;

   return gdk_pixbuf_get_pixels (rawimage);
}


const gchar *
gimv_image_get_comment (GimvImage *image, const gchar *key)
{
   g_return_val_if_fail (image, NULL);

   if (!image->comments) return NULL;

   return g_hash_table_lookup (image->comments, key);
}


static void
free_comment (gpointer key, gpointer value, gpointer data)
{
   g_free (key);
   g_free (value);
}


void
gimv_image_free_comments (GimvImage *image)
{
   g_return_if_fail (image);

   if (!image->comments) return;

   g_hash_table_foreach (image->comments,
                         (GHFunc) free_comment,
                         image);
   g_hash_table_destroy (image->comments);
   image->comments = NULL;
}


/*
 *  gimv_image_detect_type_by_ext:
 *     @ Detect image format by filename extension.
 *
 *  str    : Image file name.
 *  Return : Image type by string.
 */
const gchar *
gimv_image_detect_type_by_ext (const gchar *str)
{
   gchar *ext = fileutil_get_extention (str);
   return gimv_mime_types_get_type_from_ext (ext);
}



/****************************************************************************
 *  FIXME!!
 ****************************************************************************/
static void
rgb_to_hsv (gint *r,
            gint *g,
            gint *b)
{
   gint red, green, blue;
   gfloat h, s, v;
   gint min, max;
   gint delta;

   h = 0.0;

   red = *r;
   green = *g;
   blue = *b;

   if (red > green) {
      if (red > blue)
         max = red;
      else
         max = blue;

      if (green < blue)
         min = green;
      else
         min = blue;
   } else {
      if (green > blue)
         max = green;
      else
         max = blue;

      if (red < blue)
         min = red;
      else
         min = blue;
   }

   v = max;

   if (max != 0)
      s = ((max - min) * 255) / (gfloat) max;
   else
      s = 0;

   if (s == 0) {
      h = 0;
   } else {
      delta = max - min;
      if (red == max)
         h = (green - blue) / (gfloat) delta;
      else if (green == max)
         h = 2 + (blue - red) / (gfloat) delta;
      else if (blue == max)
         h = 4 + (red - green) / (gfloat) delta;
      h *= 42.5;

      if (h < 0)
         h += 255;
      if (h > 255)
         h -= 255;
   }

   *r = h;
   *g = s;
   *b = v;
}


static void
hsv_to_rgb (gint *h,
            gint *s,
            gint *v)
{
   gfloat hue, saturation, value;
   gfloat f, p, q, t;

   if (*s == 0) {
      *h = *v;
      *s = *v;
      *v = *v;
   } else {
      hue = *h * 6.0 / 255.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (gint) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((gint) hue) {
      case 0:
         *h = value * 255;
         *s = t * 255;
         *v = p * 255;
         break;
      case 1:
         *h = q * 255;
         *s = value * 255;
         *v = p * 255;
         break;
      case 2:
         *h = p * 255;
         *s = value * 255;
         *v = t * 255;
         break;
      case 3:
         *h = p * 255;
         *s = q * 255;
         *v = value * 255;
         break;
      case 4:
         *h = t * 255;
         *s = p * 255;
         *v = value * 255;
         break;
      case 5:
         *h = value * 255;
         *s = p * 255;
         *v = q * 255;
         break;
      }
   }
}


static void
rgb_to_hls (gint *r,
            gint *g,
            gint *b)
{
   gint red, green, blue;
   gfloat h, l, s;
   gint min, max;
   gint delta;

   red = *r;
   green = *g;
   blue = *b;

   if (red > green) {
      if (red > blue)
         max = red;
      else
         max = blue;

      if (green < blue)
         min = green;
      else
         min = blue;
   } else {
      if (green > blue)
         max = green;
      else
         max = blue;

      if (red < blue)
         min = red;
      else
         min = blue;
   }

   l = (max + min) / 2.0;

   if (max == min) {
      s = 0.0;
      h = 0.0;
   } else {
      delta = (max - min);

      if (l < 128)
         s = 255 * (gfloat) delta / (gfloat) (max + min);
      else
         s = 255 * (gfloat) delta / (gfloat) (511 - max - min);

      if (red == max)
         h = (green - blue) / (gfloat) delta;
      else if (green == max)
         h = 2 + (blue - red) / (gfloat) delta;
      else
         h = 4 + (red - green) / (gfloat) delta;

      h = h * 42.5;

      if (h < 0)
         h += 255;
      if (h > 255)
         h -= 255;
   }

   *r = h;
   *g = l;
   *b = s;
}

static gint
hls_value (gfloat n1,
           gfloat n2,
           gfloat hue)
{
   gfloat value;

   if (hue > 255)
      hue -= 255;
   else if (hue < 0)
      hue += 255;
   if (hue < 42.5)
      value = n1 + (n2 - n1) * (hue / 42.5);
   else if (hue < 127.5)
      value = n2;
   else if (hue < 170)
      value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
   else
      value = n1;

   return (gint) (value * 255);
}


static void
hls_to_rgb (gint *h,
            gint *l,
            gint *s)
{
   gfloat hue, lightness, saturation;
   gfloat m1, m2;

   hue = *h;
   lightness = *l;
   saturation = *s;

   if (saturation == 0) {
      /*  achromatic case  */
      *h = lightness;
      *l = lightness;
      *s = lightness;
   } else {
      if (lightness < 128)
         m2 = (lightness * (255 + saturation)) / 65025.0;
      else
         m2 = (lightness + saturation - (lightness * saturation)/255.0) / 255.0;

      m1 = (lightness / 127.5) - m2;

      /*  chromatic case  */
      *h = hls_value (m1, m2, hue + 85);
      *l = hls_value (m1, m2, hue);
      *s = hls_value (m1, m2, hue - 85);
   }
}


static void
pixel_apply_layer_mode(guchar *src1,
                       guchar *src2,
                       guchar *dest,
                       gint b1,
                       gint b2,
                       gboolean ha1,
                       gboolean ha2,
                       GimvImageLayerMode mode)
{
   static gint b, alpha, t, t2, red1, green1, blue1, red2, green2, blue2;
	
   alpha = (ha1 || ha2) ? MAX(b1, b2) - 1 : b1;
   switch (mode) {
   case GIMV_IMAGE_DISSOLVE_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = src2[b];
      /*  dissolve if random value is > opacity  */
      t = (rand() & 0xFF);
      dest[alpha] = (t > src2[alpha]) ? 0 : src2[alpha];
      break;

   case GIMV_IMAGE_MULTIPLY_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = (src1[b] * src2[b]) / 255;
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_SCREEN_MODE:
      for (b = 0; b < alpha; b++)
         dest[b] = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_OVERLAY_MODE:
      for (b = 0; b < alpha; b++) {
         /* screen */
         t = 255 - ((255 - src1[b]) * (255 - src2[b])) / 255;
         /* mult */
         t2 = (src1[b] * src2[b]) / 255;
         dest[b] = (t * src1[b] + t2 * (255 - src1[b])) / 255;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_DIFFERENCE_MODE:
      for (b = 0; b < alpha; b++) {
         t = src1[b] - src2[b];
         dest[b] = (t < 0) ? -t : t;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_ADDITION_MODE:
      for (b = 0; b < alpha; b++) {
         t = src1[b] + src2[b];
         dest[b] = (t > 255) ? 255 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_SUBTRACT_MODE:
      for (b = 0; b < alpha; b++) {
         t = (gint)src1[b] - (gint)src2[b];
         dest[b] = (t < 0) ? 0 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_DARKEN_ONLY_MODE:
      for (b = 0; b < alpha; b++) {
         t = src1[b];
         t2 = src2[b];
         dest[b] = (t < t2) ? t : t2;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_LIGHTEN_ONLY_MODE:
      for (b = 0; b < alpha; b++) {
         t = src1[b];
         t2 = src2[b];
         dest[b] = (t < t2) ? t2 : t;
      }
      if (ha1 && ha2)
         dest[alpha] = MIN(src1[alpha], src2[alpha]);
      else if (ha2)
         dest[alpha] = src2[alpha];
      break;

   case GIMV_IMAGE_HUE_MODE:
   case GIMV_IMAGE_SATURATION_MODE:
   case GIMV_IMAGE_VALUE_MODE:
      red1 = src1[0]; green1 = src1[1]; blue1 = src1[2];
      red2 = src2[0]; green2 = src2[1]; blue2 = src2[2];
      rgb_to_hsv (&red1, &green1, &blue1);
      rgb_to_hsv (&red2, &green2, &blue2);
      switch (mode) {
      case GIMV_IMAGE_HUE_MODE:
         red1 = red2;
         break;
      case GIMV_IMAGE_SATURATION_MODE:
         green1 = green2;
         break;
      case GIMV_IMAGE_VALUE_MODE:
         blue1 = blue2;
         break;
      default:
         break;
      }
      hsv_to_rgb (&red1, &green1, &blue1);
      dest[0] = red1; dest[1] = green1; dest[2] = blue1;
      if (ha1 && ha2)
         dest[3] = MIN(src1[3], src2[3]);
      else if (ha2)
         dest[3] = src2[3];
      break;

   case GIMV_IMAGE_COLOR_MODE:
      red1 = src1[0]; green1 = src1[1]; blue1 = src1[2];
      red2 = src2[0]; green2 = src2[1]; blue2 = src2[2];
      rgb_to_hls (&red1, &green1, &blue1);
      rgb_to_hls (&red2, &green2, &blue2);
      red1 = red2;
      blue1 = blue2;
      hls_to_rgb (&red1, &green1, &blue1);
      dest[0] = red1; dest[1] = green1; dest[2] = blue1;
      if (ha1 && ha2)
         dest[3] = MIN(src1[3], src2[3]);
      else if (ha2)
         dest[3] = src2[3];
      break;

   case GIMV_IMAGE_NORMAL_MODE:
   default:
      for (b = 0; b < b2; b++) dest[b] = src2[b];
      break;
   }
}

static gint bgcolor_red   = 255;
static gint bgcolor_green = 255;
static gint bgcolor_blue  = 255;

gboolean
gimv_image_add_layer (guchar *buffer,
                      gint width,
                      gint left,
                      gint components,
                      gint pass,
                      GimvImageLayerMode mode,
                      guchar *rgbbuffer)
{
   gint i, j, k;
   static gint alpha, cur_red, cur_green, cur_blue;
   static guchar src1[4], src2[4], dest[4];

   if (pass <= 0) {
      cur_red   = bgcolor_red;
      cur_green = bgcolor_green;
      cur_blue  = bgcolor_blue;
   }

   for (i = 0, j = 0, k = 0; i < width; i++) {
      if (pass > 0) {
         cur_red   = rgbbuffer[k];
         cur_green = rgbbuffer[k + 1];
         cur_blue  = rgbbuffer[k + 2];
      }

      switch (components) {
      case 1:
         rgbbuffer[k++] = buffer[j];
         rgbbuffer[k++] = buffer[j];
         rgbbuffer[k++] = buffer[j++];
         break;

      case 2:
         if (pass > 0 && mode > 0) {
            src1[0] = cur_red;
            src1[1] = cur_green;
            src1[2] = cur_blue;
            src1[3] = 255;
            src2[0] = buffer[j];
            src2[1] = buffer[j];
            src2[2] = buffer[j];
            src2[3] = buffer[j + 1];
            pixel_apply_layer_mode(src1, src2, dest, 4, 4, TRUE, TRUE, mode);
            alpha = dest[3];
            rgbbuffer[k++] = dest[0] * alpha / 255 + cur_red   * (255 - alpha) / 255;
            rgbbuffer[k++] = dest[1] * alpha / 255 + cur_green * (255 - alpha) / 255;
            rgbbuffer[k++] = dest[2] * alpha / 255 + cur_blue  * (255 - alpha) / 255;
         } else {
            alpha = buffer[j + 1];
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_red   * (255 - alpha) / 255;
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_green * (255 - alpha) / 255;
            rgbbuffer[k++] = buffer[j] * alpha / 255 + cur_blue  * (255 - alpha) / 255;
         }
         j += 2;
         break;

      case 3:
         rgbbuffer[k++] = buffer[j++];
         rgbbuffer[k++] = buffer[j++];
         rgbbuffer[k++] = buffer[j++];
         break;

      case 4:
         if (pass > 0 && mode > 0) {
            src1[0] = cur_red;
            src1[1] = cur_green;
            src1[2] = cur_blue;
            src1[3] = 255;
            src2[0] = buffer[j];
            src2[1] = buffer[j + 1];
            src2[2] = buffer[j + 2];
            src2[3] = buffer[j + 3];
            pixel_apply_layer_mode(src1, src2, &buffer[j], 4, 4, TRUE, TRUE, mode);
         }
         alpha = buffer[j + 3];
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_red   * (255 - alpha) / 255;
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_green * (255 - alpha) / 255;
         rgbbuffer[k++] = buffer[j++] * alpha / 255 + cur_blue  * (255 - alpha) / 255;
         j++;
         break;

      default:
         break;
      }
   }

   return FALSE;
}


GimvImage *
gimv_image_rgba2rgb (GimvImage *image,
                     gint       bg_red,
                     gint       bg_green,
                     gint       bg_blue,
                     gboolean   ignore_alpha)
{
   GimvImage *dest_image = NULL;
   gint width, height, i, j;
   guchar *src, *dest;

   /* set bg color */
   if (bg_red >= 0 && bg_red < 256)
      bgcolor_red = bg_red;
   if (bg_green >= 0 && bg_green < 256)
      bgcolor_green = bg_green;
   if (bg_blue >= 0 && bg_blue < 256)
      bgcolor_blue = bg_blue;

   if (!gimv_image_has_alpha (image)) goto ERROR;

   width  = gimv_image_width  (image);
   height = gimv_image_height (image);
   src = gimv_image_get_pixels (image);

   dest = g_malloc (width * height * 3 * sizeof (guchar));

   for (i = 0; i < height; i++) {
      if (ignore_alpha) {
         for (j = 0; j < width; j++) {
            gint pos, src_pos;
            src_pos  = (j + i * width) * 4;
            pos = (j + i * width) * 3;
            dest [pos]     = src [src_pos];
            dest [pos + 1] = src [src_pos + 1];
            dest [pos + 2] = src [src_pos + 2];
         }
      } else {
        gimv_image_add_layer (src + (width * i * 4),
                               width, 0, 4,
                               0, GIMV_IMAGE_NORMAL_MODE,
                               dest + (width * i * 3));
      }
   }

   dest_image = gimv_image_create_from_data (dest, width, height, FALSE);

ERROR:
   /* reset bg color to default value */
   bgcolor_red   = 255;
   bgcolor_green = 255;
   bgcolor_blue  = 255;

   return dest_image;
}


/*
 * Returns a copy of pixbuf src rotated 90 degrees clockwise or 90 
 * counterclockwise.
 */
static GdkPixbuf *
pixbuf_copy_rotate_90 (GdkPixbuf *src, 
                       gboolean counter_clockwise)
{
   GdkPixbuf *dest;
   gint has_alpha;
   gint src_w, src_h, src_rs;
   gint dest_w, dest_h, dest_rs;
   guchar *src_pix,  *src_p;
   guchar *dest_pix, *dest_p;
   gint i, j;
   gint a;

   if (!src) return NULL;

   src_w = gdk_pixbuf_get_width (src);
   src_h = gdk_pixbuf_get_height (src);
   has_alpha = gdk_pixbuf_get_has_alpha (src);
   src_rs = gdk_pixbuf_get_rowstride (src);
   src_pix = gdk_pixbuf_get_pixels (src);

   dest_w = src_h;
   dest_h = src_w;
   dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, dest_w, dest_h);
   dest_rs = gdk_pixbuf_get_rowstride (dest);
   dest_pix = gdk_pixbuf_get_pixels (dest);

   a = (has_alpha ? 4 : 3);

   for (i = 0; i < src_h; i++) {
      src_p = src_pix + (i * src_rs);
      for (j = 0; j < src_w; j++) {
         if (counter_clockwise)
            dest_p = dest_pix + ((dest_h - j - 1) * dest_rs) + (i * a);
         else
            dest_p = dest_pix + (j * dest_rs) + ((dest_w - i - 1) * a);

         *(dest_p++) = *(src_p++);	/* r */
         *(dest_p++) = *(src_p++);	/* g */
         *(dest_p++) = *(src_p++);	/* b */
         if (has_alpha) *(dest_p) = *(src_p++);	/* a */
      }
   }

   return dest;
}


/*
 * Returns a copy of pixbuf mirrored and or flipped.
 * TO do a 180 degree rotations set both mirror and flipped TRUE
 * if mirror and flip are FALSE, result is a simple copy.
 */
static GdkPixbuf *
pixbuf_copy_mirror (GdkPixbuf *src, 
                    gboolean mirror, 
                    gboolean flip)
{
   GdkPixbuf *dest;
   gint has_alpha;
   gint w, h;
   gint    src_rs,   dest_rs;
   guchar *src_pix, *dest_pix;
   guchar *src_p,   *dest_p;
   gint i, j;
   gint a;

   if (!src) return NULL;

   w = gdk_pixbuf_get_width (src);
   h = gdk_pixbuf_get_height (src);
   has_alpha = gdk_pixbuf_get_has_alpha (src);
   src_rs = gdk_pixbuf_get_rowstride (src);
   src_pix = gdk_pixbuf_get_pixels (src);

   dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
   dest_rs = gdk_pixbuf_get_rowstride (dest);
   dest_pix = gdk_pixbuf_get_pixels (dest);

   a = has_alpha ? 4 : 3;

   for (i = 0; i < h; i++)	{
      src_p = src_pix + (i * src_rs);
      if (flip)
         dest_p = dest_pix + ((h - i - 1) * dest_rs);
      else
         dest_p = dest_pix + (i * dest_rs);

      if (mirror) {
         dest_p += (w - 1) * a;
         for (j = 0; j < w; j++) {
            *(dest_p++) = *(src_p++);	/* r */
            *(dest_p++) = *(src_p++);	/* g */
            *(dest_p++) = *(src_p++);	/* b */
            if (has_alpha) *(dest_p) = *(src_p++);	/* a */
            dest_p -= (a + 3);
         }
      } else {
         for (j = 0; j < w; j++) {
            *(dest_p++) = *(src_p++);	/* r */
            *(dest_p++) = *(src_p++);	/* g */
            *(dest_p++) = *(src_p++);	/* b */
            if (has_alpha) *(dest_p++) = *(src_p++);	/* a */
         }
      }
   }

   return dest;
}
