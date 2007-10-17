/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
 * Copyright (C) 2003 Frank Fischer <frank_fischer@gmx.de>
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

#include <stdlib.h>
#include <string.h>

#include "gimageview.h"

#include "cursors.h"
#include "dnd.h"
#include "fileutil.h"
#include "gimv_image.h"
#include "gimv_anim.h"
#include "gimv_icon_stock.h"
#include "gimv_thumb.h"
#include "gimv_image_view.h"
#include "gimv_image_win.h"
#include "gimv_nav_win.h"
#include "gtkutils.h"
#include "menu.h"
#include "prefs.h"

#ifdef ENABLE_EXIF

#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/jpeg-data.h>

#endif


#define MIN_IMAGE_WIDTH  8
#define MIN_IMAGE_HEIGHT 8
#define IMAGE_VIEW_ENABLE_SCALABLE_LOAD 1


typedef enum {
   IMAGE_CHANGED_SIGNAL,
   LOAD_START_SIGNAL,
   LOAD_END_SIGNAL,
   SET_LIST_SIGNAL,
   UNSET_LIST_SIGNAL,
   RENDERED_SIGNAL,
   TOGGLE_ASPECT_SIGNAL,
   TOGGLE_BUFFER_SIGNAL,
   THUMBNAIL_CREATED_SIGNAL,

   IMAGE_PRESSED_SIGNAL,
   IMAGE_RELEASED_SIGNAL,
   IMAGE_CLICKED_SIGNAL,

   LAST_SIGNAL
} GimvImageViewSignalType;


typedef enum {
   MOVIE_STOP,
   MOVIE_PLAY,
   MOVIE_PAUSE,
   MOVIE_FORWARD,
   MOVIE_REVERSE,
   MOVIE_EJECT
} GimvImageViewMovieMenu;


typedef enum
{
   GimvImageViewSeekBarDraggingFlag   = 1 << 0
} GimvImageViewPlayerFlags;


typedef struct GimvImageViewImageList_Tag
{
   GList                    *list;
   GList                    *current;
   gpointer                  owner;
   GimvImageViewNextFn       next_fn;
   GimvImageViewPrevFn       prev_fn;
   GimvImageViewNthFn        nth_fn;
   GimvImageViewRemoveListFn remove_list_fn;
   gpointer                  list_fn_user_data;
} GimvImageViewImageList;


struct GimvImageViewPrivate_Tag
{
   GtkWidget       *navwin;

   /* image */
   GdkPixmap       *pixmap;
   GdkBitmap       *mask;

   /* image status */
   gint             x_pos;
   gint             y_pos;
   gint             width;
   gint             height;

   gfloat           x_scale;
   gfloat           y_scale;
   GimvImageViewOrientation rotate;

   gint             default_zoom;      /* should be defined as enum */
   gint             default_rotation;  /* should be defined as enum */
   GimvImageViewZoomType fit_to_frame;
   gboolean         keep_aspect;
   gboolean         ignore_alpha;
   gboolean         buffer;

   /* imageview status */
   gboolean         show_scrollbar;
   gboolean         continuance_play;

   /* player status */
   GimvImageViewPlayerFlags       player_flags;
   GimvImageViewPlayerVisibleType player_visible;

   /* image list and related functions */
   GimvImageViewImageList *image_list;

   /* information about image dragging */
   guint            button;
   gboolean         pressed;
   gboolean         dragging;
   gint             drag_startx;
   gint             drag_starty;
   gint             x_pos_drag_start;
   gint             y_pos_drag_start;

   /* */
   guint   load_end_signal_id;
   guint   loader_progress_update_signal_id;
   guint   loader_load_end_signal_id;

   GtkWindow *fullscreen;
};


/* object class methods */
static void gimv_image_view_dispose       (GObject *object);

/* image view class methods */
static void gimv_image_view_image_changed (GimvImageView *iv);

/* call back functions for reference popup menu */
static void cb_ignore_alpha                (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);
static void cb_keep_aspect                 (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);
static void cb_zoom                        (GimvImageView   *iv,
                                            GimvImageViewZoomType zoom,
                                            GtkWidget       *widget);
static void cb_rotate                      (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);
static void cb_toggle_scrollbar            (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);
static void cb_create_thumbnail            (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);
static void cb_toggle_buffer               (GimvImageView   *iv,
                                            guint            action,
                                            GtkWidget       *widget);

/* call back functions for player toolbar */
static void     cb_gimv_image_view_play   (GtkButton     *button,
                                           GimvImageView *iv);
static void     cb_gimv_image_view_stop   (GtkButton     *button,
                                           GimvImageView *iv);
static void     cb_gimv_image_view_fw     (GtkButton     *button,
                                           GimvImageView *iv);
static void     cb_gimv_image_view_rw     (GtkButton     *button,
                                           GimvImageView *iv);
static void     cb_gimv_image_view_eject  (GtkButton     *button,
                                           GimvImageView *iv);
static gboolean cb_seekbar_pressed  (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     GimvImageView  *iv);
static gboolean cb_seekbar_released (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     GimvImageView  *iv);

/* other call back functions */
static void     cb_destroy_loader          (GimvImageLoader   *loader,
                                            gpointer           data);
static void     cb_image_map               (GtkWidget         *widget,
                                            GimvImageView     *iv);
static gboolean cb_image_key_press         (GtkWidget         *widget,
                                            GdkEventKey       *event,
                                            GimvImageView     *iv);
static gboolean cb_image_button_press      (GtkWidget         *widget,
                                            GdkEventButton    *event,
                                            GimvImageView     *iv);
static gboolean cb_image_button_release    (GtkWidget         *widget, 
                                            GdkEventButton    *event,
                                            GimvImageView     *iv);
static gboolean cb_image_motion_notify     (GtkWidget         *widget, 
                                            GdkEventMotion    *event,
                                            GimvImageView     *iv);
static void     cb_scrollbar_value_changed (GtkAdjustment     *adj,
                                            GimvImageView     *iv);
static gboolean cb_nav_button              (GtkWidget         *widget,
                                            GdkEventButton    *event,
                                            GimvImageView     *iv);

/* callback functions for movie menu */
static void cb_movie_menu                  (GimvImageView   *iv,
                                            GimvImageViewMovieMenu operation,
                                            GtkWidget       *widget);
static void cb_movie_menu_destroy          (GtkWidget       *widget,
                                            GimvImageView   *iv);
static void cb_view_modes_menu_destroy     (GtkWidget       *widget,
                                            GimvImageView   *iv);
static void cb_continuance                 (GimvImageView   *iv,
                                            guint            active,
                                            GtkWidget       *widget);
static void cb_change_view_mode            (GtkWidget       *widget,
                                            GimvImageView   *iv);

/* other private functions */
static void allocate_draw_buffer             (GimvImageView   *iv);
static void gimv_image_view_calc_image_size  (GimvImageView   *iv);
static void gimv_image_view_rotate_render    (GimvImageView   *iv,
                                              guint            action);

static void gimv_image_view_change_draw_widget     (GimvImageView     *iv,
                                                    const gchar       *label);
static void gimv_image_view_adjust_pos_in_frame    (GimvImageView     *iv,
                                                    gboolean           center);
static gint idle_gimv_image_view_change_image_info (gpointer           data);
static void gimv_image_view_get_request_size       (GimvImageView     *iv,
                                                    gint              *width_ret,
                                                    gint              *height_ret);

static GtkWidget *gimv_image_view_create_player_toolbar (GimvImageView *iv);


static gint gimv_image_view_signals[LAST_SIGNAL] = {0};


extern GimvImageViewPlugin imageview_draw_vfunc_table;

GList *draw_area_list = NULL;


/* reference menu items */
GtkItemFactoryEntry gimv_image_view_popup_items [] =
{
   {N_("/tear"),                  NULL,         NULL,                0, "<Tearoff>"},
   {N_("/_Zoom"),                 NULL,         NULL,                0, "<Branch>"},
   {N_("/_Rotate"),               NULL,         NULL,                0, "<Branch>"},
   {N_("/Ignore _Alpha Channel"), NULL,         cb_ignore_alpha,     0, "<ToggleItem>"},
   {N_("/---"),                   NULL,         NULL,                0, "<Separator>"},
   {N_("/M_ovie"),                NULL,         NULL,                0, "<Branch>"},
   {N_("/---"),                   NULL,         NULL,                0, "<Separator>"},
   {N_("/_View modes"),           NULL,         NULL,                0, "<Branch>"},
   {N_("/Show _Scrollbar"),       "<shift>S",   cb_toggle_scrollbar, 0, "<ToggleItem>"},
   {N_("/---"),                   NULL,         NULL,                0, "<Separator>"},
   {N_("/Create _Thumbnail"),     "<shift>T",   cb_create_thumbnail, 0, NULL},
   {N_("/Memory _Buffer"),        "<control>B", cb_toggle_buffer,    0, "<ToggleItem>"},
   {NULL, NULL, NULL, 0, NULL},
};


/* for "Zoom" sub menu */
GtkItemFactoryEntry gimv_image_view_zoom_items [] =
{
   {N_("/tear"),                NULL,        NULL,            0,           "<Tearoff>"},
   {N_("/Zoom _In"),            "S",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_IN,     NULL},
   {N_("/Zoom _Out"),           "A",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_OUT,    NULL},
   {N_("/_Fit to Window"),      "W",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_FIT,    NULL},
   {N_("/_Fit _Width"),         "<shift>W",  cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_FIT_WIDTH, NULL},
   {N_("/_Fit _Height"),        "<shift>H",  cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_FIT_HEIGHT,NULL},
   {N_("/Keep _aspect ratio"),  "<shift>A",  cb_keep_aspect,  0,           "<ToggleItem>"},
   {N_("/---"),                 NULL,        NULL,            0,           "<Separator>"},
   {N_("/10%(_1)"),             "1",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_10,     NULL},
   {N_("/25%(_2)"),             "2",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_25,     NULL},
   {N_("/50%(_3)"),             "3",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_50,     NULL},
   {N_("/75%(_4)"),             "4",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_75,     NULL},
   {N_("/100%(_5)"),            "5",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_100,    NULL},
   {N_("/125%(_6)"),            "6",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_125,    NULL},
   {N_("/150%(_7)"),            "7",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_150,    NULL},
   {N_("/175%(_8)"),            "8",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_175,    NULL},
   {N_("/200%(_9)"),            "9",         cb_zoom,         GIMV_IMAGE_VIEW_ZOOM_200,    NULL},
   {NULL, NULL, NULL, 0, NULL},
};


/* for "Rotate" sub menu */
GtkItemFactoryEntry gimv_image_view_rotate_items [] =
{
   {N_("/tear"),            NULL,  NULL,       0,           "<Tearoff>"},
   {N_("/Rotate 90 degrees CW"),  "R",   cb_rotate,  GIMV_IMAGE_VIEW_ROTATE_270,  NULL},
   {N_("/Rotate 90 degrees CCW"), "E",   cb_rotate,  GIMV_IMAGE_VIEW_ROTATE_90,   NULL},
   {N_("/Rotate 180 degrees"),    "D",   cb_rotate,  GIMV_IMAGE_VIEW_ROTATE_180,  NULL},
   {NULL, NULL, NULL, 0, NULL},
};


/* for "Movie" sub menu */
GtkItemFactoryEntry gimv_image_view_playable_items [] =
{
   {N_("/tear"),             NULL,  NULL,           0,             "<Tearoff>"},
   {N_("/_Play"),            NULL,  cb_movie_menu,  MOVIE_PLAY,    NULL},
   {N_("/_Stop"),            NULL,  cb_movie_menu,  MOVIE_STOP,    NULL},
   {N_("/P_ause"),           NULL,  cb_movie_menu,  MOVIE_PAUSE,   NULL},
   {N_("/_Forward"),         NULL,  cb_movie_menu,  MOVIE_FORWARD, NULL},
   {N_("/_Reverse"),         NULL,  cb_movie_menu,  MOVIE_REVERSE, NULL},
   {N_("/---"),              NULL,  NULL,           0,             "<Separator>"},
   {N_("/_Continuance"),     NULL,  cb_continuance, 0,             "<ToggleItem>"},
   {N_("/---"),              NULL,  NULL,           0,             "<Separator>"},
   {N_("/_Eject"),           NULL,  cb_movie_menu,  MOVIE_EJECT, NULL},
   {NULL, NULL, NULL, 0, NULL},
};


static GList *GimvImageViewList = NULL;
static GdkPixmap *buffer = NULL;
static gboolean move_scrollbar_by_user = TRUE;


/****************************************************************************
 *
 *  Plugin Management
 *
 ****************************************************************************/
static gint
comp_func_priority (GimvImageViewPlugin *plugin1,
                    GimvImageViewPlugin *plugin2)
{
   g_return_val_if_fail (plugin1, 1);
   g_return_val_if_fail (plugin2, -1);

   return plugin1->priority_hint - plugin2->priority_hint;
}

