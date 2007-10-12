/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * gimv_dlist.c - Pair of reorderable stack list widget.
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
 * Foundation, Inc., 59 Temple Place - Suite 3S30, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include "gimv_dlist.h"

#include <string.h>
#include <stdlib.h>

#include "gtk2-compat.h"
#include "intl.h" /* FIXME */


enum {
   ENABLED_LIST_UPDATED_SIGNAL,
   LAST_SIGNAL
};


#define list_widget_get_row_num(widget) \
   gtk_tree_model_iter_n_children (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)), NULL);


static void       gimv_dlist_init                     (GimvDList *dslist);
static void       gimv_dlist_class_init               (GimvDListClass *klass);

/* object class functions */
static void       gimv_dlist_finalize                 (GObject *object);

/* private functions */
static void       gimv_dlist_enabled_list_updated     (GimvDList *dslist);
static void       gimv_dlist_set_sensitive            (GimvDList *dslist);
static GtkWidget *gimv_dlist_create_list_widget       (GimvDList *dslist,
                                                       gboolean    reorderble);


static GtkHBoxClass *parent_class = NULL;
static gint gimv_dlist_signals[LAST_SIGNAL] = {0};


GtkType
gimv_dlist_get_type (void)
{
   static GtkType gimv_dlist_type = 0;

   if (!gimv_dlist_type) {
      static const GTypeInfo gimv_dlist_info = {
         sizeof (GimvDListClass),
         NULL,               /* base_init */
         NULL,               /* base_finalize */
         (GClassInitFunc)    gimv_dlist_class_init,
         NULL,               /* class_finalize */
         NULL,               /* class_data */
         sizeof (GimvDList),
         0,                  /* n_preallocs */
         (GInstanceInitFunc) gimv_dlist_init,
      };

      gimv_dlist_type = g_type_register_static (GTK_TYPE_HBOX,
                                                 "GimvDList",
                                                 &gimv_dlist_info,
                                                 0);
   }

   return gimv_dlist_type;
}


static void
gimv_dlist_init (GimvDList *dslist)
{
   dslist->clist1          = NULL;
   dslist->clist2          = NULL;
   dslist->add_button      = NULL;
   dslist->del_button      = NULL;
   dslist->up_button       = NULL;
   dslist->down_button     = NULL;

   dslist->clist1_rows     = 0;
   dslist->clist1_selected = -1;
   dslist->clist1_dest_row = -1;
   dslist->clist2_rows     = 0;
   dslist->clist2_selected = -1;
   dslist->clist2_dest_row = -1;
   dslist->available_list  = NULL;
}


static void
gimv_dlist_class_init (GimvDListClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gtk_hbox_get_type ());

   gimv_dlist_signals[ENABLED_LIST_UPDATED_SIGNAL]
      = gtk_signal_new ("enabled-list-updated",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE(object_class),
                        GTK_SIGNAL_OFFSET (GimvDListClass, enabled_list_updated),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gtk_object_class_add_signals (object_class, gimv_dlist_signals, LAST_SIGNAL);

   OBJECT_CLASS_SET_FINALIZE_FUNC (klass, gimv_dlist_finalize);
}



/*******************************************************************************
 *
 *  Object Class functions.
 *
 *******************************************************************************/
static void
gimv_dlist_finalize (GObject *object)
{
   GimvDList *dslist = GIMV_DLIST (object);

   g_list_foreach (dslist->available_list, (GFunc) g_free, NULL);
   g_list_free (dslist->available_list);
   dslist->available_list = NULL;

   OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}



/*******************************************************************************
 *
 *  Callback functions for child widget.
 *
 *******************************************************************************/

