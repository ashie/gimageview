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

#include "gimv_comment_view.h"

#include <string.h>

#include "gimageview.h"

#include "gimv_comment.h"
#include "gimv_icon_stock.h"
#include "gimv_image_info.h"
#include "gimv_image_view.h"
#include "prefs.h"
#include "utils_char_code.h"
#include "utils_dnd.h"


typedef enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_KEY,
   COLUMN_VALUE,
   COLUMN_RAW_ENTRY,
   N_COLUMN
} ListStoreColumn;


static void gimv_comment_view_set_sensitive  (GimvCommentView *cv);
static void gimv_comment_view_set_combo_list (GimvCommentView *cv);
static void gimv_comment_view_data_enter     (GimvCommentView *cv);
static void gimv_comment_view_reset_data     (GimvCommentView *cv);


/******************************************************************************
 *
 *   callback functions.
 *
 ******************************************************************************/
static void
cb_switch_page (GtkNotebook *notebook,
                GtkNotebookPage *page,
                gint pagenum,
                GimvCommentView *cv)
{
   GtkWidget *widget;

   g_return_if_fail (page);
   g_return_if_fail (cv);

   if (!cv->button_area) return;

   widget = gtk_notebook_get_nth_page (notebook, pagenum);

   if (widget == cv->data_page || widget == cv->note_page)
      gtk_widget_show (cv->button_area);
   else
      gtk_widget_hide (cv->button_area);

   gimv_comment_view_set_sensitive (cv);
}


static void
gimv_comment_view_delete_selected_data (GimvCommentView *cv)
{
   GimvCommentDataEntry *entry;
   GtkTreeView *treeview;
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean found;

   g_return_if_fail (cv);

   treeview = GTK_TREE_VIEW (cv->comment_clist);
   selection  = gtk_tree_view_get_selection (treeview);

   found = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!found) return;

   gtk_tree_model_get (model, &iter,
                       COLUMN_RAW_ENTRY, &entry,
                       COLUMN_TERMINATOR);
   gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

   if (entry)
      gimv_comment_data_entry_remove (cv->comment, entry);

   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
   gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");

   gimv_comment_view_set_sensitive (cv);
}


static void
cb_save_button_pressed (GtkButton *button, GimvCommentView *cv)
{
   gchar *note;
   GtkTextBuffer *buffer;
   GtkTextIter start, end;

   g_return_if_fail (cv);
   g_return_if_fail (cv->comment);

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));

   gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
   gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);

   note = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

   if (note && *note)
      gimv_comment_update_note (cv->comment, note);

   g_free (note);

   gimv_comment_save_file (cv->comment);
}


static void
cb_reset_button_pressed (GtkButton *button, GimvCommentView *cv)
{
   g_return_if_fail (cv);
   g_return_if_fail (cv->comment);

   gimv_comment_view_change_file (cv, cv->comment->info);
}


static void
cb_del_button_pressed (GtkButton *button, GimvCommentView *cv)
{
   g_return_if_fail (cv->comment);

   gimv_comment_delete_file (cv->comment);
   g_object_unref (G_OBJECT (cv->comment));
   cv->comment = NULL;
   gimv_comment_view_clear (cv);
}


static void
cb_destroyed (GtkWidget *widget, GimvCommentView *cv)
{
   g_return_if_fail (cv);

   g_signal_handlers_disconnect_by_func (G_OBJECT (cv->notebook),
                                         G_CALLBACK (cb_switch_page), cv);

   if (cv->comment) {
      g_object_unref (G_OBJECT (cv->comment));
      cv->comment = NULL;
   }

   if (cv->accel_group) {
      gtk_accel_group_unref (cv->accel_group);
      cv->accel_group = NULL;
   }

   g_free (cv);
}


static void
cb_tree_view_cursor_changed (GtkTreeView *treeview, GimvCommentView *cv)
{
   GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gchar *key = NULL, *value = NULL;
   GtkEntry *entry1, *entry2;

   entry1 = GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry);
   entry2 = GTK_ENTRY(cv->value_entry);

   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (success) {
      gtk_tree_model_get (model, &iter,
                          COLUMN_KEY,   &key,
                          COLUMN_VALUE, &value,
                          COLUMN_TERMINATOR);
   }

   if (key)
      gtk_entry_set_text (entry1, key);
   else
      gtk_entry_set_text (entry1, "\0");
   if (value)
      gtk_entry_set_text (entry2, value);
   else
      gtk_entry_set_text (entry2, "\0");

   g_free (key);
   g_free (value);

   gimv_comment_view_set_sensitive (cv);
}


