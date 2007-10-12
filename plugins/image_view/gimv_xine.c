/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * Copyright (C) 2001-2002 the xine project
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
 *
 * the xine engine in a widget - implementation
 */

/*
 * based on gtkxine.c,v 1.52 2002/12/22 23:12:20
 * (http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/xine/gnome-xine/src/gtkxine.c)
 */

/* indent -kr -i3 -psl -pcs */

#include "gimv_xine.h"

#ifdef ENABLE_XINE

#include <xine.h>

#if (XINE_MAJOR_VERSION >= 1) && (XINE_MINOR_VERSION >= 0) && (XINE_SUB_VERSION >= 0)

#include "gimv_xine_priv.h"
#include "gimv_xine_post.h"


enum {
   PLAY_SIGNAL,
   STOP_SIGNAL,
   PLAYBACK_FINISHED_SIGNAL,
   NEED_NEXT_MRL_SIGNAL,
   BRANCHED_SIGNAL,
   LAST_SIGNAL
};


#if defined(GDK_WINDOWING_X11)
/* missing stuff from X includes */
#  ifndef XShmGetEventBase
extern int XShmGetEventBase (Display *);
#  endif
#endif /* defined(GDK_WINDOWING_X11) */

static void gimv_xine_class_init    (GimvXineClass  *klass);
static void gimv_xine_init          (GimvXine       *gxine);

/* object class methods */
static void gimv_xine_destroy       (GtkObject      *object);

/* widget class methods */
static void gimv_xine_realize       (GtkWidget      *widget);
static void gimv_xine_unrealize     (GtkWidget      *widget);
static gint gimv_xine_expose        (GtkWidget      *widget,
                                     GdkEventExpose *event);
static void gimv_xine_size_allocate (GtkWidget      *widget,
                                     GtkAllocation  *allocation);

static GtkWidgetClass *parent_class = NULL;
static gint gimv_xine_signals[LAST_SIGNAL] = {0};


GtkType
gimv_xine_get_type (void)
{
	static GtkType gimv_xine_type = 0;

   if (!gimv_xine_type) {
      static const GTypeInfo gimv_xine_info = {
         sizeof (GimvXineClass),
         NULL,               /* base_init */
         NULL,               /* base_finalize */
         (GClassInitFunc)    gimv_xine_class_init,
         NULL,               /* class_finalize */
         NULL,               /* class_data */
         sizeof (GimvXine),
         0,                  /* n_preallocs */
         (GInstanceInitFunc) gimv_xine_init,
      };

      gimv_xine_type = g_type_register_static (GTK_TYPE_WIDGET,
                                               "GimvXine",
                                               &gimv_xine_info,
                                               0);
   }

   return gimv_xine_type;
}


static void
gimv_xine_class_init (GimvXineClass *class)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;

   object_class = (GtkObjectClass *) class;
   widget_class = (GtkWidgetClass *) class;

   parent_class = gtk_type_class (gtk_widget_get_type ());

   gimv_xine_signals[PLAY_SIGNAL]
      = g_signal_new ("play",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvXineClass, play),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_xine_signals[STOP_SIGNAL]
      = g_signal_new ("stop",
                      G_SIGNAL_RUN_FIRST,
                      G_TYPE_FROM_CLASS (object_class),
                      G_STRUCT_OFFSET (GimvXineClass, stop),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_xine_signals[PLAYBACK_FINISHED_SIGNAL]
      = g_signal_new ("playback_finished",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvXineClass, playback_finished),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   /*
   gimv_xine_signals[NEED_NEXT_MRL_SIGNAL]
      = g_signal_new ("need_next_mrl",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvXineClass, need_next_mrl),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

   gimv_xine_signals[BRANCHED_SIGNAL]
      = g_signal_new ("branched",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvXineClass, branched),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
   */

   object_class->destroy       = gimv_xine_destroy;

   widget_class->realize       = gimv_xine_realize;
   widget_class->unrealize     = gimv_xine_unrealize;
   widget_class->size_allocate = gimv_xine_size_allocate;
   widget_class->expose_event  = gimv_xine_expose;
}


