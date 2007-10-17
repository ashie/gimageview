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
 *  These codes are mostly taken from Another X image viewer.
 *
 *  Another X image viewer Author:
 *     David Ramboz <dramboz@users.sourceforge.net>
 */


#ifndef _GIMV_ZLIST_H_
#define _GIMV_ZLIST_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gimv_scrolled.h"

#define GIMV_TYPE_ZLIST            (gimv_zlist_get_type ())
#define GIMV_ZLIST(widget)         (GTK_CHECK_CAST ((widget), GIMV_TYPE_ZLIST, GimvZList))
#define GIMV_ZLIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMV_TYPE_ZLIST, GimvZListClass))
#define GIMV_IS_ZLIST(widget)      (GTK_CHECK_TYPE ((widget), GIMV_TYPE_ZLIST))
#define GIMV_IS_ZLIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_ZLIST))

enum {
   GIMV_ZLIST_HORIZONTAL        = 1 << 1,
   GIMV_ZLIST_1                 = 1 << 2,
   GIMV_ZLIST_USES_DND          = 1 << 3,   /* not implemented yet */
   GIMV_ZLIST_HIGHLIGHTED       = 1 << 4
};

typedef enum {
   GIMV_ZLIST_REGION_SELECT_OFF,
   GIMV_ZLIST_REGION_SELECT_NORMAL,
   GIMV_ZLIST_REGION_SELECT_EXPAND,    /* shift mask */
   GIMV_ZLIST_REGION_SELECT_TOGGLE     /* control mask */
} GimvZListRegionSelectMode;

typedef struct _GimvZList GimvZList;
typedef struct _GimvZListClass GimvZListClass;


struct _GimvZList
{
   GimvScrolled           scrolled;

   guint                  flags;

   gint                   cell_width;
   gint                   cell_height;

   gint                   rows;
   gint                   columns;

   GArray                *cells;
   gint                   cell_count;

   gint                   selection_mode;
   GList                 *selection;
   gint                   focus;
   gint                   anchor;

   GimvZListRegionSelectMode  region_select;
   GdkGC                 *region_line_gc;
   GList                 *selection_mask;

   gint                   cell_x_pad, cell_y_pad;
   gint                   x_pad, y_pad;

   GtkWidget             *entered_cell;
};

struct _GimvZListClass
{
   GimvScrolledClass parent_class;

   void    (*clear)             (GimvZList      *list);
   void    (*cell_draw)         (GimvZList      *list,
                                 gpointer        cell,
                                 GdkRectangle   *cell_area,
                                 GdkRectangle   *area);
   void    (*cell_size_request) (GimvZList      *list,
                                 gpointer        cell,
                                 GtkRequisition *requisition);
   void    (*cell_draw_focus)   (GimvZList      *list,
                                 gpointer        cell,
                                 GdkRectangle   *cell_area);
   void    (*cell_draw_default) (GimvZList      *list,
                                 gpointer        cell,
                                 GdkRectangle   *cell_area);
   void    (*cell_select)       (GimvZList      *list,
                                 gint            index);
   void    (*cell_unselect)     (GimvZList      *list,
                                 gint            index);
};


GType      gimv_zlist_get_type                    (void);

void       gimv_zlist_construct                   (GimvZList        *list,
                                                   gint              flags);
GtkWidget *gimv_zlist_new                         (guint             flags);
void       gimv_zlist_set_to_vertical             (GimvZList        *zlist);
void       gimv_zlist_set_to_horizontal           (GimvZList        *zlist);
guint      gimv_zlist_add                         (GimvZList        *list,
                                                   gpointer          cell);
guint      gimv_zlist_insert                      (GimvZList        *list,
                                                   guint             pos,
                                                   gpointer          cell);
void       gimv_zlist_remove                      (GimvZList        *list,
                                                   gpointer          cell);
void       gimv_zlist_clear                       (GimvZList        *list);
void       gimv_zlist_set_selection_mode          (GimvZList        *list,
                                                   GtkSelectionMode  mode);
void       gimv_zlist_set_cell_padding            (GimvZList        *list,
                                                   gint              x_pad,
                                                   gint              y_pad);
void       gimv_zlist_set_cell_size               (GimvZList        *list,
                                                   gint              width,
                                                   gint              height);
void       gimv_zlist_set_1                       (GimvZList        *list,
                                                   gint              one);
gpointer   gimv_zlist_cell_from_xy                (GimvZList        *list,
                                                   gint              x,
                                                   gint              y);
gint       gimv_zlist_cell_index_from_xy          (GimvZList        *list,
                                                   gint              x,
                                                   gint              y);
void       gimv_zlist_cell_select                 (GimvZList        *list,
                                                   gint              index);
void       gimv_zlist_cell_unselect               (GimvZList        *list,
                                                   gint              index);
void       gimv_zlist_cell_toggle                 (GimvZList        *list,
                                                   gint              index);
void       gimv_zlist_unselect_all                (GimvZList        *list);
void       gimv_zlist_extend_selection            (GimvZList        *list,
                                                   gint              to);
void       gimv_zlist_cell_select_by_region       (GimvZList        *list,
                                                   guint             start_col,
                                                   guint             start_row,
                                                   guint             end_col,
                                                   guint             end_row);
void       gimv_zlist_cell_select_by_pixel_region (GimvZList        *list,
                                                   gint              start_x,
                                                   gint              start_y,
                                                   gint              end_x,
                                                   gint              end_y);
void       gimv_zlist_set_selection_mask          (GimvZList        *list,
                                                   GList            *mask_list);
void       gimv_zlist_unset_selection_mask        (GimvZList        *list);
void       gimv_zlist_moveto                      (GimvZList        *list,
                                                   gint              index);
void       gimv_zlist_cell_set_focus              (GimvZList        *list,
                                                   gint              index);
void       gimv_zlist_cell_unset_focus            (GimvZList        *list);

gboolean   gimv_zlist_get_cell_area               (GimvZList        *list,
                                                   gint              index,
                                                   GdkRectangle     *area);

/* protected */
#define    GIMV_ZLIST_CELL_FROM_INDEX(list, index) \
   (g_array_index (GIMV_ZLIST(list)->cells, gpointer, index))

void       gimv_zlist_draw_cell          (GimvZList    *list,
                                          gint          index);
gint       gimv_zlist_cell_index         (GimvZList    *list,
                                          gpointer      cell);
gint       gimv_zlist_update_cell_size   (GimvZList    *list,
                                          gpointer      cell);

#endif /* _GIMV_ZLIST_H_ */
