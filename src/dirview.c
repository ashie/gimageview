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
 * These codes are mostly taken from G-thumB.
 * G-thumB code Copyright (C) 1999-2000 MIZUNO Takehiko <mizuno@bigfoot.com>
 */

#include "dirview.h"

#ifndef ENABLE_TREEVIEW

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>

#include "gimageview.h"
#include "charset.h"
#include "dirview_priv.h"
#include "dnd.h"
#include "fileload.h"
#include "fileutil.h"
#include "gfileutil.h"
#include "gtk2-compat.h"
#include "gtk_prop.h"
#include "gtkutils.h"
#include "gimv_icon_stock.h"
#include "gimv_thumb_win.h"
#include "menu.h"
#include "prefs.h"


/* for auto-scroll and auto-expand at drag */
#define SCROLL_EDGE 20
typedef gboolean (*desirable_fn) (DirView *dv, gint x, gint y);
typedef gboolean (*scroll_fn)    (gpointer data);


typedef enum {
   DirNodeScannedFlag = 1 << 0,
   DirNodeOpenedFlag  = 1 << 1
} DirNodeFlags;


typedef struct DirNode_Tag
{
   DirNodeFlags flags;
   gboolean scanned;
   gchar *path;
} DirNode;


/* private functions */
static gint          comp_func_dirnode         (gconstpointer   node_data,
                                                gconstpointer   comp_data);
static GtkCTreeNode *dirview_find_directory    (DirView        *dv,
                                                const gchar    *path);

static void     dirview_create_ctree           (DirView        *dv);
static void     adjust_ctree                   (GtkWidget      *widget,
                                                GtkCTreeNode   *node);
GtkCTreeNode   *dirview_insert_dummy_node      (DirView        *dv,
                                                GtkCTreeNode   *parent);
GtkCTreeNode   *dirview_insert_node            (DirView        *dv,
                                                GtkCTreeNode   *parent,
                                                const gchar    *path);
static void     expand_dir                     (DirView        *dv,
                                                GtkCTreeNode   *parent);
static GList   *get_expanded_list              (DirView        *dv);
static void     refresh_dir_tree               (DirView        *dv,
                                                GtkCTreeNode   *parent);
static void     dirview_popup_menu             (DirView        *dv,
                                                GtkCTreeNode   *node,
                                                GdkEventButton *event);
static gboolean dirview_button_action          (DirView        *dv,
                                                GdkEventButton *event,
                                                gint            num);

/* for drag motion */
static void     dirview_setup_drag_scroll      (DirView        *dv,
                                                gint            x,
                                                gint            y,
                                                desirable_fn    desirable,
                                                scroll_fn       scroll);
static void     dirview_cancel_drag_scroll     (DirView        *dv);
static gboolean dirview_scrolling_is_desirable (DirView        *dv,
                                                gint            x,
                                                gint            y);
static gboolean timeout_dirview_scroll         (gpointer        data);
static gint     timeout_auto_expand_directory  (gpointer        data);


/* callback functions */
static void     cb_dirtree_destroy             (GtkWidget      *widget,
                                                DirView        *dv);
static void     cb_dirview_destroyed           (GtkWidget      *widget,
                                                DirView        *dv);
static void     cb_node_destroy                (gpointer        data);
static void     cb_expand                      (GtkWidget      *widget,
                                                GtkCTreeNode   *parent,
                                                DirView        *dv);
static void     cb_collapse                    (GtkWidget      *widget,
                                                GtkCTreeNode   *parent,
                                                DirView        *dv);
static gboolean  cb_button_press               (GtkWidget      *widget,
                                                GdkEventButton *event,
                                                DirView        *dv);
static gboolean  cb_button_release             (GtkWidget      *widget,
                                                GdkEventButton *event,
                                                DirView        *dv);
static gboolean cb_key_press                   (GtkWidget      *widget,
                                                GdkEventKey    *event,
                                                DirView        *dv);

/* callback functions for popup menu */
static void     cb_open_thumbnail              (DirView        *dv,
                                                ScanSubDirType  scan_subdir,
                                                GtkWidget      *menuitem);
static void     cb_go_to_here                  (DirView        *dv,
                                                guint           action,
                                                GtkWidget      *menuitem);
static void     cb_refresh_dir_tree            (DirView        *dv,
                                                guint           action,
                                                GtkWidget      *menuitem);
static void     cb_file_property               (DirView        *tv,
                                                guint           action,
                                                GtkWidget      *menuitem);
static void     cb_mkdir                       (DirView        *dv,
                                                guint           action,
                                                GtkWidget      *menuitem);
static void     cb_rename_dir                  (DirView        *dv,
                                                guint           action,
                                                GtkWidget      *menuitem);
static void     cb_delete_dir                  (DirView        *dv,
                                                guint           action,
                                                GtkWidget      *menuitem);

/* callback functions for toolbar buttons */
static void     cb_home_button                 (GtkWidget      *widget,
                                                DirView        *dv);
static void     cb_up_button                   (GtkWidget      *widget,
                                                DirView        *dv);
static void     cb_refresh_button              (GtkWidget      *widget,
                                                DirView        *dv);
static void     cb_dotfile_button              (GtkWidget      *widget,
                                                DirView        *dv);

/* callback functions for DnD */
static void     cb_drag_motion                 (GtkWidget      *dirtree,
                                                GdkDragContext *context,
                                                gint            x,
                                                gint            y,
                                                gint            time,
                                                gpointer        data);
static void     cb_drag_leave                  (GtkWidget      *dirtree,
                                                GdkDragContext *context,
                                                guint           time,
                                                gpointer        data);
static void     cb_drag_data_get               (GtkWidget      *widget,
                                                GdkDragContext *context,
                                                GtkSelectionData *seldata,
                                                guint info,
                                                guint time,
                                                gpointer data);
static void     cb_drag_data_received          (GtkWidget      *dirtree,
                                                GdkDragContext *context,
                                                gint            x,
                                                gint            y,
                                                GtkSelectionData *seldata,
                                                guint           info,
                                                guint32         time,
                                                gpointer        data);
static void     cb_drag_end                    (GtkWidget      *dirtree,
                                                GdkDragContext *context,
                                                gpointer        data);
static void     cb_com_swap_drag_data_received (GtkWidget      *widget,
                                                GdkDragContext *context,
                                                gint            x,
                                                gint            y,
                                                GtkSelectionData *seldata,
                                                guint           info,
                                                guint           time,
                                                gpointer        data);


static GtkItemFactoryEntry dirview_popup_items [] =
{
   {N_("/_Load Thumbnail"),                        NULL, cb_open_thumbnail, SCAN_SUB_DIR_NONE,     NULL},
   {N_("/Load Thumbnail re_cursively"),            NULL, cb_open_thumbnail, SCAN_SUB_DIR,          NULL},
   {N_("/Load Thumbnail recursively in _one tab"), NULL, cb_open_thumbnail, SCAN_SUB_DIR_ONE_TAB,  NULL},
   {N_("/---"),                         NULL, NULL,                0,     "<Separator>"},
   {N_("/_Go to here"),                 NULL, cb_go_to_here,       0,     NULL},
   {N_("/_Refresh Tree"),               NULL, cb_refresh_dir_tree, 0,     NULL},
   {N_("/---"),                         NULL, NULL,                0,     "<Separator>"},
   {N_("/_Property..."),                NULL, cb_file_property,    0,     NULL},
   {N_("/---"),                         NULL, NULL,                0,     "<Separator>"},
   {N_("/_Make Directory..."),          NULL, cb_mkdir,            0,     NULL},
   {N_("/Re_name Directory..."),        NULL, cb_rename_dir,       0,     NULL},
   {N_("/_Delete Directory..."),        NULL, cb_delete_dir,       0,     NULL},
   {NULL, NULL, NULL, 0, NULL},
};