gboolean
gimv_image_view_plugin_regist (const gchar *plugin_name,
                               const gchar *module_name,
                               gpointer impl,
                               gint     size)
{
   GimvImageViewPlugin *plugin = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (plugin, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (plugin->if_version == GIMV_IMAGE_VIEW_IF_VERSION, FALSE);
   g_return_val_if_fail (plugin->label, FALSE);

   if (!draw_area_list)
      draw_area_list = g_list_append (draw_area_list,
                                      &imageview_draw_vfunc_table);

   draw_area_list = g_list_append (draw_area_list, plugin);
   draw_area_list = g_list_sort (draw_area_list,
                                 (GCompareFunc) comp_func_priority);

   return TRUE;
}


GList *
gimv_image_view_plugin_get_list (void)
{
   if (!draw_area_list)
      draw_area_list = g_list_append (draw_area_list,
                                      &imageview_draw_vfunc_table);
   return draw_area_list;
}


/****************************************************************************
 *
 *
 *
 ****************************************************************************/
G_DEFINE_TYPE (GimvImageView, gimv_image_view, GTK_TYPE_VBOX)

enum {
  ARG_0,
  ARG_X_SCALE,
  ARG_Y_SCALE,
  ARG_ORIENTATION,
  ARG_DEFAULT_ZOOM,
  ARG_DEFAULT_ROTATION,
  ARG_KEEP_ASPECT,
  ARG_KEEP_BUFFER,
  ARG_IGNORE_ALPHA,
  ARG_SHOW_SCROLLBAR,
  ARG_CONTINUANCE_PLAY,
};


/* FIXME!! */
static void
gimv_image_view_set_arg (GtkObject *object,
                         GtkArg    *arg,
                         guint      arg_id)
{
   GimvImageView *iv = GIMV_IMAGE_VIEW(object);

   switch (arg_id) {
   case ARG_X_SCALE:
      iv->priv->x_scale = GTK_VALUE_FLOAT (*arg);
      break;
   case ARG_Y_SCALE:
      iv->priv->y_scale = GTK_VALUE_FLOAT (*arg);
      break;
   case ARG_ORIENTATION:
      iv->priv->rotate = GTK_VALUE_INT (*arg);
      break;
   case ARG_DEFAULT_ZOOM:
      iv->priv->default_zoom = GTK_VALUE_INT (*arg);
      break;
   case ARG_DEFAULT_ROTATION:
      iv->priv->default_rotation = GTK_VALUE_INT (*arg);
      break;
   case ARG_KEEP_ASPECT:
      iv->priv->keep_aspect = GTK_VALUE_BOOL (*arg);
      break;
   case ARG_KEEP_BUFFER:
      iv->priv->buffer = GTK_VALUE_BOOL (*arg);
      break;
   case ARG_IGNORE_ALPHA:
      iv->priv->ignore_alpha = GTK_VALUE_BOOL (*arg);
      break;
   case ARG_SHOW_SCROLLBAR:
      iv->priv->show_scrollbar = GTK_VALUE_BOOL (*arg);
      break;
   case ARG_CONTINUANCE_PLAY:
      iv->priv->continuance_play = GTK_VALUE_BOOL (*arg);
      break;
   default:
      break;
   }
}

static void
gimv_image_view_get_arg (GtkObject *object,
                         GtkArg    *arg,
                         guint      arg_id)
{
   GimvImageView *iv = GIMV_IMAGE_VIEW(object);

   switch (arg_id) {
   case ARG_X_SCALE:
      GTK_VALUE_FLOAT (*arg) = iv->priv->x_scale;
      break;
   case ARG_Y_SCALE:
      GTK_VALUE_FLOAT (*arg) = iv->priv->y_scale;
      break;
   case ARG_ORIENTATION:
      GTK_VALUE_INT (*arg) = iv->priv->rotate;
      break;
   case ARG_DEFAULT_ZOOM:
      GTK_VALUE_INT (*arg) = iv->priv->default_zoom;
      break;
   case ARG_DEFAULT_ROTATION:
      GTK_VALUE_INT (*arg) = iv->priv->default_rotation;
      break;
   case ARG_KEEP_ASPECT:
      GTK_VALUE_BOOL (*arg) = iv->priv->keep_aspect;
      break;
   case ARG_KEEP_BUFFER:
      GTK_VALUE_BOOL (*arg) = iv->priv->buffer;
      break;
   case ARG_IGNORE_ALPHA:
      GTK_VALUE_BOOL (*arg) = iv->priv->ignore_alpha;
      break;
   case ARG_SHOW_SCROLLBAR:
      GTK_VALUE_BOOL (*arg) = iv->priv->show_scrollbar;
      break;
   case ARG_CONTINUANCE_PLAY:
      GTK_VALUE_BOOL (*arg) = iv->priv->continuance_play;
      break;
   default:
      arg->type = GTK_TYPE_INVALID;
      break;
   }
}


static void
gimv_image_view_class_init (GimvImageViewClass *klass)
{
   GObjectClass *gobject_class;
   GtkObjectClass *object_class;

   gobject_class = (GObjectClass *) klass;
   object_class = (GtkObjectClass *) klass;

   gtk_object_add_arg_type ("GimvImageView::x_scale",
                            GTK_TYPE_FLOAT,
                            GTK_ARG_READWRITE,
                            ARG_X_SCALE);
   gtk_object_add_arg_type ("GimvImageView::y_scale",
                            GTK_TYPE_FLOAT,
                            GTK_ARG_READWRITE,
                            ARG_Y_SCALE);
   gtk_object_add_arg_type ("GimvImageView::orientation",
                            GTK_TYPE_INT,
                            GTK_ARG_READWRITE,
                            ARG_ORIENTATION);
   gtk_object_add_arg_type ("GimvImageView::default_zoom",
                            GTK_TYPE_INT,
                            GTK_ARG_READWRITE,
                            ARG_DEFAULT_ZOOM);
   gtk_object_add_arg_type ("GimvImageView::default_rotation",
                            GTK_TYPE_INT,
                            GTK_ARG_READWRITE,
                            ARG_DEFAULT_ROTATION);
   gtk_object_add_arg_type ("GimvImageView::keep_aspect",
                            GTK_TYPE_BOOL,
                            GTK_ARG_READWRITE,
                            ARG_KEEP_ASPECT);
   gtk_object_add_arg_type ("GimvImageView::keep_buffer",
                            GTK_TYPE_BOOL,
                            GTK_ARG_READWRITE,
                            ARG_KEEP_BUFFER);
   gtk_object_add_arg_type ("GimvImageView::ignore_alpha",
                            GTK_TYPE_BOOL,
                            GTK_ARG_READWRITE,
                            ARG_IGNORE_ALPHA);
   gtk_object_add_arg_type ("GimvImageView::show_scrollbar",
                            GTK_TYPE_BOOL,
                            GTK_ARG_READWRITE,
                            ARG_SHOW_SCROLLBAR);
   gtk_object_add_arg_type ("GimvImageView::continuance_play",
                            GTK_TYPE_BOOL,
                            GTK_ARG_READWRITE,
                            ARG_CONTINUANCE_PLAY);

   gimv_image_view_signals[IMAGE_CHANGED_SIGNAL]
      = g_signal_new ("image_changed",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, image_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_image_view_signals[LOAD_START_SIGNAL]
      = g_signal_new ("load_start",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, load_start),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

   gimv_image_view_signals[LOAD_END_SIGNAL]
      = g_signal_new ("load_end",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, load_end),
                      NULL, NULL,
                      gtk_marshal_NONE__POINTER_INT,
                      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

   gimv_image_view_signals[SET_LIST_SIGNAL]
      = g_signal_new ("set_list",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, set_list),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_image_view_signals[UNSET_LIST_SIGNAL]
      = g_signal_new ("unset_list",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, unset_list),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_image_view_signals[RENDERED_SIGNAL]
      = g_signal_new ("rendered",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, rendered),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_image_view_signals[TOGGLE_ASPECT_SIGNAL]
      = g_signal_new ("toggle_aspect",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, toggle_aspect),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

   gimv_image_view_signals[TOGGLE_BUFFER_SIGNAL]
      = g_signal_new ("toggle_buffer",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, toggle_buffer),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

   gimv_image_view_signals[THUMBNAIL_CREATED_SIGNAL]
      = g_signal_new ("thumbnail_created",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvImageViewClass, thumbnail_created),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

   gimv_image_view_signals[IMAGE_PRESSED_SIGNAL]
      = g_signal_new ("image_pressed",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GimvImageViewClass, image_pressed),
                      NULL, NULL,
                      gtk_marshal_BOOL__POINTER,
                      G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);

   gimv_image_view_signals[IMAGE_RELEASED_SIGNAL]
      = g_signal_new ("image_released",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GimvImageViewClass, image_released),
                      NULL, NULL,
                      gtk_marshal_BOOL__POINTER,
                      G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);

   gimv_image_view_signals[IMAGE_CLICKED_SIGNAL]
      = g_signal_new ("image_clicked",
                      G_TYPE_FROM_CLASS(gobject_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (GimvImageViewClass, image_clicked),
                      NULL, NULL,
                      gtk_marshal_BOOL__POINTER,
                      G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);

   gobject_class->dispose   = gimv_image_view_dispose;

   object_class->set_arg    = gimv_image_view_set_arg;
   object_class->get_arg    = gimv_image_view_get_arg;

   klass->image_changed     = gimv_image_view_image_changed;
   klass->load_start        = NULL;
   klass->load_end          = NULL;
   klass->set_list          = NULL;
   klass->unset_list        = NULL;
   klass->rendered          = NULL;
   klass->toggle_aspect     = NULL;
   klass->toggle_buffer     = NULL;
   klass->thumbnail_created = NULL;
}


static void
gimv_image_view_init (GimvImageView *iv)
{
   GtkWidget *player, *event_box;

   /* initialize struct data based on config */
   iv->loader     = NULL;
   iv->image      = NULL;

   iv->hadj = iv->vadj  = NULL;

   iv->draw_area        = NULL;
   iv->draw_area_funcs  = NULL;

   iv->progressbar      = NULL;

   iv->bg_color         = NULL;

   iv->cursor           = NULL;
   iv->imageview_popup  = NULL;
   iv->zoom_menu        = NULL;
   iv->rotate_menu      = NULL;
   iv->movie_menu       = NULL;
   iv->view_modes_menu  = NULL;

   /* init private data */
   iv->priv = g_new0(GimvImageViewPrivate, 1);

   iv->priv->navwin     = NULL;

   iv->priv->pixmap     = NULL;
   iv->priv->mask       = NULL;

   iv->priv->x_pos      = 0;
   iv->priv->y_pos      = 0;
   iv->priv->width      = 0;
   iv->priv->height     = 0;

   iv->priv->show_scrollbar   = FALSE;
   iv->priv->default_zoom     = conf.imgview_default_zoom;
   iv->priv->default_rotation = conf.imgview_default_rotation;
   iv->priv->fit_to_frame     = 0;

   iv->priv->x_scale          = conf.imgview_scale;
   iv->priv->y_scale          = conf.imgview_scale;
   iv->priv->rotate     = 0;
   iv->priv->keep_aspect      = conf.imgview_keep_aspect;
   iv->priv->ignore_alpha     = FALSE;

   iv->priv->buffer           = conf.imgview_buffer;

   iv->priv->show_scrollbar   = conf.imgview_scrollbar;
   iv->priv->continuance_play = conf.imgview_movie_continuance;

   iv->priv->player_flags     = 0;
   iv->priv->player_visible   = GimvImageViewPlayerVisibleAuto;

   /* list */
   iv->priv->image_list = NULL;

   iv->priv->load_end_signal_id               = 0;
   iv->priv->loader_progress_update_signal_id = 0;
   iv->priv->loader_load_end_signal_id        = 0;

   iv->priv->fullscreen = NULL;

   /* create widgets */
   iv->loader = gimv_image_loader_new ();

   iv->table = gtk_table_new (2, 2, FALSE);
   gtk_widget_show (iv->table);
   gtk_box_pack_start (GTK_BOX (iv), iv->table, TRUE, TRUE, 0);

   player = gimv_image_view_create_player_toolbar (iv);
   gtk_box_pack_start (GTK_BOX (iv), player, FALSE, FALSE, 2);

   gimv_image_view_change_draw_widget (iv, 0);

   iv->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 10.0, 10.0, 0.0));
   iv->hscrollbar = gtk_hscrollbar_new (iv->hadj);
   gtk_widget_show (iv->hscrollbar);

   iv->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 10.0, 10.0, 0.0));
   iv->vscrollbar = gtk_vscrollbar_new (iv->vadj);
   gtk_widget_show (iv->vscrollbar);

   event_box = gtk_event_box_new ();
   gtk_widget_set_name (event_box, "NavWinButton");
   gtk_widget_show (event_box);

   iv->nav_button = gimv_icon_stock_get_widget ("nav-button");
   gtk_container_add (GTK_CONTAINER (event_box), iv->nav_button);
   gtk_widget_show (iv->nav_button);

   gtk_table_attach (GTK_TABLE (iv->table), iv->vscrollbar,
                     1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
   gtk_table_attach (GTK_TABLE (iv->table), iv->hscrollbar,
                     0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
   gtk_table_attach (GTK_TABLE (iv->table), event_box,
                     1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

   /* set signals */
   g_signal_connect (G_OBJECT (iv->hadj), "value_changed",
                     G_CALLBACK (cb_scrollbar_value_changed), iv);

   g_signal_connect (G_OBJECT (iv->vadj), "value_changed",
                     G_CALLBACK (cb_scrollbar_value_changed), iv);

   g_signal_connect (G_OBJECT (event_box), "button_press_event",
                     G_CALLBACK (cb_nav_button), iv);

   /* add to list */
   GimvImageViewList = g_list_append (GimvImageViewList, iv);

   if (!iv->priv->show_scrollbar) {
      gimv_image_view_hide_scrollbar (iv);
   }
}


GtkWidget*
gimv_image_view_new (GimvImageInfo *info)
{
   GimvImageView *iv = g_object_new (GIMV_TYPE_IMAGE_VIEW, NULL);

   if (info) {
      iv->info = gimv_image_info_ref (info);
   } else {
      iv->info = NULL;
   }

   /* load image */
   if (iv->info)
      gtk_idle_add (idle_gimv_image_view_change_image_info, iv);

   return GTK_WIDGET (iv);
}


static void
gimv_image_view_dispose (GObject *object)
{
   GimvImageView *iv = GIMV_IMAGE_VIEW (object);

   if (GTK_IS_WIDGET (iv->draw_area)) {
      gtk_widget_destroy (iv->draw_area);
   }
   iv->draw_area = NULL;

   if (iv->loader) {
      if (gimv_image_loader_is_loading (iv->loader)) {
         gimv_image_view_cancel_loading (iv);
         g_signal_connect (G_OBJECT (iv->loader), "load_end",
                           G_CALLBACK (cb_destroy_loader),
                           iv);
      } else {
         g_object_unref (G_OBJECT(iv->loader));
      }
   }
   iv->loader = NULL;

   if (iv->image)
      gimv_image_unref (iv->image);
   iv->image = NULL;

   if (iv->bg_color)
      g_free (iv->bg_color);
   iv->bg_color = NULL;

   if (iv->cursor)
      gdk_cursor_destroy (iv->cursor);
   iv->cursor = NULL;

   if (iv->info)
      gimv_image_info_unref (iv->info);
   iv->info = NULL;

   if (iv->priv) {
      if (iv->priv->navwin) {
         gtk_widget_destroy (iv->priv->navwin);
         iv->priv->navwin = NULL;
      }

      if (iv->priv->pixmap) {
         gimv_image_free_pixmap_and_mask (iv->priv->pixmap, iv->priv->mask);
         iv->priv->pixmap = NULL;
         iv->priv->mask = NULL;
      }

      if (iv->priv->image_list)
         gimv_image_view_remove_list (iv, iv->priv->image_list->owner);
      iv->priv->image_list = NULL;

      g_free(iv->priv);
      iv->priv = NULL;
   }

   if (G_OBJECT_CLASS (gimv_image_view_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_image_view_parent_class)->dispose (object);
}


static void
gimv_image_view_image_changed (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs) return;

   playable = iv->draw_area_funcs->playable;
   if (!playable) return;
   if (!playable->is_playable_fn) return;

   if (gimv_image_view_is_playable (iv)) {
      GimvImageViewPlayableStatus status;

      if (iv->priv->player_visible == GimvImageViewPlayerVisibleAuto)
         gtk_widget_show (iv->player_container);
      status = gimv_image_view_playable_get_status (iv);
      gimv_image_view_playable_set_status (iv, status);

   } else {
      if (iv->priv->player_visible == GimvImageViewPlayerVisibleAuto)
         gtk_widget_hide (iv->player_container);
      gimv_image_view_playable_set_status (iv, GimvImageViewPlayableDisable);
   }
}



/*****************************************************************************
 *
 *   Callback functions for reference popup menu.
 *
 *****************************************************************************/
static void
cb_ignore_alpha (GimvImageView *iv, guint action, GtkWidget *widget)
{
   iv->priv->ignore_alpha = GTK_CHECK_MENU_ITEM(widget)->active;
   gimv_image_view_show_image (iv);
}


static void
cb_keep_aspect (GimvImageView *iv, guint action, GtkWidget *widget)
{
   iv->priv->keep_aspect = GTK_CHECK_MENU_ITEM(widget)->active;

   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[TOGGLE_ASPECT_SIGNAL], 0,
                  iv->priv->keep_aspect);
}


static void
cb_zoom (GimvImageView *iv, GimvImageViewZoomType zoom, GtkWidget *widget)
{
   gimv_image_view_zoom_image (iv, zoom, 0, 0);
}


static void
cb_rotate (GimvImageView *iv, GimvImageViewOrientation rotate, GtkWidget *widget)
{
   guint angle;

   /* convert to absolute angle */
   angle = (iv->priv->rotate + rotate) % 4;

   gimv_image_view_rotate_image (iv, angle);
}


static void
cb_toggle_scrollbar (GimvImageView *iv, guint action, GtkWidget *widget)
{
   if (GTK_CHECK_MENU_ITEM(widget)->active) {
      gimv_image_view_show_scrollbar (iv);
   } else {
      gimv_image_view_hide_scrollbar (iv);
   }
}


static void
cb_create_thumbnail (GimvImageView *iv, guint action, GtkWidget *widget)
{
   g_return_if_fail (iv);

   gimv_image_view_create_thumbnail (iv);
}


static void
cb_toggle_buffer (GimvImageView *iv, guint action, GtkWidget *widget)
{
   if (GTK_CHECK_MENU_ITEM(widget)->active) {
      gimv_image_view_load_image_buf (iv);
      iv->priv->buffer = TRUE;
   }
   else {
      iv->priv->buffer = FALSE;
      gimv_image_view_free_image_buf (iv);
   }

   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[TOGGLE_BUFFER_SIGNAL], 0,
                  iv->priv->buffer);
}



