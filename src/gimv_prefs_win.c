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

#include "gimv_prefs_win.h"

#include <stdlib.h>
#include <string.h>

#include "gimageview.h"

#include "gtk2-compat.h"
#include "gimv_cell_pixmap.h"
#include "gimv_icon_stock.h"
#include "help.h"
#include "prefs.h"
#include "prefs_ui/prefs_ui_common.h"
#include "prefs_ui/prefs_ui_etc.h"
#include "prefs_ui/prefs_ui_movie.h"
#include "prefs_ui/prefs_ui_plugin.h"
#include "prefs_ui/prefs_ui_progs.h"
#include "prefs_ui/prefs_ui_imagewin.h"
#include "prefs_ui/prefs_ui_thumbalbum.h"
#include "prefs_ui/prefs_ui_thumbwin.h"


typedef struct PrefsWin_Tag {
   GtkWidget *notebook;
   GtkWidget *tree;
   gboolean   ok_pressed;
} PrefsWin;


typedef struct GimvPrefsWinPagePrivate_Tag
{
   GimvPrefsWinPage *page;
   GtkWidget    *widget;
} GimvPrefsWinPagePrivate;


static GimvPrefsWinPage prefs_pages[] = {
   {N_("/Infomation"),                 0, NULL, NULL, gimvhelp_create_info_widget,  NULL},
   {N_("/Common"),                     0, NULL, NULL, prefs_common_page,            NULL},
   {N_("/Common/Filtering"),           0, NULL, NULL, prefs_filter_page,            prefs_filter_apply},
   {N_("/Common/Character set"),       0, NULL, NULL, prefs_charset_page,           prefs_charset_apply},

   {N_("/Image Window"),               0, NULL, NULL, prefs_imagewin_page,          NULL},
   {N_("/Image Window/Image"),         0, NULL, NULL, prefs_imagewin_image_page,    prefs_ui_imagewin_image_apply},
   {N_("/Image Window/Mouse Buttton"), 0, NULL, NULL, prefs_imagewin_mouse_page,    NULL},

   {N_("/Thumbnail Window"),           0, NULL, NULL, prefs_thumbwin_page,          prefs_ui_thumbwin_apply},
   /*
   {N_("/Thumbnail Window/Toolbar"),   0, NULL, NULL, prefs_thumbview_toolbar_page, NULL},
   */
   {N_("/Thumbnail Window/Tab"),                            0, NULL, NULL, prefs_thumbwin_tab_page,      prefs_ui_thumbwin_tab_apply},
   {N_("/Thumbnail Window/Thumbnail View"),                 0, NULL, NULL, prefs_thumbview_page,         NULL},
   {N_("/Thumbnail Window/Thumbnail View/Mouse Buttton"),   0, NULL, NULL, prefs_thumbview_mouse_page,   NULL},
   {N_("/Thumbnail Window/Thumbnail View/Album"),           0, NULL, NULL, prefs_ui_thumbalbum,          NULL},

   {N_("/Thumbnail Window/Directory View"),                 0, NULL, NULL, prefs_dirview_page,           prefs_ui_dirview_apply},
   {N_("/Thumbnail Window/Directory View/Mouse Buttton"),   0, NULL, NULL, prefs_dirview_mouse_page,     NULL},
   {N_("/Thumbnail Window/Preview"),                        0, NULL, NULL, prefs_preview_page,           prefs_ui_preview_apply},
   {N_("/Thumbnail Window/Preview/Mouse Buttton"),          0, NULL, NULL, prefs_preview_mouse_page,     NULL},

   {N_("/Movie and Audio"),            0, NULL, NULL, prefs_movie_page,               NULL},

   {N_("/Slide Show"),                 0, NULL, NULL, prefs_slideshow_page,           NULL},
   {N_("/Thumbnail Cache"),            0, NULL, NULL, prefs_cache_page,               NULL},
   {N_("/Comment"),                    0, NULL, NULL, prefs_comment_page,             prefs_comment_apply},
   {N_("/Search"),                     0, NULL, NULL, prefs_search_page,              NULL},
   {N_("/Drag and Drop"),              0, NULL, NULL, prefs_dnd_page,                 NULL},
   {N_("/External Program"),           0, NULL, NULL, prefs_progs_page,               NULL},
   {N_("/External Program/Scripts"),   0, NULL, NULL, prefs_scripts_page,             NULL},

   {N_("/Plugin"),                     0, NULL, NULL, prefs_ui_plugin,              prefs_ui_plugin_apply},
   {N_("/Plugin/Image Loader"),        0, NULL, NULL, prefs_ui_plugin_image_loader, NULL},
   {N_("/Plugin/Image Saver"),         0, NULL, NULL, prefs_ui_plugin_image_saver,  NULL},
   {N_("/Plugin/IO Stream"),           0, NULL, NULL, prefs_ui_plugin_io_stream,    NULL},
   {N_("/Plugin/Archiver"),            0, NULL, NULL, prefs_ui_plugin_archiver,     NULL},
   {N_("/Plugin/Thumbnail"),           0, NULL, NULL, prefs_ui_plugin_thumbnail,    NULL},
   {N_("/Plugin/Image View"),          0, NULL, NULL, prefs_ui_plugin_imageview,    NULL},
   {N_("/Plugin/Thumbnail View"),      0, NULL, NULL, prefs_ui_plugin_thumbview,    NULL},
};
static gint prefs_pages_num = sizeof (prefs_pages) / sizeof (prefs_pages[0]);

