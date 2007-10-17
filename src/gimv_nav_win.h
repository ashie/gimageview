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

/*
 * These codes are mostly taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#ifndef __GIMV_NAV_WIN_H__
#define __GIMV_NAV_WIN_H__

#include "gimageview.h"

#define GIMV_TYPE_NAV_WIN            (gimv_nav_win_get_type ())
#define GIMV_NAV_WIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_NAV_WIN, GimvNavWin))
#define GIMV_NAV_WIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_NAV_WIN, GimvNavWinClass))
#define GIMV_IS_NAV_WIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_NAV_WIN))
#define GIMV_IS_NAV_WIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_NAV_WIN))
#define GIMV_NAV_WIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_NAV_WIN, GimvNavWinClass))

#define GIMV_NAV_WIN_SIZE 128 /* Max size of the window. */

typedef struct GimvNavWin_Tag      GimvNavWin;
typedef struct GimvNavWinPriv_Tag  GimvNavWinPriv;
typedef struct GimvNavWinClass_Tag GimvNavWinClass;

struct GimvNavWin_Tag
{
   GtkWindow parent;

   GtkWidget *out_frame;
   GtkWidget *in_frame;
   GtkWidget *preview;

   GdkPixmap *pixmap;
   GdkBitmap *mask;

   GdkGC *gc;

   gint x_root, y_root;

   gint image_width, image_height;

   gint view_width, view_height;
   gint view_pos_x, view_pos_y;

   gint popup_x, popup_y;
   gint popup_width, popup_height;

   gint sqr_x, sqr_y;
   gint sqr_width, sqr_height;

   gint fix_x_pos, fix_y_pos;

   gdouble factor;
}; 

struct GimvNavWinClass_Tag
{
   GtkWindowClass parent_class;

   /* signals */
   void (*move) (GimvNavWin *navwin,
                 gint        x,
                 gint        y);
};

GType      gimv_nav_win_get_type             (void);
GtkWidget *gimv_nav_win_new                  (GdkPixmap  *pixmap,
                                              GdkBitmap  *mask,
                                              gint        image_width,
                                              gint        image_height,
                                              gint        view_width,
                                              gint        view_height,
                                              gint        view_pos_x,
                                              gint        view_pos_y);
void       gimv_nav_win_show                 (GimvNavWin *navwin,
                                              gint        x_root,
                                              gint        y_root);
void       gimv_nav_win_hide                 (GimvNavWin *navwin);
void       gimv_nav_win_set_pixmap           (GimvNavWin *navwin,
                                              GdkPixmap  *pixmap,
                                              GdkBitmap  *mask,
                                              gint        image_width,
                                              gint        image_height);
void       gimv_nav_win_set_orig_image_size  (GimvNavWin *navwin,
                                              gint        width,
                                              gint        height);
void       gimv_nav_win_set_view_size        (GimvNavWin *navwin,
                                              gint        width,
                                              gint        height);
void       gimv_nav_win_set_view_position    (GimvNavWin *navwin,
                                              gint        x,
                                              gint        y);

#endif /* __GIMV_NAV_WIN_H__ */
