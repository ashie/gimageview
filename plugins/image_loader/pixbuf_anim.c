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

#ifdef HAVE_GDK_PIXBUF

static gint gimv_anim_gdk_pixbuf_iterate      (GimvAnim *anim);
static gint gimv_anim_gdk_pixbuf_get_interval (GimvAnim *anim);
static void gimv_anim_gdk_pixbuf_delete_priv  (GimvAnim *anim);

#ifdef USE_GTK2
static GdkPixbufAnimationIter *gimv_anim_gdk_pixbuf_set_iterator (GimvAnim *anim);
static GdkPixbufAnimationIter *gimv_anim_gdk_pixbuf_get_iterator (GimvAnim *anim);
#else  /* USE_GTK2 */
static gint     gimv_anim_gdk_pixbuf_get_num_frames        (GimvAnim *anim);
static gint     gimv_anim_gdk_pixbuf_get_current_frame_idx (GimvAnim *anim);
static gboolean gimv_anim_gdk_pixbuf_seek                  (GimvAnim *anim,
                                                            gint       idx);
#endif /* USE_GTK2 */


/* virtual function table */
#ifdef USE_GTK2
static GimvAnimFuncTable gdk_pixbuf_vf_table = {
   get_length   : NULL,
   get_idx      : NULL,
   get_interval : gimv_anim_gdk_pixbuf_get_interval,
   iterate      : gimv_anim_gdk_pixbuf_iterate,
   seek         : NULL,
   delete       : gimv_anim_gdk_pixbuf_delete_priv,
};
#else /* USE_GTK2 */
static GimvAnimFuncTable gdk_pixbuf_vf_table = {
   get_length   : gimv_anim_gdk_pixbuf_get_num_frames,
   get_idx      : gimv_anim_gdk_pixbuf_get_current_frame_idx,
   get_interval : gimv_anim_gdk_pixbuf_get_interval,
   iterate      : gimv_anim_gdk_pixbuf_iterate,
   seek         : gimv_anim_gdk_pixbuf_seek,
   delete       : gimv_anim_gdk_pixbuf_delete_priv,
};
#endif /* USE_GTK2 */


#ifdef USE_GTK2
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
      gdk_pixbuf_unref (image->image);
      image->image = gdk_pixbuf_animation_iter_get_pixbuf (iter);
      gdk_pixbuf_ref (image->image);

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

#else /* USE_GTK2 */

static gint
gimv_anim_gdk_pixbuf_get_num_frames (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   return gdk_pixbuf_animation_get_num_frames (anim->anim);
}


static gint
gimv_anim_gdk_pixbuf_get_current_frame_idx (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   return anim->current_frame_idx;
}


static gint
gimv_anim_gdk_pixbuf_iterate (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GList *frames, *next, *prev;
   GdkPixbufFrameAction action;
   GdkPixbuf *prev_pixbuf, *next_pixbuf;
   gint dest_width, dest_height;
   gint x_offset, y_offset;
   GimvImageAngle angle;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   frames = gdk_pixbuf_animation_get_frames (anim->anim);
   g_return_val_if_fail (frames, -1);

   prev = g_list_nth (frames, anim->current_frame_idx);
   next = g_list_nth (frames, anim->current_frame_idx + 1);
   if (!next) return -1;

   next_pixbuf = gdk_pixbuf_frame_get_pixbuf (next->data);
   dest_width = gdk_pixbuf_get_width (next_pixbuf);
   dest_height = gdk_pixbuf_get_height (next_pixbuf);
   x_offset = gdk_pixbuf_frame_get_x_offset (next->data);
   y_offset = gdk_pixbuf_frame_get_y_offset (next->data);

   action = gdk_pixbuf_frame_get_action (next->data);

   prev_pixbuf = gdk_pixbuf_frame_get_pixbuf (next->data);
   if (!prev && action == GDK_PIXBUF_FRAME_REVERT) return -1;

   angle = image->angle;
   gimv_image_rotate (image, 0);

   /* FIXME (but it is gdk-pixbuf-1.0's probrem) */
   switch (action) {
   case GDK_PIXBUF_FRAME_RETAIN:
      gdk_pixbuf_composite (next_pixbuf,
                            image->image,
                            x_offset, y_offset,
                            dest_width, dest_height,
                            (double) x_offset,  (double) y_offset,
                            1.0, 1.0,
                            GDK_INTERP_NEAREST,
                            255);
      break;
   case GDK_PIXBUF_FRAME_REVERT:
   case GDK_PIXBUF_FRAME_DISPOSE:
      gdk_pixbuf_unref (image->image);
      image->image = next_pixbuf;
      gdk_pixbuf_ref (image->image);
      break;
   default:
      return anim->current_frame_idx;
      break;
   }

   gimv_image_rotate (image, angle);

   anim->current_frame_idx++;

   return anim->current_frame_idx;
}


