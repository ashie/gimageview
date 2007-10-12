/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2002 Takuro Ashie
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

#include "gimv_image_loader.h"

#include "gimv_anim.h"
#include "gtk2-compat.h"
#include "fileutil.h"


typedef enum {
   LOAD_START_SIGNAL,
   PROGRESS_UPDATE_SIGNAL,
   LOAD_END_SIGNAL,
   LAST_SIGNAL
} GimvImageLoaderSignalType;


typedef enum
{
   GIMV_IMAGE_LOADER_LOADING_FLAG   = 1 << 0,
   GIMV_IMAGE_LOADER_CANCEL_FLAG    = 1 << 1,
   GIMV_IMAGE_LOADER_SHOW_TIME_FLAG = 1 << 2,
   GIMV_IMAGE_LOADER_DEBUG_FLAG     = 1 << 3
} GimvImageLoaderFlags;


struct GimvImageLoaderPriv_Tag
{
   GimvIO      *gio;
   gboolean     use_specified_gio;
   GimvImage   *image;
   GimvImageLoaderLoadType load_type;
   gboolean     load_as_animation;

   gfloat       width_scale;
   gfloat       height_scale;

   /* size request */
   gint         max_width;
   gint         max_height;
   gboolean     keep_aspect;

   const gchar *temp_file;

   gint         flags;

   /* que */
   GimvImageInfo *next_info;
};

static void      gimv_image_loader_class_init (GimvImageLoaderClass *klass);
static void      gimv_image_loader_init       (GimvImageLoader      *loader);
static void      gimv_image_loader_destroy    (GtkObject        *object);
static gboolean  idle_gimv_image_loader_load  (gpointer          data);

/* callback */
static void      gimv_image_loader_load_end   (GimvImageLoader      *loader);

static GtkObjectClass *parent_class = NULL;
static gint gimv_image_loader_signals[LAST_SIGNAL] = {0};


/****************************************************************************
 *
 *  Plugin Management
 *
 ****************************************************************************/
static GList *gimv_image_loader_list = NULL;


static gint
comp_func_priority (GimvImageLoaderPlugin *plugin1,
                    GimvImageLoaderPlugin *plugin2)
{
   g_return_val_if_fail (plugin1, 1);
   g_return_val_if_fail (plugin2, -1);

   return plugin1->priority_hint - plugin2->priority_hint;
}


gboolean
gimv_image_loader_plugin_regist (const gchar *plugin_name,
                                 const gchar *module_name,
                                 gpointer impl,
                                 gint     size)
{
   GimvImageLoaderPlugin *image_loader = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (image_loader, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (image_loader->if_version == GIMV_IMAGE_LOADER_IF_VERSION,
                         FALSE);
   g_return_val_if_fail (image_loader->id, FALSE);

   gimv_image_loader_list = g_list_append (gimv_image_loader_list,
                                           image_loader);

   gimv_image_loader_list = g_list_sort (gimv_image_loader_list,
                                         (GCompareFunc) comp_func_priority);

   return TRUE;
}



/****************************************************************************
 *
 *
 *
 ****************************************************************************/
GtkType
gimv_image_loader_get_type (void)
{
   static GtkType gimv_image_loader_type = 0;

   if (!gimv_image_loader_type) {
      static const GtkTypeInfo gimv_image_loader_info = {
         "GimvImageLoader",
         sizeof (GimvImageLoader),
         sizeof (GimvImageLoaderClass),
         (GtkClassInitFunc) gimv_image_loader_class_init,
         (GtkObjectInitFunc) gimv_image_loader_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_image_loader_type = gtk_type_unique (gtk_object_get_type (),
                                                &gimv_image_loader_info);
   }

   return gimv_image_loader_type;
}


static void
gimv_image_loader_class_init (GimvImageLoaderClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gtk_object_get_type ());

   gimv_image_loader_signals[LOAD_START_SIGNAL]
      = gtk_signal_new ("load_start",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageLoaderClass, load_start),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_image_loader_signals[PROGRESS_UPDATE_SIGNAL]
      = gtk_signal_new ("progress_update",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageLoaderClass, load_end),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_image_loader_signals[LOAD_END_SIGNAL]
      = gtk_signal_new ("load_end",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageLoaderClass, load_end),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gtk_object_class_add_signals (object_class,
                                 gimv_image_loader_signals, LAST_SIGNAL);

   object_class->destroy  = gimv_image_loader_destroy;

   klass->load_start      = NULL;
   klass->progress_update = NULL;
   klass->load_end        = gimv_image_loader_load_end;
}


static void
gimv_image_loader_init (GimvImageLoader *loader)
{
   loader->info                    = NULL;
   loader->timer                   = g_timer_new ();

   loader->priv                    = g_new0 (GimvImageLoaderPriv, 1);
   loader->priv->gio               = NULL;
   loader->priv->use_specified_gio = FALSE;
   loader->priv->image             = NULL;
   loader->priv->load_type         = GIMV_IMAGE_LOADER_LOAD_NORMAL;
   loader->priv->load_as_animation = FALSE;
   loader->priv->width_scale       = -1.0;
   loader->priv->height_scale      = -1.0;
   loader->priv->max_width         = -1;
   loader->priv->max_height        = -1;
   loader->priv->keep_aspect       = TRUE;
   loader->priv->temp_file         = NULL;
   loader->priv->flags             = 0;
   loader->priv->next_info         = NULL;

   gtk_object_ref (GTK_OBJECT (loader));
   gtk_object_sink (GTK_OBJECT (loader));
}


GimvImageLoader *
gimv_image_loader_new (void)
{
   GimvImageLoader *loader
      = GIMV_IMAGE_LOADER (gtk_type_new (gimv_image_loader_get_type ()));

   return loader;
}


GimvImageLoader *
gimv_image_loader_new_with_image_info (GimvImageInfo *info)
{
   GimvImageLoader *loader;

   loader = gimv_image_loader_new ();
   gimv_image_loader_set_image_info (loader, info);

   return loader;
}


GimvImageLoader *
gimv_image_loader_new_with_file_name (const gchar *filename)
{
   GimvImageLoader *loader;
   GimvImageInfo *info;

   info = gimv_image_info_get (filename);
   if (!info) return NULL;
   loader = gimv_image_loader_new_with_image_info (info);

   return loader;
}


GimvImageLoader *
gimv_image_loader_ref (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);

   gtk_object_ref (GTK_OBJECT (loader));

   return loader;
}


