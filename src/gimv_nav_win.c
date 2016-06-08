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
 * These codes are mostly taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#include <math.h>

#include "gimageview.h"
#include "gimv_nav_win.h"

G_DEFINE_TYPE (GimvNavWin, gimv_nav_win, GTK_TYPE_WINDOW)

#define GIMV_NAV_WIN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMV_TYPE_NAV_WIN, GimvNavWinPrivate))
#define PEN_WIDTH         3       /* Square border width. */ 
#define BORDER_WIDTH      4       /* Window border width. */

enum {
   MOVE_SIGNAL,
   LAST_SIGNAL
};

typedef struct GimvNavWinPrivate_Tag
{
   GtkWidget *out_frame;
   GtkWidget *in_frame;
   GtkWidget *preview;

   GdkPixmap *pixmap;
   GdkBitmap *mask;

   GdkGC *gc;

   gint x_root, y_root;

   gint image_width, image_height;

   gint view_width, view_height;
   gint view_pos_x, view_pos_y;

   gint popup_x, popup_y;
   gint popup_width, popup_height;

   gint sqr_x, sqr_y;
   gint sqr_width, sqr_height;

   gint fix_x_pos, fix_y_pos;

   gdouble factor;
} GimvNavWinPrivate;

static gint gimv_nav_win_signals[LAST_SIGNAL] = {0};

/* object class */
static void     gimv_nav_win_dispose        (GObject         *object);

/* widget class */
static void     gimv_nav_win_realize        (GtkWidget       *widget);
static void     gimv_nav_win_unrealize      (GtkWidget       *widget);
static gboolean gimv_nav_win_expose         (GtkWidget       *widget,
                                             GdkEventExpose  *event);
static gboolean gimv_nav_win_key_press      (GtkWidget       *widget, 
                                             GdkEventKey     *event);
static gboolean gimv_nav_win_button_release (GtkWidget       *widget,
                                             GdkEventButton  *event);
static gboolean gimv_nav_win_motion_notify  (GtkWidget       *widget,
                                             GdkEventMotion  *event);
/* nav_win class */
static void      navwin_draw_sqr            (GimvNavWin      *navwin,
                                             gboolean         undraw,
                                             gint             x,
                                             gint             y);
static void      get_sqr_origin_as_double   (GimvNavWin      *navwin,
                                             gint             mx,
                                             gint             my,
                                             gdouble         *x,
                                             gdouble         *y);
static void      navwin_update_view         (GimvNavWin      *navwin);
static void      navwin_draw                (GimvNavWin      *navwin);
static void      navwin_grab_pointer        (GimvNavWin      *navwin);
static void      navwin_set_win_pos_size    (GimvNavWin *navwin);


static void
gimv_nav_win_class_init (GimvNavWinClass *klass)
{
   GObjectClass *gobject_class;
   GtkWidgetClass *widget_class;

   gobject_class = (GObjectClass *) klass;
   widget_class  = (GtkWidgetClass *) klass;

   gobject_class->dispose             = gimv_nav_win_dispose;

   widget_class->realize              = gimv_nav_win_realize;
   widget_class->unrealize            = gimv_nav_win_unrealize;
   widget_class->expose_event         = gimv_nav_win_expose;
   widget_class->key_press_event      = gimv_nav_win_key_press;
   widget_class->button_release_event = gimv_nav_win_button_release;
   widget_class->motion_notify_event  = gimv_nav_win_motion_notify;

   gimv_nav_win_signals[MOVE_SIGNAL]
      = g_signal_new ("move",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvNavWinClass, move),
                      NULL, NULL,
                      gtk_marshal_NONE__INT_INT,
                      G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

   g_type_class_add_private (gobject_class, sizeof (GimvNavWinPrivate));
}


static void
gimv_nav_win_init (GimvNavWin *navwin)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->fix_x_pos = priv->fix_y_pos = 1;

   priv->gc           = NULL;
   priv->pixmap       = NULL;
   priv->mask         = NULL;

   priv->out_frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (priv->out_frame), GTK_SHADOW_OUT);
   gtk_container_add (GTK_CONTAINER (navwin), priv->out_frame);
   gtk_widget_show (priv->out_frame);

   priv->in_frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (priv->in_frame), GTK_SHADOW_IN);
   gtk_container_add (GTK_CONTAINER (priv->out_frame), priv->in_frame);
   gtk_widget_show (priv->in_frame);

   priv->preview = gtk_drawing_area_new ();
   gtk_container_add (GTK_CONTAINER (priv->in_frame), priv->preview);
   gtk_widget_show (priv->preview);
}