static gboolean
cb_data_list_key_press (GtkWidget *widget, GdkEventKey *event, GimvCommentView *cv)
{
   g_return_val_if_fail (cv, FALSE);

   switch (event->keyval) {
   case GDK_KP_Enter:
   case GDK_Return:
      if (cv->selected_item) {
         gtk_widget_grab_focus (cv->value_entry);
         g_signal_stop_emission_by_name (G_OBJECT (widget),
                                         "key_press_event");
         return TRUE;
      }
      break;

   case GDK_Delete:
      gimv_comment_view_delete_selected_data  (cv);
      return TRUE;
      break;

   default:
      break;
   }

   return FALSE;
}


static gboolean
cb_entry_key_press (GtkWidget *widget, GdkEventKey *event, GimvCommentView *cv)
{
   g_return_val_if_fail (cv, FALSE);

   switch (event->keyval) {
   case GDK_Tab:
      if (event->state & GDK_SHIFT_MASK) {
         gtk_widget_grab_focus (cv->key_combo);
      } else {
         gtk_widget_grab_focus (cv->save_button);
      }
      g_signal_stop_emission_by_name (G_OBJECT (widget),
                                      "key_press_event");
      return TRUE;
      break;

   default:
      break;
   }

   return FALSE;
}


static void
cb_entry_changed (GtkEditable *entry, GimvCommentView *cv)
{
   g_return_if_fail (cv);

   gimv_comment_view_set_sensitive (cv);
}


static void
cb_entry_enter (GtkEditable *entry, GimvCommentView *cv)
{
   g_return_if_fail (cv);

   gimv_comment_view_data_enter (cv);
   gtk_widget_grab_focus (cv->comment_clist);
}


static void
cb_combo_select (GtkWidget *label, GimvCommentView *cv)
{
   GtkWidget *clist;
   gchar *key = g_object_get_data (G_OBJECT (label), "key");
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean go_next;

   g_return_if_fail (label && GTK_IS_LIST_ITEM (label));
   g_return_if_fail (cv);
   g_return_if_fail (key);

   cv->selected_item = label;
   clist = cv->comment_clist;

   treeview = GTK_TREE_VIEW (clist);
   model = gtk_tree_view_get_model (treeview);

   go_next = gtk_tree_model_get_iter_first (model, &iter);

   for (; go_next; go_next = gtk_tree_model_iter_next (model, &iter)) {
      GimvCommentDataEntry *entry;

      gtk_tree_model_get (model, &iter,
                          COLUMN_RAW_ENTRY, &entry,
                          COLUMN_TERMINATOR);
      if (!entry) continue;

      if (entry->key && !strcmp (key, entry->key)) {
         GtkTreePath *treepath = gtk_tree_model_get_path (model, &iter);

         if (!treepath) continue;
         gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
         gtk_tree_path_free (treepath);
         break;
      }
   }
}

static void
cb_combo_deselect (GtkWidget *label, GimvCommentView *cv)
{
   g_return_if_fail (label && GTK_IS_LIST_ITEM (label));
   g_return_if_fail (cv);

   cv->selected_item = NULL;
}


static void
cb_file_saved (GimvComment *comment, GimvCommentView *cv)
{
   g_return_if_fail (cv);
   if (!cv->comment) return;

   gimv_comment_view_reset_data (cv);
}


/******************************************************************************
 *
 *   other private functions.
 *
 ******************************************************************************/
static GimvCommentView *
gimv_comment_view_new ()
{
   GimvCommentView *cv;

   cv = g_new0 (GimvCommentView, 1);
   g_return_val_if_fail (cv, NULL);

   cv->comment       = NULL;

   cv->window        = NULL;
   cv->main_vbox     = NULL;
   cv->notebook      = NULL;
   cv->button_area   = NULL;

   cv->data_page     = NULL;
   cv->note_page     = NULL;

   cv->comment_clist = NULL;
   cv->selected_row  = -1;
   cv->key_combo     = NULL;
   cv->value_entry   = NULL;
   cv->note_box      = NULL;
   cv->selected_item = NULL;

   cv->save_button   = NULL;
   cv->reset_button  = NULL;
   cv->delete_button = NULL;

   cv->accel_group   = NULL;
   cv->iv            = NULL;
   cv->next_button   = NULL;
   cv->prev_button   = NULL;
   cv->changed       = FALSE;

   return cv;
}


