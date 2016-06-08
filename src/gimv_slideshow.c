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

#include "gimv_image_win.h"
#include "prefs.h"
#include "gimv_slideshow.h"

G_DEFINE_TYPE (GimvSlideshow, gimv_slideshow, G_TYPE_OBJECT)

#define GIMV_SLIDESHOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMV_TYPE_SLIDESHOW, GimvSlideshowPrivate))

typedef struct GimvSlideshowPrivate_Tag
{
   GimvImageWin *iw;
   GList        *filelist;
   GList        *current;
   guint         interval;
   gboolean      repeat;
} GimvSlideshowPrivate;

typedef struct ChangeImageData_Tag
{
   GimvSlideshow *slideshow;
   GimvImageWin  *iw;
   GimvImageInfo *info;
} ChangeImageData;

static void     gimv_slideshow_dispose (GObject       *object);
static gint     idle_slideshow_unref   (gpointer       data);
static GList   *next_image             (GimvImageView *iv,
                                        gpointer       list_owner,
                                        GList         *current,
                                        gpointer       data);
static GList   *prev_image             (GimvImageView *iv,
                                        gpointer       list_owner,
                                        GList         *current,
                                        gpointer       data);
static void     remove_list            (GimvImageView *iv,
                                        gpointer       list_owner,
                                        gpointer       data);

static gint
idle_slideshow_unref (gpointer data)
{
   GimvSlideshow *slideshow = data;

   g_object_unref (G_OBJECT (slideshow));

   return 0;
}

static gboolean
change_image_idle (gpointer user_data)
{
   ChangeImageData *data = user_data;

   gimv_image_win_change_image (data->iw, data->info);

   return FALSE;
}

static GList *
next_image (GimvImageView *iv,
            gpointer list_owner,
            GList *current,
            gpointer data)
{
   GimvImageWin *iw = data;
   GList *next, *node;
   GimvSlideshow *slideshow = list_owner;
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (priv->filelist, current->data);
   g_return_val_if_fail (node, NULL);

   next = g_list_next (current);
   if (!next && priv->repeat)
      next = g_list_first (current);
   else if (!next)
      next = current;
   g_return_val_if_fail (next, NULL);

   if (next != current) {
      ChangeImageData *change_data = g_new0 (ChangeImageData, 1);

      change_data->slideshow = slideshow;
      change_data->iw = iw;
      change_data->info = next->data;
      gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                         change_image_idle,
                         NULL,
                         change_data,
                         (GtkDestroyNotify) g_free);
      priv->current = next;
   }

   return next;
}

static GList *
prev_image (GimvImageView *iv,
            gpointer list_owner,
            GList *current,
            gpointer data)
{
   GimvImageWin *iw = data;
   GList *prev, *node;
   GimvSlideshow *slideshow = list_owner;
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (priv->filelist, current->data);
   g_return_val_if_fail (node, NULL);

   prev = g_list_previous (current);
   if (!prev && priv->repeat)
      prev = g_list_last (current);
   else if (!prev)
      prev = current;
   g_return_val_if_fail (prev, NULL);

   if (prev != current) {
      ChangeImageData *change_data = g_new0 (ChangeImageData, 1);

      change_data->slideshow = slideshow;
      change_data->iw = iw;
      change_data->info = prev->data;
      gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                         change_image_idle,
                         NULL,
                         change_data,
                         (GtkDestroyNotify) g_free);
      priv->current = prev;
   }

   return prev;
}

static GList *
nth_image (GimvImageView *iv,
           gpointer list_owner,
           GList *list,
           guint nth,
           gpointer data)
{
   GimvImageWin *iw = data;
   GList *node;
   GimvSlideshow *slideshow = list_owner;
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);
   ChangeImageData *change_data;

   g_return_val_if_fail (iv, NULL);

   node = g_list_nth (priv->filelist, nth);
   g_return_val_if_fail (node, NULL);

   change_data = g_new0 (ChangeImageData, 1);
   change_data->slideshow = slideshow;
   change_data->iw = iw;
   change_data->info = node->data;
   gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                      change_image_idle,
                      NULL,
                      change_data,
                      (GtkDestroyNotify) g_free);
   priv->current = node;

   return node;
}

static void
remove_list (GimvImageView *iv, gpointer list_owner, gpointer data)
{
   GimvSlideshow *slideshow = list_owner;

   gtk_idle_add (idle_slideshow_unref, slideshow);
}

static void
cb_show_fullscreen (GimvImageWin *iw, GimvSlideshow *slideshow)
{
}

static void
cb_hide_fullscreen (GimvImageWin *iw, GimvSlideshow *slideshow)
{
   gtk_widget_destroy (GTK_WIDGET (iw));
}

static void
gimv_slideshow_class_init (GimvSlideshowClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass *) klass;

   gobject_class->dispose = gimv_slideshow_dispose;

   g_type_class_add_private (gobject_class, sizeof (GimvSlideshowPrivate));
}

static void
gimv_slideshow_init (GimvSlideshow *slideshow)
{
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);
   priv->iw       = NULL;
   priv->filelist = NULL;
   priv->interval = conf.slideshow_interval * 1000;
   priv->repeat   = conf.slideshow_repeat;
}

GimvSlideshow *
gimv_slideshow_new (void)
{
   return GIMV_SLIDESHOW (g_object_new (GIMV_TYPE_SLIDESHOW, NULL));
}

