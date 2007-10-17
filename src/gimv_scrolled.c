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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gimv_scrolled.h"

/* for auto-scroll and auto-expand at drag */
#define AUTO_SCROLL_TIMEOUT    100
#define AUTO_SCROLL_EDGE_WIDTH 20
#define AUTO_SCROLL_SCALE_RATE 32 /* times / pixel */
typedef gboolean (*desirable_fn) (GimvScrolled *scrolled, gint x, gint y);
typedef gboolean (*scroll_fn)    (gpointer data);

#define bw(widget)                  GTK_CONTAINER(widget)->border_width

#define WIDGET_DRAW(widget, area) gdk_window_invalidate_rect (widget->window, area, TRUE)

#define adjustment_check_value(value, adj) \
{ \
   if (value < adj->lower) \
      value = adj->lower; \
   if (value > adj->upper - adj->page_size) \
      value = adj->upper - adj->page_size; \
}

enum {
   ADJUST_ADJUSTMENTS,
   LAST_SIGNAL
};

static void     gimv_scrolled_set_scroll_adjustments (GtkWidget      *widget, 
                                                 GtkAdjustment  *hadjustment, 
                                                 GtkAdjustment  *vadjustment);
static gint     gimv_scrolled_button_press           (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gint     gimv_scrolled_button_release         (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gint     gimv_scrolled_motion_notify          (GtkWidget      *widget,
                                                 GdkEventMotion *event);
static gint     gimv_scrolled_drag_motion            (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 gint            x,
                                                 gint            y,
                                                 guint           time);
static void     gimv_scrolled_drag_leave             (GtkWidget      *dirtree,
                                                 GdkDragContext *context,
                                                 guint           time);

static void     hadjustment_value_changed      (GtkAdjustment   *hadjustment,
                                                gpointer         data);
static void     vadjustment_value_changed      (GtkAdjustment   *vadjustment,
                                                gpointer         data);
static gboolean horizontal_timeout             (gpointer         data);
static gboolean vertical_timeout               (gpointer         data);


static guint           gimv_scrolled_signals [LAST_SIGNAL] = {0};


G_DEFINE_TYPE (GimvScrolled, gimv_scrolled, GTK_TYPE_CONTAINER)


static void
gimv_scrolled_class_init (GimvScrolledClass *klass)
{
   GObjectClass *gobject_class;
   GtkWidgetClass *widget_class;
   GtkContainerClass *container_class;

   gobject_class   = (GObjectClass*) klass;
   widget_class    = (GtkWidgetClass*) klass;
   container_class = (GtkContainerClass*) klass;

   widget_class->set_scroll_adjustments_signal =
      g_signal_new ("set_scroll_adjustments",
                    G_TYPE_FROM_CLASS(gobject_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET(GimvScrolledClass, set_scroll_adjustments),
                    NULL, NULL,
                    gtk_marshal_NONE__POINTER_POINTER,
                    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
    
   gimv_scrolled_signals[ADJUST_ADJUSTMENTS] = 
      g_signal_new ("adjust_adjustments",
                    G_TYPE_FROM_CLASS(gobject_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET(GimvScrolledClass, adjust_adjustments),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

   widget_class->button_press_event     = gimv_scrolled_button_press;
   widget_class->button_release_event   = gimv_scrolled_button_release;
   widget_class->motion_notify_event    = gimv_scrolled_motion_notify;
   widget_class->drag_motion            = gimv_scrolled_drag_motion;
   widget_class->drag_leave             = gimv_scrolled_drag_leave;

   klass->set_scroll_adjustments = gimv_scrolled_set_scroll_adjustments;
   klass->adjust_adjustments     = NULL;
}

static void
gimv_scrolled_init (GimvScrolled *scrolled)
{
   scrolled->x_offset         = 0;
   scrolled->y_offset         = 0;
   scrolled->h_adjustment     = NULL;
   scrolled->v_adjustment     = NULL;
   scrolled->freeze_count     = 0;
   scrolled->copy_gc          = NULL;

   /* for auto scroll */
   scrolled->autoscroll_flags = 0;
   scrolled->scroll_edge_x    = AUTO_SCROLL_EDGE_WIDTH;
   scrolled->scroll_edge_y    = AUTO_SCROLL_EDGE_WIDTH;
   scrolled->x_step           = -1;
   scrolled->y_step           = -1;
   scrolled->x_interval       = AUTO_SCROLL_TIMEOUT;
   scrolled->y_interval       = AUTO_SCROLL_TIMEOUT;
   scrolled->pressed          = FALSE;
   scrolled->drag_start_vx    = -1;
   scrolled->drag_start_vy    = -1;
   scrolled->drag_motion_x    = -1;
   scrolled->drag_motion_y    = -1;
   scrolled->step_scale       = 0.0;
   scrolled->hscroll_timer_id = -1;
   scrolled->vscroll_timer_id = -1;
}

void
gimv_scrolled_realize (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled && GTK_WIDGET(scrolled)->window);

   scrolled->copy_gc = gdk_gc_new (GTK_WIDGET(scrolled)->window);
   gdk_gc_set_exposures (scrolled->copy_gc, TRUE);
}

void
gimv_scrolled_unrealize (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);

   gdk_gc_destroy (scrolled->copy_gc);
}

static void
hadjustment_value_changed (GtkAdjustment *hadjustment,
                           gpointer data)
{
   GimvScrolled *scrolled;
   GtkWidget *widget;
   GdkRectangle area;
   int value, diff;

   widget = GTK_WIDGET(data);
   scrolled = GIMV_SCROLLED(data);

   if (!GTK_WIDGET_DRAWABLE(widget) || scrolled->x_offset == hadjustment->value)
      return;

   value = hadjustment->value;

   if (value < 0)
      value = 0;

   if (scrolled->freeze_count) {
      scrolled->x_offset = value;
      return;
   }

   if (value > scrolled->x_offset) {
      diff = value - scrolled->x_offset;

      if (diff >= widget->allocation.width) {
         scrolled->x_offset = value;
         WIDGET_DRAW(widget, NULL);
         return;
      }

      if (diff > 0 && !scrolled->freeze_count) 
         gdk_window_copy_area (widget->window, 
                               scrolled->copy_gc,
                               0, 0, 
                               widget->window,
                               diff, 0,
                               widget->allocation.width  - 2 * bw (widget) - diff, 
                               widget->allocation.height - 2 * bw (widget));

      area.x = widget->allocation.width - 2 * bw (widget) - diff;
      area.y = 0;
      area.width = diff;
      area.height = widget->allocation.height - 2 * bw (widget);

   } else {
      diff = scrolled->x_offset - value;

      if (diff >= widget->allocation.width) {
         scrolled->x_offset = value;
         WIDGET_DRAW(widget, NULL);
         return;
      }

      if (diff > 0) 
         gdk_window_copy_area (widget->window, 
                               scrolled->copy_gc,
                               diff, 0, 
                               widget->window,
                               0, 0,
                               widget->allocation.width  - 2 * bw (widget) - diff, 
                               widget->allocation.height - 2 * bw (widget));

      area.x = 0;
      area.y = 0;
      area.width = diff;
      area.height = widget->allocation.height - 2 * bw (widget);
   }


   if (diff >  0) {
      scrolled->x_offset = value;
      /* WIDGET_DRAW(widget, &area); */
      WIDGET_DRAW(widget, NULL);
   }
}


static void
vadjustment_value_changed (GtkAdjustment *vadjustment,
                           gpointer data)
{
   GimvScrolled *scrolled;
   GtkWidget *widget;
   GdkRectangle area;
   int value, diff;

   widget = GTK_WIDGET(data);
   scrolled = GIMV_SCROLLED(data);

   if (!GTK_WIDGET_DRAWABLE(widget) || scrolled->y_offset == vadjustment->value)
      return;

   value = vadjustment->value;

   if (value < 0)
      value = 0;

   if (scrolled->freeze_count) {
      scrolled->y_offset = value;
      return;
   }

   if (value > scrolled->y_offset) {
      diff = value - scrolled->y_offset;

      if (diff >= widget->allocation.height) {
         scrolled->y_offset = value;
         WIDGET_DRAW(widget, NULL);
         return;
      }

      if (diff > 0) 
         gdk_window_copy_area (widget->window, 
                               scrolled->copy_gc,
                               0, 0, 
                               widget->window,
                               0, diff,
                               widget->allocation.width  - 2 * bw (widget), 
                               widget->allocation.height - 2 * bw (widget) - diff);

      area.x = 0;
      area.y = widget->allocation.height - 2 * bw (widget) - diff;
      area.width = widget->allocation.width - 2 * bw (widget);
      area.height = diff;

   } else {
      diff = scrolled->y_offset - value;

      if (diff >= widget->allocation.height) {
         scrolled->y_offset = value;
         WIDGET_DRAW(widget, NULL);
         return;
      }

      if (diff > 0) 
         gdk_window_copy_area (widget->window, 
                               scrolled->copy_gc,
                               0, diff, 
                               widget->window,
                               0, 0,
                               widget->allocation.width - 2 * bw (widget), 
                               widget->allocation.height - 2 * bw (widget) - diff);

      area.x = 0;
      area.y = 0;
      area.width = widget->allocation.width - 2 * bw (widget);
      area.height = diff;
   }


   if (diff >  0) {
      scrolled->y_offset = value;
      /* WIDGET_DRAW(widget, &area); */
      WIDGET_DRAW(widget, NULL);
   }
}

static void     
gimv_scrolled_set_scroll_adjustments (GtkWidget *widget, 
                                 GtkAdjustment *hadjustment, 
                                 GtkAdjustment *vadjustment)
{
   GimvScrolled *scrolled;
   scrolled = GIMV_SCROLLED(widget);

   if (scrolled->h_adjustment != hadjustment) {
      if (scrolled->h_adjustment) {
         g_signal_handlers_disconnect_matched (
            G_OBJECT (scrolled->h_adjustment),
            G_SIGNAL_MATCH_DATA,
            0, 0, NULL, NULL, scrolled);
         g_object_unref (G_OBJECT(scrolled->h_adjustment));
      }
	    
      scrolled->h_adjustment = hadjustment;

      if (hadjustment) {
         g_object_ref (G_OBJECT(hadjustment));
         g_signal_connect (G_OBJECT(hadjustment),
                           "value_changed", 
                           G_CALLBACK (hadjustment_value_changed),
                           scrolled);
      }
   }


   if (scrolled->v_adjustment != vadjustment) {
      if (scrolled->v_adjustment) {
         g_signal_handlers_disconnect_matched (
            G_OBJECT (scrolled->v_adjustment),
            G_SIGNAL_MATCH_DATA,
            0, 0, NULL, NULL, scrolled);
         g_object_unref (G_OBJECT(scrolled->v_adjustment));
      }
	    
      scrolled->v_adjustment = vadjustment;

      if (vadjustment) {
         g_object_ref (G_OBJECT(vadjustment));
         g_signal_connect (G_OBJECT(vadjustment),
                           "value_changed", 
                           G_CALLBACK (vadjustment_value_changed),
                           scrolled);
      }
   }

   g_signal_emit (G_OBJECT(scrolled),
                  gimv_scrolled_signals[ADJUST_ADJUSTMENTS], 0);
}

void
gimv_scrolled_freeze (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (scrolled->freeze_count != (guint) -1);

   scrolled->freeze_count ++;
}

void
gimv_scrolled_thawn (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (scrolled->freeze_count);

   scrolled->freeze_count --;
   if (!scrolled->freeze_count) {
      g_signal_emit (G_OBJECT(scrolled),
                     gimv_scrolled_signals [ADJUST_ADJUSTMENTS], 0);
      gtk_widget_draw (GTK_WIDGET(scrolled), NULL);
   }
}


void
gimv_scrolled_page_up (GimvScrolled *scrolled)
{
   GtkAdjustment *vadj;

   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED (scrolled));

   vadj = scrolled->v_adjustment;

   vadj->value -= vadj->page_size;
   adjustment_check_value (vadj->value, vadj);

   g_signal_emit_by_name (G_OBJECT(vadj), "value_changed"); 
}


void
gimv_scrolled_page_down (GimvScrolled *scrolled)
{
   GtkAdjustment *vadj;

   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED (scrolled));

   vadj = scrolled->v_adjustment;

   vadj->value += vadj->page_size;
   adjustment_check_value (vadj->value, vadj);

   g_signal_emit_by_name (G_OBJECT(vadj), "value_changed"); 
}


void
gimv_scrolled_page_left (GimvScrolled *scrolled)
{
   GtkAdjustment *hadj;

   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED (scrolled));

   hadj = scrolled->h_adjustment;

   hadj->value -= hadj->page_size;
   adjustment_check_value (hadj->value, hadj);

   g_signal_emit_by_name (G_OBJECT(hadj), "value_changed"); 
}


void
gimv_scrolled_page_right (GimvScrolled *scrolled)
{
   GtkAdjustment *hadj;

   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED (scrolled));

   hadj = scrolled->h_adjustment;
   hadj->value = hadj->value;

   hadj->value += hadj->page_size;
   adjustment_check_value (hadj->value, hadj);

   g_signal_emit_by_name (G_OBJECT(hadj), "value_changed"); 
}


