/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2002 Takuro Ashie
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

#include "gimv_dir_view.h"

#include <string.h>

#include "charset.h"
#include "utils_dnd.h"
#include "fileutil.h"
#include "gfileutil.h"
#include "gtk_prop.h"
#include "gimv_icon_stock.h"
#include "gimv_thumb_win.h"
#include "menu.h"
#include "prefs.h"

G_DEFINE_TYPE (GimvDirView, gimv_dir_view, GTK_TYPE_VBOX)

#define GIMV_DIR_VIEW_GET_PRIVATE(obj) \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMV_TYPE_DIR_VIEW, GimvDirViewPrivate))

typedef struct GimvDirViewPrivate_Tag {
   /* for DnD */
   gint         hilit_dir;

   guint        scroll_timer_id;
   gint         drag_motion_x;
   gint         drag_motion_y;

   gint         auto_expand_timeout_id;

   /* for mouse event */
   gint         button_2pressed_queue; /* attach an action to
                                          button release event */

   guint        button_action_id;
   guint        swap_com_id;       

   GtkTreePath *drag_tree_row;
   guint        adjust_tree_id;
} GimvDirViewPrivate;

typedef enum {
   MouseActNone,
   MouseActLoadThumb,
   MouseActLoadThumbRecursive,
   MouseActPopupMenu,
   MouseActChangeTop,
   MouseActLoadThumbRecursiveInOneTab
} GimvDirViewMouseAction;

typedef enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_LABEL,
   COLUMN_FULLPATH,
   COLUMN_ICON_OPEN,
   COLUMN_ICON_CLOSE,
   COLUMN_IS_DUMMY,
   N_COLUMN
} TreeStoreColumn;

/* callback functions */
static gboolean   cb_button_press                (GtkWidget      *widget,
                                                  GdkEventButton *event,
                                                  GimvDirView    *dv);
static gboolean   cb_button_release              (GtkWidget      *widget,
                                                  GdkEventButton *event,
                                                  GimvDirView    *dv);
static gboolean   cb_scroll                      (GtkWidget      *widget,
                                                  GdkEventScroll *se,
                                                  GimvDirView    *dv);
static gboolean   cb_key_press                   (GtkWidget      *widget,
                                                  GdkEventKey    *event,
                                                  GimvDirView    *dv);
static void       cb_tree_expand                 (GtkTreeView    *treeview,
                                                  GtkTreeIter    *parent_iter,
                                                  GtkTreePath    *treepath,
                                                  gpointer        data);

/* callback functions for popup menu */
static void       cb_open_thumbnail              (GimvDirView    *dv,
                                                  ScanSubDirType  scan_subdir,
                                                  GtkWidget      *menuitem);
static void       cb_go_to_here                  (GimvDirView    *dv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);
static void       cb_refresh_dir_tree            (GimvDirView    *dv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);
static void       cb_file_property               (GimvDirView    *tv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);
static void       cb_mkdir                       (GimvDirView    *dv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);
static void       cb_rename_dir                  (GimvDirView    *dv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);
static void       cb_delete_dir                  (GimvDirView    *dv,
                                                  guint           action,
                                                  GtkWidget      *menuitem);

/* Callback functions for toolbar buttons */
static void       cb_home_button                 (GtkWidget      *widget,
                                                  GimvDirView    *dv);
static void       cb_up_button                   (GtkWidget      *widget,
                                                  GimvDirView    *dv);
static void       cb_refresh_button              (GtkWidget      *widget,
                                                  GimvDirView    *dv);
static void       cb_dotfile_button              (GtkWidget      *widget,
                                                  GimvDirView    *dv);

/* Callback functions for DnD */
static void       cb_drag_data_get               (GtkWidget        *dirtree,
                                                  GdkDragContext   *context,
                                                  GtkSelectionData *seldata,
                                                  guint             info,
                                                  guint             time,
                                                  gpointer          data);
static void       cb_drag_data_received          (GtkWidget        *dirtree,
                                                  GdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  GtkSelectionData *seldata,
                                                  guint             info,
                                                  guint32           time,
                                                  gpointer          data);
static void       cb_drag_end                    (GtkWidget        *dirtree,
                                                  GdkDragContext   *context,
                                                  gpointer          data);
static void       cb_toolbar_drag_begin          (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  gpointer          data);
static gboolean   cb_drag_motion                 (GtkWidget *widget,
                                                  GdkDragContext *drag_context,
                                                  gint x,
                                                  gint y,
                                                  guint time,
                                                  gpointer          data);
static void       cb_toolbar_drag_data_get       (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  GtkSelectionData *seldata,
                                                  guint             info,
                                                  guint             time,
                                                  gpointer          data);
static void       cb_com_swap_drag_data_received (GtkWidget        *widget,
                                                  GdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  GtkSelectionData *seldata,
                                                  guint             info,
                                                  guint             time,
                                                  gpointer          data);

/* other private functions */
static void       get_icon_pixbufs               (void);
static void       set_columns_type               (GtkTreeView    *tree_view);
static void       dirview_create_treeview        (GimvDirView    *dv,
                                                  const gchar    *root);
static GtkWidget *dirview_create_toolbar         (GimvDirView    *dv);
static void       insert_dummy_row               (GtkTreeStore   *store,
                                                  GtkTreeIter    *parent_iter);
static void       insert_row                     (GtkTreeStore   *store,
                                                  GtkTreeIter    *iter,
                                                  GtkTreeIter    *parent_iter,
                                                  const gchar    *label,
                                                  const gchar    *fullpath);
static gboolean   get_iter_from_path             (GimvDirView    *dv,
                                                  const gchar    *str,
                                                  GtkTreeIter    *iter);
static void       adjust_tree                    (GtkTreeView    *treeview,
                                                  GtkTreeIter    *iter);
static void       adjust_tree_idle               (GimvDirView    *dv,
                                                  GtkTreeIter    *iter);
static void       get_expanded_dirs              (GtkTreeView    *treeview,
                                                  GtkTreePath    *treepath,
                                                  gpointer        data);
static void       refresh_dir_tree               (GimvDirView    *dv,
                                                  GtkTreeIter    *parent_iter);
static void       dirview_popup_menu             (GimvDirView    *dv,
                                                  GdkEventButton *event);
static gboolean   dirview_button_action          (GimvDirView    *dv,
                                                  GdkEventButton *event,
                                                  gint            num);

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

