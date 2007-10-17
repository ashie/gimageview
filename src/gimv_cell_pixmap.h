/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/* 
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

/*
 *  These codes are based on gtk/gtkcellrendererpixbuf.h in gtk+-2.0.6
 *  Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 */

#ifndef __GIMV_CELL_RENDERER_PIXMAP_H__
#define __GIMV_CELL_RENDERER_PIXMAP_H__

#include <gtk/gtkcellrenderer.h>

G_BEGIN_DECLS

#define GIMV_TYPE_CELL_RENDERER_PIXMAP            (gimv_cell_renderer_pixmap_get_type ())
#define GIMV_CELL_RENDERER_PIXMAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_CELL_RENDERER_PIXMAP, GimvCellRendererPixmap))
#define GIMV_CELL_RENDERER_PIXMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_CELL_RENDERER_PIXMAP, GimvCellRendererPixmapClass))
#define GIMV_IS_CELL_RENDERER_PIXMAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_CELL_RENDERER_PIXMAP))
#define GIMV_IS_CELL_RENDERER_PIXMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_CELL_RENDERER_PIXMAP))
#define GIMV_CELL_RENDERER_PIXMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_CELL_RENDERER_PIXMAP, GimvCellRendererPixmapClass))

typedef struct GimvCellRendererPixmap_Tag         GimvCellRendererPixmap;
typedef struct GimvCellRendererPixmapClass_Tag    GimvCellRendererPixmapClass;

struct GimvCellRendererPixmap_Tag
{
   GtkCellRenderer parent;

   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GdkPixmap *pixmap_expander_open;
   GdkBitmap *mask_expander_open;
   GdkPixmap *pixmap_expander_closed;
   GdkBitmap *mask_expander_closed;
};

struct GimvCellRendererPixmapClass_Tag
{
   GtkCellRendererClass parent_class;

   /* Padding for future expansion */
   void (*_gimv_reserved1) (void);
   void (*_gimv_reserved2) (void);
   void (*_gimv_reserved3) (void);
   void (*_gimv_reserved4) (void);
};

GType            gimv_cell_renderer_pixmap_get_type (void);
GtkCellRenderer *gimv_cell_renderer_pixmap_new      (void);

G_END_DECLS

#endif /* __GIMV_CELL_RENDERER_PIXMAP_H__ */
