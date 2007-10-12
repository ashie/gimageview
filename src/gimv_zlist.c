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

#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "gtk2-compat.h"
#include "gimv_zlist.h"

#define bw(widget) ((gint) GTK_CONTAINER(widget)->border_width)

#define CELL_COL_FROM_X(list, x) \
   (GIMV_SCROLLED_VX (list, (x) - (list)->x_pad - bw (list)) / (list)->cell_width)
#define CELL_ROW_FROM_Y(list, y) \
   (GIMV_SCROLLED_VY (list, (y) - (list)->y_pad - bw (list)) / (list)->cell_height)

#define CELL_X_FROM_COL(list, col) \
   (GIMV_SCROLLED_X (list, (col) * (list)->cell_width  + (list)->x_pad)) 
#define CELL_Y_FROM_ROW(list, row) \
   (GIMV_SCROLLED_Y (list, (row) * (list)->cell_height + (list)->y_pad)) 

#define LIST_WIDTH(list)  ((list)->columns * (list)->cell_width /* + (list)->cell_x_pad */)
#define LIST_HEIGHT(list) ((list)->rows * (list)->cell_height /* + (list)->cell_y_pad */)

#define HIGHLIGHT_SIZE 2

#ifdef USE_GTK2
#  ifdef GTK_DISABLE_DEPRECATED
#     include "gimv_marshal.h"
#  endif
#  define WIDGET_DRAW(widget) gtk_widget_queue_draw (widget)
#  define WIDGET_DRAW_AREA(widget, area) \
      gtk_widget_queue_draw_area (widget, \
                                  (area)->x, (area)->y, \
                                  (area)->width, (area)->height)
#else /* USE_GTK2 */
#  define WIDGET_DRAW(widget) gtk_widget_draw (widget, NULL)
#  define WIDGET_DRAW_AREA(widget, area) gtk_widget_draw (widget, area)
#endif /* USE_GTK2 */

static void gimv_zlist_class_init              (GimvZListClass *klass);
static void gimv_zlist_init                    (GimvZList *list);

#ifdef USE_GTK2
static void  gimv_zlist_finalize               (GObject *object);
#else
static void  gimv_zlist_finalize               (GtkObject        *object);
#endif
static void  gimv_zlist_map                    (GtkWidget        *widget);
static void  gimv_zlist_unmap                  (GtkWidget        *widget);
static void  gimv_zlist_realize                (GtkWidget        *widget);
static void  gimv_zlist_unrealize              (GtkWidget        *widget);
static void  gimv_zlist_size_request           (GtkWidget        *widget,
                                                GtkRequisition   *requisition);
static void  gimv_zlist_size_allocate          (GtkWidget        *widget,
                                                GtkAllocation    *allocation);
static gint  gimv_zlist_expose                 (GtkWidget        *widget,
                                                GdkEventExpose   *event);
static void  gimv_zlist_update                 (GimvZList        *list);
static void  gimv_zlist_draw_horizontal_list   (GtkWidget        *widget,
                                                GdkRectangle     *area);
static void  gimv_zlist_draw_vertical_list     (GtkWidget        *widget,
                                                GdkRectangle     *area);
static void  gimv_zlist_draw_selection_region  (GimvZList        *list,
                                                GdkRectangle     *area);
static void  gimv_zlist_draw                   (GtkWidget        *widget,
                                                GdkRectangle     *area);
static void gimv_zlist_redraw_selection_region (GtkWidget        *list,
                                                gint              prev_x,
                                                gint              prev_y,
                                                gint              next_x,
                                                gint              next_y);

static gint  gimv_zlist_button_press           (GtkWidget        *widget,
                                                GdkEventButton   *event);
static gint  gimv_zlist_button_release         (GtkWidget        *widget,
                                                GdkEventButton   *event);
static gint  gimv_zlist_motion_notify          (GtkWidget        *widget,
                                                GdkEventMotion   *event);
static gint  gimv_zlist_key_press              (GtkWidget        *widget,
                                                GdkEventKey      *event);
static gint  gimv_zlist_focus_in               (GtkWidget        *widget,
                                                GdkEventFocus    *event);
static gint  gimv_zlist_focus_out              (GtkWidget        *widget,
                                                GdkEventFocus    *event);
static gint  gimv_zlist_drag_motion            (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                gint              x,
                                                gint              y,
                                                guint             time);
static gint gimv_zlist_drag_drop               (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                gint              x,
                                                gint              y,
                                                guint             time);
static void gimv_zlist_drag_leave              (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                guint             time);
static void gimv_zlist_highlight               (GtkWidget        *widget);
static void gimv_zlist_unhighlight             (GtkWidget        *widget);
static gint gimv_zlist_focus                   (GtkContainer     *container,
                                                GtkDirectionType  dir);
static void gimv_zlist_cell_draw_focus         (GimvZList        *list,
                                                gint              index);
static void gimv_zlist_cell_draw_default       (GimvZList        *list,
                                                gint              index);


static void gimv_zlist_forall                  (GtkContainer     *container,
                                                gboolean          include_internals,
                                                GtkCallback       callback,
                                                gpointer          callback_data);
static void gimv_zlist_adjust_adjustments      (GimvScrolled     *scrolled);
static void gimv_zlist_cell_pos                (GimvZList        *list,
                                                gint              index,
                                                gint             *row,
                                                gint             *col);
static void gimv_zlist_cell_area               (GimvZList        *list,
                                                gint              index,
                                                GdkRectangle     *cell_area);

enum {
   CLEAR,
   CELL_DRAW,
   CELL_SIZE_REQUEST,
   CELL_DRAW_FOCUS,
   CELL_DRAW_DEFAULT,
   CELL_SELECT,
   CELL_UNSELECT,
   LAST_SIGNAL
};

static GtkWidgetClass     *parent_class                = NULL;

static guint               gimv_zlist_signals [LAST_SIGNAL] = { 0 };


GtkType 
gimv_zlist_get_type (void)
{
   static GtkType type = 0;

#ifdef USE_GTK2
   if (!type) {
      static const GTypeInfo gimv_zlist_type_info = {
         sizeof (GimvZListClass),
         NULL,               /* base_init */
         NULL,               /* base_finalize */
         (GClassInitFunc)    gimv_zlist_class_init,
         NULL,               /* class_finalize */
         NULL,               /* class_data */
         sizeof (GimvZList),
         0,                  /* n_preallocs */
         (GInstanceInitFunc) gimv_zlist_init,
      };

      type = g_type_register_static (GIMV_TYPE_SCROLLED,
                                     "GimvZList",
                                     &gimv_zlist_type_info,
                                     0);
   }
#else /* USE_GTK2 */
   if (!type) {
      static const GtkTypeInfo gimv_zlist_type_info = {
         "GimvZList",
         sizeof (GimvZList),
         sizeof (GimvZListClass),
         (GtkClassInitFunc) gimv_zlist_class_init,
         (GtkObjectInitFunc) gimv_zlist_init,
         /* reserved_1 */ NULL,
         /* reserved_2 */ NULL,
         (GtkClassInitFunc) NULL,
      };

      type = gtk_type_unique (GIMV_TYPE_SCROLLED, &gimv_zlist_type_info);
   }
#endif /* USE_GTK2 */

   return type;
}


static void        
gimv_zlist_class_init (GimvZListClass *klass)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;
   GtkContainerClass *container_class;
   GimvScrolledClass *scrolled_class;

   parent_class    = gtk_type_class (GIMV_TYPE_SCROLLED);

   object_class    = (GtkObjectClass*) klass;
   widget_class    = (GtkWidgetClass*) klass;
   container_class = (GtkContainerClass*) klass;
   scrolled_class  = (GimvScrolledClass*) klass;

#if (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED)
   gimv_zlist_signals [CLEAR] = 
      g_signal_new ("clear",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, clear),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

   gimv_zlist_signals [CELL_DRAW] = 
      g_signal_new ("cell_draw",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_draw),
                    NULL, NULL,
                    gimv_marshal_VOID__POINTER_POINTER_POINTER,
                    G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);

   gimv_zlist_signals [CELL_SIZE_REQUEST] = 
      g_signal_new ("cell_size_request",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_size_request),
                    NULL, NULL,
                    gimv_marshal_VOID__POINTER_POINTER,
                    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

   gimv_zlist_signals [CELL_DRAW_FOCUS] = 
      g_signal_new ("cell_draw_focus",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_draw_focus),
                    NULL, NULL,
                    gimv_marshal_VOID__POINTER_POINTER,
                    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

   gimv_zlist_signals [CELL_DRAW_DEFAULT] = 
      g_signal_new ("cell_draw_default",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_draw_default),
                    NULL, NULL,
                    gimv_marshal_VOID__POINTER_POINTER,
                    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

   gimv_zlist_signals [CELL_SELECT] = 
      g_signal_new ("cell_select",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_select),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__INT,
                    G_TYPE_NONE, 1, G_TYPE_INT);

   gimv_zlist_signals [CELL_UNSELECT] = 
      g_signal_new ("cell_unselect",
                    G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GimvZListClass, cell_unselect),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__INT,
                    G_TYPE_NONE, 1, G_TYPE_INT);
