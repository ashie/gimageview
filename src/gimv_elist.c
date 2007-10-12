/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * gimv_elist.c - a set of clist and edit area widget.
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

#include "gimv_elist.h"

#include <stdlib.h>
#include <string.h>

#include "intl.h"
#include "gimv_marshal.h"


enum {
   LIST_UPDATED_SIGNAL,
   EDIT_AREA_SET_DATA_SIGNAL,
   ACTION_CONFIRM_SIGNAL,
   LAST_SIGNAL
};


#include <gobject/gvaluecollector.h>

#define list_widget_get_row_num(widget) \
   gtk_tree_model_iter_n_children (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)), NULL);

#define ROWDATA(columns)     columns
#define ROWDESTROY(columns)  columns + 1
#define ALL_COLUMNS(columns) columns + 2


static void gimv_elist_init       (GimvEList *editlist);
static void gimv_elist_class_init (GimvEListClass *klass);
static void gimv_elist_finalize   (GObject *object);

/* private */
static void     gimv_elist_set_move_button_sensitive   (GimvEList *editlist);
static void     gimv_elist_set_sensitive               (GimvEList *editlist);
static void     gimv_elist_edit_area_set_data          (GimvEList *editlist,
                                                        gint          row);
static gchar  **gimv_elist_edit_area_get_data          (GimvEList *editlist,
                                                        GimvEListActionType type);
static void     gimv_elist_edit_area_reset             (GimvEList *editlist);


static GtkVBoxClass *parent_class = NULL;
static gint gimv_elist_signals[LAST_SIGNAL] = {0};


GtkType
gimv_elist_get_type (void)
{
   static GtkType gimv_elist_type = 0;

   if (!gimv_elist_type) {
      static const GTypeInfo gimv_elist_info = {
         sizeof (GimvEListClass),
         NULL,               /* base_init */
         NULL,               /* base_finalize */
         (GClassInitFunc)    gimv_elist_class_init,
         NULL,               /* class_finalize */
         NULL,               /* class_data */
         sizeof (GimvEList),
         0,                  /* n_preallocs */
         (GInstanceInitFunc) gimv_elist_init,
      };

      gimv_elist_type = g_type_register_static (GTK_TYPE_VBOX,
                                                   "GimvEList",
                                                   &gimv_elist_info,
                                                   0);
   }

   return gimv_elist_type;
}


static void
gimv_elist_init (GimvEList *editlist)
{
   editlist->clist              = NULL;
   editlist->new_button         = NULL;
   editlist->add_button         = NULL;
   editlist->change_button      = NULL;
   editlist->del_button         = NULL;
   editlist->up_button          = NULL;
   editlist->down_button        = NULL;
   editlist->edit_area          = NULL;
   editlist->move_button_area   = NULL;
   editlist->action_button_area = NULL;

   editlist->max_row            = -1;
   editlist->columns            = 0;
   editlist->rows               = 0;
   editlist->selected           = -1;
   editlist->dest_row           = -1;

   editlist->edit_area_flags    = GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED;
   editlist->column_func_tables = NULL;
   editlist->get_rowdata_fn     = NULL;

   editlist->rowdata_table
      = g_hash_table_new (g_direct_hash, g_direct_equal);
   editlist->rowdata_destroy_fn_table
      = g_hash_table_new (g_direct_hash, g_direct_equal);
}


static void
gimv_elist_class_init (GimvEListClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gtk_vbox_get_type ());

   gimv_elist_signals[LIST_UPDATED_SIGNAL]
      = gtk_signal_new ("list-updated",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE(object_class),
                        GTK_SIGNAL_OFFSET (GimvEListClass, list_updated),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_elist_signals[EDIT_AREA_SET_DATA_SIGNAL]
      = gtk_signal_new ("edit-area-set-data",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE(object_class),
                        GTK_SIGNAL_OFFSET (GimvEListClass, edit_area_set_data),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_elist_signals[ACTION_CONFIRM_SIGNAL]
      = gtk_signal_new ("action-confirm",
                        GTK_RUN_LAST,
                        GTK_CLASS_TYPE(object_class),
                        GTK_SIGNAL_OFFSET (GimvEListClass, action_confirm),
                        gtk_marshal_NONE__INT_INT_POINTER,
                        GTK_TYPE_NONE, 3, GTK_TYPE_INT, GTK_TYPE_INT, GTK_TYPE_POINTER);

   gtk_object_class_add_signals (object_class, gimv_elist_signals, LAST_SIGNAL);

   OBJECT_CLASS_SET_FINALIZE_FUNC (klass, gimv_elist_finalize);
}


static void
gimv_elist_updated (GimvEList *editlist)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   gtk_signal_emit (GTK_OBJECT (editlist),
                    gimv_elist_signals[LIST_UPDATED_SIGNAL]);
}



/*******************************************************************************
 *
 *  Object class functions.
 *
 *******************************************************************************/
static void
free_rowdata (gpointer key, gpointer value, gpointer data)
{
   GimvEList *editlist = data;
   GtkDestroyNotify destroy;

   destroy = g_hash_table_lookup (editlist->rowdata_destroy_fn_table, value);
   if (destroy)
      destroy (value);
}


