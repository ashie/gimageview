/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2004 Takuro Ashie
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

#include "detailview.h"

/*
 *   For Gtk+-2.0
 */

#ifdef ENABLE_TREEVIEW

#include <time.h>
#include <string.h>

#include "charset.h"
#include "dnd.h"
#include "detailview_prefs.h"
#include "detailview_priv.h"
#include "fileutil.h"
#include "gimv_cell_pixmap.h"
#include "gimv_image.h"
#include "gimv_thumb.h"
#include "gimv_thumb_win.h"
#include "gtk2-compat.h"


enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_THUMB_DATA,
   COLUMN_PIXMAP,
   COLUMN_MASK,
   COLUMN_EDITABLE,
   N_COLUMN
};


typedef struct DetailViewData_Tag
{
   GtkWidget *treeview;
   gboolean   dragging;
   gint       press_button;
   gint       press_mask;
   gint       press_x, press_y;
   gboolean   set_selection;
} DetailViewData;


/* callback functions */
static gboolean cb_tree_selected               (GtkTreeSelection  *selection,
                                                GtkTreeModel      *model,
                                                GtkTreePath       *treepath,
                                                gboolean           selected,
                                                gpointer           data);
static gboolean cb_treeview_button_press       (GtkWidget         *widget,
                                                GdkEventButton    *event,
                                                GimvThumbView     *tv);
static gboolean cb_treeview_motion_notify      (GtkWidget         *widget,
                                                GdkEventMotion    *event,
                                                gpointer           data);
static gboolean cb_treeview_button_release     (GtkWidget         *widget,
                                                GdkEventButton    *event,
                                                GimvThumbView     *tv);
static void     cb_treeview_drag_data_received (GtkWidget         *widget,
                                                GdkDragContext    *context,
                                                gint               x,
                                                gint               y,
                                                GtkSelectionData  *seldata,
                                                guint              info,
                                                guint              time,
                                                gpointer           data);
static void     cb_column_clicked              (GtkTreeViewColumn *column,
                                                GimvThumbView     *tv);

/* other private function */
static DetailViewData *detailview_new          (GimvThumbView  *tv);
static void            set_column_types        (GimvThumbView  *tv,
                                                DetailViewData *tv_data,
                                                GtkListStore   *store,
                                                const gchar    *dest_mode);
static void            detailview_set_pixmaps  (GimvThumbView  *tv,
                                                const gchar    *dest_mode);



/******************************************************************************
 *
 *   Callback functions.
 *
 ******************************************************************************/
static gboolean
cb_tree_selected (GtkTreeSelection *selection, GtkTreeModel *model,
                  GtkTreePath *treepath, gboolean selected, gpointer data)
{
   DetailViewData *tv_data;
   GimvThumbView *tv = data;
   GimvThumb *thumb = NULL;
   GtkTreeIter iter;
   gint sel_row_pos = 0;
   gboolean success;
   gboolean under_cursor = FALSE, under_cursor_is_selected = FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   if (tv_data->press_button <= 0 && !tv_data->set_selection)
      return FALSE;

   /* get state of row under mouse cursor */
   if (!tv_data->dragging && GTK_WIDGET_MAPPED (tv_data->treeview)) {
      GtkTreePath *sel_treepath;
      success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                               tv_data->press_x, tv_data->press_y,
                                               &sel_treepath, NULL, NULL, NULL);
      if (success) {
         sel_row_pos = gtk_tree_path_compare (sel_treepath, treepath);
         if (!sel_row_pos)
            under_cursor = TRUE;

         if (gtk_tree_selection_path_is_selected (selection, sel_treepath))
            under_cursor_is_selected = TRUE;

         gtk_tree_path_free (sel_treepath);
      }
   }

   /*
    *  For DnD and popup, should not change selection when selected path was
    *  pressed.
    *  Selection should be changed when mouse button was released.
    *  F*ck'in GTK!
    */
   if (tv_data->press_button > 0
       && under_cursor_is_selected
       && !(tv_data->press_mask & GDK_CONTROL_MASK)
       && !(tv_data->press_mask & GDK_SHIFT_MASK))
   {
      return FALSE;
   }

   success = gtk_tree_model_get_iter (model, &iter, treepath);
   g_return_val_if_fail (success, FALSE);
   gtk_tree_model_get (model, &iter,
                       COLUMN_THUMB_DATA, &thumb,
                       COLUMN_TERMINATOR);

   if (thumb) {
      thumb->selected = !selected;
   }

   tv_data->set_selection = FALSE;

   return TRUE;
}


