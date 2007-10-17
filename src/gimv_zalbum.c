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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gimv_zalbum.h"

#define GIMV_ZALBUM_CELL(ptr)   ((GimvZAlbumCell *) (ptr))
#define bw(widget)         GTK_CONTAINER(widget)->border_width

#define CELL_PADDING 4
#define LABEL_HPADDING 8
#define LABEL_VPADDING 4
#define LINE_HEIGHT(font)  ((font)->ascent + (font)->descent + LABEL_VPADDING)
#define STRING_BUFFER_SIZE 1024
#define CELL_STATE(cell)   (GIMV_ZALBUM_CELL (cell)->flags & GIMV_ZALBUM_CELL_SELECTED ? GTK_STATE_SELECTED : GTK_STATE_NORMAL)

static void gimv_zalbum_finalize            (GObject *object);
static void gimv_zalbum_clear               (GimvZList      *list);
static void gimv_zalbum_cell_size_request   (GimvZList      *list,
                                             gpointer        cell,
                                             GtkRequisition *requisition);
static void gimv_zalbum_draw_cell           (GimvZList      *list,
                                             gpointer        cell,
                                             GdkRectangle   *cell_area,
                                             GdkRectangle   *area);
static gint draw_cell_pixmap                (GdkWindow      *window,
                                             GdkRectangle   *clip_rectangle,
                                             GdkGC          *fg_gc,
                                             GdkPixmap      *pixmap,
                                             GdkBitmap      *mask,
                                             gint            x,
                                             gint            y,
                                             gint            width,
                                             gint            height);
static void gimv_zalbum_draw                (GimvZAlbum     *album,
                                             GimvZAlbumCell *cell,
                                             GdkRectangle   *cell_area,
                                             GdkRectangle   *area);
static gint make_string                     (PangoLayout    *layout,
                                             gint            max_width,
                                             gchar          *buffer,
                                             gint            buffer_size);
static void gimv_zalbum_draw_string         (GtkWidget      *widget,
                                             GimvZAlbumCell *cell,
                                             const gchar    *string,
                                             gint            x,
                                             gint            y,
                                             gint            max_width,
                                             gint            max_heihgt,
                                             gint            center);
static void gimv_zalbum_prepare_cell        (GimvZAlbum     *album,
                                             GimvZAlbumCell *cell,
                                             GdkRectangle   *cell_area,
                                             GdkRectangle   *area);
static void gimv_zalbum_update_max_cell_size(GimvZAlbum     *album,
                                             GimvZAlbumCell *cell);
static void gimv_zalbum_cell_draw_focus     (GimvZList      *list,
                                             gpointer        cell,
                                             GdkRectangle   *cell_area);
static void gimv_zalbum_cell_draw_default   (GimvZList      *list,
                                             gpointer        cell,
                                             GdkRectangle   *cell_area);
static void gimv_zalbum_cell_select         (GimvZList      *list,
                                             int             index);
static void gimv_zalbum_cell_unselect       (GimvZList      *list,
                                             int             index);


/* static GdkFont *album_font = NULL; */


G_DEFINE_TYPE (GimvZAlbum, gimv_zalbum, GIMV_TYPE_ZLIST)


static void
gimv_zalbum_class_init (GimvZAlbumClass *klass) {
   GObjectClass *gobject_class;
   GtkWidgetClass *widget_class;
   GimvZListClass *zlist_class;

   gobject_class = (GObjectClass *) klass;
   widget_class  = (GtkWidgetClass *) klass;
   zlist_class   = (GimvZListClass *) klass;

   gobject_class->finalize          = gimv_zalbum_finalize;

   zlist_class->clear               = gimv_zalbum_clear;
   zlist_class->cell_draw           = gimv_zalbum_draw_cell;
   zlist_class->cell_size_request   = gimv_zalbum_cell_size_request;
   zlist_class->cell_draw_focus     = gimv_zalbum_cell_draw_focus;
   zlist_class->cell_draw_default   = gimv_zalbum_cell_draw_default;
   zlist_class->cell_select         = gimv_zalbum_cell_select;
   zlist_class->cell_unselect       = gimv_zalbum_cell_unselect;
}