extern GtkItemFactoryEntry dnd_file_popup_items [];


/******************************************************************************
 *
 *   Private functions
 *
 ******************************************************************************/
static gint
comp_func_dirnode (gconstpointer node_data, gconstpointer comp_data)
{
   const DirNode *dirnode = node_data;
   const gchar *node_path;
   const gchar *comp_path = comp_data;

   g_return_val_if_fail (comp_path && *comp_path, -1);

   if (!dirnode) return -1;

   node_path = dirnode->path;
   if (!node_path || !*node_path) return -1;

   return strcmp (node_path, comp_path);
}


static GtkCTreeNode *
dirview_find_directory (DirView *dv, const gchar *path)
{
   GtkCTreeNode *parent, *node;

   g_return_val_if_fail (dv, NULL);
   g_return_val_if_fail (path && path, NULL);

   parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
   if (!parent) return NULL;
   node = gtk_ctree_find_by_row_data_custom (GTK_CTREE (dv->dirtree),
                                             parent,
                                             (gpointer) path,
                                             comp_func_dirnode);
   return node;
}


/*
 *  dirview_create_ctree:
 *     @ create a new GtkCTree widget.
 *
 *  dv : Pointer to the DirView struct.
 */
static void
dirview_create_ctree (DirView *dv)
{
   GtkWidget *dirtree;
   GtkCTreeNode *root_node;
   gchar *root_dir;

   g_return_if_fail (dv);

   root_dir = add_slash (dv->root_dir);
   g_free (dv->root_dir);
   dv->root_dir = root_dir;

   dirtree = dv->dirtree = gtk_ctree_new (1,0);
   gtk_clist_set_column_auto_resize (GTK_CLIST (dirtree), 0, TRUE);
   gtk_clist_set_selection_mode (GTK_CLIST (dirtree), GTK_SELECTION_BROWSE);
   gtk_ctree_set_line_style (GTK_CTREE (dirtree), conf.dirview_line_style);
   gtk_ctree_set_expander_style (GTK_CTREE (dirtree), conf.dirview_expander_style);
   gtk_clist_set_row_height (GTK_CLIST (dirtree), 18);

   gtk_container_add (GTK_CONTAINER (dv->scroll_win), dirtree);

   gtk_signal_connect (GTK_OBJECT (dirtree), "tree_expand",
                       GTK_SIGNAL_FUNC (cb_expand), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree), "tree_collapse",
                       GTK_SIGNAL_FUNC (cb_collapse), dv);

   gtk_signal_connect (GTK_OBJECT (dirtree),"button_press_event",
                       GTK_SIGNAL_FUNC (cb_button_press), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree),"button_release_event",
                       GTK_SIGNAL_FUNC (cb_button_release), dv);

   gtk_signal_connect (GTK_OBJECT(dirtree), "key-press-event",
                       GTK_SIGNAL_FUNC (cb_key_press), dv);

   gtk_signal_connect (GTK_OBJECT (dv->dirtree), "destroy",
                       GTK_SIGNAL_FUNC (cb_dirtree_destroy), dv);

   /* for drag file list */
   dnd_src_set  (dirtree, dnd_types_uri, dnd_types_uri_num);
   dnd_dest_set (dirtree, dnd_types_archive, dnd_types_archive_num);
   gtk_signal_connect (GTK_OBJECT (dirtree), "drag_motion",
                       GTK_SIGNAL_FUNC (cb_drag_motion), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree), "drag_leave",
                       GTK_SIGNAL_FUNC (cb_drag_leave), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree), "drag_data_get",
                       GTK_SIGNAL_FUNC (cb_drag_data_get), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree), "drag_data_received",
                       GTK_SIGNAL_FUNC (cb_drag_data_received), dv);
   gtk_signal_connect (GTK_OBJECT (dirtree), "drag_end",
                       GTK_SIGNAL_FUNC (cb_drag_end), dv);

   root_node = dirview_insert_node (dv, NULL, dv->root_dir);
   if (root_dir)
      gtk_ctree_expand (GTK_CTREE (dirtree), root_node);

   gtk_widget_show (dirtree);
}


/*
 *  adjust_ctree:
 *     @ set position of viewport
 *
 *  dirtree : ctree widget.
 *  parent  : ctree node that contain direcotry to expand.
 */
static void
adjust_ctree (GtkWidget *dirtree, GtkCTreeNode *node)
{
   gint row = GTK_CLIST (dirtree)->rows - g_list_length ((GList*)node);

   GTK_CLIST (dirtree)->focus_row = row;
   gtk_ctree_select (GTK_CTREE (dirtree), node);

   gtk_clist_freeze (GTK_CLIST (dirtree));
   if (gtk_ctree_node_is_visible (GTK_CTREE (dirtree), node) != GTK_VISIBILITY_FULL)
      gtk_ctree_node_moveto (GTK_CTREE (dirtree), node, 0, 0, 0);
   gtk_clist_thaw (GTK_CLIST (dirtree));
}


/*
 *  dirview_insert_dummy_node:
 *     @ insert dummy node to display expander.
 *
 *  dv     : Pointer to the Dir View struct.
 *  parnt  : Parent node to insert dummy node.
 *  Return : Pointer to the new GtkCTreeNode.
 */
GtkCTreeNode *
dirview_insert_dummy_node (DirView *dv, GtkCTreeNode *parent)
{
   GtkCTreeNode *node = NULL;
   gchar *dummy = "dummy", *node_text;
   DirNode *dirdata;
   gboolean is_leaf, expanded;

   dirdata = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), parent);
   gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), parent, &node_text, NULL,
                            NULL, NULL, NULL, NULL, &is_leaf, &expanded);

   if (!expanded && !(dirdata->flags & DirNodeScannedFlag)
       && !GTK_CTREE_ROW (parent)->children
       && strcmp (node_text, ".") && strcmp (node_text, ".."))
   {
      gtk_ctree_insert_node (GTK_CTREE (dv->dirtree),
                             parent, NULL, &dummy,
                             4, NULL, NULL, NULL, NULL, FALSE, FALSE);
   }

   return node;
}


/*
 *  dirview_insert_node:
 *     @ insert new node to the dirview.
 *
 *  dv     : Pointer to the Dir View struct.
 *  parnt  : Parent node to insert new node.
 *  path   : Dir name to insert.
 *  Return : Pointer to the new GtkCTreeNode.
 */
