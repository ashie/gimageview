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

#ifndef __GIMV_DIR_VIEW_H__
#define __GIMV_DIR_VIEW_H__

#include "gimageview.h"

typedef struct GimvDirViewPrivate_Tag GimvDirViewPrivate;

typedef enum
{
   GIMV_DIR_VIEWTREE,
   GIMV_DIR_VIEWLIST,
   GIMV_DIR_VIEWTREE_WITH_FILE,
   GIMV_DIR_VIEWLIST_WITH_FILE
} GimvDirViewMode;

struct GimvDirView_Tag
{
   GtkWidget      *container;
   GtkWidget      *toolbar;
   GtkWidget      *toolbar_eventbox;
   GtkWidget      *scroll_win;
   GtkWidget      *dirtree;
   GtkWidget      *popup_menu;

   GimvThumbWin   *tw;

   gchar          *root_dir;
   GimvDirViewMode mode;
   gboolean        show_toolbar;
   gboolean        show_dotfile;

   GimvDirViewPrivate *priv;
};


void       gimv_dir_view_chroot            (GimvDirView  *dv,
                                            const gchar  *root_dir);
void       gimv_dir_view_chroot_to_parent  (GimvDirView  *dv);
void       gimv_dir_view_change_dir        (GimvDirView  *dv,
                                            const gchar  *str);
void       gimv_dir_view_go_home           (GimvDirView  *dv);
void       gimv_dir_view_refresh_list      (GimvDirView  *dv);
gchar     *gimv_dir_view_get_selected_path (GimvDirView  *dv);
void       gimv_dir_view_expand_dir        (GimvDirView  *dv,
                                            const gchar  *dir,
                                            gboolean      open_all);
gboolean   gimv_dir_view_set_opened_mark   (GimvDirView  *dv,
                                            const gchar  *path);
gboolean   gimv_dir_view_unset_opened_mark (GimvDirView  *dv,
                                            const gchar  *path);
void       gimv_dir_view_show_toolbar      (GimvDirView  *dv);
void       gimv_dir_view_hide_toolbar      (GimvDirView  *dv);

GimvDirView *gimv_dir_view_create          (const gchar  *root_dir,
                                            GtkWidget    *parent_win,
                                            GimvThumbWin *tw);

#endif /* __GIMV_DIR_VIEW_H__ */