#else /* (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED) */
   gimv_zlist_signals [CLEAR] = 
      gtk_signal_new ("clear",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, clear),
                      gtk_marshal_NONE__NONE,
                      GTK_TYPE_NONE, 0);

   gimv_zlist_signals [CELL_DRAW] = 
      gtk_signal_new ("cell_draw",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_draw),
                      gtk_marshal_NONE__POINTER_POINTER_POINTER,
                      GTK_TYPE_NONE, 3,
                      GTK_TYPE_POINTER, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

   gimv_zlist_signals [CELL_SIZE_REQUEST] = 
      gtk_signal_new ("cell_size_request",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_size_request),
                      gtk_marshal_NONE__POINTER_POINTER,
                      GTK_TYPE_NONE, 2,
                      GTK_TYPE_POINTER, GTK_TYPE_POINTER);

   gimv_zlist_signals [CELL_DRAW_FOCUS] = 
      gtk_signal_new ("cell_draw_focus",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_draw_focus),
                      gtk_marshal_NONE__POINTER_POINTER,
                      GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

   gimv_zlist_signals [CELL_DRAW_DEFAULT] = 
      gtk_signal_new ("cell_draw_default",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_draw_default),
                      gtk_marshal_NONE__POINTER_POINTER,
                      GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);

   gimv_zlist_signals [CELL_SELECT] = 
      gtk_signal_new ("cell_select",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_select),
                      gtk_marshal_NONE__INT,
                      GTK_TYPE_NONE, 1, GTK_TYPE_INT);

   gimv_zlist_signals [CELL_UNSELECT] = 
      gtk_signal_new ("cell_unselect",
                      GTK_RUN_FIRST,
                      GTK_CLASS_TYPE(object_class),
                      GTK_SIGNAL_OFFSET(GimvZListClass, cell_unselect),
                      gtk_marshal_NONE__INT,
                      GTK_TYPE_NONE, 1, GTK_TYPE_INT);

   gtk_object_class_add_signals (object_class, gimv_zlist_signals, LAST_SIGNAL);
#endif /* (defined USE_GTK2) && (defined GTK_DISABLE_DEPRECATED) */

   OBJECT_CLASS_SET_FINALIZE_FUNC (klass, gimv_zlist_finalize);

   widget_class->map                    = gimv_zlist_map;
   widget_class->unmap                  = gimv_zlist_unmap;
   widget_class->realize                = gimv_zlist_realize;
   widget_class->unrealize              = gimv_zlist_unrealize;
   widget_class->size_request           = gimv_zlist_size_request;
   widget_class->size_allocate          = gimv_zlist_size_allocate;
   widget_class->expose_event           = gimv_zlist_expose;
#ifndef USE_GTK2
   widget_class->draw                   = gimv_zlist_draw;
#endif

   widget_class->button_press_event     = gimv_zlist_button_press;
   widget_class->button_release_event   = gimv_zlist_button_release;
   widget_class->motion_notify_event    = gimv_zlist_motion_notify;
   widget_class->key_press_event        = gimv_zlist_key_press;
   widget_class->focus_in_event         = gimv_zlist_focus_in;
   widget_class->focus_out_event        = gimv_zlist_focus_out;
   widget_class->drag_motion            = gimv_zlist_drag_motion;
   widget_class->drag_drop              = gimv_zlist_drag_drop;
   widget_class->drag_leave             = gimv_zlist_drag_leave;

   container_class->forall              = gimv_zlist_forall;
   /*  container_class->focus               = gimv_zlist_focus; */
   scrolled_class->adjust_adjustments   = gimv_zlist_adjust_adjustments;

   klass->cell_draw                     = NULL;
   klass->cell_size_request             = NULL;
   klass->cell_draw_focus               = NULL;
   klass->cell_draw_default             = NULL;
   klass->cell_select                   = NULL;
   klass->cell_unselect                 = NULL;
}


static void        
gimv_zlist_init (GimvZList *list)
{
   GTK_WIDGET_UNSET_FLAGS (list, GTK_NO_WINDOW);
   GTK_WIDGET_SET_FLAGS (list, GTK_CAN_FOCUS);
  
   list->flags            = 0;
   list->cell_width       = 1;
   list->cell_height      = 1;
   list->rows             = 1;
   list->columns          = 1;
   list->cells            = NULL;
   list->cell_count       = 0;
   list->selection_mode   = GTK_SELECTION_SINGLE;
   list->selection        = NULL;
   list->focus            = -1;
   list->anchor           = -1;     
   list->cell_x_pad       = 4;
   list->cell_y_pad       = 4;
   list->x_pad            = 0;
   list->y_pad            = 0;
   list->entered_cell     = NULL;

   list->region_select    = GIMV_ZLIST_REGION_SELECT_OFF;
   list->region_line_gc   = NULL;
   list->selection_mask   = NULL;
}


void
gimv_zlist_construct (GimvZList *list, int flags)
{
   g_return_if_fail (list);

   list->flags          = flags;
   list->cells          = g_array_new (0, 0, sizeof (gpointer));
}


GtkWidget*         
gimv_zlist_new (guint flags)
{
   GimvZList *list;

#ifdef USE_GTK2
   list = g_object_new (gimv_zlist_get_type (), NULL);
#else /* USE_GTK2 */
   list = (GimvZList*) gtk_type_new (gimv_zlist_get_type());
#endif /* USE_GTK2 */
   g_return_val_if_fail (list, NULL);

   gimv_zlist_construct (list, flags);

   return (GtkWidget*) list;
}


void
gimv_zlist_set_to_vertical (GimvZList *zlist)
{
   g_return_if_fail (GIMV_IS_ZLIST (zlist));

   zlist->flags &= ~GIMV_ZLIST_HORIZONTAL;

   if (GTK_WIDGET_VISIBLE (zlist)) {
      WIDGET_DRAW (GTK_WIDGET (zlist));
   }
}


void
gimv_zlist_set_to_horizontal (GimvZList *zlist)
{
   g_return_if_fail (GIMV_IS_ZLIST (zlist));

   zlist->flags |= GIMV_ZLIST_HORIZONTAL;

   if (GTK_WIDGET_VISIBLE (zlist)) {
      WIDGET_DRAW (GTK_WIDGET (zlist));
   }
}


static void
#ifdef USE_GTK2
gimv_zlist_finalize (GObject *object)
#else  /* USE_GTK2 */
gimv_zlist_finalize (GtkObject *object)
#endif /* USE_GTK2 */
{
   if (GIMV_ZLIST (object)->region_line_gc)
      gdk_gc_destroy (GIMV_ZLIST (object)->region_line_gc);

   OBJECT_CLASS_FINALIZE_SUPER (parent_class, object);
}


static void        
gimv_zlist_map (GtkWidget *widget)
{
   GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);
   gdk_window_show (widget->window);
   gimv_zlist_update(GIMV_ZLIST (widget));
}


static void        
gimv_zlist_unmap (GtkWidget *widget)
{
   GTK_WIDGET_UNSET_FLAGS(widget, GTK_MAPPED);
   gdk_window_hide (widget->window);
}


static void        
gimv_zlist_realize (GtkWidget *widget)
{
   GdkWindowAttr attributes;
   gint attributes_mask;

   GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

   attributes.window_type   = GDK_WINDOW_CHILD;
   attributes.x             = widget->allocation.x + bw (widget);
   attributes.y             = widget->allocation.y + bw (widget);
   attributes.width         = widget->allocation.width  - 2 * bw (widget);
   attributes.height        = widget->allocation.height - 2 * bw (widget);
   attributes.wclass        = GDK_INPUT_OUTPUT;
   attributes.visual        = gtk_widget_get_visual (widget);
   attributes.colormap      = gtk_widget_get_colormap (widget);
   attributes.event_mask    = gtk_widget_get_events (widget);
   attributes.event_mask    |= (GDK_EXPOSURE_MASK |
                                GDK_BUTTON_PRESS_MASK |
                                GDK_BUTTON_RELEASE_MASK |
                                GDK_POINTER_MOTION_MASK |
                                GDK_KEY_PRESS_MASK);
   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

   widget->window = gdk_window_new (gtk_widget_get_parent_window(widget),
                                    &attributes, attributes_mask);
   gdk_window_set_user_data (widget->window, widget);
   widget->style = gtk_style_attach (widget->style, widget->window);

   gdk_window_set_background (widget->window,
                              &widget->style->base [GTK_STATE_NORMAL]);

   gimv_scrolled_realize (GIMV_SCROLLED(widget));
}


static void
gimv_zlist_unrealize (GtkWidget *widget)
{
   gimv_scrolled_unrealize (GIMV_SCROLLED(widget));

   if (parent_class->unrealize)
      (* parent_class->unrealize) (widget);
}


static void        
gimv_zlist_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
   requisition->width = requisition->height = 50;
}


static void        
gimv_zlist_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
   if (allocation->x == widget->allocation.x
       && allocation->y == widget->allocation.y
       && allocation->width == widget->allocation.width
       && allocation->height == widget->allocation.height)
   {
      return;
   }

   widget->allocation = *allocation;

   if (GTK_WIDGET_REALIZED(widget)) 
      gdk_window_move_resize (widget->window,
                              allocation->x + bw (widget), 
                              allocation->y + bw (widget),
                              allocation->width  - 2 * bw (widget), 
                              allocation->height - 2 * bw (widget));

   gimv_zlist_update (GIMV_ZLIST(widget));
}


