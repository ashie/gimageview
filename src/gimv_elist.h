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


#ifndef __GIMV_ELIST_H__
#define __GIMV_ELIST_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>

#define GIMV_TYPE_ELIST            (gimv_elist_get_type ())
#define GIMV_ELIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_ELIST, GimvEList))
#define GIMV_ELIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_ELIST, GimvEListClass))
#define GIMV_IS_ELIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_ELIST))
#define GIMV_IS_ELIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_ELIST))
#define GIMV_ELIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_ELIST, GimvEListClass))


typedef struct GimvEList_Tag      GimvEList;
typedef struct GimvEListClass_Tag GimvEListClass;

typedef struct GimvEListColumnFuncTable_Tag GimvEListColumnFuncTable;


typedef enum {
   GIMV_ELIST_EDIT_AREA_VALUE_INITIALIZED = 1 << 0,
   GIMV_ELIST_EDIT_AREA_VALUE_CHANGED     = 1 << 1,
} GimvEListEditAreaFlags;


typedef enum {
   GIMV_ELIST_CONFIRM_CANNOT_NEW    = 1 << 0,
   GIMV_ELIST_CONFIRM_CANNOT_ADD    = 1 << 1,
   GIMV_ELIST_CONFIRM_CANNOT_CHANGE = 1 << 2,
   GIMV_ELIST_CONFIRM_CANNOT_DELETE = 1 << 3
} GimvEListConfirmFlags;


typedef enum {
   GIMV_ELIST_ACTION_RESET,
   GIMV_ELIST_ACTION_ADD,
   GIMV_ELIST_ACTION_CHANGE,
   GIMV_ELIST_ACTION_DELETE,
   GIMV_ELIST_ACTION_SET_SENSITIVE,
} GimvEListActionType;


typedef void      (*GimvEListSetDataFn)      (GimvEList *editlist,
                                              GtkWidget    *widget,
                                              gint          row, /* -1 if no item is selected */
                                              gint          col,
                                              const gchar  *text,/* NULL if no item is selected */
                                              gpointer      coldata);
typedef gchar    *(*GimvEListGetDataFn)      (GimvEList *editlist,
                                              GimvEListActionType type,
                                              GtkWidget    *widget,
                                              gpointer      coldata);
typedef void      (*GimvEListResetFn)        (GimvEList *editlist,
                                              GtkWidget    *widget,
                                              gpointer      coldata);
typedef gboolean  (*GimvEListGetRowDataFn)   (GimvEList *editlist,
                                              GimvEListActionType type,
                                              gpointer     *rowdata,
                                              GtkDestroyNotify *destroy_fn);


struct GimvEList_Tag {
   GtkVBox      parent;

   /* public (read only) */
   GtkWidget   *clist;

   GtkWidget   *new_button;
   GtkWidget   *add_button;
   GtkWidget   *change_button;
   GtkWidget   *del_button;
   GtkWidget   *up_button;
   GtkWidget   *down_button;

   GtkWidget   *edit_area;
   GtkWidget   *move_button_area;
   GtkWidget   *action_button_area;

   /* private */
   gint         max_row;
   gint         columns;
   gint         rows;
   gint         selected;
   gint         dest_row;

   GimvEListEditAreaFlags     edit_area_flags;
   GimvEListColumnFuncTable **column_func_tables;
   GimvEListGetRowDataFn      get_rowdata_fn;

#if (GTK_MAJOR_VERSION >= 2)
   GHashTable *rowdata_table;
   GHashTable *rowdata_destroy_fn_table;
#endif /* (GTK_MAJOR_VERSION >= 2) */
};


struct GimvEListClass_Tag {
   GtkVBoxClass parent_class;

   void     (*list_updated)       (GimvEList *editlist);
   void     (*edit_area_set_data) (GimvEList *editlist);
   void     (*action_confirm)     (GimvEList *editlist,
                                   GimvEListActionType    action_type,
                                   gint                      selected_row,
                                   GimvEListConfirmFlags *flags);
};


struct GimvEListColumnFuncTable_Tag {
   GtkWidget        *widget;
   gpointer          coldata;
   GtkDestroyNotify  destroy_fn;

