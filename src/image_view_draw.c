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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include "cursors.h"
#include "gimv_anim.h"
#include "gimv_image_view.h"
#include "gimv_thumb_cache.h"
#include "prefs.h"


static gboolean cb_image_configure         (GtkWidget         *widget,
                                            GdkEventConfigure *event,
                                            GimvImageView     *iv);
static gboolean cb_image_expose            (GtkWidget         *widget,
                                            GdkEventExpose    *event,
                                            GimvImageView     *iv);

/* virtual functions */
static GtkWidget   *imageview_draw_create               (GimvImageView *iv);
static void         imageview_draw_create_thumbnail     (GimvImageView *iv,
                                                         const gchar *type);

static gboolean     imageview_draw_is_playable          (GimvImageView *iv,
                                                         GimvImageInfo *info);
static void         imageview_animation_play            (GimvImageView *iv);
static void         imageview_animation_stop            (GimvImageView *iv);
#if 0
static void         imageview_animation_pause           (GimvImageView *iv);
#endif
static GimvImageViewPlayableStatus
                    imageview_draw_get_status           (GimvImageView *iv);


static GimvImageViewPlayableIF imageview_draw_playable_table = {
   is_playable_fn:      imageview_draw_is_playable,
   is_seekable_fn:      NULL,
   play_fn:             imageview_animation_play,
   stop_fn:             imageview_animation_stop,
#if 0
   pause_fn:            imageview_animation_pause,
#else
   pause_fn:            NULL,
#endif
   forward_fn:          NULL,
   reverse_fn:          NULL,
   seek_fn:             NULL,
   eject_fn:            NULL,
   get_status_fn:       imageview_draw_get_status,
   get_length_fn:       NULL,
   get_position_fn:     NULL,
};


GimvImageViewPlugin imageview_draw_vfunc_table = {
   label:               GIMV_IMAGE_VIEW_DEFAULT_VIEW_MODE,
   priority_hint:       G_PRIORITY_LOW,
   is_supported_fn:     NULL,
   create_fn:           imageview_draw_create,
   create_thumbnail_fn: imageview_draw_create_thumbnail,
   fullscreen_fn:       NULL,

   scalable:            NULL,
   rotatable:           NULL,
   playable:            &imageview_draw_playable_table,
};


static GHashTable *animation_id_table        = NULL;
static GHashTable *create_thumbnail_id_table = NULL;


/*****************************************************************************
 *
 *   callback functions
 *
 *****************************************************************************/
static void
cb_destroy (GtkWidget *widget, GimvImageView *iv)
{
   g_return_if_fail (iv);

   imageview_animation_stop (iv);
}


static void
cb_load_end_create_thumbnail (GimvImageView *iv, GimvImageInfo *info,
                              gboolean cancel, gpointer data)
{
   GimvImage *imcache;
   gchar *filename;
   gboolean free_buf = GPOINTER_TO_INT (data);
   gpointer id_p;
   guint id;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   id_p = g_hash_table_lookup (create_thumbnail_id_table, iv);
   id = GPOINTER_TO_UINT (id_p);
   if (id > 0)
      g_signal_handler_disconnect (GTK_OBJECT (iv), id);
   g_hash_table_remove (create_thumbnail_id_table, iv);

   if (cancel) return;
   if (!iv->image) return;
   if (iv->info != iv->loader->info) return;

   filename = gimv_image_info_get_path_with_archive (iv->info);

   /* FIXME: conf.cache_write_type is hard coded */
   imcache = gimv_thumb_cache_save (filename,
                                    conf.cache_write_type,
                                    iv->image,
                                    iv->info);

   g_free (filename);

   if (free_buf)
      gimv_image_view_free_image_buf (iv);

   if (imcache) {
      gimv_image_unref (imcache);
      g_signal_emit_by_name (G_OBJECT (iv),
                             "thumbnail_created",
                             iv->info);
   }
}


