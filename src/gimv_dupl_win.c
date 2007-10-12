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

#include "gimv_dupl_win.h"

#include "charset.h"
#include "fileutil.h"
#include "gimv_cell_pixmap.h"
#include "gimv_icon_stock.h"
#include "gimv_image_info.h"
#include "gimv_thumb.h"
#include "prefs.h"

struct GimvDuplWinPriv_Tag
{
   gint              thumbnail_size;

   GList            *thumb_list;

   GtkTreeViewColumn *pixmap_col;
   GtkCellRenderer   *pixmap_renderer;
};

static void gimv_dupl_win_init       (GimvDuplWin      *sw);
static void gimv_dupl_win_class_init (GimvDuplWinClass *klass);
static void gimv_dupl_win_destroy    (GtkObject        *object);


static void cb_select_all_button      (GtkButton        *button,
                                       GimvDuplWin      *sw);
static void cb_finder_stop_button     (GtkWidget        *widget,
                                       GimvDuplWin      *sw);
static void cb_finder_start           (GimvDuplFinder   *finder,
                                       GimvDuplWin      *sw);
static void cb_finder_stop            (GimvDuplFinder   *finder,
                                       GimvDuplWin      *sw);
static void cb_finder_progress_update (GimvDuplFinder   *finder,
                                       GimvDuplWin      *sw);
static void cb_finder_found           (GimvDuplFinder   *finder,
                                       GimvDuplPair     *pair,
                                       GimvDuplWin      *sw);
static void cb_select_thumb           (GimvThumb        *thumb,
                                       GimvDuplWin      *sw);

#ifdef ENABLE_TREEVIEW

typedef enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_THUMBNAIL,
   COLUMN_THUMBNAIL_MASK,
   COLUMN_ICON,
   COLUMN_ICON_MASK,
   COLUMN_NAME,
   COLUMN_ACCURACY,
   COLUMN_SIZE,
   COLUMN_MTIME,
   COLUMN_THUMBDATA,
   N_COLUMN
} TreeStoreColumn;

static void     cb_tree_cursor_changed        (GtkTreeView  *treeview,
                                               GimvDuplWin  *sw);
static void     cb_change_to_thumbnail_button (GtkButton    *button,
                                               GimvDuplWin  *sw);
static void     cb_change_to_icon_button      (GtkButton    *button,
                                               GimvDuplWin  *sw);
static gboolean find_row                      (GimvDuplWin  *sw,
                                               GimvThumb    *thumb,
                                               GtkTreeIter  *iter,
                                               GtkTreeIter  *parent_iter);
static gboolean insert_node                   (GimvDuplWin  *sw,
                                               GtkTreeIter  *iter,
                                               GtkTreeIter  *parent_iter,
                                               GimvThumb    *thumb,
                                               gfloat        similar);

#else /* ENABLE_TREEVIEW */

static void    set_pixtext                    (GtkCTree     *ctree,
                                               GtkCTreeNode *node,
                                               gpointer      data);
static void    cb_change_to_thumbnail_button  (GtkButton    *button,
                                               GimvDuplWin  *sw);
static void    cb_change_to_icon_button       (GtkButton    *button,
                                               GimvDuplWin  *sw);
static void    cb_ctree_select_row            (GtkCTree     *ctree,
                                               GList        *node,
                                               gint          column,
                                               GimvDuplWin  *sw);
static GtkCTreeNode *insert_node              (GimvDuplWin  *sw,
                                               GtkCTreeNode *parent,
                                               GimvThumb    *thumb,
                                               gfloat        similar);

#endif /* ENABLE_TREEVIEW */


gchar *simwin_titles[4] = {
   N_("Name"),
   N_("Accuracy"),
   N_("Size (byte)"),
   N_("Modification Time")
};
gint simwin_column_num = sizeof (simwin_titles) / sizeof (gchar *);

static GtkDialogClass *parent_class = NULL;