static gint
cb_treeview_key_press (GtkWidget *widget,
                       GdkEventKey *event,
                       GimvThumbView *tv)
{
   DetailViewData *tv_data;
   GimvThumb *thumb = NULL;
   GtkTreePath *treepath;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   gtk_tree_view_get_cursor (GTK_TREE_VIEW (tv_data->treeview),
                             &treepath, NULL);

   if (treepath) {
      GtkTreeModel *model;
      GtkTreeIter iter;

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
      if (gtk_tree_model_get_iter (model, &iter, treepath)) {
         gtk_tree_model_get (model, &iter,
                             COLUMN_THUMB_DATA, &thumb,
                             COLUMN_TERMINATOR);
      }
      gtk_tree_path_free (treepath);
   }

   if (gimv_thumb_view_thumb_key_press_cb(widget, event, thumb))
      return FALSE;

   {
      switch (event->keyval) {
      case GDK_Left:
      case GDK_Right:
      case GDK_Up:
      case GDK_Down:
         return FALSE;
         break;
      case GDK_Return:
         if (!thumb) break;
         if (event->state & GDK_SHIFT_MASK || event->state & GDK_CONTROL_MASK) {
            /* is there somteing to do? */
         } else {
            gimv_thumb_view_set_selection_all (tv, FALSE);
         }
         gimv_thumb_view_set_selection (thumb, TRUE);
         gimv_thumb_view_open_image (tv, thumb, 0);
         break;
      case GDK_space:
         if (!thumb) break;
         gimv_thumb_view_set_selection (thumb, !thumb->selected);
         break;
      case GDK_Delete:
         gimv_thumb_view_delete_files (tv);
         break;
      default:
         break;
      }
   }

   return FALSE;
}


static gboolean
cb_treeview_button_press (GtkWidget *widget,
                          GdkEventButton *event,
                          GimvThumbView *tv)
{
   DetailViewData *tv_data;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GtkTreePath *treepath;
   GtkTreeViewColumn *column;
   GimvThumb *thumb = NULL;
   gboolean success, retval1 = FALSE, retval2 = FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   tv_data->dragging     = FALSE;
   tv_data->press_button = event->button;
   tv_data->press_mask   = event->state;
   tv_data->press_x      = event->x;
   tv_data->press_y      = event->y;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv_data->treeview));

   /* get state of row under mouse cursor */
   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                            event->x, event->y,
                                            &treepath, &column, NULL, NULL);
   if (success) {
      retval1 = gtk_tree_selection_path_is_selected (selection, treepath);
      gtk_tree_model_get_iter (model, &iter, treepath);
      gtk_tree_model_get (model, &iter,
                          COLUMN_THUMB_DATA, &thumb,
                          COLUMN_TERMINATOR);
   }

   if (thumb) {
      retval2 = gimv_thumb_view_thumb_button_press_cb (widget, event, thumb);
   }

   if (treepath)
      gtk_tree_path_free (treepath);

   /*
   if (retval1) {
      return TRUE;
   } else {
      return retval2;
   }
   */
   return retval2;
}


static gboolean
cb_treeview_motion_notify (GtkWidget *widget,
                           GdkEventMotion *event,
                           gpointer data)
{
   DetailViewData *tv_data;
   GimvThumbView *tv = data;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreePath *treepath;
   GtkTreeViewColumn *column;
   GimvThumb *thumb = NULL;
   gboolean success;
   gint dx, dy;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   if (tv_data->dragging) return FALSE;

   dx = tv_data->press_x - event->x;
   dy = tv_data->press_y - event->y;
   if (abs (dx) > 2 || abs (dy) > 2)
      tv_data->dragging = TRUE;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv_data->treeview));

   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                            event->x, event->y,
                                            &treepath, &column, NULL, NULL);
   if (success) {
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model, &iter, treepath);
      gtk_tree_model_get (model, &iter,
                          COLUMN_THUMB_DATA, &thumb,
                          COLUMN_TERMINATOR);
   }

   if (treepath)
      gtk_tree_path_free (treepath);

   return gimv_thumb_view_motion_notify_cb (widget, event, thumb);
}


