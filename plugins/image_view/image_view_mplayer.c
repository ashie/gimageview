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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_MPLAYER

#include <math.h>

#include "gimv_mplayer.h"
#include "gimv_image_view.h"
#include "gimv_plugin.h"
#include "gimv_thumb_cache.h"
#include "prefs_mplayer.h"

static gboolean   imageview_mplayer_is_supported     (GimvImageView *iv,
                                                      GimvImageInfo *info);
static GtkWidget *imageview_mplayer_create           (GimvImageView *iv);
static void       imageview_mplayer_create_thumbnail (GimvImageView *iv,
                                                      const gchar *type);

static gboolean   imageview_mplayer_is_playable      (GimvImageView *iv,
                                                      GimvImageInfo *info);
static gboolean   imageview_mplayer_is_seekable      (GimvImageView *iv);
static void       imageview_mplayer_play             (GimvImageView *iv);
static void       imageview_mplayer_stop             (GimvImageView *iv);
static void       imageview_mplayer_pause            (GimvImageView *iv);
static void       imageview_mplayer_seek             (GimvImageView *iv,
                                                      gfloat     pos);
static GimvImageViewPlayableStatus
                  imageview_mplayer_get_status       (GimvImageView *iv);
static guint      imageview_mplayer_get_length       (GimvImageView *iv);
static guint      imageview_mplayer_get_position     (GimvImageView *iv);


static GimvImageViewPlayableIF playable_if = {
   is_playable_fn:      imageview_mplayer_is_playable,
   is_seekable_fn:      imageview_mplayer_is_seekable,
   play_fn:             imageview_mplayer_play,
   stop_fn:             imageview_mplayer_stop,
   pause_fn:            imageview_mplayer_pause,
   forward_fn:          NULL,
   reverse_fn:          NULL,
   seek_fn:             imageview_mplayer_seek,
   eject_fn:            NULL,
   get_status_fn:       imageview_mplayer_get_status,
   get_length_fn:       imageview_mplayer_get_length,
   get_position_fn:     imageview_mplayer_get_position,
};


static GimvImageViewPlugin imageview_mplayer =
{
   if_version:          GIMV_IMAGE_VIEW_IF_VERSION,
   label:               N_("Movie Player (MPlayer)"),

   /* should be higher than Xine because this plugin is not linked with external library */
   priority_hint:       G_PRIORITY_DEFAULT - 10,
   is_supported_fn:     imageview_mplayer_is_supported,
   create_fn:           imageview_mplayer_create,
   create_thumbnail_fn: imageview_mplayer_create_thumbnail,
   fullscreen_fn:       NULL,

   scalable:            NULL,
   rotatable:           NULL,
   playable:            &playable_if,
};

static const gchar *mpeg_extensions[] =
{
   "mpg", "mpeg", "mpe", "mp2", "m2v", "vob",
};
static const gchar *quicktime_extensions[] =
{
   "mov", "qt", "moov", "qtvr",
};
static const gchar *asf_extensions[] =
{
   "asf",
};
static const gchar *wmv_extensions[] =
{
   "wmv",
};
static const gchar *msvideo_extensions[] =
{
   "avi",
};
static const gchar *audio_extensions[] =
{
   "au", "snd",
};
static const gchar *mp3_extensions[] =
{
   "mp3",
};
static const gchar *realmedia_extensions[] =
{
   "ra", "ram", "rm", "sml",
};
static const gchar *wav_extensions[] =
{
   "wav", "wave",
};