static void
gimv_nav_win_dispose (GObject *object)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (object);
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   if (priv->pixmap)
      gdk_pixmap_unref (priv->pixmap);
   priv->pixmap = NULL;

   if (G_OBJECT_CLASS (gimv_nav_win_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_nav_win_parent_class)->dispose (object);
}


static void
gimv_nav_win_realize (GtkWidget *widget)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->realize)
      GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->realize (widget);

   if (!priv->gc) {
      priv->gc = gdk_gc_new (widget->window);
      gdk_gc_set_function (priv->gc, GDK_INVERT);
      gdk_gc_set_line_attributes (priv->gc, 
                                  PEN_WIDTH, 
                                  GDK_LINE_SOLID, 
                                  GDK_CAP_BUTT, 
                                  GDK_JOIN_MITER);
   }
}


static void
gimv_nav_win_unrealize (GtkWidget *widget)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   if (priv->gc)
      gdk_gc_destroy (priv->gc);
   priv->gc = NULL;

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->unrealize)
      GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->unrealize (widget);
}


static gboolean
gimv_nav_win_expose (GtkWidget *widget,
                     GdkEventExpose *event)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);

   navwin_draw (navwin);

   if(gtk_grab_get_current() != GTK_WIDGET (navwin))
      navwin_grab_pointer(navwin);

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->expose_event)
      return GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->expose_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_key_press (GtkWidget *widget, 
                        GdkEventKey *event)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);
   gboolean move = FALSE;
   gint mx, my;
   guint keyval;

   mx = priv->view_pos_x;
   my = priv->view_pos_y;

   keyval = event->keyval;

   if (keyval == GDK_Left) {
      mx -= 10;
      move = TRUE;
   } else if (keyval == GDK_Right) {
      mx += 10;
      move = TRUE;
   } else if (keyval == GDK_Up) {
      my -= 10;
      move = TRUE;
   } else if (keyval == GDK_Down) {
      my += 10;
      move = TRUE;
   }

   if (move) {
      if (priv->fix_x_pos < 0) mx = priv->fix_x_pos;
      if (priv->fix_y_pos < 0) my = priv->fix_y_pos;
      priv->view_pos_x = mx;
      priv->view_pos_y = my;
      g_signal_emit (G_OBJECT (navwin),
                     gimv_nav_win_signals[MOVE_SIGNAL], 0,
                     mx, my);
      mx *= priv->factor;
      my *= priv->factor;
      navwin_draw_sqr (navwin, TRUE, mx, my);
   }

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->key_press_event)
      return GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->key_press_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_button_release  (GtkWidget *widget,
                              GdkEventButton *event)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);

   switch (event->button) {
   case 1:
      g_signal_stop_emission_by_name (G_OBJECT (widget),
                                      "button_release_event");
      gimv_nav_win_hide (navwin);
      /* gtk_widget_destroy (GTK_WIDGET (navwin)); */
      return TRUE;
      break;
   default:
      break;
   }

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->button_release_event)
      return GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->button_release_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_motion_notify (GtkWidget *widget,
                            GdkEventMotion *event)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (widget);
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);
   GdkModifierType mask;
   gint mx, my;
   gdouble x, y;

   gdk_window_get_pointer (widget->window, &mx, &my, &mask);
   get_sqr_origin_as_double (navwin, mx, my, &x, &y);

   mx = (gint) x;
   my = (gint) y;
   navwin_draw_sqr (navwin, TRUE, mx, my);

   mx = (gint) (x / priv->factor);
   my = (gint) (y / priv->factor);
   if (priv->fix_x_pos < 0) mx = priv->fix_x_pos;
   if (priv->fix_y_pos < 0) my = priv->fix_y_pos;

   g_signal_emit (G_OBJECT (navwin),
                  gimv_nav_win_signals[MOVE_SIGNAL], 0,
                  mx, my);

   if (GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->motion_notify_event)
      return GTK_WIDGET_CLASS (gimv_nav_win_parent_class)->motion_notify_event (widget, event);

   return FALSE;
}


GtkWidget *
gimv_nav_win_new (GdkPixmap *pixmap, GdkBitmap *mask,
                  gint image_width, gint image_height,
                  gint view_width, gint view_height,
                  gint fpos_x, gint fpos_y)
{
   GimvNavWin *navwin;
   GimvNavWinPrivate *priv;

   g_return_val_if_fail (pixmap, NULL);

   navwin = GIMV_NAV_WIN (g_object_new (GIMV_TYPE_NAV_WIN,
                                        "type", GTK_WINDOW_POPUP,
                                        NULL));
   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->pixmap = gdk_pixmap_ref (pixmap);
   if (mask) priv->mask = gdk_bitmap_ref (mask);

   priv->x_root = 0;
   priv->y_root = 0;

   priv->image_width  = image_width;
   priv->image_height = image_height;

   priv->view_width   = view_width;
   priv->view_height  = view_height;
   priv->view_pos_x   = fpos_x;
   priv->view_pos_y   = fpos_y;

   navwin_update_view (navwin);

   return GTK_WIDGET (navwin);
}


