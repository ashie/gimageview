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

#include "detailview.h"

#include <time.h>
#include <string.h>

#include "dnd.h"
#include "detailview_priv.h"
#include "detailview_prefs.h"
#include "fileutil.h"
#include "gimv_image_info.h"
#include "gimv_plugin.h"
#include "gimv_thumb.h"
#include "gimv_thumb_view.h"

static GimvThumbViewPlugin detailview_modes[] =
{
   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail"),
    100,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail + Icon"),
    110,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail + Thumbnail"),
    120,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},
};

GIMV_PLUGIN_GET_IMPL(detailview_modes, GIMV_PLUGIN_THUMBVIEW_EMBEDER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Thumbnail View Detail Mode"),
   version:       "0.0.2",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  gimv_prefs_ui_detailview_get_page,
};


/* for setting clist text data */
static gchar *column_data_filename   (GimvThumb *thumb);
static gchar *column_data_image_size (GimvThumb *thumb);
static gchar *column_data_size       (GimvThumb *thumb);
static gchar *column_data_type       (GimvThumb *thumb);
static gchar *column_data_atime      (GimvThumb *thumb);
static gchar *column_data_mtime      (GimvThumb *thumb);
static gchar *column_data_ctime      (GimvThumb *thumb);
static gchar *column_data_uid        (GimvThumb *thumb);
static gchar *column_data_gid        (GimvThumb *thumb);
static gchar *column_data_mode       (GimvThumb *thumb);
static gchar *column_data_cache      (GimvThumb *thumb);


