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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <gtk/gtk.h>

#include "gimv_icon_stock.h"
#include "utils_file.h"

#ifndef EXCLUDE_ICONS
/* common icons */
#include "pixmaps/gimv_icon.xpm"
#include "pixmaps/nfolder.xpm"
#include "pixmaps/paper.xpm"
#include "pixmaps/prefs.xpm"
#include "pixmaps/alert.xpm"
#include "pixmaps/question.xpm"
#include "pixmaps/image.xpm"
#include "pixmaps/archive.xpm"
#include "pixmaps/small_archive.xpm"
#include "pixmaps/folder48.xpm"
#ifdef ENABLE_SPLASH
#   include "pixmaps/gimageview.xpm"
#endif
/* icons for image window */
#include "pixmaps/back.xpm"
#include "pixmaps/forward.xpm"
#include "pixmaps/no_zoom.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"
#include "pixmaps/zoom_fit.xpm"
#include "pixmaps/zoom.xpm"
#include "pixmaps/rotate.xpm"
#if 0
#include "pixmaps/rotate0.xpm"
#include "pixmaps/rotate1.xpm"
#include "pixmaps/rotate2.xpm"
#include "pixmaps/rotate3.xpm"
#endif
#include "pixmaps/resize.xpm"
#include "pixmaps/fullscreen.xpm"
#include "pixmaps/nav_button.xpm"

/* player */
#include "pixmaps/play.xpm"
#include "pixmaps/pause.xpm"
#include "pixmaps/stop2.xpm"
#include "pixmaps/ff.xpm"
#include "pixmaps/rw.xpm"
#include "pixmaps/next_t.xpm"
#include "pixmaps/prev_t.xpm"
#include "pixmaps/eject.xpm"

/* icons for thumbnail window */
#include "pixmaps/refresh.xpm"
#include "pixmaps/skip.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/leftarrow.xpm"
#include "pixmaps/rightarrow.xpm"
#include "pixmaps/close.xpm"
#include "pixmaps/small_close.xpm"

/* icons for directory view */
#include "pixmaps/folder.xpm"
#include "pixmaps/folder-link.xpm"
#include "pixmaps/folder-open.xpm"
#include "pixmaps/folder-link-open.xpm"
#include "pixmaps/folder-go.xpm"
#include "pixmaps/folder-up.xpm"
#include "pixmaps/folder-lock.xpm"
#include "pixmaps/small_home.xpm"
#include "pixmaps/small_up.xpm"
#include "pixmaps/small_refresh.xpm"
#include "pixmaps/dotfile.xpm"
#endif /* EXCLUDE_ICONS */

/* cursors */
#include "pixmaps/hand-open-data.xbm"
#include "pixmaps/hand-open-mask.xbm"
#include "pixmaps/hand-closed-data.xbm"
#include "pixmaps/hand-closed-mask.xbm"
#include "pixmaps/void-data.xbm"
#include "pixmaps/void-mask.xbm"

#ifndef EXCLUDE_ICONS
static GimvIconStockEntry default_icons [] = {
   {"gimv_icon",         gimv_icon_xpm},
   {"nfolder",           nfolder_xpm},
   {"paper",             paper_xpm},
   {"prefs",             prefs_xpm},
   {"alert",             alert_xpm},
   {"question",          question_xpm},
   {"image",             image_xpm},
   {"archive",           archive_xpm},
   {"small_archive",     small_archive_xpm},
   {"folder48",          folder48_xpm},
#ifdef ENABLE_SPLASH
   {"gimageview",        gimageview_xpm},
#endif

   {"back",              back_xpm},
   {"forward",           forward_xpm},
   {"no_zoom",           no_zoom_xpm},
   {"zoom_in",           zoom_in_xpm},
   {"zoom_out",          zoom_out_xpm},
   {"zoom_fit",          zoom_fit_xpm},
   {"zoom",              zoom_xpm},
   {"rotate",            rotate_xpm},
#if 0
   {"rotate0",           rotate0_xpm},
   {"rotate1",           rotate1_xpm},
   {"rotate2",           rotate2_xpm},
   {"rotate3",           rotate3_xpm},
#endif
   {"resize",            resize_xpm},
   {"fullscreen",        fullscreen_xpm},
   {"nav-button",        nav_button_xpm},

   {"play",              play_xpm},
   {"pause",             pause_xpm},
   {"stop2",             stop2_xpm},
   {"ff",                ff_xpm},
   {"rw",                rw_xpm},
   {"next_t",            next_t_xpm},
   {"prev_t",            prev_t_xpm},
   {"eject",             eject_xpm},

   {"refresh",           refresh_xpm},
   {"skip",              skip_xpm},
   {"stop",              stop_xpm},
   {"leftarrow",         leftarrow_xpm},
   {"rightarrow",        rightarrow_xpm},
   {"close",             close_xpm},
   {"small_close",       small_close_xpm},

   {"folder",            folder_xpm},
   {"folder-go",         folder_go_xpm},
   {"folder-up",         folder_up_xpm},
   {"folder-link",       folder_link_xpm},
   {"folder-open",       folder_open_xpm},
   {"folder-link-open",  folder_link_open_xpm},
   {"folder-lock",       folder_lock_xpm},
   {"small_home",        small_home_xpm},
   {"small_up",          small_up_xpm},
   {"small_refresh",     small_refresh_xpm},
   {"dotfile",           dotfile_xpm},
};
static gint default_icons_num
= sizeof (default_icons) / sizeof (default_icons[0]);
#endif /* EXCLUDE_ICONS */