void
gimv_nav_win_show (GimvNavWin *navwin, gint x_root, gint y_root)
{
   GimvNavWinPrivate *priv;

   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->x_root = x_root;
   priv->y_root = y_root;

   navwin_update_view (navwin);
   navwin_set_win_pos_size (navwin);

   gtk_widget_show (GTK_WIDGET (navwin));
   navwin_grab_pointer (navwin);
}


void
gimv_nav_win_hide (GimvNavWin *navwin)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   gdk_keyboard_ungrab (GDK_CURRENT_TIME);
   gtk_grab_remove (GTK_WIDGET (navwin));

   gtk_widget_hide (GTK_WIDGET (navwin));
}


void
gimv_nav_win_set_pixmap (GimvNavWin *navwin,
                         GdkPixmap *pixmap, GdkBitmap *mask,
                         gint image_width, gint image_height)
{
   GimvNavWinPrivate *priv;

   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));
   g_return_if_fail (pixmap);

   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   if (priv->pixmap)
      gdk_pixmap_unref (priv->pixmap);
   priv->pixmap = NULL;
   priv->mask   = NULL;

   priv->pixmap = gdk_pixmap_ref (pixmap);
   if (mask) priv->mask = gdk_bitmap_ref (mask);

   priv->image_width  = image_width;
   priv->image_height = image_height;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_set_win_pos_size (navwin);
      navwin_draw (navwin);
   }
}


void
gimv_nav_win_set_orig_image_size (GimvNavWin *navwin,
                                  gint width, gint height)
{
   GimvNavWinPrivate *priv;

   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->image_width  = width;
   priv->image_height = height;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_draw (navwin);
   }
}


void
gimv_nav_win_set_view_size (GimvNavWin *navwin,
                            gint width, gint height)
{
   GimvNavWinPrivate *priv;

   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->view_width  = width;
   priv->view_height = height;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_draw (navwin);
   }
}


void
gimv_nav_win_set_view_position (GimvNavWin *navwin,
                                gint x, gint y)
{
   GimvNavWinPrivate *priv;

   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   priv->view_pos_x = x;
   priv->view_pos_y = y;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_draw (navwin);
   }
}



/******************************************************************************
 *
 *   Private functions.
 *
 ******************************************************************************/
static void
navwin_draw_sqr (GimvNavWin *navwin,
                 gboolean undraw,
                 gint x,
                 gint y)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   if ((priv->sqr_x == x) && (priv->sqr_y == y) && undraw)
      return;

   if ((priv->sqr_x == 0) 
       && (priv->sqr_y == 0)
       && (priv->sqr_width == priv->popup_width) 
       && (priv->sqr_height == priv->popup_height))
      return;

   if (undraw) {
      gdk_draw_rectangle (priv->preview->window, 
                          priv->gc, FALSE, 
                          priv->sqr_x + 1, 
                          priv->sqr_y + 1,
                          priv->sqr_width - PEN_WIDTH,
                          priv->sqr_height - PEN_WIDTH);
   }
	
   gdk_draw_rectangle (priv->preview->window, 
                       priv->gc, FALSE, 
                       x + 1, 
                       y + 1, 
                       priv->sqr_width - PEN_WIDTH,
                       priv->sqr_height - PEN_WIDTH);

   priv->sqr_x = x;
   priv->sqr_y = y;
}


static void
get_sqr_origin_as_double (GimvNavWin *navwin,
                          gint mx,
                          gint my,
                          gdouble *x,
                          gdouble *y)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   *x = MIN (mx - BORDER_WIDTH, GIMV_NAV_WIN_SIZE);
   *y = MIN (my - BORDER_WIDTH, GIMV_NAV_WIN_SIZE);

   if (*x - priv->sqr_width / 2.0 < 0.0) 
      *x = priv->sqr_width / 2.0;
	
   if (*y - priv->sqr_height / 2.0 < 0.0)
      *y = priv->sqr_height / 2.0;

   if (*x + priv->sqr_width / 2.0 > priv->popup_width - 0)
      *x = priv->popup_width - 0 - priv->sqr_width / 2.0;

   if (*y + priv->sqr_height / 2.0 > priv->popup_height - 0)
      *y = priv->popup_height - 0 - priv->sqr_height / 2.0;

   *x = *x - priv->sqr_width / 2.0;
   *y = *y - priv->sqr_height / 2.0;
}