static void
gimv_zlist_update (GimvZList *list)
{
   GtkWidget *widget;
   GtkAdjustment *adj;

   widget = GTK_WIDGET(list);

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      if (list->flags & GIMV_ZLIST_1) 
         list->cell_height = widget->allocation.height - 2 * bw (widget);

      list->rows    = MAX(1, (widget->allocation.height - 2 * bw (widget)) / list->cell_height);
      list->columns = list->cell_count % list->rows ? 
         list->cell_count / list->rows + 1 :
         list->cell_count / list->rows;

      list->y_pad = (widget->allocation.height - LIST_HEIGHT(list) - 2 * bw (widget)) / 2;
      if (list->y_pad < 0) list->y_pad = 0;

      gimv_zlist_adjust_adjustments (GIMV_SCROLLED(list));
      adj = GIMV_SCROLLED(widget)->h_adjustment;
      if (adj && !GIMV_SCROLLED(widget)->freeze_count
          && adj->value > adj->upper - adj->page_size)
      {
         adj->value = adj->upper - adj->page_size;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed", NULL);
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed", NULL);
#endif /* USE_GTK2 */
      }

   } else {
      if (list->flags & GIMV_ZLIST_1)
         list->cell_width = widget->allocation.width - 2 * bw (widget);

      list->columns = MAX(1, (widget->allocation.width - 2 * bw (widget)) / list->cell_width);
      list->rows    = list->cell_count % list->columns ?
         list->cell_count / list->columns + 1 :
         list->cell_count / list->columns;
  
      list->x_pad = (widget->allocation.width - LIST_WIDTH(list) - 2 * bw (widget)) / 2; 
      if (list->x_pad < 0) list->x_pad = 0;

      gimv_zlist_adjust_adjustments (GIMV_SCROLLED(widget));
      adj = GIMV_SCROLLED(widget)->v_adjustment;
      if (adj
          && !GIMV_SCROLLED(widget)->freeze_count
          && adj->value > adj->upper - adj->page_size)
      {
         adj->value = adj->upper - adj->page_size;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed", NULL);
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed", NULL);
#endif /* USE_GTK2 */
      }
   }
}


static gint        
gimv_zlist_expose (GtkWidget *widget, GdkEventExpose *event)
{
   if (GTK_WIDGET_DRAWABLE(widget) && event->window == widget->window)
      gimv_zlist_draw (widget, &event->area);

   return FALSE; /* xxx */
}


gboolean
gimv_zlist_get_cell_area (GimvZList *list, gint index, GdkRectangle *area)
{
   gint col, row;

   g_return_val_if_fail (GIMV_IS_ZLIST (list), FALSE);
   g_return_val_if_fail (area, FALSE);
   g_return_val_if_fail (index >= 0 && index < list->cell_count, FALSE);

   if (list->flags & GIMV_ZLIST_HORIZONTAL){
      col = index / list->rows;
      row = index % list->rows;
   } else {
      col = index % list->columns;
      row = index / list->columns;
   }

   area->x = CELL_X_FROM_COL(list, col) + list->cell_x_pad;
   area->y = CELL_Y_FROM_ROW(list, row) + list->cell_y_pad;
   area->width  = list->cell_width - list->cell_x_pad;
   area->height = list->cell_height - list->cell_y_pad;

   return TRUE;
}


static void        
gimv_zlist_draw_horizontal_list (GtkWidget *widget, GdkRectangle *area)
{
   GimvZList *list;
   gpointer *cell;
   GdkRectangle cell_area, intersect_area;
   gint first_row, last_row;
   gint first_column, last_column;
   gint i, j, idx, c;

   list = GIMV_ZLIST (widget);

   first_column = CELL_COL_FROM_X(list, area->x);
   first_column = CLAMP(first_column, 0, list->columns);
   last_column  = CELL_COL_FROM_X(list, area->x + area->width) + 1; 
   last_column  = CLAMP(last_column, 0, list->columns);

   first_row    = CELL_ROW_FROM_Y(list, area->y);
   first_row    = CLAMP(first_row, 0, list->rows);
   last_row     = CELL_ROW_FROM_Y(list, area->y + area->height) + 1;
   last_row     = CLAMP(last_row, 0, list->rows);

   /* clear the padding area (bottom & top) */
   c = list->y_pad - area->y;
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x, area->y,
                             area->width, c);

   c = area->y + area->height - (LIST_HEIGHT(list) + list->y_pad);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x, area->y + area->height - c, 
                             area->width, c);


   for (j = first_column; j < last_column; j++) {

      /* clear the cell vertical padding */
      if (list->cell_x_pad)
         gdk_window_clear_area (widget->window,
                                CELL_X_FROM_COL(list, j), area->y,
                                list->cell_x_pad, area->height);

      idx = j * list->rows + first_row;
      for (i = first_row; i < last_row; i++, idx++) {

         /* clear the cell horizontal padding */
         if (list->cell_y_pad)
            gdk_window_clear_area (widget->window,
                                   area->x, CELL_Y_FROM_ROW(list, i),
                                   area->width, list->cell_y_pad);
	
         cell_area.x = CELL_X_FROM_COL(list, j) + list->cell_x_pad;
         cell_area.y = CELL_Y_FROM_ROW(list, i) + list->cell_y_pad;
         cell_area.width  = list->cell_width - list->cell_x_pad;
         cell_area.height = list->cell_height - list->cell_y_pad;
       
         if (gdk_rectangle_intersect (area, &cell_area, &intersect_area)) {
            if (idx < list->cell_count) {
               cell = GIMV_ZLIST_CELL_FROM_INDEX (list, idx);

#ifdef USE_GTK2
               g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_DRAW], 0,
                              cell, &cell_area, &intersect_area);
#else /* USE_GTK2 */
               gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_DRAW],
                                cell, &cell_area, &intersect_area);
#endif /* USE_GTK2 */
            } else {
               gdk_window_clear_area (widget->window,
                                      intersect_area.x, intersect_area.y,
                                      intersect_area.width, intersect_area.height);
            }
         }
      } /* rows loop */
   } /* columns loop */

   /* clear the right of the list  XXX should hasppen */
   c = area->x + area->width - CELL_X_FROM_COL(list, j);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             CELL_X_FROM_COL(list, j), area->y,
                             c, area->height);
   /* clear the bottom of the list */
   c = area->y + area->height - CELL_Y_FROM_ROW(list, last_row);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x, CELL_Y_FROM_ROW(list, last_row),
                             area->width, c);
}


static void        
gimv_zlist_draw_vertical_list (GtkWidget *widget, GdkRectangle *area)
{
   GimvZList *list;
   gpointer *cell;
   GdkRectangle cell_area, intersect_area;
   gint first_row, last_row;
   gint first_column, last_column;
   gint i, j, idx, c;

   list = GIMV_ZLIST (widget);

   first_column = CELL_COL_FROM_X(list, area->x);
   first_column = CLAMP(first_column, 0, list->columns);
   last_column  = CELL_COL_FROM_X(list, area->x + area->width) + 1; 
   last_column  = CLAMP(last_column, 0, list->columns);

   first_row    = CELL_ROW_FROM_Y(list, area->y);
   first_row    = CLAMP(first_row, 0, list->rows);
   last_row     = CELL_ROW_FROM_Y(list, area->y + area->height) + 1;
   last_row     = CLAMP(last_row, 0, list->rows);

   /* clear the padding area (right & left) */
   c = list->x_pad - area->x;
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x, area->y,
                             c, area->height);

   c = area->x + area->width - (LIST_WIDTH(list) + list->x_pad);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x + area->width - c, area->y,
                             c, area->height);

   for (i = first_row; i < last_row; i++) {

      /* clear the cell horizontal padding */
      if (list->cell_y_pad)
         gdk_window_clear_area (widget->window,
                                area->x, CELL_Y_FROM_ROW(list, i),
                                area->width, list->cell_y_pad);

      idx = i * list->columns + first_column;
      for (j = first_column; j < last_column; j++, idx++) {

         /* clear the cell vertical padding */
         if (list->cell_x_pad)
            gdk_window_clear_area (widget->window,
                                   CELL_X_FROM_COL(list, j), area->y,
                                   list->cell_x_pad, area->height);

         cell_area.x = CELL_X_FROM_COL(list, j) + list->cell_x_pad;
         cell_area.y = CELL_Y_FROM_ROW(list, i) + list->cell_y_pad;
         cell_area.width  = list->cell_width - list->cell_x_pad;
         cell_area.height = list->cell_height - list->cell_y_pad;

         if (gdk_rectangle_intersect (area, &cell_area, &intersect_area)) {
            if (idx < list->cell_count) {
               cell = GIMV_ZLIST_CELL_FROM_INDEX (list, idx);

#ifdef USE_GTK2
               g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_DRAW], 0,
                              cell, &cell_area, &intersect_area);
#else /* USE_GTK2 */
               gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_DRAW],
                                cell, &cell_area, &intersect_area);