static struct {
	char *data;
	char *mask;
	int data_width;
	int data_height;
	int mask_width;
	int mask_height;
	int hot_x, hot_y;
} cursors[] = {
	{ hand_open_data_bits, hand_open_mask_bits,
	  hand_open_data_width, hand_open_data_height,
	  hand_open_mask_width, hand_open_mask_height,
	  hand_open_data_width / 2, hand_open_data_height / 2 },
	{ hand_closed_data_bits, hand_closed_mask_bits,
	  hand_closed_data_width, hand_closed_data_height,
	  hand_closed_mask_width, hand_closed_mask_height,
	  hand_closed_data_width / 2, hand_closed_data_height / 2 },
	{ void_data_bits, void_mask_bits,
	  void_data_width, void_data_height,
	  void_mask_width, void_mask_height,	  void_data_width / 2, void_data_height / 2 },
	{ NULL, NULL, 0, 0, 0, 0 }
};

static gchar       *icondir         = NULL,
                   *default_icondir = NULL;
static GdkColormap *sys_colormap    = NULL;
static GHashTable  *icons           = NULL;

gboolean
gimv_icon_stock_init (const gchar *iconset)
{
   GimvIcon *icon;

   if (!default_icondir)
      default_icondir = g_strconcat (ICONDIR, "/", DEFAULT_ICONSET, NULL);
   if (icondir)
      g_free (icondir);
   if (iconset)
      icondir = g_strconcat (ICONDIR, "/", iconset, NULL);
   /* if (!file_exists (icondir)) return FALSE; */

   icons = g_hash_table_new (g_str_hash, g_str_equal);

   if (!sys_colormap)
      sys_colormap = gdk_colormap_get_system ();

   /* set drag icon */
   icon = gimv_icon_stock_get_icon ("image");
   if (icon)
      gtk_drag_set_default_icon (sys_colormap,
                                 icon->pixmap, icon->mask,
                                 0, 0);

   return TRUE;
}

GimvIcon *
gimv_icon_stock_get_icon (const gchar *icon_name)
{
   gchar *path = NULL;
   GimvIcon *icon;

   g_return_val_if_fail (icon_name, NULL);;

   icon = g_hash_table_lookup (icons, icon_name);

   if (icon) {
      return icon;
   }

   icon = g_new0 (GimvIcon, 1);
   icon->pixmap = NULL;
   icon->mask = NULL;
   icon->pixbuf = NULL;

   if (icondir) {
      path = g_strconcat (icondir, "/", icon_name, ".xpm", NULL);
      if (file_exists (path))
         icon->pixmap = gdk_pixmap_colormap_create_from_xpm (NULL, sys_colormap,
                                                             &icon->mask, NULL, path);
   }

   if (!icon->pixmap) {
      if (path)
         g_free (path);
      path = g_strconcat (default_icondir, "/", icon_name, ".xpm", NULL);
      if (file_exists (path))
         icon->pixmap = gdk_pixmap_colormap_create_from_xpm (NULL, sys_colormap,
                                                             &icon->mask, NULL, path);
   }

   g_free (path);

#ifndef EXCLUDE_ICONS
   if (!icon->pixmap) {
      gint i;

      for (i = 0; i < default_icons_num; i++) {
         if (!strcmp (icon_name, default_icons[i].name))
            icon->pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, sys_colormap,
                                                                  &icon->mask, NULL,
                                                                  default_icons[i].xpm_data);
      }
   } 
#endif /* EXCLUDE_ICONS */

   if (icon->pixmap) {
      g_hash_table_insert (icons, (gchar *) icon_name, icon);
   } else {
      if (icon->mask) gdk_bitmap_unref (icon->mask);
      g_free (icon);
      return NULL;
   }

   return icon;
}