static void
gimv_xine_init (GimvXine *this)
{
   this->widget.requisition.width  = 8;
   this->widget.requisition.height = 8;

   this->private = gimv_xine_private_new ();
}


static void
gimv_xine_destroy (GtkObject *object)
{
   GimvXine *gtx = GIMV_XINE (object);

   g_return_if_fail (GIMV_IS_XINE (gtx));

   if (gtx->private) {
      gimv_xine_private_destroy (gtx->private);
      gtx->private = NULL;
   }

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


#if defined(GDK_WINDOWING_X11)
static void
dest_size_cb (void *gxine_gen,
              int video_width, int video_height,
              double video_pixel_aspect,
              int *dest_width, int *dest_height,
              double *dest_pixel_aspect)
{
   GimvXine *gxine = (GimvXine *) gxine_gen;
   GimvXinePrivate *priv;

   g_return_if_fail (GIMV_IS_XINE (gxine));
   priv = gxine->private;

   /* correct size with video_pixel_aspect */
   if (video_pixel_aspect >= priv->display_ratio)
      video_width =
         video_width * video_pixel_aspect / priv->display_ratio + .5;
   else
      video_height =
         video_height * priv->display_ratio / video_pixel_aspect + .5;

   *dest_width  = gxine->widget.allocation.width;
   *dest_height = gxine->widget.allocation.height;

   *dest_pixel_aspect = priv->display_ratio;
}
#endif /* GDK_WINDOWING_X11 */


static void
frame_output_cb (void *gxine_gen,
                 int video_width, int video_height,
                 double video_pixel_aspect,
                 int *dest_x, int *dest_y,
                 int *dest_width, int *dest_height,
                 double *dest_pixel_aspect,
                 int *win_x, int *win_y)
{
   GimvXine *gxine = (GimvXine *) gxine_gen;
   GimvXinePrivate *priv;

   g_return_if_fail (GIMV_IS_XINE (gxine));
   priv = gxine->private;

   /* correct size with video_pixel_aspect */
   if (video_pixel_aspect >= priv->display_ratio)
      video_width =
         video_width * video_pixel_aspect / priv->display_ratio + .5;
   else
      video_height =
         video_height * priv->display_ratio / video_pixel_aspect + .5;

   *dest_x = 0;
   *dest_y = 0;

   if (GTK_WIDGET_TOPLEVEL (&gxine->widget)) {
      gdk_window_get_position (gxine->widget.window, win_x, win_y);
   } else {
      GdkWindow *window;

      if (GTK_WIDGET_NO_WINDOW (&gxine->widget)) {
         window = gxine->widget.window;
      } else {
         window = gdk_window_get_parent (gxine->widget.window);
      }

      if (window)
         gdk_window_get_position (window, win_x, win_y);

      *win_x += gxine->widget.allocation.x;
      *win_y += gxine->widget.allocation.y;
   }

   *dest_width  = gxine->widget.allocation.width;
   *dest_height = gxine->widget.allocation.height;

   *dest_pixel_aspect = priv->display_ratio;
}


static xine_vo_driver_t *
load_video_out_driver (GimvXine *this)
{
#if defined(GDK_WINDOWING_X11)
   x11_visual_t vis;
   double res_h, res_v;
#elif defined(GDK_WINDOWING_FB)
   fb_visual_t  vis;
#endif /* defined(GDK_WINDOWING_X11) */

   GimvXinePrivate *priv;
   const char *video_driver_id = "auto";
   xine_vo_driver_t *vo_driver;

   g_return_val_if_fail (GIMV_IS_XINE (this), NULL);
   priv = this->private;

#if defined(GDK_WINDOWING_X11)
   vis.display = priv->display;
   vis.screen  = priv->screen;
   vis.d       = priv->video_window;
   res_h       = (DisplayWidth (priv->display, priv->screen) * 1000
                  / DisplayWidthMM (priv->display, priv->screen));
   res_v       = (DisplayHeight (priv->display, priv->screen) * 1000
                  / DisplayHeightMM (priv->display, priv->screen));
   priv->display_ratio = res_v / res_h;

   if (fabs (priv->display_ratio - 1.0) < 0.01) {
      priv->display_ratio = 1.0;
   }

   vis.dest_size_cb = dest_size_cb;
   vis.frame_output_cb = frame_output_cb;
   vis.user_data = this;
#elif defined(GDK_WINDOWING_FB)
   vis.frame_output_cb = frame_output_cb;
   vis.user_data = this;
   priv->display_ratio = 1.0;
#endif /* defined(GDK_WINDOWING_X11) */

   if (priv->video_driver_id)
      video_driver_id = priv->video_driver_id;
   else
      video_driver_id = gimv_xine_get_default_video_out_driver_id (this);

   if (strcmp (video_driver_id, "auto")) {
      vo_driver = xine_open_video_driver (priv->xine,
                                          video_driver_id,
                                          GIMV_XINE_DEFAULT_VISUAL_TYPE,
                                          (void *) &vis);
      if (vo_driver)
         return vo_driver;
      else
         g_print ("gtkxine: video driver %s failed.\n", video_driver_id);
   }

   return xine_open_video_driver (priv->xine, NULL,
                                  GIMV_XINE_DEFAULT_VISUAL_TYPE,
                                  (void *) &vis);
}


static xine_ao_driver_t *
load_audio_out_driver (GimvXine *this)
{
   GimvXinePrivate *priv;
   xine_ao_driver_t *ao_driver;
   const char *audio_driver_id = "auto";

   g_return_val_if_fail (GIMV_IS_XINE (this), NULL);
   priv = this->private;

   if (priv->audio_driver_id)
      audio_driver_id = priv->audio_driver_id;
   else
      audio_driver_id = gimv_xine_get_default_audio_out_driver_id (this);

   if (!strcmp (audio_driver_id, "null"))
      return NULL;

   if (strcmp (audio_driver_id, "auto")) {
      ao_driver =
         xine_open_audio_driver (priv->xine, audio_driver_id, NULL);
      if (ao_driver)
         return ao_driver;
      else
         g_print ("audio driver %s failed\n", audio_driver_id);
   }

   /* autoprobe */
   return xine_open_audio_driver (priv->xine, NULL, NULL);
}


#if defined (GDK_WINDOWING_X11)
static GdkFilterReturn
filter_xine_event(GdkXEvent *xevent, GdkEvent *gdkevent, gpointer data)
{
   XEvent *event = xevent;
   GimvXine *this = GIMV_XINE (data);
   GimvXinePrivate *priv;

   g_return_val_if_fail (GIMV_IS_XINE (this), GDK_FILTER_CONTINUE);
   priv = this->private;

   switch (event->type) {
   case Expose:
      if (event->xexpose.count != 0)
         break;

      xine_gui_send_vo_data (priv->stream,
                             XINE_GUI_SEND_EXPOSE_EVENT, &event);
      break;

   default:
      break;
   }

   if (event->type == priv->completion_event) {
      xine_gui_send_vo_data (priv->stream,
                             XINE_GUI_SEND_COMPLETION_EVENT, &event);
   }

   return GDK_FILTER_CONTINUE;
}
#endif /* defined (GDK_WINDOWING_X11) */



static void
event_listener (void *data, const xine_event_t * event)
{
    GimvXine *gtx = GIMV_XINE (data);

    g_return_if_fail (GIMV_IS_XINE (gtx));
    g_return_if_fail (event);

    switch (event->type)
    {
      case XINE_EVENT_UI_PLAYBACK_FINISHED:
	  g_signal_emit (G_OBJECT (gtx),
                    gimv_xine_signals[PLAYBACK_FINISHED_SIGNAL], 0);
	  break;

      default:
	  break;
    }
}


static void
gimv_xine_realize (GtkWidget * widget)
{
   GimvXine *this;
   GimvXinePrivate *priv;

   g_return_if_fail (widget);
   g_return_if_fail (GIMV_IS_XINE (widget));

   this = GIMV_XINE (widget);
   priv = this->private;

   /* set realized flag */

   GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

#if defined (GDK_WINDOWING_X11)
   /*
    * create our own video window
    */

   priv->video_window
      = XCreateSimpleWindow (gdk_display,
                             GDK_WINDOW_XWINDOW
                             (gtk_widget_get_parent_window (widget)),
                             0, 0,
                             widget->allocation.width,
                             widget->allocation.height, 1,
                             BlackPixel (gdk_display,
                                         DefaultScreen (gdk_display)),
                             BlackPixel (gdk_display,
                                         DefaultScreen (gdk_display)));

   widget->window = gdk_window_foreign_new (priv->video_window);

   if (!XInitThreads ()) {
      g_print ("gtkxine: XInitThreads failed - "
               "looks like you don't have a thread-safe xlib.\n");
      return;
   }

   priv->display = XOpenDisplay (NULL);

   if (!priv->display) {
      g_print ("gtkxine: XOpenDisplay failed!\n");
      return;
   }

   XLockDisplay (priv->display);

   priv->screen = DefaultScreen (priv->display);

   if (XShmQueryExtension (priv->display) == True) {
      priv->completion_event
         = XShmGetEventBase (priv->display) + ShmCompletion;
   } else {
      priv->completion_event = -1;
   }

   XSelectInput (priv->display, priv->video_window,
                 /* StructureNotifyMask | */ ExposureMask
                 /* | ButtonPressMask | PointerMotionMask */);
#else /* defined (GDK_WINDOWING_X11) */
{
   GdkWindowAttr attributes;
   gint attributes_mask;

   GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.x           = widget->allocation.x;
   attributes.y           = widget->allocation.y;
   attributes.width       = widget->allocation.width;
   attributes.height      = widget->allocation.height;
   attributes.wclass      = GDK_INPUT_OUTPUT;
   attributes.visual      = gtk_widget_get_visual (widget);
   attributes.colormap    = gtk_widget_get_colormap (widget);
   attributes.event_mask  = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

   widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                    &attributes, attributes_mask);
   gdk_window_set_user_data (widget->window, this);

   widget->style = gtk_style_attach (widget->style, widget->window);
   gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
   gdk_window_set_background (widget->window, &widget->style->black);
}
#endif /* defined (GDK_WINDOWING_X11) */

   /*
    * load audio, video drivers
    */

   priv->vo_driver = load_video_out_driver (this);

   if (!priv->vo_driver) {
      g_print ("gtkxine: couldn't open video driver\n");
      return;
   }

   priv->ao_driver = load_audio_out_driver (this);

   /*
    * create a stream object
    */

   priv->stream = xine_stream_new (priv->xine, priv->ao_driver,
                                   priv->vo_driver);

   priv->event_queue = xine_event_new_queue (priv->stream);
   xine_event_create_listener_thread (priv->event_queue, event_listener,
                                      this);


#if defined(GDK_WINDOWING_X11)
   XUnlockDisplay (priv->display);

   gdk_window_add_filter (NULL, filter_xine_event, this);
#endif /* defined(GDK_WINDOWING_X11) */

   post_init(this);

   return;
}


