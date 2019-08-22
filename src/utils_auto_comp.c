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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkscrolledwindow.h>

#include "prefs.h"
#include "utils_auto_comp.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_file_gtk.h"

#define MAX_VISIBLE_ROWS 8
#define CLIST_ROW_PAD    5

static gchar *ac_dir            = NULL;
static gchar *ac_path           = NULL;
static gchar  ac_show_dot       = FALSE;
static GList *ac_subdirs        = NULL; 
static GList *ac_alternatives   = NULL;

static GtkWidget *ac_window     = NULL;
static GtkWidget *ac_clist      = NULL;
static GtkWidget *ac_entry      = NULL;
static GtkListStore *ac_list_store = NULL;

static void 
ac_dir_free (void)
{
   if (!ac_dir) return;

   g_free (ac_dir);
   ac_dir = NULL;
}


static void 
ac_path_free (void)
{
   if (!ac_path) return;

   g_free (ac_path);
   ac_path = NULL;
}


static void 
ac_subdirs_free (void)
{
   if (!ac_subdirs) return;

   g_list_foreach (ac_subdirs, (GFunc) g_free, NULL);
   g_list_free (ac_subdirs);
   ac_subdirs = NULL;
}


static void 
ac_alternatives_free (void)
{
   if (!ac_alternatives) return;

   g_list_foreach (ac_alternatives, (GFunc) g_free, NULL);
   g_list_free (ac_alternatives);
   ac_alternatives = NULL;
}


void
auto_compl_reset (void) 
{
   ac_dir_free ();
   ac_path_free ();
   ac_subdirs_free ();
   ac_alternatives_free ();
}


gint
auto_compl_get_n_alternatives (const gchar *path)
{
   gchar *dir;
   gchar *filename;
   gint path_len;
   GList *scan;
   gint n;
   gint flags = GETDIR_FOLLOW_SYMLINK;
   gboolean show_dot;

   if (path == NULL) return 0;

   filename = g_path_get_basename (path);
   if (filename && filename[0] == '.') {
      show_dot = TRUE;
      flags = flags | GETDIR_READ_DOT;
   } else {
      show_dot = FALSE;
   }
   g_free (filename);

   if (strcmp (path, "/") == 0)
      dir = g_strdup ("/");
   else
      dir = g_dirname (path);

   if (!isdir (dir)) {
      g_free (dir);
      return 0;
   }

   if ((ac_dir == NULL) || strcmp (dir, ac_dir) || ac_show_dot != show_dot) {
      ac_dir_free ();
      ac_subdirs_free ();

      ac_dir = charset_to_internal (dir,
                                    conf.charset_filename,
                                    conf.charset_auto_detect_fn,
                                    conf.charset_filename_mode);
      if (!ac_dir && dir)
         ac_dir = g_strdup (dir);

      get_dir (dir, flags, NULL, &ac_subdirs);
      if (ac_show_dot != show_dot)
         ac_show_dot = show_dot;
   }

   ac_path_free ();
   ac_alternatives_free ();

   ac_path = g_strdup (path);
   path_len = strlen (ac_path);
   n = 0;

   for (scan = ac_subdirs; scan; scan = scan->next) {
      const gchar *subdir = (gchar*) scan->data;
      gchar *subdir_internal;

      subdir_internal = charset_to_internal (subdir,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);

      if (!subdir_internal && subdir)
         subdir_internal = g_strdup (subdir);

      if (strncmp (path, subdir_internal, path_len) != 0) {
         g_free (subdir_internal);
         continue;
      }

      ac_alternatives = g_list_prepend (ac_alternatives, 
                                        subdir_internal);

      n++;
   }

   g_free (dir);
   ac_alternatives = g_list_reverse (ac_alternatives);

   return n;
}


static gint
get_common_prefix_length (void) 
{
   gint n;
   GList *scan;
   gchar c1, c2;

   g_return_val_if_fail (ac_path != NULL, 0);
   g_return_val_if_fail (ac_alternatives != NULL, 0);

   /* if the number of alternatives is 1 return its length. */
   if (ac_alternatives->next == NULL)
      return strlen ((gchar*) ac_alternatives->data);

   n = strlen (ac_path);
   while (TRUE) {
      scan = ac_alternatives;

      c1 = ((gchar*) scan->data) [n];

      if (c1 == 0)
         return n;

      /* check that all other alternatives have the same 
       * character at position n. */

      scan = scan->next;
      
      for (; scan; scan = scan->next) {
         c2 = ((gchar*) scan->data) [n];
         if (c1 != c2)
            return n;
      }

      n++;
   }

   return -1;
}


gchar *
auto_compl_get_common_prefix (void) 
{
   gchar *alternative;
   gint n;

   if (ac_path == NULL)
      return NULL;

   if (ac_alternatives == NULL)
      return NULL;

   n = get_common_prefix_length ();
   alternative = (gchar*) ac_alternatives->data;

   return g_strndup (alternative, n);
}


GList * 
auto_compl_get_alternatives (void)
{
   return ac_alternatives;
}


