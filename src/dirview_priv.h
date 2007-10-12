/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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

#ifndef __DIRVIEW_PRIV_H__
#define __DIRVIEW_PRIV_H__

#include "dirview.h"

struct DirViewPrivate_Tag {
   /* for DnD */
   gint         hilit_dir;

   guint        scroll_timer_id;
   gint         drag_motion_x;
   gint         drag_motion_y;

   gint         auto_expand_timeout_id;

   /* for mouse event */
   gint         button_2pressed_queue; /* attach an action to
                                          button release event */

   guint        button_action_id;
   guint        swap_com_id;       

#ifdef ENABLE_TREEVIEW
   GtkTreePath *drag_tree_row;
   guint        adjust_tree_id;
#else /* ENABLE_TREEVIEW */
   gint         drag_tree_row;
   guint        change_root_id;
#endif /* ENABLE_TREEVIEW */
};

typedef enum {
   MouseActNone,
   MouseActLoadThumb,
   MouseActLoadThumbRecursive,
   MouseActPopupMenu,
   MouseActChangeTop,
   MouseActLoadThumbRecursiveInOneTab
} DirViewMouseAction;

#endif /* __DIRVIEW_PRIV_H__ */