static void
gimv_zalbum_init(GimvZAlbum *zalbum) {
   zalbum->label_pos       = GIMV_ZALBUM_CELL_LABEL_BOTTOM;
   zalbum->max_pix_width   = 0;
   zalbum->max_pix_height  = 0;
   zalbum->max_cell_width  = 0;
   zalbum->max_cell_height = 0;
}


GtkWidget *
gimv_zalbum_new (void) {
   GimvZAlbum *album;
   GtkRequisition requisition;
   gint flags = 0;

   album = g_object_new (gimv_zalbum_get_type (), NULL);
   g_return_val_if_fail (album != NULL, NULL);

   /* flags |= GIMV_ZLIST_HORIZONTAL; */

   gimv_zlist_construct (GIMV_ZLIST(album), flags);
   gimv_zlist_set_selection_mode (GIMV_ZLIST (album), GTK_SELECTION_EXTENDED);

   gimv_zalbum_cell_size_request (GIMV_ZLIST (album), NULL, &requisition);
   gimv_zlist_set_cell_padding (GIMV_ZLIST (album), 4, 4);
   gimv_zlist_set_1 (GIMV_ZLIST (album), 0);
   gimv_zlist_set_cell_size (GIMV_ZLIST (album), requisition.width, requisition.height);

   album->len = 0;

   return (GtkWidget *) album;
}


static void
gimv_zalbum_finalize (GObject *object)
{
   gimv_zalbum_clear (GIMV_ZLIST (object));

   if (G_OBJECT_CLASS(gimv_zalbum_parent_class)->finalize)
      G_OBJECT_CLASS(gimv_zalbum_parent_class)->finalize (object);
}


guint
gimv_zalbum_add (GimvZAlbum *album, const gchar *name)
{
   g_return_val_if_fail (GIMV_IS_ZALBUM (album), 0);

   return gimv_zalbum_insert (album, GIMV_ZLIST(album)->cells->len, name);
}


guint
gimv_zalbum_insert (GimvZAlbum *album, guint pos, const gchar *name)
{
   GimvZAlbumCell *cell;

   g_return_val_if_fail (GIMV_IS_ZALBUM (album), 0);

   cell = g_new(GimvZAlbumCell, 1);

   if (name)
      cell->name   = g_strdup (name);
   else
      cell->name   = NULL;

   cell->ipix      = NULL;
   cell->flags     = 0;
   cell->user_data = NULL;

   album->len++;

   return gimv_zlist_insert (GIMV_ZLIST (album), pos, cell);
}


void        
gimv_zalbum_remove (GimvZAlbum *album, guint pos)
{
   GimvZAlbumCell *cell;

   g_return_if_fail (GIMV_IS_ZALBUM (album));

   cell = GIMV_ZLIST_CELL_FROM_INDEX (GIMV_ZLIST (album), pos);
   g_return_if_fail (cell);

   gimv_zlist_remove(GIMV_ZLIST(album), cell);

   g_free ((gpointer) cell->name);
   if (cell->ipix)
      gdk_pixmap_unref (cell->ipix);
   cell->ipix = NULL;

   if (cell->user_data && cell->destroy)
      cell->destroy (cell->user_data);
   cell->destroy = NULL;
   cell->user_data = NULL;

   g_free (cell);
}