static GdkPixbuf *folder      = NULL;
static GdkPixbuf *ofolder     = NULL;
static GdkPixbuf *lfolder     = NULL;
static GdkPixbuf *lofolder    = NULL;
static GdkPixbuf *go_folder   = NULL;
static GdkPixbuf *up_folder   = NULL;
static GdkPixbuf *lock_folder = NULL;



/******************************************************************************
 *
 *   Callback functions
 *
 ******************************************************************************/
static void
gimv_dir_view_dispose (GObject *object)
{
   GimvDirView *dv = GIMV_DIR_VIEW (object);
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);

   if (dv->root_dir)
      g_free (dv->root_dir);
   dv->root_dir = NULL;

   if (priv->button_action_id)
      gtk_idle_remove (priv->button_action_id);
   priv->button_action_id = 0;

   if (priv->swap_com_id)
      gtk_idle_remove (priv->swap_com_id);
   priv->swap_com_id = 0;

   if (priv->adjust_tree_id)
      gtk_idle_remove (priv->adjust_tree_id);
   priv->adjust_tree_id = 0;
}


static gboolean
cb_button_press (GtkWidget *widget, GdkEventButton *event, GimvDirView *dv)
{
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   gint num;

   num = prefs_mouse_get_num_from_event (event, conf.dirview_mouse_button);
   if (event->type == GDK_2BUTTON_PRESS) {
      priv->button_2pressed_queue = num;
   } else if (num > 0) {
      return dirview_button_action (dv, event, num);
   }

   return FALSE;
}


static gboolean
cb_button_release (GtkWidget *widget, GdkEventButton *event, GimvDirView *dv)
{
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   gint num;

   if (priv->button_2pressed_queue) {
      num = priv->button_2pressed_queue;
      if (num > 0)
         num = 0 - num;
      priv->button_2pressed_queue = 0;
   } else {
      num = prefs_mouse_get_num_from_event (event, conf.dirview_mouse_button);
   }
   if (num < 0)
      return dirview_button_action (dv, event, num);

   return FALSE;
}


static gboolean
cb_scroll (GtkWidget *widget, GdkEventScroll *se, GimvDirView *dv)
{
   GdkEventButton be;
   gboolean retval = FALSE;
   gint num;

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

   num = prefs_mouse_get_num_from_event (&be, conf.dirview_mouse_button);
   if (num > 0)
      retval = dirview_button_action (dv, &be, num);

   be.type = GDK_BUTTON_RELEASE;
   if (num < 0)
      retval = dirview_button_action (dv, &be, num);

   return retval;
}


static gboolean
cb_key_press (GtkWidget *widget, GdkEventKey *event, GimvDirView *dv)
{
   guint keyval, popup_key;
   GdkModifierType modval, popup_mod;
   gboolean success;
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkTreePath *treepath;
   gboolean retval = FALSE;
   gchar *label, *path;

   g_return_val_if_fail (dv, FALSE);
   g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

   keyval = event->keyval;
   modval = event->state;

   if (akey.common_popup_menu || *akey.common_popup_menu)
      gtk_accelerator_parse (akey.common_popup_menu, &popup_key, &popup_mod);
   else
      return FALSE;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dv->dirtree));
   g_return_val_if_fail (selection, FALSE);

   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return FALSE;
   treepath = gtk_tree_model_get_path (model, &iter);
   if (!treepath) return FALSE;

   gtk_tree_model_get (model, &iter,
                       COLUMN_LABEL,    &label,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   if (keyval == popup_key && (!popup_mod || (modval & popup_mod))) {
      dirview_popup_menu (dv, NULL);
   } else {
      switch (keyval) {
      case GDK_KP_Enter:
      case GDK_Return:
      case GDK_ISO_Enter:
      {
         if (!strcmp (label, ".") || !strcmp (label, "..")) {
            gimv_dir_view_chroot (dv, path);
         } else {
            open_dir_images (path, dv->tw, NULL,
                             LOAD_CACHE, conf.scan_dir_recursive);
         }
         retval = TRUE;
         break;
      }
      case GDK_space:
         if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), treepath))
            gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), treepath);
         else
            gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), treepath, FALSE);
         retval = TRUE;
         break;
      case GDK_Right:
         if (modval & GDK_CONTROL_MASK) {
            gimv_dir_view_chroot (dv, path);
         } else {
            gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), treepath, FALSE);
         }
         retval = TRUE;
         break;
      case GDK_Left:
         if (modval & GDK_CONTROL_MASK) {
            gimv_dir_view_chroot_to_parent (dv);
         } else {
            gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), treepath);
         }
         retval = TRUE;
         break;
      case GDK_Up:
         if (modval & GDK_CONTROL_MASK) {
            gimv_dir_view_chroot_to_parent (dv);
            retval = TRUE;
         }
         break;
      case GDK_Down:
         if (modval & GDK_CONTROL_MASK) {
            gimv_dir_view_chroot (dv, path);
            retval = TRUE;
         }
         break;
      }
   }

   g_free (label);
   g_free (path);
   gtk_tree_path_free(treepath);

   return retval;
}


static void
cb_tree_expand (GtkTreeView *treeview,
                GtkTreeIter *parent_iter,
                GtkTreePath *treepath,
                gpointer data)
{
   GimvDirView *dv = data;
   GtkTreeStore *store;
   GtkTreeIter child_iter, iter;
   gchar *parent_dir;
   gboolean dummy;
   GList *subdir_list = NULL, *node;
   GetDirFlags flags;

   g_return_if_fail (dv);

   store = GTK_TREE_STORE (gtk_tree_view_get_model (treeview));

   gtk_tree_model_iter_children (GTK_TREE_MODEL (store),
                                 &child_iter, parent_iter);
   gtk_tree_model_get (GTK_TREE_MODEL (store), &child_iter,
                       COLUMN_IS_DUMMY, &dummy,
                       COLUMN_TERMINATOR);
   if (!dummy) return;

   gtk_tree_model_get (GTK_TREE_MODEL (store), parent_iter,
                       COLUMN_FULLPATH, &parent_dir,
                       COLUMN_TERMINATOR);

   flags = GETDIR_FOLLOW_SYMLINK | GETDIR_READ_DOT;
   get_dir (parent_dir, flags, NULL, &subdir_list);

   if (dv->show_dotfile || conf.dirview_show_current_dir) {
      insert_row (store, &iter, parent_iter, ".", parent_dir);
   }

   if (dv->show_dotfile || conf.dirview_show_parent_dir) {
      gchar *end, *grandparent = remove_slash(parent_dir);

      end = strrchr (grandparent, '/');
      if (end) *(end + 1) = '\0';

      insert_row (store, &iter, parent_iter, "..", grandparent);

      g_free (grandparent);
   }

   for (node = subdir_list; node; node = g_list_next (node)) {
      gchar *path = node->data;
      gboolean dot_file_check;

      dot_file_check = g_basename(path)[0] != '.'
         || (dv->show_dotfile && g_basename(path)[0] == '.')
         || (conf.dirview_show_current_dir && !strcmp (g_basename(path), "."))
         || (conf.dirview_show_parent_dir  && !strcmp (g_basename(path), ".."));

      if (isdir (path) && dot_file_check) {
         insert_row (store, &iter, parent_iter, g_basename (path), path);
      }
   }
   g_list_foreach (subdir_list, (GFunc) g_free, NULL);
   g_list_free (subdir_list);

   gtk_tree_store_remove (store, &child_iter);

   g_free (parent_dir);
}