GList *prefs_pages_list = NULL;

Config *config_changed    = NULL;
Config *config_prechanged = NULL;


static GtkWidget *prefs_window;
static PrefsWin   prefs_win = {
   notebook:   NULL,
   tree:       NULL,
   ok_pressed: FALSE,
};
static GList *priv_page_list = NULL;


static GList *
get_page_entries_list (void)
{
   if (!prefs_pages_list) {
      gint i;

      for (i = 0; i < prefs_pages_num; i++)
         prefs_pages_list = g_list_append (prefs_pages_list, &prefs_pages[i]);
   }

   return prefs_pages_list;
}


gboolean
gimv_prefs_win_add_page_entry (GimvPrefsWinPage *page)
{
   g_return_val_if_fail (page, FALSE);

   get_page_entries_list ();
#warning FIXME: check path
   prefs_pages_list = g_list_append (prefs_pages_list, page);
   return TRUE;
}


/*******************************************************************************
 *
 *   Private functions
 *
 *******************************************************************************/
static void
prefs_win_apply_config (GimvPrefsWinAction action)
{
   GList *node;

   for (node = priv_page_list; node; node = g_list_next (node)) {
      GimvPrefsWinPagePrivate *priv = node->data;
      if (priv && priv->widget && priv->page && priv->page->apply_fn)
         priv->page->apply_fn (action);
   }
}


/* FIXME!! too stupid ;-( */
/* Damn Damn Damn XD */
static void
prefs_win_free_strings (gboolean value_changed)
{
   gint i;
   gchar **strings[] = {
      &conf.text_viewer,
      &conf.web_browser,
      &conf.plugin_search_dir_list,
      &conf.scripts_search_dir_list,
      &conf.imgview_mouse_button,
      &conf.preview_mouse_button,
      &conf.imgtype_disables,
      &conf.imgtype_user_defs,
      &conf.comment_key_list,
      &conf.charset_locale,
      &conf.charset_internal,
   };
   gint num1 = sizeof (strings) / sizeof (gchar **);
   gint num2 = sizeof (conf.progs) / sizeof (gchar *);

   for (i = 0; i < num1 + num2; i++) {
      gchar **src, **dest, **member;
      gulong offset;

      if (i < num1)
         member = strings[i];
      else if (i < num1 + num2)
         member = &conf.progs[i - num1];
      else
         break;

      offset = (gchar *) member - (gchar *) &conf;
      src  = G_STRUCT_MEMBER_P (config_prechanged, offset);
      dest = G_STRUCT_MEMBER_P (config_changed, offset);

      if (*src == *dest) continue;

      if (value_changed) {
         g_free (*src);
      } else {
         g_free (*dest);
      }
   }
}
/* END FIXME!! */


static void
prefs_win_create_page (GimvPrefsWinPagePrivate *priv)
{
   const gchar *title = NULL;
   GtkWidget *vbox, *label;

   if (!priv || !priv->page) return;
   if (priv->widget) return;

   /* translate page title */
   if (priv->page->path)
      title = g_basename (_(priv->page->path));

   /* create page */
   if (priv->page->create_page_fn) {
      vbox = priv->page->create_page_fn ();
      label = gtk_label_new (title);
      gtk_notebook_append_page (GTK_NOTEBOOK(prefs_win.notebook),
                                vbox, label);
      priv->widget = vbox;
   }
}


