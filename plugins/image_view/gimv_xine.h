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
 * the xine engine in a widget - header
 */

#ifndef __GIMV_XINE_H__
#define __GIMV_XINE_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef ENABLE_XINE

#include <gtk/gtkwidget.h>
#include <xine.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 *  all time unit is millisecond
 */


#define GIMV_TYPE_XINE            (gimv_xine_get_type ())
#define GIMV_XINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_XINE, GimvXine))
#define GIMV_XINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_XINE, GimvXineClass))
#define GIMV_IS_XINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_XINE))
#define GIMV_IS_XINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_XINE))
#define GIMV_XINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_XINE, GimvXineClass))

typedef enum
{
   GIMV_XINE_STATUS_EXIT = -1,
   GIMV_XINE_STATUS_IDLE
} GimvXineStatus;


typedef enum
{
   GIMV_XINE_SPEED_PAUSE  = XINE_SPEED_PAUSE,
   GIMV_XINE_SPEED_SLOW_4 = XINE_SPEED_SLOW_4,
   GIMV_XINE_SPEED_SLOW_2 = XINE_SPEED_SLOW_2,
   GIMV_XINE_SPEED_NORMAL = XINE_SPEED_NORMAL,
   GIMV_XINE_SPEED_FAST_2 = XINE_SPEED_FAST_2,
   GIMV_XINE_SPEED_FAST_4 = XINE_SPEED_FAST_4
} GimvXineSpeed;


typedef struct GimvXine_Tag        GimvXine;
typedef struct GimvXineClass_Tag   GimvXineClass;
typedef struct GimvXinePrivate_Tag GimvXinePrivate;

struct GimvXine_Tag
{
   GtkWidget        widget;

   GimvXinePrivate *private;
};

struct GimvXineClass_Tag
{
   GtkWidgetClass parent_class;

   void   (*play)              (GimvXine *gtx);
   void   (*stop)              (GimvXine *gtx);
   void   (*playback_finished) (GimvXine *gtx);
   gchar *(*need_next_mrl)     (GimvXine *gtx);
   void   (*branched)          (GimvXine *gtx);
};


GtkType      gimv_xine_get_type               (void);
GtkWidget   *gimv_xine_new                    (const gchar *video_driver_id,
                                               const gchar *audio_driver_id);
const char * const *
             gimv_xine_get_video_out_plugins  (GimvXine    *gtx);
const char * const *
             gimv_xine_get_audio_out_plugins  (GimvXine    *gtx);
const gchar *gimv_xine_get_video_out_driver_id(GimvXine    *gtx);
const gchar *gimv_xine_get_audio_out_driver_id(GimvXine    *gtx);
const gchar *gimv_xine_get_default_video_out_driver_id
                                              (GimvXine    *gtx);
const gchar *gimv_xine_get_default_audio_out_driver_id
                                              (GimvXine    *gtx);
void         gimv_xine_set_visibility         (GimvXine    *gtx,
                                               GdkVisibilityState state);
void         gimv_xine_resize                 (GimvXine    *gtx, 
                                               gint         x,
                                               gint         y,
                                               gint         width, 
                                               gint         height);

gint         gimv_xine_set_mrl                (GimvXine    *gtx,
                                               const gchar *mrl);
gint         gimv_xine_play                   (GimvXine    *gtx,
                                               gint         pos,
                                               gint         start_time);
void         gimv_xine_set_speed              (GimvXine    *gtx,
                                               gint         speed);
gint         gimv_xine_get_speed              (GimvXine    *gtx);
gint         gimv_xine_get_position           (GimvXine    *gtx);
void         gimv_xine_stop                   (GimvXine    *gtx);
void         gimv_xine_set_audio_channel      (GimvXine    *gtx,
                                               gint         audio_channel);
gint         gimv_xine_get_audio_channel      (GimvXine    *gtx);

gchar      **gimv_xine_get_autoplay_plugins   (GimvXine    *gtx);

gint         gimv_xine_get_current_time       (GimvXine    *gtx);
gint         gimv_xine_get_stream_length      (GimvXine    *gtx);

gint         gimv_xine_is_playing             (GimvXine    *gtx);
gint         gimv_xine_is_seekable            (GimvXine    *gtx);
void         gimv_xine_set_video_property     (GimvXine    *gtx, 
                                               gint         property, 
                                               gint         value);
gint         gimv_xine_get_video_property     (GimvXine    *gtx, 
                                               gint         property);

