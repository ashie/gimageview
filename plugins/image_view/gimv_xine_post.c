/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/* 
 * Copyright (C) 2000-2003 the xine project
 * 
 * This file is part of xine, a unix video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 */

/*
 *  2003-04-11 Takuro Ashie <ashie@homa.ne.jp>
 *
 *    * merged from xine-ui-0.9.18 and adapt to GimvXine.
 */

#include "gimv_xine_post.h"

#include "gimv_xine_priv.h"


static char **post_audio_plugins;
static char **post_video_plugins;


static void
post_audio_plugin_cb (void *data, xine_cfg_entry_t *cfg)
{
   GimvXine *gxine = data;

   gxine->private->visual_anim.post_plugin_num = cfg->num_value;

#if 0 /* xine doesn't support rewiring on the fly */
   post_rewire_visual_anim (gxine);
#else
   gxine->private->visual_anim.post_changed = 1;
#endif
}

static void
post_video_plugin_cb (void *data, xine_cfg_entry_t *cfg)
{
   GimvXine *gxine = data;

   gxine->private->post_video_num = cfg->num_value;
   post_rewire_video_post (gxine);
}

void
post_init (GimvXine *gxine)
{
   gxine->private->visual_anim.post_output = NULL;
   gxine->private->visual_anim.post_plugin_num = -1;
   gxine->private->visual_anim.post_changed = 0;

   if (gxine->private->ao_driver) {
      const char *const *pol
         = xine_list_post_plugins_typed (gxine->private->xine, 
                                         XINE_POST_TYPE_AUDIO_VISUALIZATION);

      if (pol) {
         int  i = 0;
         int  num_plug = 0;
      
         /* We're only interested by post plugin which handle audio in input */
         while (pol[i]) {
            xine_post_t *post = xine_post_init (gxine->private->xine,
                                                pol[i], 0,
                                                &gxine->private->ao_driver,
                                                &gxine->private->vo_driver);

            if (post) {
               if (num_plug == 0)
                  post_audio_plugins = g_malloc (sizeof (char *) * 2);
               else
                  post_audio_plugins = realloc (post_audio_plugins, 
                                                sizeof (char *) * (num_plug + 2));

               post_audio_plugins[num_plug]     = strdup (pol[i]);
               post_audio_plugins[num_plug + 1] = NULL;
               num_plug++;

               xine_post_dispose (gxine->private->xine, post);
            }
            i++;
         }

         if (num_plug) {
            gxine->private->visual_anim.post_plugin_num = 
               xine_config_register_enum (gxine->private->xine,
                                          "gui.post_audio_plugin", 
                                          0, post_audio_plugins,
                                          _("Post audio plugin"),
                                          _("Post audio plugin to used with video less stream playback"),
                                          CONFIG_LEVEL_BEG,
                                          post_audio_plugin_cb, 
                                          gxine);

            gxine->private->visual_anim.post_output = 
               xine_post_init (gxine->private->xine,
                               post_audio_plugins[gxine->private->visual_anim.post_plugin_num], 0,
                               &gxine->private->ao_driver, &gxine->private->vo_driver);
         }
      }
   }

   gxine->private->post_video = NULL;
   gxine->private->post_video_num = -1;

   {
      const char *const *pol = xine_list_post_plugins_typed (gxine->private->xine,
                                                             XINE_POST_TYPE_VIDEO_FILTER);
    
      if (pol) {
         int  i = 0;
         int  num_plug = 0;

         /* We're only interested by post plugin which handle video in input */
         post_video_plugins = (char **) g_malloc (sizeof (char *) * 2);
         post_video_plugins[num_plug]     = strdup (_("None"));
         post_video_plugins[num_plug + 1] = NULL;
         num_plug++;

         while (pol[i]) {
            xine_post_t *post = xine_post_init (gxine->private->xine,
                                                pol[i], 0,
                                                &gxine->private->ao_driver,
                                                &gxine->private->vo_driver);

            if (post) {
               post_video_plugins = realloc (post_video_plugins, 
                                             sizeof (char *) * (num_plug + 2));

               post_video_plugins[num_plug]     = strdup (pol[i]);
               post_video_plugins[num_plug + 1] = NULL;
               num_plug++;

               xine_post_dispose (gxine->private->xine, post);
            }
            i++;
         }

         if (num_plug) {
            gxine->private->post_video_num = 
               xine_config_register_enum (gxine->private->xine,
                                          "gui.post_video_plugin", 
                                          0, post_video_plugins,
                                          _("Post video plugin"),
                                          _("Post video plugin"),
                                          CONFIG_LEVEL_BEG,
                                          post_video_plugin_cb,
                                          gxine);

            gxine->private->post_video = 
               xine_post_init(gxine->private->xine,
                              post_video_plugins[(gxine->private->post_video_num == 0)
                                                 ? 1 : gxine->private->post_video_num],
                              0, &gxine->private->ao_driver, &gxine->private->vo_driver);
         }
      }
   }
}

void
post_rewire_visual_anim (GimvXine *gxine)
{
   if (gxine->private->visual_anim.post_output) {
      xine_post_out_t *audio_source;

      audio_source = xine_get_audio_source (gxine->private->stream);
      xine_post_wire_audio_port (audio_source, gxine->private->ao_driver);

      xine_post_dispose (gxine->private->xine,
                         gxine->private->visual_anim.post_output);
   }

   gxine->private->visual_anim.post_output = 
      xine_post_init (gxine->private->xine,
                      post_audio_plugins[gxine->private->visual_anim.post_plugin_num], 0,
                      &gxine->private->ao_driver, &gxine->private->vo_driver);

   if(gxine->private->visual_anim.post_output && 
      (gxine->private->visual_anim.enabled == 1) && (gxine->private->visual_anim.running == 1)) {

      post_rewire_audio_post_to_stream (gxine, gxine->private->stream);
   }
}

void
post_rewire_video_post (GimvXine *gxine)
{
   if (gxine->private->post_video) {
      xine_post_out_t *video_source;

      video_source = xine_get_video_source (gxine->private->stream);
      xine_post_wire_video_port (video_source, gxine->private->vo_driver);
      xine_post_dispose (gxine->private->xine, gxine->private->post_video);
   }

   gxine->private->post_video = 
      xine_post_init(gxine->private->xine,
                     post_video_plugins[(gxine->private->post_video_num == 0)
                                        ? 1 : gxine->private->post_video_num],
                     0, &gxine->private->ao_driver, &gxine->private->vo_driver);

   if(gxine->private->post_video && (gxine->private->post_video_num > 0))
      post_rewire_video_post_to_stream (gxine, gxine->private->stream);
}

int
post_rewire_audio_port_to_stream (GimvXine *gxine, xine_stream_t *stream)
{
   xine_post_out_t * audio_source;

   audio_source = xine_get_audio_source (stream);
   return xine_post_wire_audio_port (audio_source, gxine->private->ao_driver);
}

int
post_rewire_audio_post_to_stream (GimvXine *gxine, xine_stream_t *stream)
{
   xine_post_out_t * audio_source;
  
   audio_source = xine_get_audio_source (stream);
   return xine_post_wire_audio_port (audio_source,
                                     gxine->private->visual_anim.post_output->audio_input[0]);
}

int
post_rewire_video_post_to_stream (GimvXine *gxine, xine_stream_t *stream)
{
   xine_post_out_t *video_source;

   video_source = xine_get_video_source (stream);
   return xine_post_wire_video_port (video_source,
                                     gxine->private->post_video->video_input[0]);
}
