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


#ifndef __MENU_H__
#define __MENU_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>

gint       menu_count_ifactory_entry_num (GtkItemFactoryEntry *entries);
GtkWidget *menubar_create                (GtkWidget           *window,
                                          GtkItemFactoryEntry *entries,
                                          guint                n_entries,
                                          const gchar         *path,
                                          gpointer             data);

GtkWidget *menu_create_items             (GtkWidget           *window,
                                          GtkItemFactoryEntry *entries,
                                          guint                n_entries,
                                          const gchar         *path,
                                          gpointer             data);

void       menu_item_set_sensitive       (GtkWidget    *widget,
                                          gchar        *path,
                                          gboolean      sensitive);
void       menu_check_item_set_active    (GtkWidget    *widget,
                                          gchar        *path,
                                          gboolean      active);
gboolean   menu_check_item_get_active    (GtkWidget    *widget,
                                          gchar        *path);
void       menu_set_submenu              (GtkWidget    *widget,
                                          const gchar  *path,
                                          GtkWidget    *submenu);
GtkWidget *menu_get_submenu              (GtkWidget    *widget,
                                          const gchar  *path);
void       menu_remove_submenu           (GtkWidget    *widget,
                                          const gchar  *path,
                                          GtkWidget    *submenu);
GtkWidget *create_option_menu_simple     (const gchar **menu_items,
                                          gint          def_val,
                                          gint         *data);
GtkWidget *create_option_menu            (const gchar **menu_items,
                                          gint          def_val,
                                          gpointer      func,
                                          gpointer      data);


void       menu_modal_cb                 (gpointer             data,
                                          guint                action,
                                          GtkWidget           *menuitem);
gint       menu_popup_modal              (GtkWidget           *popup,
                                          GtkMenuPositionFunc  pos_func,
                                          gpointer             pos_data,
                                          GdkEventButton      *event,
                                          gpointer             user_data);
void       menu_calc_popup_position      (GtkMenu             *menu,
                                          gint                *x_ret,
                                          gint                *y_ret,
                                          gboolean            *push_in,
                                          gpointer             data);

#endif /* __MENU_H__ */