   /* will be called when an item is selected (or unselected) */
   GimvEListSetDataFn        set_data_fn;

   /* will be called when an action button is pressed */
   GimvEListGetDataFn        get_data_fn;
   GimvEListResetFn          reset_fn;
};


GType         gimv_elist_get_type              (void);
GtkWidget    *gimv_elist_new                   (gint          colnum);
GtkWidget    *gimv_elist_new_with_titles       (gint          colnum,
                                                gchar        *titles[]);
void          gimv_elist_set_column_title_visible
                                               (GimvEList *editlist,
                                                gboolean      visible);
void          gimv_elist_set_reorderable       (GimvEList *editlist,
                                                gboolean      reorderble);
gboolean      gimv_elist_get_reorderable       (GimvEList *editlist);
void          gimv_elist_set_auto_sort         (GimvEList *editlist,
                                                gint          column);
gint          gimv_elist_append_row            (GimvEList *editlist,
                                                gchar        *data[]);
void          gimv_elist_remove_row            (GimvEList *editlist,
                                                gint          row);
gint          gimv_elist_get_n_rows            (GimvEList *editlist);
gint          gimv_elist_get_selected_row      (GimvEList *editlist);
gchar       **gimv_elist_get_row_text          (GimvEList *editlist,
                                                gint          row);
gchar        *gimv_elist_get_cell_text         (GimvEList *ditlist,
                                                gint          row,
                                                gint          col);
#if 0
void          gimv_elist_set_row_text          (GimvEList *ditlist,
                                                gint          row,
                                                gchar        *data[]);
void          gimv_elist_set_cell_text         (GimvEList *ditlist,
                                                gint          row,
                                                gint          col,
                                                gchar        *data);
#endif
void          gimv_elist_set_row_data          (GimvEList *editlist,
                                                gint          row,
                                                gpointer      data);
void          gimv_elist_set_row_data_full     (GimvEList *editlist,
                                                gint          row,
                                                gpointer      data,
                                                GtkDestroyNotify destroy_fn);
gpointer      gimv_elist_get_row_data          (GimvEList *editlist,
                                                gint          row);
void          gimv_elist_unselect_all          (GimvEList *editlist);
void          gimv_elist_set_max_row           (GimvEList *editlist,
                                                gint          rownum);
void          gimv_elist_set_column_funcs      (GimvEList *editlist,
                                                GtkWidget    *widget,
                                                gint          column,
                                                GimvEListSetDataFn set_data_fn,
                                                GimvEListGetDataFn get_data_fn,
                                                GimvEListResetFn   reset_fn,
                                                gpointer      coldata,
                                                GtkDestroyNotify destroy_fn);
void          gimv_elist_set_get_row_data_func (GimvEList *editlist,
                                                GimvEListGetRowDataFn get_rowdata_func);
void          gimv_elist_edit_area_set_value_changed
                                               (GimvEList *editlist);
GimvEListConfirmFlags gimv_elist_action_confirm
                                               (GimvEList *editlist,
                                                GimvEListActionType type);
void          gimv_elist_set_action_button_sensitive
                                               (GimvEList *editlist);


/* entry */
GtkWidget    *gimv_elist_create_entry          (GimvEList *editlist,
                                                gint          column,
                                                const gchar  *init_string,
                                                gboolean      allow_empty);

/* file name entry */
/*
GtkWidget    *gimv_elist_create_file_entry     (GimvEList *editlist,
                                                gint          column,
                                                const gchar  *init_string,
                                                gboolean      allow_empty)
*/

/* combo */
/*
GtkWidget    *gimv_elist_create_combo          (GimvEList *editlist,
                                                gint column);
*/

/* check button */
GtkWidget    *gimv_elist_create_check_button   (GimvEList *editlist,
                                                gint          column,
                                                const gchar  *label,
                                                gboolean      init_value,
                                                const gchar  *true_string,
                                                const gchar  *false_string);

/* spinner */
/*
GtkWidget    *gimv_elist_create_spinner        (GimvEList *editlist,
                                                gint column,
                                                gfloat init_val,
                                                gflost min, gloat max,
                                                gboolean set_as_integer);
*/

#endif /* __GIMV_ELIST_H__ */