/******************************************************************************
 *
 *   for auto scroll related functions
 *
 ******************************************************************************/
enum
{
   HORIZONTAL,
   VERTICAL
};

static void
cancel_auto_scroll (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED (scrolled));

   /* horizontal */
   if (scrolled->hscroll_timer_id != -1){
      gtk_timeout_remove (scrolled->hscroll_timer_id);
      scrolled->hscroll_timer_id = -1;
   }

   /* vertival */
   if (scrolled->vscroll_timer_id != -1){
      gtk_timeout_remove (scrolled->vscroll_timer_id);
      scrolled->vscroll_timer_id = -1;
   }
}


static gboolean
scrolling_is_desirable (GimvScrolled *scrolled, gint direction,
                        gint x, gint y, gfloat *scale)
{
   GtkAdjustment *adj;
   gint pos, edge_width, flags;
   gfloat upper;

   flags = scrolled->autoscroll_flags;

   if (direction == HORIZONTAL) {
      if (!(flags & GIMV_SCROLLED_AUTO_SCROLL_HORIZONTAL)
          && !(flags & GIMV_SCROLLED_AUTO_SCROLL_BOTH))
      {
         return FALSE;
      }

      adj = scrolled->h_adjustment;
      pos = x;
      edge_width = scrolled->scroll_edge_x;
   } else if (direction == VERTICAL) {
      if (!(flags & GIMV_SCROLLED_AUTO_SCROLL_VERTICAL)
          && !(flags & GIMV_SCROLLED_AUTO_SCROLL_BOTH))
      {
         return FALSE;
      }

      adj = scrolled->v_adjustment;
      pos = y;
      edge_width = scrolled->scroll_edge_y;
   } else {
      return FALSE;
   }

   upper = adj->upper - adj->page_size;

   if ((pos < edge_width) && (adj->value > adj->lower)) {
      if (scale)
         *scale = 1 + (0 - pos) / AUTO_SCROLL_SCALE_RATE;
      return TRUE;
   } else if ((pos > (adj->page_size - edge_width)) && (adj->value < upper)) {
      if (scale)
         *scale = 1 + (pos - adj->page_size) / AUTO_SCROLL_SCALE_RATE;
      return TRUE;
   }

   return 0;
}