#endif /* USE_GTK2 */
            } else {
               gdk_window_clear_area (widget->window,
                                      intersect_area.x, intersect_area.y, 
                                      intersect_area.width, intersect_area.height);
            }
         }
      } /* columns loop */
   } /* rows loop */

   /* clear the right of the list  XXX should hasppen */
   c = area->x + area->width - CELL_X_FROM_COL(list, last_column);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             CELL_X_FROM_COL(list, last_column), area->y,
                             c, area->height);
   /* clear the bottom of the list */
   c = area->y + area->height - CELL_Y_FROM_ROW(list, i);
   if (c > 0)
      gdk_window_clear_area (widget->window,
                             area->x, CELL_Y_FROM_ROW(list, i),
                             area->width, c);
}


static void
gimv_zlist_draw_selection_region (GimvZList *list, GdkRectangle *area)
{
   GtkWidget *widget;
   GimvScrolled *scrolled;
   GtkAdjustment *hadj, *vadj;
   GdkRectangle widget_area, region_area, draw_area;
   gint ds_vx, ds_vy, de_vx, de_vy;
   gchar dash[] = {2, 1};

   widget = GTK_WIDGET (list);
   scrolled = GIMV_SCROLLED (list);

   hadj = scrolled->h_adjustment;
   vadj = scrolled->v_adjustment;

   ds_vx = scrolled->drag_start_vx;
   ds_vy = scrolled->drag_start_vy;
   de_vx = GIMV_SCROLLED_VX (scrolled, scrolled->drag_motion_x);
   de_vy = GIMV_SCROLLED_VY (scrolled, scrolled->drag_motion_y);

   region_area.x = GIMV_SCROLLED_X (list, MIN (ds_vx, de_vx));
   region_area.y = GIMV_SCROLLED_Y (list, MIN (ds_vy, de_vy));
   region_area.width  = abs (de_vx - ds_vx);
   region_area.height = abs (de_vy - ds_vy);

   widget_area.x = widget_area.y = 0;
   widget_area.width  = widget->allocation.width;
   widget_area.height = widget->allocation.height;

   if (!gdk_rectangle_intersect (&widget_area, &region_area, &draw_area))
      return;

   if (!list->region_line_gc) {
      list->region_line_gc = gdk_gc_new (widget->window);
      gdk_gc_copy (list->region_line_gc, widget->style->black_gc);
      gdk_gc_set_line_attributes (list->region_line_gc, 1,
                                  GDK_LINE_ON_OFF_DASH,
                                  GDK_CAP_NOT_LAST,
                                  GDK_JOIN_MITER);
      gdk_gc_set_dashes (list->region_line_gc, 0, dash, 2);
   }

   gdk_draw_rectangle (widget->window,
                       list->region_line_gc,
                       FALSE,
                       draw_area.x, draw_area.y,
                       draw_area.width, draw_area.height);
}


static void        
gimv_zlist_draw (GtkWidget *widget, GdkRectangle *area)
{
   GimvZList *list;
   GimvScrolled *scr;
   GdkRectangle list_area;

   list = GIMV_ZLIST(widget);
   scr = GIMV_SCROLLED(widget);

   if (!GTK_WIDGET_DRAWABLE(widget) || scr->freeze_count)
      return;

   list_area.x = list_area.y = 0;
   /* xxx */
   list_area.width  = widget->allocation.width;
   list_area.height = widget->allocation.height;

   if (!area) 
      area = &list_area;

   if (!list->cell_count) {
      gdk_window_clear_area (widget->window,
                             area->x, area->y,
                             area->width, area->height);
      return;
   }

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      gimv_zlist_draw_horizontal_list (widget, area);
   } else {
      gimv_zlist_draw_vertical_list (widget, area);
   }

   if (list->focus > -1)
      gimv_zlist_cell_draw_focus (list, list->focus);

   if (list->region_select)
      gimv_zlist_draw_selection_region (list, area);

   if (list->flags & GIMV_ZLIST_HIGHLIGHTED)
      gimv_zlist_highlight (widget);
}


static void
gimv_zlist_redraw_selection_region (GtkWidget *widget,
                                    gint prev_x, gint prev_y,
                                    gint next_x, gint next_y)
{
   GimvScrolled *scrolled;
   GdkRectangle widget_area, area, draw_area;
   gint start_x, start_y;
   gint low, high;

   scrolled = GIMV_SCROLLED (widget);

   start_x = GIMV_SCROLLED_X (scrolled, scrolled->drag_start_vx);
   start_y = GIMV_SCROLLED_Y (scrolled, scrolled->drag_start_vy);

   widget_area.x = widget_area.y = 0;
   widget_area.width  = widget->allocation.width;
   widget_area.height = widget->allocation.height;

   /* horizontal line */
   low  = MIN (start_x, prev_x);
   high = MAX (start_x, prev_x);
   area.x = low - 1;
   area.y = start_y - 1;
   area.width = high - area.x + 3;
   area.height = 3;
   if (gdk_rectangle_intersect (&widget_area, &area, &draw_area))
      gimv_zlist_draw (widget, &draw_area);

   area.y = prev_y - 1;
   if (gdk_rectangle_intersect (&widget_area, &area, &draw_area))
      gimv_zlist_draw (widget, &draw_area);

   /* vertical line */
   low  = MIN (start_y, prev_y);
   high = MAX (start_y, prev_y);
   area.x = start_x - 1;
   area.y = low - 1;
   area.width = 3;
   area.height = high - area.y + 3;
   if (gdk_rectangle_intersect (&widget_area, &area, &draw_area))
      gimv_zlist_draw (widget, &draw_area);

   area.x = prev_x - 1;
   if (gdk_rectangle_intersect (&widget_area, &area, &draw_area))
      gimv_zlist_draw (widget, &draw_area);
}


static gint        
gimv_zlist_button_press (GtkWidget *widget, GdkEventButton *event)
{
   GimvZList *list;
   gpointer *cell;
   gint retval = FALSE, idx;

   list = GIMV_ZLIST(widget);

   /* grab focus */
   if (!GTK_WIDGET_HAS_FOCUS(list))
      gtk_widget_grab_focus (widget);

   /* call parent method */
   if (parent_class->button_press_event)
      retval = parent_class->button_press_event (widget, event);

   list->region_select = GIMV_ZLIST_REGION_SELECT_OFF;

   if (event->type != GDK_BUTTON_PRESS || 
       event->button != 1 ||
       event->window != widget->window)
   {
      return retval || FALSE;
   }

   /* get the selected cell's index */
   idx = gimv_zlist_cell_index_from_xy (list, event->x, event->y);

   /* set to region select mode */
   if (idx < 0) {
      if (event->state & GDK_CONTROL_MASK) {          /* toggle mode */
         list->region_select = GIMV_ZLIST_REGION_SELECT_TOGGLE;
         gimv_zlist_set_selection_mask (list, NULL);
      } else if (event->state & GDK_SHIFT_MASK) {   /* expand mode */
         list->region_select = GIMV_ZLIST_REGION_SELECT_EXPAND;
         gimv_zlist_set_selection_mask (list, NULL);
      } else {
         list->region_select = GIMV_ZLIST_REGION_SELECT_NORMAL;
         gimv_zlist_unselect_all (list);
      }

   } else {
      /* get the selected cell */
      cell = GIMV_ZLIST_CELL_FROM_INDEX (list, idx);

      /* set focus */
      if (list->focus != idx) {
         if (list->focus > -1)
            gimv_zlist_cell_draw_default (list, list->focus);
         list->focus = idx;
      }

      /* set to the DnD mode */
      if (list->flags & GIMV_ZLIST_USES_DND
          && g_list_find (list->selection, GUINT_TO_POINTER(idx)))
      {
         list->anchor = idx;
         gimv_zlist_cell_draw_focus (list, idx);
         return retval || FALSE;
      }
  
      /* set selection */
      switch (list->selection_mode) {
      case GTK_SELECTION_SINGLE:
#ifndef USE_GTK2
      case GTK_SELECTION_MULTIPLE:
#endif
         list->anchor = idx;
         gimv_zlist_cell_draw_focus (list, idx);
         break;

      case GTK_SELECTION_BROWSE:
         gimv_zlist_unselect_all (list);
         gimv_zlist_cell_select (list, idx);
         break;

      case GTK_SELECTION_EXTENDED:
         if (event->state & GDK_CONTROL_MASK) {
            list->anchor = idx;
            gimv_zlist_cell_toggle (list, idx);
         } else if (event->state & GDK_SHIFT_MASK) {
            gimv_zlist_extend_selection (list, idx);
         } else {
            list->anchor = idx;

            if (!g_list_find (list->selection, GUINT_TO_POINTER(idx))) {
               gimv_zlist_unselect_all (list);
               gimv_zlist_cell_select (list, idx);
            }
         }
         break;

      default:
         break;
      }
   }

   if (gdk_pointer_grab (widget->window, FALSE,
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_BUTTON_RELEASE_MASK,
                         NULL, NULL, event->time))
      return retval || FALSE;

   gtk_grab_add (widget);

   return retval || FALSE;
}