/*****************************************************************************
 *
 *   callback functions for movie menu.
 *
 *****************************************************************************/
static void
cb_movie_menu (GimvImageView *iv,
               GimvImageViewMovieMenu operation,
               GtkWidget *widget)
{
   g_return_if_fail (iv);

   switch (operation) {
   case MOVIE_PLAY:
      gimv_image_view_playable_play (iv);
      break;
   case MOVIE_STOP:
      gimv_image_view_playable_stop (iv);
      break;
   case MOVIE_PAUSE:
      gimv_image_view_playable_pause (iv);
      break;
   case MOVIE_FORWARD:
      gimv_image_view_playable_forward (iv);
      break;
   case MOVIE_REVERSE:
      gimv_image_view_playable_reverse (iv);
      break;
   case MOVIE_EJECT:
      gimv_image_view_playable_eject (iv);
      break;
   default:
      break;
   }
}


static void
cb_movie_menu_destroy (GtkWidget *widget, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   iv->movie_menu = NULL;
}


static void
cb_view_modes_menu_destroy (GtkWidget *widget, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   iv->view_modes_menu = NULL;
}


static void
cb_continuance (GimvImageView *iv, guint action, GtkWidget *widget)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (GTK_IS_CHECK_MENU_ITEM (widget));

   iv->priv->continuance_play = GTK_CHECK_MENU_ITEM (widget)->active;
}


static void
cb_change_view_mode (GtkWidget *widget, GimvImageView *iv)
{
   const gchar *label;

   label = g_object_get_data (G_OBJECT (widget), "GimvImageView::ViewMode");
   gimv_image_view_change_view_mode (iv, label);
}



/*****************************************************************************
 *
 *   callback functions for player toolbar.
 *
 *****************************************************************************/
static void
cb_gimv_image_view_play (GtkButton *button, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_play (iv);
}


static void
cb_gimv_image_view_stop (GtkButton *button, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_stop (iv);
}


static void
cb_gimv_image_view_fw (GtkButton *button, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_forward (iv);
}


static void
cb_gimv_image_view_eject (GtkButton *button, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_eject (iv);
}


static void
cb_gimv_image_view_rw (GtkButton *button, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_reverse (iv);
}


static gboolean
cb_seekbar_pressed (GtkWidget *widget,
                    GdkEventButton *event,
                    GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   iv->priv->player_flags |= GimvImageViewSeekBarDraggingFlag;

   return FALSE;
}


static gboolean
cb_seekbar_released (GtkWidget *widget,
                     GdkEventButton *event,
                     GimvImageView *iv)
{
   GtkAdjustment *adj;
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   adj = gtk_range_get_adjustment (GTK_RANGE (iv->player.seekbar));
   gimv_image_view_playable_seek (iv, adj->value);

   iv->priv->player_flags &= ~GimvImageViewSeekBarDraggingFlag;

   return FALSE;
}



/*****************************************************************************
 *
 *   other callback functions.
 *
 *****************************************************************************/
static void
cb_destroy_loader (GimvImageLoader *loader, gpointer data)
{
   g_signal_handlers_disconnect_by_func (G_OBJECT (loader),
                                         G_CALLBACK (cb_destroy_loader),
                                         data);
   g_object_unref (G_OBJECT (loader));
}


static void
cb_image_map (GtkWidget *widget, GimvImageView *iv)
{
   g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
                                         G_CALLBACK (cb_image_map), iv);
   gimv_image_view_show_image (iv);
   gimv_image_view_playable_play (iv);
   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[IMAGE_CHANGED_SIGNAL], 0);
}


static gboolean
cb_image_key_press (GtkWidget *widget, GdkEventKey *event, GimvImageView *iv)
{
   guint keyval, popup_key;
   GdkModifierType modval, popup_mod;
   gboolean move = FALSE;
   gint mx, my;

   g_return_val_if_fail (iv, FALSE);

   gimv_image_view_get_view_position (iv, &mx, &my);

   keyval = event->keyval;
   modval = event->state;

   if (akey.common_popup_menu || *akey.common_popup_menu)
      gtk_accelerator_parse (akey.common_popup_menu, &popup_key, &popup_mod);
   else
      return FALSE;

   if (keyval == popup_key && (!popup_mod || (modval & popup_mod))) {
      gimv_image_view_popup_menu (iv, NULL);
   } else {
      switch (keyval) {
      case GDK_Left:
      case GDK_KP_Left:
      case GDK_KP_4:
         mx -= 20;
         move = TRUE;
         break;
      case GDK_Right:
      case GDK_KP_Right:
      case GDK_KP_6:
         mx += 20;
         move = TRUE;
         break;
      case GDK_Up:
      case GDK_KP_Up:
      case GDK_KP_8:
         my -= 20;
         move = TRUE;
         break;
      case GDK_Down:
      case GDK_KP_Down:
      case GDK_KP_2:
         my += 20;
         move = TRUE;
         break;
      case GDK_KP_5:
         mx = 0;
         my = 0;
         move = TRUE;
         break;
      case GDK_Page_Up:
      case GDK_KP_Page_Up:
      case GDK_KP_9:
         gimv_image_view_prev (iv);
         return TRUE;
      case GDK_space:
      case GDK_Page_Down:
      case GDK_KP_Space:
      case GDK_KP_Page_Down:
      case GDK_KP_3:
      case GDK_KP_0:
         gimv_image_view_next (iv);
         return TRUE;
      case GDK_Home:
      case GDK_KP_Home:
      case GDK_KP_7:
        gimv_image_view_nth (iv, 0);
         return TRUE;
      case GDK_End:
      case GDK_KP_End:
      case GDK_KP_1:
      {
         gint last;
         if (iv->priv->image_list) {
            last = g_list_length (iv->priv->image_list->list) - 1;
            if (last > 0)
               gimv_image_view_nth (iv, last);
         }
         return TRUE;
      }
      case GDK_KP_Add:
      case GDK_plus:
         gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_IN, 0, 0);
         return FALSE;
         break;
      case GDK_KP_Subtract:
      case GDK_minus:
         gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_OUT, 0, 0);
         return FALSE;
         break;
      case GDK_equal:
      case GDK_KP_Equal:
      case GDK_KP_Enter:
         gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_100, 0, 0);
         return FALSE;
         break;
      case GDK_KP_Divide:
         gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_FIT, 0, 0);
         return FALSE;
         break;
      }
   }

   if (move) {
      gimv_image_view_moveto (iv, mx, my);
      return TRUE;
   }

   return FALSE;
}


static gboolean
cb_image_button_press (GtkWidget *widget, GdkEventButton *event,
                       GimvImageView *iv)
{
   GdkCursor *cursor;
   gint retval = FALSE;

   g_return_val_if_fail (iv, FALSE);

   iv->priv->pressed = TRUE;
   iv->priv->button  = event->button;

   gtk_widget_grab_focus (widget);

   g_signal_emit (G_OBJECT (iv),
                  gimv_image_view_signals[IMAGE_PRESSED_SIGNAL], 0,
                  event, &retval);

   if (iv->priv->dragging)
      return FALSE;

   if (event->button == 1) {   /* scroll image */
      if (!iv->priv->pixmap)
         return FALSE;

      cursor = cursor_get (widget->window, CURSOR_HAND_CLOSED);
      retval = gdk_pointer_grab (widget->window, FALSE,
                                 (GDK_POINTER_MOTION_MASK
                                  | GDK_POINTER_MOTION_HINT_MASK
                                  | GDK_BUTTON_RELEASE_MASK),
                                 NULL, cursor, event->time);
      gdk_cursor_destroy (cursor);

      if (retval != 0)
         return FALSE;

      iv->priv->drag_startx = event->x - iv->priv->x_pos;
      iv->priv->drag_starty = event->y - iv->priv->y_pos;
      iv->priv->x_pos_drag_start = iv->priv->x_pos;
      iv->priv->y_pos_drag_start = iv->priv->y_pos;

      allocate_draw_buffer (iv);

      return TRUE;

   }

   return FALSE;
}


static gboolean
cb_image_button_release  (GtkWidget *widget, GdkEventButton *event,
                          GimvImageView *iv)
{
   gint retval = FALSE;

   /*
   if (event->button != 1)
      return FALSE;
   */

   g_signal_emit (G_OBJECT (iv),
                  gimv_image_view_signals[IMAGE_RELEASED_SIGNAL], 0,
                  event, &retval);

   if(iv->priv->pressed && !iv->priv->dragging)
      g_signal_emit (G_OBJECT (iv),
                     gimv_image_view_signals[IMAGE_CLICKED_SIGNAL], 0,
                     event, &retval);

   if (buffer) {
      gdk_pixmap_unref (buffer);
      buffer = NULL;
   }

   iv->priv->button   = 0;
   iv->priv->pressed  = FALSE;
   iv->priv->dragging = FALSE;

   gdk_pointer_ungrab (event->time);

   return retval;
}


static gboolean
cb_image_motion_notify (GtkWidget *widget, GdkEventMotion *event,
                        GimvImageView *iv)
{
   GdkModifierType mods;
   gint x, y, x_pos, y_pos, dx, dy;

   if (!iv->priv->pressed)
      return FALSE;

   gdk_window_get_pointer (widget->window, &x, &y, &mods);

   x_pos = x - iv->priv->drag_startx;
   y_pos = y - iv->priv->drag_starty;
   dx = x_pos - iv->priv->x_pos_drag_start;
   dy = y_pos - iv->priv->y_pos_drag_start;

   if (!iv->priv->dragging && (abs(dx) > 2 || abs (dy) > 2))
      iv->priv->dragging = TRUE;

   /* scroll image */
   if (iv->priv->button == 1) {
      if (event->is_hint)
         gdk_window_get_pointer (widget->window, &x, &y, &mods);

      iv->priv->x_pos = x_pos;
      iv->priv->y_pos = y_pos;

      gimv_image_view_adjust_pos_in_frame (iv, FALSE);
   }

   gimv_image_view_draw_image (iv);

   return TRUE;
}


static void
cb_drag_data_received (GtkWidget *widget,
                       GdkDragContext *context,
                       gint x, gint y,
                       GtkSelectionData *seldata,
                       guint info,
                       guint32 time,
                       gpointer data)
{
   GimvImageView *iv = data;
   FilesLoader *files;
   GList *filelist;
   gchar *tmpstr;

   g_return_if_fail (iv || widget);

   filelist = dnd_get_file_list (seldata->data, seldata->length);

   if (filelist) {
      GimvImageInfo *info = gimv_image_info_get ((gchar *) filelist->data);

      if (!info) goto ERROR;

      gimv_image_view_change_image (iv, info);
      tmpstr = filelist->data;
      filelist = g_list_remove (filelist, tmpstr);
      g_free (tmpstr);

      if (filelist) {
         files = files_loader_new ();
         files->filelist = filelist;
         open_image_files_in_image_view (files);
         files->filelist = NULL;
         files_loader_delete (files);
      }
   }

ERROR:
   g_list_foreach (filelist, (GFunc) g_free, NULL);
   g_list_free (filelist);
}


static void
cb_scrollbar_value_changed (GtkAdjustment *adj, GimvImageView *iv)
{
   g_return_if_fail (iv);

   if (!move_scrollbar_by_user) return;

   if (iv->priv->width > iv->draw_area->allocation.width)
      iv->priv->x_pos = 0 - iv->hadj->value;
   if (iv->priv->height > iv->draw_area->allocation.height)
      iv->priv->y_pos = 0 - iv->vadj->value;

   gimv_image_view_draw_image (iv);
}


static gboolean
cb_nav_button (GtkWidget *widget, GdkEventButton *event, GimvImageView *iv)
{
   g_return_val_if_fail (iv, FALSE);

   gimv_image_view_open_navwin (iv, event->x_root, event->y_root);

   return FALSE;
}


/*****************************************************************************
 *
 *   Private functions.
 *
 *****************************************************************************/
static void
allocate_draw_buffer (GimvImageView *iv)
{
   gint fwidth, fheight;

   if (!iv) return;

   if (buffer) gdk_pixmap_unref (buffer);
   gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);
   buffer = gdk_pixmap_new (iv->draw_area->window, fwidth, fheight, -1);
}