static void
cb_gimv_dlist_cursor_changed (GtkTreeView *treeview, gpointer data)
{
   GimvDList *dslist = data;
   GtkTreeSelection *selection;
   GtkTreeModel *model = NULL;
   GtkTreeIter iter;
   gint selected;
   gboolean success;

   g_return_if_fail (treeview);
   g_return_if_fail (dslist);

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);

   if (success) {
      GtkTreePath *treepath = gtk_tree_model_get_path (model, &iter);
      gchar *path = gtk_tree_path_to_string (treepath);

      selected = atoi (path);

      gtk_tree_path_free (treepath);
      g_free (path);
   } else {
      selected = -1;
   }

   if (treeview == GTK_TREE_VIEW (dslist->clist1))
      dslist->clist1_selected = selected;
   else if  (treeview == GTK_TREE_VIEW (dslist->clist2))
      dslist->clist2_selected = selected;

   gimv_dlist_set_sensitive (dslist);
}


static void
cb_gimv_dlist_row_changed (GtkTreeModel *model,
                       GtkTreePath *treepath,
                       GtkTreeIter *iter,
                       gpointer data)
{
   GimvDList *dslist = data;

   gimv_dlist_enabled_list_updated (dslist);
}


static void
cb_gimv_dlist_row_deleted (GtkTreeModel *model,
                       GtkTreePath *treepath,
                       gpointer data)
{
   GimvDList *dslist = data;

   gimv_dlist_enabled_list_updated (dslist);
   if (dslist->clist2_dest_row < 0) {
      dslist->clist2_selected = -1;
      gimv_dlist_set_sensitive (dslist);
   }
   dslist->clist2_dest_row = -1;
}


static void
cb_gimv_dlist_add_button_pressed (GtkButton *button, gpointer data)
{
   GimvDList *dslist = data;
   gint idx;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   gboolean success;

   if (dslist->clist1_selected < 0) return;

   treeview = GTK_TREE_VIEW (dslist->clist1);
   model = gtk_tree_view_get_model (treeview);

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;

   gtk_tree_model_get (model, &iter, 2, &idx, -1);

   gimv_dlist_column_add (dslist, idx);

   gimv_dlist_enabled_list_updated (dslist);
   gimv_dlist_set_sensitive (dslist);
}


static void
cb_gimv_dlist_del_button_pressed (GtkButton *button, gpointer data)
{
   GimvDList *dslist = data;
   gint idx;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   gboolean success;

   if (dslist->clist2_selected < 0) return;

   treeview = GTK_TREE_VIEW (dslist->clist2);
   model = gtk_tree_view_get_model (treeview);

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;

   gtk_tree_model_get (model, &iter, 2, &idx, -1);

   gimv_dlist_column_del (dslist, idx);

   gimv_dlist_enabled_list_updated (dslist);
   gimv_dlist_set_sensitive (dslist);
}


static void
cb_gimv_dlist_up_button_pressed (GtkButton *button, gpointer data)
{
   GimvDList *dslist = data;
   gint selected = dslist->clist2_selected;
   gint rows = dslist->clist2_rows;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreePath *treepath;
   GtkTreeIter iter, prev_iter, dest_iter;
   gboolean success;
   GValue *values;
   gint i, colnum;

   g_return_if_fail (button && dslist);

   if (selected < 1 || selected > rows - 1) return;

   dslist->clist2_dest_row = dslist->clist2_selected - 1;

   treeview = GTK_TREE_VIEW (dslist->clist2);
   model = gtk_tree_view_get_model (treeview);
   selection = gtk_tree_view_get_selection (treeview);

   colnum = gtk_tree_model_get_n_columns (model);

   /* get src row */
   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;
   treepath = gtk_tree_model_get_path (model, &iter);

   /* get prev row */
   success = gtk_tree_path_prev (treepath);
   if (!success) {
      gtk_tree_path_free (treepath);
      return;
   }
   gtk_tree_model_get_iter (model, &prev_iter, treepath);

   /* get src data */
   values = g_new0 (GValue, colnum);
   for (i = 0; i < colnum; i++) {
      gtk_tree_model_get_value (model, &iter, i, &values[i]);
   }

   /* insert dest row before prev */
   gtk_list_store_insert_before (GTK_LIST_STORE (model),
                                 &dest_iter, &prev_iter);
   for (i = 0; i < colnum; i++) {
      gtk_list_store_set_value (GTK_LIST_STORE (model), &dest_iter,
                                i, &values[i]);
      g_value_unset (&values[i]);
   }
   g_free (values);

   /* delete src */
   gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

   /* select dest */
   gtk_tree_path_free (treepath);
   treepath = gtk_tree_model_get_path (model, &dest_iter);
   gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);

   /* clean */
   gtk_tree_path_free (treepath);
}