GtkType
gimv_dupl_win_get_type (void)
{
   static GtkType gimv_dupl_win_type = 0;

   if (!gimv_dupl_win_type) {
      static const GtkTypeInfo gimv_dupl_win_info = {
         "GimvDuplWin",
         sizeof (GimvDuplWin),
         sizeof (GimvDuplWinClass),
         (GtkClassInitFunc) gimv_dupl_win_class_init,
         (GtkObjectInitFunc) gimv_dupl_win_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_dupl_win_type = gtk_type_unique (gtk_dialog_get_type (),
                                            &gimv_dupl_win_info);
   }

   return gimv_dupl_win_type;
}


static void
gimv_dupl_win_init (GimvDuplWin *sw)
{
   GtkWidget *hbox;
   GtkWidget *scrolledwin, *radio, *button;
   gint i;

   sw->ctree           = NULL;
   sw->radio_thumb     = NULL;
   sw->radio_icon      = NULL;
   sw->select_button   = NULL;
   sw->progressbar     = NULL;

   sw->finder          = gimv_dupl_finder_new (NULL);
   sw->tv              = NULL;

   sw->priv            = g_new0 (GimvDuplWinPriv, 1);
   sw->priv->thumbnail_size  = 96;
   sw->priv->thumb_list      = NULL;
#ifdef ENABLE_TREEVIEW
   sw->priv->pixmap_col      = NULL;
   sw->priv->pixmap_renderer = NULL;
#endif /* ENABLE_TREEVIEW */

   /* window */
   gtk_window_set_title (GTK_WINDOW (sw), _("Find Duplicates - result")); 
   gtk_window_set_default_size (GTK_WINDOW (sw), 500, 400);
   gtk_window_set_position (GTK_WINDOW (sw), GTK_WIN_POS_MOUSE);

   /* ctree */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_set_border_width (GTK_CONTAINER (scrolledwin), 5);

   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (sw)->vbox),
                       scrolledwin, TRUE, TRUE, 0);

#ifdef ENABLE_TREEVIEW
   {
      GtkTreeStore *store;
      GtkTreeViewColumn *col;
      GtkCellRenderer *render;

      store = gtk_tree_store_new (N_COLUMN,
                                  GDK_TYPE_PIXMAP,
                                  GDK_TYPE_PIXMAP,
                                  GDK_TYPE_PIXMAP,
                                  GDK_TYPE_PIXMAP,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_POINTER);
      sw->ctree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      gtk_container_add (GTK_CONTAINER (scrolledwin), sw->ctree);

      gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sw->ctree), TRUE);

      /* name column */
      col = gtk_tree_view_column_new();
      sw->priv->pixmap_col = col;
      gtk_tree_view_column_set_title (col, _(simwin_titles[0]));

      render = gimv_cell_renderer_pixmap_new ();
      sw->priv->pixmap_renderer = render;
      gtk_tree_view_column_pack_start (col, render, FALSE);
      gtk_tree_view_column_add_attribute (col, render,
                                          "pixmap", COLUMN_ICON);
      gtk_tree_view_column_add_attribute (col, render,
                                          "mask", COLUMN_ICON_MASK);

      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_NAME);

      gtk_tree_view_append_column (GTK_TREE_VIEW (sw->ctree), col);
      gtk_tree_view_set_expander_column (GTK_TREE_VIEW (sw->ctree), col);

      /* other column */
      for (i = 1; i < simwin_column_num; i++) {
         col = gtk_tree_view_column_new();
         gtk_tree_view_column_set_title (col, _(simwin_titles[i]));

         render = gtk_cell_renderer_text_new ();
         gtk_tree_view_column_pack_start (col, render, TRUE);
         gtk_tree_view_column_add_attribute (col, render, "text",
                                             COLUMN_NAME + i);

         gtk_tree_view_append_column (GTK_TREE_VIEW (sw->ctree), col);
      }

      g_signal_connect (G_OBJECT (sw->ctree), "cursor_changed",
                        G_CALLBACK (cb_tree_cursor_changed), sw);
   }