static void
gimv_image_view_calc_image_size (GimvImageView *iv)
{
   gint orig_width, orig_height;
   gint width, height;
   gint fwidth, fheight;
   gfloat x_scale, y_scale;

   /* image scale */
   if (iv->priv->rotate == 0 || iv->priv->rotate == 2) {
      x_scale = iv->priv->x_scale;
      y_scale = iv->priv->y_scale;
   } else {
      x_scale = iv->priv->y_scale;
      y_scale = iv->priv->x_scale;
   }

   if (iv->priv->rotate == 0 || iv->priv->rotate == 2) {
      orig_width  = iv->info->width;
      orig_height = iv->info->height;
   } else {
      orig_width  = iv->info->height;
      orig_height = iv->info->width;
   }

   gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);
   width  = orig_width;
   height = orig_height;

   /* calculate image size */
   switch (iv->priv->fit_to_frame) {
   case GIMV_IMAGE_VIEW_ZOOM_FIT:
   case GIMV_IMAGE_VIEW_ZOOM_FIT_ZOOM_OUT_ONLY:
   {
      if (width <= fwidth && height <= fheight
          && iv->priv->fit_to_frame == GIMV_IMAGE_VIEW_ZOOM_FIT_ZOOM_OUT_ONLY)
      {
         iv->priv->x_scale = 100.0;
         iv->priv->y_scale = 100.0;
      } else {
         iv->priv->x_scale = (gfloat) fwidth  / (gfloat) width  * 100.0;
         iv->priv->y_scale = (gfloat) fheight / (gfloat) height * 100.0;

         if (iv->priv->keep_aspect) {
            if (iv->priv->x_scale > iv->priv->y_scale) {
               iv->priv->x_scale = iv->priv->y_scale;
               width  = orig_width * iv->priv->x_scale / 100.0;
               height = fheight;
            } else {
               iv->priv->y_scale = iv->priv->x_scale;
               width  = fwidth;
               height = orig_height * iv->priv->y_scale / 100.0;
            }
         } else {
            width  = fwidth;
            height = fheight;
         }
      }

      break;
   }

   case GIMV_IMAGE_VIEW_ZOOM_FIT_WIDTH:
      iv->priv->x_scale = (gfloat) fwidth / (gfloat) width * 100.0;
      iv->priv->y_scale = iv->priv->x_scale;
      width  = orig_width  * iv->priv->x_scale/ 100.0;
      height = orig_height * iv->priv->x_scale / 100.0;
      break;

   case GIMV_IMAGE_VIEW_ZOOM_FIT_HEIGHT:
      iv->priv->y_scale = (gfloat) fheight / (gfloat) height * 100.0;
      iv->priv->x_scale = iv->priv->y_scale;
      width  = orig_width  * iv->priv->y_scale / 100.0;
      height = orig_height * iv->priv->y_scale / 100.0;
      break;

   default:
      width  = orig_width  * x_scale / 100.0;
      height = orig_height * y_scale / 100.0;
      break;
   }

   iv->priv->fit_to_frame = 0;

   /*
    * zoom out if image is too big & window is not scrollable on
    * fullscreen mode
    */
   /*
   win_width  = hadj->page_size;
   win_height = vadj->page_size;

   if (im->fullscreen && (width > win_width || height > win_height)) {
         gfloat width_ratio, height_ratio;

         width_ratio = (gdouble) width / hadj->page_size;
         height_ratio = (gdouble) height / vadj->page_size;
         if (width_ratio > height_ratio) {
         width = win_width;
         height = (gdouble) height / (gdouble) width_ratio;
      } else {
         width = (gdouble) width / (gdouble) height_ratio;
         height = win_height;
      }
   }
   */

   if (width > MIN_IMAGE_WIDTH)
      iv->priv->width = width;
   else
      iv->priv->width = MIN_IMAGE_WIDTH;
   if (height > MIN_IMAGE_HEIGHT)
      iv->priv->height = height;
   else
      iv->priv->height = MIN_IMAGE_HEIGHT;
}


static void
gimv_image_view_change_draw_widget (GimvImageView *iv, const gchar *label)
{
   GList *node;
   GimvImageViewPlugin *vftable = NULL;
   const gchar *view_mode;

   g_return_if_fail (iv);

   if (iv->draw_area_funcs
       && iv->draw_area_funcs->is_supported_fn
       && iv->draw_area_funcs->is_supported_fn (iv, iv->info))
   {
      return;
   }

   /* if default movie view mode is specified */
   /* FIXME!! this code is ad-hoc */
   view_mode = conf.movie_default_view_mode;
   if (iv->info
       && (gimv_image_info_is_movie (iv->info) || gimv_image_info_is_audio (iv->info))
       && view_mode && *view_mode)
   {
      for (node = gimv_image_view_plugin_get_list();
           node;
           node = g_list_next (node))
      {
         GimvImageViewPlugin *table = node->data;

         if (!table) continue;

         if (!strcmp (view_mode, table->label)
             && table->is_supported_fn
             && table->is_supported_fn (iv, iv->info))
         {
            vftable = table;
            break;
         }
      }
   }

   /* fall down... */
   for (node = gimv_image_view_plugin_get_list();
        node && !vftable;
        node = g_list_next (node))
   {
      GimvImageViewPlugin *table = node->data;

      if (!table) continue;

      if (label && *label) {
         if (!strcmp (label, table->label)
             || !strcmp (GIMV_IMAGE_VIEW_DEFAULT_VIEW_MODE, table->label))
         {
            vftable = table;
            break;
         }
      } else {
         if (!table->is_supported_fn
             || table->is_supported_fn (iv, iv->info))
         {
            vftable = table;
            break;
         }
      }
   }

   if (!vftable)
      vftable = &imageview_draw_vfunc_table;
   g_return_if_fail (vftable->create_fn);
   if (iv->draw_area_funcs && !strcmp (iv->draw_area_funcs->label, vftable->label))
      return;
   if (iv->draw_area)
      gtk_widget_destroy (iv->draw_area);

   iv->draw_area = vftable->create_fn (iv);
   iv->draw_area_funcs = vftable;

   gtk_widget_show (iv->draw_area);

   g_signal_connect_after (G_OBJECT (iv->draw_area), "key-press-event",
                           G_CALLBACK (cb_image_key_press), iv);
   g_signal_connect (G_OBJECT (iv->draw_area), "button_press_event",
                     G_CALLBACK (cb_image_button_press), iv);
   g_signal_connect (G_OBJECT (iv->draw_area), "button_release_event",
                     G_CALLBACK (cb_image_button_release), iv);
   g_signal_connect (G_OBJECT (iv->draw_area), "motion_notify_event",
                     G_CALLBACK (cb_image_motion_notify), iv);
   SIGNAL_CONNECT_TRANSRATE_SCROLL(iv->draw_area);

   if (iv->priv->fullscreen) {
      gtk_container_add (GTK_CONTAINER (iv->priv->fullscreen), iv->draw_area);
   } else {
      gtk_table_attach (GTK_TABLE (iv->table), iv->draw_area,
                        0, 1, 0, 1,
                        GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
   }

   /* for droping file list */
   g_signal_connect (G_OBJECT (iv->draw_area), "drag_data_received",
                     G_CALLBACK (cb_drag_data_received), iv);

   dnd_dest_set (iv->draw_area, dnd_types_archive, dnd_types_archive_num);

   /* set flags */
   GTK_WIDGET_SET_FLAGS (iv->draw_area, GTK_CAN_FOCUS);
}


static void
gimv_image_view_adjust_pos_in_frame (GimvImageView *iv, gboolean center)
{
   gint fwidth, fheight;

   if (center) {
      gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);

      if (fwidth < iv->priv->width) {
         if (conf.imgview_scroll_nolimit) {
            if (iv->priv->x_pos > fwidth)
               iv->priv->x_pos = 0;
            else if (iv->priv->x_pos < 0 - iv->priv->width)
               iv->priv->x_pos = 0 - iv->priv->width + fwidth;
         } else {
            if (iv->priv->x_pos > 0)
               iv->priv->x_pos = 0;
            else if (iv->priv->x_pos < 0 - iv->priv->width + fwidth)
               iv->priv->x_pos = 0 - iv->priv->width + fwidth;
         }
      } else {
         iv->priv->x_pos = (fwidth - iv->priv->width) / 2;
      }

      if (fheight < iv->priv->height) {
         if (conf.imgview_scroll_nolimit) {
            if (iv->priv->y_pos > fheight)
               iv->priv->y_pos = 0;
            else if (iv->priv->y_pos < 0 - iv->priv->height)
               iv->priv->y_pos = 0 - iv->priv->height + fheight;
         } else {
            if (iv->priv->y_pos > 0)
               iv->priv->y_pos = 0;
            else if (iv->priv->y_pos < 0 - iv->priv->height + fheight)
               iv->priv->y_pos = 0 - iv->priv->height + fheight;
         }
      } else {
         iv->priv->y_pos = (fheight - iv->priv->height) / 2;
      }
   } else {
      if (!conf.imgview_scroll_nolimit) {
         gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);

         if (iv->priv->width <= fwidth) {
            if (iv->priv->x_pos < 0)
               iv->priv->x_pos = 0;
            if (iv->priv->x_pos > fwidth - iv->priv->width)
               iv->priv->x_pos = fwidth - iv->priv->width;
         } else if (iv->priv->x_pos < fwidth - iv->priv->width) {
            iv->priv->x_pos = fwidth - iv->priv->width;
         } else if (iv->priv->x_pos > 0) {
            iv->priv->x_pos = 0;
         }

         if (iv->priv->height <= fheight) {
            if (iv->priv->y_pos < 0)
               iv->priv->y_pos = 0;
            if (iv->priv->y_pos > fheight - iv->priv->height)
               iv->priv->y_pos = fheight - iv->priv->height;
         } else if (iv->priv->y_pos < fheight - iv->priv->height) {
            iv->priv->y_pos = fheight - iv->priv->height;
         } else if (iv->priv->y_pos > 0) {
            iv->priv->y_pos = 0;
         }
      }
   }
}

#ifdef ENABLE_EXIF
static int
get_exif_rotation (GimvImageInfo *info)
{
   ExifData *edata;
   JPEGData *jdata;	 
   unsigned int i=0;
   int rotate=0;
   ExifByteOrder byte_order;
   ExifShort v_short;
	 	 
   g_return_val_if_fail (info->filename && *(info->filename), 0);

   jdata = jpeg_data_new_from_file (info->filename);
   if (!jdata) {
      return 0;
   }

   edata = jpeg_data_get_exif_data (jdata);
   if (!edata) {
      goto ERROR;
   }

   byte_order = exif_data_get_byte_order (edata);
		
   for (i = 0; i < edata->ifd[0]->count; i++) {
      if (edata->ifd[0]->entries[i]->tag == EXIF_TAG_ORIENTATION) {
         v_short = exif_get_short (edata->ifd[0]->entries[i]->data , byte_order);
         switch (v_short) {
         case 1:            /* no rotation */
            rotate = 0;
            break;
         case 2:            /*flip vertical */
            rotate = 0;
            break;
         case 3:            /* rotate 180 degrees */
            rotate = 2;
            break;
         case 4:            /* flip vertial and rotate 180 degrees */
            rotate = 0;
            break;
         case 5:            /* rotate 90 degrees clockwise and flip vertical */
            rotate = 0;
            break;
         case 6:            /* rotate 90 degrees clockwise */
            rotate = 3;
            break;		
         case 7:            /* rotate 90 degrees counterclockwise and flip vertical */
            rotate = 0;
            break;
         case 8:           /* rotate 90 degrees counterclockwise */
            rotate = 1;
            break;
         }
      }
   }
		
   exif_data_unref (edata);
   jpeg_data_unref (jdata);	
	
   return rotate;
		
ERROR:
   jpeg_data_unref (jdata);
   return 0;
}

#endif


/*****************************************************************************
 *
 *   Public functions.
 *
 *****************************************************************************/
GList *
gimv_image_view_get_list (void)
{
   return GimvImageViewList;
}


void
gimv_image_view_change_image (GimvImageView *iv, GimvImageInfo *info)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_stop (iv);

   gimv_image_view_change_image_info (iv, info);

   gimv_image_view_change_draw_widget (iv, NULL);
   if (!g_list_find (GimvImageViewList, iv)) return;

   if (GTK_WIDGET_MAPPED (iv->draw_area)) {
      gimv_image_view_show_image (iv);
      gimv_image_view_playable_play (iv);
      g_signal_emit (G_OBJECT(iv),
                     gimv_image_view_signals[IMAGE_CHANGED_SIGNAL], 0);
   } else {
      g_signal_connect_after (G_OBJECT (iv->draw_area), "map",
                              G_CALLBACK (cb_image_map), iv);
   }
}


void
gimv_image_view_change_image_info (GimvImageView *iv, GimvImageInfo *info)
{
   g_return_if_fail (iv);

   if (!g_list_find (GimvImageViewList, iv)) return;

   /* free old image */
   if (iv->priv->pixmap)
      gimv_image_free_pixmap_and_mask (iv->priv->pixmap, iv->priv->mask);
   iv->priv->pixmap = NULL;
   iv->priv->mask = NULL;

   if (iv->image)
      gimv_image_unref (iv->image);
   iv->image = NULL;

   /* free old image info */
   if (iv->info)
      gimv_image_info_unref (iv->info);

   /* Reset fit_to_frame (zoom control) to its default value for a new image.
    * The numbers correspond to the order of options in the pulldown menu
    * in prefs_ui_imagewin.c
    */

   /*
    * 2004-09-29 Takuro Ashie <ashie@homa.ne.jp>
    *   We should define enum.
    */

   switch (iv->priv->default_zoom) {
   case 2:
      iv->priv->fit_to_frame = GIMV_IMAGE_VIEW_ZOOM_FIT;
      break;
   case 3:
      iv->priv->fit_to_frame = GIMV_IMAGE_VIEW_ZOOM_FIT_ZOOM_OUT_ONLY;
      break;
   case 4:
      iv->priv->fit_to_frame = GIMV_IMAGE_VIEW_ZOOM_FIT_WIDTH;
      break;
   case 5:
      iv->priv->fit_to_frame = GIMV_IMAGE_VIEW_ZOOM_FIT_HEIGHT;
      break;
   case 0:
      iv->priv->x_scale = conf.imgview_scale;
      iv->priv->y_scale = conf.imgview_scale;
   case 1:
   default:
      iv->priv->fit_to_frame = 0;
      break;
   }

   /* ditto for rotation */

   switch (iv->priv->default_rotation) {
   case 4:
      break;
   case 5:
#ifdef ENABLE_EXIF
      iv->priv->rotate = get_exif_rotation (info);
#else
      iv->priv->rotate = 0;
#endif
      break;
    default:
      iv->priv->rotate = iv->priv->default_rotation;
      break;
   }

   /* suggestion from sheepman <sheepman@tcn.zaq.ne.jp> */
   iv->priv->x_pos = iv->priv->y_pos = 0;

   /* allocate memory for new image info*/
   if (info)
      iv->info = gimv_image_info_ref (info);
   else
      iv->info = NULL;
}


static gint
idle_gimv_image_view_change_image_info (gpointer data)
{
   GimvImageView *iv = data;
   GList *node;

   g_return_val_if_fail (data, FALSE);

   node = g_list_find (GimvImageViewList, iv);
   if (!node) return FALSE;

   gimv_image_info_ref (iv->info);
   gimv_image_view_change_image (iv, iv->info);

   return FALSE;
}


