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
 *  These codes are mostly taken from ProView image viewer.
 *
 *  ProView image viewer Author:
 *     promax <promax@users.sourceforge.net>
 */

/*
 *  modification file from Another X image viewer
 *  David Ramboz <dramboz@users.sourceforge.net>
 */

#ifndef __GIMV_ZALBUM_H__
#define __GIMV_ZALBUM_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gimv_zlist.h"

#define GIMV_TYPE_ZALBUM            (gimv_zalbum_get_type ())
#define GIMV_ZALBUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_ZALBUM, GimvZAlbum))
#define GIMV_ZALBUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_ZALBUM, GimvZAlbumClass))
#define GIMV_IS_ZALBUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_ZALBUM))
#define GIMV_IS_ZALBUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_ZALBUM))
#define GIMV_ZALBUM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_ZALBUM, GimvZAlbumClass))

typedef struct _GimvZAlbum            GimvZAlbum;
typedef struct _GimvZAlbumCell        GimvZAlbumCell;
typedef struct _GimvZAlbumClass       GimvZAlbumClass;
typedef struct _GimvZAlbumColumnInfo  GimvZAlbumColumnInfo;

enum
{
   GIMV_ZALBUM_CELL_SELECTED = 1 << 1,
};

typedef enum
{
   GIMV_ZALBUM_CELL_LABEL_BOTTOM,
   GIMV_ZALBUM_CELL_LABEL_TOP,
   GIMV_ZALBUM_CELL_LABEL_RIGHT,
   GIMV_ZALBUM_CELL_LABEL_LEFT,
} GimvZAlbumLabelPosition;

struct _GimvZAlbum
{
   GimvZList list;

   guint8 label_pos;

   gint max_pix_width;
   gint max_pix_height;

   gint max_cell_width;
   gint max_cell_height;

   guint len;
};

struct _GimvZAlbumClass
{
   GimvZListClass parent_class;
};

struct _GimvZAlbumCell
{
   gint flags;
   const gchar *name;

   GdkPixmap *ipix;
   GdkBitmap *imask;

   gpointer user_data;
   GtkDestroyNotify destroy;
};


GType      gimv_zalbum_get_type                  (void);
GtkWidget *gimv_zalbum_new                       (void);
guint      gimv_zalbum_add                       (GimvZAlbum       *album,
                                                  const gchar      *name);
guint      gimv_zalbum_insert                    (GimvZAlbum       *album,
                                                  guint             pos,
                                                  const gchar      *name);
void       gimv_zalbum_remove                    (GimvZAlbum       *album,
                                                  guint             pos);
void       gimv_zalbum_set_label_position        (GimvZAlbum       *album,
                                                  GimvZAlbumLabelPosition pos);
void       gimv_zalbum_set_min_pixmap_size       (GimvZAlbum       *album,
                                                  guint             width,
                                                  guint             height);
void       gimv_zalbum_set_min_cell_size         (GimvZAlbum       *album,
                                                  guint             width,
                                                  guint             height);
void       gimv_zalbum_set_pixmap                (GimvZAlbum       *album,
                                                  guint             idx,
                                                  GdkPixmap        *pixmap,
                                                  GdkBitmap        *mask);
void       gimv_zalbum_set_cell_data             (GimvZAlbum       *album,
                                                  guint             idx,
                                                  gpointer          user_data);
void       gimv_zalbum_set_cell_data_full        (GimvZAlbum       *album,
                                                  guint             idx,
                                                  gpointer          data,
                                                  GtkDestroyNotify  destroy);
gpointer   gimv_zalbum_get_cell_data             (GimvZAlbum       *album,
                                                  guint             idx);

#define gimv_zalbum_freeze(album) (gimv_scrolled_freeze(GIMV_SCROLLED(album)))
#define gimv_zalbum_thawn(album)  (gimv_scrolled_thawn(GIMV_SCROLLED(album)))

#endif /* __GIMV_ZALBUM_H__ */