static void
navwin_update_view (GimvNavWin *navwin)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);
   gint popup_x, popup_y;
   gint popup_width, popup_height;
   gint w, h, x_pos, y_pos;
   gdouble factor;

   w = priv->image_width;
   h = priv->image_height;

   factor = MIN ((gdouble) (GIMV_NAV_WIN_SIZE) / w, 
                 (gdouble) (GIMV_NAV_WIN_SIZE) / h);
   priv->factor = factor;

   /* Popup window size. */
   popup_width  = MAX ((gint) floor (factor * w + 0.5), 1);
   popup_height = MAX ((gint) floor (factor * h + 0.5), 1);

   gtk_drawing_area_size (GTK_DRAWING_AREA (priv->preview),
                          popup_width,
                          popup_height);

   /* The square. */
   x_pos = priv->view_pos_x;
   y_pos = priv->view_pos_y;

   priv->sqr_width = priv->view_width * factor;
   priv->sqr_width = MAX (priv->sqr_width, BORDER_WIDTH);
   priv->sqr_width = MIN (priv->sqr_width, popup_width);

   priv->sqr_height = priv->view_height * factor;
   priv->sqr_height = MAX (priv->sqr_height, BORDER_WIDTH); 
   priv->sqr_height = MIN (priv->sqr_height, popup_height); 

   priv->sqr_x = x_pos * factor;
   if (priv->sqr_x < 0) priv->sqr_x = 0;
   priv->sqr_y = y_pos * factor;
   if (priv->sqr_y < 0) priv->sqr_y = 0;

   /* fix x (or y) if image is smaller than frame */
   if (priv->view_width  > priv->image_width) 
      priv->fix_x_pos = x_pos;
   else
      priv->fix_x_pos = 1;
   if (priv->view_height > priv->image_height)
      priv->fix_y_pos = y_pos;
   else
      priv->fix_y_pos = 1;

   /* Popup window position. */
   popup_x = MIN (priv->x_root - priv->sqr_x 
                  - BORDER_WIDTH 
                  - priv->sqr_width / 2,
                  gdk_screen_width () - popup_width - BORDER_WIDTH * 2);
   popup_y = MIN (priv->y_root - priv->sqr_y 
                  - BORDER_WIDTH
                  - priv->sqr_height / 2,
                  gdk_screen_height () - popup_height - BORDER_WIDTH * 2);

   priv->popup_x      = popup_x;
   priv->popup_y      = popup_y;
   priv->popup_width  = popup_width;
   priv->popup_height = popup_height;

   priv->view_pos_x   = x_pos;
   priv->view_pos_y   = y_pos;

   gtk_widget_draw (priv->preview, NULL);
}


static void
navwin_grab_pointer (GimvNavWin *navwin)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);
   GdkCursor *cursor;

   gtk_grab_add (GTK_WIDGET (navwin));

   cursor = gdk_cursor_new (GDK_CROSSHAIR); 

   gdk_pointer_grab (GTK_WIDGET (navwin)->window, TRUE,
                     GDK_BUTTON_RELEASE_MASK |
                     GDK_POINTER_MOTION_HINT_MASK |
                     GDK_BUTTON_MOTION_MASK |
                     GDK_EXTENSION_EVENTS_ALL,
                     priv->preview->window, 
                     cursor, 0);

   gdk_cursor_destroy (cursor); 

   /* Capture keyboard events. */
   gdk_keyboard_grab (GTK_WIDGET (navwin)->window, TRUE, GDK_CURRENT_TIME);
   gtk_widget_grab_focus (GTK_WIDGET (navwin));
}


static void
navwin_draw (GimvNavWin *navwin)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   gdk_draw_pixmap (priv->preview->window,
                    priv->preview->style->white_gc,
                    priv->pixmap,
                    0, 0, 0, 0, -1, -1);

   navwin_draw_sqr (navwin, FALSE, 
                    priv->sqr_x, 
                    priv->sqr_y);
}


static void
navwin_set_win_pos_size (GimvNavWin *navwin)
{
   GimvNavWinPrivate *priv = GIMV_NAV_WIN_GET_PRIVATE (navwin);

   gtk_window_move   (GTK_WINDOW (navwin),
                      priv->popup_x,
                      priv->popup_y);
   gtk_window_resize (GTK_WINDOW (navwin),
                      priv->popup_width  + BORDER_WIDTH * 2, 
                      priv->popup_height + BORDER_WIDTH * 2);
}