/******************************************************************************
 *
 *   Callback functions for popup menu
 *
 ******************************************************************************/
static void
cb_open_thumbnail (GimvDirView *dv, ScanSubDirType scan_subdir, GtkWidget *menuitem)
{
   gchar *path;

   path = gimv_dir_view_get_selected_path (dv);
   if (!path) return;

   open_dir_images (path, dv->tw, NULL, LOAD_CACHE, scan_subdir);

   g_free (path);
}


static void
cb_go_to_here (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   gchar *path;

   path = gimv_dir_view_get_selected_path (dv);
   if (!path) return;

   gimv_dir_view_chroot (dv, path);

   g_free (path);
}


static void
cb_refresh_dir_tree (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   GtkTreeModel *model;
   GtkTreeSelection *selection;
   GtkTreeIter iter;
   gboolean success;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dv->dirtree));
   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return;

   refresh_dir_tree (dv, &iter);
}


static void
cb_file_property (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   GimvImageInfo *info;
   gchar *path, *tmpstr;

   g_return_if_fail (dv);

   tmpstr = gimv_dir_view_get_selected_path (dv);
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
cb_mkdir (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   gchar *parent_path;
   gboolean success;
   GtkTreeIter iter;

   g_return_if_fail (dv);

   parent_path = gimv_dir_view_get_selected_path (dv);
   if (!parent_path) return;

   success = make_dir_dialog (
      parent_path,
      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (dv))));

   if (success) {
      if (get_iter_from_path (dv, parent_path, &iter))
         refresh_dir_tree (dv, &iter);
      else
         refresh_dir_tree (dv, NULL);
   }

   g_free (parent_path);
}


static void
cb_rename_dir (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   gboolean success;
   gchar *path;

   g_return_if_fail (dv);

   path = gimv_dir_view_get_selected_path (dv);
   if (!path) return;

   success = rename_dir_dialog
      (path, GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (dv))));

   if (success) {
      gchar *tmp_path, *parent_dir;
      GtkTreeIter iter;

      tmp_path = remove_slash (path);
      parent_dir = g_dirname (tmp_path);

      if (get_iter_from_path (dv, parent_dir, &iter))
         refresh_dir_tree (dv, &iter);
      else
         refresh_dir_tree (dv, NULL);

      g_free (tmp_path);
      g_free (parent_dir);
   }
}


static void
cb_delete_dir (GimvDirView *dv, guint action, GtkWidget *menuitem)
{
   gchar *path, *parent_dir;
   GtkTreeIter iter;

   g_return_if_fail (dv);

   path = gimv_dir_view_get_selected_path (dv);
   if (!path) return;

   if (path [strlen (path) - 1] == '/')
      path [strlen (path) - 1] = '\0';

   delete_dir (path, GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (dv))));
   g_free (path);

   parent_dir = g_dirname (path);

   /* refresh dir tree */
   if (get_iter_from_path (dv, parent_dir, &iter))
      refresh_dir_tree (dv, &iter);
   else
      refresh_dir_tree (dv, NULL);

   g_free (parent_dir);
}



/******************************************************************************
 *
 *   Callback functions for toolbar buttons.
 *
 ******************************************************************************/
static void
cb_home_button (GtkWidget *widget, GimvDirView *dv)
{
   g_return_if_fail (dv);

   gimv_dir_view_go_home (dv);
}


static void
cb_up_button (GtkWidget *widget, GimvDirView *dv)
{
   gimv_dir_view_chroot_to_parent (dv);
}


static void
cb_refresh_button (GtkWidget *widget, GimvDirView *dv)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (widget && dv);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   success = gtk_tree_model_get_iter_first (model, &iter);
   if (!success) return;

   refresh_dir_tree (dv, &iter);
}


static void
cb_dotfile_button (GtkWidget *widget, GimvDirView *dv)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (widget && dv);

   dv->show_dotfile = !dv->show_dotfile;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   success = gtk_tree_model_get_iter_first (model, &iter);
   if (!success) return;

   refresh_dir_tree (dv, &iter);
}


/******************************************************************************
 *
 *   Callback functions for DnD
 *
 ******************************************************************************/
static void
cb_drag_data_get (GtkWidget *dirtree,
                  GdkDragContext *context,
                  GtkSelectionData *seldata,
                  guint info,
                  guint time,
                  gpointer data)
{
   GimvDirView *dv = data;
   gchar *path, *urilist;

   g_return_if_fail (dv);

   path = gimv_dir_view_get_selected_path(dv);
   if (!path) return;
   if (!*path) {
      g_free (path);
      return;
   }

   if (path [strlen (path) - 1] == '/')
      path [strlen (path) - 1] = '\0';
   urilist = g_strconcat ("file://", path, "\r\n", NULL);

   gtk_selection_data_set(seldata, seldata->target,
                          8, urilist, strlen(urilist));

   g_free (path);
   g_free (urilist);
}


