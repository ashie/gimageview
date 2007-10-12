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

#ifndef __GIMV_PREFS_UI_UTILS_H__
#define __GIMV_PREFS_UI_UTILS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>


GtkWidget *gimv_prefs_ui_mouse_prefs    (const gchar **items,
                                         const gchar  *defval,
                                         gchar       **dest);
GtkWidget *gimv_prefs_ui_dir_list_prefs (const gchar  *title,
                                         const gchar  *dialog_title,
                                         const gchar  *defval,
                                         gchar       **prefsval,
                                         gchar         sepchar);
GtkWidget *gimv_prefs_ui_double_clist   (const gchar  *title,
                                         const gchar  *clist1_title,
                                         const gchar  *clist2_title,
                                         GList        *available_list,
                                         const gchar  *defval,
                                         gchar       **prefsval,
                                         gchar         sepchar);


#define gimv_prefs_ui_create_frame(label, frame, vbox, parent, expand) \
{ \
   frame = gtk_frame_new (label); \
   gtk_container_set_border_width(GTK_CONTAINER (frame), 0); \
   gtk_box_pack_start(GTK_BOX (parent), frame, expand, TRUE, 0); \
   gtk_widget_show (frame); \
   vbox = gtk_vbox_new (FALSE, 0); \
   gtk_container_set_border_width (GTK_CONTAINER(frame), 5); \
   gtk_container_add (GTK_CONTAINER (frame), vbox); \
   gtk_widget_show (vbox); \
}

#endif /* __GIMV_PREFS_UI_UTILS_H__ */
