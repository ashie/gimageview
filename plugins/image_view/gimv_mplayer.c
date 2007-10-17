/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GimvMPlayer -- a GTK+ mplayer's embedder
 * Copyright (C) 2002 Takuro Ashie <ashie@homa.ne.jp>
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
 */

/*
 * These codes are based on the GtkPlayer (http://gtkplayerembed.sourceforge.net/).
 * Copyright (C) 2002 Colin Leroy <colin@colino.net>
 */

#include "gimv_mplayer.h"

#ifdef ENABLE_MPLAYER

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <gtk/gtk.h>

#if (GTK_MAJOR_VERSION == 1) && (GTK_MAJOR_VERION <= 2)
#  ifndef GDK_WINDOWING_X11
#     define GDK_WINDOWING_X11
#  endif
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif /* GDK_WINDOWING_X11 */


#define GIMV_MPLAYER_REFRESH_RATE 100 /* 0.1 [sec] */
#define GIMV_MPLAYER_BUF_SIZE 1024

enum {
   PLAY_SIGNAL,
   STOP_SIGNAL,
   PAUSE_SIGNAL,
   POS_CHANGED_SIGNAL,
   IDENTIFIED_SIGNAL,
   LAST_SIGNAL
};


typedef struct ChildContext_Tag ChildContext;
typedef void (*ProcessLineFunc) (ChildContext *context,
                                 const gchar  *line,
                                 gint          len,
                                 gboolean      is_stderr);

struct ChildContext_Tag
{
   pid_t  pid;
   GList *args;
   gint   checker_id;

   GimvMPlayer     *player;

   int              stdout_fd;
   int              stderr_fd;
   int              stdin_fd;

   gchar            stdout_buf[GIMV_MPLAYER_BUF_SIZE];
   gint             stdout_size;
   gchar            stderr_buf[GIMV_MPLAYER_BUF_SIZE];
   gint             stderr_size;

   ProcessLineFunc  process_line_fn;
   gpointer         data;
   GtkDestroyNotify destroy_fn;
};


typedef struct  GetDriversContext_Tag
{
   const gchar *separator;
   gint         sep_len;
   gboolean     passing_header;
   GList       *drivers_list;
} GetDriversContext;


/* object class */
static void     gimv_mplayer_destroy       (GtkObject        *object);

/* widget class */
static void     gimv_mplayer_realize       (GtkWidget        *widget);
static void     gimv_mplayer_unrealize     (GtkWidget        *widget);
static void     gimv_mplayer_size_allocate (GtkWidget        *widget,
                                            GtkAllocation    *allocation);

/* mplayer class */
static ChildContext *gimv_mplayer_get_player_child (GimvMPlayer *player);
static void          gimv_mplayer_send_command     (GimvMPlayer *player,
                                                    const gchar *command);


/* MPlayer handling */
static ChildContext *start_command         (GimvMPlayer      *player,
                                            /* include command string */
                                            GList            *arg_list,
                                            const gchar      *work_dir,
                                            gboolean          main_iterate,
                                            ProcessLineFunc   func,
                                            gpointer          data,
                                            GtkDestroyNotify  destroy_fn);
static gint     timeout_check_child        (gpointer          data);
static gboolean process_output             (ChildContext     *context);

static void     child_context_destroy      (ChildContext     *context);

/* line process functions */
static void     process_line               (ChildContext     *context,
                                            const gchar      *line,
                                            gint              len,
                                            gboolean          is_stderr);
static void     process_line_get_drivers   (ChildContext     *context,
                                            const gchar      *line,
                                            gint              len,
                                            gboolean          is_stderr);
static void     process_line_identify      (ChildContext     *context,
                                            const gchar      *line,
                                            gint              len,
                                            gboolean          is_stderr);


static gint gimv_mplayer_signals[LAST_SIGNAL] = {0};

static GHashTable *player_context_table = NULL;
static GHashTable *vo_drivers_table     = NULL;
static GHashTable *ao_drivers_table     = NULL;


G_DEFINE_TYPE (GimvMPlayer, gimv_mplayer, GTK_TYPE_WIDGET)