GtkWidget *
gimv_icon_stock_get_widget (const gchar *icon_name)
{
   GimvIcon *icon;
   GtkWidget *widget = NULL;

   g_return_val_if_fail (icon_name, NULL);

   icon = gimv_icon_stock_get_icon (icon_name);

   if (icon)
      widget = gtk_image_new_from_pixmap (icon->pixmap, icon->mask);   

   return widget;
}

void
gimv_icon_stock_change_widget_icon (GtkWidget *widget, const gchar *icon_name)
{
   GimvIcon *icon;

   g_return_if_fail (widget);
   g_return_if_fail (icon_name && *icon_name);

   g_return_if_fail (GTK_IS_IMAGE (widget));   

   icon = gimv_icon_stock_get_icon (icon_name);
   if (!icon) return;

   gtk_image_set_from_pixmap (GTK_IMAGE (widget), icon->pixmap, icon->mask);
}

void
gimv_icon_stock_set_window_icon (GdkWindow *window, gchar *name)
{
   GimvIcon *icon;

   icon = gimv_icon_stock_get_icon (name);
   if (icon)
      gdk_window_set_icon (window, NULL,
                           icon->pixmap, icon->mask);
}

void
gimv_icon_stock_free_icon (const gchar *icon_name)
{
   GimvIcon *icon;

   g_return_if_fail (icon_name);

   icon = gimv_icon_stock_get_icon (icon_name);
   if (!icon) return;

   g_hash_table_remove (icons, icon_name);
   gdk_pixmap_unref (icon->pixmap);
   if (icon->pixbuf)
      g_object_unref (icon->pixbuf);
   g_free (icon);
}

GdkPixbuf *
gimv_icon_stock_get_pixbuf  (const gchar *icon_name)
{
   GimvIcon *icon;
   gchar *path = NULL;

   g_return_val_if_fail (icon_name, NULL);

   icon = gimv_icon_stock_get_icon (icon_name);
   if (!icon) return NULL;
   if (icon->pixbuf) return icon->pixbuf;

   if (icondir) {
      path = g_strconcat (icondir, "/", icon_name, ".xpm", NULL);
      if (file_exists (path))
         icon->pixbuf = gdk_pixbuf_new_from_file (path, NULL);
   }

   if (!icon->pixbuf) {
      g_free (path);
      path = g_strconcat (default_icondir, "/", icon_name, ".xpm", NULL);
      if (file_exists (path))
         icon->pixbuf = gdk_pixbuf_new_from_file (path, NULL);
   }

   g_free (path);
   path = NULL;

#ifndef EXCLUDE_ICONS
   if (!icon->pixbuf) {
      gint i;

      for (i = 0; i < default_icons_num; i++) {
         if (!strcmp (icon_name, default_icons[i].name))
            icon->pixbuf = gdk_pixbuf_new_from_xpm_data (
               (const char **)default_icons[i].xpm_data);
      }
   } 
#endif /* EXCLUDE_ICONS */

   return icon->pixbuf;
}

void
gimv_icon_stock_free_pixbuf (const gchar *icon_name)
{
   GimvIcon *icon;

   g_return_if_fail (icon_name);

   icon = gimv_icon_stock_get_icon (icon_name);
   if (!icon) return;

   g_object_unref (icon->pixbuf);
   icon->pixbuf = NULL;
}

GdkCursor *
gimv_icon_stock_get_cursor (GdkWindow *window, CursorType type)
{
	GdkBitmap *data;
	GdkBitmap *mask;
	GdkColor black, white;
	GdkCursor *cursor;

	g_return_val_if_fail (window != NULL, NULL);
	g_return_val_if_fail (type >= 0 && type < CURSOR_NUM_CURSORS, NULL);

	g_assert (cursors[type].data_width == cursors[type].mask_width);
	g_assert (cursors[type].data_height == cursors[type].mask_height);

	data = gdk_bitmap_create_from_data (window,
                                       cursors[type].data,
                                       cursors[type].data_width,
                                       cursors[type].data_height);
	mask = gdk_bitmap_create_from_data (window,
                                       cursors[type].mask,
                                       cursors[type].mask_width,
                                       cursors[type].mask_height);

	g_assert (data != NULL && mask != NULL);

	gdk_color_black (gdk_window_get_colormap (window), &black);
	gdk_color_white (gdk_window_get_colormap (window), &white);

	cursor = gdk_cursor_new_from_pixmap (data, mask, &white, &black,
                                        cursors[type].hot_x, cursors[type].hot_y);
	g_assert (cursor != NULL);

	gdk_bitmap_unref (data);
	gdk_bitmap_unref (mask);

	return cursor;
}
