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

#include <string.h>

#include "gimageview.h"

#include "dnd.h"
#include "gimv_thumb.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "gimv_zalbum.h"
#include "gtk2-compat.h"
#include "gtkutils.h"
#include "prefs.h"


#define THUMBALBUM_LABEL  "Album"
#define THUMBALBUM2_LABEL "Album 2"
#define THUMBALBUM3_LABEL "Album 3"


static void       thumbalbum_freeze                   (GimvThumbView  *tv);
static void       thumbalbum_thaw                     (GimvThumbView  *tv);
static void       thumbalbum_append_thumb_frame       (GimvThumbView  *tv,
                                                       GimvThumb      *thumb,
                                                       const gchar    *dest_mode);
static void       thumbalbum_update_thumbnail         (GimvThumbView  *tv,
                                                       GimvThumb      *thumb,
                                                       const gchar    *dest_mode);
static GList     *thumbalbum_get_load_list            (GimvThumbView  *tv);
static void       thumbalbum_remove_thumbnail         (GimvThumbView  *tv,
                                                       GimvThumb      *thumb);
static void       thumbalbum_adjust                   (GimvThumbView  *tv,
                                                       GimvThumb      *thumb);
static GtkWidget *thumbalbum_create                   (GimvThumbView  *tv,
                                                       const gchar    *dest_mode);
static gboolean   thumbalbum_set_selection            (GimvThumbView  *tv,
                                                       GimvThumb      *thumb,
                                                       gboolean        select);
static void       thumbalbum_set_focus                (GimvThumbView  *tv,
                                                       GimvThumb      *thumb);
static GimvThumb *thumbalbum_get_focus                (GimvThumbView  *tv);
static gboolean   thumbalbum_thumbnail_is_in_viewport (GimvThumbView  *tv,
                                                       GimvThumb      *thumb);

GimvThumbViewPlugin thumbalbum_modes[] =
{
   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Album"),
    0,
    thumbalbum_create,
    thumbalbum_freeze,
    thumbalbum_thaw,
    thumbalbum_append_thumb_frame,
    thumbalbum_update_thumbnail,
    thumbalbum_remove_thumbnail,
    thumbalbum_get_load_list,
    thumbalbum_adjust,
    thumbalbum_set_selection,
    thumbalbum_set_focus,
    thumbalbum_get_focus,
    thumbalbum_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Album 2"),
    10,
    thumbalbum_create,
    thumbalbum_freeze,
    thumbalbum_thaw,
    thumbalbum_append_thumb_frame,
    thumbalbum_update_thumbnail,
    thumbalbum_remove_thumbnail,
    thumbalbum_get_load_list,
    thumbalbum_adjust,
    thumbalbum_set_selection,
    thumbalbum_set_focus,
    thumbalbum_get_focus,
    thumbalbum_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Album 3"),
    20,
    thumbalbum_create,
    thumbalbum_freeze,
    thumbalbum_thaw,
    thumbalbum_append_thumb_frame,
    thumbalbum_update_thumbnail,
    thumbalbum_remove_thumbnail,
    thumbalbum_get_load_list,
    thumbalbum_adjust,
    thumbalbum_set_selection,
    thumbalbum_set_focus,
    thumbalbum_get_focus,
    thumbalbum_thumbnail_is_in_viewport},
};
gint thumbalbum_modes_num
   = sizeof (thumbalbum_modes) / sizeof (GimvThumbViewPlugin);


typedef struct ThumbViewData_Tag
{
   GtkWidget *album;
} ThumbViewData;


/* defined in thumbview_list.c */
void   album_create_title_idx_list  (void);
gchar *album_create_label_str       (GimvThumb *thumb);


/******************************************************************************
 *
 *   Callback functions.
 *
 ******************************************************************************/