static gboolean
cb_treeview_button_release (GtkWidget *widget,
                            GdkEventButton *event,
                            GimvThumbView *tv)
{
   DetailViewData *tv_data;
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   GtkTreePath *treepath;
   GtkTreeViewColumn *column;
   GimvThumb *thumb = NULL;
   gboolean success, retval1 = FALSE, retval2 = FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   tv_data->press_button = -1;
   tv_data->press_mask   = -1;
   tv_data->press_x      = -1;
   tv_data->press_y      = -1;

   if (tv_data->dragging) {
      tv_data->dragging = FALSE;
      return FALSE;
   }

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv_data->treeview));

   /* get state of row under mouse cursor */
   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                            event->x, event->y,
                                            &treepath, &column, NULL, NULL);
   if (success) {
      retval1 = gtk_tree_selection_path_is_selected (selection, treepath);
      gtk_tree_model_get_iter (model, &iter, treepath);
      gtk_tree_model_get (model, &iter,
                          COLUMN_THUMB_DATA, &thumb,
                          COLUMN_TERMINATOR);

      if (event->type == GDK_BUTTON_RELEASE
          && (event->button == 1)
          && !(event->state & GDK_SHIFT_MASK)
          && !(event->state & GDK_CONTROL_MASK)
          && !tv_data->dragging)
      {
         gimv_thumb_view_set_selection_all (tv, FALSE);
         gimv_thumb_view_set_selection (thumb, TRUE);
      }
   }

   if (thumb) {
      retval2 = gimv_thumb_view_thumb_button_release_cb (widget, event, thumb);
   }

   if (treepath)
      gtk_tree_path_free (treepath);

   /*
   if (retval1) {
      return TRUE;
   } else {
      return retval2;
   }
   */
   return retval2;
}


static gboolean
cb_treeview_scroll (GtkWidget *widget, GdkEventScroll *se, GimvThumbView *tv)
{
   GdkEventButton be;
   gboolean retval = FALSE;

   g_return_val_if_fail (GTK_IS_WIDGET(widget), FALSE);

   be.type       = GDK_BUTTON_PRESS;
   be.window     = se->window;
   be.send_event = se->send_event;
   be.time       = se->time;
   be.x          = se->x;
   be.y          = se->y;
   be.axes       = NULL;
   be.state      = se->state;
   be.device     = se->device;
   be.x_root     = se->x_root;
   be.y_root     = se->y_root;
   switch ((se)->direction) {
   case GDK_SCROLL_UP:
      be.button = 4;
      break;
   case GDK_SCROLL_DOWN:
      be.button = 5;
      break;
   case GDK_SCROLL_LEFT:
      be.button = 6;
      break;
   case GDK_SCROLL_RIGHT:
      be.button = 7;
      break;
   default:
      g_warning ("invalid scroll direction!");
      be.button = 0;
      break;
   }

   retval = cb_treeview_button_press (widget, &be, tv);

   be.type = GDK_BUTTON_RELEASE;
   retval = cb_treeview_button_release (widget, &be, tv);

   return retval;
}


static void
cb_treeview_drag_data_received (GtkWidget *widget,
                                GdkDragContext *context,
                                gint x, gint y,
                                GtkSelectionData *seldata,
                                guint info,
                                guint time,
                                gpointer data)
{
   gimv_thumb_view_drag_data_received_cb (widget, context, x, y,
                                          seldata, info, time, data);

   /* F*ck! */
   g_signal_stop_emission_by_name (G_OBJECT (widget), "drag_data_received");
}


static void
cb_column_clicked (GtkTreeViewColumn *column, GimvThumbView *tv)
{
   GimvThumbWin *tw;
   DetailViewData *tv_data;
   GList *col_list, *node;
   gint col, idx;
   GimvSortItem sort_item, current_item;
   GimvSortFlag sort_flag = 0, current_flag;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (tv->progress) return;

   tw = tv->tw;
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->treeview);

   col_list = gtk_tree_view_get_columns (GTK_TREE_VIEW (tv_data->treeview));
   col = g_list_index (col_list, column);

   node = g_list_nth (detailview_title_idx_list, col - 1);
   if (!node) return;

   idx = GPOINTER_TO_INT (node->data);

   switch (idx) {
   case 1:
      sort_item = GIMV_SORT_NAME;
      break;
   case 2:
      sort_item = GIMV_SORT_SIZE;
      break;
   case 3:
      sort_item = GIMV_SORT_TYPE;
      break;
   case 5:
      sort_item = GIMV_SORT_ATIME;
      break;
   case 6:
      sort_item = GIMV_SORT_MTIME;
      break;
   case 7:
      sort_item = GIMV_SORT_CTIME;
      break;
   default:
      return;
   }

   current_item = gimv_thumb_win_get_sort_type (tw, &current_flag);

   if (current_item == sort_item) {
      if (current_flag & GIMV_SORT_REVERSE)
         sort_flag &= ~GIMV_SORT_REVERSE;
      else
         sort_flag |= GIMV_SORT_REVERSE;
   }

   gimv_thumb_win_sort_thumbnail (tw, sort_item, sort_flag,
                                  GIMV_THUMB_WIN_CURRENT_PAGE);
}


