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


#define PEN_WIDTH         3       /* Square border width. */ 
#define BORDER_WIDTH      4       /* Window border width. */


enum {
   MOVE_SIGNAL,
   LAST_SIGNAL
};


static GtkWindowClass *parent_class = NULL;
static gint gimv_nav_win_signals[LAST_SIGNAL] = {0};


/* object class */
static void     gimv_nav_win_class_init     (GimvNavWinClass *klass);
static void     gimv_nav_win_init           (GimvNavWin      *navwin);
static void     gimv_nav_win_destroy        (GtkObject       *object);

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


GtkType
gimv_nav_win_get_type (void)
{
   static GtkType gimv_nav_win_type = 0;

   if (!gimv_nav_win_type) {
      static const GtkTypeInfo gimv_nav_win_info = {
         "GimvNavWin",
         sizeof (GimvNavWin),
         sizeof (GimvNavWinClass),
         (GtkClassInitFunc) gimv_nav_win_class_init,
         (GtkObjectInitFunc) gimv_nav_win_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_nav_win_type = gtk_type_unique (GTK_TYPE_WINDOW,
                                           &gimv_nav_win_info);
   }

   return gimv_nav_win_type;
}


static void
gimv_nav_win_class_init (GimvNavWinClass *klass)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;

   object_class = (GtkObjectClass *) klass;
   widget_class = (GtkWidgetClass *) klass;
   parent_class = gtk_type_class (GTK_TYPE_WINDOW);

   object_class->destroy = gimv_nav_win_destroy;

   widget_class->realize              = gimv_nav_win_realize;
   widget_class->unrealize            = gimv_nav_win_unrealize;
   widget_class->expose_event         = gimv_nav_win_expose;
   widget_class->key_press_event      = gimv_nav_win_key_press;
   widget_class->button_release_event = gimv_nav_win_button_release;
   widget_class->motion_notify_event  = gimv_nav_win_motion_notify;

   gimv_nav_win_signals[MOVE_SIGNAL]
      = gtk_signal_new ("move",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE(object_class),
                        GTK_SIGNAL_OFFSET (GimvNavWinClass, move),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2, GTK_TYPE_INT, GTK_TYPE_INT);
}


static void
gimv_nav_win_init (GimvNavWin *navwin)
{
   navwin->fix_x_pos = navwin->fix_x_pos = 1;

   navwin->gc           = NULL;
   navwin->pixmap       = NULL;
   navwin->mask         = NULL;

   navwin->out_frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (navwin->out_frame), GTK_SHADOW_OUT);
   gtk_container_add (GTK_CONTAINER (navwin), navwin->out_frame);
   gtk_widget_show (navwin->out_frame);

   navwin->in_frame = gtk_frame_new (NULL);
   gtk_frame_set_shadow_type (GTK_FRAME (navwin->in_frame), GTK_SHADOW_IN);
   gtk_container_add (GTK_CONTAINER (navwin->out_frame), navwin->in_frame);
   gtk_widget_show (navwin->in_frame);

   navwin->preview = gtk_drawing_area_new ();
   gtk_container_add (GTK_CONTAINER (navwin->in_frame), navwin->preview);
   gtk_widget_show (navwin->preview);
}