static gboolean
cb_album_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   GimvThumbView *tv = data;
   gint row;
   GimvThumb *thumb;
   gboolean retval = event->button == 3 ? TRUE : FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), retval);

   row = gimv_zlist_cell_index_from_xy (GIMV_ZLIST (widget), event->x, event->y);
   if (row < 0) {
      gtk_drag_source_unset (widget);
      gtk_object_set_data (GTK_OBJECT (widget), "drag-unset", GINT_TO_POINTER (1));
      gimv_thumb_win_notebook_drag_src_unset (tv->tw);   /* FIXME!! */
      return retval;
   }

   thumb = gimv_zalbum_get_cell_data (GIMV_ZALBUM (widget), row);
   if (!thumb) return retval;

   retval = gimv_thumb_view_thumb_button_press_cb (widget, event, thumb);

   return retval;
}


static gboolean
cb_album_button_release (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   GimvThumbView *tv = data;
   GimvThumb *thumb;
   gint row;
   gpointer dnd_unset;
   gboolean retval;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   dnd_unset = gtk_object_get_data (GTK_OBJECT (widget), "drag-unset");
   if (dnd_unset)
      dnd_src_set  (widget, dnd_types_uri, dnd_types_uri_num);
   dnd_unset = FALSE;

   row = gimv_zlist_cell_index_from_xy (GIMV_ZLIST (widget), event->x, event->y);
   if (row < 0) {
      gimv_thumb_win_notebook_drag_src_reset (tv->tw);   /* FIXME!! */
      return FALSE;
   }

   thumb = gimv_zalbum_get_cell_data (GIMV_ZALBUM (widget), row);
   if (!thumb) return FALSE;

   retval = gimv_thumb_view_thumb_button_release_cb (widget, event, thumb);

   gtk_widget_grab_focus (widget);
   thumbalbum_set_focus (tv, thumb);

   return retval;
}


static void
cb_select_cell (GimvZAlbum *album, gint idx, GimvThumbView *tv)
{
   GimvThumb *thumb;

   g_return_if_fail (GIMV_IS_ZALBUM (album));
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumb = gimv_zalbum_get_cell_data (GIMV_ZALBUM (album), idx);
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   thumb->selected = TRUE;
}

static void
cb_unselect_cell (GimvZAlbum *album, gint idx, GimvThumbView *tv)
{
   GimvThumb *thumb;

   g_return_if_fail (GIMV_IS_ZALBUM (album));
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumb = gimv_zalbum_get_cell_data (GIMV_ZALBUM (album), idx);
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   thumb->selected = FALSE;
}