static GtkWidget *
create_data_page (GimvCommentView *cv)
{
   GtkWidget *vbox, *vbox1, *hbox, *hbox1;
   GtkWidget *scrolledwin, *clist, *combo, *entry;
   GtkWidget *label;
   gchar *titles[] = {_("Key"), _("Value")};
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;

   label = gtk_label_new (_("Data"));
   gtk_widget_set_name (label, "TabLabel");
   gtk_widget_show (label);
   cv->data_page = vbox = gtk_vbox_new (FALSE, 0);
   gtk_widget_show (vbox);

   gtk_notebook_append_page (GTK_NOTEBOOK(cv->notebook),
                             vbox, label);

   /* scrolled window & clist */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
                                       GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(vbox), scrolledwin, TRUE, TRUE, 0);

   store = gtk_list_store_new (N_COLUMN,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_POINTER);
   clist =  gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   cv->comment_clist = clist;

   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);

   /* set column for key */
   col = gtk_tree_view_column_new ();
   gtk_tree_view_column_set_resizable (col, TRUE);
   gtk_tree_view_column_set_title (col, titles[0]);
   render = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, render, TRUE);
   gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_KEY);
   gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

   /* set column for value */
   col = gtk_tree_view_column_new ();
   gtk_tree_view_column_set_resizable (col, TRUE);
   gtk_tree_view_column_set_title (col, titles[1]);
   render = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, render, TRUE);
   gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_VALUE);
   gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

   g_signal_connect (G_OBJECT (clist),"cursor-changed",
                     G_CALLBACK (cb_tree_view_cursor_changed), cv);

   gtk_container_add (GTK_CONTAINER (scrolledwin), clist);
   g_signal_connect (G_OBJECT (clist), "key_press_event",
                     G_CALLBACK (cb_data_list_key_press), cv);
   /* entry area */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   vbox1 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
   label = gtk_label_new (_("Key: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

   cv->key_combo = combo = gtk_combo_new ();
   gtk_box_pack_start (GTK_BOX (vbox1), combo, TRUE, TRUE, 0);
   gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), FALSE);

   vbox1 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);
   label = gtk_label_new (_("Value: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
   cv->value_entry = entry = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX (vbox1), entry, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (entry), "changed",
                     G_CALLBACK (cb_entry_changed), cv);
   g_signal_connect (G_OBJECT (entry), "activate",
                     G_CALLBACK (cb_entry_enter), cv);
   g_signal_connect (G_OBJECT (entry), "key_press_event",
                     G_CALLBACK (cb_entry_key_press), cv);

   gtk_widget_show_all (cv->data_page);

   return vbox;
}


static void
cb_text_buffer_changed (GtkTextBuffer *buffer, GimvCommentView *cv)
{
   if (cv->comment) {
      cv->changed = TRUE;
   }
}


static GtkWidget *
create_note_page (GimvCommentView *cv)
{
   GtkWidget *scrolledwin;
   GtkWidget *label;
   GtkTextBuffer *textbuf;

   /* "Note" page */
   label = gtk_label_new (_("Note"));
   gtk_widget_set_name (label, "TabLabel");
   gtk_widget_show (label);

   cv->note_page = scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
                                       GTK_SHADOW_IN);
   gtk_widget_show (scrolledwin);

   cv->note_box = gtk_text_view_new ();
   textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));
   g_signal_connect (G_OBJECT (textbuf), "changed",
                     G_CALLBACK (cb_text_buffer_changed), cv);
   gtk_container_add (GTK_CONTAINER (scrolledwin), cv->note_box);
   gtk_widget_show (cv->note_box);

   gtk_notebook_append_page (GTK_NOTEBOOK(cv->notebook),
                             scrolledwin, label);

   return cv->note_page;
}


static void
gimv_comment_view_set_sensitive_all (GimvCommentView *cv, gboolean sensitive)
{
   g_return_if_fail (cv);

   gtk_widget_set_sensitive (cv->comment_clist, sensitive);
   gtk_widget_set_sensitive (cv->note_box, sensitive);

   gtk_widget_set_sensitive (cv->key_combo, sensitive);
   gtk_widget_set_sensitive (cv->value_entry, sensitive);

   gtk_widget_set_sensitive (cv->save_button, sensitive);
   gtk_widget_set_sensitive (cv->reset_button, sensitive);
   gtk_widget_set_sensitive (cv->delete_button, sensitive);
}