static void
gimv_xine_unrealize (GtkWidget *widget)
{
   GimvXine *this;
   GimvXinePrivate *priv;

   g_return_if_fail (widget);
   g_return_if_fail (GIMV_IS_XINE (widget));

   this = GIMV_XINE (widget);
   priv = this->private;

   /*
    * stop the playback 
    */
   gimv_xine_stop(this);
   xine_close (priv->stream);
   xine_event_dispose_queue (priv->event_queue);
   xine_dispose (priv->stream);
   priv->stream = NULL;
   xine_close_audio_driver(priv->xine, priv->ao_driver);
   xine_close_video_driver(priv->xine, priv->vo_driver);
   priv->ao_driver = NULL;
   priv->vo_driver = NULL;

#if defined (GDK_WINDOWING_X11)
   /* stop event thread */
   gdk_window_remove_filter (NULL, filter_xine_event, this);
#endif /* defined (GDK_WINDOWING_X11) */

   /* Hide all windows */
   if (GTK_WIDGET_MAPPED (widget))
      gtk_widget_unmap (widget);

   GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

   /* This destroys widget->window and unsets the realized flag */
   if (GTK_WIDGET_CLASS (parent_class)->unrealize)
      (*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


GtkWidget *
gimv_xine_new (const gchar *video_driver_id, const gchar *audio_driver_id)
{
   GtkWidget *this = GTK_WIDGET (g_object_new (gimv_xine_get_type (), NULL));
   GimvXinePrivate *priv;

   g_return_val_if_fail (GIMV_IS_XINE (this), NULL);
   priv = GIMV_XINE (this)->private;

   if (video_driver_id) {
      g_free (priv->video_driver_id);
      priv->video_driver_id = strdup (video_driver_id);
   }

   if (audio_driver_id) {
      g_free (priv->audio_driver_id);
      priv->audio_driver_id = strdup (audio_driver_id);
   }

   return this;
}


const char * const *
gimv_xine_get_video_out_plugins (GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return xine_list_video_output_plugins (gtx->private->xine);
}


const char * const *
gimv_xine_get_audio_out_plugins (GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return xine_list_audio_output_plugins (gtx->private->xine);
}


const gchar *
gimv_xine_get_video_out_driver_id(GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return gtx->private->video_driver_id;
}


const gchar *
gimv_xine_get_audio_out_driver_id(GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return gtx->private->audio_driver_id;
}


const gchar *
gimv_xine_get_default_video_out_driver_id(GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return xine_config_register_string (gtx->private->xine,
                                       "video.driver",
                                       "auto",
                                       "video driver to use",
                                       NULL, 10, NULL, NULL);
}


const gchar *
gimv_xine_get_default_audio_out_driver_id(GimvXine *gtx)
{
   g_return_val_if_fail(GIMV_IS_XINE (gtx), NULL);
   g_return_val_if_fail(gtx->private, NULL);

   return xine_config_register_string (gtx->private->xine,
                                       "audio.driver",
                                       "auto",
                                       "audio driver to use",
                                       NULL, 10, NULL, NULL);
}


static gint
gimv_xine_expose (GtkWidget *widget, GdkEventExpose *event)
{
   /*
	GimvXine *this = GIMV_XINE (widget);
	 */

   return TRUE;
}


static void
gimv_xine_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
   GimvXine *this;

   g_return_if_fail (widget);
   g_return_if_fail (GIMV_IS_XINE (widget));

   this = GIMV_XINE (widget);

   widget->allocation = *allocation;

   if (GTK_WIDGET_REALIZED (widget)) {
      gdk_window_move_resize (widget->window,
                              allocation->x,
                              allocation->y,
                              allocation->width, allocation->height);
   }
}


gint
gimv_xine_set_mrl (GimvXine *gtx, const gchar *mrl)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, FALSE);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), FALSE);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, FALSE);

   return xine_open (priv->stream, mrl);
}