static void
gimv_elist_finalize (GObject *object)
{
   GimvEList *editlist = GIMV_ELIST (object);
   gint i;

   /* remove column funcs */
   for (i = 0; i < editlist->columns; i++) {
      GimvEListColumnFuncTable *table = editlist->column_func_tables[i];
      if (table->destroy_fn)
         table->destroy_fn (table->coldata);
      g_free (table);
      editlist->column_func_tables[i] = NULL;
   }
   g_free (editlist->column_func_tables);
   editlist->column_func_tables = NULL;

   g_hash_table_foreach (editlist->rowdata_table, free_rowdata, editlist);
   g_hash_table_destroy (editlist->rowdata_table);
   g_hash_table_destroy (editlist->rowdata_destroy_fn_table);

   OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}



/*******************************************************************************
 *
 *  Callback functions for child widget.
 *
 *******************************************************************************/
static void
cb_editlist_cursor_changed (GtkTreeView *treeview, gpointer data)
{
   GimvEList *editlist = data;
   GtkTreeSelection *selection;
   GtkTreeModel *model = NULL;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (treeview);
   g_return_if_fail (editlist);

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);

   if (success) {
      GtkTreePath *treepath = gtk_tree_model_get_path (model, &iter);
      gchar *path = gtk_tree_path_to_string (treepath);

      editlist->selected = atoi (path);

      gtk_tree_path_free (treepath);
      g_free (path);
   } else {
      editlist->selected = -1;
   }

   gimv_elist_edit_area_set_data (editlist, editlist->selected);

   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_row_changed (GtkTreeModel *model,
                         GtkTreePath *treepath,
                         GtkTreeIter *iter,
                         gpointer data)
{
   GimvEList *editlist = data;

   gimv_elist_updated (editlist);

   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_row_deleted (GtkTreeModel *model,
                         GtkTreePath *treepath,
                         gpointer data)
{
   GimvEList *editlist = data;

   gimv_elist_updated (editlist);

   if (editlist->dest_row < 0) {
      editlist->selected = -1;
      gimv_elist_set_sensitive (editlist);
   }
   editlist->dest_row = -1;

   gimv_elist_edit_area_set_data (editlist, editlist->selected);

   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_up_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreePath *treepath;
   GtkTreeIter iter, prev_iter, dest_iter;
   gboolean success;
   GValue *values;
   gint i, colnum;

   gint selected = editlist->selected;
   gint rows = editlist->rows;

   g_return_if_fail (button && editlist);

   if (selected < 1 || selected > rows - 1) return;

   editlist->dest_row = editlist->selected - 1;

   treeview = GTK_TREE_VIEW (editlist->clist);
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
cb_editlist_down_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   gint selected = editlist->selected;
   gint rows = editlist->rows;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter, next_iter, dest_iter;
   GtkTreePath *treepath;
   gboolean success; 
   GValue *values;
   gint i, colnum;

   g_return_if_fail (button && editlist);

   if (selected < 0 || selected > rows - 2) return;

   editlist->dest_row = editlist->selected + 1;

   treeview = GTK_TREE_VIEW (editlist->clist);
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


static void
cb_editlist_new_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   GimvEListConfirmFlags flags;

   flags = gimv_elist_action_confirm (editlist, GIMV_ELIST_ACTION_RESET);
   if (flags & GIMV_ELIST_CONFIRM_CANNOT_NEW) return;

   gimv_elist_unselect_all (editlist);
   gimv_elist_edit_area_reset (editlist);
   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_add_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   GimvEListConfirmFlags flags;
   gint row;
   gchar **text;
   gpointer rowdata = NULL;
   GtkDestroyNotify destroy_fn = NULL;
   gboolean set_rowdata = FALSE;

   g_return_if_fail (GIMV_IS_ELIST (editlist));

   flags = gimv_elist_action_confirm (editlist, GIMV_ELIST_ACTION_ADD);
   if (flags & GIMV_ELIST_CONFIRM_CANNOT_ADD) return;

   text = gimv_elist_edit_area_get_data (editlist, GIMV_ELIST_ACTION_ADD);
   g_return_if_fail (text);

   row = gimv_elist_append_row (editlist, text);
   g_strfreev (text);

   if (editlist->get_rowdata_fn)
      set_rowdata = editlist->get_rowdata_fn (editlist,
                                              GIMV_ELIST_ACTION_ADD,
                                              &rowdata, &destroy_fn);
   if (set_rowdata) {
      gimv_elist_set_row_data_full (editlist, row, rowdata, destroy_fn);
   }

   gimv_elist_updated (editlist);

   gimv_elist_edit_area_reset (editlist);
   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_change_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   GimvEListConfirmFlags flags;
   gchar **text;
   gint i;
   gpointer rowdata = NULL;
   GtkDestroyNotify destroy_fn = NULL;
   gboolean set_rowdata = FALSE;
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (GIMV_IS_ELIST (editlist));

   if (editlist->selected < 0 || editlist->selected >= editlist->rows) return;

   flags = gimv_elist_action_confirm (editlist, GIMV_ELIST_ACTION_CHANGE);
   if (flags & GIMV_ELIST_CONFIRM_CANNOT_CHANGE) return;

   text = gimv_elist_edit_area_get_data (editlist,
                                            GIMV_ELIST_ACTION_CHANGE);
   g_return_if_fail (text);

   treeview = GTK_TREE_VIEW (editlist->clist);

   selection = gtk_tree_view_get_selection (treeview);
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) {
      g_strfreev (text);
      return;
   }

   for (i = 0; i < editlist->columns; i++) {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, i, text[i], -1);
   }

   g_strfreev (text);

   if (editlist->get_rowdata_fn)
      set_rowdata = editlist->get_rowdata_fn (editlist,
                                              GIMV_ELIST_ACTION_CHANGE,
                                              &rowdata, &destroy_fn);
   if (set_rowdata) {
      gimv_elist_set_row_data_full (editlist, editlist->selected,
                                       rowdata, destroy_fn);
   }

   gimv_elist_updated (editlist);

   editlist->edit_area_flags &= ~GIMV_ELIST_EDIT_AREA_VALUE_CHANGED;

   gimv_elist_set_sensitive (editlist);
}