static void
gimv_zalbum_clear (GimvZList *list)
{
   GimvZAlbumCell *cell;
   gint i;

   g_return_if_fail (list);

   for (i = 0; i < list->cell_count; i++) {
      cell = GIMV_ZLIST_CELL_FROM_INDEX (list, i);
      g_free ((gpointer) cell->name);
      if (cell->ipix)
         gdk_pixmap_unref (cell->ipix);
      cell->ipix = NULL;

      if (cell->user_data && cell->destroy)
         cell->destroy (cell->user_data);
      cell->destroy = NULL;
      cell->user_data = NULL;

      g_free (cell);
   }

   GIMV_ZALBUM (list)->len = 0;
}


static void
gimv_zalbum_cell_size_request (GimvZList *list, gpointer cell, GtkRequisition *requisition)
{
   GimvZAlbum *album;
   gint text_height = 0;

   g_return_if_fail (list && requisition);

   album = GIMV_ZALBUM (list);

   switch (album->label_pos) {
   case GIMV_ZALBUM_CELL_LABEL_LEFT:
   case GIMV_ZALBUM_CELL_LABEL_RIGHT:
      text_height = 0;
      break;
   case GIMV_ZALBUM_CELL_LABEL_BOTTOM:
   case GIMV_ZALBUM_CELL_LABEL_TOP:
   default:
      text_height = LINE_HEIGHT (gtk_style_get_font (GTK_WIDGET (list)->style));
      break;
   }

   requisition->width
      = MAX (album->max_cell_width, album->max_pix_width)
      + 2 * CELL_PADDING;
   requisition->height
      = MAX (album->max_cell_height, album->max_pix_height)
      + 2 * CELL_PADDING + text_height;
}


static void
gimv_zalbum_draw_cell (GimvZList *list,
                       gpointer cell,
                       GdkRectangle *cell_area,
                       GdkRectangle *area)
{
   g_return_if_fail (list && cell);

   gimv_zalbum_draw ((GimvZAlbum *) list, (GimvZAlbumCell *) cell, cell_area, area);
}


static gint
draw_cell_pixmap (GdkWindow    *window,
                  GdkRectangle *clip_rectangle,
                  GdkGC        *fg_gc,
                  GdkPixmap    *pixmap,
                  GdkBitmap    *mask,
                  gint          x,
                  gint          y,
                  gint          width,
                  gint          height)
{
   gint xsrc = 0, ysrc = 0;

   if (mask) {
      gdk_gc_set_clip_mask (fg_gc, mask);
      gdk_gc_set_clip_origin (fg_gc, x, y);
   }

   if (x < clip_rectangle->x) {
      xsrc = clip_rectangle->x - x;
      width -= xsrc;
      x = clip_rectangle->x;
   }
   if (x + width > clip_rectangle->x + clip_rectangle->width)
      width = clip_rectangle->x + clip_rectangle->width - x;

   if (y < clip_rectangle->y) {
      ysrc = clip_rectangle->y - y;
      height -= ysrc;
      y = clip_rectangle->y;
   }
   if (y + height > clip_rectangle->y + clip_rectangle->height)
      height = clip_rectangle->y + clip_rectangle->height - y;

   gdk_draw_pixmap (window, fg_gc, pixmap, xsrc, ysrc, x, y, width, height);

   if (mask) {
      gdk_gc_set_clip_origin (fg_gc, 0, 0);
      gdk_gc_set_clip_mask (fg_gc, NULL);
   }

   return x + MAX (width, 0);
}