static void
cb_draw_area_map (GtkWidget *widget, GimvImageView *iv)
{
   if (iv->bg_color) {
      gimv_image_view_set_bg_color (iv,
                                    iv->bg_color->red,
                                    iv->bg_color->green,
                                    iv->bg_color->blue);
   }

   /* set cursor */
   if (!iv->cursor)
      iv->cursor = cursor_get (iv->draw_area->window, CURSOR_HAND_OPEN);
   gdk_window_set_cursor (iv->draw_area->window, iv->cursor);
}


static gboolean
cb_image_configure (GtkWidget *widget, GdkEventConfigure *event, GimvImageView *iv)
{
   gint width, height;
   gint fwidth, fheight;
   gint x_pos, y_pos;

   gimv_image_view_get_view_position (iv, &x_pos, &y_pos);
   gimv_image_view_get_image_size (iv, &width, &height);
   gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);

   if (fwidth < width) {
      if (x_pos < 0 || x_pos < 0 - fwidth || x_pos > width)
         x_pos = 0;
   } else {
      x_pos = (width - fwidth) / 2;
   }

   if (fheight < height) {
      if (y_pos < 0 || y_pos < 0 - fheight || y_pos > height)
         y_pos = 0;
   } else {

      y_pos = (height - fheight) / 2;
   }

   gimv_image_view_set_view_position (iv, x_pos, y_pos);
   gimv_image_view_draw_image (iv);

   return TRUE;
}


static gboolean
cb_image_expose (GtkWidget *widget, GdkEventExpose *event, GimvImageView *iv)
{
   gimv_image_view_draw_image (iv);
   return TRUE;
}



/*****************************************************************************
 *
 *   other private functions
 *
 *****************************************************************************/
static gboolean
timeout_animation (gpointer data)
{
   GimvImageView *iv = data;
   gint idx, interval;

   if (!iv->image) goto END;
   if (!GIMV_IS_ANIM (iv->image)) goto END;

   idx = gimv_anim_iterate ((GimvAnim *) iv->image);

   /* repeat */
   if (idx < 0) {
      if (!gimv_anim_seek ((GimvAnim *) iv->image, 0))
         goto END;
   }

   gimv_image_view_show_image (iv);

   interval = gimv_anim_get_interval ((GimvAnim *) iv->image);
   if (interval > 0) {
      guint timer = gtk_timeout_add (interval, timeout_animation, iv);
      g_hash_table_insert (animation_id_table,
                           iv, GUINT_TO_POINTER (timer));
   } else {
      goto END;
   }

   return FALSE;

END:
   g_hash_table_remove (animation_id_table, iv);
   gimv_image_view_playable_set_status (iv, GimvImageViewPlayableStop);
   return FALSE;
}


static gboolean
idle_animation_play (gpointer data)
{
   GimvImageView *iv = data;
   gint interval;

   if (!GIMV_IS_IMAGE_VIEW (iv)) goto END;
   if (!iv->info) goto END;

   if (!gimv_image_info_is_animation (iv->info)) goto END;

   imageview_animation_stop (iv);

   interval = gimv_anim_get_interval ((GimvAnim *) iv->image);
   if (interval > 0) {
      guint timer = gtk_timeout_add (interval, timeout_animation, iv);
      g_hash_table_insert (animation_id_table,
                           iv, GUINT_TO_POINTER (timer));
      gimv_image_view_playable_set_status (iv, GimvImageViewPlayablePlay);
   } else {
      goto END;
   }

   return FALSE;

END:
   g_hash_table_remove (animation_id_table, iv);
   gimv_image_view_playable_set_status (iv, GimvImageViewPlayableStop);
   return FALSE;
}



/*****************************************************************************
 *
 *   Virtual functions
 *
 *****************************************************************************/