static void
cb_editlist_delete_button (GtkButton *button, gpointer data)
{
   GimvEList *editlist = data;
   GimvEListConfirmFlags flags;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (editlist->selected >= 0);

   flags = gimv_elist_action_confirm (editlist, GIMV_ELIST_ACTION_DELETE);
   if (flags & GIMV_ELIST_CONFIRM_CANNOT_DELETE) return;

   gimv_elist_remove_row (editlist, editlist->selected);

   gimv_elist_updated (editlist);

   gimv_elist_set_sensitive (editlist);
}


/*******************************************************************************
 *
 *  Private functions.
 *
 *******************************************************************************/
static void
gimv_elist_set_move_button_sensitive (GimvEList *editlist)
{
   gint rownum = editlist->rows;
   gint selected = editlist->selected;
   gboolean reorderble = gimv_elist_get_reorderable (editlist);

   if (!reorderble || selected < 0) {
      gtk_widget_set_sensitive (editlist->up_button, FALSE);
      gtk_widget_set_sensitive (editlist->down_button, FALSE);

   } else {
      /* up button */
      if (selected == 0)
         gtk_widget_set_sensitive (editlist->up_button, FALSE);
      else
         gtk_widget_set_sensitive (editlist->up_button, TRUE);

      /* down button */
      if (selected < rownum - 1)
         gtk_widget_set_sensitive (editlist->down_button, TRUE);
      else
         gtk_widget_set_sensitive (editlist->down_button, FALSE);
   }
}


static void
gimv_elist_set_sensitive (GimvEList *editlist)
{
   gimv_elist_set_move_button_sensitive (editlist);
   gimv_elist_set_action_button_sensitive (editlist);
}


static GtkWidget *
gimv_elist_create_list_widget (GimvEList *editlist, gint colnum)
{
   GtkWidget *clist;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;
   GType *types;
   gint i;

   store = gtk_list_store_new (1, G_TYPE_STRING); /* this column is dummy */

   /*
    *  types[colnum] is row data
    *  types[colnum+1] is GtkDestroyNotify
    */
   types = g_new0 (GType, ALL_COLUMNS(colnum));
   for (i = 0; i < colnum; i++)
      types[i] = G_TYPE_STRING;
   types[ROWDATA(colnum)]    = G_TYPE_POINTER;
   types[ROWDESTROY(colnum)] = G_TYPE_POINTER;
   gtk_list_store_set_column_types (store, ALL_COLUMNS (colnum), types);
   g_free (types);

   clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   editlist->clist = clist;
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);
   gtk_tree_view_set_reorderable (GTK_TREE_VIEW (clist), TRUE);

   g_signal_connect (G_OBJECT (store), "row_changed",
                     G_CALLBACK (cb_editlist_row_changed),
                     editlist);
   g_signal_connect (G_OBJECT (store), "row_deleted",
                     G_CALLBACK (cb_editlist_row_deleted),
                     editlist);
   g_signal_connect (G_OBJECT (clist),"cursor_changed",
                     G_CALLBACK (cb_editlist_cursor_changed),
                     editlist);

   /* set column */
   for (i = 0; i < colnum; i++) {
      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_resizable (col, TRUE);
      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render, "text", i);
      gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);
   }

   editlist->columns = colnum;

   return clist;
}


static GimvEListColumnFuncTable *
gimv_elist_column_func_table_new (void)
{
   GimvEListColumnFuncTable *table;
   table = g_new0 (GimvEListColumnFuncTable, 1);
   table->widget            = NULL;
   table->coldata           = NULL;
   table->destroy_fn        = NULL;
   table->set_data_fn       = NULL;
   table->get_data_fn       = NULL;
   table->reset_fn          = NULL;

   return table;
}