static void
cb_gimv_dlist_down_button_pressed (GtkButton *button, gpointer data)
{
   GimvDList *dslist = data;
   gint selected = dslist->clist2_selected;
   gint rows = dslist->clist2_rows;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter, next_iter, dest_iter;
   GtkTreePath *treepath;
   gboolean success;
   GValue *values;
   gint i, colnum;

   g_return_if_fail (button && dslist);

   if (selected < 0 || selected > rows - 2) return;

   dslist->clist2_dest_row = dslist->clist2_selected + 1;

   treeview = GTK_TREE_VIEW (dslist->clist2);
   model = gtk_tree_view_get_model (treeview);
   selection = gtk_tree_view_get_selection (treeview);
 
   colnum = gtk_tree_model_get_n_columns (model);

   /* get src row */
   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;

   /* get prev row */
   next_iter = iter;
   success = gtk_tree_model_iter_next (model, &next_iter);
   if (!success) return;

   /* get src data */
   values = g_new0 (GValue, colnum);
   for (i = 0; i < colnum; i++) {
      gtk_tree_model_get_value (model, &iter, i, &values[i]);
   }

   /* insert dest row before prev */
   gtk_list_store_insert_after (GTK_LIST_STORE (model),
                                &dest_iter, &next_iter);
   for (i = 0; i < colnum; i++) {
      gtk_list_store_set_value (GTK_LIST_STORE (model), &dest_iter,
                                i, &values[i]);
      g_value_unset (&values[i]);
   }
   g_free (values);

   /* delete src */
   gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

   /* select dest */
   treepath = gtk_tree_model_get_path (model, &dest_iter);
   gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);

   /* clean */
   gtk_tree_path_free (treepath);
}



/*******************************************************************************
 *
 *  Private functions.
 *
 *******************************************************************************/
static void
gimv_dlist_enabled_list_updated (GimvDList *dslist)
{
   g_return_if_fail (GIMV_IS_DLIST (dslist));

   gtk_signal_emit (GTK_OBJECT (dslist),
                    gimv_dlist_signals[ENABLED_LIST_UPDATED_SIGNAL]);
}


static void
gimv_dlist_set_sensitive (GimvDList *dslist)
{
   gint selected1 = dslist->clist1_selected;
   gint selected2 = dslist->clist2_selected;
   gint rows1 = dslist->clist1_rows;
   gint rows2 = dslist->clist2_rows;

   /* g_return_if_fail (dslist); */

   /* add button */
   if (selected1 < 0 || selected1 >= rows1) {
      gtk_widget_set_sensitive (dslist->add_button, FALSE);
   } else {
      gtk_widget_set_sensitive (dslist->add_button, TRUE);
   }

   /* delete button */
   if (selected2 < 0 || selected2 >= rows2) {
      gtk_widget_set_sensitive (dslist->del_button, FALSE);
   } else {
      gtk_widget_set_sensitive (dslist->del_button, TRUE);
   }

   /* up button */
   if (selected2 > 0 && selected2 < rows2) {
      gtk_widget_set_sensitive (dslist->up_button, TRUE);
   } else {
      gtk_widget_set_sensitive (dslist->up_button, FALSE);
   }

   /* down button */
   if (selected2 >= 0 && selected2 < rows2 - 1) {
      gtk_widget_set_sensitive (dslist->down_button, TRUE);
   } else {
      gtk_widget_set_sensitive (dslist->down_button, FALSE);
   }
}