DetailViewColumn detailview_columns [] =
{
   {NULL,                    0,   FALSE, NULL,                  GTK_JUSTIFY_CENTER, FALSE},
   {N_("Name"),              100, TRUE,  column_data_filename,  GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Size (byte)"),       100, TRUE,  column_data_size,      GTK_JUSTIFY_RIGHT,  FALSE},
   {N_("Type"),              100, TRUE,  column_data_type,      GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Cache type"),        100, FALSE, column_data_cache,     GTK_JUSTIFY_LEFT,   TRUE},
   {N_("Access time"),       150, TRUE,  column_data_atime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Modification time"), 150, TRUE,  column_data_mtime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Change time"),       150, TRUE,  column_data_ctime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("User"),              40,  TRUE,  column_data_uid,       GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Group"),             40,  TRUE,  column_data_gid,       GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Mode"),              100, TRUE,  column_data_mode,      GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Image size"),        100, TRUE,  column_data_image_size,GTK_JUSTIFY_CENTER, TRUE},
};
gint detailview_columns_num = sizeof (detailview_columns) / sizeof (DetailViewColumn);


GtkTargetEntry detailview_dnd_targets[] = {
   {"text/uri-list", 0, TARGET_URI_LIST},
};
const gint detailview_dnd_targets_num = sizeof(detailview_dnd_targets) / sizeof(GtkTargetEntry);


GList *detailview_title_idx_list     = NULL;
gint   detailview_title_idx_list_num = 0;


#ifndef ENABLE_TREEVIEW

#include "dnd.h"
#include "gimv_image.h"
#include "gimv_thumb.h"
#include "gimv_thumb_win.h"
#include "gtk2-compat.h"

typedef struct DetailViewData_Tag
{
   GtkWidget       *clist;
   gint             page_pos_x[3];
   gint             page_pos_y[3];
   const gchar     *dest_mode;
   gint             hilit_dir;
} DetailViewData;

/* callback functions */
static gboolean cb_clist_button_press (GtkWidget      *widget,
                                       GdkEventButton *event,
                                       gpointer        data);
static gboolean cb_clist_motion_notify(GtkWidget      *widget,
                                       GdkEventMotion *event,
                                       gpointer        data);
static void     cb_click_column       (GtkWidget      *widget,
                                       gint            col,
                                       GimvThumbView  *tv);

/* other private function */
static DetailViewData *detailview_new (GimvThumbView  *tv);
static void detailview_set_pixmaps    (GimvThumbView  *tv,
                                       const gchar    *dest_mode);

static gboolean  detailview_dragging = FALSE;

#endif /* ENABLE_TREEVIEW */


/******************************************************************************
 *
 *   for title index list.
 *
 ******************************************************************************/
void
detailview_create_title_idx_list (void)
{
   static gchar *config_columns_string = NULL;
   gchar **titles, *data_order;
   gint i = 0;

   detailview_prefs_get_value("data_order", (gpointer) &data_order);
 
   if (!data_order) {
      config_columns_string = NULL;
      if (detailview_title_idx_list)
         g_list_free (detailview_title_idx_list);
      detailview_title_idx_list_num = 0;
      return;
   }

   if (data_order == config_columns_string) return;

   if (detailview_title_idx_list) g_list_free (detailview_title_idx_list);
   detailview_title_idx_list = NULL;

   titles = g_strsplit (data_order, ",", -1);

   g_return_if_fail (titles);

   detailview_title_idx_list_num = 0;
   config_columns_string = data_order;

   while (titles[i]) {
      gint idx;
      idx = detailview_get_title_idx (titles[i]);
      if (idx > 0) {
         detailview_title_idx_list
            = g_list_append (detailview_title_idx_list, GINT_TO_POINTER (idx));
         detailview_title_idx_list_num++;
      }
      i++;
   }

   g_strfreev (titles);
}


gint
detailview_get_titles_num (void)
{
   return detailview_columns_num;
}


gchar *
detailview_get_title (gint idx)
{
   g_return_val_if_fail (idx > 0 && idx < detailview_columns_num, NULL);

   return detailview_columns[idx].title;
}


gint
detailview_get_title_idx (const gchar *title)
{
   gint i;

   g_return_val_if_fail (title, -1);

   for (i = 1; i < detailview_columns_num; i++) {
      if (!detailview_columns[i].title) continue;
      if (!strcmp (detailview_columns[i].title, title))
         return i;
   }

   return -1;
}


void
detailview_apply_config (void)
{
   GList *node;

   detailview_create_title_idx_list ();

   node = gimv_thumb_view_get_list();

   while (node) {
      GimvThumbView *tv = node->data;

      if (!strcmp (tv->summary_mode, DETAIL_VIEW_LABEL)
          || !strcmp (tv->summary_mode, DETAIL_ICON_LABEL)
          || !strcmp (tv->summary_mode, DETAIL_THUMB_LABEL))
      {
         gimv_thumb_view_set_widget (tv, tv->tw, tv->container, tv->summary_mode);
      }

      node = g_list_next (node);
   }
}



/******************************************************************************
 *
 *   for creating column text.
 *
 ******************************************************************************/
static gchar *
column_data_filename (GimvThumb *thumb)
{
   GimvThumbView *tv;

   if (!thumb) return NULL;
   tv = gimv_thumb_get_parent_thumbview (thumb);

   /*
   if (tv->mode == THUMB_VIEW_MODE_DIR)
      return g_strdup (g_basename (thumb->info->filename));
   else
      return g_strdup (thumb->info->filename);
   */

   {
      const gchar *filename;
      gchar *retval;

      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR)
         filename = g_basename (gimv_image_info_get_path (thumb->info));
      else
         filename = gimv_image_info_get_path (thumb->info);

      retval = gimv_filename_to_internal (filename);

      return retval;
   }
}


static gchar *
column_data_image_size (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   if (thumb->info->width > 0 && thumb->info->width > 0)
      return g_strdup_printf ("%d x %d", thumb->info->width, thumb->info->height);
   else
      return g_strdup (_("Unknwon"));

}


static gchar *
column_data_size (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_size2str (thumb->info->st.st_size, FALSE);
}


static gchar *
column_data_type (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return g_strdup (gimv_image_detect_type_by_ext (thumb->info->filename));
}


static gchar *
column_data_atime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_atime);
}


static gchar *
column_data_mtime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_mtime);
}


static gchar *
column_data_ctime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_ctime);
}