GtkCTreeNode *
dirview_insert_node (DirView *dv, GtkCTreeNode *parent, const gchar *path)
{
   GtkCTreeNode *node = NULL;
   DirNode *dirnode;
   GimvIcon *folder, *ofolder;
   gchar *dir, *internal_str;
   const gchar *filename, *text;

   g_return_val_if_fail (dv && path, NULL);

   dir = remove_slash (path);
   filename = g_basename (dir);

   if (isdir (path)
       && (!(!dv->show_dotfile && (filename[0] == '.'))
           || (conf.dirview_show_current_dir && !strcmp (filename, "."))
           || (conf.dirview_show_parent_dir  && !strcmp (filename, ".."))
           || !parent))
   {
      dirnode = g_new0 (DirNode, 1);
      dirnode->flags = 0;
      if (!strcmp(filename, ".") || !strcmp(filename, "..")) {
         gchar *tmpstr = relpath2abs (path);
         dirnode->path = add_slash (tmpstr);
         g_free (tmpstr);
      } else {
         dirnode->path = add_slash (path);
      }

      if (parent)
         text = filename;
      else
         text = path;

      if (islink (path))
         folder = gimv_icon_stock_get_icon ("folder-link");
      else if (!strcmp (filename, "."))
         folder = gimv_icon_stock_get_icon ("folder-go");
      else if (!strcmp (filename, ".."))
         folder = gimv_icon_stock_get_icon ("folder-up");
      else if (access(path, R_OK))
         folder = gimv_icon_stock_get_icon ("folder-lock");
      else
         folder = gimv_icon_stock_get_icon ("folder");

      if (islink (path))
         ofolder = gimv_icon_stock_get_icon ("folder-link-open");
      else
         ofolder = gimv_icon_stock_get_icon ("folder-open");

      internal_str = charset_to_internal (text, conf.charset_filename,
                                          conf.charset_auto_detect_fn,
                                          conf.charset_filename_mode);

      node = gtk_ctree_insert_node (GTK_CTREE (dv->dirtree), parent,
                                    NULL, (gchar **) &internal_str, 4,
                                    folder->pixmap, folder->mask,
                                    ofolder->pixmap, ofolder->mask,
                                    FALSE, FALSE);

      g_free (internal_str);

      gtk_ctree_node_set_row_data_full (GTK_CTREE (dv->dirtree), node,
                                        dirnode, cb_node_destroy);

      /* set expander */
      if (!access(path, R_OK))
         dirview_insert_dummy_node (dv, node);

      /* set mark */
      if (gimv_thumb_view_find_opened_dir (dirnode->path)) {
         dirview_set_opened_mark (dv, dirnode->path);
      }
   }

   g_free (dir);

   return node;
}


/*
 *  expand_dir:
 *     @ Expand specified direcotry tree.
 *
 *  dv      : Pointer to the DirView struct.
 *  parent  : ctree node that contain direcotry to expand.
 */
static void
expand_dir (DirView *dv, GtkCTreeNode *parent)
{
   GtkWidget *dirtree;
   DirNode *parent_dirnode;
   DIR *dir;
   struct dirent *dirent;
   gchar *path;

   g_return_if_fail (dv);

   dirtree = dv->dirtree;
   if (!dirtree) return;

   parent_dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(dirtree), parent);

   dir = opendir (parent_dirnode->path);
   if (dir) {
      while ((dirent = readdir(dir))) {
         path = g_strconcat (parent_dirnode->path, dirent->d_name, NULL);
         if (isdir (path))
            dirview_insert_node (dv, parent, path);
         g_free (path);
      }
      closedir (dir);
      gtk_ctree_sort_node (GTK_CTREE(dirtree), parent);
   }

   parent_dirnode->flags |= DirNodeScannedFlag;
}


GList *
get_expanded_list (DirView *dv)
{
   GList *list = NULL;
   GtkCTreeNode *node;
   gchar *text;
   gboolean is_leaf, expanded;
   DirNode *dirnode;

   g_return_val_if_fail (dv, NULL);
   g_return_val_if_fail (dv->dirtree, NULL);

   node = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
   for (; node; node = GTK_CTREE_NODE_NEXT (node)) {
      gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, &text, NULL,
                               NULL, NULL, NULL, NULL, &is_leaf, &expanded);

      if (expanded) {
         dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);
         list = g_list_append (list, g_strdup (dirnode->path));
      }
   }

   return list;
}


/*
 *  refresh_dir_tree:
 *     @ Refresh sub direcotry list.
 *
 *  dv      : Pointer to the DirView struct.
 *  parent  : ctree node that contain direcotry to refresh.
 */
static void
refresh_dir_tree (DirView *dv,  GtkCTreeNode *parent)
{
   GList *expand_list, *node;
   GtkCTreeNode *treenode;
   gchar *root_dir, *selected_path;

   g_return_if_fail (dv);

   selected_path = dirview_get_selected_path (dv);
   if (!selected_path) selected_path = g_strdup (dv->root_dir);

   /* get expanded directory list */
   expand_list = get_expanded_list (dv);

   /* replace root node */
   root_dir = g_strdup (dv->root_dir);
   dirview_change_root (dv, root_dir);
   g_free (root_dir);

   /* restore expanded directory */
   for (node = expand_list; node; node = g_list_next (node)) {
      dirview_expand_dir (dv, node->data, FALSE);
   }

   /* restore selection */
   treenode = dirview_find_directory (dv, selected_path);
   if (treenode)
      adjust_ctree (dv->dirtree, treenode);

   g_list_foreach (expand_list, (GFunc) g_free, NULL);
   g_list_free (expand_list);

   g_free (selected_path);
}


static void
dirview_popup_menu (DirView *dv, GtkCTreeNode *node,  GdkEventButton *event)
{
   GtkWidget *dirview_popup, *menuitem;
   GtkItemFactory *ifactory;
   DirNode *dirnode;
   gint n_menu_items;
   gchar *path, *parent, *node_text;
   gboolean expanded, is_leaf;
   GtkMenuPositionFunc pos_fn = NULL;
   guint button;
   guint32 time;

   g_return_if_fail (dv);
   g_return_if_fail (dv->dirtree);

   if (!node) {
      GList *sel_list;
      sel_list = GTK_CLIST (dv->dirtree)->selection;
      if (!sel_list) return;

      node = sel_list->data;
   }

   if (event) {
      button = event->button;
      time = event->time;
   } else {
      button = 0;
      time = GDK_CURRENT_TIME;
      pos_fn = menu_calc_popup_position;
   }

   gtk_ctree_get_node_info (GTK_CTREE (dv->dirtree), node, &node_text, NULL,
                            NULL, NULL, NULL, NULL, &is_leaf, &expanded);
   dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(dv->dirtree), node);

   if (dv->popup_menu) {
      gtk_widget_unref (dv->popup_menu);
      dv->popup_menu = NULL;
   }

   n_menu_items = sizeof(dirview_popup_items)
      / sizeof(dirview_popup_items[0]) -  1;
   dirview_popup = menu_create_items(NULL, dirview_popup_items,
                                     n_menu_items, "<DirViewPop>", dv);

   /* set sensitive */
   ifactory = gtk_item_factory_from_widget (dirview_popup);

   if (!strcmp (node_text, ".") || !strcmp (node_text, ".."))
   {
      menuitem = gtk_item_factory_get_item (ifactory, "/Refresh Tree");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Make Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   if (!iswritable (dirnode->path)) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Make Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   path = g_strdup (dirnode->path);
   if (path [strlen (path) - 1] == '/')
      path [strlen (path) - 1] = '\0';
   parent = g_dirname (path);

   if (!parent || !strcmp (parent, ".") || !iswritable (parent)
       || !strcmp (node_text, ".") || !strcmp (node_text, ".."))
   {
      menuitem = gtk_item_factory_get_item (ifactory, "/Rename Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Delete Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   g_free (path);
   g_free (parent);

   /* popup menu */
   gtk_menu_popup(GTK_MENU (dirview_popup), NULL, NULL,
                  pos_fn, dv->dirtree->window, button, time);

   dv->popup_menu = dirview_popup;

#ifdef USE_GTK2
   gtk_object_ref (GTK_OBJECT (dv->popup_menu));
   gtk_object_sink (GTK_OBJECT (dv->popup_menu));
#endif
}


typedef struct ButtonActionData_Tag
{
   DirView *dv;
   gchar   *path, *label;
   gint     action_num;
} ButtonActionData;


static void
free_button_action_data (ButtonActionData *data)
{
   g_free (data->path);
   g_free (data->label);
   g_free (data);
}


static gboolean
idle_dirview_button_action (gpointer data)
{
   ButtonActionData *bdata = data;
   DirView *dv = bdata->dv;
   gchar *path = bdata->path, *label = bdata->label;

   dv->priv->button_action_id = 0;

   switch (abs (bdata->action_num)) {
   case MouseActLoadThumb:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         dirview_change_root (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR_NONE);
      }
      break;
   case MouseActLoadThumbRecursive:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         dirview_change_root (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR);
      }
      break;
   case MouseActLoadThumbRecursiveInOneTab:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         dirview_change_root (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR_ONE_TAB);
      }
      break;
   case MouseActChangeTop:
      dirview_change_root (dv, path);
      break;
   default:
      break;
   }

   free_button_action_data (bdata);

   return FALSE;
}