static GimvMimeTypeEntry mplayer_mime_types[] =
{
   {
      mime_type:      "application/vnd.ms-asf",
      description:    "ASF Media",
      extensions:     asf_extensions,
      extensions_len: sizeof (asf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "audio/au",
      description:    "Basic audio",
      extensions:     audio_extensions,
      extensions_len: sizeof (audio_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "audio/x-mp3",
      description:    "MPEG layer 3 Audio",
      extensions:     mp3_extensions,
      extensions_len: sizeof (mp3_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "audio/x-pn-realaudio",
      description:    "RealAudio/Video",
      extensions:     realmedia_extensions,
      extensions_len: sizeof (realmedia_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "audio/x-real-audio",
      description:    "RealAudio/Video",
      extensions:     realmedia_extensions,
      extensions_len: sizeof (realmedia_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "audio/x-wav",
      description:    "WAV Audio",
      extensions:     wav_extensions,
      extensions_len: sizeof (wav_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/mpeg",
      description:    "MPEG Video",
      extensions:     mpeg_extensions,
      extensions_len: sizeof (mpeg_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/qicktime",
      description:    "Quicktime Video",
      extensions:     quicktime_extensions,
      extensions_len: sizeof (quicktime_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/x-ms-asf",
      description:    "MS ASF Video",
      extensions:     asf_extensions,
      extensions_len: sizeof (asf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/x-ms-wmv",
      description:    "MS WMV Video",
      extensions:     wmv_extensions,
      extensions_len: sizeof (wmv_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/x-msvideo",
      description:    "Microsoft Video",
      extensions:     msvideo_extensions,
      extensions_len: sizeof (msvideo_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

/* FIXME */
#include <mplayer_image_loader.h>
static const gchar *
gimv_plugin_get_impl (guint idx, gpointer *impl, guint *size)
{
   g_return_val_if_fail(impl, NULL);
   *impl = NULL;
   g_return_val_if_fail(size, NULL);
   *size = 0;

   switch (idx) {
   case 0:
      *impl = &imageview_mplayer;
      *size = sizeof(imageview_mplayer);
      return GIMV_PLUGIN_IMAGEVIEW_EMBEDER;
   case 1:
      gimv_mplayer_image_loader_get_impl(impl, size);
      return GIMV_PLUGIN_IMAGE_LOADER;
   default:
      return NULL;
   }

   return NULL;
}
GIMV_PLUGIN_GET_MIME_TYPE(mplayer_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("MPlayer Embeder & Movie Frame Loader"),
   version:       "0.1.2",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  gimv_prefs_ui_mplayer_get_page,
};
static GimvPluginInfo *this;


gchar *
g_module_check_init (GModule *module)
{
   g_module_symbol (module, "gimv_plugin_info", (gpointer) &this);
   return NULL;
}

GimvPluginInfo *
gimv_mplayer_plugin_get_info (void)
{
   return this;
}


static GimvMPlayer *
get_mplayer (GimvImageView *iv)
{
   GtkWidget *frame;
   GimvMPlayer *mplayer;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), NULL);
   g_return_val_if_fail (GTK_IS_BIN (iv->draw_area), NULL);

   frame = GTK_BIN (iv->draw_area)->child;
   g_return_val_if_fail (GTK_IS_BIN (frame), NULL);

   mplayer = GIMV_MPLAYER (GTK_BIN (frame)->child);
   g_return_val_if_fail (GIMV_IS_MPLAYER (mplayer), NULL);

   return mplayer;
}


static void
imageview_mplayer_real_play (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   const gchar *filename;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   if (!iv->info) return;
   if (!gimv_image_info_is_movie (iv->info) && !gimv_image_info_is_audio (iv->info))
      return;

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   filename = gimv_image_info_get_path (iv->info);

   if (gimv_mplayer_set_file (GIMV_MPLAYER (mplayer), filename)) {
      gimv_mplayer_play (GIMV_MPLAYER (mplayer));
   }
}


static void
cb_mplayer_play (GimvMPlayer *mplayer, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_set_status (iv, GimvImageViewPlayablePlay);
}


static void
cb_mplayer_stop (GimvMPlayer *mplayer, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_set_status (iv, GimvImageViewPlayableStop);
}


static void
cb_mplayer_pause (GimvMPlayer *mplayer, GimvImageView *iv)
{
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   gimv_image_view_playable_set_status (iv, GimvImageViewPlayablePause);
}


static void
cb_mplayer_pos_changed (GimvMPlayer *mplayer, GimvImageView *iv)
{
   gfloat pos, now, len;

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   now = gimv_mplayer_get_position (mplayer);
   len = gimv_mplayer_get_length (mplayer);

   if (len > 0)
      pos = (gdouble) now / (gdouble) len * 100.0;
   else
      pos = 0.0;

   gimv_image_view_playable_set_position (iv, pos);
}


static void
cb_mplayer_identified (GimvMPlayer *mplayer, GimvImageView *iv)
{
   GtkWidget *frame;
   gint width, height;

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   g_return_if_fail (GTK_IS_BIN (iv->draw_area));
   frame = GTK_BIN (iv->draw_area)->child;
   g_return_if_fail (GTK_IS_ASPECT_FRAME (frame));

   width  = gimv_mplayer_get_width  (mplayer);
   height = gimv_mplayer_get_height (mplayer);
   if (width <= 0 || height <= 0) return;

   gtk_aspect_frame_set (GTK_ASPECT_FRAME (frame),  0.5, 0.5,
                         (gfloat) width / (gfloat) height, FALSE);

   while (gtk_events_pending ()) gtk_main_iteration ();
}



/*****************************************************************************
 *
 *   Virtual functions
 *
 *****************************************************************************/
static gboolean
imageview_mplayer_is_supported (GimvImageView *iv, GimvImageInfo *info)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), FALSE);
   if (!info) return FALSE;

   return gimv_image_info_is_movie (info) || gimv_image_info_is_audio (info);
}


static GtkWidget *
imageview_mplayer_create (GimvImageView *iv)
{
   GtkWidget *widget, *frame, *mplayer;

   widget = gtk_event_box_new ();

   frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.33333, FALSE);
   gtk_container_add (GTK_CONTAINER (widget), frame);
   gtk_widget_show (frame);

   mplayer = gimv_mplayer_new ();
   gtk_container_add (GTK_CONTAINER (frame), mplayer);
   gtk_widget_show (mplayer);

   g_signal_connect (G_OBJECT (mplayer), "play",
                     G_CALLBACK (cb_mplayer_play), iv);
   g_signal_connect (G_OBJECT (mplayer), "stop",
                     G_CALLBACK (cb_mplayer_stop), iv);
   g_signal_connect (G_OBJECT (mplayer), "pause",
                     G_CALLBACK (cb_mplayer_pause), iv);
   g_signal_connect (G_OBJECT (mplayer), "position_changed",
                     G_CALLBACK (cb_mplayer_pos_changed), iv);
   g_signal_connect (G_OBJECT (mplayer), "identified",
                     G_CALLBACK (cb_mplayer_identified), iv);

   gimv_mplayer_set_video_out_driver (GIMV_MPLAYER (mplayer),
                                      gimv_prefs_mplayer_get_driver("vo"));
   gimv_mplayer_set_audio_out_driver (GIMV_MPLAYER (mplayer),
                                      gimv_prefs_mplayer_get_driver("ao"));

   return widget;
}


static void
imageview_mplayer_create_thumbnail (GimvImageView *iv, const gchar *cache_write_type)
{
   GimvImage *imcache;
   GimvMPlayer *mplayer;
   GimvImage *image;
   gchar *filename, *tmpfile;

   if (!gimv_image_info_is_movie (iv->info)) return;

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   if (!gimv_mplayer_is_running (GIMV_MPLAYER (mplayer))) return;

   tmpfile = gimv_mplayer_get_frame (mplayer, NULL, NULL, -1.0, 0, TRUE);
   image = gimv_image_load_file (tmpfile, FALSE);
   if (!image) {
      g_free (tmpfile);
      return;
   }

   filename = gimv_image_info_get_path_with_archive (iv->info);

   imcache = gimv_thumb_cache_save (filename,
                                    cache_write_type,
                                    image,
                                    iv->info);

   if (imcache) {
      gimv_image_unref (imcache);
      g_signal_emit_by_name (G_OBJECT (iv),
                             "thumbnail_created",
                             iv->info);
   }

   g_free (filename);
   gimv_image_unref (image);
   g_free (tmpfile);
}


static gboolean
imageview_mplayer_is_playable (GimvImageView *iv, GimvImageInfo *info)
{
   return TRUE;
}


static gboolean
imageview_mplayer_is_seekable (GimvImageView *iv)
{
   return TRUE;
}

static void
imageview_mplayer_play (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   GimvMPlayerStatus status;
   gfloat speed;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   status = gimv_mplayer_get_status (mplayer);
   speed = gimv_mplayer_get_speed (mplayer);

   if (!gimv_mplayer_is_running (mplayer)) {
      imageview_mplayer_real_play (iv);
   } else if (status == GimvMPlayerStatusPause) {
      gimv_mplayer_toggle_pause (mplayer);
   } else if (status == GimvMPlayerStatusPlay
              && fabs (speed - 1.0) < 0.0001)
   {
      imageview_mplayer_pause (iv);
   } else if (status == GimvMPlayerStatusPlay) {
      gimv_mplayer_set_speed (GIMV_MPLAYER (mplayer), 1.0);
   }
}


static void
imageview_mplayer_pause (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   GimvMPlayerStatus status;

   g_return_if_fail (iv);
   if (!iv->info) return;
   if (!gimv_image_info_is_movie (iv->info) && !gimv_image_info_is_audio (iv->info))
      return;
   g_return_if_fail (GTK_IS_BIN (iv->draw_area));

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   status = gimv_mplayer_get_status (mplayer);
   if (status == GimvMPlayerStatusPlay)
      gimv_mplayer_toggle_pause (mplayer);
}


static void
imageview_mplayer_stop (GimvImageView *iv)
{
   GimvMPlayer *mplayer;

   g_return_if_fail (iv);
   if (!iv->info) return;
   if (!gimv_image_info_is_movie (iv->info) && !gimv_image_info_is_audio (iv->info))
      return;
   g_return_if_fail (GTK_IS_BIN (iv->draw_area));

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   gimv_mplayer_stop (mplayer);
}

static GimvImageViewPlayableStatus
imageview_mplayer_get_status (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   GimvMPlayerStatus status;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), GimvImageViewPlayableStop);

   mplayer = get_mplayer (iv);
   g_return_val_if_fail (mplayer, GimvImageViewPlayableStop);

   status = gimv_mplayer_get_status (mplayer);

   if (status == GimvMPlayerStatusPause) {
      return GimvImageViewPlayablePause;
   } else if (status == GimvMPlayerStatusStop) {
      return GimvImageViewPlayableStop;
   } else if (status == GimvMPlayerStatusPlay) {
      return GimvImageViewPlayablePlay;
   }

   return GimvImageViewPlayablePlay;
}


static void
imageview_mplayer_seek (GimvImageView *iv, gfloat pos)
{
   GimvMPlayer *mplayer;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));

   mplayer = get_mplayer (iv);
   g_return_if_fail (mplayer);

   gimv_mplayer_seek (mplayer, pos);
}


static guint
imageview_mplayer_get_length (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   gfloat len;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);

   mplayer = get_mplayer (iv);
   g_return_val_if_fail (mplayer, GimvImageViewPlayableStop);

   len = gimv_mplayer_get_length (mplayer);

   return len * 10000;
}


static guint
imageview_mplayer_get_position (GimvImageView *iv)
{
   GimvMPlayer *mplayer;
   gfloat pos;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), 0);

   mplayer = get_mplayer (iv);
   g_return_val_if_fail (mplayer, GimvImageViewPlayableStop);

   pos = gimv_mplayer_get_position (mplayer);

   return pos * 10000;
}

#endif /* ENABLE_MPLAYER */