static gchar *
column_data_uid (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_uid2str (thumb->info->st.st_uid);
}


static gchar *
column_data_gid (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_gid2str (thumb->info->st.st_gid);
}


static gchar *
column_data_mode (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_mode2str (thumb->info->st.st_mode);
}


static gchar *
column_data_cache (GimvThumb *thumb)
{
   const gchar *cache_type;

   if (!thumb) return NULL;

   cache_type = gimv_thumb_get_cache_type (thumb);

   return (gchar *) cache_type;
}



/*
 *   For Gtk+-1.2
 */

#ifndef ENABLE_TREEVIEW

/******************************************************************************
 *
 *   Callback functions.
 *
 ******************************************************************************/
static gint
cb_clist_key_press (GtkWidget *widget,
                    GdkEventKey *event,
                    GimvThumbView *tv)
{
   GimvThumb *thumb = NULL;

   g_return_val_if_fail (tv, FALSE);

   if (GTK_CLIST (widget)->focus_row > 0)
      thumb = gtk_clist_get_row_data (GTK_CLIST (widget),
                                      GTK_CLIST (widget)->focus_row);

   if (gimv_thumb_view_thumb_key_press_cb(widget, event, thumb))
      return FALSE;

   {
      switch (event->keyval) {
      case GDK_Left:
      case GDK_Right:
      case GDK_Up:
      case GDK_Down:
#ifdef USE_GTK2
         return FALSE;
#else /* USE_GTK2 */
         return TRUE;
#endif /* USE_GTK2 */
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
cb_clist_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   GimvThumbView *tv = data;
   gint row;
   GimvThumb *thumb;
   gboolean clear = TRUE, found = FALSE, success;

   detailview_dragging = FALSE;

   g_return_val_if_fail (tv, FALSE);

   gtk_widget_grab_focus (widget);

   success = gtk_clist_get_selection_info (GTK_CLIST (widget),
                                           event->x, event->y,
                                           &row, NULL);

   if (event->button != 4 && event->button != 5) {
      GTK_CLIST (widget)->anchor = row;
      GTK_CLIST (widget)->focus_row = row;
#ifdef USE_GTK2
      GTK_CLIST_GET_CLASS (GTK_CLIST (widget))->refresh (GTK_CLIST (widget));
#else /* USE_GTK2 */
      GTK_CLIST_CLASS (GTK_OBJECT (widget)->klass)->refresh (GTK_CLIST (widget));
#endif /* USE_GTK2 */
   }

   if (!success) {
      gimv_thumb_view_set_selection_all (tv, FALSE);
      return FALSE;
   }

   thumb = gtk_clist_get_row_data (GTK_CLIST (widget), row);
   if (!thumb) return FALSE;

   /* set selection */
   if (event->type == GDK_BUTTON_PRESS && (event->button == 1)) {
      if (event->state & GDK_SHIFT_MASK) {
         if (event->state & GDK_CONTROL_MASK)
            clear = FALSE;

         found = gimv_thumb_view_set_selection_multiple (tv, thumb,
                                                         TRUE, clear);
         if (!found)
            gimv_thumb_view_set_selection_multiple (tv, thumb, FALSE, clear);
      } else if (!thumb->selected) {
         if (~event->state & GDK_CONTROL_MASK)
            gimv_thumb_view_set_selection_all (tv, FALSE);
         gimv_thumb_view_set_selection (thumb, TRUE);
      } else if (thumb->selected) {
         if (event->state & GDK_CONTROL_MASK)
            gimv_thumb_view_set_selection (thumb, FALSE);
      }
   }

   return gimv_thumb_view_thumb_button_press_cb (widget, event, thumb);
}


static gboolean
cb_clist_button_release (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   GimvThumbView *tv = data;
   gint row;
   GimvThumb *thumb;
   gboolean success;

   if (!tv) goto ERROR;

   success = gtk_clist_get_selection_info (GTK_CLIST (widget),
                                           event->x, event->y,
                                           &row, NULL);

   if (event->button != 4 && event->button != 5) {
      GTK_CLIST (widget)->anchor = row;
      GTK_CLIST (widget)->focus_row = row;
#ifdef USE_GTK2
      GTK_CLIST_GET_CLASS (GTK_CLIST (widget))->refresh (GTK_CLIST (widget));
#else /* USE_GTK2 */
      GTK_CLIST_CLASS (GTK_OBJECT (widget)->klass)->refresh (GTK_CLIST (widget));
#endif /* USE_GTK2 */
   }

   if (!success) {
      gimv_thumb_view_set_selection_all (tv, FALSE);
      goto ERROR;
   }

   thumb = gtk_clist_get_row_data (GTK_CLIST (widget), row);
   if (!thumb) goto ERROR;

   if (event->type == GDK_BUTTON_RELEASE
       && (event->button == 1)
       && !(event->state & GDK_SHIFT_MASK)
       && !(event->state & GDK_CONTROL_MASK)
       && !detailview_dragging)
   {
      gimv_thumb_view_set_selection_all (tv, FALSE);
      gimv_thumb_view_set_selection (thumb, TRUE);
   }

   return gimv_thumb_view_thumb_button_release_cb (widget, event, thumb);

ERROR:
   detailview_dragging = FALSE;
   return FALSE;
}


static gboolean
cb_clist_motion_notify (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   GimvThumbView *tv = data;
   gint row, on_row;
   GimvThumb *thumb = NULL;

   detailview_dragging = TRUE;

   g_return_val_if_fail (tv, FALSE);

   on_row = gtk_clist_get_selection_info (GTK_CLIST (widget),
                                          event->x, event->y, &row, NULL);
   if (!on_row) {
      thumb = gtk_clist_get_row_data (GTK_CLIST (widget), row);
   }

   return gimv_thumb_view_motion_notify_cb (widget, event, thumb);
}


static void
cb_click_column(GtkWidget *widget, gint col, GimvThumbView *tv)
{
   GimvThumbWin *tw;
   GtkWidget *sort_item;
   GList *node;
   gint idx;

   if (!tv || tv->progress) return;

   tw = tv->tw;
   if (!tw) return;

   node = g_list_nth (detailview_title_idx_list, col - 1);
   if (!node) return;

   idx = GPOINTER_TO_INT (node->data);

   switch (idx) {
   case 1:
      sort_item = tw->menuitem.sort_name;
      break;
   case 2:
      sort_item = tw->menuitem.sort_size;
      break;
   case 3:
      sort_item = tw->menuitem.sort_type;
      break;
   case 5:
      sort_item = tw->menuitem.sort_access;
      break;
   case 6:
      sort_item = tw->menuitem.sort_time;
      break;
   case 7:
      sort_item = tw->menuitem.sort_change;
      break;
   default:
      return;
   }

   if (!sort_item) return;

   if (GTK_CHECK_MENU_ITEM(sort_item)->active) {
      gtk_check_menu_item_set_active
         (GTK_CHECK_MENU_ITEM(tw->menuitem.sort_rev),
          !GTK_CHECK_MENU_ITEM(tw->menuitem.sort_rev)->active);
   } else {
      gtk_check_menu_item_set_active
         (GTK_CHECK_MENU_ITEM(sort_item), TRUE);
   }
}


static void
cb_select_row (GtkCList *clist, gint row, gint column,
               GdkEventButton *event, gpointer user_data)
{
   GimvThumbView *tv = user_data;
   GimvThumb *thumb;
   GList *node;

   g_return_if_fail (tv);
   if (!tv->thumblist) return;

   node = g_list_nth (tv->thumblist, row);
   thumb = node->data;

   if (thumb) {
      thumb->selected = TRUE;
   }
}


static void
cb_unselect_row (GtkCList *clist, gint row, gint column,
                 GdkEventButton *event, gpointer user_data)
{
   GimvThumbView *tv = user_data;
   GimvThumb *thumb;
   GList *node;

   g_return_if_fail (tv);
   if (!tv->thumblist) return;

   node = g_list_nth (tv->thumblist, row);
   thumb = node->data;

   if (thumb) {
      thumb->selected = FALSE;
   }
}


#if 0
static void
cb_drag_motion (GtkWidget *clist, GdkDragContext *context,
                gint x, gint y, gint time, gpointer data)
{
   ThumbView *tv = data;
   DetailViewData *tv_data;
   GimvThumb *thumb;
   gint row, on_row;

   g_return_if_fail (tv);

   on_row = gtk_clist_get_selection_info (GTK_CLIST (clist),
                                          x, y, &row, NULL);
   if (!on_row) return;

   thumb = gtk_clist_get_row_data (GTK_CLIST (clist), row);
   if (!thumb) return;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) return;

   /* hilight row under cursor */
   if (thumb && gimv_thumb_is_dir (thumb) && tv_data->hilit_dir != row) {
      if (tv_data->hilit_dir != -1) {
         gtk_clist_set_foreground (GTK_CLIST (clist), tv_data->hilit_dir,
                                   &(clist->style->fg[GTK_STATE_NORMAL]));
         gtk_clist_set_background (GTK_CLIST (clist), tv_data->hilit_dir,
                                   &(clist->style->base[GTK_STATE_NORMAL]));
      }

      tv_data->hilit_dir = row;

      gtk_clist_set_foreground (GTK_CLIST (clist), tv_data->hilit_dir,
                                &(clist->style->fg[GTK_STATE_SELECTED]));
      gtk_clist_set_background (GTK_CLIST (clist), tv_data->hilit_dir,
                                &(clist->style->bg[GTK_STATE_SELECTED]));
   }
}


static void
cb_drag_leave (GtkWidget *clist, GdkDragContext *context, guint time,
               gpointer data)
{
   ThumbView *tv = data;
   DetailViewData *tv_data;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) return;

   gtk_clist_set_foreground (GTK_CLIST (clist), tv_data->hilit_dir,
                             &(clist->style->fg[GTK_STATE_NORMAL]));
   gtk_clist_set_background (GTK_CLIST (clist), tv_data->hilit_dir,
                             &(clist->style->base[GTK_STATE_NORMAL]));
   tv_data->hilit_dir = -1;
}
#endif