static void
gimv_comment_view_set_sensitive (GimvCommentView *cv)
{
   const gchar *key_str, *value_str;

   g_return_if_fail (cv);

   key_str   = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry));
   value_str = gtk_entry_get_text (GTK_ENTRY (cv->value_entry));

   if (!cv->comment || !GTK_WIDGET_VISIBLE (cv->button_area)) {
      gimv_comment_view_set_sensitive_all (cv, FALSE);
      return;
   } else {
      gimv_comment_view_set_sensitive_all (cv, TRUE);
   }
}


static void
entry_list_remove_item (GtkWidget *widget, GtkContainer *container)
{
   g_return_if_fail (widget && GTK_IS_WIDGET (widget));
   g_return_if_fail (container && GTK_IS_CONTAINER (container));

   gtk_container_remove (container, widget);
}


static void
gimv_comment_view_set_combo_list (GimvCommentView *cv)
{
   GList *list;
   GtkWidget *first_label = NULL;

   g_return_if_fail (cv);
   g_return_if_fail (cv->key_combo);

   gtk_container_foreach (GTK_CONTAINER (GTK_COMBO (cv->key_combo)->list),
                          (GtkCallback) (entry_list_remove_item),
                          GTK_COMBO (cv->key_combo)->list);

   list = gimv_comment_get_data_entry_list ();
   while (list) {
      GimvCommentDataEntry *data_entry = list->data;
      GimvImageInfo *info;
      GtkWidget *label;

      list = g_list_next (list);

      if (!data_entry) continue;
      if (!data_entry->display) continue;
      if (!data_entry->key || !*data_entry->key) continue;

      if (cv->comment)
         info = cv->comment->info;
      else
         info = NULL;

      if (!strcmp ("X-IMG-File-Path-In-Arcvhie", data_entry->key)
          && info && !gimv_image_info_is_in_archive (info))
      {
         continue;
      }

      label = gtk_list_item_new_with_label (_(data_entry->display_name));

      g_object_set_data_full (G_OBJECT (label), "key",
                              g_strdup (data_entry->key),
                              (GtkDestroyNotify) g_free);
      gtk_container_add (GTK_CONTAINER (GTK_COMBO (cv->key_combo)->list),
                         label);
      gtk_widget_show (label);

      g_signal_connect (G_OBJECT(label), "select",
                        G_CALLBACK (cb_combo_select), cv);
      g_signal_connect (G_OBJECT(label), "deselect",
                        G_CALLBACK (cb_combo_deselect), cv);

      if (!first_label)
         first_label = label;
   }

   gtk_list_item_select (GTK_LIST_ITEM (first_label));
}


static void
gimv_comment_view_data_enter (GimvCommentView *cv)
{
   GimvCommentDataEntry *entry;
   const gchar *key, *dname, *value;
   gchar *text[16];
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean go_next;
   GimvCommentDataEntry *src_entry;

   g_return_if_fail (cv);
   if (!cv->selected_item) return;

   key = g_object_get_data (G_OBJECT (cv->selected_item), "key");
   g_return_if_fail (key);

   dname = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry));
   value = gtk_entry_get_text (GTK_ENTRY (cv->value_entry));
   g_return_if_fail (dname && *dname);

   entry = gimv_comment_append_data (cv->comment, key, value);

   g_return_if_fail (entry);

   treeview = GTK_TREE_VIEW (cv->comment_clist);
   model = gtk_tree_view_get_model (treeview);

   go_next = gtk_tree_model_get_iter_first (model, &iter);
   for (; go_next; go_next = gtk_tree_model_iter_next (model, &iter)) {
      gtk_tree_model_get (model, &iter,
                          COLUMN_RAW_ENTRY, &src_entry,
                          COLUMN_TERMINATOR);
      if (src_entry == entry) break;
   }

   text[0] = entry->display_name;
   text[1] = entry->value;
   if (!entry->userdef) text[0] = _(text[0]);

   if (!go_next)
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
   gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                       COLUMN_KEY,       text[0],
                       COLUMN_VALUE,     text[1],
                       COLUMN_RAW_ENTRY, entry,
                       COLUMN_TERMINATOR);

   cv->changed = TRUE;

   gimv_comment_view_set_sensitive (cv);
}