static gboolean
ac_window_button_press_cb (GtkWidget *widget,
                           GdkEventButton *event,
                           gpointer *data)
{
   gint x, y, w, h;

   gdk_window_get_origin (ac_window->window, &x, &y);
   gdk_window_get_size (ac_window->window, &w, &h);

   /* Checks if the button press happened inside the window, 
    * if not closes the window. */
   if ((event->x >= 0) && (event->x <= w)
       && (event->y  >= 0) && (event->y <= h)) {
      /* In window. */
      return FALSE;
   }

   auto_compl_hide_alternatives ();

   return TRUE;
}


static gboolean
ac_window_key_press_cb (GtkWidget *widget,
                        GdkEventKey *event,
                        gpointer *data)
{
   if (event->keyval == GDK_Escape) {
      auto_compl_hide_alternatives ();
      return TRUE;
   }

   /* allow keyboard navigation in the alternatives clist */
   if (event->keyval == GDK_Up 
       || event->keyval == GDK_Down 
       || event->keyval == GDK_Page_Up 
       || event->keyval == GDK_Page_Down 
       || event->keyval == GDK_space)
      return FALSE;
    
   if (event->keyval == GDK_Return) {
      event->keyval = GDK_space;
      return FALSE;
   }

   auto_compl_hide_alternatives ();
   gtk_widget_event (ac_entry, (GdkEvent*) event);
   return TRUE;
}


static void
cb_tree_cursor_changed (GtkTreeView *treeview, gpointer data)
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gchar *text, *full_path;
   gboolean success;

   g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;

   gtk_tree_model_get (model, &iter,
                       0, &text,
                       -1);
   if (!text) return;

   full_path = g_strconcat (ac_dir, "/", text, NULL);
   gtk_entry_set_text (GTK_ENTRY (ac_entry), full_path);

   g_free (text);
   g_free (full_path);

   gtk_editable_set_position (GTK_EDITABLE (ac_entry), -1);
}


/* displays a list of alternatives under the entry widget. */
void
auto_compl_show_alternatives (GtkWidget *entry)
{
   gint x, y, w, h;
   GList *scan;
   gint n;
   GtkTreeIter iter;

   if (ac_window == NULL) {
      GtkWidget *scroll;
      GtkWidget *frame;
      GtkTreeViewColumn *col;
      GtkCellRenderer *render;

      ac_window = gtk_window_new (GTK_WINDOW_POPUP);

      ac_list_store = gtk_list_store_new (1, G_TYPE_STRING);
      ac_clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ac_list_store));
      gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ac_clist), TRUE);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ac_clist), FALSE);

      col = gtk_tree_view_column_new();
      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, FALSE);
      gtk_tree_view_column_add_attribute (col, render, "text", 0);
 
      gtk_tree_view_append_column (GTK_TREE_VIEW (ac_clist), col);

      scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);

      gtk_container_add (GTK_CONTAINER (ac_window), frame);
      gtk_container_add (GTK_CONTAINER (frame), scroll);
      gtk_container_add (GTK_CONTAINER (scroll), ac_clist);

      g_signal_connect (G_OBJECT (ac_window),
                        "button-press-event",
                        G_CALLBACK (ac_window_button_press_cb),
                        NULL);
      g_signal_connect (G_OBJECT (ac_window),
                        "key-press-event",
                        G_CALLBACK (ac_window_key_press_cb),
                        NULL);

      g_signal_connect (G_OBJECT (ac_clist),
                        "cursor_changed",
                        G_CALLBACK (cb_tree_cursor_changed),
                        NULL);
   }

   ac_entry = entry;
   n = 0;

   gtk_list_store_clear (ac_list_store);

   for (scan = ac_alternatives; scan; scan = scan->next) {
      gchar *basename = g_path_get_basename (scan->data);
      gtk_list_store_append (ac_list_store, &iter);
      gtk_list_store_set (ac_list_store, &iter,
                          0, basename,
                          -1);
      g_free (basename);

      if (n == 0) {
         GtkTreeSelection *selection;
         GtkTreePath *treepath;
         selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ac_clist));
         treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (ac_list_store),
                                             &iter);
         gtk_tree_selection_select_path (selection, treepath);
         gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (ac_clist),
                                       treepath, NULL,
                                       TRUE, 0.0, 0.0);
         gtk_tree_path_free (treepath);
      }

      n++;
   }

   gdk_window_get_geometry (entry->window, &x, &y, &w, &h, NULL);
   gdk_window_get_deskrelative_origin (entry->window, &x, &y);
   gtk_widget_set_uposition (ac_window, x, y + h);
   gtk_widget_set_size_request (ac_window, w, 200);

   gtk_widget_show_all (ac_window);
   gdk_pointer_grab (ac_window->window, 
                     TRUE,
                     (GDK_POINTER_MOTION_MASK 
                      | GDK_BUTTON_PRESS_MASK 
                      | GDK_BUTTON_RELEASE_MASK),
                     NULL, 
                     NULL, 
                     GDK_CURRENT_TIME);
   gdk_keyboard_grab (ac_window->window,
                      FALSE,
                      GDK_CURRENT_TIME);
   gtk_grab_add (ac_window);
}


void
auto_compl_hide_alternatives (void)
{
   if (ac_window && GTK_WIDGET_VISIBLE (ac_window)) {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      gdk_keyboard_ungrab (GDK_CURRENT_TIME);
      gtk_grab_remove (ac_window);
      gtk_widget_hide (ac_window);
   }
}