static gint        
gimv_zlist_button_release (GtkWidget *widget, GdkEventButton *event)
{
   GimvZList *list;
   gpointer *cell;
   gint retval = FALSE, index;

   list = GIMV_ZLIST(widget);

   /* call parent class callback */
   if (parent_class->button_release_event)
      retval = parent_class->button_release_event (widget, event);

   if (GTK_WIDGET_HAS_GRAB(widget))
      gtk_grab_remove (widget);

   if (gdk_pointer_is_grabbed ())
      gdk_pointer_ungrab (event->time);
  
   index = gimv_zlist_cell_index_from_xy (list, event->x, event->y);
   if (index < 0) goto FUNC_END;

   cell = GIMV_ZLIST_CELL_FROM_INDEX (list, index);

   switch (list->selection_mode) {
   case GTK_SELECTION_SINGLE:
      if (list->anchor == index) {
         list->focus = index;
         gimv_zlist_unselect_all (list);
         gimv_zlist_cell_toggle (list, index);
      }
      list->anchor = index;
      break;

#ifndef USE_GTK2
   case GTK_SELECTION_MULTIPLE:
      if (list->anchor == index) {
         list->focus = index;
         gimv_zlist_cell_toggle (list, index);
      }
      list->anchor = index;
      break;
#endif

   case GTK_SELECTION_EXTENDED:
      if (event->state & GDK_CONTROL_MASK) {
      } else if (event->state & GDK_SHIFT_MASK) {
      } else {
         if (event->button == 1 && !list->region_select) {
            gimv_zlist_unselect_all (list);
            gimv_zlist_cell_select (list, index);
         }
      }

      if ((list->flags & GIMV_ZLIST_USES_DND)) {
         gint x, y;

         /* on a drag-drop event, the event->x & event->y always seem to be 0 */
         gdk_window_get_pointer (widget->window, &x, &y, NULL);
         index = gimv_zlist_cell_index_from_xy (list, x, y);
         if (index < 0) goto FUNC_END;
         /*
           if (list->anchor == index)
           gimv_zlist_extend_selection (list, list->focus);
         */
      }

      list->anchor = list->focus;
      break;

   default:
      break;
   }

FUNC_END:

   /* unset region select */
   if (list->region_select)
      gimv_zlist_draw (widget, NULL);
   gimv_zlist_unset_selection_mask (list);
   list->region_select = GIMV_ZLIST_REGION_SELECT_OFF;

   return retval || FALSE;
}


static gint       
gimv_zlist_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
   GimvScrolled *scrolled;
   GimvZList *list;
   gpointer *cell;
   gint index, x, y, retval = FALSE;

   gint start_x, start_y, end_x, end_y, prev_x = 0, prev_y = 0;
   gint flags;
   gboolean pressed;

   list = GIMV_ZLIST(widget);
   scrolled = GIMV_SCROLLED (widget);

   flags = scrolled->autoscroll_flags;
   pressed = scrolled->pressed;

   index = gimv_zlist_cell_index_from_xy (list, x, y);

   if (list->region_select) {
      prev_x = scrolled->drag_motion_x;
      prev_y = scrolled->drag_motion_y;
   }

   /* call parent class callback */
   if (parent_class->motion_notify_event)
      retval = parent_class->motion_notify_event (widget, event);

   if (list->region_select) {
      if ((pressed && (flags & GIMV_SCROLLED_AUTO_SCROLL_MOTION))
          || (flags & GIMV_SCROLLED_AUTO_SCROLL_MOTION_ALL))
      {
         start_x = MIN (scrolled->drag_start_vx, GIMV_SCROLLED_VX (scrolled, event->x));
         start_y = MIN (scrolled->drag_start_vy, GIMV_SCROLLED_VY (scrolled, event->y));
         end_x   = MAX (scrolled->drag_start_vx, GIMV_SCROLLED_VX (scrolled, event->x));
         end_y   = MAX (scrolled->drag_start_vy, GIMV_SCROLLED_VY (scrolled, event->y));

         gimv_zlist_cell_select_by_pixel_region (list,
                                                 start_x, start_y,
                                                 end_x, end_y);

         gimv_zlist_redraw_selection_region (widget,
                                             prev_x, prev_y,
                                             event->x, event->y);
      }
   }

   if (list->flags & GIMV_ZLIST_USES_DND) return retval || FALSE;
  
   gdk_window_get_pointer (widget->window, &x, &y, NULL);

   index = gimv_zlist_cell_index_from_xy (list, x, y);
   if (index < 0) return retval || FALSE;


   cell = GIMV_ZLIST_CELL_FROM_INDEX (list, index);

   if (gdk_pointer_is_grabbed() && GTK_WIDGET_HAS_GRAB(widget)) {

      if (index == list->focus) return retval || FALSE;

      gimv_zlist_cell_draw_default (list, list->focus);
      list->focus = index;

      switch (list->selection_mode) {
      case GTK_SELECTION_SINGLE:
#ifndef USE_GTK2
      case GTK_SELECTION_MULTIPLE:
#endif
         gimv_zlist_cell_draw_focus (list, index);
         break;

      case GTK_SELECTION_BROWSE:
         gimv_zlist_unselect_all (list);
         gimv_zlist_cell_select (list, index);
         break;
    
#if 0
      case GTK_SELECTION_EXTENDED:
         if (event->state & GDK_CONTROL_MASK) {
            list->anchor = index;
            gimv_zlist_cell_toggle (list, index);
         } else {
            gimv_zlist_extend_selection (list, index);
         }
         break;
#endif

      default:
         break;
      }
   } else {    
   }

   return retval || FALSE;
}

/* 
 * GtkContainers doesn't seem to forward the focus movement to
 * containers which don't have child widgets
 * 
 */

static gint        
gimv_zlist_key_press (GtkWidget *widget, GdkEventKey *event)
{
   gint direction = -1;

   g_return_val_if_fail (widget && event, FALSE);
  
   if (GTK_WIDGET_HAS_FOCUS (widget)) {
      switch (event->keyval) {
      case GDK_Up:
         direction = GTK_DIR_UP;
         break;
      case GDK_Down:
         direction = GTK_DIR_DOWN;
         break;
      case GDK_Left:
         direction = GTK_DIR_LEFT;
         break;
      case GDK_Right:
         direction = GTK_DIR_RIGHT;
         break;
      /*
      case GDK_Tab:
      case GDK_ISO_Left_Tab:
         if (event->state & GDK_SHIFT_MASK)
            direction = GTK_DIR_TAB_BACKWARD;
         else
            direction = GTK_DIR_TAB_FORWARD;
          break;
      */
      case GDK_Page_Up:
         gimv_scrolled_page_up (GIMV_SCROLLED (widget));
         break;
      case GDK_Page_Down:
         gimv_scrolled_page_down (GIMV_SCROLLED (widget));
         break;
      default:
         break;
      } 
   }
  
   if (direction != -1) {
      gimv_zlist_focus (GTK_CONTAINER(widget), direction);
      return FALSE;
   }

   if (parent_class->key_press_event &&
       (* parent_class->key_press_event) (widget, event))
      return TRUE;

   return FALSE;
}


static gint        
gimv_zlist_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
   GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

#ifndef USE_GTK2
   gtk_widget_draw_focus (widget);
#endif  

   return FALSE;
}


static gint        
gimv_zlist_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
   GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

#ifndef USE_GTK2
   gtk_widget_draw_default (widget);
#endif
  
   return FALSE;
}


static gint        
gimv_zlist_drag_motion (GtkWidget      *widget,
                        GdkDragContext *context,
                        gint            x,
                        gint            y,
                        guint           time)
{
   GimvZList *list;
   gint index, retval = FALSE;

   g_return_val_if_fail (widget, FALSE);

   if (parent_class->button_press_event)
      retval = parent_class->drag_motion (widget, context, x, y, time);

   list = GIMV_ZLIST(widget);

   if (!(list->flags & GIMV_ZLIST_USES_DND))
      return retval || FALSE;

   if (!(list->flags & GIMV_ZLIST_HIGHLIGHTED)) {
      list->flags |= GIMV_ZLIST_HIGHLIGHTED;
      gimv_zlist_highlight (widget);
   }

   index = gimv_zlist_cell_index_from_xy (GIMV_ZLIST(widget), x, y);
   if (index < 0)
      return retval || FALSE;
   /*
   if (g_list_find (ZLIST(widget)->selection, GUINT_TO_POINTER(index)))
      return TRUE;
   */
   return retval || FALSE;
}


static gint        
gimv_zlist_drag_drop (GtkWidget *widget,
                      GdkDragContext *context,
                      gint x,
                      gint y,
                      guint time)
{
   GimvZList *list;

   list = GIMV_ZLIST(widget);

   if (!(list->flags & GIMV_ZLIST_USES_DND))
      return FALSE;

   return FALSE;
}


static void        
gimv_zlist_drag_leave (GtkWidget *widget,
                       GdkDragContext *context,
                       guint time)
{
   GimvZList *list;

   if (parent_class->drag_leave)
      parent_class->drag_leave (widget, context, time);

   list = GIMV_ZLIST(widget);
   if (list->flags & GIMV_ZLIST_HIGHLIGHTED) {
      list->flags &= ~GIMV_ZLIST_HIGHLIGHTED;
      gimv_zlist_unhighlight (widget);
   }
}