void
gimv_image_loader_unref (GimvImageLoader *loader)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));

   gtk_object_unref (GTK_OBJECT (loader));
}


static void
gimv_image_loader_destroy (GtkObject *object)
{
   GimvImageLoader *loader = GIMV_IMAGE_LOADER (object);

   if (gimv_image_loader_is_loading (loader))
      gimv_image_loader_load_stop (loader);

   if (loader->info)
      gimv_image_info_unref (loader->info);
   loader->info = NULL;

   if (loader->timer)
      g_timer_destroy (loader->timer);
   loader->timer = NULL;

   if (loader->priv) {
      if (loader->priv->next_info)
         gimv_image_info_unref (loader->priv->next_info);
      loader->priv->next_info = NULL;

      if (loader->priv->gio)
         gimv_io_unref (loader->priv->gio);
      loader->priv->gio = NULL;

      if (loader->priv->image)
         gimv_image_unref (loader->priv->image);
      loader->priv->image = NULL;

      if (loader->priv->temp_file)
         g_free ((gchar *) loader->priv->temp_file);
      loader->priv->temp_file = NULL;

      g_free (loader->priv);
      loader->priv = NULL;
   }

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


void
gimv_image_loader_set_image_info (GimvImageLoader *loader, GimvImageInfo *info)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (info);
   g_return_if_fail (loader->priv);

   if (gimv_image_loader_is_loading (loader)) {
      gimv_image_loader_load_stop (loader);
      if (loader->priv->next_info)
         gimv_image_info_unref (loader->priv->next_info);
      loader->priv->next_info = gimv_image_info_ref (info);
      return;
   }

   if (loader->priv->image)
      gimv_image_unref (loader->priv->image);
   loader->priv->image = NULL;

   g_timer_reset (loader->timer);

   if (loader->priv->temp_file)
      g_free ((gchar *)loader->priv->temp_file);
   loader->priv->temp_file = NULL;

   if (loader->info)
      gimv_image_info_unref (loader->info);

   if (info)
      loader->info = gimv_image_info_ref (info);
   else
      loader->info = NULL;
}


void
gimv_image_loader_set_gio (GimvImageLoader *loader, GimvIO *gio)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (!gimv_image_loader_is_loading (loader));
   g_return_if_fail (loader->priv);

   loader->priv->gio = gimv_io_ref (gio);
   if (gio)
      loader->priv->use_specified_gio = TRUE;
   else
      loader->priv->use_specified_gio = FALSE;
}