static void
prefs_win_set_page (const gchar *path)
{
   GimvPrefsWinPagePrivate *priv = NULL;
   GList *node;
   gint num;

   if (!path || !*path) {
      if (!priv_page_list) return;
      priv = priv_page_list->data;
   } else {
      for (node = priv_page_list; node; node = g_list_next (node)) {
         priv = node->data;
         if (priv->page && !strcmp (path, priv->page->path)) 
            break;
         priv = NULL;
      }
   }

   if (!priv) {
      if (!priv_page_list) return;
      priv = priv_page_list->data;
   }

   if (!priv->widget) {
      prefs_win_create_page (priv);
   }
   if (priv->widget)
      gtk_widget_show (priv->widget);
   else
      return;

   num = gtk_notebook_page_num (GTK_NOTEBOOK (prefs_win.notebook),
                                priv->widget);
   if (num >= 0)
      gtk_notebook_set_page (GTK_NOTEBOOK (prefs_win.notebook), num);
}



/*******************************************************************************
 *
 *  functions for creating navigate tree
 *
 *******************************************************************************/
static void
get_icon (const gchar *name, GdkPixmap **pixmap, GdkBitmap **mask)
{
   GimvIcon *icon;

   if (!name || !*name) return;

   icon  = gimv_icon_stock_get_icon (name);
   if (icon) {
      if (pixmap)
         *pixmap = icon->pixmap;
      if (mask)
         *mask = icon->mask;
   } else {
      if (pixmap)
         *pixmap = NULL;
      if (mask)
         *mask = NULL;
   }
}


typedef enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_ICON_OPEN,
   COLUMN_ICON_OPEN_MASK,
   COLUMN_ICON_CLOSE,
   COLUMN_ICON_CLOSE_MASK,
   COLUMN_NAME,
   COLUMN_PRIV_DATA,
   N_COLUMN
} TreeStoreColumn;


struct FindNode {
   gchar               *path;
   GimvPrefsWinPagePrivate *priv;
   GtkTreeIter         iter;
};


static gboolean
find_node_by_path (GtkTreeModel *model,
                   GtkTreePath *path, GtkTreeIter *iter,
                   gpointer data)
{
   struct FindNode *node = data;
   GimvPrefsWinPagePrivate *priv;

   gtk_tree_model_get (model, iter,
                       COLUMN_PRIV_DATA, &priv,
                       COLUMN_TERMINATOR);
   if (priv && !strcmp (priv->page->path, node->path)) {
      node->priv = priv;
      node->iter = *iter;
      return TRUE;
   }

   return FALSE;
}


static gboolean
prefs_win_navtree_get_parent (GimvPrefsWinPagePrivate *priv, GtkTreeIter *iter)
{
   GtkTreeModel *model;
   struct FindNode node;

   g_return_val_if_fail (priv, FALSE);
   g_return_val_if_fail (priv->page, FALSE);
   g_return_val_if_fail (priv->page->path, FALSE);

   node.path = g_dirname (priv->page->path);
   node.priv  = NULL;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs_win.tree));
   gtk_tree_model_foreach (model, find_node_by_path, &node);

   g_free (node.path);

   if (node.priv) {
      *iter = node.iter;
      return TRUE;
   }

   return FALSE;
}


static gboolean
cb_tree_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkTreePath *treepath;
   gboolean success, retval = FALSE;

   g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
   if (!selection) return FALSE;
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return FALSE;
   treepath = gtk_tree_model_get_path (model, &iter);
   if (!treepath) return FALSE;

   switch (event->keyval) {
   case GDK_KP_Enter:
   case GDK_Return:
   case GDK_ISO_Enter:
   case GDK_space:
      if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), treepath))
         gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), treepath);
      else
         gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), treepath, FALSE);
      retval = TRUE;
      break;
   case GDK_Right:
      gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), treepath, FALSE);
      retval = TRUE;
      break;
   case GDK_Left:
      gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), treepath);
      retval = TRUE;
      break;
   default:
      break;
   }

   gtk_tree_path_free(treepath);

   return retval;
}


static void
cb_tree_cursor_changed (GtkTreeView *treeview, gpointer data)
{
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   GimvPrefsWinPagePrivate *priv;

   g_return_if_fail (treeview);

   selection = gtk_tree_view_get_selection (treeview);
   gtk_tree_selection_get_selected (selection, &model, &iter);
   gtk_tree_model_get (model, &iter,
                       COLUMN_PRIV_DATA, &priv,
                       COLUMN_TERMINATOR);

   g_return_if_fail (priv);
   g_return_if_fail (priv->page);

   prefs_win_set_page (priv->page->path);
}