void
gimv_image_view_draw_image (GimvImageView *iv)
{
   GdkGC *bg_gc;
   gboolean free = FALSE;

   if (!GTK_WIDGET_MAPPED (iv)) return;

   /* allocate buffer */
   if (!buffer) {
      allocate_draw_buffer (iv);
      free = TRUE;
   }

   /* fill background by default bg color */
   bg_gc = iv->draw_area->style->bg_gc[GTK_WIDGET_STATE (iv->draw_area)];
   gdk_draw_rectangle (buffer, bg_gc, TRUE, 0, 0, -1, -1);

   /* draw image to buffer */
   if (iv->priv->pixmap) {
      if (iv->priv->mask) {
         gdk_gc_set_clip_mask (iv->draw_area->style->black_gc, iv->priv->mask);
         gdk_gc_set_clip_origin (iv->draw_area->style->black_gc,
                                 iv->priv->x_pos, iv->priv->y_pos);
      }

      gdk_draw_pixmap (buffer,
                       iv->draw_area->style->black_gc,
                       iv->priv->pixmap,
                       0, 0,
                       iv->priv->x_pos, iv->priv->y_pos,
                       -1, -1);

      if (iv->priv->mask) {
         gdk_gc_set_clip_mask (iv->draw_area->style->black_gc, NULL);
         gdk_gc_set_clip_origin (iv->draw_area->style->black_gc, 0, 0);
      }
   }

   /* draw from buffer to foreground */
   gdk_draw_pixmap (iv->draw_area->window,
                    iv->draw_area->style->fg_gc[GTK_WIDGET_STATE (iv->draw_area)],
                    buffer, 0, 0, 0, 0, -1, -1);

   /* free buffer */
   if (free) {
      gdk_pixmap_unref (buffer);
      buffer = NULL;
   }

   gimv_image_view_reset_scrollbar (iv);
}


void
gimv_image_view_show_image (GimvImageView *iv)
{
   gimv_image_view_rotate_image (iv, iv->priv->rotate);
}


static gboolean
check_can_draw_image (GimvImageView *iv)
{
   gboolean is_movie = FALSE;
   const gchar *newfile = NULL;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);
   g_return_val_if_fail (iv->draw_area, FALSE);
   g_return_val_if_fail (GTK_IS_DRAWING_AREA (iv->draw_area), FALSE);

   /* if (!GTK_WIDGET_MAPPED (iv->draw_area)) return FALSE; */

#warning FIXME!!
   if (iv->info && gimv_image_info_is_movie (iv->info))
      is_movie = TRUE;

   if (is_movie) return TRUE;
   if (iv->image) return TRUE;

   if (iv->info)
      newfile = gimv_image_info_get_path (iv->info);

   if (newfile && *newfile) {
      if (!file_exists (newfile))
         g_print (_("File not exist: %s\n"), newfile);
      else
         g_print(_("Not an image (or unsupported) file: %s\n"), newfile);
   }

   /* clear draw area */
   gimv_image_view_draw_image (iv);

   return FALSE;
}




void
gimv_image_view_create_thumbnail (GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->info) return;
   if (iv->info->flags & GIMV_IMAGE_INFO_MRL_FLAG) return;   /* Dose it need? */

   if (!iv->draw_area_funcs) return;
   if (!iv->draw_area_funcs->create_thumbnail_fn) return;

   iv->draw_area_funcs->create_thumbnail_fn (iv, conf.cache_write_type);
}


/*****************************************************************************
 *
 *   functions for creating/show/hide/setting status child widgets
 *
 *****************************************************************************/
void
gimv_image_view_change_view_mode (GimvImageView *iv,
                                  const gchar *label)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_change_image (iv, NULL);
   gimv_image_view_change_draw_widget (iv, label);
}


static GtkWidget *
gimv_image_view_create_player_toolbar (GimvImageView *iv)
{
   GtkWidget *hbox, *toolbar;
   GtkWidget *button, *seekbar;
   GtkWidget *iconw;
   GtkObject *adj;

   hbox = gtk_hbox_new (FALSE, 0);
   toolbar = gtkutil_create_toolbar ();
   gtk_toolbar_set_style (GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
   gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
   gtk_widget_show (toolbar);

   iv->player_container = hbox;
   iv->player_toolbar = toolbar;

   /* Reverse button */
   iconw = gimv_icon_stock_get_widget ("rw");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("RW"),
                                     _("Reverse"), _("Reverse"),
                                     iconw,
                                     G_CALLBACK (cb_gimv_image_view_rw), iv);
   gtk_widget_set_sensitive (button, FALSE);
   iv->player.rw = button;

   /* play button */
   iconw = gimv_icon_stock_get_widget ("play");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Play"),
                                     _("Play"), _("Play"),
                                     iconw,
                                     G_CALLBACK (cb_gimv_image_view_play), iv);
   gtk_widget_set_sensitive (button, FALSE);
   iv->player.play = button;
   iv->player.play_icon = iconw;

   /* stop button */
   iconw = gimv_icon_stock_get_widget ("stop2");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Stop"),
                                     _("Stop"), _("Stop"),
                                     iconw,
                                     G_CALLBACK (cb_gimv_image_view_stop), iv);
   gtk_widget_set_sensitive (button, FALSE);
   iv->player.stop = button;

   /* Forward button */
   iconw = gimv_icon_stock_get_widget ("ff");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("FF"),
                                     _("Forward"), _("Forward"),
                                     iconw,
                                     G_CALLBACK (cb_gimv_image_view_fw), iv);
   gtk_widget_set_sensitive (button, FALSE);
   iv->player.fw = button;

   /* Eject button */
   iconw = gimv_icon_stock_get_widget ("eject");
   button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                                     _("Eject"),
                                     _("Eject"), _("Eject"),
                                     iconw,
                                     G_CALLBACK (cb_gimv_image_view_eject), iv);
   iv->player.eject = button;
   gtk_widget_set_sensitive (button, FALSE);

   adj = gtk_adjustment_new (0.0, 0.0, 100.0, 0.1, 1.0, 1.0);
   seekbar = gtk_hscale_new (GTK_ADJUSTMENT (adj));
   gtk_scale_set_draw_value (GTK_SCALE (seekbar), FALSE);
   gtk_box_pack_start (GTK_BOX (hbox), seekbar, TRUE, TRUE, 0);
   gtk_widget_show (seekbar);
   iv->player.seekbar = seekbar;

   g_signal_connect (G_OBJECT (iv->player.seekbar), "button_press_event",
                     G_CALLBACK (cb_seekbar_pressed), iv);
   g_signal_connect (G_OBJECT (iv->player.seekbar), "button_release_event",
                     G_CALLBACK (cb_seekbar_released), iv);

   return hbox;
}


GtkWidget *
gimv_image_view_create_zoom_menu (GtkWidget *window,
                                  GimvImageView *iv,
                                  const gchar *path)
{
   GtkWidget *menu;
   guint n_menu_items;

   g_return_val_if_fail (window, NULL);
   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (path && *path, NULL);

   n_menu_items = sizeof(gimv_image_view_zoom_items)
      / sizeof(gimv_image_view_zoom_items[0]) - 1;
   menu = menu_create_items(window, gimv_image_view_zoom_items,
                            n_menu_items, path, iv);
   iv->zoom_menu = menu;
   menu_check_item_set_active (menu, "/Keep aspect ratio", iv->priv->keep_aspect);

   return menu;
}


GtkWidget *
gimv_image_view_create_rotate_menu (GtkWidget *window,
                                    GimvImageView *iv,
                                    const gchar *path)
{
   GtkWidget *menu;
   guint n_menu_items;

   g_return_val_if_fail (window, NULL);
   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (path && *path, NULL);

   n_menu_items = sizeof(gimv_image_view_rotate_items)
      / sizeof(gimv_image_view_rotate_items[0]) - 1;
   menu = menu_create_items(window, gimv_image_view_rotate_items,
                            n_menu_items, path, iv);
   iv->rotate_menu = menu;

   return menu;
}


GtkWidget *
gimv_image_view_create_movie_menu (GtkWidget *window,
                                   GimvImageView *iv,
                                   const gchar *path)
{
   GtkWidget *menu;
   guint n_menu_items;
   GimvImageViewPlayableStatus status;

   n_menu_items = sizeof(gimv_image_view_playable_items)
      / sizeof(gimv_image_view_playable_items[0]) - 1;
   menu = menu_create_items(window, gimv_image_view_playable_items,
                            n_menu_items, path, iv);
   iv->movie_menu = menu;

   menu_check_item_set_active (iv->movie_menu, "/Continuance",
                               iv->priv->continuance_play);
   status = gimv_image_view_playable_get_status (iv);
   gimv_image_view_playable_set_status (iv, status);

   g_signal_connect (G_OBJECT (menu), "destroy",
                     G_CALLBACK (cb_movie_menu_destroy), iv);

   return menu;
}


GtkWidget *
gimv_image_view_create_view_modes_menu (GtkWidget *window,
                                        GimvImageView *iv,
                                        const gchar *path)
{
   GtkWidget *menu;
   GList *node;

   menu = gtk_menu_new();
   iv->view_modes_menu = menu;

   for (node = gimv_image_view_plugin_get_list(); node; node = g_list_next (node)) {
      GtkWidget *menu_item;
      GimvImageViewPlugin *vftable = node->data;

      if (!vftable) continue;

      menu_item = gtk_menu_item_new_with_label (_(vftable->label));
      g_object_set_data (G_OBJECT (menu_item),
                         "GimvImageView::ViewMode",
                         (gpointer) vftable->label);
      g_signal_connect (G_OBJECT (menu_item), "activate",
                        G_CALLBACK(cb_change_view_mode), iv);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
   }

   g_signal_connect (G_OBJECT (menu), "destroy",
                     G_CALLBACK (cb_view_modes_menu_destroy), iv);

   return menu;
}


GtkWidget *
gimv_image_view_create_popup_menu (GtkWidget *window,
                                   GimvImageView *iv,
                                   const gchar *path)
{
   guint n_menu_items;

   g_return_val_if_fail (window, NULL);
   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (path && *path, NULL);

   n_menu_items = sizeof(gimv_image_view_popup_items)
      / sizeof(gimv_image_view_popup_items[0]) - 1;
   iv->imageview_popup = menu_create_items(window, gimv_image_view_popup_items,
                                           n_menu_items, path, iv);

   gimv_image_view_create_zoom_menu (window, iv, path);
   gimv_image_view_create_rotate_menu (window, iv, path);

   menu_set_submenu (iv->imageview_popup, "/Zoom",   iv->zoom_menu);
   menu_set_submenu (iv->imageview_popup, "/Rotate", iv->rotate_menu);

   menu_check_item_set_active (iv->imageview_popup, "/Show Scrollbar",
                               iv->priv->show_scrollbar);
   menu_check_item_set_active (iv->imageview_popup, "/Memory Buffer",
                               iv->priv->buffer);

   gimv_image_view_create_movie_menu (window, iv, path);
   menu_set_submenu (iv->imageview_popup, "/Movie",  iv->movie_menu);

   gimv_image_view_create_view_modes_menu (window, iv, path);
   menu_set_submenu (iv->imageview_popup, "/View modes",  iv->view_modes_menu);

   return iv->imageview_popup;
}


void
gimv_image_view_popup_menu (GimvImageView *iv, GdkEventButton *event)
{
   guint button;
   guint32 time;
   GtkMenuPositionFunc pos_fn = NULL;

   g_return_if_fail (iv);
   /* g_return_if_fail (event); */

   if (event) {
      button = event->button;
      time = gdk_event_get_time ((GdkEvent *) event);
   } else {
      button = 0;
      time = GDK_CURRENT_TIME;
      pos_fn = menu_calc_popup_position;
   }

   if (iv->imageview_popup)
      gtk_menu_popup (GTK_MENU (iv->imageview_popup),
                      NULL, NULL,
                      pos_fn, iv->draw_area->window,
                      button, time);
}


void
gimv_image_view_set_bg_color (GimvImageView *iv, gint red, gint green, gint brue)
{
   g_return_if_fail (iv);
   g_return_if_fail (iv->draw_area);

   if (!iv->bg_color) {
      iv->bg_color = g_new0 (GdkColor, 1);
      iv->bg_color->pixel = 0;
   }
   iv->bg_color->red   = red;
   iv->bg_color->green = green;
   iv->bg_color->blue  = brue;

   if (GTK_WIDGET_MAPPED (iv->draw_area)) {
      GdkColormap *colormap;
      GtkStyle *style;
      colormap = gdk_window_get_colormap(iv->draw_area->window);
      gdk_colormap_alloc_color (colormap, iv->bg_color, FALSE, TRUE);
      style = gtk_style_copy (gtk_widget_get_style (iv->draw_area));
      style->bg[GTK_STATE_NORMAL] = *iv->bg_color;
      gtk_widget_set_style (iv->draw_area, style);
      gtk_style_unref (style);
   }
}


void
gimv_image_view_show_scrollbar (GimvImageView *iv)
{
   g_return_if_fail (iv);
   g_return_if_fail (iv->hscrollbar);
   g_return_if_fail (iv->vscrollbar);

   gtk_widget_show (iv->hscrollbar);
   gtk_widget_show (iv->vscrollbar);
   gtk_widget_show (iv->nav_button);

   iv->priv->show_scrollbar = TRUE;
}


void
gimv_image_view_hide_scrollbar (GimvImageView *iv)
{
   g_return_if_fail (iv);
   g_return_if_fail (iv->hscrollbar);
   g_return_if_fail (iv->vscrollbar);

   gtk_widget_hide (iv->hscrollbar);
   gtk_widget_hide (iv->vscrollbar);
   gtk_widget_hide (iv->nav_button);

   iv->priv->show_scrollbar = FALSE;
}


void
gimv_image_view_set_progressbar (GimvImageView *iv, GtkWidget *progressbar)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   iv->progressbar = progressbar;
}


void
gimv_image_view_set_player_visible (GimvImageView *iv,
                                    GimvImageViewPlayerVisibleType type)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   switch (type) {
   case GimvImageViewPlayerVisibleHide:
      gtk_widget_hide (iv->player_container);
      iv->priv->player_visible = GimvImageViewPlayerVisibleHide;
      break;
   case GimvImageViewPlayerVisibleShow:
      gtk_widget_show (iv->player_container);
      iv->priv->player_visible = GimvImageViewPlayerVisibleShow;
      break;
   case GimvImageViewPlayerVisibleAuto:
   default:
      if (gimv_image_view_is_playable (iv)) {
         gtk_widget_show (iv->player_container);
      } else {
         gtk_widget_hide (iv->player_container);
      }
      iv->priv->player_visible = GimvImageViewPlayerVisibleAuto;
      break;
   }
}