#else /* ENABLE_TREEVIEW */
   {
      for (i = 0; i < simwin_column_num; i++)
         simwin_titles[i] = _(simwin_titles[i]);
      sw->ctree = gtk_ctree_new_with_titles (simwin_column_num, 0, simwin_titles);
      gtk_clist_set_column_width (GTK_CLIST (sw->ctree), 0, 250);
      gtk_clist_set_column_width (GTK_CLIST (sw->ctree), 1, 50);
      gtk_clist_set_column_width (GTK_CLIST (sw->ctree), 2, 50);
      gtk_clist_set_column_width (GTK_CLIST (sw->ctree), 3, 150);
      /*
      for (i = 0; i < simwin_column_num; i++)
         gtk_clist_set_column_auto_resize (GTK_CLIST (sw->ctree), i, TRUE);
      */
      gtk_clist_set_column_justification(GTK_CLIST (sw->ctree), 1,
                                         GTK_JUSTIFY_CENTER);
      gtk_clist_set_column_justification(GTK_CLIST (sw->ctree), 2,
                                         GTK_JUSTIFY_RIGHT);
      gtk_container_add (GTK_CONTAINER (scrolledwin), sw->ctree);

      gtk_signal_connect (GTK_OBJECT (sw->ctree), "tree_select_row",
                          GTK_SIGNAL_FUNC (cb_ctree_select_row), sw);
   }
#endif /* ENABLE_TREEVIEW */

   /* button */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (sw)->action_area), 
                       hbox, TRUE, TRUE, 0);

   /* radio button */
   radio = gtk_radio_button_new_with_label (NULL, _("Thumbnail"));
   sw->radio_thumb = radio;
   gtk_signal_connect (GTK_OBJECT (radio), "clicked",
                       GTK_SIGNAL_FUNC (cb_change_to_thumbnail_button), sw);
   gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);

   radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio),
                                                        _("Icon"));
   sw->radio_icon  = radio;
   gtk_signal_connect (GTK_OBJECT (radio), "clicked",
                       GTK_SIGNAL_FUNC (cb_change_to_icon_button), sw);
   gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);

   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);

   /* Select All */
   button = gtk_button_new_with_label (_("Select All"));
   sw->select_button = button;
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "clicked",
                       GTK_SIGNAL_FUNC (cb_select_all_button), sw);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   /* close button */
   button = gtk_button_new_with_label (_("Stop"));
   sw->stop_button = button;
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT (button), "clicked",
                       GTK_SIGNAL_FUNC (cb_finder_stop_button), sw);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   /* gtk_widget_grab_focus (button); */

   /* progress bar */
   sw->progressbar = gtk_progress_bar_new();
   gtk_box_pack_end (GTK_BOX (hbox), sw->progressbar, FALSE, FALSE, 0);

   /* finder */
   gtk_signal_connect (GTK_OBJECT (sw->finder), "start",
                       GTK_SIGNAL_FUNC (cb_finder_start), sw);
   gtk_signal_connect (GTK_OBJECT (sw->finder), "stop",
                       GTK_SIGNAL_FUNC (cb_finder_stop), sw);
   gtk_signal_connect (GTK_OBJECT (sw->finder), "progress_update",
                       GTK_SIGNAL_FUNC (cb_finder_progress_update), sw);
   gtk_signal_connect (GTK_OBJECT (sw->finder), "found",
                       GTK_SIGNAL_FUNC (cb_finder_found), sw);
}


static void
gimv_dupl_win_class_init (GimvDuplWinClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gtk_dialog_get_type ());

   object_class->destroy = gimv_dupl_win_destroy;
}