static gboolean
cb_album_key_press (GtkWidget *widget,
                    GdkEventKey *event,
                    GimvThumbView *tv)
{
   GimvThumb *thumb = NULL;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   if (GIMV_ZLIST (widget)->focus >= 0)
      thumb = gimv_zalbum_get_cell_data (GIMV_ZALBUM (widget),
                                         GIMV_ZLIST (widget)->focus);

   if (gimv_thumb_view_thumb_key_press_cb(widget, event, thumb))
      return FALSE;

   {
      switch (event->keyval) {
      case GDK_Left:
      case GDK_Right:
      case GDK_Up:
      case GDK_Down:
         if (event->state & GDK_SHIFT_MASK) {
            gimv_thumb_view_set_selection_all (tv, FALSE);
            gimv_thumb_view_set_selection (thumb, TRUE);
            gimv_thumb_view_open_image (tv, thumb,
                                        GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW);
         }
         return TRUE;
      case GDK_Return:
         if (!thumb) break;
         if (event->state & GDK_SHIFT_MASK || event->state & GDK_CONTROL_MASK) {
            /* is there somteing to do? */
         } else {
            gimv_thumb_view_set_selection_all (tv, FALSE);
         }
         gimv_thumb_view_set_selection (thumb, TRUE);
         gimv_thumb_view_open_image (tv, thumb,
                                     GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO);
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


/******************************************************************************
 *
 *   private functions.
 *
 ******************************************************************************/
static ThumbViewData *
thumbalbum_new (GimvThumbView *tv)
{
   ThumbViewData *tv_data;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   if (!tv_data) {
      tv_data = g_new0 (ThumbViewData, 1);
      tv_data->album = NULL;
      gtk_object_set_data_full (GTK_OBJECT (tv), THUMBALBUM_LABEL, tv_data,
                                (GtkDestroyNotify) g_free);
   }

   return tv_data;
}



/******************************************************************************
 *
 *   public functions.
 *
 ******************************************************************************/
static void
thumbalbum_freeze (GimvThumbView *tv)
{
   ThumbViewData *tv_data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data && GIMV_IS_ZALBUM (tv_data->album));

   gimv_zalbum_freeze (tv_data->album);
}


static void
thumbalbum_thaw (GimvThumbView *tv)
{
   ThumbViewData *tv_data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data && GIMV_IS_ZALBUM (tv_data->album));

   gimv_zalbum_thawn (tv_data->album);
}


static void
thumbalbum_append_thumb_frame (GimvThumbView *tv, GimvThumb *thumb,
                               const gchar *dest_mode)
{
   ThumbViewData *tv_data;
   const gchar *filename;
   gchar *label;
   GdkPixmap *pixmap = NULL;
   GdkBitmap *mask = NULL;
   gint pos;
   guint idx;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data);

   pos = g_list_index (tv->thumblist, thumb);

   filename = g_basename(gimv_image_info_get_path (thumb->info));

   if (!strcmp (dest_mode, THUMBALBUM3_LABEL)) {
      label = album_create_label_str (thumb);
   } else {
      label = gimv_filename_to_internal (filename);
   }

   idx = gimv_zalbum_insert (GIMV_ZALBUM (tv_data->album), pos, label);
   g_free (label);

   gimv_zalbum_set_cell_data (GIMV_ZALBUM (tv_data->album), idx, thumb);

   thumbalbum_set_selection (tv, thumb, thumb->selected);

   if (!strcmp (THUMBALBUM2_LABEL, dest_mode)) {
      gimv_thumb_get_icon (thumb, &pixmap, &mask);
   } else {
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   }

   if (pixmap)
      gimv_zalbum_set_pixmap (GIMV_ZALBUM (tv_data->album), idx, pixmap, mask);
}


static void
thumbalbum_update_thumbnail (GimvThumbView *tv, GimvThumb *thumb,
                             const gchar *dest_mode)
{
   ThumbViewData *tv_data;
   GdkPixmap *pixmap = NULL;
   GdkBitmap *mask = NULL;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data && tv_data->album);

   pos = g_list_index (tv->thumblist, thumb);

   /* set thumbnail */
   if (!strcmp (THUMBALBUM2_LABEL, dest_mode)) {
      gimv_thumb_get_icon (thumb, &pixmap, &mask);
   } else {
      gimv_thumb_get_thumb (thumb, &pixmap, &mask);
   }
   if (pixmap)
      gimv_zalbum_set_pixmap (GIMV_ZALBUM (tv_data->album), pos, pixmap, mask);
}


static GList *
thumbalbum_get_load_list (GimvThumbView *tv)
{
   GList *loadlist = NULL, *node;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   for (node = tv->thumblist; node; node = g_list_next (node)) {
      GimvThumb *thumb = node->data;
      GdkPixmap *pixmap = NULL;
      GdkBitmap *mask = NULL;

      if (!strcmp (THUMBALBUM2_LABEL, tv->summary_mode)) {
         gimv_thumb_get_icon (thumb, &pixmap, &mask);
      } else {
         gimv_thumb_get_thumb (thumb, &pixmap, &mask);
      }
      if (!pixmap)
         loadlist = g_list_append (loadlist, thumb);
   }

   return loadlist;
}


static void
thumbalbum_remove_thumbnail (GimvThumbView *tv, GimvThumb *thumb)
{
   ThumbViewData *tv_data;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data && tv_data->album);

   pos = g_list_index (tv->thumblist, thumb);
   g_return_if_fail (pos >= 0);

   gimv_zalbum_remove (GIMV_ZALBUM (tv_data->album), pos);
}