static void
gimv_mplayer_class_init (GimvMPlayerClass *class)
{
   GtkObjectClass *object_class;
   GtkWidgetClass *widget_class;

   object_class = (GtkObjectClass *) class;
   widget_class = (GtkWidgetClass *) class;

   gimv_mplayer_signals[PLAY_SIGNAL]
      = g_signal_new ("play",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvMPlayerClass, play),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_mplayer_signals[STOP_SIGNAL]
      = g_signal_new ("stop",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvMPlayerClass, stop),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_mplayer_signals[PAUSE_SIGNAL]
      = g_signal_new ("pause",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvMPlayerClass, pause),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_mplayer_signals[POS_CHANGED_SIGNAL]
      = g_signal_new ("position-chaned",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvMPlayerClass, position_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_mplayer_signals[IDENTIFIED_SIGNAL]
      = g_signal_new ("identified",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvMPlayerClass, identified),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   /* object class methods */
   object_class->destroy       = gimv_mplayer_destroy;

   /* widget class methods */
   widget_class->realize       = gimv_mplayer_realize;
   widget_class->unrealize     = gimv_mplayer_unrealize;
   widget_class->size_allocate = gimv_mplayer_size_allocate;

   /* mplayer class methods */
   class->play                 = NULL;
   class->stop                 = NULL;
   class->pause                = NULL;
   class->position_changed     = NULL;
   class->identified           = NULL;
}



/******************************************************************************
 *
 *   object class methods.
 *
 ******************************************************************************/
static void
gimv_mplayer_media_info_init (GimvMPlayerMediaInfo *info)
{
   g_return_if_fail (info);

   info->length = -1.0;
   info->video  = NULL;
   info->audio  = NULL;
}


static void
gimv_mplayer_media_info_delete (GimvMPlayerMediaInfo *info)
{
   info->length = -1.0;

   g_free (info->video);
   info->video = NULL;

   g_free (info->audio);
   info->audio = NULL;
}


static void
gimv_mplayer_init (GimvMPlayer *player)
{
   player->filename   = NULL;
   player->pos        = 0.0;
   player->speed      = 1.0;
   player->status     = GimvMPlayerStatusStop;
   player->flags      = GimvMPlayerEmbedFlag
                        | GimvMPlayerAllowFrameDroppingFlag;

   player->command    = g_strdup ("mplayer");
   player->vo         = NULL;
   player->ao         = NULL;
   player->args       = NULL;

   if (GIMV_MPLAYER_INCLUDE_FILE)
      player->include_file = g_strdup (GIMV_MPLAYER_INCLUDE_FILE);
   else
      player->include_file = NULL;

   gimv_mplayer_media_info_init (&player->media_info);
}


static void
gimv_mplayer_destroy (GtkObject *object)
{
   GimvMPlayer *player = GIMV_MPLAYER (object);

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   gimv_mplayer_flush_tmp_files (player);
   gimv_mplayer_media_info_delete (&player->media_info);

   g_free (player->filename);
   player->filename = NULL;

   g_free (player->command);
   player->command = NULL;

   g_free (player->vo);
   player->vo = NULL;

   g_free (player->ao);
   player->ao = NULL;

   g_free (player->include_file);
   player->include_file = NULL;

   /* FIXME: free player->args */

   if (GTK_OBJECT_CLASS (gimv_mplayer_parent_class)->destroy)
      GTK_OBJECT_CLASS (gimv_mplayer_parent_class)->destroy (object);
}



/******************************************************************************
 *
 *   widget class methods.
 *
 ******************************************************************************/
static void gimv_mplayer_send_configure (GimvMPlayer *player);

static void
gimv_mplayer_realize (GtkWidget *widget)
{
   GimvMPlayer *player;
   GdkWindowAttr attributes;
   gint attributes_mask;

   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_MPLAYER (widget));

   player = GIMV_MPLAYER (widget);
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
   gdk_window_set_user_data (widget->window, player);

   widget->style = gtk_style_attach (widget->style, widget->window);
   gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
   gdk_window_set_background (widget->window, &widget->style->black);

   gimv_mplayer_send_configure (player);
}


static void
gimv_mplayer_unrealize (GtkWidget *widget)
{
   GimvMPlayer *player = GIMV_MPLAYER (widget);

   if (gimv_mplayer_is_running (player)) {
      gimv_mplayer_stop (player);
   }

   if (GTK_WIDGET_CLASS(gimv_mplayer_parent_class)->unrealize)
      GTK_WIDGET_CLASS(gimv_mplayer_parent_class)->unrealize (widget);
}


static void
gimv_mplayer_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
   g_return_if_fail (widget != NULL);
   g_return_if_fail (GIMV_IS_MPLAYER (widget));
   g_return_if_fail (allocation != NULL);

   widget->allocation = *allocation;
   /* FIXME, TODO-1.3: back out the MAX() statements */
   widget->allocation.width = MAX (1, widget->allocation.width);
   widget->allocation.height = MAX (1, widget->allocation.height);

   if (GTK_WIDGET_REALIZED (widget)) {
      gdk_window_move_resize (widget->window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      gimv_mplayer_send_configure (GIMV_MPLAYER (widget));
   }
}


static void
gimv_mplayer_send_configure (GimvMPlayer *player)
{
   GtkWidget *widget;
   GdkEventConfigure event;

   widget = GTK_WIDGET (player);

   event.type = GDK_CONFIGURE;
   event.window = widget->window;
   event.send_event = TRUE;
   event.x = widget->allocation.x;
   event.y = widget->allocation.y;
   event.width = widget->allocation.width;
   event.height = widget->allocation.height;
  
   gtk_widget_event (widget, (GdkEvent*) &event);
}


/* To fit image to frame when use XVideo driver */
static void
gimv_mplayer_send_dummy_configure (GimvMPlayer *player)
{
   GtkWidget *widget;
   gint width, height;

   g_return_if_fail (GTK_IS_WIDGET (player));

   widget = GTK_WIDGET (player);
   width  = widget->allocation.width;
   height = widget->allocation.height;

   gdk_window_resize (widget->window, width - 1, height - 1);
   gdk_window_resize (widget->window, width, height);
}



/******************************************************************************
 *
 *   mplayer class methods.
 *
 ******************************************************************************/
gint 
gimv_mplayer_get_width (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), -1);
   if (player->media_info.video)
      return player->media_info.video->width;
   else
      return -1;
}


gint 
gimv_mplayer_get_height (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), -1);
   if (player->media_info.video)
      return player->media_info.video->height;
   else
      return -1;
}


gfloat
gimv_mplayer_get_length (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), -1.0);
   return player->media_info.length;
}


gfloat
gimv_mplayer_get_position (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), 0.0);
   return player->pos;
}


GimvMPlayerStatus
gimv_mplayer_get_status (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), GimvMPlayerStatusStop);
   return player->status;
}


static ChildContext *
gimv_mplayer_get_player_child (GimvMPlayer *player)
{
   if (!player_context_table) return NULL;

   return g_hash_table_lookup (player_context_table, player);
}


static void
gimv_mplayer_send_command (GimvMPlayer *player, const gchar *command)
{
   ChildContext *context;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   if (!gimv_mplayer_is_running(player)) return;

   if (!player_context_table) return;
   context = g_hash_table_lookup (player_context_table, player);
   if (!context) return;

   g_return_if_fail (context->stdin_fd > 0);

   write (context->stdin_fd, command, strlen (command));
}