static void
gimv_elist_edit_area_set_data (GimvEList *editlist, gint row)
{
   gint i;

   g_return_if_fail (row < editlist->rows);

   gtk_signal_emit (GTK_OBJECT (editlist),
                    gimv_elist_signals[EDIT_AREA_SET_DATA_SIGNAL]);

   for (i = 0; i < editlist->columns; i++) {
      GimvEListColumnFuncTable *table = editlist->column_func_tables[i];
      gchar *text = NULL;

      if (!table) continue;
      if (!table->set_data_fn) continue;

      if (row >= 0) {
         GtkTreeView *treeview = GTK_TREE_VIEW (editlist->clist);
         GtkTreeModel *model = gtk_tree_view_get_model (treeview);
         GtkTreeIter iter;
         gboolean success;

         success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
         if (success) {
            gtk_tree_model_get (model, &iter, i, &text, -1);
         }
      } 

      table->set_data_fn (editlist, table->widget, row, i,
                          text, table->coldata);

      g_free (text);
   }

   editlist->edit_area_flags &= ~GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED;
   editlist->edit_area_flags &= ~GIMV_ELIST_EDIT_AREA_VALUE_CHANGED;

   gimv_elist_set_sensitive (editlist);
}


static gchar **
gimv_elist_edit_area_get_data (GimvEList *editlist, GimvEListActionType type)
{
   gint i;
   gchar **text;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (editlist->columns > 0, NULL);

   text = g_new0 (gchar *, editlist->columns + 1);

   for (i = 0; i < editlist->columns; i++) {
      GimvEListColumnFuncTable *table = editlist->column_func_tables[i];

      text[i] = NULL;

      if (!table) {
         g_warning ("GimvEList: function table for column %d is not found", i);
         continue;
      }
      if (!table->get_data_fn) {
         g_warning ("GimvEList: get_data_fn method for column %d is notimplemented", i);
         continue;
      }

      text[i] = table->get_data_fn (editlist, type, table->widget, table->coldata);
   }
   text[editlist->columns] = NULL;

   return text;
}


static void
gimv_elist_edit_area_reset (GimvEList *editlist)
{
   gint i;

   for (i = 0; i < editlist->columns; i++) {
      GimvEListColumnFuncTable *table = editlist->column_func_tables[i];

      if (!table) continue;
      if (!table->reset_fn) continue;

      table->reset_fn (editlist, table->widget, table->coldata);
   }

   editlist->edit_area_flags |= GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED;
   editlist->edit_area_flags &= ~GIMV_ELIST_EDIT_AREA_VALUE_CHANGED;

   gimv_elist_set_sensitive (editlist);
}



/*******************************************************************************
 *
 *  Public functions.
 *
 *******************************************************************************/
GtkWidget *
gimv_elist_new (gint colnum)
{
   GimvEList *editlist;
   GtkWidget *main_vbox;
   GtkWidget *vbox, *vbox1, *hbox, *hbox1, *scrollwin;
   GtkWidget *clist, *button, *arrow;
   gint i;

   g_return_val_if_fail (colnum > 0, NULL);

   editlist = g_object_new (gimv_elist_get_type (), NULL);
   main_vbox = GTK_WIDGET (editlist);

   /* clist */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER (hbox), 0);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show (hbox);

   scrollwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                       GTK_SHADOW_IN);
   gtk_container_set_border_width(GTK_CONTAINER (scrollwin), 5);
   gtk_box_pack_start (GTK_BOX (hbox), scrollwin, TRUE, TRUE, 0);
   gtk_widget_set_usize (scrollwin, -1, 120);
   gtk_widget_show (scrollwin);

   clist = gimv_elist_create_list_widget (editlist, colnum);
   gtk_container_add (GTK_CONTAINER (scrollwin), clist);
   gtk_widget_show (clist);


   /* move buttons */
   vbox = editlist->move_button_area = gtk_vbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);
   gtk_widget_show (vbox);

   vbox1 = gtk_vbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), vbox1, FALSE, FALSE, 2);
   gtk_widget_show (vbox1);

   button = editlist->up_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Up"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox1), button, FALSE, FALSE, 2);
   gtk_widget_show (button);

   button = editlist->down_button = gtk_button_new ();
#ifdef USE_ARROW
   arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#else /* USE_ARROW */
   arrow = gtk_label_new (_("Down"));
   gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);
#endif /* USE_ARROW */
   gtk_container_add (GTK_CONTAINER (button), arrow);
   gtk_widget_show (arrow);
   gtk_box_pack_start (GTK_BOX (vbox1), button, FALSE, FALSE, 2);
   gtk_widget_show (button);


   /* edit area */
   editlist->edit_area = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (editlist->edit_area), 5);
   gtk_box_pack_start (GTK_BOX (main_vbox), editlist->edit_area,
                       FALSE, TRUE, 0);
   gtk_widget_show (editlist->edit_area);


   /* edit buttons */
   hbox = editlist->action_button_area = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
   gtk_widget_show (hbox);

   hbox1 = gtk_hbox_new (TRUE, 0);
   gtk_box_pack_end (GTK_BOX (hbox), hbox1, FALSE, TRUE, 0);
   gtk_widget_show (hbox1);

   button = editlist->new_button = gtk_button_new_with_label (_("New"));
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   gtk_widget_show (button);

   button = editlist->add_button = gtk_button_new_with_label (_("Add"));
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   gtk_widget_show (button);

   button = editlist->change_button = gtk_button_new_with_label (_("Change"));
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   gtk_widget_show (button);

   button = editlist->del_button = gtk_button_new_with_label (_("Delete"));
   gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, TRUE, 2);
   gtk_widget_show (button);