static void
gimv_zalbum_draw (GimvZAlbum *album, GimvZAlbumCell *cell,
                  GdkRectangle *cell_area,
                  GdkRectangle *area)
{
   GtkWidget *widget;
   gint text_area_height, v_text_area_height, xdest, ydest;
   gint xpad, ypad;
   gboolean center = TRUE;
   GdkRectangle widget_area, draw_area;

   g_return_if_fail (album && cell && cell_area);

   widget = GTK_WIDGET (album);

   if (!GTK_WIDGET_DRAWABLE (widget))
      return;

   widget_area.x = widget_area.y = 0;
   widget_area.width  = widget->allocation.width;
   widget_area.height = widget->allocation.height;
   if (!area) {
      draw_area = widget_area;
   } else {
      if (!gdk_rectangle_intersect (area, &widget_area, &draw_area))
         return;
   }

   gimv_zalbum_prepare_cell (album, cell, cell_area, &draw_area);

   switch (album->label_pos) {
   case GIMV_ZALBUM_CELL_LABEL_RIGHT:
   case GIMV_ZALBUM_CELL_LABEL_LEFT:
      text_area_height = cell_area->height;
      v_text_area_height = 0;
      break;
   case GIMV_ZALBUM_CELL_LABEL_BOTTOM:
   case GIMV_ZALBUM_CELL_LABEL_TOP:
   default:
      text_area_height = LINE_HEIGHT (gtk_style_get_font (GTK_WIDGET (album)->style));
      v_text_area_height = text_area_height;
      break;
   }

   if (cell->ipix) {
      GdkRectangle pixmap_area, intersect_area;
      gboolean need_draw;
      gint w, h;
      gint iwidth = 0, iheight = 0;

      gdk_window_get_size (cell->ipix, &iwidth, &iheight);

      w    = iwidth;
      h    = iheight;

      switch (album->label_pos) {
      case GIMV_ZALBUM_CELL_LABEL_RIGHT:
         xpad = 0;
         break;
      case GIMV_ZALBUM_CELL_LABEL_LEFT:
         /* not implemented yet */
         /* break; */
      case GIMV_ZALBUM_CELL_LABEL_BOTTOM:
      case GIMV_ZALBUM_CELL_LABEL_TOP:
      default:
         xpad = (cell_area->width  - w)  / 2;
         break;
      }
      ypad = (cell_area->height - v_text_area_height - h) / 2;

      if (xpad < 0) {
         w += xpad;
         xpad = 0;
      }

      if (ypad < 0) {
         h += ypad;
         ypad = 0;
      }

      pixmap_area.x = cell_area->x + xpad;
      pixmap_area.y = cell_area->y + ypad;
      pixmap_area.width  = iwidth;
      pixmap_area.height = iheight;

      need_draw = gdk_rectangle_intersect (&draw_area, &pixmap_area, &intersect_area);

      if (cell->ipix && need_draw) {
         draw_cell_pixmap (widget->window,
                           &intersect_area,
                           widget->style->fg_gc[GTK_STATE_NORMAL],
                           cell->ipix, cell->imask,
                           pixmap_area.x, pixmap_area.y,
                           iwidth, iheight);
      }
   }

   switch (album->label_pos) {
   case GIMV_ZALBUM_CELL_LABEL_RIGHT:
      xdest = cell_area->x + album->max_pix_width + LABEL_HPADDING;
      ydest = cell_area->y
         + gtk_style_get_font (GTK_WIDGET (album)->style)->ascent;
      center = FALSE;
      break;

   case GIMV_ZALBUM_CELL_LABEL_LEFT:
      /* not implemented yet */
      /* break; */

   case GIMV_ZALBUM_CELL_LABEL_TOP:
      /* not implemented yet */
      /* break; */

   case GIMV_ZALBUM_CELL_LABEL_BOTTOM:
   default:
      xdest = cell_area->x;
      ydest = (cell_area->y + cell_area->height - v_text_area_height)
         + gtk_style_get_font (GTK_WIDGET (album)->style)->ascent
         + LABEL_VPADDING;
      center = TRUE;
      break;
   }

   gimv_zalbum_draw_string (widget, cell, cell->name,
                            xdest, ydest,
                            cell_area->width, text_area_height,
                            center);

}


