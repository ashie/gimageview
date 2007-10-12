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

#ifndef __GIMV_MPLAYER_H__
#define __GIMV_MPLAYER_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtkwidget.h>

#ifdef ENABLE_MPLAYER

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMV_MPLAYER(obj)            (GTK_CHECK_CAST ((obj), gimv_mplayer_get_type (), GimvMPlayer))
#define GIMV_MPLAYER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), gimv_mplayer_get_type (), GimvMPlayerClass))
#define GIMV_IS_MPLAYER(obj)         (GTK_CHECK_TYPE (obj, gimv_mplayer_get_type ()))
#define GIMV_IS_MPLAYER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), gimv_mplayer_get_type ()))


#ifdef DATADIR
#define GIMV_MPLAYER_INCLUDE_FILE DATADIR "/mplayerrc"
#else /* DATADIR */
#define GIMV_MPLAYER_INCLUDE_FILE NULL
#endif /* DATADIR */


typedef struct GimvMPlayer_Tag          GimvMPlayer;
typedef struct GimvMPlayerClass_Tag     GimvMPlayerClass;
typedef struct GimvMPlayerMediaInfo_Tag GimvMPlayerMediaInfo;


typedef enum {
   GimvMPlayerStatusStop,
   GimvMPLayerStatusDetecting,
   GimvMPlayerStatusPlay,
   GimvMPlayerStatusPause
} GimvMPlayerStatus;


typedef enum {
   GimvMPlayerEmbedFlag              = 1 << 0,
   GimvMPlayerAllowFrameDroppingFlag = 1 << 1,
} GimvMPlayerFlags;


struct GimvMPlayerMediaInfo_Tag
{
   gfloat   length;

   /* video */
   struct GimvMPlayerVideoInfo_Tag {
      gchar  format[16];
      gint   bitrate;
      gint   width;
      gint   height;
      gfloat aspect;
      gfloat fps;
   } *video;

   /* audio */
   struct GimvMPlayerAudioInfo_Tag {
      gchar  codec[16];
      gint   format;
      gint   bitrate;
      gint   rate;
      gint   n_ch;
   } *audio;

   /* other */
#if 0
   GimvMPlayerStreamType stream_type;
   typedef union GimvMPlayeStreamInfo_Tag {
      GimvMPlayerDVDInfo *dvdinfo;
      GimvMPlayerVCDInfo *vcdinfo;
   } GimvMPlayeStreamInfo *stream_info;
#endif
};


struct GimvMPlayer_Tag
{
   GtkWidget          parent;

   gchar             *filename;
   gfloat             pos;
   gfloat             speed;
   GimvMPlayerStatus  status;
   GimvMPlayerFlags   flags;

   /* command */
   gchar             *command;
   gchar             *vo;        /* video out driver's name */
   gchar             *ao;        /* audio out driver's name */
   GList             *args;      /* other arguments specified by user */

   /* config file for GimvMPlayer */
   gchar             *include_file;

   /* media info */
   GimvMPlayerMediaInfo media_info;
};

struct GimvMPlayerClass_Tag
{
   GtkWidgetClass parent_class;

   /* signals */

   void (*play)             (GimvMPlayer *player);
   void (*stop)             (GimvMPlayer *player);
   void (*pause)            (GimvMPlayer *player);
   void (*position_changed) (GimvMPlayer *player);
   void (*identified)       (GimvMPlayer *player);
};


GtkType           gimv_mplayer_get_type              (void);
GtkWidget        *gimv_mplayer_new                   (void);
gboolean          gimv_mplayer_set_file              (GimvMPlayer  *player,
                                                      const gchar  *file);
void              gimv_mplayer_set_flags             (GimvMPlayer  *player,
                                                      GimvMPlayerFlags flags);
void              gimv_mplayer_unset_flags           (GimvMPlayer  *player, 
                                                      GimvMPlayerFlags flags);
GimvMPlayerFlags  gimv_mplayer_get_flags             (GimvMPlayer *player,
                                                      GimvMPlayerFlags flags);
void              gimv_mplayer_set_video_out_driver  (GimvMPlayer *player,
                                                      const gchar *vo_driver);
void              gimv_mplayer_set_audio_out_driver  (GimvMPlayer *player,
                                                      const gchar *ao_driver);

const GList      *gimv_mplayer_get_video_out_drivers (GimvMPlayer *player,
                                                      gboolean     refresh);
const GList      *gimv_mplayer_get_audio_out_drivers (GimvMPlayer *player,
                                                      gboolean     refresh);

gint              gimv_mplayer_get_width             (GimvMPlayer  *player);
gint              gimv_mplayer_get_height            (GimvMPlayer  *player);
gfloat            gimv_mplayer_get_length            (GimvMPlayer  *player);
gfloat            gimv_mplayer_get_position          (GimvMPlayer  *player);
GimvMPlayerStatus gimv_mplayer_get_status            (GimvMPlayer  *player);
gboolean          gimv_mplayer_is_running            (GimvMPlayer  *player);

void              gimv_mplayer_play                  (GimvMPlayer  *player);
void              gimv_mplayer_start                 (GimvMPlayer  *player,
                                                      gfloat        pos,
                                                      gfloat        speed);
void              gimv_mplayer_stop                  (GimvMPlayer  *player);
void              gimv_mplayer_toggle_pause          (GimvMPlayer  *player);
void              gimv_mplayer_set_speed             (GimvMPlayer  *player,
                                                      gfloat        speed);
gfloat            gimv_mplayer_get_speed             (GimvMPlayer  *player);
void              gimv_mplayer_seek                  (GimvMPlayer  *player,
                                                      gfloat        percentage);
void              gimv_mplayer_seek_by_time          (GimvMPlayer  *player,
                                                      gfloat        second);
void              gimv_mplayer_seek_relative         (GimvMPlayer  *player,
                                                      gfloat        second);
void              gimv_mplayer_set_audio_delay       (GimvMPlayer  *player,
                                                      gfloat        second);

/*
 *  gimv_mplayer_get_frame()
 *
 *  mplayer:
 *  vo_driver: vo_driver to use ("png", "jpeg", "gif89a" ...).
 *             If this value is NULL, use "png".
 *  tmp_dir:   Directory to output image file
 *  pos:       Stream position of the frame [sec]. 
 *             If 0 or negative value, ensure to get current frame.
 *  frames:    Count of output frames to ensure to extract the frame completely.
 *             If 0 or negative value, use default value (5 frames).
 *  maine_iterate:
 *  Return:    File name of the saved frame image.
 *             The image file should be moved/copied or proccessed immediately.
 */
gchar            *gimv_mplayer_get_frame             (GimvMPlayer *player,
                                                      const gchar *vo_driver,
                                                      const gchar *tmp_dir,
                                                      gfloat       pos,
                                                      gint         frames,
                                                      gboolean     main_iterate);
/* will be called automatically when the player is destroyed */
void              gimv_mplayer_flush_tmp_files       (GimvMPlayer *player);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENABLE_MPLAYER */

#endif /* __GIMV_MPLAYER_H__ */