GimvSlideshow *
gimv_slideshow_new_with_filelist (GList *filelist, GList *start)
{
   GimvSlideshow *slideshow;
   GimvSlideshowPrivate *priv;

   g_return_val_if_fail (filelist, NULL);

   slideshow = gimv_slideshow_new ();
   priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);
   priv->filelist = g_list_first (filelist);
   priv->current  = start ? start : filelist;

   return slideshow;
}

static void
gimv_slideshow_dispose (GObject *object)
{
   GimvSlideshow *slideshow = GIMV_SLIDESHOW (object);
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   g_list_foreach (priv->filelist, (GFunc) gimv_image_info_unref, NULL);
   g_list_free (g_list_first (priv->filelist));
}

static GimvImageWin *
gimv_slideshow_open_window (GimvSlideshow *slideshow)
{
   GimvSlideshowPrivate *priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);
   GimvImageWin *iw;
   GimvImageView *iv;
   GList *current;

   g_return_val_if_fail (priv->filelist, NULL);

   if (priv->iw)
      return priv->iw;

   /* current = g_list_first (slideshow->filelist); */
   current = priv->current ? priv->current
                           : priv->filelist;
   g_return_val_if_fail (current, NULL);

   priv->iw = GIMV_IMAGE_WIN (gimv_image_win_new (NULL));
   g_return_val_if_fail (priv->iw, NULL);

   iw = priv->iw;
   iv = iw->iv;

   /* override some parameters */
   g_object_set(G_OBJECT(iv),
                "x_scale",           conf.slideshow_img_scale,
                "y_scale",           conf.slideshow_img_scale,
                "default_zoom",      conf.slideshow_zoom,
                "default_rotation",  conf.slideshow_rotation,
                "keep_aspect",       conf.slideshow_keep_aspect,
                NULL);

   gimv_image_win_change_image (GIMV_IMAGE_WIN (iw), current->data);

   gimv_image_win_show_menubar   (iw, conf.slideshow_menubar);
   gimv_image_win_show_toolbar   (iw, conf.slideshow_toolbar);
   gimv_image_win_show_player    (iw, conf.slideshow_player);
   gimv_image_win_show_statusbar (iw, conf.slideshow_statusbar);
   switch (conf.slideshow_screen_mode) {
   case GimvSlideshowWinModeFullScreen:
      gimv_image_win_set_fullscreen (iw, TRUE);
      break;
   case GimvSlideshowWinModeMaximize:
      gimv_image_win_set_maximize (iw, TRUE);
      break;
   default:
      break;
   }
   if (conf.slideshow_set_bg)
      gimv_image_win_set_fullscreen_bg_color (iw,
                                              conf.slideshow_bg_color[0],
                                              conf.slideshow_bg_color[1],
                                              conf.slideshow_bg_color[2]);
   gimv_image_win_set_flags (iw,
                             GimvImageWinHideFrameFlag
                             | GimvImageWinNotSaveStateFlag);

   gimv_image_win_slideshow_set_repeat (iw, conf.slideshow_repeat);
   gimv_image_win_slideshow_set_interval (iw, priv->interval);

   /* set bg color */
   if (conf.slideshow_set_bg) {
      gimv_image_view_set_bg_color (iv,
                                    conf.slideshow_bg_color[0],
                                    conf.slideshow_bg_color[1],
                                    conf.slideshow_bg_color[2]);
   }

   gimv_image_view_set_list (iv,
                             priv->filelist,
                             current,
                             (gpointer) slideshow,
                             next_image,
                             prev_image,
                             nth_image,
                             remove_list,
                             iw);

   g_signal_connect_after (G_OBJECT (iw), "show_fullscreen",
                           G_CALLBACK (cb_show_fullscreen), slideshow);
   g_signal_connect_after (G_OBJECT (iw), "hide_fullscreen",
                           G_CALLBACK (cb_hide_fullscreen), slideshow);

   gtk_widget_show (GTK_WIDGET (priv->iw));

   return iw;
}

void
gimv_slideshow_play (GimvSlideshow *slideshow)
{
   GimvSlideshowPrivate *priv;
   GimvImageWin *iw = NULL;

   g_return_if_fail (GIMV_IS_SLIDESHOW (slideshow));

   priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   if (!priv->iw)
      iw = gimv_slideshow_open_window (slideshow);
   else
      iw = priv->iw;

   g_return_if_fail (iw);

   gimv_image_win_slideshow_play (iw);
}

void
gimv_slideshow_stop (GimvSlideshow *slideshow)
{
   GimvSlideshowPrivate *priv;
   GimvImageWin *iw;

   g_return_if_fail (GIMV_IS_SLIDESHOW (slideshow));

   priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   iw = priv->iw;
   g_return_if_fail (iw);

   gimv_image_win_slideshow_stop (iw);
}

void
gimv_slideshow_set_interval (GimvSlideshow *slideshow, guint interval)
{
   GimvSlideshowPrivate *priv;
   GimvImageWin *iw;

   g_return_if_fail (GIMV_IS_SLIDESHOW (slideshow));

   priv = GIMV_SLIDESHOW_GET_PRIVATE (slideshow);

   priv->interval = interval;

   iw = priv->iw;
   if (iw)
      gimv_image_win_slideshow_set_interval (priv->iw, interval);
}