static GtkWidget *
imageview_draw_create (GimvImageView *iv)
{
   GtkWidget *widget;

   if (!animation_id_table)
      animation_id_table
         = g_hash_table_new (g_direct_hash, g_direct_equal);

   if (!create_thumbnail_id_table)
      create_thumbnail_id_table
         = g_hash_table_new (g_direct_hash, g_direct_equal);

   widget = gtk_drawing_area_new ();

   g_signal_connect       (G_OBJECT (widget), "destroy",
                           G_CALLBACK (cb_destroy), iv);
   g_signal_connect_after (G_OBJECT (widget), "map",
                           G_CALLBACK (cb_draw_area_map), iv);
   g_signal_connect       (G_OBJECT (widget), "configure_event",
                           G_CALLBACK (cb_image_configure), iv);
   g_signal_connect       (G_OBJECT (widget), "expose_event",
                           G_CALLBACK (cb_image_expose), iv);

   gtk_widget_add_events (widget,
                          GDK_FOCUS_CHANGE
                          | GDK_BUTTON_PRESS_MASK | GDK_2BUTTON_PRESS
                          | GDK_KEY_PRESS | GDK_KEY_RELEASE
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

   return widget;
}


static void
imageview_draw_create_thumbnail (GimvImageView *iv, const gchar *cache_write_type)
{
   if (!iv->image) {
      gpointer id_p;
      guint id;

      id_p = g_hash_table_lookup (create_thumbnail_id_table, iv);
      id = GPOINTER_TO_UINT (id_p);
      if (id > 0)
         g_signal_handler_disconnect (GTK_OBJECT (iv), id);
      id = g_signal_connect (G_OBJECT (iv), "load_end",
                             G_CALLBACK (cb_load_end_create_thumbnail),
                             GINT_TO_POINTER (TRUE));
      g_hash_table_insert (create_thumbnail_id_table,
                           iv, GUINT_TO_POINTER (id));
      gimv_image_view_load_image_buf (iv);
   } else {
      cb_load_end_create_thumbnail (iv, iv->info,
                                    FALSE, GINT_TO_POINTER (FALSE));
   }
}


static gboolean
imageview_draw_is_playable (GimvImageView *iv, GimvImageInfo *info)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);
   if (!info) return FALSE;

   return gimv_image_info_is_animation (info);
}


static void
imageview_animation_play (GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   gtk_idle_add (idle_animation_play, iv);
}


#if 0
static void
imageview_animation_stop (GimvImageView *iv)
{
   g_return_if_fail (iv);

   imageview_animation_pause (iv);

   if (!iv->info) return;
   if (!gimv_image_info_is_animation (iv->info)) return;

   gimv_anim_seek ((GimvAnim *) iv->image, 0);

   imageview_show_image (iv);

   imageview_playable_set_status (iv, GimvImageViewPlayableStop);
}


static void
imageview_animation_pause (GimvImageView *iv)
{
   gpointer id_p;
   guint id;

   g_return_if_fail (iv);

   if (!animation_id_table) return;

   id_p = g_hash_table_lookup (animation_id_table, iv);
   id = GPOINTER_TO_UINT (id_p);
   if (id > 0)
      gtk_timeout_remove (id);
   g_hash_table_remove (animation_id_table, iv);

   imageview_playable_set_status (iv, GimvImageViewPlayablePause);
}

#else

static void
imageview_animation_stop (GimvImageView *iv)
{
   gpointer id_p;
   guint id;

   g_return_if_fail (iv);

   if (!animation_id_table) return;

   id_p = g_hash_table_lookup (animation_id_table, iv);
   id = GPOINTER_TO_UINT (id_p);
   if (id > 0)
      gtk_timeout_remove (id);
   g_hash_table_remove (animation_id_table, iv);

   gimv_image_view_playable_set_status (iv, GimvImageViewPlayableStop);
}

#endif


static GimvImageViewPlayableStatus
imageview_draw_get_status (GimvImageView *iv)
{
   gpointer timer_p;
   guint timer;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), GimvImageViewPlayableDisable);

   if (!iv->info || !gimv_image_info_is_animation (iv->info))
      return GimvImageViewPlayableDisable;

   timer_p = g_hash_table_lookup (animation_id_table, iv);
   timer = GPOINTER_TO_UINT (timer_p);
   if (timer > 0)
      return GimvImageViewPlayablePlay;
   else
      return GimvImageViewPlayableStop;
}