static gboolean
cb_drag_motion (GtkWidget *widget,
                GdkDragContext *drag_context,
                gint x, gint y, guint time,
                gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *dest_path = NULL;
	GtkTreeViewDropPosition pos;
	gboolean success, retval = FALSE;
   /* GimvDirView *dv = data; */

   g_return_val_if_fail(GTK_IS_TREE_VIEW(widget), FALSE);
	g_return_val_if_fail(data, FALSE);

	success = gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget),
                                               x, y,
                                               &dest_path, &pos);
	if (!success) return FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_model_get_iter(model, &iter, dest_path);
   /*
	gtk_tree_model_get(model, &iter,
                      COLUMN_FULLPATH, &path,
                      COLUMN_TERMINATOR);
   */

	if (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE ||
	    pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
	{
			gdk_drag_status(drag_context, GDK_ACTION_MOVE, time);
	}
	else if (pos == GTK_TREE_VIEW_DROP_BEFORE ||
            pos == GTK_TREE_VIEW_DROP_AFTER)
	{
			gdk_drag_status(drag_context, 0, time);
			retval = TRUE;
	}

	if (dest_path)
		gtk_tree_path_free(dest_path);

	return retval;
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
   GimvDirView *dv = data;
   GtkTreePath *treepath;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;
   gchar *path;

   g_return_if_fail (dv);

   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (dirtree),
                                            x, y,
                                            &treepath, NULL,
                                            NULL, NULL);
   if (!success) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dirtree));
   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   if (path && *path) {
      if (iswritable (path)) {
         dnd_file_operation (path, context, seldata, time, dv->tw);
         success = gtk_tree_model_get_iter_first (model, &iter);
         if (success)
            refresh_dir_tree (dv, &iter);
         else
            refresh_dir_tree (dv, NULL);
      } else {
         gchar error_message[BUF_SIZE], *dir_internal;

         dir_internal = charset_to_internal (path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
         g_snprintf (error_message, BUF_SIZE,
                     _("Permission denied: %s"),
                     dir_internal);
         gtkutil_message_dialog (
            _("Error!!"), error_message,
            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (dv))));

         g_free (dir_internal);
      }
   }

   gtk_tree_path_free (treepath);
   g_free (path);
}


static void
cb_drag_end (GtkWidget *dirtree, GdkDragContext *context, gpointer data)
{
   GimvDirView *dv = data;
   GtkTreeModel *model;
   GtkTreeIter iter;

   g_return_if_fail (dirtree && dv);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dirtree));
   if (gtk_tree_model_get_iter_first (model, &iter))
      refresh_dir_tree (dv, &iter);
   else
      refresh_dir_tree (dv, NULL);
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
   GimvThumbWin *tw = GIMV_DIR_VIEW (widget)->tw;
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

      p = g_object_get_data (G_OBJECT (src_widget), "gimv-component");
      src = GPOINTER_TO_INT (p);
      if (!src) return;

      p = g_object_get_data (G_OBJECT (widget), "gimv-component");
      dest = GPOINTER_TO_INT (p);
      if (!dest) return;

      {
         SwapCom *swap = g_new0 (SwapCom, 1);
         swap->tw = tw;
         swap->src = src;
         swap->dest = dest;
         /* to avoid gtk's bug, exec redraw after exit this callback function */
         gtk_idle_add_full (/* GTK_PRIORITY_REDRAW */G_PRIORITY_LOW,
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
 *   Other private  functions
 *
 ******************************************************************************/
static void
get_icon_pixbufs (void)
{
   if (!folder) { 
      folder = gimv_icon_stock_get_pixbuf ("folder");
      if (folder) g_object_ref (folder);
   }

   if (!ofolder) {
      ofolder = gimv_icon_stock_get_pixbuf ("folder-open");
      if (ofolder) g_object_ref (ofolder);
   }

   if (!lfolder) { 
      lfolder = gimv_icon_stock_get_pixbuf ("folder-link");
      if (lfolder) g_object_ref (lfolder);
   }

   if (!lofolder) {
      lofolder = gimv_icon_stock_get_pixbuf ("folder-link-open");
      if (lofolder) g_object_ref (lofolder);
   }

   if (!go_folder) {
      go_folder = gimv_icon_stock_get_pixbuf ("folder-go");
      if (go_folder) g_object_ref (go_folder);
   }

   if (!up_folder) {
      up_folder = gimv_icon_stock_get_pixbuf ("folder-up");
      if (up_folder) g_object_ref (up_folder);
   }

   if (!lock_folder) {
      lock_folder = gimv_icon_stock_get_pixbuf ("folder-lock");
      if (lock_folder) g_object_ref (lock_folder);
   }
}


static void
set_columns_type (GtkTreeView *tree_view)
{
   GtkTreeViewColumn *col;
   GtkCellRenderer *render;

   gtk_tree_view_set_rules_hint (tree_view, FALSE);
   gtk_tree_view_set_rules_hint (tree_view, TRUE);

   col = gtk_tree_view_column_new();
   gtk_tree_view_column_set_title (col, "Directory Name");

   render = gtk_cell_renderer_pixbuf_new ();
   gtk_tree_view_column_pack_start (col, render, FALSE);
   gtk_tree_view_column_add_attribute (col, render,
                                       "pixbuf", COLUMN_ICON_CLOSE);
   gtk_tree_view_column_add_attribute (col, render,
                                       "pixbuf-expander-open",
                                       COLUMN_ICON_OPEN);
   gtk_tree_view_column_add_attribute (col, render,
                                       "pixbuf-expander-closed",
                                       COLUMN_ICON_CLOSE);

   render = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, render, TRUE);
   gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_LABEL);

   gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);
   gtk_tree_view_set_expander_column (tree_view, col);
}