#ifdef USE_ARROW
   gtk_widget_set_size_request (editlist->up_button, 20, 20);
   gtk_widget_set_size_request (editlist->down_button, 20, 20);
#endif /* USE_ARROW */
   gtk_widget_set_size_request (editlist->new_button, 70, -1);

   g_signal_connect (G_OBJECT (editlist->up_button), "clicked",
                     G_CALLBACK (cb_editlist_up_button), editlist);
   g_signal_connect (G_OBJECT (editlist->down_button), "clicked",
                     G_CALLBACK (cb_editlist_down_button), editlist);
   g_signal_connect (G_OBJECT (editlist->new_button), "clicked",
                     G_CALLBACK (cb_editlist_new_button), editlist);
   g_signal_connect (G_OBJECT (editlist->add_button), "clicked",
                     G_CALLBACK (cb_editlist_add_button), editlist);
   g_signal_connect (G_OBJECT (editlist->change_button), "clicked",
                     G_CALLBACK (cb_editlist_change_button), editlist);
   g_signal_connect (G_OBJECT (editlist->del_button), "clicked",
                     G_CALLBACK (cb_editlist_delete_button), editlist);

   /* initialize column func tables */
   editlist->column_func_tables
      = g_new0 (GimvEListColumnFuncTable *, editlist->columns);
   for (i = 0; i < colnum; i++) {
      editlist->column_func_tables[i]
         = gimv_elist_column_func_table_new ();
   }

   gimv_elist_set_sensitive (editlist);

   return main_vbox;
}


GtkWidget *
gimv_elist_new_with_titles (gint colnum, gchar *titles[])
{
   GimvEList *editlist;
   gint i;
   GList *list, *node;

   editlist = GIMV_ELIST (gimv_elist_new (colnum));

   list = gtk_tree_view_get_columns (GTK_TREE_VIEW (editlist->clist));
   for (node = list, i = 0; node; node = g_list_next (node), i++) {
      GtkTreeViewColumn *col = node->data;
      gtk_tree_view_column_set_title (col, titles[i]);
   }
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (editlist->clist), TRUE);

   return GTK_WIDGET (editlist);
}


void
gimv_elist_set_column_title_visible (GimvEList *editlist, gboolean visible)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (editlist->clist), visible);
}


void
gimv_elist_set_reorderable (GimvEList *editlist, gboolean reorderble)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   gtk_tree_view_set_reorderable (GTK_TREE_VIEW (editlist->clist), reorderble);

   if (reorderble)
      gtk_widget_show (editlist->move_button_area);
   else
      gtk_widget_hide (editlist->move_button_area);
}


void
gimv_elist_set_auto_sort (GimvEList *editlist, gint column)
{
   GList *list, *node;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (column < editlist->columns);

   list = gtk_tree_view_get_columns (GTK_TREE_VIEW (editlist->clist));

   for (node = list; node; node = g_list_next (node)) {
      GtkTreeViewColumn *treecolumn = node->data;
      gtk_tree_view_column_set_reorderable (treecolumn, FALSE);
   }

   if (column >= 0) {
      GtkTreeViewColumn *treecolumn;
      node = g_list_nth (list, column);
      treecolumn = node->data;
      gtk_tree_view_column_set_reorderable (treecolumn, TRUE);
      gtk_tree_view_column_set_sort_column_id (treecolumn, column);
      gtk_tree_view_column_set_sort_order (treecolumn, GTK_SORT_ASCENDING);
   }

   g_list_free (list);
}


gboolean
gimv_elist_get_reorderable (GimvEList *editlist)
{
   g_return_val_if_fail (GIMV_IS_ELIST (editlist), FALSE);

   return gtk_tree_view_get_reorderable (GTK_TREE_VIEW (editlist->clist));
}


gint
gimv_elist_append_row (GimvEList *editlist, gchar *data[])
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkListStore *store;
   GtkTreeIter iter;
   gint i;
   gint retval;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), -1);
   g_return_val_if_fail (editlist->max_row < 0
                         || editlist->rows <= editlist->max_row, -1);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);
   store = GTK_LIST_STORE (model);
   
   gtk_list_store_append (store, &iter);

   for (i = 0; i < editlist->columns; i++) {
      gtk_list_store_set (store, &iter, i, data[i], -1);
   }
   editlist->rows = list_widget_get_row_num(editlist->clist);
   retval = editlist->rows - 1;

   gimv_elist_set_sensitive (editlist);

   return retval;
}