static gboolean
gimv_anim_gdk_pixbuf_seek (GimvAnim *anim, gint idx)
{
   GimvImage *image = (GimvImage *) anim;
   GList *frames, *frame;
   GimvImageAngle angle;

   g_return_val_if_fail (image, FALSE);
   g_return_val_if_fail (anim->anim, FALSE);


   frames = gdk_pixbuf_animation_get_frames (anim->anim);
   g_return_val_if_fail (frames, FALSE);

   frame = g_list_nth (frames, idx);
   if (!frame) return FALSE;

   anim->current_frame_idx = idx;
   gdk_pixbuf_unref (image->image);

   if (gdk_pixbuf_frame_get_action (frame->data) == GDK_PIXBUF_FRAME_RETAIN) {
      GdkPixbuf *pixbuf = gdk_pixbuf_frame_get_pixbuf (frame->data);
      image->image = gdk_pixbuf_copy (pixbuf);
   } else {
      image->image = gdk_pixbuf_frame_get_pixbuf (frame->data);
      gdk_pixbuf_ref (image->image);
   }

   angle = image->angle;
   image->angle = 0;
   gimv_image_rotate (image, angle);

   return TRUE;
}


static gint
gimv_anim_gdk_pixbuf_get_interval (GimvAnim *anim)
{
   GimvImage *image = (GimvImage *) anim;
   GList *frames, *frame;

   g_return_val_if_fail (image, -1);
   g_return_val_if_fail (anim->anim, -1);

   frames = gdk_pixbuf_animation_get_frames (anim->anim);
   g_return_val_if_fail (frames, FALSE);

   frame = g_list_nth (frames, anim->current_frame_idx);
   if (!frame) return FALSE;

   return MAX (10, gdk_pixbuf_frame_get_delay_time (frame->data) * 10);
}


void
gimv_anim_gdk_pixbuf_delete_priv (GimvAnim *anim)
{
   gdk_pixbuf_animation_unref (anim->anim);
}
#endif /* USE_GTK2 */


GimvImage *
gimv_anim_new_from_gdk_pixbuf_animation (GdkPixbufAnimation *pixbuf_anim)
{
   GdkPixbuf *pixbuf;
   GimvImage *image = NULL;
   GimvAnim  *anim;

#ifdef USE_GTK2

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
         gimv_image_unref (image);
         return NULL;
      }
      image->image = gdk_pixbuf_animation_iter_get_pixbuf (iter);
   }

   if (image->image) {
      gdk_pixbuf_ref (image->image);
   } else {
      gimv_image_unref (image);
      return NULL;
   }

#else
   {
      GList *frames = gdk_pixbuf_animation_get_frames (pixbuf_anim);
      gint num = gdk_pixbuf_animation_get_num_frames (pixbuf_anim);

      if (frames && num > 1) {
         anim = gimv_anim_new ();
         image = (GimvImage *) anim;
         anim->current_frame_idx = 0;

         pixbuf = gdk_pixbuf_frame_get_pixbuf (frames->data);
         image->image = gdk_pixbuf_copy (pixbuf);

         anim->anim = pixbuf_anim;
         gdk_pixbuf_animation_ref (anim->anim);
         anim->table = &gdk_pixbuf_vf_table;

      } else if (frames) {
         image = gimv_image_new ();
         image->image = gdk_pixbuf_frame_get_pixbuf (frames->data);
         gdk_pixbuf_ref (image->image);
      }
   }
#endif

   return image;
}

#endif /* HAVE_GDK_PIXBUF */
