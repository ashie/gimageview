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

#include "pixbuf_anim.h"

static gint gimv_anim_gdk_pixbuf_iterate      (GimvAnim *anim);
static gint gimv_anim_gdk_pixbuf_get_interval (GimvAnim *anim);
static void gimv_anim_gdk_pixbuf_delete_priv  (GimvAnim *anim);

static GdkPixbufAnimationIter *gimv_anim_gdk_pixbuf_set_iterator (GimvAnim *anim);
static GdkPixbufAnimationIter *gimv_anim_gdk_pixbuf_get_iterator (GimvAnim *anim);


/* virtual function table */
static GimvAnimFuncTable gdk_pixbuf_vf_table = {
   get_length   : NULL,
   get_idx      : NULL,
   get_interval : gimv_anim_gdk_pixbuf_get_interval,
   iterate      : gimv_anim_gdk_pixbuf_iterate,
   seek         : NULL,
   delete       : gimv_anim_gdk_pixbuf_delete_priv,
};


static GdkPixbufAnimationIter *
gimv_anim_gdk_pixbuf_set_iterator (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GdkPixbufAnimationIter *iter;

   g_return_val_if_fail (image, NULL);
   g_return_val_if_fail (anim->anim, NULL);
   if (!GIMV_IS_ANIM (image)) return NULL;

   iter = gdk_pixbuf_animation_get_iter (anim->anim, NULL);
   g_return_val_if_fail (iter, NULL);
   g_object_ref (iter);
   g_object_set_data_full (G_OBJECT (anim->anim),
                           "GimvAnim::iterator", iter,
                           (GDestroyNotify) g_object_unref);

   return iter;
}

static GdkPixbufAnimationIter *
gimv_anim_gdk_pixbuf_get_iterator (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GdkPixbufAnimationIter *iter;

   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (anim->anim, FALSE);
   if (!GIMV_IS_ANIM (image)) return FALSE;

   iter = g_object_get_data (G_OBJECT (anim->anim), "GimvAnim::iterator");
   if (iter) return iter;

   return gimv_anim_gdk_pixbuf_set_iterator (anim);
}


static gint
gimv_anim_gdk_pixbuf_iterate (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GdkPixbufAnimationIter *iter;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   iter = gimv_anim_gdk_pixbuf_get_iterator (anim);
   g_return_val_if_fail (iter, -1);

   if (gdk_pixbuf_animation_iter_advance (iter, NULL)) {
      GimvImageAngle angle;
      anim->current_frame_idx++;
      g_object_unref (image->image);
      image->image = gdk_pixbuf_animation_iter_get_pixbuf (iter);
      g_object_ref (image->image);

      /* restore angle */
      angle = image->angle;
      image->angle = GIMV_IMAGE_ROTATE_0;
      gimv_image_rotate (image, angle);
   } else {
   }

   return anim->current_frame_idx;
}


static gint
gimv_anim_gdk_pixbuf_get_interval (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GdkPixbufAnimationIter *iter;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   iter = gimv_anim_gdk_pixbuf_get_iterator (anim);
   g_return_val_if_fail (iter, -1);
   return gdk_pixbuf_animation_iter_get_delay_time (iter);
}


void
gimv_anim_gdk_pixbuf_delete_priv (GimvAnim *anim)
{
   g_object_unref (anim->anim);
}

GimvImage *
gimv_anim_new_from_gdk_pixbuf_animation (GdkPixbufAnimation *pixbuf_anim)
{
   GdkPixbuf *pixbuf;
   GimvImage *image = NULL;
   GimvAnim  *anim;

   if (gdk_pixbuf_animation_is_static_image (pixbuf_anim)) {
      image = gimv_image_new ();
      pixbuf = gdk_pixbuf_animation_get_static_image (pixbuf_anim);
      image->image = pixbuf;
   } else {
      GdkPixbufAnimationIter *iter;

      anim = gimv_anim_new ();
      image = (GimvImage *) anim;
      anim->anim = pixbuf_anim;
      anim->table = &gdk_pixbuf_vf_table;
      gimv_anim_gdk_pixbuf_set_iterator (anim);
      iter = gimv_anim_gdk_pixbuf_get_iterator (anim);
      if (!iter) {
         g_object_unref (G_OBJECT (image));
         return NULL;
      }
      image->image = gdk_pixbuf_animation_iter_get_pixbuf (iter);
   }

   if (image->image) {
      g_object_ref (image->image);
   } else {
      g_object_unref (G_OBJECT (image));
      return NULL;
   }

   return image;
}