static GtkWidget *
gimv_dlist_create_list_widget (GimvDList *dslist, gboolean reorderble)
{
   GtkWidget *clist;

   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;

   store = gtk_list_store_new (3,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_INT);
   clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   dslist->clist2 = clist;
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (clist), FALSE);

   if (reorderble)
      gtk_tree_view_set_reorderable (GTK_TREE_VIEW (clist), TRUE);

   g_signal_connect (G_OBJECT (store), "row_changed",
                     G_CALLBACK (cb_gimv_dlist_row_changed),
                     dslist);
   g_signal_connect (G_OBJECT (store), "row_deleted",
                     G_CALLBACK (cb_gimv_dlist_row_deleted),
                     dslist);
   g_signal_connect (G_OBJECT (clist),"cursor_changed",
                     G_CALLBACK (cb_gimv_dlist_cursor_changed),
                     dslist);

   /* set column */
   col = gtk_tree_view_column_new();
   render = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, render, TRUE);
   gtk_tree_view_column_add_attribute (col, render, "text", 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

   return clist;
}



/*******************************************************************************
 *
 *  Public functions.
 *
 *******************************************************************************/
GtkWidget *
gimv_dlist_new (const gchar *clist1_title,
                 const gchar *clist2_title)
{
   GimvDList *dslist = g_object_new (gimv_dlist_get_type (), NULL);

   GtkWidget *hbox = GTK_WIDGET (dslist);
   GtkWidget *vbox, *vbox1, *vbox2, *vbox3, *hseparator;
   GtkWidget *label, *scrollwin1, *scrollwin2, *clist, *button, *arrow;

   /* possible columns */
   vbox1 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
   gtk_widget_show (vbox1);

   label = gtk_label_new (clist1_title);
   gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   scrollwin1 = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin1),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin1),
                                       GTK_SHADOW_IN);
   gtk_container_set_border_width(GTK_CONTAINER(scrollwin1), 5);
   gtk_box_pack_start (GTK_BOX (vbox1), scrollwin1, TRUE, TRUE, 0);
   gtk_widget_show (scrollwin1);

   clist = dslist->clist1 = gimv_dlist_create_list_widget (dslist, FALSE);
   gtk_container_add (GTK_CONTAINER (scrollwin1), clist);
   gtk_widget_show (clist);


   /* add/delete buttons */
   vbox3 = gtk_vbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
   gtk_widget_show (vbox3);

   vbox = gtk_vbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (vbox3), vbox, FALSE, FALSE, 0);
   gtk_widget_show (vbox);

   button = dslist->add_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Add"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
   gtk_widget_show (button);

   button = dslist->del_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Delete"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
   gtk_widget_show (button);

   hseparator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, FALSE, 2);
   gtk_widget_show (hseparator);


   /* move buttons */
   button = dslist->up_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Up"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
   gtk_widget_show (button);

   button = dslist->down_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Down"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 2);
   gtk_widget_show (button);


   /* Use list */
   vbox2 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
   gtk_widget_show (vbox2);

   label = gtk_label_new (clist2_title);
   gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   scrollwin2 = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin2),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin2),
                                       GTK_SHADOW_IN);
   gtk_container_set_border_width(GTK_CONTAINER(scrollwin2), 5);
   gtk_box_pack_start (GTK_BOX (vbox2), scrollwin2, TRUE, TRUE, 0);
   gtk_widget_show (scrollwin2);

   clist = dslist->clist2 = gimv_dlist_create_list_widget (dslist, TRUE);
   gtk_container_add (GTK_CONTAINER (scrollwin2), clist);
   dslist->clist2_rows = list_widget_get_row_num (clist);
   gtk_widget_show (clist);

   gtk_widget_set_size_request (scrollwin1, -1, 200);
   gtk_widget_set_size_request (scrollwin2, -1, 200);

#ifdef USE_ARROW
   gtk_widget_set_size_request (dslist->add_button,  20, 20);
   gtk_widget_set_size_request (dslist->del_button,  20, 20);
   gtk_widget_set_size_request (dslist->up_button,   20, 20);
   gtk_widget_set_size_request (dslist->down_button, 20, 20);
#endif /* USE_ARROW */

   g_signal_connect (G_OBJECT (dslist->add_button), "clicked",
                     G_CALLBACK (cb_gimv_dlist_add_button_pressed),
                     dslist);
   g_signal_connect (G_OBJECT (dslist->del_button), "clicked",
                     G_CALLBACK (cb_gimv_dlist_del_button_pressed),
                     dslist);
   g_signal_connect (G_OBJECT (dslist->up_button), "clicked",
                     G_CALLBACK (cb_gimv_dlist_up_button_pressed),
                     dslist);
   g_signal_connect (G_OBJECT (dslist->down_button), "clicked",
                     G_CALLBACK (cb_gimv_dlist_down_button_pressed),
                     dslist);

   return hbox;
}


