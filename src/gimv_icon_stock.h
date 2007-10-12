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

#ifndef __GIMV_ICON_STOCK_H__
#define __GIMV_ICON_STOCK_H__

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>


#define DEFAULT_ICONSET "default"


typedef struct GimvIconStockEntry_Tag
{
   gchar   *name;
   char   **xpm_data;
} GimvIconStockEntry;


typedef struct GimvIcon_Tag {
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GdkPixbuf *pixbuf;
} GimvIcon;


gboolean   gimv_icon_stock_init               (const gchar *iconset);
GimvIcon  *gimv_icon_stock_get_icon           (const gchar *icon_name);
GtkWidget *gimv_icon_stock_get_widget         (const gchar *icon_name);
void       gimv_icon_stock_change_widget_icon (GtkWidget   *widget,
                                               const gchar *icon_name);
void       gimv_icon_stock_free_icon          (const gchar *icon_name);
void       gimv_icon_stock_set_window_icon    (GdkWindow   *window,
                                               gchar       *name);

GdkPixbuf *gimv_icon_stock_get_pixbuf         (const gchar *icon_name);
void       gimv_icon_stock_free_pixbuf        (const gchar *icon_name);

#endif /* __GIMV_ICON_STOCK_H__ */