static void
dirview_create_treeview (GimvDirView *dv, const gchar *root)
{
   GtkTreeStore *store;
   GtkTreeIter root_iter;
   GtkTreePath *treepath;

   get_icon_pixbufs();

   store = gtk_tree_store_new (N_COLUMN,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               GDK_TYPE_PIXBUF,
                               GDK_TYPE_PIXBUF,
                               G_TYPE_BOOLEAN);

   dv->dirtree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

   insert_row (store, &root_iter, NULL, root, root);

   g_object_unref (G_OBJECT (store));

   g_signal_connect (G_OBJECT (dv->dirtree), "row-expanded",
                     G_CALLBACK (cb_tree_expand), dv);
   g_signal_connect (G_OBJECT (dv->dirtree),"button_press_event",
                     G_CALLBACK (cb_button_press), dv);
   g_signal_connect (G_OBJECT (dv->dirtree),"button_release_event",
                     G_CALLBACK (cb_button_release), dv);
   g_signal_connect (G_OBJECT(dv->dirtree), "scroll-event",
                     G_CALLBACK(cb_scroll), dv);
   g_signal_connect (G_OBJECT (dv->dirtree), "key_press_event",
                     G_CALLBACK (cb_key_press), dv);

   /* for DnD */
   gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (dv->dirtree),
                                           GDK_BUTTON1_MASK
                                           | GDK_BUTTON2_MASK
                                           | GDK_BUTTON3_MASK,
                                           dnd_types_uri,
                                           dnd_types_uri_num,
                                           GDK_ACTION_ASK  | GDK_ACTION_COPY
                                           | GDK_ACTION_MOVE | GDK_ACTION_LINK);
   gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (dv->dirtree),
                                         dnd_types_uri,
                                         dnd_types_uri_num,
                                         GDK_ACTION_ASK  | GDK_ACTION_COPY
                                         | GDK_ACTION_MOVE | GDK_ACTION_LINK);
   g_signal_connect (G_OBJECT (dv->dirtree), "drag_data_get",
                     G_CALLBACK (cb_drag_data_get), dv);
   g_signal_connect (G_OBJECT (dv->dirtree), "drag_motion",
                     G_CALLBACK (cb_drag_motion), dv);
   g_signal_connect (G_OBJECT (dv->dirtree), "drag_data_received",
                     G_CALLBACK (cb_drag_data_received), dv);
   g_signal_connect (G_OBJECT (dv->dirtree), "drag_end",
                     G_CALLBACK (cb_drag_end), dv);

   set_columns_type (GTK_TREE_VIEW (dv->dirtree));
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dv->dirtree), FALSE);

   gtk_container_add (GTK_CONTAINER (dv->scroll_win), dv->dirtree);
   gtk_widget_show (dv->dirtree);

   treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &root_iter);
   gtk_tree_view_expand_row (GTK_TREE_VIEW (dv->dirtree), treepath, FALSE);

   gtk_tree_path_free (treepath);

   adjust_tree_idle (dv, NULL);
}


static GtkWidget *
dirview_create_toolbar (GimvDirView *dv)
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
                                     G_CALLBACK (cb_home_button),
                                     dv);

   /* preference button */
   iconw = gimv_icon_stock_get_widget ("small_up");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Up"),
                                     _("Up"),
                                     _("Up"),
                                     iconw,
                                     G_CALLBACK (cb_up_button),
                                     dv);

   /* refresh button */
   iconw = gimv_icon_stock_get_widget ("small_refresh");
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Refresh"),
                                    _("Refresh"),
                                    _("Refresh"),
                                    iconw,
                                    G_CALLBACK (cb_refresh_button),
                                    dv);

   /* preference button */
   iconw = gimv_icon_stock_get_widget ("dotfile");
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Dotfile"),
                                    _("Show/Hide dotfile"),
                                    _("Show/Hide dotfile"),
                                    iconw,
                                    G_CALLBACK (cb_dotfile_button),
                                    dv);

   gtk_widget_show_all (toolbar);
   gtk_toolbar_set_style (GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

   return toolbar;
}


/*
 *  remove all children, and insert dummy row.
 */
static void
insert_dummy_row (GtkTreeStore *store, GtkTreeIter *parent_iter)
{
   GtkTreeIter iter;
   gboolean success;

   g_return_if_fail (store);

   gtk_tree_store_prepend (store, &iter, parent_iter);
   gtk_tree_store_set (store, &iter,
                       COLUMN_IS_DUMMY,   TRUE,
                       COLUMN_TERMINATOR);

   success = gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store),
                                            &iter,
                                            parent_iter,
                                            1);
   if (!success) return;

   while (gtk_tree_store_is_ancestor (store, parent_iter, &iter)) {
      gtk_tree_store_remove (store, &iter);
   }
}


/*
 *  fullpath shouldn't be terminated with "/" character.
 */
static void
insert_row (GtkTreeStore *store,
            GtkTreeIter *iter, GtkTreeIter *parent_iter,
            const gchar *label, const gchar *fullpath)
{
   GdkPixbuf *icon, *oicon;
   gchar *path, *tmpstr;
   gchar *label_internal;

   g_return_if_fail (store);
   g_return_if_fail (iter);
   g_return_if_fail (label && *label);
   g_return_if_fail (fullpath && *fullpath);

   path = remove_slash (fullpath);

   if (!strcmp (label, ".")) {
      icon  = go_folder;
      oicon = go_folder;
   } else if (!strcmp (label, "..")) {
      icon  = up_folder;
      oicon = up_folder;
   } else if (access(path, R_OK)) {
      icon  = lock_folder;
      oicon = lock_folder;
   } else if (islink(path)) {
      icon  = lfolder;
      oicon = lofolder;
   } else if (isdir(path)) {
      icon  = folder;
      oicon = ofolder;
   } else {
      return;
   }

   tmpstr = add_slash (path);
   g_free(path);
   path = tmpstr;

   /* convert charset */
   label_internal = charset_to_internal (label, conf.charset_filename,
                                         conf.charset_auto_detect_fn,
                                         conf.charset_filename_mode);

   gtk_tree_store_append (store, iter, parent_iter);
   gtk_tree_store_set (store, iter,
                       COLUMN_LABEL,      label_internal,
                       COLUMN_FULLPATH,   path,
                       COLUMN_ICON_OPEN,  oicon,
                       COLUMN_ICON_CLOSE, icon,
                       COLUMN_IS_DUMMY,   FALSE,
                       COLUMN_TERMINATOR);

   if (strcmp (label, ".") && strcmp (label, "..") && !access(path, R_OK))
      insert_dummy_row (store, iter);

   g_free (label_internal);
   g_free (path);
}


static gboolean
is_in_view (GtkTreeView *treeview, GtkTreePath *treepath)
{
   GdkRectangle widget_area, cell_area;

   if (!GTK_WIDGET_REALIZED (treeview))
      return FALSE;

   /* widget area */
   gtkutil_get_widget_area (GTK_WIDGET (treeview), &widget_area);

   gtk_tree_view_get_cell_area(treeview, treepath, NULL, &cell_area);

   if (cell_area.y >= 0 && cell_area.y < widget_area.height)
      return TRUE;
   else
      return FALSE;
}


static void
adjust_tree (GtkTreeView *treeview, GtkTreeIter *iter)
{
   GtkTreeModel *model;
   GtkTreePath *treepath;
   GtkTreeSelection *selection;
   GtkTreeIter tmp_iter;
   gboolean success;

   model = gtk_tree_view_get_model (treeview);
   selection = gtk_tree_view_get_selection (treeview);

   if (iter) {
      treepath = gtk_tree_model_get_path (model, iter);
      tmp_iter = *iter;
   } else {
      treepath = gtk_tree_path_new_from_string ("0");
      success = gtk_tree_model_get_iter (model, &tmp_iter, treepath);
      if (!success) goto FUNC_END;
   }

   gtk_tree_selection_select_path (selection, treepath);
   if (!is_in_view(treeview, treepath))
      gtk_tree_view_scroll_to_cell (treeview, treepath, NULL,
                                    TRUE, 0.0, 0.0);

 FUNC_END:
   gtk_tree_path_free (treepath);
   treepath = NULL;
}