static void
gimv_zlist_highlight (GtkWidget *widget)
{
#ifdef USE_GTK2
   gtk_paint_shadow (widget->style,
                     widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                     NULL, NULL, NULL,
                     0, 0,
                     widget->allocation.width - 2 * bw (widget),
                     widget->allocation.height - 2 * bw (widget));
#else /* USE_GTK2 */
   gtk_draw_shadow (widget->style,
                    widget->window,
                    GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                    0, 0,
                    widget->allocation.width - 2 * bw (widget),
                    widget->allocation.height - 2 * bw (widget));
#endif /* USE_GTK2 */

   gdk_draw_rectangle (widget->window,
                       widget->style->black_gc,
                       FALSE,
                       0, 0,
                       widget->allocation.width - 2 * bw (widget) - 1,
                       widget->allocation.height - 2 * bw (widget) - 1);
  
}
    

static void
gimv_zlist_unhighlight (GtkWidget *widget)
{
   GdkRectangle area;

   area.x = 0; area.y = 0;
   area.width  = HIGHLIGHT_SIZE;
   area.height = widget->allocation.height - 2 * bw (widget);
   WIDGET_DRAW_AREA (widget, &area);
  
   area.width  = widget->allocation.width - 2 * bw (widget);
   area.height = HIGHLIGHT_SIZE;
   WIDGET_DRAW_AREA (widget, &area);
  
   area.y      = widget->allocation.height - 2 * bw (widget) - HIGHLIGHT_SIZE;
   WIDGET_DRAW_AREA (widget, &area);

   area.x      = widget->allocation.width - 2 * bw (widget) - HIGHLIGHT_SIZE;
   area.y      = 0;
   area.width  = HIGHLIGHT_SIZE;
   area.height = widget->allocation.height - 2 * bw (widget);
   WIDGET_DRAW_AREA (widget, &area);
}


static void        
gimv_zlist_forall (GtkContainer *container,
                   gboolean include_internals,
                   GtkCallback callback,
                   gpointer callback_data)
{
   /*
   GimvZList *list;
   gint i;

   list = ZLIST(container);

   if (include_internals)
   for (i = 0; i < list->cell_count; i++)
      (* callback) (GIMV_ZLIST_CELL_FROM_INDEX (list, i), callback_data);
   */
}


static gint
gimv_zlist_focus (GtkContainer *container, GtkDirectionType dir)
{
   GimvZList *list;
   gint   focus;

   list = GIMV_ZLIST(container);

   if (list->focus < 0)
      return FALSE;

   focus = list->focus;
   switch (dir) {
   case GTK_DIR_LEFT:
      focus -= list->flags & GIMV_ZLIST_HORIZONTAL ? list->rows : 1;
      break;
   case GTK_DIR_RIGHT:
      focus += list->flags & GIMV_ZLIST_HORIZONTAL ? list->rows : 1;
      break;
   case GTK_DIR_UP:
   case GTK_DIR_TAB_BACKWARD:
      focus -= list->flags & GIMV_ZLIST_HORIZONTAL ? 1 : list->columns;
      break;
   case GTK_DIR_DOWN:
   case GTK_DIR_TAB_FORWARD:
      focus += list->flags & GIMV_ZLIST_HORIZONTAL ? 1 : list->columns;
      break;
   default:
      return FALSE;
      break;
   }

   if (focus < 0 || focus >= list->cell_count)
      return FALSE;

   gimv_zlist_cell_draw_default (list, list->focus);
   list->focus = focus;
   gimv_zlist_cell_draw_focus (list, focus);
   gimv_zlist_moveto (list, focus);

   return TRUE;
}


guint
gimv_zlist_add (GimvZList *list, gpointer cell)
{
   g_return_val_if_fail (GIMV_IS_ZLIST (list), 0);

   return gimv_zlist_insert (list, list->cells->len, cell);
}


guint
gimv_zlist_insert (GimvZList *list, guint pos, gpointer cell)
{
   GtkRequisition requisition = { 0 };
   gint adjust = FALSE;

   g_return_val_if_fail (GIMV_IS_ZLIST (list), 0);

   if (pos > list->cells->len)
      pos = list->cells->len;
   list->cells = g_array_insert_val (list->cells, pos, cell);

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_SIZE_REQUEST], 0,
                  cell, &requisition);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_SIZE_REQUEST],
                    cell, &requisition);
#endif /* USE_GTK2 */

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      if (list->cell_count && list->cell_count % list->rows == 0) {
         list->columns++;
         adjust = TRUE;
      }
   } else {
      if (list->cell_count && list->cell_count % list->columns == 0) {
         list->rows ++;
         adjust = TRUE;
      }
   }

   list->cell_count++;

   if (requisition.width  + list->cell_x_pad > list->cell_width ||
       requisition.height + list->cell_y_pad > list->cell_height)
   {
      list->cell_width
         = MAX(list->cell_width, requisition.width + list->cell_x_pad);
      list->cell_height
         = MAX(list->cell_height, requisition.height + list->cell_y_pad);

      gimv_zlist_update (list);
      WIDGET_DRAW (GTK_WIDGET (list));

   } else {

      if (adjust)
         gimv_zlist_adjust_adjustments (GIMV_SCROLLED(list));

      gimv_zlist_update (list);
      gimv_zlist_draw_cell (list, list->cell_count - 1);
   }

   return pos;
}


void        
gimv_zlist_remove (GimvZList *list, gpointer cell)
{
   GdkRectangle area;
   GList *item;
   gint index, *i;

   index = gimv_zlist_cell_index (list, cell);
   if (index == -1)
      return;

   list->cells = g_array_remove_index (list->cells, index);
   list->cell_count --;

   list->selection = g_list_remove (list->selection, GUINT_TO_POINTER(index));
   item = list->selection;
   while (item) {
      i = (guint *) &item->data;
      if (*i > index)
         *i -= 1;
      item = item->next;
   }

   if (list->focus == index)
      list->focus = -1;
   else if (list->focus > index)
      list->focus --;

   if (list->anchor == index)
      list->anchor = -1;
   else if (list->anchor > index)
      list->anchor --;

   if ((list->flags & GIMV_ZLIST_HORIZONTAL) && list->cell_count % list->rows == 0) {
      list->columns --;
      gimv_zlist_adjust_adjustments (GIMV_SCROLLED(list));
   } else if (!(list->flags & GIMV_ZLIST_HORIZONTAL) && list->cell_count % list->columns == 0) {
      list->rows --;
      gimv_zlist_adjust_adjustments (GIMV_SCROLLED(list));
   }

   gimv_zlist_cell_area (list, index, &area);

   /* using gdk_window_copy_area will be faster but harder too */
   area.width  = GTK_WIDGET(list)->allocation.width  - area.x;
   area.height = GTK_WIDGET(list)->allocation.height - area.y; 

   gimv_zlist_update (list);
   gimv_zlist_draw (GTK_WIDGET(list), &area); 

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      area.width -= list->cell_width;
      area.height = area.y;
      area.x     += list->cell_width;
      area.y      = 0;
   } else {
      area.width  = area.x;
      area.height -= list->cell_height;
      area.x      = 0;
      area.y      += list->cell_height;
   }

   WIDGET_DRAW_AREA (GTK_WIDGET (list), &area);
}


static void        
gimv_zlist_adjust_adjustments (GimvScrolled *scrolled)
{
   GimvZList *list;
   GtkWidget *widget;
   GtkAdjustment *adj;

   list = GIMV_ZLIST(scrolled);
   widget = GTK_WIDGET(scrolled);

   if (!GTK_WIDGET_DRAWABLE(widget) || scrolled->freeze_count)
      return;

   if (scrolled->h_adjustment) {
      adj = scrolled->h_adjustment;

      adj->page_size      = widget->allocation.width - 2 * bw (widget);
      adj->page_increment = adj->page_size;
      adj->step_increment = list->cell_width;
      adj->lower          = 0;
      adj->upper          = LIST_WIDTH(list) + 2 * list->x_pad + list->cell_x_pad;

#ifdef USE_GTK2
      g_signal_emit_by_name (G_OBJECT(adj), "changed");
#else /* USE_GTK2 */
      gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");
#endif /* USE_GTK2 */
   }

   if (scrolled->v_adjustment) {
      adj = scrolled->v_adjustment;

      adj->page_size      = widget->allocation.height - 2 * bw (widget);
      adj->page_increment = adj->page_size;
      adj->step_increment = list->cell_height;
      adj->lower          = 0;
      adj->upper          = LIST_HEIGHT(list) + 2 * list->y_pad + list->cell_y_pad;

#ifdef USE_GTK2
      g_signal_emit_by_name (G_OBJECT(adj), "changed");
#else /* USE_GTK2 */
      gtk_signal_emit_by_name (GTK_OBJECT(adj), "changed");
#endif /* USE_GTK2 */
   }
}