static gboolean
vertical_timeout (gpointer data)
{
   GimvScrolled *scrolled = data;
   GtkAdjustment *vadj;
   gint step;

   vadj = scrolled->v_adjustment;

   if (scrolled->y_step < 0)
      step = vadj->step_increment;
   else
      step = scrolled->y_step;

   step *= scrolled->step_scale;

   if (scrolled->drag_motion_y < scrolled->scroll_edge_y)
      vadj->value = vadj->value - step;
   else
      vadj->value = vadj->value + step;

   adjustment_check_value (vadj->value, vadj);
   g_signal_emit_by_name (G_OBJECT(vadj), "value_changed"); 

   return TRUE;
}


static gboolean
horizontal_timeout (gpointer data)
{
   GimvScrolled *scrolled = data;
   GtkAdjustment *hadj;
   gint step;

   hadj = scrolled->h_adjustment;

   if (scrolled->x_step < 0)
      step = hadj->step_increment;
   else
      step = scrolled->x_step;

   step *= scrolled->step_scale;

   if (scrolled->drag_motion_x < scrolled->scroll_edge_x)
      hadj->value = hadj->value - step;
   else
      hadj->value = hadj->value + step;

   adjustment_check_value (hadj->value, hadj);
   g_signal_emit_by_name (G_OBJECT(hadj), "value_changed"); 

   return TRUE;
}