gboolean
gimv_mplayer_is_running (GimvMPlayer *player)
{
   if (gimv_mplayer_get_player_child (player))
      return TRUE;

   return FALSE;
}


GtkWidget *
gimv_mplayer_new (void)
{
   GimvMPlayer *player = GIMV_MPLAYER (g_object_new (GIMV_TYPE_MPLAYER, NULL));

   return GTK_WIDGET (player);
}


static void
playback_done (gpointer data)
{
   GimvMPlayer *player = data;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   player->pos = 0.0;
   player->status = GimvMPlayerStatusStop;
   g_signal_emit (G_OBJECT (player), gimv_mplayer_signals[STOP_SIGNAL], 0);
}


void
gimv_mplayer_play (GimvMPlayer *player)
{
   gimv_mplayer_start (player, 0.0, 1.0);
}


void
gimv_mplayer_start (GimvMPlayer *player, gfloat pos, gfloat speed)
{
   GList *arg_list = NULL;
   ChildContext *context;
   gchar buf[16];
   gint buf_size = sizeof (buf) / sizeof (gchar);
   struct stat st;

   g_return_if_fail (GIMV_IS_MPLAYER (player));
   g_return_if_fail (player->filename);

   if (player->status == GimvMPlayerStatusPause) {
      gimv_mplayer_toggle_pause (player);
      return;
   }

   if (gimv_mplayer_is_running (player)) return;

   /* create argument list */
   arg_list = g_list_append (arg_list, g_strdup (player->command));
   arg_list = g_list_append (arg_list, g_strdup("-slave"));

   if (GTK_WIDGET_REALIZED (GTK_WIDGET (player))) {
#if defined(GDK_WINDOWING_X11)
      if (player->flags & GimvMPlayerEmbedFlag) {
         g_snprintf (buf, buf_size, "%ld",
                     GDK_WINDOW_XWINDOW (GTK_WIDGET (player)->window));
         arg_list = g_list_append (arg_list, g_strdup("-wid"));
         arg_list = g_list_append (arg_list, g_strdup(buf));
      }
#endif /* GDK_WINDOWING_X11 */

      if (player->vo && !strcmp ("x11", player->vo) &&
          player->flags & GimvMPlayerEmbedFlag)
      {
         g_snprintf (buf, buf_size, "scale=%d:%d",
                     GTK_WIDGET (player)->allocation.width,
                     GTK_WIDGET (player)->allocation.height);
         arg_list = g_list_append (arg_list, g_strdup("-vop"));
         arg_list = g_list_append (arg_list, g_strdup(buf));
      }
   }

   if (player->include_file && *player->include_file
       && !stat(player->include_file, &st))
   {
      arg_list = g_list_append (arg_list, g_strdup ("-include"));
      arg_list = g_list_append (arg_list, g_strdup (player->include_file));
   }

   if (player->vo) {
      arg_list = g_list_append (arg_list, g_strdup("-vo"));
      arg_list = g_list_append (arg_list, g_strdup(player->vo));
   }

   if (player->ao) {
      arg_list = g_list_append (arg_list, g_strdup("-ao"));
      arg_list = g_list_append (arg_list, g_strdup(player->ao));
   }

   if (player->flags & GimvMPlayerAllowFrameDroppingFlag) {
      arg_list = g_list_append (arg_list, g_strdup ("-framedrop"));
   }

   /* pos */
   if (pos > 0.01) {
      arg_list = g_list_append (arg_list, g_strdup ("-ss"));
      g_snprintf (buf, buf_size, "%f", pos);
      arg_list = g_list_append (arg_list, g_strdup (buf));
   }

   /* speed */
   if (speed < 0.01 && speed > 100.01) {
      player->speed = 1.0;
   } else {
      player->speed = speed;
   }
   arg_list = g_list_append (arg_list, g_strdup ("-speed"));
   g_snprintf (buf, buf_size, "%f", player->speed);
   arg_list = g_list_append (arg_list, g_strdup (buf));

   /* FIXME: add player->args */

   arg_list = g_list_append (arg_list, g_strdup(player->filename));

   /* real start */
   context = start_command (player, arg_list, NULL, TRUE,
                            process_line, player, playback_done);

   /* add to hash table */
   if (!player_context_table)
      player_context_table = g_hash_table_new (g_direct_hash, g_direct_equal);
   g_hash_table_insert (player_context_table, player, context);
}


void
gimv_mplayer_stop (GimvMPlayer *player)
{
   ChildContext *context;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   context = gimv_mplayer_get_player_child (player);
   if (!context) return;

   if (player->status == GimvMPlayerStatusPause)
      gimv_mplayer_toggle_pause (player);

   gimv_mplayer_send_command (player, "q\n");
   /* for old MPlayer */
   gimv_mplayer_send_command (player, "quit\n");

   waitpid (context->pid, NULL, WUNTRACED);
   context->pid = 0;
   child_context_destroy (context);

   gtk_widget_queue_draw (GTK_WIDGET (player));
}


void
gimv_mplayer_toggle_pause (GimvMPlayer *player)
{
   gimv_mplayer_send_command (player, "pause\n");
}


void
gimv_mplayer_set_speed (GimvMPlayer *player, gfloat speed)
{
   g_return_if_fail (GIMV_IS_MPLAYER (player));
   g_return_if_fail (speed > 0.00999 && speed < 100.00001);

   if (player->status == GimvMPlayerStatusPlay
       || player->status == GimvMPlayerStatusPause)
   {
      gfloat pos = player->pos;
      gimv_mplayer_stop (player);
      gimv_mplayer_start (player, pos, speed);
   } else {
      player->speed = speed;
   }
}