GimvImageViewPlayerVisibleType
gimv_image_view_get_player_visible (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);
   return iv->priv->player_visible;
}


static gboolean
cb_navwin_button_release  (GtkWidget *widget,
                           GdkEventButton *event,
                           GimvImageView *iv)
{
   GimvNavWin *navwin;
   GimvImageViewZoomType zoom_type = -1;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   switch (event->button) {
   case 4:
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_OUT;
      break;
   case 5:
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_IN;
      break;
   default:
      break;
   }

   if (zoom_type >= 0) {
      gint vx, vy;

      g_signal_handlers_block_by_func (G_OBJECT(widget),
                                       G_CALLBACK (cb_navwin_button_release),
                                       iv); 
      gimv_image_view_zoom_image (iv, zoom_type, 0, 0);
      g_signal_handlers_unblock_by_func (G_OBJECT(widget),
                                         G_CALLBACK (cb_navwin_button_release),
                                         iv); 

      gimv_nav_win_set_orig_image_size (navwin,
                                        iv->priv->width,
                                        iv->priv->height);
      gimv_image_view_get_view_position (iv, &vx, &vy);
      gimv_nav_win_set_view_position (navwin, vx, vy);
   }

   return FALSE;
}


#define ZOOM_KEY_NUM 12

static void
zoom_key_parse (guint keys[ZOOM_KEY_NUM], GdkModifierType mods[ZOOM_KEY_NUM])
{
   gchar **keyconfs[ZOOM_KEY_NUM] = {
      &akey.imgwin_zoomin,
      &akey.imgwin_zoomout,
      &akey.imgwin_fit_img,
      &akey.imgwin_zoom10,
      &akey.imgwin_zoom25,
      &akey.imgwin_zoom50,
      &akey.imgwin_zoom75,
      &akey.imgwin_zoom100,
      &akey.imgwin_zoom125,
      &akey.imgwin_zoom150,
      &akey.imgwin_zoom175,
      &akey.imgwin_zoom200,
   };
   gint i;

   if (!keys) return;
   if (!mods) return;

   for (i = 0; i < ZOOM_KEY_NUM; i++) {
      gchar *keyconf;

      keys[i] = mods[i] = 0;
      if (!keyconfs[i] || !*keyconfs[i] || !**keyconfs[i]) continue;
      keyconf = *keyconfs[i];

      gtk_accelerator_parse (keyconf, &keys[i], &mods[i]);
   }
}


static gboolean
cb_navwin_key_press (GtkWidget *widget, 
                     GdkEventKey *event,
                     GimvImageView *iv)
{
   GimvNavWin *navwin;
   GimvImageViewZoomType zoom_type = -1;
   guint zoom_key[ZOOM_KEY_NUM], keyval;
   GdkModifierType zoom_mod[ZOOM_KEY_NUM], modval;

   g_return_val_if_fail (GIMV_IS_NAV_WIN (widget), FALSE);
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   navwin = GIMV_NAV_WIN (widget);

   keyval = event->keyval;
   modval = event->state;

   zoom_key_parse (zoom_key, zoom_mod);

   if (keyval == GDK_equal
              || (keyval == zoom_key[0] && (!zoom_mod[0] || (modval & zoom_mod[0]))))
   {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_IN;
   } else if (event->keyval == GDK_minus
              || (keyval == zoom_key[1] && (!zoom_mod[1] || (modval & zoom_mod[1])))) 
   {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_OUT;
   } else if (keyval == zoom_key[3] && (!zoom_mod[3] || (modval & zoom_mod[3]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_10;
   } else if (keyval == zoom_key[4] && (!zoom_mod[4] || (modval & zoom_mod[4]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_25;
   } else if (keyval == zoom_key[5] && (!zoom_mod[5] || (modval & zoom_mod[5]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_50;
   } else if (keyval == zoom_key[6] && (!zoom_mod[6] || (modval & zoom_mod[6]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_75;
   } else if (keyval == zoom_key[7] && (!zoom_mod[7] || (modval & zoom_mod[7]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_100;
   } else if (keyval == zoom_key[8] && (!zoom_mod[8] || (modval & zoom_mod[8]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_125;
   } else if (keyval == zoom_key[9] && (!zoom_mod[9] || (modval & zoom_mod[9]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_150;
   } else if (keyval == zoom_key[10] && (!zoom_mod[10] || (modval & zoom_mod[10]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_175;
   } else if (keyval == zoom_key[11] && (!zoom_mod[11] || (modval & zoom_mod[11]))) {
      zoom_type = GIMV_IMAGE_VIEW_ZOOM_200;
   }

   if (zoom_type >= 0) {
      gint vx, vy;

      g_signal_handlers_block_by_func (G_OBJECT(widget),
                                       G_CALLBACK (cb_navwin_key_press),
                                       iv); 
      gimv_image_view_zoom_image (iv, zoom_type, 0, 0);
      g_signal_handlers_unblock_by_func (G_OBJECT(widget),
                                         G_CALLBACK (cb_navwin_key_press),
                                         iv); 

      gimv_nav_win_set_orig_image_size (navwin,
                                        iv->priv->width,
                                        iv->priv->height);
      gimv_image_view_get_view_position (iv, &vx, &vy);
      gimv_nav_win_set_view_position (navwin, vx, vy);
   }

   return FALSE;
}


static void
cb_navwin_move (GimvNavWin *navwin, gint x, gint y, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_NAV_WIN (navwin));
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_moveto (iv, x, y);
}


void
gimv_image_view_open_navwin (GimvImageView *iv, gint x_root, gint y_root)
{
   GtkWidget *navwin;
   GimvImage *image;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   gint src_width, src_height, dest_width, dest_height;
   gint fwidth, fheight;
   gint fpos_x, fpos_y;

   g_return_if_fail (iv);
   if (!iv->priv->pixmap) return;

   /* get pixmap for navigator */
   gdk_window_get_size (iv->priv->pixmap, &src_width, &src_height);
   image = gimv_image_create_from_drawable (iv->priv->pixmap, 0, 0,
                                            src_width, src_height);
   g_return_if_fail (image);

   if (src_width > src_height) {
      dest_width  = GIMV_NAV_WIN_SIZE;
      dest_height = src_height * GIMV_NAV_WIN_SIZE / src_width;
   } else {
      dest_height = GIMV_NAV_WIN_SIZE;
      dest_width  = src_width * GIMV_NAV_WIN_SIZE / src_height;
   }

   gimv_image_scale_get_pixmap (image, dest_width, dest_height,
                                &pixmap, &mask);
   if (!pixmap) goto ERROR;

   gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);
   gimv_image_view_get_view_position (iv, &fpos_x, &fpos_y);

   /* open navigate window */
   if (iv->priv->navwin) {
      navwin = iv->priv->navwin;
      gimv_nav_win_set_pixmap (GIMV_NAV_WIN (navwin), pixmap, mask,
                               iv->priv->width, iv->priv->height);
      gimv_nav_win_set_view_size (GIMV_NAV_WIN (navwin), fwidth, fheight);
      gimv_nav_win_set_view_position (GIMV_NAV_WIN (navwin), fpos_x, fpos_y);
      gimv_nav_win_show (GIMV_NAV_WIN (navwin), x_root, y_root);
   } else {
      navwin = gimv_nav_win_new (pixmap, mask,
                                 iv->priv->width, iv->priv->height,
                                 fwidth, fheight,
                                 fpos_x, fpos_y);
      g_signal_connect (G_OBJECT (navwin), "button_release_event",
                        G_CALLBACK (cb_navwin_button_release), iv);
      g_signal_connect (G_OBJECT (navwin), "key-press-event",
                        G_CALLBACK(cb_navwin_key_press), iv);
      g_signal_connect (G_OBJECT (navwin), "move",
                        G_CALLBACK (cb_navwin_move), iv);
      gimv_nav_win_show (GIMV_NAV_WIN (navwin), x_root, y_root);
      iv->priv->navwin = navwin;
   }

   /* free */
   gdk_pixmap_unref (pixmap);

ERROR:
   gimv_image_unref (image);
}


void
gimv_image_view_set_fullscreen (GimvImageView *iv, GtkWindow *fullscreen)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (GTK_IS_WINDOW (fullscreen));

   if (iv->priv->fullscreen) return;
   iv->priv->fullscreen = fullscreen;
   gtk_widget_reparent (iv->draw_area, GTK_WIDGET (iv->priv->fullscreen));
}


void
gimv_image_view_unset_fullscreen (GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->priv->fullscreen) return;

   gtk_widget_reparent (iv->draw_area, iv->table);
   iv->priv->fullscreen = NULL;
}



/*****************************************************************************
 *
 *  loading functions
 *
 *****************************************************************************/
void
gimv_image_view_free_image_buf (GimvImageView *iv)
{
   if (!iv)
      return;

   if (!iv->image) return;
   if (iv->priv->buffer) return;

#warning FIXME!!
   if (gimv_image_info_is_animation (iv->info)
       || gimv_image_info_is_movie (iv->info)
       || gimv_image_info_is_audio (iv->info))
   {
      return;
   }

   gimv_image_unref (iv->image);
   iv->image = NULL;
}
static gint
progress_timeout (gpointer data)
{
   gfloat new_val;
   GtkAdjustment *adj;

   adj = GTK_PROGRESS (data)->adjustment;

   new_val = adj->value + 1;
   if (new_val > adj->upper)
      new_val = adj->lower;

   gtk_progress_set_value (GTK_PROGRESS (data), new_val);

   return (TRUE);
}


static void
cb_loader_progress_update (GimvImageLoader *loader, GimvImageView *iv)
{
   while (gtk_events_pending()) gtk_main_iteration();
}


static void
cb_loader_load_end (GimvImageLoader *loader, GimvImageView *iv)
{
   GimvImage *image, *rgb_image;

   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   image = gimv_image_loader_get_image (loader);
   if (!image) goto ERROR;
   gimv_image_ref (image);
   gimv_image_loader_unref_image (loader);

   /* FIXME */
   if (gimv_image_has_alpha (image) && !GIMV_IS_ANIM (image)) {
      gint bg_r = 255, bg_g = 255, bg_b = 255;
      GtkStyle *style;

      style = gtk_widget_get_style (iv->draw_area);
      bg_r = style->bg[GTK_STATE_NORMAL].red   / 256;
      bg_g = style->bg[GTK_STATE_NORMAL].green / 256;
      bg_b = style->bg[GTK_STATE_NORMAL].blue  / 256;
      
      rgb_image = gimv_image_rgba2rgb (image,
                                       bg_r, bg_g, bg_b,
                                       iv->priv->ignore_alpha);
      if (rgb_image) {
         gimv_image_unref (image);
         image = rgb_image;
      }
   }

   if (!g_list_find (GimvImageViewList, iv)) {
      gimv_image_unref (image);
      goto ERROR;
   } else {
      if (iv->image)
         gimv_image_unref (iv->image);
      iv->image = image;
   }

   gimv_image_view_rotate_render (iv, iv->priv->rotate);

ERROR:
   g_signal_handlers_disconnect_by_func (G_OBJECT (iv->loader),
                                         G_CALLBACK (cb_loader_progress_update),
                                         iv);
   g_signal_handlers_disconnect_by_func (G_OBJECT (iv->loader),
                                         G_CALLBACK (cb_loader_load_end),
                                         iv);

   iv->priv->loader_progress_update_signal_id = 0;
   iv->priv->loader_load_end_signal_id        = 0;

   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[LOAD_END_SIGNAL], 0,
                  iv->info, FALSE);
}


static gboolean
gimv_image_view_need_load (GimvImageView *iv)
{
   gint width, height;

   if (!iv->image) return TRUE;

#if IMAGE_VIEW_ENABLE_SCALABLE_LOAD

   if (iv->priv->rotate == 0 || iv->priv->rotate == 2) {
      width  = gimv_image_width (iv->image);
      height = gimv_image_height (iv->image);
   } else {
      width  = gimv_image_height (iv->image);
      height = gimv_image_width  (iv->image);
   }

   if ( width == iv->info->width
       &&  height == iv->info->height)
   {
      /* full scale image was already loaded */
      return FALSE;

   } else {
      gint req_width, req_height;

      gimv_image_view_get_request_size (iv, &req_width, &req_height);

      if (req_width >= 0 && req_height >= 0
          && width  >= req_width
          && height >= req_height)
      {
         /* the image is bigger than request size */
         return FALSE;
      }
   }

   return TRUE;

#endif /* IMAGE_VIEW_ENABLE_SCALABLE_LOAD */

   return FALSE;
}


static void
gimv_image_view_load_image_buf_start (GimvImageView *iv)
{
   const gchar *filename;
   gint req_width, req_height;

   g_return_if_fail (iv);

   if (!iv->info) return;
   if (!g_list_find (GimvImageViewList, iv)) return;

   if (!gimv_image_view_need_load (iv)) return;

#warning FIXME!!
   if (gimv_image_info_is_archive (iv->info)
       || gimv_image_info_is_movie (iv->info)
       || gimv_image_info_is_audio (iv->info))
   {
      return;
   }

   filename = gimv_image_info_get_path (iv->info);
   if (!filename || !*filename) return;

   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[LOAD_START_SIGNAL], 0,
                  iv->info);

   if (gimv_image_info_is_in_archive (iv->info)) {
      guint timer = 0;

      /* set progress bar */
      if (iv->progressbar) {
         gtk_progress_set_activity_mode (GTK_PROGRESS (iv->progressbar), TRUE);
         timer = gtk_timeout_add (50,
                                  (GtkFunction) progress_timeout,
                                  iv->progressbar);
         gtk_grab_add (iv->progressbar);
      }

      /* extract */
      gimv_image_info_extract_archive (iv->info);

      /* unset progress bar */
      if (iv->progressbar) {
         gtk_timeout_remove (timer);
         gtk_progress_set_activity_mode (GTK_PROGRESS (iv->progressbar), FALSE);
         gtk_progress_bar_update (GTK_PROGRESS_BAR(iv->progressbar), 0.0);
         gtk_grab_remove (iv->progressbar);
      }
   }

   /* load image buf */
   /* iv->loader->flags |= GIMV_IMAGE_LOADER_DEBUG_FLAG; */
   if (iv->priv->loader_progress_update_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv->loader),
                                   iv->priv->loader_progress_update_signal_id);
   iv->priv->loader_progress_update_signal_id = 
      g_signal_connect (G_OBJECT (iv->loader), "progress_update",
                        G_CALLBACK (cb_loader_progress_update),
                        iv);
   if (iv->priv->loader_load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv->loader),
                                   iv->priv->loader_load_end_signal_id);
   iv->priv->loader_load_end_signal_id = 
      g_signal_connect (G_OBJECT (iv->loader), "load_end",
                        G_CALLBACK (cb_loader_load_end),
                        iv);

   gimv_image_loader_set_image_info (iv->loader, iv->info);
   gimv_image_loader_set_as_animation (iv->loader, TRUE);

#if IMAGE_VIEW_ENABLE_SCALABLE_LOAD
   gimv_image_view_get_request_size (iv, &req_width, &req_height);
   gimv_image_loader_set_size_request (iv->loader, req_width, req_height, TRUE);
#endif

   gimv_image_loader_load_start (iv->loader);
}


static void
cb_loader_load_restart (GimvImageLoader *loader, GimvImageView *iv)
{
   if (iv->priv->loader_load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv->loader),
                                   iv->priv->loader_load_end_signal_id);
   iv->priv->loader_load_end_signal_id = 0;
   gimv_image_view_load_image_buf_start (iv);
}


void
gimv_image_view_load_image_buf (GimvImageView *iv)
{
   if (gimv_image_loader_is_loading (iv->loader)) {
      if (iv->info == iv->loader->info) return;

      gimv_image_view_cancel_loading (iv);

      iv->priv->loader_load_end_signal_id
         = g_signal_connect (G_OBJECT (iv->loader), "load_end",
                             G_CALLBACK (cb_loader_load_restart),
                             iv);
   } else {
      gimv_image_view_load_image_buf_start (iv);
   }
}


gboolean
gimv_image_view_is_loading (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);
   g_return_val_if_fail (iv->loader, FALSE);

   return gimv_image_loader_is_loading (iv->loader);
}


void
gimv_image_view_cancel_loading (GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!gimv_image_loader_is_loading (iv->loader)) return;

   gimv_image_loader_load_stop (iv->loader);

   if (iv->priv->loader_progress_update_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv->loader),
                                   iv->priv->loader_progress_update_signal_id);
   iv->priv->loader_progress_update_signal_id = 0;

   if (iv->priv->loader_load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv->loader),
                                   iv->priv->loader_load_end_signal_id);
   iv->priv->loader_load_end_signal_id = 0;

   /*
   g_signal_emit (G_OBJECT(iv),
                  gimv_image_view_signals[LOAD_END_SIGNAL], 0,
                  iv->info, TRUE);
   */
}



/*****************************************************************************
 *
 *   scalable interface functions
 *
 *****************************************************************************/
void
gimv_image_view_zoom_image (GimvImageView *iv, GimvImageViewZoomType zoom,
                            gfloat x_scale, gfloat y_scale)
{
   gint cx_pos, cy_pos, fwidth, fheight;
   gfloat src_x_scale, src_y_scale;

   g_return_if_fail (iv);

   src_x_scale = iv->priv->x_scale;
   src_y_scale = iv->priv->y_scale;

   iv->priv->fit_to_frame = 0;

   switch (zoom) {
   case GIMV_IMAGE_VIEW_ZOOM_IN:
      if (iv->priv->x_scale < GIMV_IMAGE_VIEW_MAX_SCALE
          && iv->priv->y_scale < GIMV_IMAGE_VIEW_MAX_SCALE)
      {
         iv->priv->x_scale = iv->priv->x_scale + GIMV_IMAGE_VIEW_MIN_SCALE;
         iv->priv->y_scale = iv->priv->y_scale + GIMV_IMAGE_VIEW_MIN_SCALE;
      }
      break;
   case GIMV_IMAGE_VIEW_ZOOM_OUT:
      if (iv->priv->x_scale > GIMV_IMAGE_VIEW_MIN_SCALE
          && iv->priv->y_scale > GIMV_IMAGE_VIEW_MIN_SCALE)
      {
         iv->priv->x_scale = iv->priv->x_scale - GIMV_IMAGE_VIEW_MIN_SCALE;
         iv->priv->y_scale = iv->priv->y_scale - GIMV_IMAGE_VIEW_MIN_SCALE;
      }
      break;
   case GIMV_IMAGE_VIEW_ZOOM_FIT:
   case GIMV_IMAGE_VIEW_ZOOM_FIT_ZOOM_OUT_ONLY:
   case GIMV_IMAGE_VIEW_ZOOM_FIT_WIDTH:
   case GIMV_IMAGE_VIEW_ZOOM_FIT_HEIGHT:
      iv->priv->fit_to_frame = zoom;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_10:
      iv->priv->x_scale = iv->priv->y_scale =  10;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_25:
      iv->priv->x_scale = iv->priv->y_scale =  25;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_50:
      iv->priv->x_scale = iv->priv->y_scale =  50;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_75:
      iv->priv->x_scale = iv->priv->y_scale =  75;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_100:
      iv->priv->x_scale = iv->priv->y_scale = 100;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_125:
      iv->priv->x_scale = iv->priv->y_scale = 125;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_150:
      iv->priv->x_scale = iv->priv->y_scale = 150;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_175:
      iv->priv->x_scale = iv->priv->y_scale = 175;
      break;
   case GIMV_IMAGE_VIEW_ZOOM_200:
      iv->priv->x_scale = iv->priv->y_scale = 200;
      break;	 
   case GIMV_IMAGE_VIEW_ZOOM_300:
      iv->priv->x_scale = iv->priv->y_scale = 300;
      break;	 
   case GIMV_IMAGE_VIEW_ZOOM_FREE:
      iv->priv->x_scale = x_scale;
      iv->priv->y_scale = y_scale;
      break;
   default:
      break;
   }

   gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);

   cx_pos = (iv->priv->x_pos - fwidth  / 2) * iv->priv->x_scale / src_x_scale;
   cy_pos = (iv->priv->y_pos - fheight / 2) * iv->priv->y_scale / src_y_scale;

   iv->priv->x_pos = cx_pos + fwidth  / 2;
   iv->priv->y_pos = cy_pos + fheight / 2;

   gimv_image_view_show_image (iv);
}


gboolean
gimv_image_view_get_image_size (GimvImageView   *iv,
                                gint        *width,
                                gint        *height)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   if (width)
      *width = iv->priv->width;
   if (height)
      *height = iv->priv->height;

   return TRUE;
}


static void
gimv_image_view_get_request_size (GimvImageView *iv,
                                  gint *width_ret, gint *height_ret)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (width_ret && height_ret);

   *width_ret  = -1;
   *height_ret = -1;

   if (iv->priv->fit_to_frame) {
      gint fwidth, fheight;

      gimv_image_view_get_image_frame_size (iv, &fwidth, &fheight);

      if (iv->priv->rotate == 0 || iv->priv->rotate == 2) {
         *width_ret  = fwidth;
         *height_ret = fheight;
      } else {
         *width_ret  = fheight;
         *height_ret = fwidth;
      }

      return;

   } else if (iv->priv->width > MIN_IMAGE_WIDTH && iv->priv->height > MIN_IMAGE_HEIGHT) {
      *width_ret  = iv->info->width  * iv->priv->x_scale / 100.0;
      *height_ret = iv->info->height * iv->priv->y_scale / 100.0;
      return;
   } else {
      /*
       * the image was changed, so the image size may be unknown yet.
       * (iv->info will know it, but (currently) we don't trust it yet. )
       */
   }
}