gboolean
gimv_image_loader_set_as_animation (GimvImageLoader *loader,
                                    gboolean animation)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (!gimv_image_loader_is_loading (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   loader->priv->load_as_animation = animation;

   return TRUE;
}


gboolean
gimv_image_loader_set_load_type (GimvImageLoader *loader,
                                 GimvImageLoaderLoadType type)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (!gimv_image_loader_is_loading (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   loader->priv->load_type = type;

   return TRUE;
}


gboolean
gimv_image_loader_set_scale (GimvImageLoader *loader,
                             gfloat w_scale,
                             gfloat h_scale)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (!gimv_image_loader_is_loading (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   loader->priv->width_scale  = w_scale;
   loader->priv->height_scale = h_scale;

   return TRUE;
}


gboolean
gimv_image_loader_set_size_request (GimvImageLoader *loader,
                                    gint max_width,
                                    gint max_height,
                                    gboolean keep_aspect)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (!gimv_image_loader_is_loading (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   loader->priv->max_width   = max_width;
   loader->priv->max_height  = max_height;
   loader->priv->keep_aspect = keep_aspect;

   return TRUE;
}


void
gimv_image_loader_load_start (GimvImageLoader *loader)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (loader->info);
   g_return_if_fail (loader->priv);

   g_return_if_fail (!gimv_image_loader_is_loading(loader));

   loader->priv->flags &= ~GIMV_IMAGE_LOADER_LOADING_FLAG;
   loader->priv->flags &= ~GIMV_IMAGE_LOADER_CANCEL_FLAG;

   /* gtk_idle_add (idle_gimv_image_loader_load, loader); */
   gimv_image_loader_load (loader);
}


void
gimv_image_loader_load_stop (GimvImageLoader *loader)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (loader->priv);

   if (loader->priv->flags & GIMV_IMAGE_LOADER_LOADING_FLAG) {
      loader->priv->flags |= GIMV_IMAGE_LOADER_CANCEL_FLAG;
   }
}


gboolean
gimv_image_loader_is_loading (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);

   if (loader->priv && (loader->priv->flags & GIMV_IMAGE_LOADER_LOADING_FLAG))
      return TRUE;
   else
      return FALSE;
}


GimvImage *
gimv_image_loader_get_image (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);
   g_return_val_if_fail (loader->priv, NULL);

   return loader->priv->image;
}


void
gimv_image_loader_unref_image (GimvImageLoader *loader)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (loader->priv);

   if (loader->priv->image)
      gimv_image_unref (loader->priv->image);
   loader->priv->image = NULL;
}


GimvIO *
gimv_image_loader_get_gio (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);
   g_return_val_if_fail (loader->priv, NULL);

   return loader->priv->gio;
}


const gchar *
gimv_image_loader_get_path (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), NULL);
   g_return_val_if_fail (loader->priv, NULL);

   if (!loader->info) return NULL;

   if (gimv_image_info_need_temp_file (loader->info)) {
      if (!loader->priv->temp_file)
         loader->priv->temp_file
            = gimv_image_info_get_temp_file_path (loader->info);
      return loader->priv->temp_file;
   }

   return gimv_image_info_get_path (loader->info);
}


GimvImageLoaderLoadType
gimv_image_loader_get_load_type (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader),
                         GIMV_IMAGE_LOADER_LOAD_NORMAL);
   g_return_val_if_fail (loader->priv,
                         GIMV_IMAGE_LOADER_LOAD_NORMAL);

   return loader->priv->load_type;
}


gboolean
gimv_image_loader_load_as_animation (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   return loader->priv->load_as_animation;
}


gboolean
gimv_image_loader_get_scale (GimvImageLoader *loader,
                             gfloat *width_scale_ret,
                             gfloat *height_scale_ret)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (width_scale_ret && height_scale_ret, FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   *width_scale_ret  = loader->priv->width_scale;
   *height_scale_ret = loader->priv->height_scale;

   if (*width_scale_ret < 0.0 || *height_scale_ret < 0.0)
      return FALSE;

   return TRUE;
}


gboolean
gimv_image_loader_get_size_request (GimvImageLoader *loader,
                                    gint *max_width,
                                    gint *max_height,
                                    gboolean *keep_aspect)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (max_width && max_height && keep_aspect, FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   *max_width   = loader->priv->max_width;
   *max_height  = loader->priv->max_height;
   *keep_aspect = loader->priv->keep_aspect;

   return TRUE;
}


gboolean
gimv_image_loader_progress_update (GimvImageLoader *loader)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_LOADER (loader), FALSE);
   g_return_val_if_fail (gimv_image_loader_is_loading (loader), FALSE);
   g_return_val_if_fail (loader->priv, FALSE);

   gtk_signal_emit (GTK_OBJECT(loader),
                    gimv_image_loader_signals[PROGRESS_UPDATE_SIGNAL]);

   if (loader->priv->flags & GIMV_IMAGE_LOADER_CANCEL_FLAG)
      return FALSE;
   else
      return TRUE;
}


