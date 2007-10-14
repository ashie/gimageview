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


gint            idle_slideshow_delete (gpointer   data);
static GList   *next_image            (GimvImageView *iv,
                                       gpointer   list_owner,
                                       GList     *current,
                                       gpointer   data);
static GList   *prev_image            (GimvImageView *iv,
                                       gpointer   list_owner,
                                       GList     *current,
                                       gpointer   data);
static void     remove_list           (GimvImageView *iv,
                                       gpointer   list_owner,
                                       gpointer   data);


/******************************************************************************
 *
 *  Other private functions
 *
 ******************************************************************************/
gint
idle_slideshow_delete (gpointer data)
{
   GimvSlideShow *slideshow = data;

   gimv_slideshow_delete (slideshow);

   return 0;
}


typedef struct ChangeImageData_Tag
{
   GimvSlideShow *slideshow;
   GimvImageWin  *iw;
   GimvImageInfo *info;
} ChangeImageData;


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
   GimvSlideShow *slideshow = list_owner;

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (slideshow->filelist, current->data);
   g_return_val_if_fail (node, NULL);

   next = g_list_next (current);
   if (!next && slideshow->repeat)
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
      slideshow->current = next;
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
   GimvSlideShow *slideshow = list_owner;

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (slideshow->filelist, current->data);
   g_return_val_if_fail (node, NULL);

   prev = g_list_previous (current);
   if (!prev && slideshow->repeat)
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
      slideshow->current = prev;
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
   GimvSlideShow *slideshow = list_owner;
   ChangeImageData *change_data;

   g_return_val_if_fail (iv, NULL);

   node = g_list_nth (slideshow->filelist, nth);
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
   slideshow->current = node;

   return node;
}


static void
remove_list (GimvImageView *iv, gpointer list_owner, gpointer data)
{
   GimvSlideShow *slideshow = list_owner;

   g_return_if_fail (iv);
   g_return_if_fail (slideshow);

   /* free slide show data safely */
   gtk_idle_add (idle_slideshow_delete, slideshow);
}


static void
cb_show_fullscreen (GimvImageWin *iw, GimvSlideShow *slideshow)
{
}


static void
cb_hide_fullscreen (GimvImageWin *iw, GimvSlideShow *slideshow)
{
   gtk_widget_destroy (GTK_WIDGET (iw));
}



/******************************************************************************
 *
 *  Public functions
 *
 ******************************************************************************/
GimvSlideShow *
gimv_slideshow_new (void)
{
   GimvSlideShow *slideshow;

   slideshow = g_new0 (GimvSlideShow, 1);

   slideshow->iw               = NULL;
   slideshow->filelist         = NULL;
   slideshow->repeat           = conf.slideshow_repeat;

   return slideshow;
}


void
gimv_slideshow_delete (GimvSlideShow *slideshow)
{
   GimvImageWin *iw;
   GimvImageView *iv;

   g_return_if_fail (slideshow);

   iw = slideshow->iw;
   iv = iw->iv;

   g_list_foreach (slideshow->filelist, (GFunc) gimv_image_info_unref, NULL);
   g_list_free (g_list_first (slideshow->filelist));

   g_free (slideshow);
}


GimvSlideShow *
gimv_slideshow_new_with_filelist (GList *filelist, GList *start)
{
   GimvSlideShow *slideshow;

   g_return_val_if_fail (filelist, NULL);

   slideshow           = gimv_slideshow_new ();
   slideshow->filelist = g_list_first (filelist);
   slideshow->current  = start ? start : filelist;

   return slideshow;
}


GimvImageWin *
gimv_slideshow_open_window (GimvSlideShow *slideshow)
{
   GimvImageWin *iw;
   GimvImageView *iv;
   GList *current;

   g_return_val_if_fail (slideshow, NULL);
   g_return_val_if_fail (slideshow->filelist, NULL);

   if (slideshow->iw)
      return slideshow->iw;

   /* current = g_list_first (slideshow->filelist); */
   current = slideshow->current ? slideshow->current
                                : slideshow->filelist;
   g_return_val_if_fail (slideshow, NULL);

   slideshow->iw = GIMV_IMAGE_WIN (gimv_image_win_new (NULL));
   g_return_val_if_fail (slideshow->iw, NULL);

   iw = slideshow->iw;
   iv = iw->iv;

   /* override some parameters */
   gtk_object_set(GTK_OBJECT(iv),
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
   case GimvSlideShowWinModeFullScreen:
      gimv_image_win_set_fullscreen (iw, TRUE);
      break;
   case GimvSlideShowWinModeMaximize:
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
   gimv_image_win_slideshow_set_interval (iw, conf.slideshow_interval * 1000);

   /* set bg color */
   if (conf.slideshow_set_bg) {
      gimv_image_view_set_bg_color (iv,
                                    conf.slideshow_bg_color[0],
                                    conf.slideshow_bg_color[1],
                                    conf.slideshow_bg_color[2]);
   }

   gimv_image_view_set_list (iv,
                             slideshow->filelist,
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

   gtk_widget_show (GTK_WIDGET (slideshow->iw));

   return iw;
}


/*
 *  gimv_slideshow_play:
 *     @ Execute slide show;
 *
 *  slideshow : Poiter to GimvSlideShow struct.
 */
void
gimv_slideshow_play (GimvSlideShow *slideshow)
{
   GimvImageWin *iw = NULL;

   g_return_if_fail (slideshow);

   if (!slideshow->iw)
      iw = gimv_slideshow_open_window (slideshow);
   else
      iw = slideshow->iw;

   g_return_if_fail (iw);

   gimv_image_win_slideshow_play (iw);
}


void
gimv_slideshow_stop (GimvSlideShow *slideshow)
{
   GimvImageWin *iw;

   g_return_if_fail (slideshow);

   iw = slideshow->iw;
   g_return_if_fail (iw);

   gimv_image_win_slideshow_stop (iw);
}


void
gimv_slideshow_set_interval (GimvSlideShow *slideshow, guint interval)
{
   GimvImageWin *iw;

   g_return_if_fail (slideshow);

   iw = slideshow->iw;
   g_return_if_fail (iw);

   gimv_image_win_slideshow_set_interval (slideshow->iw, interval);
}