static void
gimv_nav_win_destroy (GtkObject *object)
{
   GimvNavWin *navwin = GIMV_NAV_WIN (object);

   if (navwin->pixmap)
      gdk_pixmap_unref (navwin->pixmap);
   navwin->pixmap = NULL;

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gimv_nav_win_realize (GtkWidget *widget)
{
   GimvNavWin *navwin;

   g_return_if_fail (GIMV_IS_NAV_WIN (widget));

   navwin = GIMV_NAV_WIN (widget);

   if (GTK_WIDGET_CLASS (parent_class)->realize)
      GTK_WIDGET_CLASS (parent_class)->realize (widget);

   if (!navwin->gc) {
      navwin->gc = gdk_gc_new (widget->window);
      gdk_gc_set_function (navwin->gc, GDK_INVERT);
      gdk_gc_set_line_attributes (navwin->gc, 
                                  PEN_WIDTH, 
                                  GDK_LINE_SOLID, 
                                  GDK_CAP_BUTT, 
                                  GDK_JOIN_MITER);
   }
}


static void
gimv_nav_win_unrealize (GtkWidget *widget)
{
   GimvNavWin *navwin;

   g_return_if_fail (GIMV_IS_NAV_WIN (widget));

   navwin = GIMV_NAV_WIN (widget);

   if (navwin->gc)
      gdk_gc_destroy (navwin->gc);
   navwin->gc = NULL;

   if (GTK_WIDGET_CLASS (parent_class)->unrealize)
      GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}


static gboolean
gimv_nav_win_expose (GtkWidget *widget,
                     GdkEventExpose *event)
{
   GimvNavWin *navwin;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   navwin_draw (navwin);

   if(gtk_grab_get_current() != GTK_WIDGET (navwin))
      navwin_grab_pointer(navwin);

   if (GTK_WIDGET_CLASS (parent_class)->expose_event)
      return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_key_press (GtkWidget *widget, 
                        GdkEventKey *event)
{
   GimvNavWin *navwin;
   gboolean move = FALSE;
   gint mx, my;
   guint keyval;
   GdkModifierType modval;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   mx = navwin->view_pos_x;
   my = navwin->view_pos_y;

   keyval = event->keyval;
   modval = event->state;

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
      if (navwin->fix_x_pos < 0) mx = navwin->fix_x_pos;
      if (navwin->fix_y_pos < 0) my = navwin->fix_y_pos;
      navwin->view_pos_x = mx;
      navwin->view_pos_y = my;
      gtk_signal_emit (GTK_OBJECT (navwin),
                       gimv_nav_win_signals[MOVE_SIGNAL],
                       mx, my);
      mx *= navwin->factor;
      my *= navwin->factor;
      navwin_draw_sqr (navwin, TRUE, mx, my);
   }

   if (GTK_WIDGET_CLASS (parent_class)->key_press_event)
      return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_button_release  (GtkWidget *widget,
                              GdkEventButton *event)
{
   GimvNavWin *navwin;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   switch (event->button) {
   case 1:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
                                    "button_release_event");
      gimv_nav_win_hide (navwin);
      /* gtk_widget_destroy (GTK_WIDGET (navwin)); */
      return TRUE;
      break;
   default:
      break;
   }

   if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
      return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

   return FALSE;
}


static gboolean
gimv_nav_win_motion_notify (GtkWidget *widget,
                            GdkEventMotion *event)
{
   GimvNavWin *navwin;
   GdkModifierType mask;
   gint mx, my;
   gdouble x, y;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   gdk_window_get_pointer (widget->window, &mx, &my, &mask);
   get_sqr_origin_as_double (navwin, mx, my, &x, &y);

   mx = (gint) x;
   my = (gint) y;
   navwin_draw_sqr (navwin, TRUE, mx, my);

   mx = (gint) (x / navwin->factor);
   my = (gint) (y / navwin->factor);
   if (navwin->fix_x_pos < 0) mx = navwin->fix_x_pos;
   if (navwin->fix_y_pos < 0) my = navwin->fix_y_pos;

   gtk_signal_emit (GTK_OBJECT (navwin),
                    gimv_nav_win_signals[MOVE_SIGNAL],
                    mx, my);

   if (GTK_WIDGET_CLASS (parent_class)->motion_notify_event)
      return GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

   return FALSE;
}


GtkWidget *
gimv_nav_win_new (GdkPixmap *pixmap, GdkBitmap *mask,
                  gint image_width, gint image_height,
                  gint view_width, gint view_height,
                  gint fpos_x, gint fpos_y)
{
   GimvNavWin *navwin;

   g_return_val_if_fail (pixmap, NULL);

   navwin = GIMV_NAV_WIN (gtk_object_new (GIMV_TYPE_NAV_WIN,
                                          "type", GTK_WINDOW_POPUP,
                                          NULL));

   navwin->pixmap = gdk_pixmap_ref (pixmap);
   if (mask) navwin->mask = gdk_bitmap_ref (mask);

   navwin->x_root = 0;
   navwin->y_root = 0;

   navwin->image_width  = image_width;
   navwin->image_height = image_height;

   navwin->view_width   = view_width;
   navwin->view_height  = view_height;
   navwin->view_pos_x   = fpos_x;
   navwin->view_pos_y   = fpos_y;

   navwin_update_view (navwin);

   return GTK_WIDGET (navwin);
}


void
gimv_nav_win_show (GimvNavWin *navwin, gint x_root, gint y_root)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   navwin->x_root = x_root;
   navwin->y_root = y_root;

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
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));
   g_return_if_fail (pixmap);

   if (navwin->pixmap)
      gdk_pixmap_unref (navwin->pixmap);
   navwin->pixmap = NULL;
   navwin->mask   = NULL;

   navwin->pixmap = gdk_pixmap_ref (pixmap);
   if (mask) navwin->mask = gdk_bitmap_ref (mask);

   navwin->image_width  = image_width;
   navwin->image_height = image_height;

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
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   navwin->image_width  = width;
   navwin->image_height = height;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_draw (navwin);
   }
}