gfloat
gimv_mplayer_get_speed (GimvMPlayer *player)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), 1.0);

   return player->speed;
}


void
gimv_mplayer_seek (GimvMPlayer *player, gfloat percentage)
{
   gchar buf[64];

   g_return_if_fail (percentage > -0.000001 && percentage < 100.000001);

   g_snprintf (buf, 64, "seek %f 1\n", percentage);
   gimv_mplayer_send_command (player, buf);
}


void
gimv_mplayer_seek_by_time (GimvMPlayer *player, gfloat second)
{
   gchar buf[64];

   g_return_if_fail (second >= 0);

   g_snprintf (buf, 64, "seek %f 2\n", second);
   gimv_mplayer_send_command (player, buf);
}


void
gimv_mplayer_seek_relative (GimvMPlayer *player, gfloat second)
{
   gchar buf[64];

   g_return_if_fail (second >= 0);

   g_snprintf (buf, 64, "seek %f 0\n", second);
   gimv_mplayer_send_command (player, buf);
}


void
gimv_mplayer_set_audio_delay (GimvMPlayer *player, gfloat second)
{
   gchar buf[64];

   g_snprintf (buf, 64, "audio_delay %f\n", second);
   gimv_mplayer_send_command (player, buf);
}


static void
identify_done (gpointer data)
{
   /* gtk_main_quit (); */
}


gboolean
gimv_mplayer_set_file (GimvMPlayer *player,
                       const gchar *file)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), FALSE);
   g_return_val_if_fail (!gimv_mplayer_is_running (player), FALSE);

   g_free (player->filename);
   player->filename = NULL;
   gimv_mplayer_media_info_delete (&player->media_info);

   /* identify */
   if (file && *file) {
      GList *args = NULL;
      gboolean identify_supported = TRUE;
      struct stat st;

      args = g_list_append (args, g_strdup (player->command));
      args = g_list_append (args, g_strdup ("-vo"));
      args = g_list_append (args, g_strdup ("null"));
      args = g_list_append (args, g_strdup ("-ao"));
      args = g_list_append (args, g_strdup ("null"));
      args = g_list_append (args, g_strdup ("-identify"));
      args = g_list_append (args, g_strdup (file));
      args = g_list_append (args, g_strdup ("-frames"));
      args = g_list_append (args, g_strdup ("0"));
      if (player->include_file && *player->include_file
          && !stat(player->include_file, &st))
      {
         args = g_list_append (args, g_strdup ("-include"));
         args = g_list_append (args, g_strdup (player->include_file));
      }

      start_command (player, args, NULL, FALSE,
                     process_line_identify, &identify_supported, identify_done);
#if 0
      gtk_grab_add (GTK_WIDGET (player));
      gtk_main ();
      gtk_grab_remove (GTK_WIDGET (player));
#endif

      if (player->media_info.video || player->media_info.audio
          || !identify_supported)
      {
         player->filename = g_strdup (file);
         g_signal_emit (G_OBJECT (player),
                        gimv_mplayer_signals[IDENTIFIED_SIGNAL], 0);
         return TRUE;
      } else {
         /* error handling */
         return FALSE;
      }
   }

   return TRUE;
}


void
gimv_mplayer_set_flags (GimvMPlayer *player, GimvMPlayerFlags flags)
{
   g_return_if_fail (GIMV_IS_MPLAYER (player));

   player->flags |= flags;
}


void
gimv_mplayer_unset_flags (GimvMPlayer *player, GimvMPlayerFlags flags)
{
   g_return_if_fail (GIMV_IS_MPLAYER (player));

   player->flags &= ~flags;
}


GimvMPlayerFlags
gimv_mplayer_get_flags (GimvMPlayer *player, GimvMPlayerFlags flags)
{
   g_return_val_if_fail (GIMV_IS_MPLAYER (player), 0);

   return player->flags;
}


static void
get_drivers_done (gpointer data)
{
   /* gtk_main_quit (); */
}


static GList *
get_drivers (GimvMPlayer *player, gboolean refresh,
             GHashTable *drivers_table,
             const gchar *separator, const gchar *option)
{
   GList *drivers_list = NULL;
   const gchar *command = player ? player->command : "mplayer";
   gchar *key;
   gboolean found;

   g_return_val_if_fail (drivers_table, NULL);
   g_return_val_if_fail (separator && option, NULL);

   found = g_hash_table_lookup_extended (drivers_table, command,
                                         (gpointer) &key,
                                         (gpointer) &drivers_list);
   if (!found) drivers_list = NULL;

   if (drivers_list && refresh) {
      g_hash_table_remove (drivers_table, command);
      g_free (key);
      key = NULL;
      g_list_foreach (drivers_list, (GFunc) g_free, NULL);
      g_list_free (drivers_list);
      drivers_list = NULL;
   }

   if (!drivers_list) {
      GetDriversContext dcontext;
      GList *args = NULL;

      args = g_list_append (args, g_strdup (command));
      args = g_list_append (args, g_strdup (option));
      args = g_list_append (args, g_strdup ("help"));

      dcontext.separator      = separator;
      dcontext.sep_len        = strlen (separator);
      dcontext.passing_header = TRUE;
      dcontext.drivers_list   = g_list_append (NULL, g_strdup ("default"));

      start_command (player, args, NULL, FALSE,
                     process_line_get_drivers, &dcontext, get_drivers_done);
      /* gtk_main (); */

      if (dcontext.drivers_list)
         g_hash_table_insert (drivers_table,
                              g_strdup (command),
                              dcontext.drivers_list);
      drivers_list = dcontext.drivers_list;
   }

   return drivers_list;
}