static gboolean
thumbalbum_set_selection (GimvThumbView *tv, GimvThumb *thumb, gboolean select)
{
   ThumbViewData *tv_data;
   gint pos;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   if (g_list_length (tv->thumblist) < 1) return FALSE;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_val_if_fail (tv_data && tv_data->album, FALSE);

   pos = g_list_index (tv->thumblist, thumb);

   if (pos >= 0) {
      thumb->selected = select;
      if (thumb->selected)
         gimv_zlist_cell_select (GIMV_ZLIST (tv_data->album), pos);
      else
         gimv_zlist_cell_unselect (GIMV_ZLIST (tv_data->album), pos);
   }

   return TRUE;
}


static void
thumbalbum_set_focus (GimvThumbView *tv, GimvThumb *thumb)
{
   ThumbViewData *tv_data;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data && tv_data->album);

   pos = g_list_index (tv->thumblist, thumb);

   if (pos < 0)
      gimv_zlist_cell_unset_focus (GIMV_ZLIST (tv_data->album));
   else
      gimv_zlist_cell_set_focus (GIMV_ZLIST (tv_data->album), pos);
}


static GimvThumb *
thumbalbum_get_focus (GimvThumbView *tv)
{
   ThumbViewData *tv_data;
   gint pos;
   GList *node;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_val_if_fail (tv_data && tv_data->album, NULL);

   pos = GIMV_ZLIST (tv_data->album)->focus;
   if (pos < 0) return NULL;

   node = g_list_nth (tv->thumblist, pos);
   if (node)
      return node->data;

   return NULL;
}


static gboolean
thumbalbum_thumbnail_is_in_viewport (GimvThumbView *tv, GimvThumb *thumb)
{
   ThumbViewData *tv_data;
   GList *node;
   gint index;
   gboolean success;
   GdkRectangle area, cell_area, intersect_area;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_val_if_fail (tv_data, FALSE);

   node = g_list_find (tv->thumblist, thumb);
   index = g_list_position (tv->thumblist, node);

   /* widget area */
   gtkutil_get_widget_area (tv_data->album, &area);

   /* cell area */
   success = gimv_zlist_get_cell_area (GIMV_ZLIST (tv_data->album), index, &cell_area);
   g_return_val_if_fail (success, FALSE);

   /* intersect? */
   if (gdk_rectangle_intersect (&area, &cell_area, &intersect_area))
      return TRUE;
   else
      return FALSE;
}


static void
thumbalbum_adjust (GimvThumbView *tv, GimvThumb *thumb)
{
   ThumbViewData *tv_data;
   GList *node;
   gint pos;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = g_list_find (gimv_thumb_view_get_list(), tv);
   if (!node) return;

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   g_return_if_fail (tv_data);

   pos = g_list_index (tv->thumblist, thumb);

   gimv_zlist_moveto (GIMV_ZLIST (tv_data->album), pos);

   return;
}


