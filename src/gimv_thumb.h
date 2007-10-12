/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GimvThumbClass - Server side thumbnnail cache.
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

#ifndef __GIMV_THUMB_H__
#define __GIMV_THUMB_H__

#include "gimageview.h"
#include "fileload.h"


#define GIMV_TYPE_THUMB            (gimv_thumb_get_type ())
#define GIMV_THUMB(obj)            (GTK_CHECK_CAST (obj, gimv_thumb_get_type (), GimvThumb))
#define GIMV_THUMB_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_thumb_get_type, GimvThumbClass))
#define GIMV_IS_THUMB(obj)         (GTK_CHECK_TYPE (obj, gimv_thumb_get_type ()))
#define GIMV_IS_THUMB_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_THUMB))


typedef struct GimvThumbPriv_Tag  GimvThumbPriv;
typedef struct GimvThumbClass_Tag GimvThumbClass;


#define ICON_SIZE 18


struct GimvThumb_Tag
{
   GtkObject       parent;

   GimvImageInfo  *info;

   /* image */
   GdkPixmap      *thumbnail;
   GdkBitmap      *thumbnail_mask;
   GdkPixmap      *icon;
   GdkBitmap      *icon_mask;
   gchar          *cache_type;

   gint            thumb_width;
   gint            thumb_height;

   /* will be removed */
   gboolean        selected;
};


struct GimvThumbClass_Tag
{
   GtkObjectClass  parent_class;
};


GtkType        gimv_thumb_get_type             (void);
GimvThumb     *gimv_thumb_new                  (GimvImageInfo  *info);
gboolean       gimv_thumb_load                 (GimvThumb      *thumb,
                                                gint            thumb_size,
                                                ThumbLoadType   type);
gboolean       gimv_thumb_is_loading           (GimvThumb      *thumb);
void           gimv_thumb_load_stop            (GimvThumb      *thumb);

void           gimv_thumb_get_thumb            (GimvThumb      *thumb,
                                                GdkPixmap     **pixmap,
                                                GdkBitmap     **mask);
GtkWidget     *gimv_thumb_get_thumb_by_widget  (GimvThumb      *thumb);
void           gimv_thumb_get_icon             (GimvThumb      *thumb,
                                                GdkPixmap     **pixmap,
                                                GdkBitmap     **mask);
GtkWidget     *gimv_thumb_get_icon_by_widget   (GimvThumb      *thumb);
const gchar   *gimv_thumb_get_cache_type       (GimvThumb      *thumb);
gboolean       gimv_thumb_has_thumbnail        (GimvThumb      *thumb);
gchar         *gimv_thumb_find_thumbcache      (const gchar    *filename,
                                                gchar         **type);

#ifdef USE_GTK2
#  define gimv_thumb_get_parent_thumbview(thumb) \
   ((GimvThumbView *) g_object_get_data(G_OBJECT(thumb), "GimvThumbView"))
#  define gimv_thumb_set_parent_thumbview(thumb, tv) \
   g_object_set_data(G_OBJECT(thumb), "GimvThumbView", tv);
#else
#  define gimv_thumb_get_parent_thumbview(thumb) \
   ((GimvThumbView *) gtk_object_get_data(GTK_OBJECT(thumb), "GimvThumbView"))
#  define gimv_thumb_set_parent_thumbview(thumb, tv) \
   gtk_object_set_data(GTK_OBJECT(thumb), "GimvThumbView", tv);
#endif

#endif /* __GIMV_THUMB_H__ */