/*****************************************************************************
 *
 *   rotatable interface functions
 *
 *****************************************************************************/
static void
gimv_image_view_rotate_render (GimvImageView *iv,
                               GimvImageViewOrientation angle)
{
   switch (angle) {
   case GIMV_IMAGE_VIEW_ROTATE_90:
      iv->image = gimv_image_rotate_90 (iv->image, TRUE);
      break;
   case GIMV_IMAGE_VIEW_ROTATE_180:
      iv->image = gimv_image_rotate_180 (iv->image);
      break;
   case GIMV_IMAGE_VIEW_ROTATE_270:
      iv->image = gimv_image_rotate_90 (iv->image, FALSE);
      break;
   default:
      break;
   }
}


typedef struct RotateData_Tag {
   GimvImageViewOrientation angle;
} RotateData;


static void
cb_gimv_image_view_rotate_load_end (GimvImageView *iv, GimvImageInfo *info,
                                    gboolean cancel, gpointer user_data)
{
   RotateData *data = user_data;
   GimvImageViewOrientation angle = data->angle, rotate_angle;
   gboolean can_draw;
   gchar *path, *cache;

   if (iv->priv->load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv), iv->priv->load_end_signal_id);
   iv->priv->load_end_signal_id = 0;

   if (cancel) return;

   can_draw = check_can_draw_image (iv);
   if (!can_draw) goto func_end;

   /* rotate image */
   rotate_angle = (angle - iv->priv->rotate) % 4;
   if (rotate_angle < 0)
      rotate_angle = rotate_angle + 4;

   iv->priv->rotate = angle;
   gimv_image_view_rotate_render (iv, rotate_angle);

   /* set image size */
   gimv_image_view_calc_image_size (iv);

   /* image rendering */
   gimv_image_free_pixmap_and_mask (iv->priv->pixmap, iv->priv->mask);
   gimv_image_scale_get_pixmap (iv->image,
                                iv->priv->width, iv->priv->height,
                                &iv->priv->pixmap, &iv->priv->mask);

   /* reset image geometory */
   gimv_image_view_adjust_pos_in_frame (iv, TRUE);

   /* draw image */
   gimv_image_view_draw_image (iv);

   /* create thumbnail if not exist */
   path  = gimv_image_info_get_path_with_archive (iv->info);
   cache = gimv_thumb_find_thumbcache(path, NULL);
   if (path && *path && !cache) {
      gimv_image_view_create_thumbnail (iv);
   }
   g_free (path);
   g_free (cache);
   path  = NULL;
   cache = NULL;

   gimv_image_view_free_image_buf (iv);

func_end:
   if (iv->priv->load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv), iv->priv->load_end_signal_id);
   iv->priv->load_end_signal_id = 0;

   g_signal_emit (G_OBJECT(iv), gimv_image_view_signals[RENDERED_SIGNAL], 0);
}


void
gimv_image_view_rotate_image (GimvImageView *iv, GimvImageViewOrientation angle)
{
   RotateData *data;

   if (iv->priv->load_end_signal_id)
      g_signal_handler_disconnect (G_OBJECT (iv), iv->priv->load_end_signal_id);

   data = g_new0 (RotateData, 1);
   data->angle = angle;

   if (gimv_image_view_need_load (iv)) {
      iv->priv->load_end_signal_id
         = g_signal_connect_data (G_OBJECT (iv), "load_end",
                                  G_CALLBACK (cb_gimv_image_view_rotate_load_end),
                                  data, (GClosureNotify) g_free, 0);

      gimv_image_view_load_image_buf (iv);
   } else {
      cb_gimv_image_view_rotate_load_end (iv, iv->info, FALSE, data);
      g_free (data);
   }
}


void
gimv_image_view_rotate_ccw (GimvImageView *iv)
{
   guint angle;

   g_return_if_fail (iv);

   /* convert to absolute angle */
   angle = (iv->priv->rotate + 1) % 4;

   gimv_image_view_rotate_image (iv, angle);
}


void
gimv_image_view_rotate_cw (GimvImageView *iv)
{
   guint angle;

   g_return_if_fail (iv);

   /* convert to absolute angle */
   angle = (iv->priv->rotate + 3) % 4;

   gimv_image_view_rotate_image (iv, angle);
}


GimvImageViewOrientation
gimv_image_view_get_orientation (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);
   return iv->priv->rotate;
}



/*****************************************************************************
 *
 *   scrollable interface functions
 *
 *****************************************************************************/
void
gimv_image_view_get_image_frame_size (GimvImageView *iv, gint *width, gint *height)
{
   g_return_if_fail (width && height);

   *width = *height = 0;

   g_return_if_fail (iv);

   if (iv->priv->fullscreen) {
      *width  = GTK_WIDGET(iv->priv->fullscreen)->allocation.width;
      *height = GTK_WIDGET(iv->priv->fullscreen)->allocation.height;
   } else {      
      *width  = iv->draw_area->allocation.width;
      *height = iv->draw_area->allocation.height;
   }
}


gboolean
gimv_image_view_get_view_position (GimvImageView *iv, gint *x, gint *y)
{
   g_return_val_if_fail (iv, FALSE);
   g_return_val_if_fail (x && y, FALSE);

   *x = 0 - iv->priv->x_pos;
   *y = 0 - iv->priv->y_pos;

   return TRUE;
}


void
gimv_image_view_moveto (GimvImageView *iv, gint x, gint y)
{
   g_return_if_fail (iv);

   iv->priv->x_pos = 0 - x;
   iv->priv->y_pos = 0 - y;

   gimv_image_view_adjust_pos_in_frame (iv, TRUE);

   gimv_image_view_draw_image (iv);
}


void
gimv_image_view_reset_scrollbar (GimvImageView *iv)
{
   g_return_if_fail (iv);
   g_return_if_fail (iv->draw_area);
   g_return_if_fail (iv->hadj);
   g_return_if_fail (iv->vadj);

   /* horizontal */
   if (iv->priv->x_pos < 0)
      iv->hadj->value = 0 - iv->priv->x_pos;
   else
      iv->hadj->value = 0;

   if (iv->priv->width > iv->draw_area->allocation.width)
      iv->hadj->upper = iv->priv->width;
   else
      iv->hadj->upper = iv->draw_area->allocation.width;
   iv->hadj->page_size = iv->draw_area->allocation.width;

   /* vertical */
   if (iv->priv->y_pos < 0)
      iv->vadj->value = 0 - iv->priv->y_pos;
   else
      iv->vadj->value = 0;

   if (iv->priv->height > iv->draw_area->allocation.height)
      iv->vadj->upper = iv->priv->height;
   else
      iv->vadj->upper = iv->draw_area->allocation.height;
   iv->vadj->page_size = iv->draw_area->allocation.height;

   move_scrollbar_by_user = FALSE;

   g_signal_emit_by_name (G_OBJECT(iv->hadj), "changed");
   g_signal_emit_by_name (G_OBJECT(iv->vadj), "changed");

   move_scrollbar_by_user = TRUE;
}



/*****************************************************************************
 *
 *   playable interface functions
 *
 *****************************************************************************/
gboolean
gimv_image_view_is_playable (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);

   if (!iv->info) return FALSE;
   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return FALSE;

   playable = iv->draw_area_funcs->playable;

   if (!playable->is_playable_fn) return FALSE;

   return playable->is_playable_fn (iv, iv->info);
}


void
gimv_image_view_playable_play (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;
   playable = iv->draw_area_funcs->playable;

   if (gimv_image_view_is_playable (iv)) {
      g_return_if_fail (playable->play_fn);
      playable->play_fn (iv);
   }
}


void
gimv_image_view_playable_stop (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->stop_fn) return;
   playable->stop_fn (iv);
}


void
gimv_image_view_playable_pause (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->pause_fn) return;
   playable->pause_fn (iv);
}


void
gimv_image_view_playable_forward (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->forward_fn) return;
   playable->forward_fn (iv);
}


void
gimv_image_view_playable_reverse (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->reverse_fn) return;
   playable->reverse_fn (iv);
}