void
gimv_nav_win_set_view_size (GimvNavWin *navwin,
                            gint width, gint height)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   navwin->view_width  = width;
   navwin->view_height = height;

   if (GTK_WIDGET_MAPPED (navwin)) {
      navwin_update_view (navwin);
      navwin_draw (navwin);
   }
}


void
gimv_nav_win_set_view_position (GimvNavWin *navwin,
                                gint x, gint y)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   navwin->view_pos_x = x;
   navwin->view_pos_y = y;

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
   if ((navwin->sqr_x == x) && (navwin->sqr_y == y) && undraw)
      return;

   if ((navwin->sqr_x == 0) 
       && (navwin->sqr_y == 0)
       && (navwin->sqr_width == navwin->popup_width) 
       && (navwin->sqr_height == navwin->popup_height))
      return;

   if (undraw) {
      gdk_draw_rectangle (navwin->preview->window, 
                          navwin->gc, FALSE, 
                          navwin->sqr_x + 1, 
                          navwin->sqr_y + 1,
                          navwin->sqr_width - PEN_WIDTH,
                          navwin->sqr_height - PEN_WIDTH);
   }
	
   gdk_draw_rectangle (navwin->preview->window, 
                       navwin->gc, FALSE, 
                       x + 1, 
                       y + 1, 
                       navwin->sqr_width - PEN_WIDTH,
                       navwin->sqr_height - PEN_WIDTH);

   navwin->sqr_x = x;
   navwin->sqr_y = y;
}


static void
get_sqr_origin_as_double (GimvNavWin *navwin,
                          gint mx,
                          gint my,
                          gdouble *x,
                          gdouble *y)
{
   *x = MIN (mx - BORDER_WIDTH, GIMV_NAV_WIN_SIZE);
   *y = MIN (my - BORDER_WIDTH, GIMV_NAV_WIN_SIZE);

   if (*x - navwin->sqr_width / 2.0 < 0.0) 
      *x = navwin->sqr_width / 2.0;
	
   if (*y - navwin->sqr_height / 2.0 < 0.0)
      *y = navwin->sqr_height / 2.0;

   if (*x + navwin->sqr_width / 2.0 > navwin->popup_width - 0)
      *x = navwin->popup_width - 0 - navwin->sqr_width / 2.0;

   if (*y + navwin->sqr_height / 2.0 > navwin->popup_height - 0)
      *y = navwin->popup_height - 0 - navwin->sqr_height / 2.0;

   *x = *x - navwin->sqr_width / 2.0;
   *y = *y - navwin->sqr_height / 2.0;
}


