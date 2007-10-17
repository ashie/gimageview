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

#ifndef __GIMV_IMAGE_WIN_H__
#define __GIMV_IMAGE_WIN_H__

#include "gimageview.h"
#include "gimv_image_view.h"


#define GIMV_TYPE_IMAGE_WIN            (gimv_image_win_get_type ())
#define GIMV_IMAGE_WIN(obj)            (GTK_CHECK_CAST (obj, gimv_image_win_get_type (), GimvImageWin))
#define GIMV_IMAGE_WIN_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_image_win_get_type, GimvImageWinClass))
#define GIMV_IS_IMAGE_WIN(obj)         (GTK_CHECK_TYPE (obj, gimv_image_win_get_type ()))
#define GIMV_IS_IMAGE_WIN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_IMAGE_WIN))


typedef struct GimvImageWinClass_Tag GimvImageWinClass;
typedef struct GimvImageWinPriv_Tag  GimvImageWinPriv;


typedef enum {
   GimvImageWinCreatingFlag                 = 1 << 0,
   GimvImageWinShowMenuBarFlag              = 1 << 1,
   GimvImageWinShowToolBarFlag              = 1 << 2,
   GimvImageWinShowPlayerFlag               = 1 << 3,
   GimvImageWinShowStatusBarFlag            = 1 << 4,
   GimvImageWinHideFrameFlag                = 1 << 5,
   GimvImageWinSeekBarDraggingFlag          = 1 << 6,
   GimvImageWinMovieFlag                    = 1 << 7,
   GimvImageWinMaximizeFlag                 = 1 << 8,
   GimvImageWinFullScreenFlag               = 1 << 9,
   GimvImageWinSlideShowRepeatFlag          = 1 << 10,
   GimvImageWinSlideShowPlayingFlag         = 1 << 11,
   GimvImageWinSlideShowSeekBarDraggingFlag = 1 << 12,
   GimvImageWinNotSaveStateFlag             = 1 << 13
} GimvImageWinFlags;


struct GimvImageWin_Tag
{
   GtkWindow        parent;

   /* Image View */
   GimvImageView   *iv;

   /* other widgets */
   GtkWidget       *menubar_handle;
   GtkWidget       *menubar;
   GtkWidget       *toolbar_handle;
   GtkWidget       *toolbar;
   GtkWidget       *player_handle;
   GtkWidget       *player_bar;

   struct    /* buttons in toolbar */
   {
      GtkWidget *fileopen;
      GtkWidget *prefs;
      GtkWidget *prev;
      GtkWidget *next;
      GtkWidget *xscale;
      GtkWidget *yscale;
      GtkWidget *rotate;
   } button;

   struct    /* buttons in player */
   {
      GtkWidget *prev;
      GtkWidget *rw;
      GtkWidget *play;
      GtkWidget *stop;
      GtkWidget *fw;
      GtkWidget *next;
      GtkWidget *seekbar;
   } player;

   GtkWidget       *main_vbox;  /* Image Container */ 

   GtkWidget       *status_bar_container;
   GtkWidget       *status_bar1;
   GtkWidget       *status_bar2;
   GtkWidget       *progressbar;

   /* sub menus */
   GtkWidget       *view_menu;
   GtkWidget       *move_menu;
   GtkWidget       *help_menu;

   /* fullscreen window */
   GtkWidget       *fullscreen;

   GimvImageWinPriv *priv;
};


struct GimvImageWinClass_Tag
{
   GtkWindowClass parent_class;

   /* signals */
   void (*show_fullscreen) (GimvImageWin *iw);
   void (*hide_fullscreen) (GimvImageWin *iw);
};


GType         gimv_image_win_get_type                  (void);
GtkWidget    *gimv_image_win_new                       (GimvImageInfo *info);
GimvImageWin *gimv_image_win_open_window               (GimvImageInfo *info);
GimvImageWin *gimv_image_win_open_shared_window        (GimvImageInfo *info);
GimvImageWin *gimv_image_win_open_window_auto          (GimvImageInfo *info);

GList        *gimv_image_win_get_list                  (void);
GimvImageWin *gimv_image_win_get_shared_window         (void);

void          gimv_image_win_player_set_sensitive_all  (GimvImageWin  *iw,
                                                        gboolean       sensitive);
void          gimv_image_win_change_image              (GimvImageWin  *iw,
                                                        GimvImageInfo *info);

void          gimv_image_win_set_sensitive             (GimvImageWin  *iw,
                                                        gboolean       sensitive);
void          gimv_image_win_save_state                (GimvImageWin  *iw);

void          gimv_image_win_slideshow_play            (GimvImageWin  *iw);
void          gimv_image_win_slideshow_stop            (GimvImageWin  *iw);
void          gimv_image_win_slideshow_set_interval    (GimvImageWin  *iw,
                                                        guint          interval);
void          gimv_image_win_slideshow_set_repeat      (GimvImageWin  *iw,
                                                        gboolean       repeat);

void          gimv_image_win_show_menubar              (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_show_toolbar              (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_show_player               (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_show_statusbar            (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_set_maximize              (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_set_fullscreen            (GimvImageWin  *iw,
                                                        gboolean       show);
void          gimv_image_win_set_fullscreen_bg_color   (GimvImageWin  *iw,
                                                        gint           red,
                                                        gint           green,
                                                        gint           blue);
void          gimv_image_win_set_flags                 (GimvImageWin  *iw,
                                                        GimvImageWinFlags flags);
void          gimv_image_win_unset_flags               (GimvImageWin  *iw,
                                                        GimvImageWinFlags flags);

#endif /* __GIMV_IMAGE_WIN_H__ */