void
gimv_elist_remove_row (GimvEList *editlist, gint row)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkListStore *store;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (row >= 0 && row < editlist->rows);

   gimv_elist_set_row_data (editlist, row, NULL);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);
   store = GTK_LIST_STORE (model);

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   if (success) {
      gtk_list_store_remove (store, &iter);
      editlist->selected = -1;
   }

   editlist->rows = list_widget_get_row_num(editlist->clist);

   gimv_elist_updated (editlist);
   gimv_elist_set_sensitive (editlist);
}


gint
gimv_elist_get_n_rows (GimvEList *editlist)
{
   g_return_val_if_fail (GIMV_IS_ELIST (editlist), -1);

   return editlist->rows;
}


gint
gimv_elist_get_selected_row (GimvEList *editlist)
{
   g_return_val_if_fail (GIMV_IS_ELIST (editlist), -1);

   return editlist->selected;
}


gchar **
gimv_elist_get_row_text (GimvEList *editlist, gint row)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gchar **text;
   gint i;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (row >= 0 &&  row < editlist->rows, NULL);
   g_return_val_if_fail (editlist->columns > 0, NULL);

   text = g_new0 (gchar *, editlist->columns + 1);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   g_return_val_if_fail (success, NULL);

   for (i = 0; i < editlist->columns; i++) {
      text[i] = NULL;
      gtk_tree_model_get (model, &iter, i, &text[i], -1);
      if (!text[i])
         text[i] = g_strdup ("");
   }

   text[editlist->columns] = NULL;

   return text;
}


gchar *
gimv_elist_get_cell_text (GimvEList *editlist, gint row, gint col)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gchar *text = NULL;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (row >= 0 &&  row < editlist->rows, NULL);
   g_return_val_if_fail (editlist->columns > 0, NULL);
   g_return_val_if_fail (col < editlist->columns, NULL);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   g_return_val_if_fail (success, NULL);

   gtk_tree_model_get (model, &iter, col, &text, -1);

   return text;
}


void
gimv_elist_set_row_data (GimvEList *editlist,
                         gint          row,
                         gpointer      data)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (row >= 0 &&  row < editlist->rows);

   gimv_elist_set_row_data_full (editlist, row, data, NULL);
}


void
gimv_elist_set_row_data_full (GimvEList *editlist,
                              gint          row,
                              gpointer      data,
                              GtkDestroyNotify destroy_fn)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkDestroyNotify destroy;
   gboolean success;
   gpointer rowdata;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (row >= 0 &&  row < editlist->rows);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);
   rowdata = gimv_elist_get_row_data (editlist, row);

   if (rowdata) {
      destroy = g_hash_table_lookup (editlist->rowdata_destroy_fn_table,
                                     rowdata);
      if (destroy) {
         destroy (rowdata);
         g_hash_table_remove (editlist->rowdata_destroy_fn_table, rowdata);
      }

      g_hash_table_remove (editlist->rowdata_table, rowdata);
   }

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   g_return_if_fail (success);

   gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                       ROWDATA(editlist->columns),    data,
                       ROWDESTROY(editlist->columns), destroy_fn,
                       -1);

   if (data) {
      g_hash_table_insert (editlist->rowdata_table,
                           data, data);
      g_hash_table_insert (editlist->rowdata_destroy_fn_table,
                           data, destroy_fn);
   }

   gimv_elist_updated (editlist);
}


gpointer
gimv_elist_get_row_data (GimvEList *editlist,
                            gint          row)
{
   GtkTreeView *treeview;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gpointer data;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (row >= 0 &&  row < editlist->rows, NULL);

   treeview = GTK_TREE_VIEW (editlist->clist);
   model = gtk_tree_view_get_model (treeview);

   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   if (!success) return NULL;

   gtk_tree_model_get (model, &iter,
                       ROWDATA (editlist->columns), &data, -1);

   return data;
}


void
gimv_elist_unselect_all (GimvEList *editlist)
{
   GtkTreeView *treeview;
   GtkTreeSelection *selection;

   g_return_if_fail (GIMV_IS_ELIST (editlist));

   treeview = GTK_TREE_VIEW (editlist->clist);
   selection = gtk_tree_view_get_selection (treeview);

   gtk_tree_selection_unselect_all (selection);
   editlist->selected = -1;

   gimv_elist_set_sensitive (editlist);
}


void
gimv_elist_set_max_row (GimvEList *editlist, gint rownum)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   editlist->max_row = rownum;
}


void
gimv_elist_set_column_funcs (GimvEList *editlist,
                             GtkWidget *widget,
                             gint column,
                             GimvEListSetDataFn set_data_fn,
                             GimvEListGetDataFn get_data_fn,
                             GimvEListResetFn   reset_fn,
                             gpointer coldata,
                             GtkDestroyNotify destroy_fn)
{
   GimvEListColumnFuncTable *table;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (column >= 0 && column < editlist->columns);

   table = editlist->column_func_tables[column];

   table->widget      = widget;
   table->coldata     = coldata;
   table->destroy_fn  = destroy_fn;

   table->set_data_fn = set_data_fn;
   table->get_data_fn = get_data_fn;
   table->reset_fn    = reset_fn;
}