void          
gimv_zlist_clear (GimvZList *list)
{
   GimvScrolled *scrolled;

   g_return_if_fail (list);

   gimv_zlist_unselect_all (list);

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CLEAR], 0);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CLEAR]);
#endif /* USE_GTK2 */

   g_array_set_size (list->cells, 0);
   list->cell_count     = 0;
   list->focus          = -1;
   list->anchor         = -1;
   list->entered_cell   = NULL;

   if (list->flags & GIMV_ZLIST_HORIZONTAL)
      list->columns = 1;
   else
      list->rows = 1;

   scrolled = GIMV_SCROLLED(list);
   gimv_zlist_adjust_adjustments (GIMV_SCROLLED(list)); 
   scrolled->h_adjustment->value = 0;
   scrolled->v_adjustment->value = 0;
#ifdef USE_GTK2
   g_signal_emit_by_name (G_OBJECT(scrolled->h_adjustment), "value_changed");
   g_signal_emit_by_name (G_OBJECT(scrolled->v_adjustment), "value_changed");
#else /* USE_GTK2 */
   gtk_signal_emit_by_name (GTK_OBJECT(scrolled->h_adjustment), "value_changed");
   gtk_signal_emit_by_name (GTK_OBJECT(scrolled->v_adjustment), "value_changed");
#endif /* USE_GTK2 */

   gimv_zlist_draw (GTK_WIDGET(list), NULL);
}


void
gimv_zlist_set_cell_padding (GimvZList *list, gint x_pad, gint y_pad)
{
   g_return_if_fail (list);
   g_return_if_fail (x_pad >= 0 && y_pad >= 0);

   list->cell_width  += x_pad - list->cell_x_pad;
   list->cell_height += y_pad - list->cell_y_pad;

   list->cell_x_pad = x_pad;
   list->cell_y_pad = y_pad;

   gimv_zlist_update (list);
   WIDGET_DRAW (GTK_WIDGET (list));
}


void          
gimv_zlist_set_cell_size (GimvZList *list, gint width, gint height)
{
   g_return_if_fail (GIMV_IS_ZLIST (list));

   if (width > 0)
      list->cell_width  = width  + list->cell_x_pad;

   if (height > 0)
      list->cell_height = height + list->cell_y_pad;

   gimv_zlist_update (list);
   gimv_zlist_draw (GTK_WIDGET(list), NULL);
}


void
gimv_zlist_set_selection_mode (GimvZList *list, GtkSelectionMode mode)
{
   g_return_if_fail (GIMV_IS_ZLIST (list));

   list->selection_mode = mode;
   gimv_zlist_unselect_all (list);
   WIDGET_DRAW (GTK_WIDGET (list));
}


gint    
gimv_zlist_cell_index_from_xy (GimvZList *list, gint x, gint y)
{
   GimvScrolled *scr;
   gint row, column, cell_x, cell_y, index;

   scr = GIMV_SCROLLED(list);

   if (!list->cell_count || x < list->x_pad || y < list->y_pad)
      return -1;

   row    = CELL_ROW_FROM_Y(list, y);
   column = CELL_COL_FROM_X(list, x);

   if (row >= list->rows || column >= list->columns)
      return -1;

   cell_x = CELL_X_FROM_COL(list, column);
   if (x < list->cell_x_pad * 2 + cell_x)
      return -1;

   cell_y = CELL_Y_FROM_ROW(list, row);
   if (y < list->cell_y_pad * 2 + cell_y)
      return -1;

   index = list->flags & GIMV_ZLIST_HORIZONTAL ? column * list->rows + row : row * list->columns + column;  

   return index < list->cell_count ? index : -1;
}


void
gimv_zlist_set_1 (GimvZList *list, gint one)
{
   g_return_if_fail (list);

   if (one) 
      list->flags |= GIMV_ZLIST_1;
   else
      list->flags &= ~GIMV_ZLIST_1;

   gimv_zlist_update (list);
   WIDGET_DRAW (GTK_WIDGET (list));
}


gpointer
gimv_zlist_cell_from_xy (GimvZList *list, gint x, gint y)
{
   gint index;

   index = gimv_zlist_cell_index_from_xy (list, x, y);
   return index < 0 ? NULL : GIMV_ZLIST_CELL_FROM_INDEX (list, index); 
}


static void        
gimv_zlist_cell_pos (GimvZList *list, gint index, gint *row, gint *col)
{
   g_return_if_fail (list && index != -1 && row && col);

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      *row = index % list->rows;
      *col = index / list->rows;
   } else { 
      *row = index / list->columns;
      *col = index % list->columns;
   }
}


static void
gimv_zlist_cell_area (GimvZList *list, gint index, GdkRectangle *cell_area)
{
   gint row, col;
   g_return_if_fail (list && index != -1 && cell_area);

   gimv_zlist_cell_pos (list, index, &row, &col);

   cell_area->x      = CELL_X_FROM_COL(list, col) + list->cell_x_pad;
   cell_area->y      = CELL_Y_FROM_ROW(list, row) + list->cell_y_pad;
   cell_area->width  = list->cell_width - list->cell_x_pad;
   cell_area->height = list->cell_height - list->cell_y_pad;

}


void
gimv_zlist_draw_cell (GimvZList *list, gint index)
{
   GtkWidget *cell;
   GdkRectangle cell_area;

   g_return_if_fail (list && index != -1);

   if (!GTK_WIDGET_DRAWABLE(list))
      return;

   gimv_zlist_cell_area (list, index, &cell_area);
   cell = GIMV_ZLIST_CELL_FROM_INDEX (list, index);

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_DRAW], 0,
                  cell, &cell_area, &cell_area);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_DRAW],
                    cell, &cell_area, &cell_area);
#endif /* USE_GTK2 */

   if (index == list->focus)
      gimv_zlist_cell_draw_focus (list, index);
}


gint         
gimv_zlist_cell_index (GimvZList *list, gpointer cell)
{
   gint i;
   g_return_val_if_fail (list && cell, -1);

   for (i = 0; i < list->cell_count; i++) 
      if (GIMV_ZLIST_CELL_FROM_INDEX (list, i) == cell)
         return i;

   return -1;
}


gint
gimv_zlist_update_cell_size (GimvZList *list, gpointer cell)
{
   GtkRequisition requisition;
  
   g_return_val_if_fail (list && cell, FALSE);

   if (list->flags & GIMV_ZLIST_1)
      return FALSE;

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_SIZE_REQUEST], 0,
                  cell, &requisition);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_SIZE_REQUEST],
                    cell, &requisition);
#endif /* USE_GTK2 */

   if (requisition.width  + list->cell_x_pad > list->cell_width ||
       requisition.height + list->cell_y_pad > list->cell_height) {
      list->cell_width  = MAX(list->cell_width,  requisition.width  + list->cell_x_pad);
      list->cell_height = MAX(list->cell_height, requisition.height + list->cell_y_pad);

      gimv_zlist_update (list);
      WIDGET_DRAW (GTK_WIDGET (list));

      return TRUE;
   } 

   return FALSE;
}


/*
 *
 * Focus & Selection handling
 * 
 */

static void         
gimv_zlist_cell_draw_focus (GimvZList *list, gint index)
{
   GdkRectangle cell_area;
   g_return_if_fail (list && index != -1);

   gimv_zlist_cell_area (list, index, &cell_area);

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_DRAW_FOCUS], 0,
                  GIMV_ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_DRAW_FOCUS], 
                    GIMV_ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
#endif /* USE_GTK2 */
}


static void         
gimv_zlist_cell_draw_default (GimvZList *list, gint index)
{
   GdkRectangle cell_area;

   g_return_if_fail (list);
   if (index < 0) return;
   /* g_return_if_fail (index != -1); */

   gimv_zlist_cell_area (list, index, &cell_area);

#ifdef USE_GTK2
   g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_DRAW_DEFAULT], 0,
                  GIMV_ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
#else /* USE_GTK2 */
   gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_DRAW_DEFAULT], 
                    GIMV_ZLIST_CELL_FROM_INDEX (list, index), &cell_area);
#endif /* USE_GTK2 */
}


void        
gimv_zlist_cell_select (GimvZList *list, gint index)
{
   GList *node;

   g_return_if_fail (GIMV_IS_ZLIST (list) && index != -1);

   node = g_list_find (list->selection, GUINT_TO_POINTER(index));
   if (!node) {
#ifdef USE_GTK2
      g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_SELECT], 0, index);
#else /* USE_GTK2 */
      gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_SELECT], index);
#endif /* USE_GTK2 */
      list->selection = g_list_prepend (list->selection, GUINT_TO_POINTER(index));
      gimv_zlist_draw_cell (list, index);
   }
}


void
gimv_zlist_cell_unselect (GimvZList *list, gint index)
{
   GList *node;

   g_return_if_fail (GIMV_IS_ZLIST (list) && index != -1);

   node = g_list_find (list->selection, GUINT_TO_POINTER(index));
   if (node) {
#ifdef USE_GTK2
      g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_UNSELECT], 0, index);
#else /* USE_GTK2 */
      gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_UNSELECT], index);
#endif /* USE_GTK2 */
      list->selection = g_list_remove (list->selection, GUINT_TO_POINTER(index));
      gimv_zlist_draw_cell (list, index);
   }
}