void
gimv_image_view_playable_seek (GimvImageView *iv, guint pos)
{
   GimvImageViewPlayableIF *playable;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->seek_fn) return;
   playable->seek_fn (iv, pos);
}


void
gimv_image_view_playable_eject (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;


   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (!playable->eject_fn) return;
   playable->eject_fn (iv);
}


GimvImageViewPlayableStatus
gimv_image_view_playable_get_status (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), GimvImageViewPlayableDisable);

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable)
      return GimvImageViewPlayableDisable;

   playable = iv->draw_area_funcs->playable;

   if (!playable->get_status_fn) return GimvImageViewPlayableDisable;
   return playable->get_status_fn (iv);
}


guint
gimv_image_view_playable_get_length (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return 0;

   playable = iv->draw_area_funcs->playable;

   if (!playable->get_length_fn) return 0;
   return playable->get_length_fn (iv);
}


guint
gimv_image_view_playable_get_position (GimvImageView *iv)
{
   GimvImageViewPlayableIF *playable;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);

   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return 0;

   playable = iv->draw_area_funcs->playable;

   if (!playable->get_position_fn) return 0;
   return playable->get_position_fn (iv);
}



/*****************************************************************************
 *
 *   list interface functions
 *
 *****************************************************************************/
void
gimv_image_view_set_list (GimvImageView       *iv,
                          GList           *list,
                          GList           *current,
                          gpointer         list_owner,
                          GimvImageViewNextFn  next_fn,
                          GimvImageViewPrevFn  prev_fn,
                          GimvImageViewNthFn   nth_fn,
                          GimvImageViewRemoveListFn remove_list_fn,
                          gpointer         list_fn_user_data)
{
   g_return_if_fail (iv);
   g_return_if_fail (list);
   g_return_if_fail (current);

   if (!g_list_find (GimvImageViewList, iv)) return;

   if (iv->priv->image_list)
      gimv_image_view_remove_list (iv, iv->priv->image_list->owner);

   iv->priv->image_list = g_new0 (GimvImageViewImageList, 1);

   iv->priv->image_list->list = list;
   if (!current)
      iv->priv->image_list->current = list;
   else
      iv->priv->image_list->current = current;

   iv->priv->image_list->owner             = list_owner;
   iv->priv->image_list->next_fn           = next_fn;
   iv->priv->image_list->prev_fn           = prev_fn;
   iv->priv->image_list->nth_fn            = nth_fn;
   iv->priv->image_list->remove_list_fn    = remove_list_fn;
   iv->priv->image_list->list_fn_user_data = list_fn_user_data;

   g_signal_emit (G_OBJECT(iv), gimv_image_view_signals[SET_LIST_SIGNAL], 0);
}


void
gimv_image_view_remove_list (GimvImageView *iv, gpointer list_owner)
{
   g_return_if_fail (iv);

   if (!iv->priv->image_list) return;

   if (iv->priv->image_list->remove_list_fn
       && iv->priv->image_list->owner == list_owner)
   {
      iv->priv->image_list->remove_list_fn
         (iv,
          iv->priv->image_list->owner,
          iv->priv->image_list->list_fn_user_data);
   }

   g_free (iv->priv->image_list);
   iv->priv->image_list = NULL;

   g_signal_emit (G_OBJECT(iv), gimv_image_view_signals[UNSET_LIST_SIGNAL], 0);
}


gint
gimv_image_view_image_list_length (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);
   if (!iv->priv->image_list) return 0;

   return g_list_length (iv->priv->image_list->list);
}


gint
gimv_image_view_image_list_position (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);
   if (!iv->priv->image_list) return 0;

   return g_list_position (iv->priv->image_list->list,
                           iv->priv->image_list->current);
}


GList *
gimv_image_view_image_list_current (GimvImageView *iv)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), NULL);
   if (!iv->priv->image_list) return NULL;

   return iv->priv->image_list->current;
}


static gint
idle_gimv_image_view_next (gpointer data)
{
   GimvImageView *iv = data;

   g_return_val_if_fail (iv, FALSE);
   if (!iv->priv->image_list) return FALSE;
   if (!iv->priv->image_list->next_fn) return FALSE;

   iv->priv->image_list->current
      = iv->priv->image_list->next_fn (iv,
                                       iv->priv->image_list->owner,
                                       iv->priv->image_list->current,
                                       iv->priv->image_list->list_fn_user_data);

   return FALSE;
}


void
gimv_image_view_next (GimvImageView *iv)
{
   g_return_if_fail (iv);
   gtk_idle_add (idle_gimv_image_view_next, iv);
}


static gint
idle_gimv_image_view_prev (gpointer data)
{
   GimvImageView *iv = data;

   g_return_val_if_fail (iv, FALSE);
   if (!iv->priv->image_list) return FALSE;
   if (!iv->priv->image_list->prev_fn) return FALSE;

   iv->priv->image_list->current
      = iv->priv->image_list->prev_fn (iv,
                                       iv->priv->image_list->owner,
                                       iv->priv->image_list->current,
                                       iv->priv->image_list->list_fn_user_data);

   return FALSE;
}


void
gimv_image_view_prev (GimvImageView *iv)
{
   g_return_if_fail (iv);

   gtk_idle_add (idle_gimv_image_view_prev, iv);
}


typedef struct NthFnData_Tag {
   GimvImageView *iv;
   gint       nth;
} NthFnData;


static gint
idle_gimv_image_view_nth (gpointer data)
{
   NthFnData *nth_fn_data = data;
   GimvImageView *iv = data;
   gint nth;

   g_return_val_if_fail (nth_fn_data, FALSE);

   iv = nth_fn_data->iv;
   nth = nth_fn_data->nth;

   g_free (nth_fn_data);
   nth_fn_data = NULL;
   data = NULL;

   g_return_val_if_fail (iv, FALSE);
   if (!iv->priv->image_list) return FALSE;
   if (!iv->priv->image_list->nth_fn) return FALSE;

   iv->priv->image_list->current
      = iv->priv->image_list->nth_fn (iv,
                                      iv->priv->image_list->owner,
                                      iv->priv->image_list->list,
                                      nth,
                                      iv->priv->image_list->list_fn_user_data);

   return FALSE;
}


void
gimv_image_view_nth (GimvImageView *iv, guint nth)
{
   NthFnData *nth_fn_data;

   g_return_if_fail (iv);

   nth_fn_data = g_new (NthFnData, 1);
   nth_fn_data->iv  = iv;
   nth_fn_data->nth = nth;

   gtk_idle_add (idle_gimv_image_view_nth, nth_fn_data);
}


static GList *
next_image (GimvImageView *iv,
            gpointer list_owner,
            GList *current,
            gpointer data)
{
   GList *next, *node;

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (iv == list_owner, NULL);
   g_return_val_if_fail (current, NULL);

   if (!iv->priv->image_list) return NULL;

   node = g_list_find (iv->priv->image_list->list, current->data);
   g_return_val_if_fail (node, NULL);

   next = g_list_next (current);
   if (!next)
      next = current;
   g_return_val_if_fail (next, NULL);

   if (next != current) {
      gimv_image_view_change_image (iv, next->data);
   }

   return next;
}


static GList *
prev_image (GimvImageView *iv,
            gpointer list_owner,
            GList *current,
            gpointer data)
{
   GList *prev, *node;

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (iv == list_owner, NULL);
   g_return_val_if_fail (current, NULL);

   if (!iv->priv->image_list) return NULL;

   node = g_list_find (iv->priv->image_list->list, current->data);
   g_return_val_if_fail (node, NULL);

   prev = g_list_previous (current);
   if (!prev)
      prev = current;
   g_return_val_if_fail (prev, NULL);

   if (prev != current) {
      gimv_image_view_change_image (iv, prev->data);
   }

   return prev;
}


static GList *
nth_image (GimvImageView *iv,
           gpointer list_owner,
           GList *list,
           guint nth,
           gpointer data)
{
   GList *node;

   g_return_val_if_fail (iv, NULL);
   g_return_val_if_fail (iv == list_owner, NULL);

   if (!iv->priv->image_list) return NULL;

   node = g_list_nth (iv->priv->image_list->list, nth);
   g_return_val_if_fail (node, NULL);

   gimv_image_view_change_image (iv, node->data);

   return node;
}


static void
remove_list (GimvImageView *iv, gpointer list_owner, gpointer data)
{
   g_return_if_fail (iv == list_owner);

   if (!iv->priv->image_list) return;

   g_list_foreach (iv->priv->image_list->list, (GFunc) gimv_image_info_unref, NULL);
   g_list_free (iv->priv->image_list->list);
   iv->priv->image_list->list = NULL;
   iv->priv->image_list->current = NULL;
}


void
gimv_image_view_set_list_self (GimvImageView *iv, GList *list, GList *current)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (list);
 
   if (!current || !g_list_find (list, current->data))
      current = list;

   gimv_image_view_set_list (iv, list, current, iv,
                       next_image,
                       prev_image,
                       nth_image,
                       remove_list,
                       iv);
}


gboolean
gimv_image_view_has_list (GimvImageView *iv)
{
   if (iv->priv->image_list) {
      return TRUE;
   } else {
      return FALSE;
   }
}

void
gimv_image_view_playable_set_status (GimvImageView *iv,
                                     GimvImageViewPlayableStatus status)
{
   GimvImageViewPlayableIF *playable;
   GtkWidget *play, *stop, *pause, *forward, *reverse, *eject;
   GtkItemFactory *ifactory;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   if (!iv->draw_area_funcs || !iv->draw_area_funcs->playable) return;

   playable = iv->draw_area_funcs->playable;

   if (iv->movie_menu && GTK_IS_WIDGET (iv->movie_menu)) {
      ifactory = gtk_item_factory_from_widget (iv->movie_menu);
      play     = gtk_item_factory_get_item (ifactory, "/Play");
      stop     = gtk_item_factory_get_item (ifactory, "/Stop");
      pause    = gtk_item_factory_get_item (ifactory, "/Pause");
      forward  = gtk_item_factory_get_item (ifactory, "/Forward");
      reverse  = gtk_item_factory_get_item (ifactory, "/Reverse");
      eject    = gtk_item_factory_get_item (ifactory, "/Eject");
   } else {
      play = stop = pause = forward = reverse = eject = NULL;
   }

   if (status == GimvImageViewPlayableDisable) {
      gimv_icon_stock_change_widget_icon  (iv->player.play_icon, "play");
      gtk_widget_set_sensitive (iv->player.play,    FALSE);
      gtk_widget_set_sensitive (iv->player.stop,    FALSE);
      gtk_widget_set_sensitive (iv->player.fw,      FALSE);
      gtk_widget_set_sensitive (iv->player.rw,      FALSE);
      gtk_widget_set_sensitive (iv->player.eject,   FALSE);
      gtk_widget_set_sensitive (iv->player.seekbar, FALSE);
      if (play)    gtk_widget_set_sensitive (play,    FALSE);
      if (stop)    gtk_widget_set_sensitive (stop,    FALSE);
      if (pause)   gtk_widget_set_sensitive (pause,   FALSE);
      if (forward) gtk_widget_set_sensitive (forward, FALSE);
      if (reverse) gtk_widget_set_sensitive (reverse, FALSE);
      if (eject)   gtk_widget_set_sensitive (eject, FALSE);
      gimv_image_view_playable_set_position (iv, 0.0);
      return;

   } else {
      gboolean enable;

      enable = playable->play_fn ? TRUE : FALSE;
      gtk_widget_set_sensitive (iv->player.play,  enable);
      if (play) gtk_widget_set_sensitive (play, enable);

      enable = playable->pause_fn ? TRUE : FALSE;
      if (pause) gtk_widget_set_sensitive (pause, enable);

      enable = playable->stop_fn ? TRUE : FALSE;
      gtk_widget_set_sensitive (iv->player.stop,  enable);
      if (stop) gtk_widget_set_sensitive (stop, enable);

      enable = playable->forward_fn ? TRUE : FALSE;
      gtk_widget_set_sensitive (iv->player.fw,    enable);
      if (forward) gtk_widget_set_sensitive (forward, enable);

      enable = playable->reverse_fn ? TRUE : FALSE;
      gtk_widget_set_sensitive (iv->player.rw,    enable);
      if (reverse) gtk_widget_set_sensitive (reverse, enable);

      enable = playable->eject_fn ? TRUE : FALSE;
      gtk_widget_set_sensitive (iv->player.eject, enable);
      if (eject) gtk_widget_set_sensitive (eject, enable);

      if (playable->is_seekable_fn
          && playable->is_seekable_fn (iv)
          && playable->seek_fn)
      {
         gtk_widget_set_sensitive (iv->player.seekbar, TRUE);
      }
   }

   switch (status) {
   case GimvImageViewPlayableStop:
      gimv_image_view_playable_set_position (iv, 0.0);
      gtk_widget_set_sensitive (iv->player.stop,    FALSE);
      gtk_widget_set_sensitive (iv->player.fw,      FALSE);
      gtk_widget_set_sensitive (iv->player.rw,      FALSE);
      if (stop)    gtk_widget_set_sensitive (stop,    FALSE);
      if (pause)   gtk_widget_set_sensitive (pause,   FALSE);
      if (forward) gtk_widget_set_sensitive (forward, FALSE);
      if (reverse) gtk_widget_set_sensitive (reverse, FALSE);
      break;
   case GimvImageViewPlayableForward:
   case GimvImageViewPlayableReverse:
      gtk_widget_set_sensitive (iv->player.fw,      FALSE);
      gtk_widget_set_sensitive (iv->player.rw,      FALSE);
      if (pause)   gtk_widget_set_sensitive (pause,   FALSE);
      if (forward) gtk_widget_set_sensitive (forward, FALSE);
      if (reverse) gtk_widget_set_sensitive (reverse, FALSE);
      break;
   case GimvImageViewPlayablePlay:
      if (!playable->pause_fn) {
         gtk_widget_set_sensitive (iv->player.play, FALSE);
         if (pause) gtk_widget_set_sensitive (pause, FALSE);
      }
      break;
   case GimvImageViewPlayablePause:
   default:
      break;
   }

   if (status == GimvImageViewPlayablePlay && playable->pause_fn) {
      gimv_icon_stock_change_widget_icon (iv->player.play_icon, "pause");
   } else {
      gimv_icon_stock_change_widget_icon (iv->player.play_icon, "play");
   }
}


void
gimv_image_view_playable_set_position (GimvImageView *iv, gfloat pos)
{
   GtkAdjustment *adj;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   adj = gtk_range_get_adjustment (GTK_RANGE (iv->player.seekbar));

   if (iv->priv && !(iv->priv->player_flags & GimvImageViewSeekBarDraggingFlag)) {
      adj->value = pos;
      g_signal_emit_by_name (G_OBJECT(adj), "value_changed"); 
   }

}