static void
cb_drag_data_received (GtkWidget *clist,
                       GdkDragContext *context,
                       gint x, gint y,
                       GtkSelectionData *seldata,
                       guint info,
                       guint time,
                       gpointer data)
{
#if 0
   ThumbView *tv = data;
   GimvThumb *thumb;
   gint row, on_row;

   g_return_if_fail (tv);

   on_row = gtk_clist_get_selection_info (GTK_CLIST (clist),
                                          x, y, &row, NULL);
   if (!on_row) return;

   thumb = gtk_clist_get_row_data (GTK_CLIST (clist), row);
   if (!thumb) return;

   if (gimv_thumb_is_dir (thumb))
      tv->dnd_destdir = gimv_thumb_get_image_name (thumb);
#endif

   gimv_thumb_view_drag_data_received_cb (clist, context, x, y,
                                          seldata, info, time, data);
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
   gint i, num;

   g_return_val_if_fail (tv, NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) {
      tv_data = g_new0 (DetailViewData, 1);
      tv_data->clist = NULL;
      num = sizeof (tv_data->page_pos_x) / sizeof (tv_data->page_pos_x[0]);
      for (i = 0; i < num; i++) {
         tv_data->page_pos_x[i] = 0;
         tv_data->page_pos_y[i] = 0;
      }
      tv_data->hilit_dir = -1;
      gtk_object_set_data_full (GTK_OBJECT (tv), DETAIL_VIEW_LABEL, tv_data,
                                (GtkDestroyNotify) g_free);
   }

   return tv_data;
}