static void
visual_anim_play(GimvXine *gtx)
{
   if(gtx->private->visual_anim.enabled == 2) {
      gtx->private->visual_anim.running = 1;
  }
}

static void
visual_anim_stop(GimvXine *gtx)
{
   if(gtx->private->visual_anim.enabled == 2) {
      xine_stop(gtx->private->visual_anim.stream);
      gtx->private->visual_anim.running = 0;
  }
}

gint
gimv_xine_play (GimvXine *gtx, gint pos, gint start_time)
{
   GimvXinePrivate *priv;
   gint retval;
   gboolean has_video;

   g_return_val_if_fail (gtx, -1);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), -1);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, -1);

#warning FIXME
   if(priv->visual_anim.post_changed
      && (xine_get_status (priv->stream) == XINE_STATUS_STOP))
   {
      post_rewire_visual_anim (gtx);
      priv->visual_anim.post_changed = 0;
   }

   has_video = xine_get_stream_info (priv->stream, XINE_STREAM_INFO_HAS_VIDEO);
   if (has_video)
      has_video = !xine_get_stream_info (priv->stream,
                                         XINE_STREAM_INFO_IGNORE_VIDEO);

   priv->visual_anim.enabled = 1;

   if ((has_video && priv->visual_anim.enabled == 1)
       && priv->visual_anim.running)
   {
      if (post_rewire_audio_port_to_stream(gtx, priv->stream))
         priv->visual_anim.running = 0;

   } else if (!has_video && (priv->visual_anim.enabled == 1)
              && (priv->visual_anim.running == 0)
              && priv->visual_anim.post_output)
   {
      if (post_rewire_audio_post_to_stream(gtx, priv->stream))
         priv->visual_anim.running = 1;

   } else if (has_video && priv->post_video && (priv->post_video_num > 0)) {
      post_rewire_video_post_to_stream(gtx, priv->stream);
   }

   retval = xine_play (priv->stream, pos, start_time);

   if (retval) {
      if(has_video) {
         if((priv->visual_anim.enabled == 2) && priv->visual_anim.running)
            visual_anim_stop(gtx);
      } else {
         if(!priv->visual_anim.running)
            visual_anim_play(gtx);
      }
      g_signal_emit (G_OBJECT(gtx),
                     gimv_xine_signals[PLAY_SIGNAL], 0);
   }

   return retval;
}