static gboolean
dirview_button_action (DirView *dv, GdkEventButton *event, gint num)
{
   GimvThumbWin *tw;
   GtkWidget *dirtree;
   GtkCTreeNode *node;
   DirNode *dirnode;
   gchar *node_text;
   gint row;
   gboolean is_leaf, expanded, retval = FALSE;

   g_return_val_if_fail (dv, FALSE);

   dirtree = dv->dirtree;
   tw = dv->tw;

   gtk_clist_get_selection_info(GTK_CLIST(dirtree),
                                event->x, event->y, &row, NULL);
   node = gtk_ctree_node_nth(GTK_CTREE(dirtree), row);
   if (!node) return FALSE;

   gtk_ctree_get_node_info (GTK_CTREE (dirtree), node, &node_text, NULL,
                            NULL, NULL, NULL, NULL, &is_leaf, &expanded);

   gtk_clist_freeze(GTK_CLIST(dirtree));
   gtk_ctree_select(GTK_CTREE(dirtree), node);
   GTK_CLIST(dirtree)->focus_row = row;
   gtk_clist_thaw(GTK_CLIST(dirtree));
   dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(dirtree), node);

   switch (abs(num)) {
   case MouseActLoadThumb:
   case MouseActLoadThumbRecursive:
   case MouseActLoadThumbRecursiveInOneTab:
   case MouseActChangeTop:
   {
      ButtonActionData *data = g_new0 (ButtonActionData, 1);

      data->dv         = dv;
      data->path       = g_strdup (dirnode->path);
      data->label      = g_strdup (node_text);
      data->action_num = num;

      dv->priv->button_action_id
         = gtk_idle_add (idle_dirview_button_action, data);

      return FALSE;
      break;
   }
   case MouseActPopupMenu:
      dirview_popup_menu (dv, node, event);
      if (num > 0) retval = TRUE;
      break;
   default:
      break;
   }

   return retval;
}



/******************************************************************************
 *
 *   for drag motion related functions
 *
 ******************************************************************************/
/*
 *  dirview_setup_drag_scroll:
 *     @ set timer for auto scroll.
 *
 *  dv        : Pointer to the DirView struct.
 *  x         : X position of mouse cursor.
 *  y         : Y positoon of mouse cursor.
 *  desirable :
 *  scroll    : time out function for auto scroll.
 */
static void
dirview_setup_drag_scroll (DirView *dv, gint x, gint y,
                           desirable_fn desirable, scroll_fn scroll)
{
   dirview_cancel_drag_scroll (dv);

   dv->priv->drag_motion_x = x;
   dv->priv->drag_motion_y = y;

   if ((* desirable) (dv, x, y))
      dv->priv->scroll_timer_id
         = gtk_timeout_add (conf.dirview_auto_scroll_time,
                            scroll, dv);
}


/*
 *  dirview_cancel_drag_scroll:
 *     @ remove timer for auto scroll..
 *
 *  dv : Pointer to the DirView struct.
 */
static void
dirview_cancel_drag_scroll (DirView *dv)
{
   g_return_if_fail (dv);

   if (dv->priv->scroll_timer_id){
      gtk_timeout_remove (dv->priv->scroll_timer_id);
      dv->priv->scroll_timer_id = 0;
   }
}


/*
 *  dirview_scrolling_is_desirable:
 *     @ If the cursor is in a position close to either edge (top or bottom)
 *     @ and there is possible to scroll the window, this routine returns
 *     @ true.
 *
 *  dv     : Pointer to the DirView struct for scrolling.
 *  x      : X position of mouse cursor.
 *  y      : Y position of mouse cursor.
 *  Return : possiblilty of scrolling (TRUE or FALSE).
 */
static gboolean
dirview_scrolling_is_desirable (DirView *dv, gint x, gint y)
{
   GtkCTree *dirtree;
   GtkAdjustment *vadj;

   dirtree = GTK_CTREE (dv->dirtree);

   vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (dv->scroll_win));

   if (y < SCROLL_EDGE) {
      if (vadj->value > vadj->lower)
         return TRUE;
   } else {
      if (y > (vadj->page_size - SCROLL_EDGE)){
         if (vadj->value < vadj->upper - vadj->page_size)
            return TRUE;
      }
   }

   return FALSE;
}


/*
 *  timeout_dirview_scroll:
 *     @ Timer callback to scroll the tree.
 *
 *  data   : Pointer to the DirView struct.
 *  Return : TRUE
 */
static gboolean
timeout_dirview_scroll (gpointer data)
{
   DirView *dv = data;
   GtkAdjustment *vadj;
   gfloat vpos;

   vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (dv->scroll_win));

   if (dv->priv->drag_motion_y < SCROLL_EDGE) {
      vpos = vadj->value - vadj->step_increment;
      if (vpos < vadj->lower)
         vpos = vadj->lower;

      gtk_adjustment_set_value (vadj, vpos);
   } else {
      vpos = vadj->value + vadj->step_increment;
      if (vpos > vadj->upper - vadj->page_size)
         vpos = vadj->upper - vadj->page_size;

      gtk_adjustment_set_value (vadj, vpos);
   }

   return TRUE;
}


/*
 *  timeout_auto_expand_directory:
 *     @ This routine is invoked in a delayed fashion if the user
 *     @ keeps the drag cursor still over the widget.
 *
 *  data   : Pointer to the DirView struct.
 *  Retuan : FALSE
 */
static gint
timeout_auto_expand_directory (gpointer data)
{
   DirView *dv = data;
   GtkCTreeNode *node;

   g_return_val_if_fail (dv, FALSE);

   node = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree),
                              dv->priv->drag_tree_row);
   if (!node) {
      dv->priv->auto_expand_timeout_id = 0;
      return FALSE;
   }

   if (!GTK_CTREE_ROW (node)->expanded) {
      gtk_ctree_expand (GTK_CTREE (dv->dirtree), node);
   }

   return FALSE;
}



/******************************************************************************
 *
 *   Callback functions
 *
 ******************************************************************************/
static void
cb_dirtree_destroy (GtkWidget *widget, DirView *dv)
{
   g_return_if_fail (dv);
   
   dv->dirtree = NULL;
}


static void
cb_dirview_destroyed (GtkWidget *widget, DirView *dv)
{
   g_return_if_fail (dv);

   dirview_cancel_drag_scroll (dv);

   if (dv->priv->button_action_id)
      gtk_idle_remove (dv->priv->button_action_id);
   dv->priv->button_action_id = 0;

   if (dv->priv->swap_com_id)
      gtk_idle_remove (dv->priv->swap_com_id);
   dv->priv->swap_com_id = 0;

   if (dv->priv->change_root_id)
      gtk_idle_remove (dv->priv->change_root_id);
   dv->priv->change_root_id = 0;

   gtk_widget_destroy (dv->dirtree);

   g_free (dv->root_dir);
   g_free (dv->priv);
   g_free (dv);
}


static void
cb_node_destroy (gpointer data)
{
   DirNode *node = data;
   g_free (node->path);
   g_free (node);
}