static void
detailview_set_pixmaps (GimvThumbView *tv, const gchar *dest_mode)
{
   GimvThumb *thumb;
   GList *node;
   gint i, num;

   if (!tv) return;

   node = g_list_first (tv->thumblist);
   num = g_list_length (node);

   /* set images */
   for ( i = 0 ; i < num ; i++ ) {
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
   DetailViewData *tv_data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->clist);

   gtk_clist_freeze (GTK_CLIST (tv_data->clist));
}


void
detailview_thaw (GimvThumbView *tv)
{
   DetailViewData *tv_data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->clist);

   gtk_clist_thaw (GTK_CLIST (tv_data->clist));
}


void
detailview_append_thumb_frame (GimvThumbView *tv, GimvThumb *thumb,
                               const gchar *dest_mode)
{
   DetailViewData *tv_data;
   GList *data_node;
   gint j, colnum;
   gchar **image_data;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->clist);

   colnum = detailview_title_idx_list_num + 1;
   image_data = g_malloc0 (sizeof (gchar *) * colnum);

   pos = g_list_index (tv->thumblist, thumb);

   image_data[0] = NULL;
   data_node = detailview_title_idx_list;
   for (j = 1; j < colnum && data_node; j++) {
      gint idx = GPOINTER_TO_INT (data_node->data);
      if (detailview_columns[idx].func) {
         image_data[j] = detailview_columns[idx].func (thumb);
      } else {
         image_data[j] = NULL;
      }
      data_node = g_list_next (data_node);
   }

   gtk_clist_insert (GTK_CLIST (tv_data->clist), pos, image_data);
   gtk_clist_set_row_data (GTK_CLIST (tv_data->clist), pos, thumb);

   if (thumb->selected)
      gtk_clist_select_row (GTK_CLIST (tv_data->clist), pos, -1);
   else
      gtk_clist_unselect_row (GTK_CLIST (tv_data->clist), pos, -1);

   data_node = detailview_title_idx_list;
   for (j = 1; j < colnum && data_node; j++) {
      gint idx = GPOINTER_TO_INT (data_node->data);
      if (detailview_columns[idx].free && image_data[j]) {
         g_free (image_data[j]);
      }
      image_data[j] = NULL;
      data_node = g_list_next (data_node);
   }

   g_free (image_data);
}