void
gimv_xine_set_speed (GimvXine *gtx, gint speed)
{
   gimv_xine_set_param (gtx, XINE_PARAM_SPEED, speed);
}


gint
gimv_xine_get_speed (GimvXine *gtx)
{
   return gimv_xine_get_param (gtx, XINE_PARAM_SPEED);
}


gint
gimv_xine_trick_mode (GimvXine *gtx, gint mode, gint value)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_trick_mode (priv->stream, mode, value);
}

static gint
gimv_xine_get_pos_length (GimvXine *gtx, gint *pos_stream,
                          gint *pos_time, gint *length_time)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_pos_length (priv->stream, pos_stream, pos_time,
                               length_time);
}


gint
gimv_xine_get_current_time (GimvXine *gtx)
{
   gint pos_stream = 0, pos_time = 0, length_time = 0;

   gimv_xine_get_pos_length (gtx, &pos_stream, &pos_time, &length_time);
   return pos_time;
}


gint
gimv_xine_get_stream_length (GimvXine *gtx)
{
   gint pos_stream = 0, pos_time = 0, length_time = 0;

   gimv_xine_get_pos_length (gtx, &pos_stream, &pos_time, &length_time);
   return length_time;
}


void
gimv_xine_stop (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->stream);

   xine_stop (priv->stream);

   g_signal_emit (G_OBJECT(gtx),
                  gimv_xine_signals[STOP_SIGNAL], 0);
}