static gboolean
get_iter_from_path (GimvDirView *dv, const gchar *str, GtkTreeIter *iter)
{
   GtkTreeModel *model;
   GtkTreeIter parent_iter, child_iter;
   GtkTreePath *treepath;
   GtkTreeSelection *selection;
   gchar *destpath;
   gint len_src, len_dest;
   gboolean go_next, retval = FALSE;

   g_return_val_if_fail (dv, FALSE);
   g_return_val_if_fail (iter, FALSE);

   destpath = add_slash (str);
   if (!destpath) return FALSE;

   len_src  = strlen (dv->root_dir);
   len_dest = strlen (destpath);

   if (len_dest < len_src || strncmp (dv->root_dir, destpath, len_src)) {
      goto ERROR;
   }

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dv->dirtree));

   go_next = gtk_tree_model_get_iter_from_string (model, &parent_iter, "0");
   if (!go_next) goto ERROR;
   treepath = gtk_tree_model_get_path (model, &parent_iter);

   if (!strcmp (destpath, dv->root_dir)) {
      gtk_tree_path_free (treepath);
      g_free (destpath);
      return gtk_tree_model_get_iter_first (model, iter);
   }

   gtk_tree_view_expand_row (GTK_TREE_VIEW (dv->dirtree), treepath, FALSE);
   gtk_tree_path_free (treepath);
   treepath = NULL;

   go_next = gtk_tree_model_get_iter_from_string (model, &parent_iter, "0:0");

   while (go_next) {
      gint len;
      gchar *path, *label;

      gtk_tree_model_get (model, &parent_iter,
                          COLUMN_LABEL,    &label,
                          COLUMN_FULLPATH, &path,
                          COLUMN_TERMINATOR);

      len = strlen (path); /* path will be terminated by "/" */

      /* It is just the directory! */
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         go_next = gtk_tree_model_iter_next (model, &parent_iter);
      } else if (!strcmp (destpath, path)) {
         *iter = parent_iter;
         go_next = FALSE;
         retval = TRUE;

      /* It is the parent directory of the destpath */
      } else if (!strncmp (destpath, path, len)) {
         treepath = gtk_tree_model_get_path (model, &parent_iter);
         go_next = gtk_tree_view_expand_row (GTK_TREE_VIEW (dv->dirtree), treepath,
                                             FALSE);
         go_next = gtk_tree_model_iter_children (model,
                                                 &child_iter,
                                                 &parent_iter);
         parent_iter = child_iter;

         gtk_tree_path_free (treepath);
         treepath = NULL;

      /* No match. Search the next... */
      } else {
         go_next = gtk_tree_model_iter_next (model, &parent_iter);
      }

      g_free (path);
      g_free (label);
   }


   g_free (destpath);
   return retval;

 ERROR:
   g_free (destpath);
   return FALSE;
}


struct AdjustTreeIdle {
   GimvDirView     *dv;
   GtkTreeView *treeview;
   gboolean     has_iter;
   GtkTreeIter  iter;
};


static gint
idle_adjust_tree (gpointer data)
{
   struct AdjustTreeIdle *idle = data;
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (idle->dv);

   priv->adjust_tree_id = 0;
   if (idle->has_iter)
      adjust_tree (idle->treeview, &idle->iter);
   else
      adjust_tree (idle->treeview, NULL);

   return FALSE;
}


static void
adjust_tree_idle (GimvDirView *dv, GtkTreeIter *iter)
{
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   GtkTreeView *treeview = GTK_TREE_VIEW (dv->dirtree);
   struct AdjustTreeIdle *idle;

   g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

   idle = g_new0(struct AdjustTreeIdle, 1);

   idle->dv = dv;
   idle->treeview = treeview;
   if (iter) {
      idle->iter = *iter;
      idle->has_iter = TRUE;
   } else {
      idle->has_iter = FALSE;
   }

   priv->adjust_tree_id = 
      gtk_idle_add_full (G_PRIORITY_DEFAULT,
                         idle_adjust_tree, NULL, idle,
                         (GtkDestroyNotify) g_free);
}


static void
get_expanded_dirs (GtkTreeView *treeview, GtkTreePath *treepath, gpointer data)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   GList **list;
   gchar *path;

   g_return_if_fail (data);
   list = data;

   model = gtk_tree_view_get_model (treeview);

   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   g_return_if_fail (path && *path);

   *list = g_list_append (*list, path);
}


static void
refresh_dir_tree (GimvDirView *dv, GtkTreeIter *parent_iter)
{
   GtkTreeView *treeview;
   GtkTreeStore *store;
   GtkTreeIter root_iter, iter;
   GtkTreePath *treepath;
   gchar *root_dir, *selected_path;
   GList *expand_list = NULL, *node;
   gboolean selected;

   g_return_if_fail (dv);

   selected_path = gimv_dir_view_get_selected_path (dv);
   if (!selected_path) selected_path = g_strdup (dv->root_dir);

   /* get expanded directory list */
   gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (dv->dirtree),
                                    get_expanded_dirs,
                                    &expand_list);

   /* replace root node */
   root_dir = g_strdup (dv->root_dir);
#if 1 /* almost same with gimv_dir_view_chroot () */
   g_free (dv->root_dir);
   dv->root_dir = add_slash (root_dir);

   treeview = GTK_TREE_VIEW (dv->dirtree);

   store = GTK_TREE_STORE (gtk_tree_view_get_model (treeview));
   gtk_tree_store_clear (store);
   insert_row (store, &root_iter, NULL, dv->root_dir, dv->root_dir);

   treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &root_iter);

   gtk_tree_path_free (treepath);
#endif
   g_free (root_dir);

   /* restore expanded directory */
   for (node = expand_list; node; node = g_list_next (node)) {
      gimv_dir_view_expand_dir (dv, node->data, FALSE);
   }

   g_list_foreach (expand_list, (GFunc) g_free, NULL);
   g_list_free (expand_list);

   /* adjust tree pos */
   selected = get_iter_from_path (dv, selected_path, &iter);
   if (selected)
      adjust_tree_idle (dv, &iter);

   g_free (selected_path);
}