static void
cb_expand (GtkWidget *dirtree, GtkCTreeNode *parent, DirView *dv)
{
   GtkCTreeNode *node;
   DirNode *dirnode;

   g_return_if_fail (dv);

   if (!GTK_CTREE_ROW (parent)->children) return;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dirtree), parent);

   if (!(dirnode->flags & DirNodeScannedFlag)) {
      gtk_clist_freeze (GTK_CLIST (dirtree));

      node = gtk_ctree_find_by_row_data (GTK_CTREE (dirtree), parent, NULL);
      gtk_ctree_remove_node (GTK_CTREE (dirtree), node);

      expand_dir (dv, parent);

      gtk_clist_thaw (GTK_CLIST (dirtree));
   }
}


static void
cb_collapse (GtkWidget *dirtree, GtkCTreeNode *parent, DirView *dv)
{
   g_return_if_fail (dv && parent);
}


static gboolean
cb_button_press (GtkWidget *widget, GdkEventButton *event, DirView *dv)
{
   gint num;

   g_return_val_if_fail (dv, TRUE);
   g_return_val_if_fail (event, TRUE);

   num = prefs_mouse_get_num_from_event (event, conf.dirview_mouse_button);
   if (event->type == GDK_2BUTTON_PRESS) {
      dv->priv->button_2pressed_queue = num;
   } else if (num > 0) {
      return dirview_button_action (dv, event, num);
   }

   return FALSE;
}


static gboolean
cb_button_release (GtkWidget *widget, GdkEventButton *event, DirView *dv)
{
   gint num;

   g_return_val_if_fail (dv, FALSE);
   g_return_val_if_fail (event, FALSE);

   if (dv->priv->button_2pressed_queue) {
      num = dv->priv->button_2pressed_queue;
      if (num > 0)
         num = 0 - num;
      dv->priv->button_2pressed_queue = 0;
   } else {
      num = prefs_mouse_get_num_from_event (event, conf.dirview_mouse_button);
   }
   if (num < 0)
      return dirview_button_action (dv, event, num);

   return FALSE;
}


static gboolean
cb_key_press (GtkWidget *widget, GdkEventKey *event, DirView *dv)
{
   guint keyval, popup_key;
   GdkModifierType modval, popup_mod;
   GtkCTreeNode *node;
   GList *sel_list;
   DirNode *dirnode;
   gchar *node_text;
   gboolean expanded, is_leaf;

   g_return_val_if_fail (dv, FALSE);

   keyval = event->keyval;
   modval = event->state;

   sel_list = GTK_CLIST (dv->dirtree)->selection;
   if (!sel_list) return FALSE;
   node = sel_list->data;
   gtk_ctree_get_node_info (GTK_CTREE (widget), node, &node_text, NULL,
                            NULL, NULL, NULL, NULL, &is_leaf, &expanded);

   dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(widget), node);
   if (!dirnode) return FALSE;

   if (akey.common_popup_menu || *akey.common_popup_menu)
      gtk_accelerator_parse (akey.common_popup_menu, &popup_key, &popup_mod);
   else
      return FALSE;

   if (keyval == popup_key && (!popup_mod || (modval & popup_mod))) {
      dirview_popup_menu (dv, NULL, NULL);
   } else {
      switch (keyval) {
      case GDK_KP_Enter:
      case GDK_Return:
      case GDK_ISO_Enter:
         if (!strcmp (node_text, ".") || !strcmp (node_text, "..")) {
            dirview_change_root (dv, dirnode->path);
         } else {
            open_dir_images (dirnode->path, dv->tw, NULL,
                             LOAD_CACHE, conf.scan_dir_recursive);
         }
         break;
      case GDK_space:
         gtk_ctree_toggle_expansion (GTK_CTREE (widget), node);
         break;
      case GDK_Right:
         if (modval & GDK_CONTROL_MASK) {
            dirview_change_root (dv, dirnode->path);
         } else {
            gtk_ctree_expand (GTK_CTREE (widget), node);
         }
         return TRUE;
         break;
      case GDK_Left:
         if (modval & GDK_CONTROL_MASK) {
            dirview_change_root_to_parent (dv);
         } else {
            gtk_ctree_collapse (GTK_CTREE (widget), node);
         }
         return TRUE;
         break;
      case GDK_Up:
         if (modval & GDK_CONTROL_MASK)
            dirview_change_root_to_parent (dv);
         return TRUE;
         break;
      case GDK_Down:
         if (modval & GDK_CONTROL_MASK)
            dirview_change_root (dv, dirnode->path);
         return TRUE;
      }
   }

   return FALSE;
}



/******************************************************************************
 *
 *   Callback functions for popup menu
 *
 ******************************************************************************/
static void
cb_open_thumbnail (DirView *dv, ScanSubDirType scan_subdir, GtkWidget *menuitem)
{
   gchar *path;

   path = dirview_get_selected_path (dv);
   if (!path) return;

   open_dir_images (path, dv->tw, NULL, LOAD_CACHE, scan_subdir);

   g_free (path);
}


static void
cb_go_to_here (DirView *dv, guint action, GtkWidget *menuitem)
{
   gchar *path;

   path = dirview_get_selected_path (dv);
   if (!path) return;

   dirview_change_root (dv, path);

   g_free (path);
}


static void
cb_refresh_dir_tree (DirView *dv, guint action, GtkWidget *menuitem)
{
   refresh_dir_tree (dv, NULL);
}

static void
cb_file_property (DirView *dv, guint action, GtkWidget *menuitem)
{
   GimvImageInfo *info;
   gchar *path, *tmpstr;

   g_return_if_fail (dv);

   tmpstr = dirview_get_selected_path (dv);
   if (!tmpstr) return;

   path = remove_slash (tmpstr);
   g_free(tmpstr);

   info = gimv_image_info_get (path);
   if (!info) {
      g_free (path);
      return;
   }

   dlg_prop_from_image_info (info, 0);

   gimv_image_info_unref (info);
   g_free (path);
}


static void
cb_mkdir (DirView *dv, guint action, GtkWidget *menuitem)
{
   GtkCTreeNode *node;
   DirNode *dirnode;
   gboolean success;

   g_return_if_fail (dv);
   node = GTK_CLIST (dv->dirtree)->selection->data;
   if (!node) return;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE(dv->dirtree), node);
   if (!dirnode) return;

   success = make_dir_dialog
      (dirnode->path, GTK_WINDOW(gtk_widget_get_toplevel(dv->container)));

   if (success) {
      refresh_dir_tree (dv, node);
   }
}


static void
cb_rename_dir (DirView *dv, guint action, GtkWidget *menuitem)
{
   GtkCTreeNode *node, *parent;
   DirNode *dirnode;
   gboolean success;

   g_return_if_fail (dv);
   node = GTK_CLIST (dv->dirtree)->selection->data;
   if (!node) return;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE(dv->dirtree), node);
   if (!dirnode) return;

   success = rename_dir_dialog
      (dirnode->path, GTK_WINDOW(gtk_widget_get_toplevel(dv->container)));

   if (success) {
      parent = (GTK_CTREE_ROW (node))->parent;
      if (parent)
         refresh_dir_tree (dv, parent);
   }
}


static void
cb_delete_dir (DirView *dv, guint action, GtkWidget *menuitem)
{
   GtkCTreeNode *node, *parent;
   DirNode *dirnode;
   gchar *path;

   g_return_if_fail (dv);
   node = GTK_CLIST (dv->dirtree)->selection->data;
   if (!node) return;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE(dv->dirtree), node);
   if (!dirnode) return;

   path = remove_slash (dirnode->path);

   delete_dir (path, GTK_WINDOW(gtk_widget_get_toplevel(dv->container)));
   g_free (path);

   /* refresh dir tree */
   parent = (GTK_CTREE_ROW (node))->parent;
   if (parent)
      refresh_dir_tree (dv, parent);
}