/******************************************************************************
 *
 *   private functions.
 *
 ******************************************************************************/
static DetailViewData *
detailview_new (GimvThumbView *tv)
{
   DetailViewData *tv_data;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) {
      tv_data = g_new0 (DetailViewData, 1);
      tv_data->treeview      = NULL;
      tv_data->dragging      = FALSE;
      tv_data->press_button  = -1;
      tv_data->press_mask    = -1;
      tv_data->press_x       = -1;
      tv_data->press_y       = -1;
      tv_data->set_selection = FALSE;
      g_object_set_data_full (G_OBJECT (tv), DETAIL_VIEW_LABEL, tv_data,
                              (GDestroyNotify) g_free);
   }

   return tv_data;
}


static void
set_column_types (GimvThumbView *tv, DetailViewData *tv_data,
                  GtkListStore *store, const gchar *dest_mode)
{
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;
   GList *node;
   gint i, xpad, ypad;

   /* pixmap column */
   col = gtk_tree_view_column_new();
   gtk_tree_view_column_set_title (col, "");
   /* gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED); */
   /* gtk_tree_view_column_set_resizable(col, TRUE); */

   render = gimv_cell_renderer_pixmap_new ();
   xpad = GTK_CELL_RENDERER (render)->xpad;
   ypad = GTK_CELL_RENDERER (render)->xpad;

   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gint xsize = ICON_SIZE + xpad * 2, ysize = ICON_SIZE + ypad * 2;
      gtk_cell_renderer_set_fixed_size (render, xsize, ysize);
   }

   if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gint xsize = tv->thumb_size + xpad * 2;
      gint ysize = tv->thumb_size + ypad * 2;
      gtk_cell_renderer_set_fixed_size (render, xsize, ysize);
   }

   gtk_tree_view_column_pack_start (col, render, FALSE);
   gtk_tree_view_column_add_attribute (col, render,
                                       "pixmap", COLUMN_PIXMAP);
   gtk_tree_view_column_add_attribute (col, render,
                                       "mask", COLUMN_MASK);
   
   gtk_tree_view_append_column (GTK_TREE_VIEW (tv_data->treeview), col);


   /* other column */
   for (node = detailview_title_idx_list, i = N_COLUMN;
        node;
        node = g_list_next (node), i++)
   {
      gint idx = GPOINTER_TO_INT (node->data);

      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title (col, _(detailview_columns[idx].title));
      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width (col, detailview_columns[idx].width);
      gtk_tree_view_column_set_resizable(col, TRUE);

      render = gtk_cell_renderer_text_new ();
      switch (detailview_columns[idx].justification) {
      case GTK_JUSTIFY_CENTER:
         g_object_set(G_OBJECT(render), "xalign", 0.5, NULL);
         gtk_tree_view_column_set_alignment(col, 0.5);
         break;
      case GTK_JUSTIFY_RIGHT:
         g_object_set(G_OBJECT(render), "xalign", 1.0, NULL);
         gtk_tree_view_column_set_alignment(col, 0.5);
         break;
      case GTK_JUSTIFY_LEFT:
         g_object_set(G_OBJECT(render), "xalign", 0.0, NULL);
         break;
      default:
         break;
      }
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render,
                                          "text", i);
      if (!strcmp (detailview_columns[idx].title, "Name")) {
         /*
         gtk_tree_view_column_add_attribute (col, render,
                                             "editable", COLUMN_EDITABLE);
         */
      }

      gtk_tree_view_append_column (GTK_TREE_VIEW (tv_data->treeview), col);

      gtk_tree_view_column_set_clickable (col, TRUE);
      g_signal_connect (G_OBJECT (col), "clicked",
                        G_CALLBACK (cb_column_clicked), tv);
   }
}


