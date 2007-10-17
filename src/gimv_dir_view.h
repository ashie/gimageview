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

#ifndef __DIRVIEW_H__
#define __DIRVIEW_H__

#include "gimageview.h"

typedef struct DirViewPrivate_Tag DirViewPrivate;

typedef enum
{
   DIRVIEW_TREE,
   DIRVIEW_LIST,
   DIRVIEW_TREE_WITH_FILE,
   DIRVIEW_LIST_WITH_FILE
} DirViewMode;


struct DirView_Tag
{
   GtkWidget      *container;
   GtkWidget      *toolbar;
   GtkWidget      *toolbar_eventbox;
   GtkWidget      *scroll_win;
   GtkWidget      *dirtree;
   GtkWidget      *popup_menu;

   GimvThumbWin   *tw;

   gchar          *root_dir;
   DirViewMode     mode;
   gboolean        show_toolbar;
   gboolean        show_dotfile;

   DirViewPrivate *priv;
};


void       dirview_change_root           (DirView     *dv,
                                          const gchar *root_dir);
void       dirview_change_root_to_parent (DirView     *dv);
void       dirview_change_dir            (DirView     *dv,
                                          const gchar *str);
void       dirview_go_home               (DirView     *dv);
void       dirview_refresh_list          (DirView     *dv);
gchar     *dirview_get_selected_path     (DirView     *dv);
void       dirview_expand_dir            (DirView     *dv,
                                          const gchar *dir,
                                          gboolean     open_all);
gboolean   dirview_set_opened_mark       (DirView     *dv,
                                          const gchar *path);
gboolean   dirview_unset_opened_mark     (DirView     *dv,
                                          const gchar *path);
void       dirview_show_toolbar          (DirView     *dv);
void       dirview_hide_toolbar          (DirView     *dv);

DirView   *dirview_create                (const gchar *root_dir,
                                          GtkWidget   *parent_win,
                                          GimvThumbWin *tw);

#endif /* __DIRVIEW_H__ */