/******************************************************************************
 *
 *   Callback functions for toolbar buttons.
 *
 ******************************************************************************/
static void
cb_home_button (GtkWidget *widget, DirView *dv)
{
   g_return_if_fail (dv);

   dirview_go_home (dv);
}


static void
cb_up_button (GtkWidget *widget, DirView *dv)
{
   dirview_change_root_to_parent (dv);
}


static void
cb_refresh_button (GtkWidget *widget, DirView *dv)
{
   GtkCTreeNode *parent;

   g_return_if_fail (dv);

   parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
   if (parent)
      refresh_dir_tree (dv, parent);
}


static void
cb_dotfile_button (GtkWidget *widget, DirView *dv)
{
   GtkCTreeNode *parent;

   g_return_if_fail (widget && dv);

   dv->show_dotfile = !dv->show_dotfile;

   parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
   refresh_dir_tree (dv, parent);
}



/******************************************************************************
 *
 *   Callback functions for DnD
 *
 ******************************************************************************/
static void
cb_drag_motion (GtkWidget *dirtree, GdkDragContext *context,
                gint x, gint y, gint time, gpointer data)
{
   DirView *dv = data;
   GtkCTreeNode *node;
   DirNode *node_data;
   gint row, on_row;

   g_return_if_fail (dv);

   /* Set up auto-scrolling */
   if (conf.dirview_auto_scroll)
      dirview_setup_drag_scroll (dv, x, y,
                                 dirview_scrolling_is_desirable,
                                 timeout_dirview_scroll);

   if (!GTK_CLIST (dirtree)->selection) return;

   on_row = gtk_clist_get_selection_info (GTK_CLIST (dirtree), x, y, &row, NULL);

   /* Remove the auto-expansion timeout if we are on the blank area of the
    * tree or on a row different from the previous one.
    */
   if ((!on_row || row != dv->priv->drag_tree_row)
       && dv->priv->auto_expand_timeout_id != 0)
   {
      gtk_timeout_remove (dv->priv->auto_expand_timeout_id);
      dv->priv->auto_expand_timeout_id = 0;
   }

   if (on_row) {
      node = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), row);
      node_data = gtk_ctree_node_get_row_data (GTK_CTREE (dirtree),
                                               gtk_ctree_node_nth (GTK_CTREE (dirtree),
                                                                   row));

      /* hilight row under cursor */
      if (node_data && dv->priv->hilit_dir != row) {
         if (dv->priv->hilit_dir != -1) {
            /*
              gtk_clist_set_foreground (GTK_CLIST (dirtree), dv->hilit_dir,
              &(dirtree->style->fg[GTK_STATE_NORMAL]));
            */
            gtk_clist_set_background (GTK_CLIST (dirtree), dv->priv->hilit_dir,
                                      &(dirtree->style->base[GTK_STATE_NORMAL]));
         }

         dv->priv->hilit_dir = row;

         /*
           gtk_clist_set_foreground (GTK_CLIST (dirtree), dv->hilit_dir,
           &(dirtree->style->fg[GTK_STATE_PRELIGHT]));
         */
         gtk_clist_set_background (GTK_CLIST (dirtree), dv->priv->hilit_dir,
                                   &(dirtree->style->bg[GTK_STATE_PRELIGHT]));
      }

      /* Install the timeout handler for auto-expansion */
      if (conf.dirview_auto_expand && row != dv->priv->drag_tree_row) {
         dv->priv->auto_expand_timeout_id
            = gtk_timeout_add (conf.dirview_auto_expand_time,
                               timeout_auto_expand_directory,
                               dv);
         dv->priv->drag_tree_row = row;
      }
   }
}


static void
cb_drag_leave (GtkWidget *dirtree, GdkDragContext *context, guint time,
               gpointer data)
{
   DirView *dv = data;

   dirview_cancel_drag_scroll (dv);

   /*
     gtk_clist_set_foreground (GTK_CLIST (dirtree), dv->hilit_dir,
     &(dirtree->style->fg[GTK_STATE_NORMAL]));
   */
   gtk_clist_set_background (GTK_CLIST (dirtree), dv->priv->hilit_dir,
                             &(dirtree->style->base[GTK_STATE_NORMAL]));
   dv->priv->hilit_dir = -1;

   /* remove auto expand handler */
   if (dv->priv->auto_expand_timeout_id != 0) {
      gtk_timeout_remove (dv->priv->auto_expand_timeout_id);
      dv->priv->auto_expand_timeout_id = 0;
   }

   dv->priv->drag_tree_row = -1;
}


static void
cb_drag_data_get (GtkWidget *dirtree,
                  GdkDragContext *context,
                  GtkSelectionData *seldata,
                  guint info,
                  guint time,
                  gpointer data)
{
   DirView *dv = data;
   GtkCTreeNode *node;
   DirNode *dirnode;
   gchar *uri, *tmpstr;

   g_return_if_fail (dv && dirtree);

   dirview_cancel_drag_scroll (dv);

   node = GTK_CLIST (dirtree)->selection->data;
   if (!node) return;

   dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(dirtree), node);
   g_return_if_fail (dirnode);

   tmpstr = remove_slash (dirnode->path);

   uri = g_strconcat ("file://", tmpstr, "\r\n", NULL);

   gtk_selection_data_set(seldata, seldata->target,
                          8, uri, strlen(uri));
   g_free (tmpstr);
   g_free (uri);
}


static void
cb_drag_data_received (GtkWidget *dirtree,
                       GdkDragContext *context,
                       gint x, gint y,
                       GtkSelectionData *seldata,
                       guint info,
                       guint32 time,
                       gpointer data)
{
   gint row;
   DirNode *node;
   DirView *dv = data;
   GtkCTreeNode *parent;

   g_return_if_fail (dv);

   dirview_cancel_drag_scroll (dv);

   if (!GTK_CLIST (dirtree)->selection) return;

   gtk_clist_get_selection_info (GTK_CLIST (dirtree), x, y, &row, NULL);
   parent = gtk_ctree_node_nth (GTK_CTREE (dirtree), row);
   node = gtk_ctree_node_get_row_data (GTK_CTREE (dirtree), parent);

   if (node) {
      if (iswritable (node->path)) {
         dnd_file_operation (node->path, context, seldata, time, dv->tw);
         parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
         refresh_dir_tree (dv, parent);
      } else {
         gchar error_message[BUF_SIZE], *dir_internal;

         dir_internal = charset_to_internal (node->path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
         g_snprintf (error_message, BUF_SIZE,
                     _("Permission denied: %s"),
                     dir_internal);
         gtkutil_message_dialog
            (_("Error!!"), error_message,
             GTK_WINDOW(gtk_widget_get_toplevel(dv->container)));

         g_free (dir_internal);
      }
   }
}


static void
cb_drag_end (GtkWidget *dirtree, GdkDragContext *context, gpointer data)
{
   DirView *dv = data;
   GtkCTreeNode *parent;

   g_return_if_fail (dirtree && dv);

   dirview_cancel_drag_scroll (dv);

   parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);

   /* refresh_dir_tree (dv, parent); */
}


static void
cb_toolbar_drag_begin (GtkWidget *widget,
                       GdkDragContext *context,
                       gpointer data)
{
   GdkColormap *colormap;
   GimvIcon *icon;

   icon = gimv_icon_stock_get_icon ("paper");
   colormap = gdk_colormap_get_system ();
   gtk_drag_set_icon_pixmap (context, colormap,
                             icon->pixmap, icon->mask,
                             0, 0);
}