static void
gimv_comment_view_reset_data (GimvCommentView *cv)
{
   GimvImageInfo *info;
   GList *node;
   gchar *text[2];

   gimv_comment_view_clear (cv);
   if (cv->comment) {
      info = cv->comment->info;
      node = cv->comment->data_list;
      while (node) {
         GimvCommentDataEntry *entry = node->data;
         GtkTreeModel *model;
         GtkTreeIter iter;

         node = g_list_next (node);

         if (!entry) continue;
         if (!entry->display) continue;

         if (!strcmp ("X-IMG-File-Path-In-Arcvhie", entry->key)
             && info && !gimv_image_info_is_in_archive (info))
         {
            continue;
         }

         text[0] = _(entry->display_name);
         text[1] = entry->value;

         model = gtk_tree_view_get_model (GTK_TREE_VIEW (cv->comment_clist));
         gtk_list_store_append (GTK_LIST_STORE (model), &iter);
         gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                             COLUMN_KEY,       text[0],
                             COLUMN_VALUE,     text[1],
                             COLUMN_RAW_ENTRY, entry,
                             COLUMN_TERMINATOR);
      }

      if (cv->comment->note && *cv->comment->note) {
         GtkTextBuffer *buffer;

         buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));
         gtk_text_buffer_set_text (buffer, cv->comment->note, -1);
      }
   }

   cv->changed = FALSE;

   gimv_comment_view_set_sensitive (cv);
}


/******************************************************************************
 *
 *   public functions.
 *
 ******************************************************************************/
void
gimv_comment_view_clear (GimvCommentView *cv)
{
   GtkTextBuffer *buffer;
   GtkTreeModel *model;

   g_return_if_fail (cv);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (cv->comment_clist));
   gtk_list_store_clear (GTK_LIST_STORE (model));

   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (cv->key_combo)->entry), "\0");
   gtk_entry_set_text (GTK_ENTRY (cv->value_entry), "\0");

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (cv->note_box));
   gtk_text_buffer_set_text (buffer, "\0", -1);

   cv->changed = FALSE;

   gimv_comment_view_set_sensitive (cv);
}


gboolean
gimv_comment_view_change_file (GimvCommentView *cv, GimvImageInfo *info)
{
   g_return_val_if_fail (cv, FALSE);

   gimv_comment_view_clear (cv);

   if (cv->comment) {
      g_object_unref (G_OBJECT (cv->comment));
      cv->comment = NULL;
   }

   cv->comment = gimv_comment_get_from_image_info (info);
   g_signal_connect (G_OBJECT (cv->comment), "file_saved",
                     G_CALLBACK (cb_file_saved), cv);

   gimv_comment_view_reset_data (cv);
   gimv_comment_view_set_combo_list (cv);

   return TRUE;
}


GimvCommentView *
gimv_comment_view_create (void)
{
   GimvCommentView *cv;
   GtkWidget *hbox, *hbox1;
   GtkWidget *button;
   guint key;

   cv = gimv_comment_view_new ();

   cv->accel_group = gtk_accel_group_new ();
   cv->iv = NULL;
   cv->next_button = NULL;
   cv->prev_button = NULL;
   cv->changed = FALSE;

   cv->main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_widget_set_name (cv->main_vbox, "GimvCommentView");
   g_signal_connect (G_OBJECT (cv->main_vbox), "destroy",
                     G_CALLBACK (cb_destroyed), cv);

   cv->notebook = gtk_notebook_new ();
   gtk_container_set_border_width (GTK_CONTAINER (cv->notebook), 1);
   gtk_notebook_set_scrollable (GTK_NOTEBOOK (cv->notebook), TRUE);
   gtk_box_pack_start(GTK_BOX(cv->main_vbox), cv->notebook, TRUE, TRUE, 0);
   gtk_widget_show (cv->notebook);

   g_signal_connect (G_OBJECT(cv->notebook), "switch-page",
                     G_CALLBACK(cb_switch_page), cv);

   create_data_page (cv);
   create_note_page (cv);

   /* button area */
   hbox = cv->button_area = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start(GTK_BOX(cv->main_vbox), cv->button_area, FALSE, FALSE, 2);
   gtk_widget_show (cv->main_vbox);

   hbox1 = cv->inner_button_area = gtk_hbox_new (TRUE, 0);
   gtk_box_pack_end (GTK_BOX (hbox), hbox1, FALSE, TRUE, 0);
   gtk_box_set_homogeneous (GTK_BOX(hbox1), FALSE);

   button = gtk_button_new_with_label (_("_Save"));
   cv->save_button = button;
   key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                               _("_Save"));
   gtk_widget_add_accelerator(button, "clicked",
                              cv->accel_group, key, GDK_MOD1_MASK, 0);
   g_signal_connect (G_OBJECT (button),"clicked",
                     G_CALLBACK (cb_save_button_pressed), cv);
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   /* GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT); */

   button = gtk_button_new_with_label (_("_Reset"));
   cv->reset_button = button;
   key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                               _("_Reset"));
   gtk_widget_add_accelerator(button, "clicked",
                              cv->accel_group, key, GDK_MOD1_MASK, 0);
   g_signal_connect (G_OBJECT (button),"clicked",
                     G_CALLBACK (cb_reset_button_pressed), cv);
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   /* GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT); */

   button = gtk_button_new_with_label (_("_Delete"));
   cv->delete_button = button;
   key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                               _("_Delete"));
   gtk_widget_add_accelerator(button, "clicked",
                              cv->accel_group, key, GDK_MOD1_MASK, 0);
   g_signal_connect (G_OBJECT (button),"clicked",
                     G_CALLBACK (cb_del_button_pressed), cv);
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   /* GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT); */

   gtk_widget_show_all (cv->main_vbox);

   gimv_comment_view_set_sensitive (cv);

   return cv;
}