static void
dirview_popup_menu (GimvDirView *dv, GdkEventButton *event)
{
   GtkTreeModel *model;
   GtkTreePath *treepath;
   GtkTreeIter iter;
   gboolean success;
   gchar *path, *parent, *tmpstr, *label;

   GtkItemFactory *ifactory;
   GtkWidget *dirview_popup, *menuitem;
   gint n_menu_items;
   guint button;
   guint32 time;
   GtkMenuPositionFunc pos_fn = NULL;

   g_return_if_fail (dv);

   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (dv->dirtree),
                                            event->x, event->y,
                                            &treepath, NULL, NULL, NULL);
   if (!success) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_LABEL,    &label,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   tmpstr = remove_slash (path);
   parent = g_dirname (tmpstr);
   g_free (tmpstr);

   if (event) {
      button = event->button;
      time = event->time;
   } else {
      button = 0;
      time = GDK_CURRENT_TIME;
      pos_fn = menu_calc_popup_position;
   }

   if (dv->popup_menu) {
      gtk_widget_unref (dv->popup_menu);
      dv->popup_menu = NULL;
   }

   n_menu_items = sizeof(dirview_popup_items)
      / sizeof(dirview_popup_items[0]) -  1;
   dirview_popup = menu_create_items(NULL, dirview_popup_items,
                                     n_menu_items, "<GimvDirViewPop>", dv);


   /* set sensitive */
   ifactory = gtk_item_factory_from_widget (dirview_popup);

   if (!strcmp (label, ".") || !strcmp (label, ".."))
   {
      menuitem = gtk_item_factory_get_item (ifactory, "/Refresh Tree");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Make Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   if (!iswritable (path)) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Make Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   if (!parent || !strcmp (parent, ".") || !iswritable (parent)
       || !strcmp (label, ".") || !strcmp (label, ".."))
   {
      menuitem = gtk_item_factory_get_item (ifactory, "/Rename Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Delete Directory...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   gtk_tree_path_free (treepath);
   g_free (label);
   g_free (path);
   g_free (parent);

   /* popup menu */
   gtk_menu_popup(GTK_MENU (dirview_popup), NULL, NULL,
                  pos_fn, dv->dirtree->window, button, time);

   dv->popup_menu = dirview_popup;

   g_object_ref (G_OBJECT (dv->popup_menu));
   gtk_object_sink (GTK_OBJECT (dv->popup_menu));
}


typedef struct ButtonActionData_Tag
{
   GimvDirView *dv;
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
   GimvDirView *dv = bdata->dv;
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   gchar *path = bdata->path, *label = bdata->label;

   priv->button_action_id = 0;

   switch (abs (bdata->action_num)) {
   case MouseActLoadThumb:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         gimv_dir_view_chroot (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR_NONE);
      }
      break;
   case MouseActLoadThumbRecursive:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         gimv_dir_view_chroot (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR);
      }
      break;
   case MouseActLoadThumbRecursiveInOneTab:
      if (!strcmp (label, ".") || !strcmp (label, "..")) {
         gimv_dir_view_chroot (dv, path);
      } else {
         open_dir_images (path, dv->tw, NULL, LOAD_CACHE, SCAN_SUB_DIR_ONE_TAB);
      }
      break;
   case MouseActChangeTop:
      gimv_dir_view_chroot (dv, path);
      break;
   default:
      break;
   }

   free_button_action_data (bdata);

   return FALSE;
}


static gboolean
dirview_button_action (GimvDirView *dv, GdkEventButton *event, gint num)
{
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   gchar *path = NULL, *label;
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   gboolean success, retval = FALSE;
   GtkTreePath *treepath;
   GtkTreeViewColumn *treecolumn;
   gint cell_x, cell_y;
   GtkTreeIter iter;

   success = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (dv->dirtree),
                                            event->x, event->y,
                                            &treepath, &treecolumn,
                                            &cell_x, &cell_y);
   if (!success) return FALSE;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dv->dirtree));
   gtk_tree_selection_select_path (selection, treepath);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   gtk_tree_model_get_iter (model, &iter, treepath);
   gtk_tree_model_get (model, &iter,
                       COLUMN_LABEL,    &label,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   switch (abs(num)) {
   case MouseActLoadThumb:
   case MouseActLoadThumbRecursive:
   case MouseActLoadThumbRecursiveInOneTab:
   case MouseActChangeTop:
   {
      ButtonActionData *data = g_new0 (ButtonActionData, 1);

      data->dv         = dv;
      data->path       = path;
      data->label      = label;
      data->action_num = num;

      priv->button_action_id
         = gtk_idle_add (idle_dirview_button_action, data);

      gtk_tree_path_free (treepath);

      return FALSE;
      break;
   }
   case MouseActPopupMenu:
      dirview_popup_menu (dv, event);
      if (num > 0) retval = TRUE;
      break;
   default:
      break;
   }

   g_free (path);
   g_free (label);
   gtk_tree_path_free (treepath);

   return retval;
}



/******************************************************************************
 *
 *   Public  functions
 *
 ******************************************************************************/
void
gimv_dir_view_chroot (GimvDirView *dv, const gchar *root_dir)
{
   GtkTreeView *treeview;
   GtkTreeStore *store;
   GtkTreeIter root_iter;
   GtkTreePath *treepath;
   gchar *dest_dir;

   g_return_if_fail (dv);
   g_return_if_fail (root_dir && *root_dir);

   dest_dir = add_slash (root_dir);

   if (!isdir (dest_dir)) {
      g_free (dest_dir);
      return;
   }

   g_free (dv->root_dir);
   dv->root_dir = dest_dir;

   treeview = GTK_TREE_VIEW (dv->dirtree);

   store = GTK_TREE_STORE (gtk_tree_view_get_model (treeview));
   gtk_tree_store_clear (store);
   insert_row (store, &root_iter, NULL, dv->root_dir, dv->root_dir);

   treepath = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &root_iter);
   gtk_tree_view_expand_row (treeview, treepath, FALSE);

   gtk_tree_path_free (treepath);

   adjust_tree_idle (dv, NULL);
}


void
gimv_dir_view_chroot_to_parent (GimvDirView *dv)
{
   gchar *end;
   gchar *root;

   g_return_if_fail (dv);

   root = g_strdup (dv->root_dir);
   end = strrchr (root, '/');
   if (end && end != root) *end = '\0';
   end = strrchr (root, '/');
   if (end) *(end + 1) = '\0';

   gimv_dir_view_change_dir (dv, root);
   g_free (root);
}