static void
gimv_dupl_win_destroy (GtkObject *object)
{
   GimvDuplWin *sw = GIMV_DUPL_WIN (object);

   g_return_if_fail (sw);

   if (sw->priv) {
      g_list_foreach (sw->priv->thumb_list, (GFunc) gtk_object_unref, NULL);
      g_list_free (sw->priv->thumb_list);
      sw->priv->thumb_list = NULL;
      g_free (sw->priv);
      sw->priv = NULL;
   }

   if (sw->finder) {
      gtk_object_unref (GTK_OBJECT (sw->finder));
      sw->finder = NULL;
   }

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
ghfunc_select_thumb (GimvThumb *thumb)
{
   gimv_thumb_view_set_selection (thumb, TRUE);
}


static void
cb_select_all_button (GtkButton *button, GimvDuplWin *sw)
{
   g_return_if_fail (sw);
   if (!sw->tv) return;

   gimv_thumb_view_set_selection_all (sw->tv, FALSE);
   g_list_foreach (sw->priv->thumb_list, (GFunc) ghfunc_select_thumb, NULL);
}


static void
cb_finder_stop_button (GtkWidget *widget, GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   if (sw->finder)
      gimv_dupl_finder_stop (sw->finder);
}


static void
cb_finder_start (GimvDuplFinder *finder, GimvDuplWin *sw)
{
   gfloat progress;

   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));
   g_return_if_fail (sw);

   gtk_widget_set_sensitive (sw->stop_button, TRUE);

   gtk_progress_set_show_text (GTK_PROGRESS (sw->progressbar), TRUE);
   gtk_progress_set_format_string (GTK_PROGRESS (sw->progressbar),
                                   _("Finding similar images..."));
   progress = gimv_dupl_finder_get_progress (finder);
   gtk_progress_bar_update (GTK_PROGRESS_BAR (sw->progressbar), progress);
}


static void
cb_finder_stop (GimvDuplFinder *finder, GimvDuplWin *sw)
{
   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));
   g_return_if_fail (sw);

   gtk_widget_set_sensitive (sw->stop_button, FALSE);

   gtk_progress_bar_update (GTK_PROGRESS_BAR (sw->progressbar), 0.0);
   /* gtk_progress_set_show_text (GTK_PROGRESS (sw->progressbar), FALSE); */
   gtk_progress_set_format_string (GTK_PROGRESS (sw->progressbar),
                                   _("Completed"));
}


static void
cb_finder_progress_update (GimvDuplFinder *finder, GimvDuplWin *sw)
{
   gfloat progress;

   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));
   g_return_if_fail (sw);

   progress = gimv_dupl_finder_get_progress (finder);
   gtk_progress_bar_update (GTK_PROGRESS_BAR (sw->progressbar), progress);
}


static void
cb_finder_found (GimvDuplFinder *finder, GimvDuplPair *pair, GimvDuplWin *sw)
{
   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));
   g_return_if_fail (sw);
   g_return_if_fail (pair);

   gimv_dupl_win_set_thumb (sw, pair->thumb1, pair->thumb2, pair->similarity);
}


static void
cb_select_thumb (GimvThumb *thumb, GimvDuplWin *sw)
{
   g_return_if_fail (GIMV_IS_THUMB (thumb));
   g_return_if_fail (sw);

  if (conf.simwin_sel_thumbview) {
      gimv_thumb_view_set_selection_all (sw->tv, FALSE);
      gimv_thumb_view_set_selection (thumb, TRUE);
      gimv_thumb_view_adjust (sw->tv, thumb);
   }

   if (conf.simwin_sel_preview)
      gimv_thumb_view_open_image (sw->tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW);
   if (conf.simwin_sel_new_win)
      gimv_thumb_view_open_image (sw->tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_NEW_WIN);
   if (conf.simwin_sel_shared_win)
      gimv_thumb_view_open_image (sw->tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN);
}


#ifdef ENABLE_TREEVIEW

static void
cb_tree_cursor_changed (GtkTreeView *treeview, GimvDuplWin *sw)
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   GimvThumb *thumb;

   g_return_if_fail (treeview);
   g_return_if_fail (sw);

   if (!sw->tv) return;

   selection = gtk_tree_view_get_selection (treeview);
   if (!gtk_tree_selection_get_selected (selection, &model, &iter)) return;
   gtk_tree_model_get (model, &iter,
                       COLUMN_THUMBDATA, &thumb,
                       COLUMN_TERMINATOR);

   g_return_if_fail (GIMV_IS_THUMB (thumb));

   cb_select_thumb (thumb, sw);
}