static void
setup_drag_scroll (GimvScrolled *scrolled,
                   gint x, gint y)
{
   GtkAdjustment *hadj, *vadj;
   gboolean desirable;

   hadj = scrolled->h_adjustment;
   vadj = scrolled->v_adjustment;

   cancel_auto_scroll (scrolled);

   scrolled->drag_motion_x = x;
   scrolled->drag_motion_y = y;

   /* horizonal */
   desirable = scrolling_is_desirable (scrolled, HORIZONTAL,
                                       x, y, &scrolled->step_scale);

   if (desirable)
      scrolled->hscroll_timer_id
         = gtk_timeout_add (scrolled->x_interval,
                            horizontal_timeout, scrolled);

   /* vertical */
   desirable = scrolling_is_desirable (scrolled, VERTICAL,
                                       x, y, &scrolled->step_scale);

   if (desirable)
      scrolled->vscroll_timer_id
         = gtk_timeout_add (scrolled->y_interval,
                            vertical_timeout, scrolled);
}


static gint
gimv_scrolled_button_press (GtkWidget *widget, GdkEventButton *event)
{
   GimvScrolled *scrolled;

   g_return_val_if_fail (widget && event, FALSE);
   g_return_val_if_fail (GIMV_IS_SCROLLED(widget), FALSE);

   scrolled = GIMV_SCROLLED (widget);

   scrolled->pressed = TRUE;
   scrolled->drag_start_vx = GIMV_SCROLLED_VX (scrolled, (gint) event->x);
   scrolled->drag_start_vy = GIMV_SCROLLED_VY (scrolled, (gint) event->y);

   return FALSE;
}