gint
gimv_dlist_append_available_item (GimvDList *dslist, const gchar *item)
{
   GtkWidget *clist = dslist->clist1;
   gchar *text = g_strdup (item);
   gchar *i18n_text = _(text);
   gint idx;
   GtkTreeModel *model;
   GtkListStore *store;
   GtkTreeIter iter;

   g_return_val_if_fail (GIMV_IS_DLIST (dslist), -1);

   dslist->available_list = g_list_append (dslist->available_list, text);
   idx = g_list_index (dslist->available_list, text);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (clist));
   store = GTK_LIST_STORE (model);

   gtk_list_store_append (store, &iter);
   gtk_list_store_set (store, &iter,
                       0, i18n_text, 1, text, 2, idx, -1);

   dslist->clist1_rows = list_widget_get_row_num (dslist->clist1);
   gimv_dlist_set_sensitive (dslist);

   return idx;
}


void
gimv_dlist_column_add (GimvDList *dslist, gint idx)
{
   gchar *text, *i18n_text;
   GList *list;
   GtkTreeView *treeview1;
   GtkTreeView *treeview2;
   GtkTreeModel *model1;
   GtkTreeModel *model2;
   GtkTreeIter iter1, iter2, next;
   gboolean go_next;
   gchar *str = NULL;

   list = g_list_nth (dslist->available_list, idx);
   g_return_if_fail (list);
   text = list->data;
   i18n_text = _(text);
   g_return_if_fail (text);

   treeview1 = GTK_TREE_VIEW (dslist->clist1);
   treeview2 = GTK_TREE_VIEW (dslist->clist2);
   model1 = gtk_tree_view_get_model (treeview1);
   model2 = gtk_tree_view_get_model (treeview2);

   /* find row from clist1 */
   go_next = gtk_tree_model_get_iter_first (model1, &iter1);
   for (; go_next; go_next = gtk_tree_model_iter_next (model1, &iter1)) {
      gtk_tree_model_get (model1, &iter1, 1, &str, -1);
      if (str && !strcmp (text, str)) break;
      g_free (str);
      str = NULL;
   }
   if (!str) return;

   /* append to clist2 */
   gtk_list_store_append (GTK_LIST_STORE (model2), &iter2);
   gtk_list_store_set (GTK_LIST_STORE (model2), &iter2,
                       0, i18n_text, 1, text, 2, idx, -1);

   /* set cursor of clist1 */
   next = iter1;
   if (gtk_tree_model_iter_next (model1, &next)) {
      GtkTreePath *path = gtk_tree_model_get_path (model1, &next);
      gtk_tree_view_set_cursor (treeview1, path, NULL, FALSE);
      gtk_tree_path_free (path);
   } else {
      GtkTreePath *path = gtk_tree_model_get_path (model1, &iter1);
      if (gtk_tree_path_prev (path))
         gtk_tree_view_set_cursor (treeview1, path, NULL, FALSE);
      gtk_tree_path_free (path);
   }

   /* remove from clist1 */
   gtk_list_store_remove (GTK_LIST_STORE (model1), &iter1);

   /* clean :-) */
   g_free (str);

   /* reset */
   dslist->clist1_rows = gtk_tree_model_iter_n_children (model1, NULL);
   dslist->clist2_rows = gtk_tree_model_iter_n_children (model2, NULL);

   g_signal_emit_by_name (G_OBJECT (treeview1), "cursor-changed");
}