static GtkWidget *
prefs_win_create_navtree (void)
{
   GtkTreeStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;
   GtkWidget *tree;
   GList *node;

   store = gtk_tree_store_new (N_COLUMN,
                               GDK_TYPE_PIXMAP,
                               GDK_TYPE_PIXMAP,
                               GDK_TYPE_PIXMAP,
                               GDK_TYPE_PIXMAP,
                               G_TYPE_STRING,
                               G_TYPE_POINTER);
   tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   prefs_win.tree = tree;
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

   g_signal_connect (G_OBJECT (tree), "key_press_event",
                     G_CALLBACK (cb_tree_key_press), NULL);
   g_signal_connect (G_OBJECT (tree), "cursor_changed",
                     G_CALLBACK (cb_tree_cursor_changed), NULL);

   /* name column */
   col = gtk_tree_view_column_new();

   render = gimv_cell_renderer_pixmap_new ();
   gtk_tree_view_column_pack_start (col, render, FALSE);
   gtk_tree_view_column_add_attribute (col, render, "pixmap",
                                       COLUMN_ICON_CLOSE);
   gtk_tree_view_column_add_attribute (col, render, "mask",
                                       COLUMN_ICON_CLOSE_MASK);
   gtk_tree_view_column_add_attribute (col, render, "pixmap_expander_open",
                                       COLUMN_ICON_OPEN);
   gtk_tree_view_column_add_attribute (col, render, "mask_expander_open",
                                       COLUMN_ICON_OPEN_MASK);
   gtk_tree_view_column_add_attribute (col, render, "pixmap_expander_closed",
                                       COLUMN_ICON_CLOSE);
   gtk_tree_view_column_add_attribute (col, render, "mask_expander_closed",
                                       COLUMN_ICON_CLOSE_MASK);

   render = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, render, TRUE);
   gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_NAME);

   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), col);
   gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree), col);

   /* create pages */
   for (node = get_page_entries_list(); node; node = g_list_next (node)) {
      GimvPrefsWinPage *page = node->data;
      const gchar *title;
      GdkPixmap *pixmap = NULL, *opixmap = NULL;
      GdkBitmap *mask = NULL, *omask = NULL;
      GimvPrefsWinPagePrivate *priv;
      GtkTreeIter iter, parent_iter;

      if (!page || !page->path) continue;

      /* translate page title */
      title = g_basename (_(page->path));

      /* get node icon */
      get_icon (page->icon,      &pixmap,  &mask);
      get_icon (page->icon_open, &opixmap, &omask);

      /* set private data */
      priv = g_new0 (GimvPrefsWinPagePrivate, 1);
      priv->page   = page;
      priv->widget = NULL;

      priv_page_list = g_list_append (priv_page_list, priv);

      if (prefs_win_navtree_get_parent (priv, &parent_iter)) {
         gtk_tree_store_append (store, &iter, &parent_iter);
      } else {
         gtk_tree_store_append (store, &iter, NULL);
      }
      gtk_tree_store_set (store, &iter,
                          COLUMN_ICON_CLOSE,      pixmap,
                          COLUMN_ICON_CLOSE_MASK, mask,
                          COLUMN_ICON_OPEN,       opixmap,
                          COLUMN_ICON_OPEN_MASK,  omask,
                          COLUMN_NAME,            title,
                          COLUMN_PRIV_DATA,       priv,
                          COLUMN_TERMINATOR);
   }

   return tree;
}



/*******************************************************************************
 *
 *   Callback functions for preference window.
 *
 *******************************************************************************/
static void
cb_prefs_win_destroy ()
{
   if (prefs_win.ok_pressed) {
      prefs_win_free_strings (TRUE);
      conf = *config_changed;
      prefs_win_apply_config (GIMV_PREFS_WIN_ACTION_OK);
   } else {
      prefs_win_free_strings (FALSE);
      conf = *config_prechanged;
      prefs_win_apply_config (GIMV_PREFS_WIN_ACTION_CANCEL);
   }

   g_free (config_changed);
   g_free (config_prechanged);
   config_changed       = NULL;
   config_prechanged    = NULL;

   prefs_window         = NULL;
   prefs_win.notebook   = NULL;
   prefs_win.tree       = NULL;
   prefs_win.ok_pressed = FALSE;

   g_list_foreach (priv_page_list, (GFunc) g_free, NULL);
   g_list_free (priv_page_list);
   priv_page_list = NULL;

   prefs_save_config ();
}