static gint
gimv_scrolled_button_release (GtkWidget *widget, GdkEventButton *event)
{
   g_return_val_if_fail (widget && event, FALSE);
   g_return_val_if_fail (GIMV_IS_SCROLLED(widget), FALSE);

   gimv_scrolled_stop_auto_scroll (GIMV_SCROLLED (widget));

   return FALSE;
}


static gint
gimv_scrolled_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
   GimvScrolled *scrolled;
   gint flags;
   gboolean pressed;

   g_return_val_if_fail (widget && event, FALSE);
   g_return_val_if_fail (GIMV_IS_SCROLLED(widget), FALSE);

   scrolled = GIMV_SCROLLED (widget);

   flags = scrolled->autoscroll_flags;
   pressed = scrolled->pressed;

   scrolled->drag_motion_x = event->x;
   scrolled->drag_motion_y = event->y;

   if ((pressed && (flags & GIMV_SCROLLED_AUTO_SCROLL_MOTION))
       || (flags & GIMV_SCROLLED_AUTO_SCROLL_MOTION_ALL))
   {
      setup_drag_scroll (scrolled, (gint) event->x, (gint) event->y);
   }

   return FALSE;
}


static gint
gimv_scrolled_drag_motion (GtkWidget *widget,
                           GdkDragContext *context,
                           gint x,
                           gint y,
                           guint time)
{
   GimvScrolled *scrolled;
   gint flags;

   g_return_val_if_fail (widget && context, FALSE);
   g_return_val_if_fail (GIMV_IS_SCROLLED(widget), FALSE);

   scrolled = GIMV_SCROLLED (widget);

   flags = scrolled->autoscroll_flags;

   if (flags & GIMV_SCROLLED_AUTO_SCROLL_DND) {
      setup_drag_scroll (GIMV_SCROLLED (widget), x, y);
   }

   return FALSE;
}