/* FIXME */
static void
get_string_area_size (GimvZAlbum *album, const gchar *str,
                      gint *width_ret, gint *height_ret, gint *lines_ret)
{
   PangoLayout *layout;
   gint i, width, height, lines = 1;

   for (i = 0; str && str[i]; i++)
      if (str[i] == '\n') lines++;

   layout = gtk_widget_create_pango_layout (GTK_WIDGET (album), "(NULL)");
   pango_layout_set_text (layout, str, -1);
   pango_layout_get_pixel_size (layout, &width, &height);

   if (width_ret)
      *width_ret = width;
   if (height_ret)
      *height_ret = height;

   if (lines_ret) {
      *lines_ret = lines;
   }

   g_object_unref (layout);
}


static gint
make_string (PangoLayout *layout, gint max_width,
             gchar *buffer, gint buffer_size)
{
   gint utf8len, len, src_len, dots_width, width, height;

   pango_layout_set_text (layout, "...", -1);
   pango_layout_get_pixel_size (layout, &dots_width, &height);

   utf8len = g_utf8_strlen (buffer, -1);
   len = src_len = strlen (buffer);

   while (utf8len > 0 && len > 0) {
      len = g_utf8_offset_to_pointer(buffer, utf8len) - buffer;

      pango_layout_set_text (layout, buffer, len);
      pango_layout_get_pixel_size (layout, &width, &height);
      if (width + dots_width > max_width) {
         utf8len--;
      } else {
         break;
      }
   }

   if (len < src_len && len < buffer_size - 4) {
      gchar *str = g_utf8_offset_to_pointer (buffer, utf8len);
      str[0] = '.';
      str[1] = '.';
      str[2] = '.';
      str[3] = '\0';
      return len + 3;
   } else {
      buffer[buffer_size - 1] = '\0';
      return buffer_size;
   }
}


static void
gimv_zalbum_draw_string (GtkWidget *widget, GimvZAlbumCell *cell,
                         const gchar *string,
                         gint x, gint y,
                         gint max_width, gint max_height,
                         gint center)
{
   gchar buffer[STRING_BUFFER_SIZE];
   gint x_pad, y_pad;
   gint width = 0, height = 0, str_height, len;
   PangoLayout *layout;
   gint vpad = LABEL_VPADDING;

   if (!string || !*string) return;

   layout = gtk_widget_create_pango_layout (widget, "(NULL)");
   pango_layout_get_pixel_size (layout, &width, &height);
   str_height = height;
   pango_layout_set_text (layout, string, -1);
   pango_layout_get_pixel_size (layout, &width, &height);

   len = strlen(string);
   strncpy (buffer, string, MIN (len, STRING_BUFFER_SIZE));
   buffer[MIN (len, STRING_BUFFER_SIZE - 1)] = '\0';

   x_pad = (max_width - width) / 2;

   if (x_pad < 0) {
      make_string (layout, max_width,
                   buffer, STRING_BUFFER_SIZE);
   } else {
      gint len = MIN (strlen (string), STRING_BUFFER_SIZE - 1);
      strncpy (buffer, string, len);
      buffer[len] = '\0';
   }

   pango_layout_set_text (layout, buffer, -1);
   pango_layout_get_pixel_size (layout, &width, &height);
   x_pad = (max_width - width) / 2;

   if (!center)
      x_pad = 0;

   if (GIMV_ZALBUM (widget)->label_pos == GIMV_ZALBUM_CELL_LABEL_TOP
       || GIMV_ZALBUM (widget)->label_pos == GIMV_ZALBUM_CELL_LABEL_BOTTOM)
   {
      vpad /= 2;
   }

   if (height < max_height) {
      y_pad = (max_height - height) / 2 +  vpad;
   } else {
      y_pad = vpad;
   }

   gdk_draw_layout (widget->window,
                    widget->style->fg_gc[CELL_STATE(cell)],
                    x + x_pad,
                    y + y_pad - str_height,
                    layout);

  g_object_unref (layout);
}
/* END FIXME!! */