static GtkWidget *
thumbalbum_create (GimvThumbView *tv, const gchar *dest_mode)
{
   ThumbViewData *tv_data;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   tv_data = gtk_object_get_data (GTK_OBJECT (tv), THUMBALBUM_LABEL);
   if (!tv_data) {
      tv_data = thumbalbum_new (tv);
      g_return_val_if_fail (tv_data, NULL);
   }

   /* create zalbum widget */
   tv_data->album = gimv_zalbum_new ();

   if (!strcmp (THUMBALBUM2_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->album, "List2IconMode");
      gimv_zlist_set_to_horizontal (GIMV_ZLIST (tv_data->album));
      gimv_zalbum_set_label_position (GIMV_ZALBUM (tv_data->album),
                                      GIMV_ZALBUM_CELL_LABEL_RIGHT);
      gimv_zalbum_set_min_pixmap_size (GIMV_ZALBUM (tv_data->album),
                                       ICON_SIZE, ICON_SIZE);
   } else if (!strcmp (THUMBALBUM3_LABEL, dest_mode)) {
      gtk_widget_set_name (tv_data->album, "List2ThumbMode");
      album_create_title_idx_list  ();
      gimv_zlist_set_to_horizontal (GIMV_ZLIST (tv_data->album));
      gimv_zalbum_set_label_position (GIMV_ZALBUM (tv_data->album),
                                      GIMV_ZALBUM_CELL_LABEL_RIGHT);
      gimv_zalbum_set_min_pixmap_size (GIMV_ZALBUM (tv_data->album),
                                       tv->thumb_size,
                                       tv->thumb_size);
   } else {
      gtk_widget_set_name (tv_data->album, "Thumbnail2Mode");
      gimv_zalbum_set_min_pixmap_size (GIMV_ZALBUM (tv_data->album),
                                       tv->thumb_size,
                                       tv->thumb_size);
   }

   gimv_scrolled_set_auto_scroll (GIMV_SCROLLED (tv_data->album),
                                  GIMV_SCROLLED_AUTO_SCROLL_BOTH
                                  | GIMV_SCROLLED_AUTO_SCROLL_DND
                                  | GIMV_SCROLLED_AUTO_SCROLL_MOTION);

   gimv_scrolled_set_h_auto_scroll_resolution (GIMV_SCROLLED (tv_data->album),
                                               20, 10);
   gimv_scrolled_set_v_auto_scroll_resolution (GIMV_SCROLLED (tv_data->album),
                                               20, 10);

   gimv_zlist_set_selection_mode (GIMV_ZLIST (tv_data->album),
                                  GTK_SELECTION_EXTENDED);

   if (!strcmp (THUMBALBUM_LABEL, dest_mode)) {
      gimv_zlist_set_cell_padding (GIMV_ZLIST (tv_data->album),
                                   conf.thumbalbum_col_space,
                                   conf.thumbalbum_row_space);
   } else {
      gimv_zlist_set_cell_padding (GIMV_ZLIST (tv_data->album), 5, 5);
   }

   gtk_widget_show (tv_data->album);

   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "button_press_event",
                             GTK_SIGNAL_FUNC (cb_album_button_press), tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "button_release_event",
                             GTK_SIGNAL_FUNC (cb_album_button_release), tv);
   SIGNAL_CONNECT_TRANSRATE_SCROLL (tv_data->album);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "key-press-event",
                             GTK_SIGNAL_FUNC (cb_album_key_press), tv);

   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "cell_select",
                             GTK_SIGNAL_FUNC (cb_select_cell), tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "cell_unselect",
                             GTK_SIGNAL_FUNC (cb_unselect_cell), tv);

   /* Drag and Drop */
   dnd_src_set  (tv_data->album, dnd_types_uri, dnd_types_uri_num);
   dnd_dest_set (tv_data->album, dnd_types_uri, dnd_types_uri_num);
   gtk_object_set_data (GTK_OBJECT (tv_data->album), "gimv-tab", tv);

   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "drag_begin",
                             GTK_SIGNAL_FUNC (gimv_thumb_view_drag_begin_cb),
                             tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "drag_data_get",
                             GTK_SIGNAL_FUNC (gimv_thumb_view_drag_data_get_cb),
                             tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "drag_data_received",
                             GTK_SIGNAL_FUNC (gimv_thumb_view_drag_data_received_cb),
                             tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "drag-data-delete",
                             GTK_SIGNAL_FUNC (gimv_thumb_view_drag_data_delete_cb),
                             tv);
   gtk_signal_connect_after (GTK_OBJECT (tv_data->album), "drag_end",
                             GTK_SIGNAL_FUNC (gimv_thumb_view_drag_end_cb), tv);

   /* append thumbnail frames */
   if (tv->thumblist) {
      GList *node;

      for (node = tv->thumblist; node; node = g_list_next (node))
         thumbalbum_append_thumb_frame (tv, node->data, dest_mode);

      gimv_zlist_cell_set_focus (GIMV_ZLIST (tv_data->album), 0);
   }

   return tv_data->album;
}





#include "fileutil.h"
#define DEFAULT_DATA_ORDER "Name,Size,Time"
static gboolean show_data_title = FALSE;
static const gchar *data_order = DEFAULT_DATA_ORDER;
static gchar *
label_filename (GimvThumb *thumb)
{
   const gchar *filename;
   gchar *tmpstr;
   gchar buf[BUF_SIZE];

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   filename = g_basename(gimv_image_info_get_path (thumb->info));

   tmpstr = gimv_filename_to_internal (filename);

   if (show_data_title)
      g_snprintf (buf, BUF_SIZE, _("Name : %s"), tmpstr);
   else
      return tmpstr;

   g_free (tmpstr);

   return g_strdup (buf);
}