gint
gimv_xine_get_error (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_error (priv->stream);
}


gint
gimv_xine_get_status (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_status (priv->stream);
}


gint
gimv_xine_is_playing (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   if (xine_get_status (priv->stream) == XINE_STATUS_PLAY)
      return TRUE;
   else
      return FALSE;
}


void
gimv_xine_set_param (GimvXine *gtx, gint param, gint value)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->stream);

   xine_set_param (priv->stream, param, value);
}


gint
gimv_xine_get_param (GimvXine *gtx, gint param)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_param (priv->stream, param);
}


gint
gimv_xine_get_audio_lang (GimvXine *gtx, gint channel, gchar *lang)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_audio_lang (priv->stream, channel, lang);
}


gint
gimv_xine_get_spu_lang (GimvXine *gtx, gint channel, gchar *lang)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_spu_lang (priv->stream, channel, lang);
}


gint
gimv_xine_get_stream_info (GimvXine *gtx, gint info)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_stream_info (priv->stream, info);
}


const gchar *
gimv_xine_get_meta_info (GimvXine *gtx, gint info)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_meta_info (priv->stream, info);
}


gint
gimv_xine_get_current_frame (GimvXine *gtx,
                             gint *width,
                             gint *height,
                             gint *ratio_code, gint *format,
                             uint8_t *img)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->stream, 0);

   return xine_get_current_frame (priv->stream, width, height, ratio_code,
                                  format, img);
}