static void
cb_toolbar_drag_data_get (GtkWidget *widget,
                          GdkDragContext *context,
                          GtkSelectionData *seldata,
                          guint info,
                          guint time,
                          gpointer data)
{
   switch (info) {
   case TARGET_GIMV_COMPONENT:
      gtk_selection_data_set(seldata, seldata->target,
                             8, "dummy", strlen("dummy"));
      break;
   }
}


typedef struct SwapCom_Tag
{
   GimvThumbWin *tw;
   gint src;
   gint dest;
} SwapCom;


static gint
idle_thumbwin_swap_component (gpointer data)
{
   SwapCom *swap = data;
   gimv_thumb_win_swap_component (swap->tw, swap->src, swap->dest);
   return FALSE;
}


static void
cb_com_swap_drag_data_received (GtkWidget *widget,
                                GdkDragContext *context,
                                gint x, gint y,
                                GtkSelectionData *seldata,
                                guint info,
                                guint time,
                                gpointer data)
{
   GimvThumbWin *tw = data;
   GtkWidget *src_widget;
   gpointer p;
   gint src, dest;

   switch (info) {
   case TARGET_GIMV_COMPONENT:
      src_widget = gtk_drag_get_source_widget (context);
      if (gdk_window_get_toplevel (src_widget->window)
          != gdk_window_get_toplevel (widget->window))
      {
         return;
      }

      p = gtk_object_get_data (GTK_OBJECT (src_widget), "gimv-component");
      src = GPOINTER_TO_INT (p);
      if (!src) return;

      p = gtk_object_get_data (GTK_OBJECT (widget), "gimv-component");
      dest = GPOINTER_TO_INT (p);
      if (!dest) return;

      {
         SwapCom *swap = g_new0 (SwapCom, 1);
         swap->tw = tw;
         swap->src = src;
         swap->dest = dest;
         /* to avoid gtk's bug, exec redraw after exit this callback function */
         gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                            idle_thumbwin_swap_component, NULL, swap,
                            (GtkDestroyNotify) g_free);
      }

      break;

   default:
      break;
   }
}



/******************************************************************************
 *
 *   Public  functions
 *
 ******************************************************************************/
/*
 *  dirview_change_root:
 *     @ Change top directory.
 *
 *   dirtree   : GtkCtree widget.
 *   root_drir : path to change.

*/
void
dirview_change_root (DirView *dv, const gchar *root_dir)
{
   gchar *dest_dir;
   GtkCTreeNode *root_node;

   g_return_if_fail (dv && root_dir);

   dest_dir = add_slash (root_dir);

   if (!isdir (dest_dir)) {
      g_free (dest_dir);
      return;
   }

   g_free (dv->root_dir);
   dv->root_dir = dest_dir;

   gtk_clist_clear (GTK_CLIST (dv->dirtree));

   root_node = dirview_insert_node (dv, NULL, dv->root_dir);
   if (!root_node) return;

   /* set opened mark */
   if (gimv_thumb_view_find_opened_dir (dv->root_dir)) {
      dirview_set_opened_mark (dv, dv->root_dir);
   }

   gtk_ctree_expand (GTK_CTREE (dv->dirtree), root_node);

   adjust_ctree (dv->dirtree, root_node);
}


void
dirview_change_root_to_parent (DirView *dv)
{
   gchar *end;
   gchar *root;

   g_return_if_fail (dv);

   root = g_strdup (dv->root_dir);
   end = strrchr (root, '/');
   if (end && end != root) *end = '\0';
   end = strrchr (root, '/');
   if (end) *(end + 1) = '\0';

   dirview_change_dir (dv, root);

   g_free (root);
}


/*
 *  dirview_change_dir:
 *     @ Change selection to specified directory.
 *
 *   dirtree : GtkCtree widget:
 *   str     : path to change.
 */
void
dirview_change_dir (DirView *dv, const gchar *str)
{
   GtkWidget *dirtree;
   GtkCTreeNode *node;
   gchar *path, *row, *ptr;
   gboolean is_leaf, expanded;
   size_t length;

   g_return_if_fail (dv);

   dirtree = dv->dirtree;

   path = add_slash (str);

   if (!isdir (path)) {
      g_free (path);
      return;
   }

   ptr = path;

   /* FIXME */
   /* if selected path in directory view is same as str, adjust to it */
   /* END FIXME */

   /* get root node */
   node = gtk_ctree_node_nth (GTK_CTREE (dirtree), 0);
   gtk_ctree_get_node_info (GTK_CTREE (dirtree), node, &row, NULL,
                            NULL, NULL, NULL, NULL, &is_leaf, &expanded);

   /* if root node is specified directory, adjust to root */
   if (!(strcmp (ptr, row))) {
      g_free (path);
      adjust_ctree (dirtree, node);
      return;
   }

   if (!is_leaf && !expanded)
      gtk_ctree_expand (GTK_CTREE (dirtree), node);

   /* get first child */
   node = GTK_CTREE_ROW(node)->children;
   if (!node) {
      g_free (path);
      dirview_change_root (dv, str);
      return;
   }

   /*
    *  if specified directory string is shorter than root node's one,
    *  change top direcotry.
    */
   length = strlen (dv->root_dir);
   if (strlen (ptr) >= length) {
      ptr = ptr + length;
   } else {
      g_free (path);
      dirview_change_root (dv, str);
      return;
   }

   /* search children */
   gtk_clist_freeze (GTK_CLIST (dirtree));
   while (TRUE) {
      gtk_ctree_get_node_info (GTK_CTREE (dirtree), node, &row, NULL,
                               NULL, NULL, NULL, NULL, &is_leaf, &expanded);
      if (!strncmp (row, ptr, strlen (row)) && *(ptr+strlen (row)) == '/' ) {
         ptr = ptr + strlen (row) + 1;
         while (*ptr == '/')
            ptr++;
         if (!is_leaf && GTK_CTREE_ROW (node)->children && !expanded) {
            gtk_ctree_expand(GTK_CTREE (dirtree), node);
         }
         if (!(ptr && *ptr))
            break;
         if (!(node = GTK_CTREE_ROW (node)->children))
            break;
      } else if (!(node = GTK_CTREE_ROW (node)->sibling)) {
         break;
      }
   }
   gtk_clist_thaw (GTK_CLIST (dirtree));

   g_free (path);

   if (node) {
      adjust_ctree (dirtree, node);
   } else {
      dirview_change_root (dv, str);
   }
}


/*
 *  dirview_go_home:
 *     @ Change top directory to home directory.
 *
 *  dv : Pointer to the DirView struct.
 */
void
dirview_go_home (DirView *dv)
{
   const gchar *home = g_getenv ("HOME");

   dirview_change_root (dv, home);
   dirview_change_dir (dv, home);
}


void
dirview_refresh_list (DirView *dv)
{
   GtkCTreeNode *parent;

   g_return_if_fail (dv);

   parent = gtk_ctree_node_nth (GTK_CTREE (dv->dirtree), 0);
   if (parent)
      refresh_dir_tree (dv, parent);
}


gchar *
dirview_get_selected_path (DirView *dv)
{
   DirNode *dirnode;
   GtkCTreeNode *node;

   g_return_val_if_fail (dv, NULL);
   g_return_val_if_fail (dv->dirtree, NULL);
   if (!GTK_CLIST(dv->dirtree)->selection) return NULL;

   node =  GTK_CLIST (dv->dirtree)->selection->data;
   if (!node) return NULL;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);
   if (!dirnode) return NULL;

   return g_strdup (dirnode->path);
}


void
dirview_expand_dir (DirView *dv, const gchar *dir, gboolean open_all)
{
   GtkCTreeNode *node;

   g_return_if_fail (dv);
   g_return_if_fail (dir && *dir);

   node = dirview_find_directory (dv, dir);
   if (!node) return;

   if (open_all)
      gtk_ctree_expand_recursive (GTK_CTREE (dv->dirtree), node);
   else
      gtk_ctree_expand (GTK_CTREE (dv->dirtree), node);
}