static void
detailview_set_pixmaps (GimvThumbView *tv, const gchar *dest_mode)
{
   GimvThumb *thumb;
   GList *node;
   gint i, num;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = g_list_first (tv->thumblist);
   num = g_list_length (node);

   /* set images */
   for (i = 0 ; i < num ; i++) {
      thumb = node->data;
      detailview_update_thumbnail (tv, thumb, dest_mode);
      node = g_list_next (node);
   }
}



/******************************************************************************
 *
 *   public functions.
 *
 ******************************************************************************/
void
detailview_freeze (GimvThumbView *tv)
{
}


void
detailview_thaw (GimvThumbView *tv)
{
}


void
detailview_append_thumb_frame (GimvThumbView *tv, GimvThumb *thumb,
                               const gchar *dest_mode)
{
   DetailViewData *tv_data;
   GtkTreeModel *model;
   GList *data_node;
   gint j, pos, colnum;
   GtkTreeIter iter;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   pos = g_list_index (tv->thumblist, thumb);
   colnum = detailview_title_idx_list_num + N_COLUMN;

   tv_data = g_object_get_data (G_OBJECT(tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->treeview);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));

   gtk_list_store_insert (GTK_LIST_STORE (model), &iter, pos);
   gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                       COLUMN_THUMB_DATA, thumb,
                       COLUMN_TERMINATOR);

   gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                       COLUMN_EDITABLE, TRUE,
                       COLUMN_TERMINATOR);

   data_node = detailview_title_idx_list;
   for (j = N_COLUMN;
        j < colnum && data_node;
        j++, data_node = g_list_next (data_node))
   {
      gint idx = GPOINTER_TO_INT (data_node->data);
      gchar *text = NULL;

      if (detailview_columns[idx].func)
         text = detailview_columns[idx].func (thumb);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          j, text,
                          COLUMN_TERMINATOR);

      if (detailview_columns[idx].free && text)
         g_free (text);
   }

   detailview_set_selection (tv, thumb, thumb->selected);
}


void
detailview_update_thumbnail (GimvThumbView  *tv, GimvThumb *thumb,
                             const gchar *dest_mode)
{
   DetailViewData *tv_data;
   GdkPixmap *pixmap = NULL;
   GdkBitmap *mask;
   GList *node;
   gint pos, row, col, i;
   GtkTreeModel *model;
   GtkTreeIter iter;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->treeview);

   node = g_list_find (tv->thumblist, thumb);
   pos = g_list_position (tv->thumblist, node);

   row = pos;
   col = 0;

   /* set thumbnail */
   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gimv_thumb_get_icon (thumb, &pixmap, &mask);
   } else if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   }

   if (!pixmap) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
   gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
   gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                       COLUMN_PIXMAP,   pixmap,
                       COLUMN_MASK,     mask,
                       COLUMN_TERMINATOR);

   /* reset column data */
   node = detailview_title_idx_list;
   for (i = N_COLUMN, node = detailview_title_idx_list;
        node;
        i++, node = g_list_next (node))
   {
      gint idx = GPOINTER_TO_INT (node->data);
      gchar *str;

      if (!detailview_columns[idx].need_sync) continue;

      str = detailview_columns[idx].func (thumb);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          i, str,
                          COLUMN_TERMINATOR);

      if (detailview_columns[idx].free)
         g_free (str);
   }

   return;
}


GList *
detailview_get_load_list (GimvThumbView *tv)
{
   GList *loadlist = NULL, *node;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   if (!strcmp (DETAIL_VIEW_LABEL, tv->summary_mode)) return NULL;

   for (node = tv->thumblist; node; node = g_list_next (node)) {
      GimvThumb *thumb = node->data;
      GdkPixmap *pixmap = NULL;
      GdkBitmap *mask = NULL;

      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
      if (!pixmap)
         loadlist = g_list_append (loadlist, thumb);
   }

   return loadlist;
}


void
detailview_remove_thumbnail (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data);

   pos = g_list_index (tv->thumblist, thumb);
   if (pos < 0) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));

   if (gtk_tree_model_iter_nth_child (model, &iter, NULL, pos))
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}