const GList *
gimv_mplayer_get_video_out_drivers (GimvMPlayer *player,
                                    gboolean refresh)
{
   if (player)
      g_return_val_if_fail (GIMV_IS_MPLAYER (player), NULL);

   if (!vo_drivers_table)
      vo_drivers_table = g_hash_table_new (g_str_hash, g_str_equal);

   return get_drivers (player, refresh, vo_drivers_table,
                       "Available video output drivers:",
                       "-vo");
}


const GList *
gimv_mplayer_get_audio_out_drivers (GimvMPlayer *player,
                                    gboolean refresh)
{
   if (player)
      g_return_val_if_fail (GIMV_IS_MPLAYER (player), NULL);

   if (!ao_drivers_table)
      ao_drivers_table = g_hash_table_new (g_str_hash, g_str_equal);

   return get_drivers (player, refresh, ao_drivers_table,
                       "Available audio output drivers:",
                       "-ao");
}


void
gimv_mplayer_set_video_out_driver (GimvMPlayer *player, const gchar *vo_driver)
{
   const GList *list, *node;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   g_free (player->vo);

   if (!vo_driver || !*vo_driver || !strcasecmp (vo_driver, "default")) {
      player->vo = NULL;
      return;
   }

   list = gimv_mplayer_get_video_out_drivers (player, FALSE);

   /* validate */
   for (node = list; node; node = g_list_next (node)) {
      gchar *str = node->data;
      if (*str && !strcmp (str, vo_driver)) {
         player->vo = g_strdup (vo_driver);
         return;
      }
   }

   player->vo = NULL;
}


void
gimv_mplayer_set_audio_out_driver (GimvMPlayer *player, const gchar *ao_driver)
{
   const GList *list, *node;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   g_free (player->ao);

   list = gimv_mplayer_get_audio_out_drivers (player, FALSE);

   if (!ao_driver | !*ao_driver || !strcasecmp (ao_driver, "default")) {
      player->ao = NULL;
      return;
   }

   /* validate */
   for (node = list; node; node = g_list_next (node)) {
      gchar *str = node->data;

      if (*str && !strcmp (str, ao_driver)) {
         player->ao = g_strdup (ao_driver);
         return;
      }
   }

   player->ao = NULL;
}


#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
/* table of tmporary directories list.*/
GHashTable *tmp_dirs_table = NULL;

#define PATH_SIZE 1024

static gchar *
pickup_newest_file (const gchar *dir)
{
   DIR *dp;
   struct dirent *entry;
   gchar path[PATH_SIZE], newest[PATH_SIZE];
   time_t newest_time = 0;

   newest[0] = '\0';

   dp = opendir (dir);
   if (!dp) return NULL;

   while ((entry = readdir (dp))) {
      struct stat st;

      if (dir [strlen (dir) - 1] == '/')
         g_snprintf (path, PATH_SIZE, "%s%s", dir, entry->d_name);
      else
         g_snprintf (path, PATH_SIZE, "%s/%s", dir, entry->d_name);
 
      if (stat (path, &st) < 0) continue;
      if (S_ISDIR(st.st_mode)) continue;

      if (*newest || st.st_mtime > newest_time) {
         memcpy(newest, path, PATH_SIZE);
         newest_time = st.st_mtime;
      }
   }

   closedir (dp);

   if (*newest)
      return g_strdup (newest);
   else
      return NULL;
}


static gboolean
prepare_tmp_dir (GimvMPlayer *mplayer, const gchar *dir)
{
   GList *list, *node;
   const gchar *tmpdir = NULL;

   g_return_val_if_fail (dir && *dir, FALSE);

   list = g_hash_table_lookup (tmp_dirs_table, mplayer);
   for (node = list; node; node = g_list_next (node)) {
      if (node->data && !strcmp (dir, node->data)) {
         tmpdir = node->data;
         break;
      }
   }

   /* if directory is already created by other process, abort. */
   if (!tmpdir && !access(dir, F_OK)) return FALSE;

   /* create the directory */
   mkdir (dir, S_IRWXU);
   if (access(dir, R_OK) || access(dir, W_OK) || access(dir, X_OK))
      return FALSE;

   if (!tmpdir) {
      list = g_list_append (list, g_strdup (dir));
      g_hash_table_insert (tmp_dirs_table, mplayer, list);
   }

   return TRUE;
}


static void
get_frame_done (gpointer data)
{
   gboolean *main_iterate = data;

   if (main_iterate && *main_iterate)
      gtk_main_quit ();
}


/* FIXME!! thread safe? */
gchar *
gimv_mplayer_get_frame (GimvMPlayer *player,
                        const gchar *vo_driver,
                        const gchar *tmp_dir,
                        gfloat       pos,
                        gint         frames,
                        gboolean     main_iterate)
{
   GList *arg_list = NULL;
   gchar dir[PATH_SIZE], buf[PATH_SIZE], *filename;

   if (!tmp_dirs_table)
      tmp_dirs_table = g_hash_table_new (g_direct_hash, g_direct_equal);

   g_return_val_if_fail (GIMV_IS_MPLAYER (player), NULL);
   g_return_val_if_fail (player->filename, NULL);

   if (!tmp_dir || !*tmp_dir) tmp_dir = g_get_tmp_dir ();
   if (access(tmp_dir, R_OK) || access(tmp_dir, W_OK) || access(tmp_dir, X_OK))
      return NULL;

   vo_driver = vo_driver && *vo_driver ? vo_driver : "png";
   if (pos < 0.01) pos = player->pos > 0.01 ? player->pos : 0.0;
   if (frames <= 0 || frames >= 100) frames = 5;

   g_snprintf (dir, PATH_SIZE, "%s%s.%d", tmp_dir, "/gimv_mplayer", getpid ());
   if (!prepare_tmp_dir (player, dir)) return NULL;

   /* create arg list */
   arg_list = g_list_append (arg_list, g_strdup (player->command));

   arg_list = g_list_append (arg_list, g_strdup("-vo"));
   arg_list = g_list_append (arg_list, g_strdup(vo_driver));

   arg_list = g_list_append (arg_list, g_strdup("-ao"));
   arg_list = g_list_append (arg_list, g_strdup("null"));

   arg_list = g_list_append (arg_list, g_strdup ("-frames"));
   g_snprintf (buf, PATH_SIZE, "%d", frames);
   arg_list = g_list_append (arg_list, g_strdup (buf));

   if (pos > 0.01) {
      arg_list = g_list_append (arg_list, g_strdup ("-ss"));
      g_snprintf (buf, PATH_SIZE, "%f", pos);
      arg_list = g_list_append (arg_list, g_strdup (buf));
   }

   arg_list = g_list_append (arg_list, g_strdup (player->filename));

   /* get screen shot */
   start_command (player, arg_list, dir, main_iterate,
                  NULL, &main_iterate, get_frame_done);
   if (main_iterate)
      gtk_main ();
   filename = pickup_newest_file (dir);

   return filename;
}