static void
gimv_zalbum_prepare_cell (GimvZAlbum *album,
                          GimvZAlbumCell *cell,
                          GdkRectangle *cell_area,
                          GdkRectangle *area)
{
   GtkWidget *widget;
   GdkRectangle intersect_area;

   widget = GTK_WIDGET(album);

   if (gdk_rectangle_intersect (area, cell_area, &intersect_area))
      gdk_draw_rectangle (widget->window,
                          widget->style->bg_gc[CELL_STATE(cell)], TRUE,
                          intersect_area.x, intersect_area.y,
                          intersect_area.width, intersect_area.height);

   gtk_paint_shadow (widget->style, widget->window,
                     CELL_STATE(cell), GTK_SHADOW_OUT,
                     NULL, NULL, NULL,
                     cell_area->x, cell_area->y,
                     cell_area->width, cell_area->height);

   cell_area->x += CELL_PADDING;
   cell_area->y += CELL_PADDING;
   cell_area->width -= CELL_PADDING * 2;
   cell_area->height -= CELL_PADDING * 2;
}


void
gimv_zalbum_set_label_position (GimvZAlbum *album, GimvZAlbumLabelPosition pos)
{
   g_return_if_fail (GIMV_IS_ZALBUM (album));

   album->label_pos = pos;

   if (GTK_WIDGET_VISIBLE (album)) {
      gtk_widget_queue_draw (GTK_WIDGET (album));
   }
}


void
gimv_zalbum_set_min_pixmap_size (GimvZAlbum *album,
                                 guint width, guint height)
{
   g_return_if_fail (GIMV_IS_ZALBUM (album));

   album->max_pix_width  = width;
   album->max_pix_height = height;
}


void
gimv_zalbum_set_min_cell_size (GimvZAlbum *album,
                               guint width, guint height)
{
   g_return_if_fail (GIMV_IS_ZALBUM (album));

   album->max_cell_width  = width;
   album->max_cell_height = height;
}


static void
gimv_zalbum_update_max_cell_size (GimvZAlbum *album, GimvZAlbumCell *cell)
{
   gint pix_width, pix_height, iwidth, iheight, cell_width, cell_height;
   gint string_area_width, str_width, str_height;

   g_return_if_fail (album);
   g_return_if_fail (cell);

   if (cell->ipix) {
      gdk_window_get_size (cell->ipix, &iwidth, &iheight);
   } else {
      iwidth  = album->max_pix_width;
      iheight = album->max_pix_height;
   }

   album->max_pix_width  = MAX(album->max_pix_width,  iwidth);
   album->max_pix_height = MAX(album->max_pix_height, iheight);

   if (cell->name) {
      get_string_area_size (album, cell->name,
                            &str_width, &str_height, NULL);
      /*
      str_width  = gdk_string_width (font, cell->name);
      str_height = gdk_string_height (font, cell->name);
      */
   } else {
      str_width  = 0;
      str_height = 0;
   }

   switch (album->label_pos) {
   case GIMV_ZALBUM_CELL_LABEL_LEFT:
   case GIMV_ZALBUM_CELL_LABEL_RIGHT:
   {
      pix_width  = album->max_pix_width;
      pix_height = album->max_pix_height;
      string_area_width = str_width + LABEL_HPADDING;

      break;
   }
   case GIMV_ZALBUM_CELL_LABEL_BOTTOM:
   case GIMV_ZALBUM_CELL_LABEL_TOP:
   default:
      pix_width  = iwidth;
      pix_height = iheight;
      string_area_width = 0;

      break;
   }

   cell_width  = pix_width + string_area_width;
   cell_height = MAX (pix_height, str_height);

   album->max_cell_width 
      = MAX (MAX(album->max_cell_width,  cell_width), album->max_pix_width);
   album->max_cell_height
      = MAX (MAX(album->max_cell_height, cell_height), album->max_pix_height);
}


