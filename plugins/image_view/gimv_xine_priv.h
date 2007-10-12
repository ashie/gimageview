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

#ifndef __GIMV_XINE_PRIVATE_H__
#define __GIMV_XINE_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_XINE

#include <xine.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pwd.h>
#include <sys/types.h>
#include <pthread.h>

#include <gtk/gtk.h>
#include "gimv_xine.h"

#if (GTK_MAJOR_VERSION == 1) && (GTK_MAJOR_VERION <= 2)
#  ifndef GDK_WINDOWING_X11
#     define GDK_WINDOWING_X11
#  endif
#endif

#if defined (GDK_WINDOWING_X11)
#  include <X11/Xlib.h>
#  include <X11/keysym.h>
#  include <X11/cursorfont.h>
#  include <X11/Xatom.h>
#  include <X11/extensions/XShm.h>
#  include <gdk/gdkx.h>
#  define GIMV_XINE_DEFAULT_VISUAL_TYPE XINE_VISUAL_TYPE_X11
#elif defined (GDK_WINDOWING_FB)
#  define GIMV_XINE_DEFAULT_VISUAL_TYPE XINE_VISUAL_TYPE_FB
#else
#  define GIMV_XINE_DEFAULT_VISUAL_TYPE XINE_VISUAL_TYPE_NONE
#endif


/*
 * config related constants
 */
#define CONFIG_LEVEL_BEG         0 /* => beginner */
#define CONFIG_LEVEL_ADV        10 /* advanced user */
#define CONFIG_LEVEL_EXP        20 /* expert */
#define CONFIG_LEVEL_MAS        30 /* motku */
#define CONFIG_LEVEL_DEB        40 /* debugger (only available in debug mode) */

#define CONFIG_NO_DESC          NULL
#define CONFIG_NO_HELP          NULL
#define CONFIG_NO_CB            NULL
#define CONFIG_NO_DATA          NULL


struct GimvXinePrivate_Tag
{
   xine_t                  *xine;

   xine_stream_t           *stream;
   xine_event_queue_t      *event_queue;
   double                   display_ratio;

   char                    *configfile;

   char                    *video_driver_id;
   char                    *audio_driver_id;

   xine_vo_driver_t        *vo_driver;
   xine_ao_driver_t        *ao_driver;

   int                      xpos, ypos;
   int                      oldwidth, oldheight;

#if defined (GDK_WINDOWING_X11)
   Display                 *display;
   int                      screen;
   Window                   video_window;
   int                      completion_event;

   pthread_t                thread;
#endif /* defined (GDK_WINDOWING_X11) */

   int                       post_video_num;
   xine_post_t              *post_video;

   struct {
      xine_stream_t         *stream;
      xine_event_queue_t    *event_queue;
      int                    running;
      int                    current;
      int                    enabled; /* 0, 1:vpost, 2:vanim */

      char                 **mrls;
      int                    num_mrls;

      int                    post_plugin_num;
      xine_post_t           *post_output;
      int                    post_changed;

   } visual_anim;
};


typedef void (*GimvXinePrivScaleLineFn) (guchar *source, guchar *dest,
                                         gint width, gint step);

typedef struct GimvXinePrivImage_Tag {
   gint width;
   gint height;
   gint ratio_code;
   gint format;
   guchar *img, *y, *u, *v, *yuy2;

   gint u_width, v_width;
   gint u_height, v_height;

   GimvXinePrivScaleLineFn scale_line;
   unsigned long scale_factor;
} GimvXinePrivImage;


GimvXinePrivate   *gimv_xine_private_new       (void);
void               gimv_xine_private_destroy   (GimvXinePrivate   *priv);
GimvXinePrivImage *gimv_xine_priv_image_new    (gint               imgsize);
void               gimv_xine_priv_image_delete (GimvXinePrivImage *image);
guchar            *gimv_xine_priv_yuv2rgb      (GimvXinePrivImage *image);

#endif /* ENABLE_XINE */

#endif /* __GIMV_XINE_PRIVATE_H__ */