static void
navwin_update_view (GimvNavWin *navwin)
{
   gint popup_x, popup_y;
   gint popup_width, popup_height;
   gint w, h, x_pos, y_pos;
   gdouble factor;

   w = navwin->image_width;
   h = navwin->image_height;

   factor = MIN ((gdouble) (GIMV_NAV_WIN_SIZE) / w, 
                 (gdouble) (GIMV_NAV_WIN_SIZE) / h);
   navwin->factor = factor;

   /* Popup window size. */
   popup_width  = MAX ((gint) floor (factor * w + 0.5), 1);
   popup_height = MAX ((gint) floor (factor * h + 0.5), 1);

   gtk_drawing_area_size (GTK_DRAWING_AREA (navwin->preview),
                          popup_width,
                          popup_height);

   /* The square. */
   x_pos = navwin->view_pos_x;
   y_pos = navwin->view_pos_y;

   navwin->sqr_width = navwin->view_width * factor;
   navwin->sqr_width = MAX (navwin->sqr_width, BORDER_WIDTH);
   navwin->sqr_width = MIN (navwin->sqr_width, popup_width);

   navwin->sqr_height = navwin->view_height * factor;
   navwin->sqr_height = MAX (navwin->sqr_height, BORDER_WIDTH); 
   navwin->sqr_height = MIN (navwin->sqr_height, popup_height); 

   navwin->sqr_x = x_pos * factor;
   if (navwin->sqr_x < 0) navwin->sqr_x = 0;
   navwin->sqr_y = y_pos * factor;
   if (navwin->sqr_y < 0) navwin->sqr_y = 0;

   /* fix x (or y) if image is smaller than frame */
   if (navwin->view_width  > navwin->image_width) 
      navwin->fix_x_pos = x_pos;
   else
      navwin->fix_x_pos = 1;
   if (navwin->view_height > navwin->image_height)
      navwin->fix_y_pos = y_pos;
   else
      navwin->fix_y_pos = 1;

   /* Popup window position. */
   popup_x = MIN (navwin->x_root - navwin->sqr_x 
                  - BORDER_WIDTH 
                  - navwin->sqr_width / 2,
                  gdk_screen_width () - popup_width - BORDER_WIDTH * 2);
   popup_y = MIN (navwin->y_root - navwin->sqr_y 
                  - BORDER_WIDTH
                  - navwin->sqr_height / 2,
                  gdk_screen_height () - popup_height - BORDER_WIDTH * 2);

   navwin->popup_x      = popup_x;
   navwin->popup_y      = popup_y;
   navwin->popup_width  = popup_width;
   navwin->popup_height = popup_height;

   navwin->view_pos_x   = x_pos;
   navwin->view_pos_y   = y_pos;

   gtk_widget_draw (navwin->preview, NULL);
}


static void
navwin_grab_pointer (GimvNavWin *navwin)
{
   GdkCursor *cursor;

   gtk_grab_add (GTK_WIDGET (navwin));

   cursor = gdk_cursor_new (GDK_CROSSHAIR); 

   gdk_pointer_grab (GTK_WIDGET (navwin)->window, TRUE,
                     GDK_BUTTON_RELEASE_MASK |
                     GDK_POINTER_MOTION_HINT_MASK |
                     GDK_BUTTON_MOTION_MASK |
                     GDK_EXTENSION_EVENTS_ALL,
                     navwin->preview->window, 
                     cursor, 0);

   gdk_cursor_destroy (cursor); 

   /* Capture keyboard events. */
   gdk_keyboard_grab (GTK_WIDGET (navwin)->window, TRUE, GDK_CURRENT_TIME);
   gtk_widget_grab_focus (GTK_WIDGET (navwin));
}


static void
navwin_draw (GimvNavWin *navwin)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   gdk_draw_pixmap (navwin->preview->window,
                    navwin->preview->style->white_gc,
                    navwin->pixmap,
                    0, 0, 0, 0, -1, -1);

   navwin_draw_sqr (navwin, FALSE, 
                    navwin->sqr_x, 
                    navwin->sqr_y);
}


static void
navwin_set_win_pos_size (GimvNavWin *navwin)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));

   gtk_window_move   (GTK_WINDOW (navwin),
                      navwin->popup_x,
                      navwin->popup_y);
   gtk_window_resize (GTK_WINDOW (navwin),
                      navwin->popup_width  + BORDER_WIDTH * 2, 
                      navwin->popup_height + BORDER_WIDTH * 2);
}