void
gimv_dlist_column_del (GimvDList *dslist, gint idx)
{
   gchar *text, *i18n_text;
   GList *list;
   GtkTreeView *treeview1;
   GtkTreeView *treeview2;
   GtkTreeModel *model1;
   GtkTreeModel *model2;
   GtkTreeIter iter, iter1, iter2, next;
   gboolean go_next;
   gchar *str = NULL;

   list = g_list_nth (dslist->available_list, idx);
   g_return_if_fail (list);
   text = list->data;
   i18n_text = _(text);
   g_return_if_fail (text);

   treeview1 = GTK_TREE_VIEW (dslist->clist1);
   treeview2 = GTK_TREE_VIEW (dslist->clist2);
   model1 = gtk_tree_view_get_model (treeview1);
   model2 = gtk_tree_view_get_model (treeview2);

   /* find row from clist2 */
   go_next = gtk_tree_model_get_iter_first (model2, &iter2);
   for (; go_next; go_next = gtk_tree_model_iter_next (model2, &iter2)) {
      gtk_tree_model_get (model2, &iter2, 1, &str, -1);
      if (str && !strcmp (text, str)) break;
      g_free (str);
      str = NULL;
   }
   if (!str) return;

   /* append to clist1 */
   go_next = gtk_tree_model_get_iter_first (model1, &iter1);
   for (; go_next; go_next = gtk_tree_model_iter_next (model1, &iter1)) {
      gint idx1;
      gtk_tree_model_get (model1, &iter1, 2, &idx1, -1);
      if (idx < idx1) break;
   }

   if (go_next)
      gtk_list_store_insert_before (GTK_LIST_STORE (model1), &iter, &iter1);
   else
      gtk_list_store_append (GTK_LIST_STORE (model1), &iter);
   gtk_list_store_set (GTK_LIST_STORE (model1), &iter,
                       0, i18n_text, 1, text, 2, idx, -1);

   /* set cursor of clist2 */
   next = iter2;
   if (gtk_tree_model_iter_next (model2, &next)) {
      GtkTreePath *path = gtk_tree_model_get_path (model2, &next);
      gtk_tree_view_set_cursor (treeview2, path, NULL, FALSE);
      gtk_tree_path_free (path);
   } else {
      GtkTreePath *path = gtk_tree_model_get_path (model2, &iter2);
      if (gtk_tree_path_prev (path))
         gtk_tree_view_set_cursor (treeview2, path, NULL, FALSE);
      gtk_tree_path_free (path);
   }

   /* remove from clist2 */
   gtk_list_store_remove (GTK_LIST_STORE (model2), &iter2);

   /* clean :-) */
   g_free (str);

   dslist->clist1_rows = gtk_tree_model_iter_n_children (model1, NULL);
   dslist->clist2_rows = gtk_tree_model_iter_n_children (model2, NULL);

   g_signal_emit_by_name (G_OBJECT (treeview2), "cursor-changed");
}


void
gimv_dlist_column_add_by_label (GimvDList *dslist, const gchar *label)
{
   GList *list;
   gint j, idx = -1;

   g_return_if_fail (GIMV_IS_DLIST (dslist));
   g_return_if_fail (label && *label);

   list = dslist->available_list;
   for (j = 0; list; j++, list = g_list_next (list)) {
      if (!strcmp (label, (gchar *)list->data)) {
         idx = j;
         break;
      }
   }
   if (idx >= 0)
      gimv_dlist_column_add (dslist, idx);

   gimv_dlist_set_sensitive (dslist);
}


gint
gimv_dlist_get_n_available_items (GimvDList  *dslist)
{
   g_return_val_if_fail (GIMV_IS_DLIST (dslist), 0);
   return dslist->clist1_rows;
}


gint
gimv_dlist_get_n_enabled_items (GimvDList  *dslist)
{
   g_return_val_if_fail (GIMV_IS_DLIST (dslist), 0);
   return dslist->clist2_rows;
}


gchar *
gimv_dlist_get_enabled_row_text (GimvDList *dslist, gint row)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gchar *text;

   g_return_val_if_fail (GIMV_IS_DLIST (dslist), NULL);
   g_return_val_if_fail (row >= 0 && row < dslist->clist2_rows, NULL);

   treeview = GTK_TREE_VIEW (dslist->clist2);
   model = gtk_tree_view_get_model (treeview);

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   if (!success) return NULL;

   gtk_tree_model_get (model, &iter, 1, &text, -1);
   if (!text) return NULL;

   return text;
}