void
gimv_elist_set_get_row_data_func (GimvEList *editlist,
                                     GimvEListGetRowDataFn get_rowdata_func)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   editlist->get_rowdata_fn = get_rowdata_func;
}


void
gimv_elist_edit_area_set_value_changed (GimvEList *editlist)
{
   g_return_if_fail (GIMV_IS_ELIST (editlist));

   editlist->edit_area_flags &= ~GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED;
   editlist->edit_area_flags |= GIMV_ELIST_EDIT_AREA_VALUE_CHANGED;

   gimv_elist_set_sensitive (editlist);
}


GimvEListConfirmFlags
gimv_elist_action_confirm (GimvEList *editlist,
                           GimvEListActionType type)
{
   GimvEListConfirmFlags retval = 0;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), retval);

   if (editlist->edit_area_flags & GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED)
      retval |= GIMV_ELIST_CONFIRM_CANNOT_NEW;

   if (editlist->max_row >= 0 && editlist->rows >= editlist->max_row)
      retval |= GIMV_ELIST_CONFIRM_CANNOT_ADD;

   if (!(editlist->edit_area_flags & GIMV_ELIST_EDIT_AREA_VALUE_CHANGED))
      retval |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;

   if (editlist->selected < 0 || editlist->selected >= editlist->rows) {
      retval |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
      retval |= GIMV_ELIST_CONFIRM_CANNOT_DELETE;
   }

   gtk_signal_emit (GTK_OBJECT (editlist),
                    gimv_elist_signals[ACTION_CONFIRM_SIGNAL],
                    type,
                    editlist->selected,
                    &retval);

   return retval;
}


void
gimv_elist_set_action_button_sensitive (GimvEList *editlist)
{
   GimvEListConfirmFlags flags;

   g_return_if_fail (GIMV_IS_ELIST (editlist));

   flags = gimv_elist_action_confirm (editlist,
                                         GIMV_ELIST_ACTION_SET_SENSITIVE);

   gtk_widget_set_sensitive (editlist->new_button,
                             !(flags & GIMV_ELIST_CONFIRM_CANNOT_NEW));
   gtk_widget_set_sensitive (editlist->add_button,
                             !(flags & GIMV_ELIST_CONFIRM_CANNOT_ADD));
   gtk_widget_set_sensitive (editlist->change_button,
                             !(flags & GIMV_ELIST_CONFIRM_CANNOT_CHANGE));
   gtk_widget_set_sensitive (editlist->del_button,
                             !(flags & GIMV_ELIST_CONFIRM_CANNOT_DELETE));
}



/*******************************************************************************
 *
 *  convenient function to create entry.
 *
 *******************************************************************************/
typedef struct GimvEListEntryData_Tag
{
   GimvEList *editlist;
   GtkWidget    *entry;
   gchar        *init_string;
   gboolean      allow_empty;
} GimvEListEntryData;


static void
cb_editlist_entry_set_data (GimvEList *editlist, GtkWidget *widget,
                            gint row, gint col,
                            const gchar *text, gpointer coldata)
{
   GimvEListEntryData *entry_data = coldata;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (GTK_IS_ENTRY (widget));
   g_return_if_fail (entry_data);

   if (text)
      gtk_entry_set_text (GTK_ENTRY (widget), text);
}


static gchar *
cb_editlist_entry_get_data (GimvEList *editlist,
                            GimvEListActionType type,
                            GtkWidget *widget,
                            gpointer coldata)
{
   GimvEListEntryData *entry_data = coldata;
   const gchar *text;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (GTK_IS_ENTRY (widget), NULL);
   g_return_val_if_fail (entry_data, NULL);

   text = gtk_entry_get_text (GTK_ENTRY (widget));

   if (text)
      return g_strdup (text);
   else
      return NULL;
}


static void
cb_editlist_entry_reset (GimvEList *editlist,
                         GtkWidget *widget,
                         gpointer coldata)
{
   GimvEListEntryData *entry_data = coldata;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (GTK_IS_ENTRY (widget));
   g_return_if_fail (entry_data);

   gtk_entry_set_text (GTK_ENTRY (widget), entry_data->init_string);
}


static void
cb_editlist_entry_changed (GtkEntry *entry, gpointer data)
{
   GimvEListEntryData *entry_data = data;
   GimvEList *editlist;
   const gchar *text;

   g_return_if_fail (entry_data);
   g_return_if_fail (GIMV_IS_ELIST (entry_data->editlist));

   editlist = entry_data->editlist;

   text = gtk_entry_get_text (entry);

   gimv_elist_edit_area_set_value_changed (editlist);
}


static void
editlist_entry_destroy (gpointer data)
{
   GimvEListEntryData *entry_data = data;

   g_free (entry_data->init_string);
   g_free (entry_data);
}


static void
cb_editlist_entry_confirm (GimvEList *editlist,
                           GimvEListActionType type,
                           gint selected_row,
                           GimvEListConfirmFlags *flags,
                           gpointer data)
{
   GimvEListEntryData *entry_data = data;
   const gchar *text;

   text = gtk_entry_get_text (GTK_ENTRY (entry_data->entry));
   if (!entry_data->allow_empty && (!text || !*text)) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
   }

   return;
}