gint         gimv_xine_get_log_section_count  (GimvXine    *gtx);
gchar      **gimv_xine_get_log_names          (GimvXine    *gtx);
gchar      **gimv_xine_get_log                (GimvXine    *gtx,
                                               gint         buf);

gchar      **gimv_xine_get_browsable_input_plugin_ids (GimvXine    *gtx);
gchar      **gimv_xine_get_autoplay_input_plugin_ids  (GimvXine    *gtx);
gchar      **gimv_xine_get_autoplay_mrls              (GimvXine    *gtx,
                                                       const gchar *plugin_id,
                                                       gint        *num_mrls);

gint         gimv_xine_get_audio_lang         (GimvXine    *gtx,
                                               gint         channel,
                                               gchar       *lang);
gint         gimv_xine_get_spu_lang           (GimvXine    *gtx,
                                               gint         channel,
                                               gchar       *lang);
gint         gimv_xine_get_stream_info        (GimvXine    *gtx,
                                               gint         info);
const gchar *gimv_xine_get_meta_info          (GimvXine    *gtx,
                                               gint         info);
gint         gimv_xine_get_current_frame      (GimvXine    *gtx,
                                               gint        *width,
                                               gint        *height,
                                               gint        *ratio_code,
                                               gint        *format,
                                               uint8_t     *img);
guchar      *gimv_xine_get_current_frame_rgb  (GimvXine    *gtx,
                                               gint        *width_ret,
                                               gint        *height_ret);


gint         gimv_xine_trick_mode             (GimvXine    *gtx,
                                               gint         mode,
                                               gint         value);
gint         gimv_xine_get_error              (GimvXine    *gtx);
gint         gimv_xine_get_status             (GimvXine    *gtx);
void         gimv_xine_set_param              (GimvXine    *gtx,
                                               gint         param,
                                               gint         value);
gint         gimv_xine_get_param              (GimvXine    *gtx,
                                               gint         param);
void         gimv_xine_event_send             (GimvXine    *gtx,
                                               const xine_event_t *event);

gchar       *gimv_xine_get_file_extensions    (GimvXine    *gtx);
gchar       *gimv_xine_get_mime_types         (GimvXine    *gtx);

const gchar *gimv_xine_config_register_string (GimvXine    *gtx,
                                               const gchar *key,
                                               const gchar *def_value,
                                               const gchar *description,
                                               const gchar *help,
                                               gint         exp_level,
                                               xine_config_cb_t changed_cb,
                                               void        *cb_data);
gint         gimv_xine_config_register_range  (GimvXine    *gtx,
                                               const gchar *key,
                                               gint         def_value,
                                               gint         min,
                                               gint         max,
                                               const gchar *description,
                                               const gchar *help,
                                               gint         exp_level,
                                               xine_config_cb_t changed_cb,
                                               void        *cb_data);
gint         gimv_xine_config_register_enum   (GimvXine    *gtx,
                                               const gchar *key,
                                               gint         def_value,
                                               gchar      **values,
                                               const gchar *description,
                                               const gchar *help,
                                               gint         exp_level,
                                               xine_config_cb_t changed_cb,
                                               void        *cb_data);
gint         gimv_xine_config_register_num    (GimvXine    *gtx,
                                               const gchar *key,
                                               gint         def_value,
                                               const gchar *description,
                                               const gchar *help,
                                               gint         exp_level,
                                               xine_config_cb_t changed_cb,
                                               void        *cb_data);
gint         gimv_xine_config_register_bool   (GimvXine    *gtx,
                                               const gchar *key,
                                               gint         def_value,
                                               const gchar *description,
                                               const gchar *help,
                                               gint         exp_level,
                                               xine_config_cb_t changed_cb,
                                               void        *cb_data);
int          gimv_xine_config_get_first_entry (GimvXine    *gtx,
                                               xine_cfg_entry_t *entry);
int          gimv_xine_config_get_next_entry  (GimvXine    *gtx,
                                               xine_cfg_entry_t *entry);
int          gimv_xine_config_lookup_entry    (GimvXine    *gtx,
                                               const gchar *key,
                                               xine_cfg_entry_t *entry);
void         gimv_xine_config_update_entry    (GimvXine    *gtx,
                                               xine_cfg_entry_t *entry);
void         gimv_xine_config_load            (GimvXine    *gtx,
                                               const gchar *cfg_filename);
void         gimv_xine_config_save            (GimvXine    *gtx,
                                               const gchar *cfg_filename);
void         gimv_xine_config_reset           (GimvXine    *gtx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENABLE_XINE */

#endif /* __GIMV_XINE_H__ */