void
detailview_update_thumbnail (GimvThumbView *tv, GimvThumb *thumb,
                             const gchar *dest_mode)
{
   DetailViewData *tv_data;
   GdkPixmap *pixmap = NULL;
   GdkBitmap *mask;
   GList *node;
   gint pos, row, col, i;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->clist);

   pos = g_list_index (tv->thumblist, thumb);

   row = pos;
   col = 0;

   /* set thumbnail */
   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gimv_thumb_get_icon (thumb, &pixmap, &mask);
   } else if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   }

   if (!pixmap) return;

   gtk_clist_set_pixmap (GTK_CLIST (tv_data->clist),
                         row, col, pixmap, mask);

   /* reset column data */
   node = detailview_title_idx_list;
   for (i = 1, node = detailview_title_idx_list;
        node;
        i++, node = g_list_next (node))
   {
      gint idx = GPOINTER_TO_INT (node->data);
      gchar *str;

      if (!detailview_columns[idx].need_sync) continue;

      str = detailview_columns[idx].func (thumb);

      gtk_clist_set_text (GTK_CLIST (tv_data->clist),
                          row, i,
                          str);

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
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data);

   pos = g_list_index (tv->thumblist, thumb);
   if (pos < 0) return;

   gtk_clist_remove (GTK_CLIST (tv_data->clist), pos);
}


void
detailview_adjust (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   GList *node;
   gint pos;

   g_return_if_fail (tv);

   node = g_list_find (gimv_thumb_view_get_list(), tv);
   if (!node) return;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data);

   node = g_list_find (tv->thumblist, thumb);
   if (!node) return;
   pos = g_list_position (tv->thumblist, node);

   gtk_clist_moveto (GTK_CLIST (tv_data->clist), pos, 0, 0, 0);
}