GtkWidget *
gimv_elist_create_entry (GimvEList *editlist, gint column,
                         const gchar *init_string,
                         gboolean allow_empty)
{
   GimvEListEntryData *entry_data;
   GtkWidget *entry;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (column >= 0 && column < editlist->columns, NULL);

   entry_data = g_new0 (GimvEListEntryData, 1);
   entry_data->editlist = editlist;
   if (!init_string || !*init_string)
      entry_data->init_string = g_strdup ("\0");
   else
      entry_data->init_string = g_strdup (init_string);
   entry_data->allow_empty = allow_empty;

   entry_data->entry = entry = gtk_entry_new ();

   gtk_entry_set_text (GTK_ENTRY (entry_data->entry), entry_data->init_string);

   gimv_elist_set_column_funcs (editlist,
                                entry, column,
                                cb_editlist_entry_set_data,
                                cb_editlist_entry_get_data,
                                cb_editlist_entry_reset,
                                entry_data,
                                editlist_entry_destroy);

   g_signal_connect (G_OBJECT (editlist), "action_confirm",
                     G_CALLBACK (cb_editlist_entry_confirm),
                     entry_data);
   g_signal_connect (G_OBJECT (entry),"changed",
                     G_CALLBACK (cb_editlist_entry_changed),
                     entry_data);

   gimv_elist_set_sensitive (editlist);

   return entry;
}



/*******************************************************************************
 *
 *  convenient function to create check button.
 *
 *******************************************************************************/
typedef struct GimvEListCheckButtonData_Tag
{
   GimvEList *editlist;
   gboolean   init_value;
   gchar     *true_string;
   gchar     *false_string;
} GimvEListCheckButtonData;

static void
cb_editlist_check_button_set_data (GimvEList *editlist, GtkWidget *widget,
                                   gint row, gint col,
                                   const gchar *text, gpointer coldata)
{
   GimvEListCheckButtonData *button_data = coldata;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (GTK_IS_CHECK_BUTTON (widget));
   g_return_if_fail (button_data);

   if (row < 0) return;

   if (text && *text && !strcmp (text, button_data->true_string)) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
   } else {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
   }
}


static gchar *
cb_editlist_check_button_get_data (GimvEList *editlist,
                                   GimvEListActionType type,
                                   GtkWidget *widget,
                                   gpointer coldata)
{
   GimvEListCheckButtonData *button_data = coldata;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (GTK_IS_CHECK_BUTTON (widget), NULL);
   g_return_val_if_fail (button_data, NULL);

   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
      return g_strdup (button_data->true_string);
   else
      return g_strdup (button_data->false_string);
}


static void
cb_editlist_check_button_reset (GimvEList *editlist,
                                GtkWidget *widget,
                                gpointer coldata)
{
   GimvEListCheckButtonData *button_data = coldata;

   g_return_if_fail (GIMV_IS_ELIST (editlist));
   g_return_if_fail (GTK_IS_CHECK_BUTTON (widget));

   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                 button_data->init_value);
}


static void
cb_editlist_check_button_toggled (GtkCheckButton *check_button, gpointer data)
{
   GimvEListCheckButtonData *button_data = data;
   gimv_elist_edit_area_set_value_changed (button_data->editlist);
}


static void
editlist_check_button_destroy (gpointer data)
{
   GimvEListCheckButtonData *button_data = data;

   g_free (button_data->true_string);
   g_free (button_data->false_string);
   g_free (button_data);
}


GtkWidget *
gimv_elist_create_check_button (GimvEList *editlist, gint column,
                                const gchar *label,
                                gboolean init_value,
                                const gchar *true_string,
                                const gchar *false_string)
{
   GimvEListCheckButtonData *button_data;
   GtkWidget *check_button;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);
   g_return_val_if_fail (column >= 0 && column < editlist->columns, NULL);
   g_return_val_if_fail (true_string && *true_string, NULL);
   g_return_val_if_fail (false_string && *false_string, NULL);

   button_data = g_new0 (GimvEListCheckButtonData, 1);
   button_data->editlist     = editlist;
   button_data->init_value   = init_value;
   button_data->true_string  = g_strdup (true_string);
   button_data->false_string = g_strdup (false_string);

   if (label)
      check_button = gtk_check_button_new_with_label (label);
   else
      check_button = gtk_check_button_new ();

   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                 button_data->init_value);

   gimv_elist_set_column_funcs (GIMV_ELIST (editlist),
                                   check_button, column,
                                   cb_editlist_check_button_set_data,
                                   cb_editlist_check_button_get_data,
                                   cb_editlist_check_button_reset,
                                   button_data,
                                   editlist_check_button_destroy);

   g_signal_connect (G_OBJECT (check_button),"toggled",
                     G_CALLBACK (cb_editlist_check_button_toggled),
                     button_data);

   gimv_elist_set_sensitive (editlist);

   return check_button;
}
