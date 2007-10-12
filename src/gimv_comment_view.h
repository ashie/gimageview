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

#ifndef __GIMV_COMMENT_VIEW_H__
#define __GIMV_COMMENT_VIEW_H__

#include "gimageview.h"
#include "gimv_comment.h"

struct GimvCommentView_Tag
{
   GimvComment *comment;

   GtkWidget *window;
   GtkWidget *main_vbox;
   GtkWidget *notebook;
   GtkWidget *button_area;
   GtkWidget *inner_button_area;

   GtkWidget *data_page, *note_page;

   GtkWidget *comment_clist;
   gint       selected_row;

   GtkWidget *key_combo, *value_entry;
   GtkWidget *selected_item;

   GtkWidget *note_box;

   GtkWidget *save_button;
   GtkWidget *reset_button;
   GtkWidget *delete_button;

   GtkAccelGroup *accel_group;

   gboolean changed;

   /* for list */
   GimvImageView *iv;
   GtkWidget     *next_button;
   GtkWidget     *prev_button;
};


void             gimv_comment_view_clear         (GimvCommentView   *cv);
gboolean         gimv_comment_view_change_file   (GimvCommentView   *cv,
                                                  GimvImageInfo *info);
GimvCommentView *gimv_comment_view_create        (void);
GimvCommentView *gimv_comment_view_create_window (GimvImageInfo *info);

#endif /* __GIMV_COMMENT_VIEW_H__ */