gboolean
detailview_set_selection (GimvThumbView *tv, GimvThumb *thumb, gboolean select)
{
   DetailViewData *tv_data;
   gint pos;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   if (g_list_length (tv->thumblist) < 1) return FALSE;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data && tv_data->clist, FALSE);

   pos = g_list_index (tv->thumblist, thumb);

   if (pos >= 0) { 
      thumb->selected = select;
      if (thumb->selected)
         gtk_clist_select_row (GTK_CLIST (tv_data->clist), pos, -1);
      else
         gtk_clist_unselect_row (GTK_CLIST (tv_data->clist), pos, -1);
   }

   return TRUE;
}


void
detailview_set_focus (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   gint pos;
   GtkCList *clist;

   g_return_if_fail (tv);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_if_fail (tv_data && tv_data->clist);

   pos = g_list_index (tv->thumblist, thumb);

   clist = GTK_CLIST (tv_data->clist);

   clist->anchor = pos;
   clist->focus_row = pos;
#ifdef USE_GTK2
   GTK_CLIST_GET_CLASS (clist)->refresh (clist);
#else /* USE_GTK2 */
   GTK_CLIST_CLASS (GTK_OBJECT (clist)->klass)->refresh (clist);
#endif /* USE_GTK2 */
}


GimvThumb *
detailview_get_focus (GimvThumbView *tv)
{
   DetailViewData *tv_data;
   gint pos;
   GtkCList *clist;
   GList *node;

   g_return_val_if_fail (tv, NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data && tv_data->clist, NULL);

   clist = GTK_CLIST (tv_data->clist);

   pos = clist->focus_row;
   if (pos < 0) return NULL;

   node = g_list_nth (tv->thumblist, pos);
   if (node)
      return node->data;

   return NULL;
}


#define CELL_SPACING 1
gboolean
detailview_thumbnail_is_in_viewport (GimvThumbView *tv, GimvThumb *thumb)
{
   DetailViewData *tv_data;
   GList *node;
   gint row;
   GtkCList *clist;
   GdkRectangle area, row_area, intersect_area;

   g_return_val_if_fail (tv, FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   g_return_val_if_fail (tv_data, FALSE);
   clist = GTK_CLIST (tv_data->clist);

   node = g_list_find (tv->thumblist, thumb);
   row = g_list_position (tv->thumblist, node);

   /* widget area */
   gtkutil_get_widget_area (tv_data->clist, &area);

   /* row area */
   row_area.x = 0;
   row_area.y = (clist->row_height * (row + 1))
      + ((row + 1) * CELL_SPACING)
      + clist->voffset;
   row_area.width  = area.width;
   row_area.height = clist->row_height + CELL_SPACING * 2;

   /* intersect? */
   if (gdk_rectangle_intersect (&area, &row_area, &intersect_area))
      return TRUE;
   else
      return FALSE;
}
#undef CELL_SPACING


GtkWidget *
detailview_create (GimvThumbView *tv, const gchar *dest_mode)
{
   gint i, num;
   DetailViewData *tv_data;
   GList *node;
   gboolean show_title;

   g_return_val_if_fail (tv, NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), DETAIL_VIEW_LABEL);
   if (!tv_data) {
      tv_data = detailview_new (tv);
      g_return_val_if_fail (tv_data, NULL);
   }

   detailview_create_title_idx_list ();

   /* create clist widget */
   num = detailview_title_idx_list_num + 1;
   tv_data->clist = gtk_clist_new (num);

   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->clist, "DetailIconMode");
   }
   if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->clist, "DetailThumbMode");
   }

   gtk_clist_set_selection_mode(GTK_CLIST (tv_data->clist),
                                GTK_SELECTION_MULTIPLE);
   gtk_clist_set_button_actions (GTK_CLIST (tv_data->clist), 0, GTK_BUTTON_IGNORED);
   gtk_clist_set_shadow_type (GTK_CLIST (tv_data->clist), GTK_SHADOW_OUT);
   /* gtk_clist_set_button_actions (GTK_CLIST (clist), 0, GTK_BUTTON_SELECTS); */
   gtk_widget_show (tv_data->clist);

   gtk_signal_connect_after (GTK_OBJECT (tv_data->clist), "key-press-event",
                             GTK_SIGNAL_FUNC (cb_clist_key_press), tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->clist),"button_press_event",
                             GTK_SIGNAL_FUNC (cb_clist_button_press), tv);
   SIGNAL_CONNECT_TRANSRATE_SCROLL (tv_data->clist);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->clist),"button_release_event",
                             GTK_SIGNAL_FUNC (cb_clist_button_release), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist),"motion_notify_event",
                       GTK_SIGNAL_FUNC (cb_clist_motion_notify), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "click_column",
                       GTK_SIGNAL_FUNC (cb_click_column), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "select-row",
                       GTK_SIGNAL_FUNC (cb_select_row), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "unselect-row",
                       GTK_SIGNAL_FUNC (cb_unselect_row), tv);

   /* for drop file list */
   dnd_src_set  (tv_data->clist, detailview_dnd_targets, detailview_dnd_targets_num);
   dnd_dest_set (tv_data->clist, detailview_dnd_targets, detailview_dnd_targets_num);

   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_begin",
                       GTK_SIGNAL_FUNC (gimv_thumb_view_drag_begin_cb), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_data_get",
                       GTK_SIGNAL_FUNC (gimv_thumb_view_drag_data_get_cb), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_data_received",
                       GTK_SIGNAL_FUNC (cb_drag_data_received), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag-data-delete",
                       GTK_SIGNAL_FUNC (gimv_thumb_view_drag_data_delete_cb), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_end",
                       GTK_SIGNAL_FUNC (gimv_thumb_view_drag_end_cb), tv);