static gchar *
label_size (GimvThumb *thumb)
{
   gchar *size_str, buf[BUF_SIZE];

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   size_str = fileutil_size2str (thumb->info->st.st_size, FALSE);
   if (show_data_title)
      g_snprintf (buf, BUF_SIZE, _("Size : %s bytes"), size_str);
   else
      g_snprintf (buf, BUF_SIZE, _("%s bytes"), size_str);
   g_free (size_str);

   return g_strdup (buf);
}


static gchar *
label_mtime (GimvThumb *thumb)
{
   gchar *time_str, *str;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   time_str = fileutil_time2str (thumb->info->st.st_mtime);
   if (show_data_title)
      str = g_strconcat (_("Time : "), time_str, NULL);
   else
      return time_str;
   g_free (time_str);

   return str;
}


static gchar *
label_image_type (GimvThumb *thumb)
{
   gchar buf[BUF_SIZE];
   const gchar *filename;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   filename = gimv_image_info_get_path (thumb->info);
   if (show_data_title)
      g_snprintf (buf, BUF_SIZE, _("Type: %s"),
                  gimv_image_detect_type_by_ext (filename));
   else
      return g_strdup (gimv_image_detect_type_by_ext (filename));

   return g_strdup (buf);
}
typedef gchar *(*AlbumDataStr) (GimvThumb *thumb);
typedef struct _AlbumDisplayData
{
   gchar             *title;
   AlbumDataStr    func;
} AlbumDisplayData;
static const gchar *config_order_string = NULL;
static GList *album_title_idx_list      = NULL;
static gint   album_title_idx_list_num  = 0;
static AlbumDisplayData album_display_data [] =
{
   {N_("Name"),  label_filename},
   {N_("Size"),  label_size},
   {N_("Time"),  label_mtime},
   {N_("Type"),  label_image_type},
};
static gint album_display_data_num
= sizeof (album_display_data) / sizeof (AlbumDisplayData);
gchar *
album_create_label_str (GimvThumb *thumb)
{
   GList *node;
   gchar *label;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   /* create icon label string */
   node = album_title_idx_list;
   label = NULL;
   while (node) {
      gchar *tmpstr, *oldstr;
      gint idx = GPOINTER_TO_INT (node->data);

      if (!label) {
         label = album_display_data[idx].func (thumb);
      } else {
         tmpstr = album_display_data[idx].func (thumb);
         oldstr = label;
         label = g_strconcat (label, "\n", tmpstr, NULL);
         g_free (tmpstr);
         g_free (oldstr);
      }

      node = g_list_next (node);
   }

   return label;
}
static gint
album_get_title_idx (const gchar *title)
{
   gint i;

   g_return_val_if_fail (title, -1);

   for (i = 0; i < album_display_data_num; i++) {
      if (!album_display_data[i].title) continue;
      if (!strcmp (album_display_data[i].title, title))
         return i;
   }

   return -1;
}
void
album_create_title_idx_list (void)
{
   gchar **titles;
   gint i = 0;

   if (!data_order) {
      config_order_string = NULL;
      if (album_title_idx_list)
         g_list_free (album_title_idx_list);
      album_title_idx_list_num = 0;
      return;
   }

   if (data_order == config_order_string) return;

   if (album_title_idx_list) g_list_free (album_title_idx_list);
   album_title_idx_list = NULL;

   titles = g_strsplit (data_order, ",", -1);

   g_return_if_fail (titles);

   album_title_idx_list_num = 0;
   config_order_string = data_order;

   while (titles[i]) {
      gint idx;
      idx = album_get_title_idx (titles[i]);
      if (idx >= 0) {
         album_title_idx_list = g_list_append (album_title_idx_list,
                                               GINT_TO_POINTER (idx));
         album_title_idx_list_num++;
      }
      i++;
   }

   g_strfreev (titles);
}