static void
gimv_mplayer_remove_dir (const gchar *dirname)
{
   GList *list = NULL, *node;
   DIR *dp;

   dp = opendir (dirname);
   if (dp) {
      struct dirent *entry;

      while ((entry = readdir (dp))) {
         const gchar *format;
         gchar path[PATH_SIZE];

         if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
            continue;

         format = path[strlen(path) - 1] == '/' ? "%s%s" : "%s/%s";
         g_snprintf (path, PATH_SIZE, format, dirname, entry->d_name);
         list = g_list_append (list, g_strdup (path));
      }
   }
   closedir (dp);

   for (node = list; node; node = g_list_next (node)) {
      struct stat st;

      if (stat (node->data, &st)) continue;

      if (S_ISDIR(st.st_mode))
         gimv_mplayer_remove_dir (node->data);
      else
         remove (node->data);
   }

   g_list_foreach (list, (GFunc) g_free, NULL);
   g_list_free (list);

   remove (dirname);
}


void
gimv_mplayer_flush_tmp_files (GimvMPlayer *player)
{
   GList *list, *node;

   g_return_if_fail (GIMV_IS_MPLAYER (player));

   if (!tmp_dirs_table) return;

   list = g_hash_table_lookup (tmp_dirs_table, player);

   for (node = list; node; node = g_list_next (node)) {
      const gchar *dir = node->data;

      gimv_mplayer_remove_dir (dir);
   }

   if (list) {
      g_hash_table_remove (tmp_dirs_table, player);
      g_list_foreach (list, (GFunc) g_free, NULL);
      g_list_free (list);
   }
}



/******************************************************************************
 *
 *   MPlayer handling.
 *
 ******************************************************************************/
static ChildContext *
start_command (GimvMPlayer *player,
               GList *arg_list, /* include command */
               const gchar *work_dir,
               gboolean main_iterate,
               ProcessLineFunc func, gpointer data, GtkDestroyNotify destroy_fn)
{
   ChildContext *context;
   pid_t pid;
   int in_fd[2], out_fd[2], err_fd[2];

   if (pipe (out_fd) < 0) goto ERROR0;
   if (pipe (err_fd) < 0) goto ERROR1;
   if (pipe (in_fd)  < 0) goto ERROR2;

   pid = fork ();

   if (pid < 0) {
      g_warning ("GimvMPlayer: faild to fork(2): %s", strerror (errno));
      return NULL;
   }

   if (pid > 0) {
      /* parent process */
      context = g_new0 (ChildContext, 1);
      context->pid             = pid;
      context->args            = arg_list;
      context->player          = player;
      context->process_line_fn = func;
      context->data            = data;
      context->destroy_fn      = destroy_fn;

      context->stdout_fd       = out_fd[0];
      fcntl (context->stdout_fd, F_SETFL, O_NONBLOCK);
      close (out_fd[1]);
      context->stdout_buf[0]   = '\0';
      context->stdout_size     = 0;

      context->stderr_fd       = err_fd[0];
      fcntl (context->stderr_fd, F_SETFL, O_NONBLOCK);
      close (err_fd[1]);
      context->stderr_buf[0]   = '\0';
      context->stderr_size     = 0;

      context->stdin_fd        = in_fd[1];
      close (in_fd[0]);

      /* observe output */
      if (main_iterate) {
         context->checker_id = gtk_timeout_add (GIMV_MPLAYER_REFRESH_RATE, 
                                                timeout_check_child, context);
      } else {
         while (timeout_check_child (context))
            usleep (GIMV_MPLAYER_REFRESH_RATE * 1000);
      }

      return context;

   } else if (pid == 0) {
      GList *list;
      gchar **argv;
      gint i = 0, n_args;

      /* child process */      

      /* set pipe */
      close (out_fd[0]);
      close (err_fd[0]);
      close (in_fd[1]);
      dup2 (out_fd[1], STDOUT_FILENO);
      dup2 (err_fd[1], STDERR_FILENO);
      dup2 (in_fd[0],  STDIN_FILENO);

      /* set args */
      n_args = g_list_length (arg_list);
      argv = g_new0 (gchar *, n_args + 1);
      for (list = arg_list; list; list = g_list_next (list))
         argv[i++] = list->data;
      argv[i] = NULL;

      /* set working directory */
      if (work_dir && *work_dir)
         chdir (work_dir);

      /* exec */
      putenv ("LC_ALL=C");

      execvp (argv[0], argv);

      close (out_fd[1]);
      close (err_fd[1]);
      close (in_fd[0]);

      if (errno) {
         g_warning ("GimvMPlayer: cannot execute command: %s", strerror (errno));
      } else {
         g_warning ("GimvMPlayer: cannot execute command: Unknown error");
      }

      _exit (255);
   }

   close (in_fd[0]);
   close (in_fd[1]);
 ERROR2:
   close (err_fd[0]);
   close (err_fd[1]);
 ERROR1:
   close (out_fd[0]);
   close (out_fd[1]);
 ERROR0:
   g_list_foreach (arg_list, (GFunc) g_free, NULL);
   g_list_free (arg_list);
   return NULL;
}