#if 0
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_motion",
                       GTK_SIGNAL_FUNC (cb_drag_motion), tv);
   gtk_signal_connect (GTK_OBJECT (tv_data->clist), "drag_leave",
                       GTK_SIGNAL_FUNC (cb_drag_leave), tv);
#endif
   gtk_object_set_data (GTK_OBJECT (tv_data->clist), "gimv-tab", tv);

   gtk_clist_set_use_drag_icons (GTK_CLIST (tv_data->clist), FALSE);

   for (i = 0; i < num; i++) {
      gtk_clist_set_column_auto_resize (GTK_CLIST (tv_data->clist), i, TRUE);
   }

   /* set column titles */
   node = detailview_title_idx_list;
   for (i = 1; node; i++) {
      gint idx = GPOINTER_TO_INT (node->data);
      gtk_clist_set_column_title (GTK_CLIST (tv_data->clist),
                                  i, _(detailview_columns[idx].title));
      gtk_clist_set_column_justification(GTK_CLIST (tv_data->clist), i,
                                         detailview_columns[idx].justification);
      node = g_list_next (node);
   }

   detailview_prefs_get_value ("show_title", (gpointer) &show_title);

   if (show_title)
      gtk_clist_column_titles_show (GTK_CLIST (tv_data->clist));

   if (!strcmp (DETAIL_ICON_LABEL, dest_mode)) {
      gtk_clist_set_column_width (GTK_CLIST(tv_data->clist), 0, ICON_SIZE);
      gtk_clist_set_row_height (GTK_CLIST(tv_data->clist), ICON_SIZE);
   }
   if (!strcmp (DETAIL_THUMB_LABEL, dest_mode)) {
      gtk_clist_set_column_width (GTK_CLIST (tv_data->clist), 0, tv->thumb_size);
      gtk_clist_set_row_height (GTK_CLIST (tv_data->clist), tv->thumb_size);
   }

   /* set data */
   if (tv->thumblist) {
      GList *node;

      for (node = tv->thumblist; node; node = g_list_next (node))
         detailview_append_thumb_frame (tv, node->data, dest_mode);
      detailview_set_pixmaps (tv, dest_mode);
   }

   return tv_data->clist;
}

#endif /* ENABLE_TREEVIEW */