void        
gimv_zlist_cell_toggle (GimvZList *list, gint index)
{
   g_return_if_fail (GIMV_IS_ZLIST (list) && index != -1);

   if (g_list_find (list->selection, GUINT_TO_POINTER(index)))
      gimv_zlist_cell_unselect (list, index);
   else
      gimv_zlist_cell_select (list, index);
}


void        
gimv_zlist_unselect_all (GimvZList *list)
{
   GList *item;

   item = list->selection;
   while (item) {
#ifdef USE_GTK2
      g_signal_emit (G_OBJECT(list), gimv_zlist_signals [CELL_UNSELECT], 0,
                     GPOINTER_TO_UINT(item->data));
#else /* USE_GTK2 */
      gtk_signal_emit (GTK_OBJECT(list), gimv_zlist_signals [CELL_UNSELECT], 
                       GPOINTER_TO_UINT(item->data));
#endif /* USE_GTK2 */
      gimv_zlist_draw_cell (list, GPOINTER_TO_UINT(item->data));

      item = item->next;
   }

   g_list_free (list->selection);
   list->selection = NULL;
}


void        
gimv_zlist_extend_selection (GimvZList *list, gint to)
{
   GList *item;
   gint i, s, e;

   g_return_if_fail (list && to != -1);

   if (list->anchor < to) {
      s = list->anchor;
      e = to;
   } else {
      s = to;
      e = list->anchor;
   }

   for (i = s; i <= e; i++) 
      /* XXX the state of the cell should be cached in the cells array */
      if (!g_list_find (list->selection, GUINT_TO_POINTER(i)))
         gimv_zlist_cell_select (list, i);
      else if (i == list->focus)
         gimv_zlist_cell_draw_focus (list, i);

   item = list->selection;
   while (item) {
      i = GPOINTER_TO_UINT(item->data);
      item = item->next;

      if (i < s || i > e)
         gimv_zlist_cell_unselect (list, i);    
   }
}


static void
select_each_cell_by_region (GimvZList *list, gint index, gboolean in_region)
{
   gboolean select = FALSE, selected = FALSE;
   gboolean is_mask = FALSE;

   g_return_if_fail (index < list->cell_count);

   if (g_list_find (list->selection, GUINT_TO_POINTER (index)))
      selected = TRUE;

   if (g_list_find (list->selection_mask, GUINT_TO_POINTER (index)))
      is_mask = TRUE;

   switch (list->region_select) {
   case GIMV_ZLIST_REGION_SELECT_TOGGLE:
      if ((in_region && !is_mask) || (!in_region && is_mask))
         select = TRUE;
      break;

   case GIMV_ZLIST_REGION_SELECT_EXPAND:
      if (in_region || is_mask)
         select = TRUE;
      break;

   case GIMV_ZLIST_REGION_SELECT_NORMAL:
      if (in_region)
         select = TRUE;
      break;
   default:
      break;
   }

   if (select && !selected)
      gimv_zlist_cell_select (list, index);
   if (!select && selected)
      gimv_zlist_cell_unselect (list, index);
}


void
gimv_zlist_cell_select_by_region (GimvZList *list,
                                  guint start_col, guint start_row,
                                  guint end_col, guint end_row)
{
   gint index = 0, i, j, cols, rows;
   guint col, row;

   g_return_if_fail (list);
   g_return_if_fail (GIMV_IS_ZLIST (list));
   g_return_if_fail (end_col >= start_col);
   g_return_if_fail (end_row >= start_row);

   if (list->flags & GIMV_ZLIST_HORIZONTAL) {
      cols = list->rows;
      rows = list->columns;
   } else {
      cols = list->columns;
      rows = list->rows;
   }

   for (i = 0; i < rows; i++) {
      for (j = 0; j < cols; j++) {
         gboolean in_region = FALSE;

         if (list->flags & GIMV_ZLIST_HORIZONTAL) {
            index = list->rows * i + j;
         } else {
            index = list->columns * i + j;
         }
         if (index >= list->cell_count) break;

         if (list->flags & GIMV_ZLIST_HORIZONTAL) {
            col = i;
            row = j;
         } else {
            col = j;
            row = i;
         }

         if (row >= start_row && row <= end_row && col >= start_col && col <= end_col)
            in_region = TRUE;

         select_each_cell_by_region (list, index, in_region);
      }

      if (index >= list->cell_count) break;
   }
}


void
gimv_zlist_cell_select_by_pixel_region (GimvZList *list,
                                        gint start_x, gint start_y,
                                        gint end_x, gint end_y)
{
   gint start_col, start_row, end_col, end_row, cell_x, cell_y;

   g_return_if_fail (list);
   g_return_if_fail (GIMV_IS_ZLIST (list));
   g_return_if_fail (end_x >= start_x);
   g_return_if_fail (end_y >= start_y);

   start_x = start_x - list->x_pad - bw (list);
   if (start_x < 0) start_x = 0;
   start_y = start_y - list->y_pad - bw (list);
   if (start_y < 0) start_y = 0;

   end_x = end_x - list->x_pad - bw (list);
   if (end_x < 0) end_x = 0;
   end_y = end_y - list->y_pad - bw (list);
   if (end_y < 0) end_y = 0;

   start_col = start_x / list->cell_width;
   start_row = start_y / list->cell_height;
   end_col   = end_x   / list->cell_width;
   end_row   = end_y   / list->cell_height;

   /*
   cell_x = CELL_X_FROM_COL(list, start_col);
   if (start_x  < list->cell_x_pad * 2 + cell_x && start_col > 1)
      start_col--;
      cell_y = CELL_Y_FROM_ROW(list, start_row);
   if (start_y < list->cell_y_pad * 2 + cell_y && start_row > 1)
      start_row--;
   */

   cell_x = CELL_X_FROM_COL(list, end_col);

   if (GIMV_SCROLLED_X (GIMV_SCROLLED (list), end_x) < list->cell_x_pad * 2 + cell_x
       && end_col > 0 && end_col > start_col)
   {
      end_col--;
   }

   cell_y = CELL_Y_FROM_ROW(list, end_row);

   if (GIMV_SCROLLED_Y (GIMV_SCROLLED (list), end_y) < list->cell_y_pad * 2 + cell_y
       && end_row > 0 && end_row > start_row)
   {
      end_row--;
   }

   gimv_zlist_cell_select_by_region (list,
                                     start_col, start_row,
                                     end_col, end_row);
}


void
gimv_zlist_set_selection_mask (GimvZList *list, GList *mask_list)
{
   g_return_if_fail (list);
   g_return_if_fail (GIMV_IS_ZLIST (list));

   if (mask_list) {
      list->selection_mask = mask_list;
   } else {
      if (list->selection)
         list->selection_mask = g_list_copy (list->selection);
      else
         list->selection_mask = NULL;
   }
}


void
gimv_zlist_unset_selection_mask (GimvZList *list)
{
   g_return_if_fail (list);
   g_return_if_fail (GIMV_IS_ZLIST (list));

   if (list->selection_mask)
      g_list_free (list->selection_mask);
   list->selection_mask = NULL;
}


void
gimv_zlist_moveto (GimvZList *list, gint index)
{
   GdkRectangle cell_area;
   GtkAdjustment *adj;

   g_return_if_fail (list && index != -1);

   if (!GTK_WIDGET_DRAWABLE(list) || GIMV_SCROLLED(list)->freeze_count)
      return;

   gimv_zlist_cell_area (list, index, &cell_area);

   if (list->flags & GIMV_ZLIST_HORIZONTAL) { /* horizontal list */

      adj = GIMV_SCROLLED(list)->h_adjustment;

      if (cell_area.x < 0) {
         adj->value += cell_area.x;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed");
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed");
#endif /* USE_GTK2 */
      }

      if (cell_area.x + cell_area.width > GTK_WIDGET(list)->allocation.width) {
         adj->value += (cell_area.x - adj->page_size) + cell_area.width;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed");
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed");
#endif /* USE_GTK2 */
      }

   } else { /* vertical list */

      adj = GIMV_SCROLLED(list)->v_adjustment;

      if (cell_area.y < 0) {
         adj->value += cell_area.y;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed");
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed");
#endif /* USE_GTK2 */
      }

      if (cell_area.y + cell_area.height > GTK_WIDGET(list)->allocation.height) {
         adj->value += (cell_area.y - adj->page_size) + cell_area.height;
#ifdef USE_GTK2
         g_signal_emit_by_name (G_OBJECT(adj), "value_changed");
#else /* USE_GTK2 */
         gtk_signal_emit_by_name (GTK_OBJECT(adj), "value_changed");
#endif /* USE_GTK2 */
      }
   }
}


void
gimv_zlist_cell_set_focus (GimvZList *list, gint index)
{
   g_return_if_fail (GIMV_IS_ZLIST (list));
   g_return_if_fail (index >= 0 && index < list->cell_count);

   list->focus = index;
   gimv_zlist_cell_draw_focus (list, list->focus);
}


void
gimv_zlist_cell_unset_focus (GimvZList *list)
{
   gint focus;

   g_return_if_fail (GIMV_IS_ZLIST (list));

   focus = list->focus;
   list->focus = -1;

   if (focus >= 0 && focus < list->cell_count)
      gimv_zlist_cell_draw_default (list, focus);
}