static void
cb_change_to_thumbnail_button (GtkButton *button, GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   gtk_tree_view_column_clear_attributes (sw->priv->pixmap_col,
                                          sw->priv->pixmap_renderer);
   gtk_tree_view_column_add_attribute (sw->priv->pixmap_col,
                                       sw->priv->pixmap_renderer,
                                       "pixmap", COLUMN_THUMBNAIL);
   gtk_tree_view_column_add_attribute (sw->priv->pixmap_col,
                                       sw->priv->pixmap_renderer,
                                       "mask", COLUMN_THUMBNAIL_MASK);
}


static void
cb_change_to_icon_button (GtkButton *button, GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   gtk_tree_view_column_clear_attributes (sw->priv->pixmap_col,
                                          sw->priv->pixmap_renderer);
   gtk_tree_view_column_add_attribute (sw->priv->pixmap_col,
                                       sw->priv->pixmap_renderer,
                                       "pixmap", COLUMN_ICON);
   gtk_tree_view_column_add_attribute (sw->priv->pixmap_col,
                                       sw->priv->pixmap_renderer,
                                       "mask", COLUMN_ICON_MASK);
}


static gboolean
find_row (GimvDuplWin *sw, GimvThumb *thumb,
          GtkTreeIter *iter, GtkTreeIter *parent_iter)
{
   GtkTreeModel *model;
   GtkTreeIter tmp_iter;
   GimvThumb *tmp_thumb;
   gboolean go_next;

   g_return_val_if_fail (sw && thumb && iter, FALSE);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (sw->ctree));

   if (!parent_iter)
      go_next = gtk_tree_model_get_iter_first (model, iter);
   else
      go_next = gtk_tree_model_iter_children (model, iter, parent_iter);

   for (; go_next; go_next = gtk_tree_model_iter_next (model, iter)) {
      gtk_tree_model_get (model, iter,
                          COLUMN_THUMBDATA, &tmp_thumb,
                          COLUMN_TERMINATOR);

      if (tmp_thumb == thumb)
         return TRUE;

      if (find_row (sw, thumb, &tmp_iter, iter)) {
         *iter = tmp_iter;
         return TRUE;
      }
   }

   return FALSE;
}


static gboolean
insert_node (GimvDuplWin *sw,
             GtkTreeIter *iter, GtkTreeIter *parent_iter,
             GimvThumb *thumb, gfloat similar)
{
   GtkTreeModel *model;
   GdkPixmap *thumb_pixmap, *icon_pixmap;
   GdkBitmap *thumb_mask, *icon_mask;
   gchar *text[32], accuracy[32], *tmpstr;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   gimv_thumb_get_thumb (thumb, &thumb_pixmap, &thumb_mask);
   gimv_thumb_get_icon (thumb,  &icon_pixmap,  &icon_mask);

   text[0] = (gchar *) gimv_image_info_get_path (thumb->info);
   text[0] = charset_to_internal (text[0],
                                  conf.charset_filename,
                                  conf.charset_auto_detect_fn,
                                  conf.charset_filename_mode);

   if (similar > 0) {
      g_snprintf (accuracy, 32, "%2.1f%%", similar * 100);
      text[1] = accuracy;
   } else {
      text[1] = NULL;
   }

   tmpstr  = fileutil_size2str (thumb->info->st.st_size, FALSE);
   text[2] = charset_locale_to_internal (tmpstr);
   g_free (tmpstr);

   tmpstr  = fileutil_time2str (thumb->info->st.st_mtime);
   text[3] = charset_locale_to_internal (tmpstr);
   g_free (tmpstr);

   gtk_object_ref (GTK_OBJECT(thumb));
   sw->priv->thumb_list = g_list_append (sw->priv->thumb_list, thumb);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (sw->ctree));
   gtk_tree_store_append (GTK_TREE_STORE (model), iter, parent_iter);
   gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                       COLUMN_THUMBNAIL,      thumb_pixmap,
                       COLUMN_THUMBNAIL_MASK, thumb_mask,
                       COLUMN_ICON,           icon_pixmap,
                       COLUMN_ICON_MASK,      icon_mask,
                       COLUMN_NAME,           text[0],
                       COLUMN_ACCURACY,       text[1],
                       COLUMN_SIZE,           text[2],
                       COLUMN_MTIME,          text[3],
                       COLUMN_THUMBDATA,      thumb,
                       COLUMN_TERMINATOR);

   g_free (text[0]);
   g_free (text[2]);
   g_free (text[3]);

   return TRUE;
}