void
gimv_dir_view_change_dir (GimvDirView *dv, const gchar *str)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gchar *destpath;
  gboolean success;

   g_return_if_fail (dv);

   destpath = add_slash (str);
   if (!destpath) return;

   if (!isdir (destpath)) {
      g_free (destpath);
      return;
   }

   /* FIXME */
   /* if selected path in directory view is same as str, adjust to it */
   /* END FIXME */

   success = get_iter_from_path (dv, destpath, &iter);

   if (success) {
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
      adjust_tree_idle (dv, &iter);
   } else {
      gimv_dir_view_chroot (dv, str);
   }

   g_free (destpath);
}


void
gimv_dir_view_go_home (GimvDirView *dv)
{
   const gchar *home = g_getenv ("HOME");

   gimv_dir_view_chroot (dv, home);
   gimv_dir_view_change_dir (dv, home);
}


void
gimv_dir_view_refresh_list (GimvDirView *dv)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean success;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   success = gtk_tree_model_get_iter_first (model, &iter);
   if (!success) return;

   refresh_dir_tree (dv, &iter);
}


gchar *
gimv_dir_view_get_selected_path (GimvDirView *dv)
{
   gboolean success;
   GtkTreeSelection *selection;
   GtkTreeModel *model;
   GtkTreeIter iter;
   gchar *path;

   g_return_val_if_fail (dv, NULL);
   g_return_val_if_fail (dv->dirtree, NULL);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dv->dirtree));
   g_return_val_if_fail (selection, NULL);

   success = gtk_tree_selection_get_selected (selection, &model, &iter);
   if (!success) return NULL;

   gtk_tree_model_get (model, &iter,
                       COLUMN_FULLPATH, &path,
                       COLUMN_TERMINATOR);

   return path;
}


void
gimv_dir_view_expand_dir (GimvDirView *dv, const gchar *dir, gboolean open_all)
{
   GtkTreeIter iter;
   GtkTreeModel *model;
   GtkTreePath *treepath;
   gboolean success;

   success = get_iter_from_path (dv, dir, &iter);
   if (!success) return;

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (dv->dirtree));
   treepath = gtk_tree_model_get_path (model, &iter);

   gtk_tree_view_expand_row (GTK_TREE_VIEW (dv->dirtree), treepath, open_all);

   gtk_tree_path_free (treepath);
}


gboolean
gimv_dir_view_set_opened_mark (GimvDirView *dv, const gchar *path)
{
   /* note implemented yet */
   return TRUE;
}


gboolean
gimv_dir_view_unset_opened_mark (GimvDirView *dv, const gchar *path)
{
   /* note implemented yet */
   return TRUE;
}


void
gimv_dir_view_show_toolbar (GimvDirView *dv)
{
   g_return_if_fail (dv);

   dv->show_toolbar = TRUE;
   gtk_widget_show (dv->toolbar_eventbox);
}


void
gimv_dir_view_hide_toolbar (GimvDirView *dv)
{
   g_return_if_fail (dv);

   dv->show_toolbar = FALSE;
   gtk_widget_hide (dv->toolbar_eventbox);
}


static void
gimv_dir_view_class_init (GimvDirViewClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass *) klass;

   gobject_class->dispose = gimv_dir_view_dispose;

   g_type_class_add_private (gobject_class, sizeof (GimvDirViewPrivate));
}


static void
gimv_dir_view_init (GimvDirView *dv)
{
   GimvDirViewPrivate *priv = GIMV_DIR_VIEW_GET_PRIVATE (dv);
   GtkWidget *eventbox;
   const gchar *home = g_getenv ("HOME");

   dv->root_dir     = add_slash (home);
   dv->dirtree      = NULL;
   dv->popup_menu   = NULL;
   dv->tw           = NULL;
   dv->show_toolbar = conf.dirview_show_toolbar;
   dv->show_dotfile = conf.dirview_show_dotfile;

   priv->hilit_dir         = -1;
   priv->scroll_timer_id   = 0;
   priv->drag_tree_row     = NULL;
   priv->button_action_id  = 0;
   priv->swap_com_id       = 0;
   priv->adjust_tree_id    = 0;

   /* main vbox */
   gtk_widget_set_name (GTK_WIDGET (dv), "GimvDirView");
   gtk_widget_show (GTK_WIDGET (dv));

   dnd_dest_set (GTK_WIDGET (dv), dnd_types_component, dnd_types_component_num);
   g_object_set_data (G_OBJECT (dv),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_DIR_VIEW));
   g_signal_connect (G_OBJECT (dv), "drag_data_received",
                     G_CALLBACK (cb_com_swap_drag_data_received), NULL);

   /* toolbar */
   eventbox = dv->toolbar_eventbox = gtk_event_box_new ();
   gtk_container_set_border_width (GTK_CONTAINER (eventbox), 1);
   gtk_box_pack_start (GTK_BOX (dv), eventbox, FALSE, FALSE, 0);
   gtk_widget_show (eventbox);

   dv->toolbar = dirview_create_toolbar (dv);
   gtk_container_add (GTK_CONTAINER (eventbox), dv->toolbar);

   if (!dv->show_toolbar)
      gtk_widget_hide (dv->toolbar_eventbox);

   dnd_src_set  (eventbox, dnd_types_component, dnd_types_component_num);
   g_object_set_data (G_OBJECT (eventbox),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_DIR_VIEW));
   g_signal_connect (G_OBJECT (eventbox), "drag_begin",
                     G_CALLBACK (cb_toolbar_drag_begin), dv);
   g_signal_connect (G_OBJECT (eventbox), "drag_data_get",
                     G_CALLBACK (cb_toolbar_drag_data_get), dv);

   /* scrolled window */
   dv->scroll_win = gtk_scrolled_window_new (NULL, NULL);
   gtk_container_set_border_width (GTK_CONTAINER (dv->scroll_win), 1);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dv->scroll_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dv->scroll_win),
                                       GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(dv), dv->scroll_win, TRUE, TRUE, 0);
   gtk_widget_show (dv->scroll_win);

   /* ctree */
   dirview_create_treeview (dv, dv->root_dir);
}


GtkWidget *
gimv_dir_view_new (const gchar *root_dir, GimvThumbWin *tw)
{
   GimvDirView *dv = GIMV_DIR_VIEW (g_object_new (GIMV_TYPE_DIR_VIEW, NULL));
   dv->tw = tw;
   if (root_dir)
      gimv_dir_view_chroot(dv, root_dir);
   return GTK_WIDGET (dv);
}