guchar *
gimv_xine_get_current_frame_rgb (GimvXine *gtx,
                                 gint *width_ret,
                                 gint *height_ret)
{
   GimvXinePrivate *priv;
   gint err = 0;
   GimvXinePrivImage *image;
   guchar *rgb = NULL;
   gint width, height;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);
   g_return_val_if_fail (width_ret && height_ret, NULL);

   width  = xine_get_stream_info (priv->stream, XINE_STREAM_INFO_VIDEO_WIDTH);
   height = xine_get_stream_info (priv->stream, XINE_STREAM_INFO_VIDEO_HEIGHT);

   image = gimv_xine_priv_image_new (sizeof (guchar) * width * height * 2);

   err = xine_get_current_frame(priv->stream,
                                &image->width, &image->height,
                                &image->ratio_code,
                                &image->format,
                                image->img);

   if (err == 0) goto ERROR;

   /* the dxr3 driver does not allocate yuv buffers */
   /* image->u and image->v are always 0 for YUY2 */
   if (!image->img) goto ERROR;

   rgb = gimv_xine_priv_yuv2rgb (image);
   *width_ret  = image->width;
   *height_ret = image->height;

 ERROR:
   gimv_xine_priv_image_delete (image);
   return rgb;
}


gint
gimv_xine_get_log_section_count (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_get_log_section_count (priv->xine);
}


gchar **
gimv_xine_get_log_names (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar **) xine_get_log_names (priv->xine);
}

gchar **
gimv_xine_get_log (GimvXine *gtx, gint buf)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar **) xine_get_log (priv->xine, buf);
}


void
gimv_xine_register_log_cb (GimvXine *gtx, xine_log_cb_t cb, void *user_data)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->xine);

   return xine_register_log_cb (priv->xine, cb, user_data);
}

gchar **
gimv_xine_get_browsable_input_plugin_ids (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar **) xine_get_browsable_input_plugin_ids (priv->xine);
}


xine_mrl_t **
gimv_xine_get_browse_mrls (GimvXine *gtx,
                           const gchar *plugin_id,
                           const gchar *start_mrl, gint *num_mrls)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (xine_mrl_t **) xine_get_browse_mrls (priv->xine, plugin_id,
                                                start_mrl, num_mrls);
}


gchar **
gimv_xine_get_autoplay_input_plugin_ids (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar **) xine_get_autoplay_input_plugin_ids (priv->xine);
}


gchar **
gimv_xine_get_autoplay_mrls (GimvXine *gtx,
                             const gchar *plugin_id, gint *num_mrls)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar **) xine_get_autoplay_mrls (priv->xine, plugin_id,
                                             num_mrls);
}


gchar *
gimv_xine_get_file_extensions (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar *) xine_get_file_extensions (priv->xine);
}


gchar *
gimv_xine_get_mime_types (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar *) xine_get_mime_types (priv->xine);
}


const gchar *
gimv_xine_config_register_string (GimvXine *gtx,
                                  const gchar *key,
                                  const gchar *def_value,
                                  const gchar *description,
                                  const gchar *help,
                                  gint exp_level,
                                  xine_config_cb_t changed_cb,
                                  void *cb_data)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, NULL);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), NULL);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, NULL);

   return (gchar *) xine_config_register_string (priv->xine, key, def_value,
                                                 description, help,
                                                 exp_level, changed_cb,
                                                 cb_data);
}