void
gimv_zalbum_set_pixmap (GimvZAlbum *album, guint idx,
                        GdkPixmap *pixmap, GdkBitmap *mask)
{
   GimvZAlbumCell *cell;

   g_return_if_fail (GIMV_IS_ZALBUM (album));

   cell = GIMV_ZLIST_CELL_FROM_INDEX(album, idx);

   g_return_if_fail (cell);

   if (cell->ipix)
      gdk_pixmap_unref (cell->ipix);
   cell->ipix  = NULL;
   cell->imask = NULL;

   if (!pixmap) return;

   cell->ipix  = gdk_pixmap_ref (pixmap);
   if (mask) cell->imask = gdk_bitmap_ref (mask);

   if (cell->ipix) {
      gimv_zalbum_update_max_cell_size (album, cell);

      if (!gimv_zlist_update_cell_size(GIMV_ZLIST(album), (gpointer) cell))
         gimv_zlist_draw_cell (GIMV_ZLIST(album), idx);
   }
}


void
gimv_zalbum_set_cell_data (GimvZAlbum *album, guint idx, gpointer user_data)
{
   gimv_zalbum_set_cell_data_full (album, idx, user_data, NULL);
}


void
gimv_zalbum_set_cell_data_full (GimvZAlbum *album,
                                guint idx,
                                gpointer user_data,
                                GtkDestroyNotify  destroy)
{
   GimvZAlbumCell *cell;

   g_return_if_fail (GIMV_IS_ZALBUM (album));

   if (idx < 0 || idx >= album->len) return;

   cell = GIMV_ZLIST_CELL_FROM_INDEX (GIMV_ZLIST (album), idx);
   g_return_if_fail (cell);

   cell->user_data = user_data;
   cell->destroy = destroy;
}


gpointer
gimv_zalbum_get_cell_data (GimvZAlbum *album, guint idx)
{
   GimvZAlbumCell *cell;

   g_return_val_if_fail (GIMV_IS_ZALBUM (album), NULL);

   if (idx < 0 || idx >= album->len) return NULL;

   cell = GIMV_ZLIST_CELL_FROM_INDEX (GIMV_ZLIST (album), idx);
   g_return_val_if_fail (cell, NULL);

   return cell->user_data;
}


static void
gimv_zalbum_cell_draw_focus (GimvZList *list, gpointer cell, GdkRectangle *cell_area)
{
   g_return_if_fail (GIMV_IS_ZALBUM (list));

   if (!GTK_WIDGET_MAPPED (list)) return;

   gtk_paint_shadow(GTK_WIDGET(list)->style, GTK_WIDGET(list)->window,
                    CELL_STATE(cell), GTK_SHADOW_IN,
                    NULL, NULL, NULL,
                    cell_area->x, cell_area->y,
                    cell_area->width, cell_area->height);
}


static void
gimv_zalbum_cell_draw_default (GimvZList *list, gpointer cell, GdkRectangle *cell_area)
{
   g_return_if_fail (GIMV_IS_ZALBUM (list));

   if (!GTK_WIDGET_MAPPED (list)) return;

   gtk_paint_shadow(GTK_WIDGET(list)->style, GTK_WIDGET(list)->window,
                    CELL_STATE(cell), GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    cell_area->x, cell_area->y,
                    cell_area->width, cell_area->height);
}


static void
gimv_zalbum_cell_select (GimvZList *list, int index)
{
   GimvZAlbumCell *cell;

   cell = GIMV_ZLIST_CELL_FROM_INDEX (list, index);
   g_return_if_fail (cell);

   GIMV_ZALBUM_CELL (cell)->flags |= GIMV_ZALBUM_CELL_SELECTED;
}


static void
gimv_zalbum_cell_unselect (GimvZList *list, int index)
{
   GimvZAlbumCell *cell;

   cell = GIMV_ZLIST_CELL_FROM_INDEX (list, index);
   g_return_if_fail (cell);

   GIMV_ZALBUM_CELL (cell)->flags &= ~GIMV_ZALBUM_CELL_SELECTED;
}