static void
gimv_scrolled_drag_leave (GtkWidget *widget,
                          GdkDragContext *context,
                          guint time)
{
   g_return_if_fail (widget && context);
   g_return_if_fail (GIMV_IS_SCROLLED(widget));

   cancel_auto_scroll (GIMV_SCROLLED (widget));
}


void
gimv_scrolled_set_auto_scroll (GimvScrolled *scrolled,
                               GimvScrolledAutoScrollFlags flags)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   scrolled->autoscroll_flags |= flags;
}


void
gimv_scrolled_unset_auto_scroll (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   scrolled->autoscroll_flags = 0;
}


void
gimv_scrolled_set_auto_scroll_edge_width (GimvScrolled *scrolled,
                                          gint x_edge,
                                          gint y_edge)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   if (x_edge < 0)
      scrolled->scroll_edge_x = AUTO_SCROLL_EDGE_WIDTH;
   else
      scrolled->scroll_edge_x = x_edge;

   if (y_edge < 0)
      scrolled->scroll_edge_y = AUTO_SCROLL_EDGE_WIDTH;
   else
      scrolled->scroll_edge_y = y_edge;
}


void
gimv_scrolled_set_h_auto_scroll_resolution (GimvScrolled *scrolled,
                                            gint step,
                                            gint interval)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   scrolled->x_step = step;
   if (interval <= 0)
      scrolled->x_interval = AUTO_SCROLL_TIMEOUT;
   else
      scrolled->x_interval = interval;
}


void
gimv_scrolled_set_v_auto_scroll_resolution (GimvScrolled *scrolled,
                                            gint step,
                                            gint interval)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   scrolled->y_step = step;
   if (interval <= 0)
      scrolled->y_interval = AUTO_SCROLL_TIMEOUT;
   else
      scrolled->y_interval = interval;
}


gboolean
gimv_scrolled_is_dragging (GimvScrolled *scrolled)
{
   g_return_val_if_fail (scrolled, FALSE);
   g_return_val_if_fail (scrolled, FALSE);

   if (scrolled->pressed
       && scrolled->drag_motion_x >= 0
       && scrolled->drag_motion_y >= 0)
   {
      return TRUE;
   }

   return FALSE;
}


void
gimv_scrolled_stop_auto_scroll (GimvScrolled *scrolled)
{
   g_return_if_fail (scrolled);
   g_return_if_fail (GIMV_IS_SCROLLED(scrolled));

   cancel_auto_scroll (scrolled);

   scrolled->pressed = FALSE;
   scrolled->drag_start_vx = -1;
   scrolled->drag_start_vy = -1;
   scrolled->drag_motion_x = -1;
   scrolled->drag_motion_y = -1;
}