static gint
timeout_check_child (gpointer data)
{
   ChildContext *context = data;
   GimvMPlayer *player = context->player;

   if (context->pid <= 0) return FALSE;

   if (!process_output (context)) {
      pid_t pid;
      int status;

      pid = waitpid (context->pid, &status, WNOHANG);
      context->pid  = 0;
      context->checker_id = 0;

      child_context_destroy (context);

      gtk_widget_queue_draw (GTK_WIDGET (player));

      return FALSE;
   }

   return TRUE;
}


static void
process_lines (ChildContext *context,
               gchar buf[GIMV_MPLAYER_BUF_SIZE * 2], gint bufsize,
               gchar stock_buf[GIMV_MPLAYER_BUF_SIZE], gint *remain_size,
               gboolean is_stderr)
{
   gint i, len, size;
   gchar *src, *end;

   g_return_if_fail (buf && stock_buf);
   g_return_if_fail (size > 0 || size < GIMV_MPLAYER_BUF_SIZE);
   g_return_if_fail (remain_size);

   src = buf;
   size = bufsize;

   while (src && size >= 0) {
      end = NULL;

      for (i = 0; i < size; i++) {
         if (src[i] == '\n' || src[i] == '\r') {
            end = src + i;
            break;
         }
      }

      if (!end) break;

      *end = '\0';
      len = end - src;

      if (context->process_line_fn)
         context->process_line_fn (context, src, len, is_stderr);

      size -= len;
      src = end + 1;
   }

   if (size <= 0 || size > GIMV_MPLAYER_BUF_SIZE - 1 || !src || !*src) {
      stock_buf[0] = '\0';
      *remain_size = 0;
   } else {
      memcpy (stock_buf, src, size);
      stock_buf[size] = '\0';
      *remain_size = size;
   }
}


static gboolean
process_output (ChildContext *context)
{
   gint n, size;
   gchar buf[GIMV_MPLAYER_BUF_SIZE * 2], *next;
   gfloat pos;

   /* stderr */
   size = context->stderr_size;

   if (size > 0 && size < GIMV_MPLAYER_BUF_SIZE) {
      memcpy (buf, context->stderr_buf, size);
      next = buf + size;
   } else {
      next = buf;
   }

   n = read (context->stderr_fd, buf, GIMV_MPLAYER_BUF_SIZE - 1);

   switch (errno) {
   case 0:
   case EAGAIN:
      break;
   default:
      /* g_warning ("GimvMPlayer: stderr: %s", strerror (errno)); */
      break;
   }

   if (n >= 0) {
      next[MIN (GIMV_MPLAYER_BUF_SIZE - 1, n)] = '\0';
      size += MIN (GIMV_MPLAYER_BUF_SIZE - 1, n);

      process_lines (context, buf, size,
                     context->stderr_buf, &context->stderr_size,
                     TRUE);
   }

   /* stdout */
   size = context->stdout_size;

   if (size > 0 && size < GIMV_MPLAYER_BUF_SIZE) {
      memcpy (buf, context->stdout_buf, size);
      next = buf + size;
   } else {
      next = buf;
   }

   n = read (context->stdout_fd, next, GIMV_MPLAYER_BUF_SIZE - 1);

   switch (errno) {
   case 0:
   case EAGAIN:
      if (n < 0)
         return TRUE;
      break;
   default:
      /* g_warning ("GimvMPlayer: stdout: %s", strerror (errno)); */
      break;
   }

   if (n < 0) return TRUE;

   if (n >= 0) {
      gint len = MIN (GIMV_MPLAYER_BUF_SIZE - 1, n);

      next[len] = '\0';
      size += len;
      pos = context->player->pos;

      process_lines (context, buf, size,
                     context->stdout_buf, &context->stdout_size,
                     FALSE);

      if (fabs (context->player->pos - pos) > 0.1)
         g_signal_emit (G_OBJECT (context->player),
                        gimv_mplayer_signals[POS_CHANGED_SIGNAL], 0);
   }

   if (n == 0)
      return FALSE;
   else
      return TRUE;
}


static void
child_context_destroy (ChildContext *context)
{
   g_return_if_fail(context);

   if (context == gimv_mplayer_get_player_child (context->player))
      g_hash_table_remove (player_context_table, context->player);

   if (context->checker_id > 0) {
      gtk_timeout_remove (context->checker_id);
      context->checker_id = 0;
   }

   if (context->pid > 0) {
      kill (context->pid, SIGKILL);
      waitpid (context->pid, NULL, WUNTRACED);
      context->pid = 0;
   }

   g_list_foreach (context->args, (GFunc) g_free, NULL);
   g_list_free (context->args);
   context->args = NULL;

   if (context->stdout_fd)
      close (context->stdout_fd);
   context->stdout_fd = 0;

   if (context->stderr_fd)
      close (context->stderr_fd);
   context->stderr_fd = 0;

   if (context->stdin_fd)
      close (context->stdin_fd);
   context->stdin_fd = 0;

   if (context->destroy_fn)
      context->destroy_fn (context->data);
   context->data = NULL;

   g_free (context);
}



/******************************************************************************
 *
 *   Line processing funcs.
 *
 ******************************************************************************/