void
detailview_adjust (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   GList *node;
   gint pos;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = g_list_find (gimv_thumb_view_get_list(), tv);
   if (!node) return;

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data);

   pos = g_list_index (tv->thumblist, thumb);
   if (pos < 0) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));
   success = gtk_tree_model_iter_nth_child (model, &iter, NULL, pos);
   if (success) {
      GtkTreePath *treepath;
      treepath = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tv_data->treeview),
                                    treepath, NULL,
                                    TRUE, 0.0, 0.0);
      gtk_tree_path_free (treepath);
   }
}


gboolean
detailview_set_selection (GimvThumbView *tv, GimvThumb *thumb, gboolean select)
{
   DetailViewData *tv_data;
   GList *node;
   gint pos;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   if (g_list_length (tv->thumblist) < 1) return FALSE;

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data && tv_data->treeview, FALSE);

   node = g_list_find (tv->thumblist, thumb);
   pos = g_list_position (tv->thumblist, node);

   if (pos >= 0) {
      GtkTreeView *treeview = GTK_TREE_VIEW (tv_data->treeview);
      GtkTreeModel *model = gtk_tree_view_get_model (treeview);
      GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
      GtkTreeIter iter;
      gboolean found;

      found = gtk_tree_model_iter_nth_child (model, &iter, NULL, pos);
      if (!found) return TRUE;

      thumb->selected = select;
      tv_data->set_selection = TRUE;
      if (thumb->selected) {
         gtk_tree_selection_select_iter (selection, &iter);
      } else {
         gtk_tree_selection_unselect_iter (selection, &iter);
      }
      tv_data->set_selection = FALSE;
   }

   return TRUE;
}


void
detailview_set_focus (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   if (g_list_length (tv->thumblist) < 1) return;

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->treeview);

   pos = g_list_index (tv->thumblist, thumb);

   if (pos >= 0) {
      GtkTreeView *treeview = GTK_TREE_VIEW (tv_data->treeview);
      GtkTreeModel *model = gtk_tree_view_get_model (treeview);
      GtkTreeIter iter;
      GtkTreePath *path;
      gboolean found;

      found = gtk_tree_model_iter_nth_child (model, &iter, NULL, pos);
      if (!found) return;
      path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_set_cursor (treeview, path, NULL, FALSE);
      gtk_tree_path_free (path);
   } else {
      gtk_widget_grab_focus (tv_data->treeview);
   }
}


GimvThumb *
detailview_get_focus (GimvThumbView *tv)
{
   GimvThumb *thumb = NULL;
   DetailViewData *tv_data;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);
   if (g_list_length (tv->thumblist) < 1) return NULL;

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data && tv_data->treeview, NULL);

   {
      GtkTreeView *treeview = GTK_TREE_VIEW (tv_data->treeview);
      GtkTreeModel *model = gtk_tree_view_get_model (treeview);
      GtkTreeIter iter;
      GtkTreePath *path;

      gtk_tree_view_get_cursor (treeview, &path, NULL);
      if (path) {
         if (gtk_tree_model_get_iter (model, &iter, path))
            gtk_tree_model_get (model, &iter,
                                COLUMN_THUMB_DATA, &thumb,
                                COLUMN_TERMINATOR);
         gtk_tree_path_free (path);
      }
   }

   return thumb;
}


gboolean
detailview_thumbnail_is_in_viewport (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   GList *node;
   gint row, row_top, row_bottom;
   GdkRectangle area;

   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkTreePath *treepath;
   GimvThumb *tmp_thumb;
   gboolean success;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   node = g_list_find (tv->thumblist, thumb);
   row = g_list_position (tv->thumblist, node);

   /* widget area */
   gtkutil_get_widget_area (tv_data->treeview, &area);

   /* get row range */
   model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv_data->treeview));

   if (!GTK_WIDGET_MAPPED (tv_data->treeview)) return FALSE;

   /* get index of top row */
   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                            0, 0, &treepath, NULL, NULL, NULL);
   if (!success) return FALSE;
   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_THUMB_DATA, &tmp_thumb,
                       COLUMN_TERMINATOR);
   row_top = g_list_index (tv->thumblist, tmp_thumb);
   gtk_tree_path_free (treepath);

   /* get index of bottom row */
   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv_data->treeview),
                                            0, area.height, &treepath,
                                            NULL, NULL, NULL);
   if (!success) return FALSE;
   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_THUMB_DATA, &tmp_thumb,
                       COLUMN_TERMINATOR);
   row_bottom = g_list_index (tv->thumblist, tmp_thumb);
   gtk_tree_path_free (treepath);

   /* intersect? */
   if (row >= row_top && row <= row_bottom)
      return TRUE;
   else
      return FALSE;
}