static void
cb_prev_button_pressed (GtkButton *button, GimvCommentView *cv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (cv->iv));

   gimv_image_view_prev (cv->iv);
}


static void
cb_next_button_pressed (GtkButton *button, GimvCommentView *cv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (cv->iv));

   gimv_image_view_next (cv->iv);
}


GimvCommentView *
gimv_comment_view_create_window (GimvImageInfo *info)
{
   GimvCommentView *cv;
   gchar buf[BUF_SIZE];

   g_return_val_if_fail (info, NULL);

   cv = gimv_comment_view_create ();
   if (!cv) return NULL;

   gtk_container_set_border_width (GTK_CONTAINER (cv->main_vbox), 5);

   cv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   g_snprintf (buf, BUF_SIZE, _("Edit Comment (%s)"),
               gimv_image_info_get_path (info));
   gtk_window_set_title (GTK_WINDOW (cv->window), buf); 
   gtk_window_set_default_size (GTK_WINDOW (cv->window), 400, 350);
   gtk_window_set_position (GTK_WINDOW (cv->window), GTK_WIN_POS_MOUSE);
   if (cv->accel_group)
      gtk_window_add_accel_group (GTK_WINDOW (cv->window), cv->accel_group);
   gtk_container_add (GTK_CONTAINER (cv->window), cv->main_vbox);

   gtk_widget_show_all (cv->window);

   gimv_comment_view_change_file (cv, info);

   gimv_icon_stock_set_window_icon (cv->window->window, "gimv_icon");

   return cv;
}


/* FIXME */
void
gimv_comment_view_set_image_view (GimvCommentView *cv, GimvImageView *iv)
{
   g_return_if_fail (cv);

   /* add next and prev button */
   if (iv) {
      GtkWidget *hbox, *sep, *button;
      guint key;

      /* cv->iv = iv; */

      hbox = gtk_hbox_new (TRUE, 0);
      gtk_box_pack_start (GTK_BOX (cv->button_area), hbox, TRUE, TRUE, 0);

      sep = gtk_vseparator_new ();
      gtk_box_pack_start (GTK_BOX (cv->button_area),
                          sep, TRUE, TRUE, 2);

      button = gtk_button_new_with_label (_("_Prev"));
      cv->prev_button = button;
      key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Prev"));
      gtk_widget_add_accelerator(button, "clicked",
                                 cv->accel_group, key, GDK_MOD1_MASK, 0);
      g_signal_connect (G_OBJECT (button),"clicked",
                        G_CALLBACK (cb_prev_button_pressed), cv);
      gtk_box_pack_start (GTK_BOX (hbox),
                          button, FALSE, TRUE, 2);

      button = gtk_button_new_with_label (_("_Next"));
      cv->next_button = button;
      key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(button)->child),
                                  _("_Next"));
      gtk_widget_add_accelerator(button, "clicked",
                                 cv->accel_group, key, GDK_MOD1_MASK, 0);
      g_signal_connect (G_OBJECT (button),"clicked",
                        G_CALLBACK (cb_next_button_pressed), cv);
      gtk_box_pack_start (GTK_BOX (hbox),
                          button, FALSE, TRUE, 2);
   }
}