#else /* ENABLE_TREEVIEW */

static void
set_pixtext (GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
   gboolean thumbnail = GPOINTER_TO_INT (data);
   GimvThumb *thumb;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   guint8 spacing;
   gboolean is_leaf, expanded;
   gchar *text;

   g_return_if_fail (ctree);
   g_return_if_fail (node);

   thumb = gtk_ctree_node_get_row_data (ctree, node);
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   if (thumbnail)
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   else
      gimv_thumb_get_icon (thumb, &pixmap, &mask);

   gtk_ctree_get_node_info (ctree, node, &text, &spacing,
                            NULL, NULL, NULL, NULL,
                            &is_leaf, &expanded);
   gtk_ctree_set_node_info (ctree, node,
                            text, spacing,
                            pixmap, mask, pixmap, mask,
                            is_leaf, expanded);
}


static void
cb_change_to_thumbnail_button (GtkButton *button, GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   gtk_clist_set_row_height (GTK_CLIST (sw->ctree), sw->priv->thumbnail_size);
   gtk_ctree_post_recursive (GTK_CTREE (sw->ctree), NULL,
                             (GtkCTreeFunc) set_pixtext,
                             GINT_TO_POINTER (TRUE));
}


static void
cb_change_to_icon_button (GtkButton *button, GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   gtk_clist_set_row_height (GTK_CLIST (sw->ctree), ICON_SIZE);
   gtk_ctree_post_recursive (GTK_CTREE (sw->ctree), NULL,
                             (GtkCTreeFunc) set_pixtext,
                             GINT_TO_POINTER (FALSE));
}


static void
cb_ctree_select_row (GtkCTree *ctree, GList *node, gint column, GimvDuplWin *sw)
{
   GimvThumb *thumb;

   g_return_if_fail (ctree);
   g_return_if_fail (node);
   g_return_if_fail (sw);

   if (!sw->tv) return;

   thumb = gtk_ctree_node_get_row_data (ctree, GTK_CTREE_NODE (node));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   cb_select_thumb (thumb, sw);
}


static GtkCTreeNode *
insert_node (GimvDuplWin *sw,
             GtkCTreeNode *parent,
             GimvThumb *thumb,
             gfloat similar)
{
   GtkCTreeNode *node;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   gchar *text[32], accuracy[32], *tmpstr;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   if (GTK_TOGGLE_BUTTON (sw->radio_thumb)->active)
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   else
      gimv_thumb_get_icon (thumb, &pixmap, &mask);

   text[0] = (gchar *) gimv_image_info_get_path (thumb->info);
   text[0] = charset_to_internal (text[0],
                                  conf.charset_filename,
                                  conf.charset_auto_detect_fn,
                                  conf.charset_filename_mode);

   if (similar > 0) {
      g_snprintf (accuracy, 32, "%2.1f%%", similar * 100);
      text[1] = accuracy;
   } else {
      text[1] = NULL;
   }

   tmpstr  = fileutil_size2str (thumb->info->st.st_size, FALSE);
   text[2] = charset_locale_to_internal (tmpstr);
   g_free (tmpstr);

   tmpstr  = fileutil_time2str (thumb->info->st.st_mtime);
   text[3] = charset_locale_to_internal (tmpstr);
   g_free (tmpstr);

   node = gtk_ctree_insert_node (GTK_CTREE (sw->ctree),
                                 parent, NULL, text, 4,
                                 pixmap, mask,
                                 pixmap, mask,
                                 FALSE, FALSE);
   gtk_object_ref (GTK_OBJECT(thumb));
   sw->priv->thumb_list = g_list_append (sw->priv->thumb_list, thumb);
   gtk_ctree_node_set_row_data (GTK_CTREE (sw->ctree), node, thumb);

   g_free (text[0]);
   g_free (text[2]);
   g_free (text[3]);

   return node;
}