gint
gimv_xine_config_register_range (GimvXine *gtx,
                                 const gchar *key,
                                 gint def_value,
                                 gint min, gint max,
                                 const gchar *description,
                                 const gchar *help,
                                 gint exp_level,
                                 xine_config_cb_t changed_cb, void *cb_data)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_register_range (priv->xine, key, def_value, min, max,
                                      description, help,
                                      exp_level, changed_cb, cb_data);
}


gint
gimv_xine_config_register_enum (GimvXine *gtx,
                                const gchar *key,
                                gint def_value,
                                gchar **values,
                                const gchar *description,
                                const gchar *help,
                                gint exp_level,
                                xine_config_cb_t changed_cb, void *cb_data)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_register_enum (priv->xine, key, def_value, values,
                                     description, help,
                                     exp_level, changed_cb, cb_data);
}


gint
gimv_xine_config_register_num (GimvXine *gtx,
                               const gchar *key,
                               gint def_value,
                               const gchar *description,
                               const gchar *help,
                               gint exp_level,
                               xine_config_cb_t changed_cb, void *cb_data)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_register_num (priv->xine, key, def_value,
                                    description, help,
                                    exp_level, changed_cb, cb_data);
}


gint
gimv_xine_config_register_bool (GimvXine *gtx,
                                const gchar *key,
                                gint def_value,
                                const gchar *description,
                                const gchar *help,
                                gint exp_level,
                                xine_config_cb_t changed_cb, void *cb_data)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_register_bool (priv->xine, key, def_value,
                                     description, help,
                                     exp_level, changed_cb, cb_data);
}


int
gimv_xine_config_get_first_entry (GimvXine *gtx, xine_cfg_entry_t *entry)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_get_first_entry (priv->xine, entry);
}


int
gimv_xine_config_get_next_entry (GimvXine *gtx, xine_cfg_entry_t *entry)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_get_next_entry (priv->xine, entry);
}


int
gimv_xine_config_lookup_entry (GimvXine *gtx,
                               const gchar *key, xine_cfg_entry_t *entry)
{
   GimvXinePrivate *priv;

   g_return_val_if_fail (gtx, 0);
   g_return_val_if_fail (GIMV_IS_XINE (gtx), 0);

   priv = gtx->private;
   g_return_val_if_fail (priv->xine, 0);

   return xine_config_lookup_entry (priv->xine, key, entry);
}


void
gimv_xine_config_update_entry (GimvXine *gtx, xine_cfg_entry_t *entry)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->xine);

   xine_config_update_entry (priv->xine, entry);
}


void
gimv_xine_config_load (GimvXine *gtx, const gchar *cfg_filename)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->xine);

   if (cfg_filename)
      xine_config_load (priv->xine, cfg_filename);
   else
      xine_config_load (priv->xine, priv->configfile);
}


void
gimv_xine_config_save (GimvXine *gtx, const gchar *cfg_filename)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->xine);

   if (cfg_filename)
      xine_config_save (priv->xine, cfg_filename);
   else {
      xine_config_save (priv->xine, priv->configfile);
   }
}


void
gimv_xine_config_reset (GimvXine *gtx)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->xine);

   xine_config_reset (priv->xine);
}


void
gimv_xine_event_send (GimvXine *gtx, const xine_event_t *event)
{
   GimvXinePrivate *priv;

   g_return_if_fail (gtx);
   g_return_if_fail (GIMV_IS_XINE (gtx));

   priv = gtx->private;
   g_return_if_fail (priv->stream);

   xine_event_send (priv->stream, event);
}

#endif /* (XINE_MAJOR_VERSION >= 1) && (XINE_MINOR_VERSION >= 0) && (XINE_SUB_VERSION >= 0) */
#endif /* ENABLE_XINE */
