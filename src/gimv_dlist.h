/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * gimv_dlist.c - pair of reorderable stack list widget.
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

#ifndef __GIMV_DLIST_H__
#define __GIMV_DLIST_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>

#define GIMV_DLIST(obj)         GTK_CHECK_CAST (obj, gimv_dlist_get_type (), GimvDList)
#define GIMV_DLIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimv_dlist_get_type, GimvDListClass)
#define GIMV_IS_DLIST(obj)      GTK_CHECK_TYPE (obj, gimv_dlist_get_type ())


typedef struct GimvDList_Tag      GimvDList;
typedef struct GimvDListClass_Tag GimvDListClass;


struct GimvDList_Tag {
   GtkHBox     parent;

   /* public (read only) */
   GtkWidget   *clist1;
   GtkWidget   *clist2;
   GtkWidget   *add_button;
   GtkWidget   *del_button;
   GtkWidget   *up_button;
   GtkWidget   *down_button;

   /* private */
   gint         clist1_rows;
   gint         clist1_selected;
   gint         clist1_dest_row;
   gint         clist2_rows;
   gint         clist2_selected;
   gint         clist2_dest_row;
   GList       *available_list;
};


struct GimvDListClass_Tag {
   GtkHBoxClass parent_class;

#if 0
   gboolean (*available_list_updated) (GimvDList *dslist); /* need?*/
#endif
   gboolean (*enabled_list_updated)   (GimvDList *dslist);
};


/*
 *  "available" or "clist1" means left side CList (or TreeView).
 *  "enabled" or "clist2" means right side CList (or TreeView).
 */

GtkType       gimv_dlist_get_type              (void);

GtkWidget    *gimv_dlist_new                   (const gchar  *clist1_title,
                                                const gchar  *clist2_title);

/* FIXME: item should be untranslated text.
   Currently, GimvDList translates item text by itself. */
gint          gimv_dlist_append_available_item (GimvDList   *dslist,
                                                const gchar  *item);
void          gimv_dlist_column_add            (GimvDList  *dslist,
                                                gint         idx);
void          gimv_dlist_column_del            (GimvDList  *dslist,
                                                gint         idx);
void          gimv_dlist_column_add_by_label   (GimvDList   *dslist,
                                                const gchar  *label);
gint          gimv_dlist_get_n_available_items (GimvDList  *dslist);
gint          gimv_dlist_get_n_enabled_items   (GimvDList  *dslist);

/* FIXME: currently, this function returns the untranslated text */
gchar        *gimv_dlist_get_enabled_row_text  (GimvDList   *dslist,
                                                gint          row);


#if 0   /* API draft */

void          gimv_dlist_up                    (GimvDList  *dslist,
                                                gint         idx);
void          gimv_dlist_down                  (GimvDList  *dslist,
                                                gint         idx);
gboolean      gimv_dlist_swap_idx              (GimvDList  *dslist,
                                                gint         src_idx,
                                                gint         dest_idx);

void          gimv_dlist_clist1_set_reorderble (GimvDList  *dslist,
                                                gboolean     reorderble);

void          gimv_dlist_get_idx_by_label      (GimvDList  *dslist,
                                                const gchar *label);
void          gimv_dlist_get_idx_by_data       (GimvDList  *dslist,
                                                gpointer    *data);
gboolean      gimv_dlist_idx_is_enabled        (GimvDList  *dslist,
                                                gint         idx);
const gchar  *gimv_dlist_get_label             (GimvDList  *dslist,
                                                gint         idx);
gpointer      gimv_dlist_get_data              (GimvDList  *dslist,
                                                gint         idx);

/* returned list should be free, but its content data shouldn't be free. */
GList        *gimv_dlist_get_available_list    (GimvDList *dslist);
GList        *gimv_dlist_get_enabled_list      (GimvDList *dslist);

void          gimv_dlist_set_translate_func    (GimvDList   *gimv_dlist,
                                                nanndakke?);
#endif

#endif /* __GIMV_DLIST_H__ */