static void
process_line (ChildContext *context,
              const gchar *line, gint len,
              gboolean is_stderr)
{
   GimvMPlayer *player = context->player;

   if (is_stderr) {
      /* fprintf (stderr, "%s\n", line); */
   } else {
      if (strstr (line, "PAUSE")) {
         player->status = GimvMPlayerStatusPause;
         g_signal_emit (G_OBJECT (player),
                        gimv_mplayer_signals[PAUSE_SIGNAL], 0);

      } else if (len > 2 && (!strncmp (line, "A:", 2)
                          || !strncmp (line, "V:", 2)))
      {
         gchar timestr[16];
         const gchar *begin = line + 2, *end;

         /* set status */
         if (player->status != GimvMPlayerStatusPlay) {
            player->status = GimvMPlayerStatusPlay;
            if (GTK_WIDGET_MAPPED (player)) {
               gimv_mplayer_send_dummy_configure (player);
            }
            g_signal_emit (G_OBJECT (player),
                           gimv_mplayer_signals[PLAY_SIGNAL], 0);
         }

         /* get movie position */
         while (isspace (*begin) && *begin) begin++;
         if (*begin) {
            end = begin + 1;
            while (!isspace (*end) && *end) end++;
            len = end - begin;
            if (len > 0 && len < 15) {
               memcpy(timestr, begin, len);
               timestr[len] = '\0';
               player->pos = atof (timestr);
            }
         }

      } else if (!strncasecmp (line, "VIDEO: ", 7)) {
         if (!strncmp (line + 7, "no video!!!", 11)) {
         } else {
         }

      } else if (!strncasecmp (line, "Audio: ", 7)) {
         if (!strncmp (line + 7, "no sound!!!", 11)) {
         } else {
         }

      } else {
      }
   }
}


static void
process_line_get_drivers (ChildContext *context,
                          const gchar *line, gint len,
                          gboolean is_stderr)
{
   GetDriversContext *dcontext;
   gchar *str, *id_str, *exp_str = NULL, *end;

   g_return_if_fail (context);
   g_return_if_fail (context->data);
   if (!line || !*line || len <= 0) return;

   dcontext = context->data;

   if (dcontext->passing_header) {
      if (len >= dcontext->sep_len
          && !strncmp (line, dcontext->separator, dcontext->sep_len))
      {
         dcontext->passing_header = FALSE;
      }
      return;
   }

   str = id_str = g_strdup (line);

   /* get id */
   while (isspace (*id_str) && *id_str) id_str++;
   if (!*id_str) {
      g_free (str);
      return;
   }
   end = id_str + 1;
   while (!isspace (*end) && *end) end++;
   if (*end) exp_str = end + 1;
   *end = '\0';

   /* get name */
   if (exp_str) {
      /* exp_str = g_strstrip (exp_str); */
   }

   /* append to list */
   dcontext->drivers_list = g_list_append (dcontext->drivers_list,
                                           g_strdup (id_str));

   g_free (str);
}


static void
process_line_identify (ChildContext *context,
                       const gchar *line, gint len,
                       gboolean is_stderr)
{
   GimvMPlayer *player;
   GimvMPlayerMediaInfo *info;
   const gchar *errstr = "Error: 'identify' is not a mplayer/mencoder option";

   g_return_if_fail (context);
   g_return_if_fail (context->player);
   g_return_if_fail (context->data);
   if (!line || !*line || len <= 0) return;

   player = context->player;
   info = &player->media_info;

   if (!strncmp (line, errstr, strlen (errstr))) {
      *((gboolean *) context->data) = FALSE;
      return;
   }

   if (!strncmp (line, "ID_LENGTH=", 10)) {
      /* MPlayer-0.90rc1 or later only */
      info->length = atof (line + 10);

   /* video */
   } else if (!strncmp (line, "ID_VIDEO", 8)) {
      if (!info->video)
         info->video = g_new0 (struct GimvMPlayerVideoInfo_Tag, 1);

      if (!strncmp (line, "ID_VIDEO_FORMAT=", 16)) {
         if (strlen (line + 16) < sizeof (info->video->format) / sizeof (gchar))
            strcpy (info->video->format, line + 16);

      } else if (!strncmp (line, "ID_VIDEO_BITRATE=", 17)) {
         info->video->bitrate = atof (line + 17);

      } else if (!strncmp (line, "ID_VIDEO_WIDTH=", 15)) {
         info->video->width = atoi (line + 15);

      } else if (!strncmp (line, "ID_VIDEO_HEIGHT=", 16)) {
         info->video->height = atoi (line + 16);

      } else if (!strncmp (line, "ID_VIDEO_FPS=", 12)) {
         info->video->fps = atof (line + 12);

      } else if (!strncmp (line, "ID_VIDEO_ASPECT=", 16)) {
         info->video->aspect = atof (line + 16);
      }

   /* audio */
   } else if (!strncmp (line, "ID_AUDIO", 8)) {
      if (!info->audio)
         info->audio = g_new0 (struct GimvMPlayerAudioInfo_Tag, 1);

      if (!strncmp (line, "ID_AUDIO_CODEC=", 15)) {
         if (strlen (line + 15) < sizeof (info->audio->codec) / sizeof (gchar))
            strcpy (info->audio->codec, line + 15);

      } else if (!strncmp (line, "ID_AUDIO_FORMAT=", 16)) {

      } else if (!strncmp (line, "ID_AUDIO_BITRATE=", 17)) {
         info->audio->bitrate = atof (line + 17);

      } else if (!strncmp (line, "ID_AUDIO_RATE=", 14)) {
         info->audio->rate = atoi (line + 14);

      } else if (!strncmp (line, "ID_AUDIO_NCH=", 13)) {
         info->audio->n_ch = atoi (line + 13);
      }
   }
}

#endif /* ENABLE_MPLAYER */