static void
cb_dialog_response (GtkDialog *dialog, gint arg, gpointer data)
{
   switch (arg) {
   case GTK_RESPONSE_ACCEPT:
      prefs_win.ok_pressed = TRUE;
      gtk_widget_destroy (prefs_window);
      break;
   case GTK_RESPONSE_APPLY:
      memcpy (&conf, config_changed, sizeof(Config));
      prefs_win_apply_config (GIMV_PREFS_WIN_ACTION_APPLY);
      break;
   case GTK_RESPONSE_REJECT:
      gtk_widget_destroy (prefs_window);
      break;
   default:
      break;
   }
}



/*******************************************************************************
 *
 *   prefs_open_window:
 *      @ Create preference window. If already opened, raise it and return.
 *
 *   GtkWidget *prefs_window (global variable):
 *      Pointer to preference window.
 *
 *   Static PrefsWin prefs_win (local variable):
 *      store pointer to eache widget;
 *
 *******************************************************************************/
GtkWidget *
gimv_prefs_win_open (const gchar *path, GtkWindow *parent)
{
   GtkWidget *notebook, *pane, *scrolledwin, *navtree;

   /* if preference window is alredy opend, raise it and return */
   if (prefs_window) {
      gdk_window_raise (prefs_window->window);
      return prefs_window;
   }

   prefs_win.ok_pressed = FALSE;

   /* allocate buffer for new config */
   config_prechanged = g_memdup (&conf, sizeof(Config));
   config_changed = g_memdup (&conf, sizeof(Config));

   /* create config window */
   prefs_window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (prefs_window), parent);
   gtk_window_set_wmclass(GTK_WINDOW(prefs_window), "prefs", GIMV_PROG_NAME);
   gtk_window_set_default_size (GTK_WINDOW(prefs_window), 600, 450);
   gtk_window_set_title (GTK_WINDOW (prefs_window), _("Preference")); 
   gtk_signal_connect (GTK_OBJECT(prefs_window), "destroy",
                       GTK_SIGNAL_FUNC(cb_prefs_win_destroy), NULL);

   /* pane */
   pane = gtk_hpaned_new ();
   gtk_container_set_border_width (GTK_CONTAINER (pane), 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_window)->vbox), 
                       pane, TRUE, TRUE, 0);
   gtk_widget_show (pane);

   notebook = prefs_win.notebook = gtk_notebook_new ();
   gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
   gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
   /* gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook)); */
   gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
   gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
   gtk_widget_show (notebook);

   /* scrolled window */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
                                       GTK_SHADOW_IN);
   gtk_widget_set_usize (scrolledwin, 170, -1);
   gtk_widget_show (scrolledwin);

   /* navigation tree */
   navtree = prefs_win_create_navtree ();
   gtk_container_add (GTK_CONTAINER (scrolledwin), navtree);
   gtk_widget_show (navtree);

   gtk_paned_add1 (GTK_PANED (pane), scrolledwin);
   gtk_paned_add2 (GTK_PANED (pane), notebook);

   /* button */
   gtk_dialog_add_buttons (GTK_DIALOG (prefs_window),
                           GTK_STOCK_APPLY,  GTK_RESPONSE_APPLY,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                           GTK_STOCK_OK,     GTK_RESPONSE_ACCEPT,
                           NULL);
   gtk_signal_connect_object (GTK_OBJECT (prefs_window), "response",
                              GTK_SIGNAL_FUNC (cb_dialog_response),
                              NULL);

   gtk_window_set_position (GTK_WINDOW (prefs_window), GTK_WIN_POS_CENTER);
   gtk_widget_show (prefs_window);
   gimv_icon_stock_set_window_icon (prefs_window->window, "prefs");

   prefs_win_set_page (NULL);

   return prefs_window;
}


static GtkWindow *auauauau_window = NULL;

static gboolean
idle_prefs_win_open (gpointer data)
{
   gchar *path = data;
   gimv_prefs_win_open (path, auauauau_window);
   auauauau_window = NULL;
   g_free (path);
   return FALSE;
}


void
gimv_prefs_win_open_idle (const gchar *path, GtkWindow *parent)
{
   gchar *str = NULL;

   if (path)
      str = g_strdup (path);

   auauauau_window = parent;

   gtk_idle_add (idle_prefs_win_open, str);
}


gboolean
gimv_prefs_win_is_opened (void)
{
   if (prefs_window)
      return TRUE;
   else
      return FALSE;
}


GtkWidget *
gimv_prefs_win_get (void)
{
   return prefs_window;
}