gboolean
dirview_set_opened_mark (DirView *dv, const gchar *path)
{
   GtkCTreeNode *node;
   DirNode *dirnode;

   g_return_val_if_fail (dv, FALSE);
   g_return_val_if_fail (path && path, FALSE);

   node = dirview_find_directory (dv, path);

   if (!node) return FALSE;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);
   dirnode->flags |= DirNodeOpenedFlag;

   gtk_ctree_node_set_foreground (GTK_CTREE (dv->dirtree), node,
                                  &dv->dirtree->style->fg[GTK_STATE_ACTIVE]);

   return TRUE;
}


gboolean
dirview_unset_opened_mark (DirView *dv, const gchar *path)
{
   GtkCTreeNode *node;
   DirNode *dirnode;

   g_return_val_if_fail (dv, FALSE);
   g_return_val_if_fail (path && path, FALSE);

   node = dirview_find_directory (dv, path);

   if (!node) return FALSE;

   dirnode = gtk_ctree_node_get_row_data (GTK_CTREE (dv->dirtree), node);
   dirnode->flags &= ~DirNodeOpenedFlag;

   gtk_ctree_node_set_foreground (GTK_CTREE (dv->dirtree), node,
                                  &dv->dirtree->style->fg[GTK_STATE_NORMAL]);

   return TRUE;
}


/*
 *  dirview_show_toolbar:
 *     @
 *
 *  dv : Pointer to the DirView struct.
 */
void
dirview_show_toolbar (DirView *dv)
{
   g_return_if_fail (dv);

   dv->show_toolbar = TRUE;
   gtk_widget_show (dv->toolbar_eventbox);
}


/*
 *  dirview_hide_toolbar:
 *     @
 *
 *  dv : Pointer to the DirView struct.
 */
void
dirview_hide_toolbar (DirView *dv)
{
   g_return_if_fail (dv);

   dv->show_toolbar = FALSE;
   gtk_widget_hide (dv->toolbar_eventbox);
}


/*
 *  dirview_create_toolbar:
 *     @ 
 *
 *  dv : Pointer to the DirView struct.
 */
static GtkWidget *
dirview_create_toolbar (DirView *dv)
{
   GtkWidget *toolbar;
   GtkWidget *button;
   GtkWidget *iconw;   

   g_return_val_if_fail (dv, NULL);

   toolbar = gtkutil_create_toolbar ();

   /* file open button */
   iconw = gimv_icon_stock_get_widget ("small_home");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Home"),
                                     _("Home"),
                                     _("Home"),
                                     iconw,
                                     GTK_SIGNAL_FUNC (cb_home_button),
                                     dv);

   /* preference button */
   iconw = gimv_icon_stock_get_widget ("small_up");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Up"),
                                     _("Up"),
                                     _("Up"),
                                     iconw,
                                     GTK_SIGNAL_FUNC (cb_up_button),
                                     dv);

   /* refresh button */
   iconw = gimv_icon_stock_get_widget ("small_refresh");
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Refresh"),
                                    _("Refresh"),
                                    _("Refresh"),
                                    iconw,
                                    GTK_SIGNAL_FUNC (cb_refresh_button),
                                    dv);

   /* preference button */
   iconw = gimv_icon_stock_get_widget ("dotfile");
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Dotfile"),
                                    _("Show/Hide dotfile"),
                                    _("Show/Hide dotfile"),
                                    iconw,
                                    GTK_SIGNAL_FUNC (cb_dotfile_button),
                                    dv);

   gtk_widget_show_all (toolbar);
   gtk_toolbar_set_style (GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

   return toolbar;
}


static gint
idle_change_root (gpointer data)
{
   DirView *dv = data;

   g_return_val_if_fail (dv, FALSE);

   dv->priv->change_root_id = 0;

   dirview_change_root (dv, dv->root_dir);
   dirview_change_dir (dv, dv->root_dir);
   /*
   if (dv)
   dirview_go_home (dv);
   */

   return FALSE;
}


/******************************************************************************
 *
 *   dirview_create:
 *      @ create directory view widget.
 *
 *   tw     :
 *   Return : directory view packed container widget.
 *
 ******************************************************************************/
DirView *
dirview_create (const gchar *root_dir, GtkWidget *parent_win, GimvThumbWin *tw)
{
   DirView *dv;
   GtkWidget *eventbox;
   const gchar *home = g_getenv ("HOME");

   dv       = g_new0 (DirView, 1);
   dv->priv = g_new0 (DirViewPrivate, 1);

   if (root_dir)
      dv->root_dir = g_strdup (root_dir);
   else
      dv->root_dir = g_strdup (home);

   dv->popup_menu        = NULL;
   dv->tw                = tw;
   dv->show_toolbar      = conf.dirview_show_toolbar;
   dv->show_dotfile      = conf.dirview_show_dotfile;
   dv->priv->hilit_dir        = -1;
   dv->priv->scroll_timer_id  = 0;
   dv->priv->button_action_id = 0;
   dv->priv->swap_com_id      = 0;
   dv->priv->change_root_id   = 0;

   /* main vbox */
   dv->container = gtk_vbox_new (FALSE, 0);
   gtk_widget_set_name (dv->container, "DirView");
   gtk_widget_show (dv->container);

   dnd_dest_set (dv->container, dnd_types_component, dnd_types_component_num);
   gtk_object_set_data (GTK_OBJECT (dv->container),
                        "gimv-component",
                        GINT_TO_POINTER (GIMV_COM_DIR_VIEW));
   gtk_signal_connect (GTK_OBJECT (dv->container), "drag_data_received",
                       GTK_SIGNAL_FUNC (cb_com_swap_drag_data_received), dv->tw);

   /* toolbar */
   eventbox = dv->toolbar_eventbox = gtk_event_box_new ();
   gtk_container_set_border_width (GTK_CONTAINER (eventbox), 1);
   gtk_box_pack_start (GTK_BOX (dv->container), eventbox, FALSE, FALSE, 0);
   gtk_widget_show (eventbox);

   dv->toolbar = dirview_create_toolbar (dv);
   gtk_container_add (GTK_CONTAINER (eventbox), dv->toolbar);

   if (!dv->show_toolbar)
      gtk_widget_hide (dv->toolbar_eventbox);

   dnd_src_set  (eventbox, dnd_types_component, dnd_types_component_num);
   gtk_object_set_data (GTK_OBJECT (eventbox),
                        "gimv-component",
                        GINT_TO_POINTER (GIMV_COM_DIR_VIEW));
   gtk_signal_connect (GTK_OBJECT (eventbox), "drag_begin",
                       GTK_SIGNAL_FUNC (cb_toolbar_drag_begin), tw);
   gtk_signal_connect (GTK_OBJECT (eventbox), "drag_data_get",
                       GTK_SIGNAL_FUNC (cb_toolbar_drag_data_get), tw);

   /* scrolled window */
   dv->scroll_win = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_set_border_width (GTK_CONTAINER (dv->scroll_win), 1);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dv->scroll_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_box_pack_start(GTK_BOX(dv->container), dv->scroll_win, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT (dv->scroll_win), "destroy",
                       GTK_SIGNAL_FUNC (cb_dirview_destroyed), dv);
   gtk_widget_show (dv->scroll_win);

   /* ctree */
   dirview_create_ctree (dv);

   dv->priv->change_root_id = gtk_idle_add (idle_change_root, dv);
   /* dirview_go_home (dirtree); */

   return dv;
}

#endif /* ENABLE_TREEVIEW */