GtkWidget *
detailview_create (GimvThumbView *tv, const gchar *dest_mode)
{
   DetailViewData *tv_data;
   GtkListStore *store;
   GtkTreeSelection *selection;
   GType types[128];
   const gint ntypes   = sizeof (types) / sizeof (GType);
   gint i, ncolumns;
   gboolean show_title;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   detailview_create_title_idx_list ();
   ncolumns = detailview_title_idx_list_num + N_COLUMN;
   if (ncolumns > ntypes) {
      g_warning("Too many columns are specified: %d\n", ncolumns - N_COLUMN);
      ncolumns = ntypes;
   }

   tv_data = g_object_get_data (G_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) {
      tv_data = detailview_new (tv);
      g_return_val_if_fail (tv_data, NULL);
   }


   /* create tree view widget */
   store = gtk_list_store_new (1, G_TYPE_STRING);   /* this column is dummy */

   /* set real columns */
   for (i = 0; i < ncolumns; i++)
      types[i] = G_TYPE_STRING;
   types[COLUMN_THUMB_DATA] = G_TYPE_POINTER;
   types[COLUMN_PIXMAP]     = GDK_TYPE_PIXMAP;
   types[COLUMN_MASK]       = GDK_TYPE_PIXMAP;
   types[COLUMN_EDITABLE]   = G_TYPE_BOOLEAN;
   gtk_list_store_set_column_types (store, ncolumns, types);

   tv_data->treeview
      = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv_data->treeview), TRUE);
   set_column_types (tv, tv_data, store, dest_mode);
   detailview_prefs_get_value ("show_title", (gpointer) &show_title);
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv_data->treeview),
                                      show_title);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv_data->treeview));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
   gtk_tree_selection_set_select_function (selection,
                                           cb_tree_selected,
                                           tv, NULL);

   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->treeview, "DetailIconMode");
   }
   if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->treeview, "DetailThumbMode");
   }

   gtk_widget_show (tv_data->treeview);

   g_signal_connect (G_OBJECT (tv_data->treeview),
                     "key-press-event",
                     G_CALLBACK (cb_treeview_key_press), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview),
                     "button-press-event",
                     G_CALLBACK (cb_treeview_button_press), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview),
                     "scroll-event",
                     G_CALLBACK (cb_treeview_scroll), tv);
   /* SIGNAL_CONNECT_TRANSRATE_SCROLL (tv_data->treeview); */
   g_signal_connect (G_OBJECT (tv_data->treeview),
                     "button-release-event",
                     G_CALLBACK (cb_treeview_button_release), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview),
                     "motion-notify-event",
                     G_CALLBACK (cb_treeview_motion_notify), tv);

   /* for drop file list */
   dnd_src_set  (tv_data->treeview, detailview_dnd_targets, detailview_dnd_targets_num);
   dnd_dest_set (tv_data->treeview, detailview_dnd_targets, detailview_dnd_targets_num);

   g_signal_connect (G_OBJECT (tv_data->treeview), "drag_begin",
                     G_CALLBACK (gimv_thumb_view_drag_begin_cb), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview), "drag_data_get",
                     G_CALLBACK (gimv_thumb_view_drag_data_get_cb), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview), "drag_data_received",
                     G_CALLBACK (cb_treeview_drag_data_received), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview), "drag-data-delete",
                     G_CALLBACK (gimv_thumb_view_drag_data_delete_cb), tv);
   g_signal_connect (G_OBJECT (tv_data->treeview), "drag_end",
                     G_CALLBACK (gimv_thumb_view_drag_end_cb), tv);
   g_object_set_data (G_OBJECT (tv_data->treeview), "gimv-tab", tv);


   /* set data */
   if (tv->thumblist) {
      GList *node;

      for (node = tv->thumblist; node; node = g_list_next (node))
         detailview_append_thumb_frame (tv, node->data, dest_mode);
      detailview_set_pixmaps (tv, dest_mode);
   }

   return tv_data->treeview;
}

#endif /* ENABLE_TREEVIEW */