void
gimv_image_loader_load (GimvImageLoader *loader)
{
   GList *node;
   gboolean cancel = FALSE;

   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (loader->priv);

   if (gimv_image_loader_is_loading (loader)) {
      return;
   }

   /* set flags */
   loader->priv->flags &= ~GIMV_IMAGE_LOADER_LOADING_FLAG;
   loader->priv->flags &= ~GIMV_IMAGE_LOADER_CANCEL_FLAG;
   loader->priv->flags |= GIMV_IMAGE_LOADER_LOADING_FLAG;

   gtk_signal_emit (GTK_OBJECT(loader),
                    gimv_image_loader_signals[LOAD_START_SIGNAL]);

   g_timer_reset (loader->timer);
   g_timer_start (loader->timer);

   /* load GimvIO and name of temporary file */
   if (loader->info) {
      if (!loader->priv->use_specified_gio) {
         if (loader->priv->gio)
            gimv_io_unref (loader->priv->gio);
         loader->priv->gio = gimv_image_info_get_gio (loader->info);

         if (gimv_image_info_need_temp_file (loader->info)) {
            g_free ((gchar *) loader->priv->temp_file);
            loader->priv->temp_file
               = gimv_image_info_get_temp_file (loader->info);
            if (!loader->priv->temp_file) {
               if (loader->priv->gio) {
                  gimv_io_unref (loader->priv->gio);
                  loader->priv->gio = NULL;
               }
               g_timer_stop (loader->timer);
               g_timer_reset (loader->timer);
               return;
            }
         }
      }
   }

   /* free old image */
   if (loader->priv->image)
      gimv_image_unref (loader->priv->image);
   loader->priv->image = NULL;

   /* try all loader */
   for (node = gimv_image_loader_list; node; node = g_list_next (node)) {
      GimvImageLoaderPlugin *plugin = node->data;

      cancel = !gimv_image_loader_progress_update (loader);
      if (cancel) break;

      if (!plugin->loader) continue;

      if (loader->priv->gio)
         gimv_io_seek (loader->priv->gio, 0, SEEK_SET);

      loader->priv->image = plugin->loader (loader, NULL);

      if (loader->priv->image) {
         if (loader->info && !GIMV_IMAGE_INFO_IS_SYNCED(loader->info)) {
            GimvImageInfoFlags flags = GIMV_IMAGE_INFO_SYNCED_FLAG;

            /* set image info */
            if (GIMV_IS_ANIM (loader->priv->image)) {
               flags |= GIMV_IMAGE_INFO_ANIMATION_FLAG;
            }
            gimv_image_info_set_size (loader->info,
                                 gimv_image_width (loader->priv->image),
                                 gimv_image_height (loader->priv->image));
            gimv_image_info_set_flags (loader->info, flags);
         }
         break;
      }

      if (loader->priv->flags & GIMV_IMAGE_LOADER_CANCEL_FLAG) break;
   }

   /* free GimvIO if needed */
   if (!loader->priv->use_specified_gio) {
      if (loader->priv->gio)
         gimv_io_unref (loader->priv->gio);
      loader->priv->gio = NULL;
   }

   g_timer_stop (loader->timer);

   if (loader->priv->flags & GIMV_IMAGE_LOADER_SHOW_TIME_FLAG)
      g_print ("GimvImageLoader: %f[sec] (%s)\n",
               g_timer_elapsed (loader->timer, NULL),
               gimv_image_loader_get_path (loader));

   /* reset flags and emit signal */
   loader->priv->flags &= ~GIMV_IMAGE_LOADER_LOADING_FLAG;
   if (loader->priv->flags & GIMV_IMAGE_LOADER_CANCEL_FLAG) {
      loader->priv->flags &= ~GIMV_IMAGE_LOADER_CANCEL_FLAG;
      if (loader->priv->flags & GIMV_IMAGE_LOADER_DEBUG_FLAG)
         g_print ("----- loading canceled -----\n");
      /* emit canceled signal? */
      gtk_signal_emit (GTK_OBJECT(loader),
                       gimv_image_loader_signals[LOAD_END_SIGNAL]);
   } else {
      if (loader->priv->flags & GIMV_IMAGE_LOADER_DEBUG_FLAG)
         g_print ("----- loading done -----\n");
      gtk_signal_emit (GTK_OBJECT(loader),
                       gimv_image_loader_signals[LOAD_END_SIGNAL]);
   }
}


static gboolean
idle_gimv_image_loader_load (gpointer data)
{
   GimvImageLoader *loader = data;

   gimv_image_loader_load (loader);

   return FALSE;
}


static void
gimv_image_loader_load_end (GimvImageLoader *loader)
{
   g_return_if_fail (GIMV_IS_IMAGE_LOADER (loader));
   g_return_if_fail (loader->priv);

   if (!loader->priv->next_info)
      return;

   loader->info = loader->priv->next_info;
   loader->priv->next_info = NULL;
   gtk_idle_add (idle_gimv_image_loader_load, loader);
}