#endif /* ENABLE_TREEVIEW */



/******************************************************************************
 *
 *   Public Functions.
 *
 ******************************************************************************/
GimvDuplWin *
gimv_dupl_win_new (gint thumbnail_size)
{
   GimvDuplWin *sw;

   sw = GIMV_DUPL_WIN (gtk_type_new (gimv_dupl_win_get_type ()));

   /* FIXME */
   sw->priv->thumbnail_size = thumbnail_size;
   gtk_widget_show_all (GTK_WIDGET (sw));
   gimv_icon_stock_set_window_icon (GTK_WIDGET (sw)->window, "gimv_icon");
   /* END FIXME */

   return sw;
}


void
gimv_dupl_win_set_relation (GimvDuplWin *sw, GimvThumbView *tv)
{
   g_return_if_fail (sw);
   g_return_if_fail (tv);

   sw->tv = tv;

   gtk_widget_set_sensitive (sw->select_button, TRUE);
}


void
gimv_dupl_win_unset_relation (GimvDuplWin *sw)
{
   g_return_if_fail (sw);

   sw->tv = NULL;

   gtk_widget_set_sensitive (sw->select_button, FALSE);
}


void
gimv_dupl_win_set_thumb (GimvDuplWin *sw,
                         GimvThumb *thumb1,
                         GimvThumb*thumb2,
                         gfloat     similar)
{
   g_return_if_fail (sw);
   g_return_if_fail (GIMV_IS_THUMB (thumb1));
   g_return_if_fail (GIMV_IS_THUMB (thumb2));

#ifdef ENABLE_TREEVIEW
{
   GtkTreeIter parent_iter, iter;
   gboolean success;

   success = find_row (sw, thumb1, &parent_iter, NULL);
   if (!success) {
      GtkTreeView *treeview = GTK_TREE_VIEW (sw->ctree);
      GtkTreeModel *model;
      GtkTreeSelection *selection;

      success = insert_node (sw, &parent_iter, NULL, thumb1, -1);

      selection = gtk_tree_view_get_selection (treeview);
      if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
         if (find_row (sw, thumb1, &iter, NULL)) {
            GtkTreePath *treepath = gtk_tree_model_get_path (model, &parent_iter);

            g_signal_handlers_block_by_func (sw->ctree,
                                           cb_tree_cursor_changed,
                                           sw);
            gtk_tree_view_set_cursor (treeview, treepath, NULL, FALSE);
            g_signal_handlers_unblock_by_func (sw->ctree,
                                             cb_tree_cursor_changed,
                                             sw);
            gtk_tree_path_free (treepath);
         }
      }
   }
   if (success) {
      GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (sw->ctree));
      GtkTreePath *treepath = gtk_tree_model_get_path (model, &parent_iter);

      success = insert_node (sw, &iter, &parent_iter, thumb2, similar);
      gtk_tree_view_expand_row (GTK_TREE_VIEW (sw->ctree), treepath, FALSE);
      gtk_tree_path_free (treepath);
   }
}
#else /* ENABLE_TREEVIEW */
{
   GtkCTreeNode *parent, *node;

   node = gtk_ctree_find_by_row_data (GTK_CTREE (sw->ctree), NULL, thumb1);
   if (node)
      parent = node;
   else
      parent = insert_node (sw, node, thumb1, -1);

   node = insert_node (sw, parent, thumb2, similar);
   gtk_ctree_expand (GTK_CTREE (sw->ctree), parent);
}
#endif /* ENABLE_TREEVIEW */
}
