/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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

#include "gimv_image_saver.h"


typedef enum {
   SAVE_START_SIGNAL,
   PROGRESS_UPDATE_SIGNAL,
   SAVE_END_SIGNAL,
   LAST_SIGNAL
} GimvImageLoaderSignalType;


typedef enum
{
   GIMV_IMAGE_SAVER_SAVING_FLAG    = 1 << 0,
   GIMV_IMAGE_SAVER_CANCEL_FLAG    = 1 << 1,
   GIMV_IMAGE_SAVER_SHOW_TIME_FLAG = 1 << 2,
   GIMV_IMAGE_SAVER_DEBUG_FLAG     = 1 << 3
} GimvImageSaverFlags;


struct GimvImageSaverPriv_Tag
{
   GimvImageSaverFlags flags;
};


static void gimv_image_saver_destroy    (GtkObject           *object);


static gint gimv_image_saver_signals[LAST_SIGNAL] = {0};


/*******************************************************************************
 *
 *  Plugin Management
 *
 ******************************************************************************/
GHashTable *image_savers = NULL;

gboolean
gimv_image_saver_plugin_regist (const gchar *plugin_name,
                                const gchar *module_name,
                                gpointer impl,
                                gint     size)
{
   GimvImageSaverPlugin *saver_funcs = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (saver_funcs, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (saver_funcs->if_version == GIMV_IMAGE_SAVER_IF_VERSION, FALSE);
   g_return_val_if_fail (saver_funcs->format && saver_funcs->save, FALSE);

   if (!image_savers)
      image_savers = g_hash_table_new (g_str_hash, g_str_equal);

   g_hash_table_insert (image_savers, saver_funcs->format, saver_funcs);

   return TRUE;
}



/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
G_DEFINE_TYPE (GimvImageSaver, gimv_image_saver, GTK_TYPE_OBJECT)


static void
gimv_image_saver_class_init (GimvImageSaverClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;

   gimv_image_saver_signals[SAVE_START_SIGNAL]
      = gtk_signal_new ("save_start",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageSaverClass, save_start),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_image_saver_signals[PROGRESS_UPDATE_SIGNAL]
      = gtk_signal_new ("progress_update",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageSaverClass, save_end),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_image_saver_signals[SAVE_END_SIGNAL]
      = gtk_signal_new ("save_end",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvImageSaverClass, save_end),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   object_class->destroy  = gimv_image_saver_destroy;

   klass->save_start      = NULL;
   klass->progress_update = NULL;
   klass->save_end        = NULL;
}


static void
gimv_image_saver_init (GimvImageSaver *saver)
{
   saver->info        = NULL;
   saver->image       = NULL;
   saver->format      = NULL;
   saver->filename    = NULL;
   saver->comments    = NULL;
   saver->n_comments  = 0;
   saver->param       = NULL;

   saver->timer       = g_timer_new ();

   saver->priv        = g_new0 (GimvImageSaverPriv, 1);
   saver->priv->flags = 0;

   gtk_object_ref (GTK_OBJECT (saver));
   gtk_object_sink (GTK_OBJECT (saver));
}


static void
gimv_image_saver_destroy (GtkObject *object)
{
   GimvImageSaver *saver = GIMV_IMAGE_SAVER (object);

   gimv_image_saver_reset (saver);

   if (saver->timer) {
      g_timer_destroy (saver->timer);
      saver->timer = NULL;
   }

   if (saver->priv) {
      g_free (saver->priv);
      saver->priv = NULL;
   }

   if (GTK_OBJECT_CLASS (gimv_image_saver_parent_class)->destroy)
      (*GTK_OBJECT_CLASS (gimv_image_saver_parent_class)->destroy) (object);
}


GimvImageSaver *
gimv_image_saver_new (void)
{
   GimvImageSaver *saver
      = GIMV_IMAGE_SAVER (gtk_type_new (gimv_image_saver_get_type ()));

   return saver;
}


GimvImageSaver *
gimv_image_saver_new_with_attr (GimvImage *image,
                                const gchar *path,
                                const gchar *format)
{
   GimvImageSaver *saver
      = GIMV_IMAGE_SAVER (gtk_type_new (gimv_image_saver_get_type ()));

   gimv_image_saver_set_image (saver, image);
   gimv_image_saver_set_path (saver, path);
   gimv_image_saver_set_format (saver, format);

   return saver;
}


GimvImageSaver *
gimv_image_saver_ref (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), NULL);

   gtk_object_ref (GTK_OBJECT (saver));

   return saver;
}


void
gimv_image_saver_unref (GimvImageSaver *saver)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));

   gtk_object_unref (GTK_OBJECT (saver));
}


void
gimv_image_saver_reset (GimvImageSaver *saver)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));

   if (saver->info) {
      gimv_image_info_unref (saver->info);
      saver->info = NULL;
   }

   if (saver->image) {
      gimv_image_unref (saver->image);
      saver->image = NULL;
   }

   if (saver->format) {
      g_free (saver->format);
      saver->format = NULL;
   }

   if (saver->filename) {
      g_free (saver->filename);
      saver->filename = NULL;
   }

   if (saver->comments) {
      gint i;

      for (i = 0; i < saver->n_comments; i++) {
         g_free (saver->comments[i].key);
         g_free (saver->comments[i].text);
      }
      g_free (saver->comments);
      saver->comments = NULL;
   }
   saver->n_comments = 0;

   if (saver->param) {
      /* FIXME */
      saver->param = NULL;
   }

   if (saver->timer) {
      g_timer_stop (saver->timer);
      g_timer_reset (saver->timer);
   }

   if (saver->priv) {
      saver->priv->flags = 0;
   }
}


void
gimv_image_saver_set_image (GimvImageSaver *saver, GimvImage *image)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));

   saver->image = gimv_image_ref (image);
}


void
gimv_image_saver_set_path (GimvImageSaver *saver,
                           const gchar    *path)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));
   g_return_if_fail (path && *path);

   saver->filename = g_strdup (path);
}


void
gimv_image_saver_set_format (GimvImageSaver *saver,
                             const gchar    *format)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));
   g_return_if_fail (format && *format);

   saver->format = g_strdup (format);
}


void
gimv_image_saver_set_image_info (GimvImageSaver *saver, GimvImageInfo *info)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));

   saver->info = gimv_image_info_ref (info);
}


void
gimv_image_saver_set_comment (GimvImageSaver *saver,
                              const gchar    *key,
                              const gchar    *value)
{
   gint pos;

   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));
   g_return_if_fail (!gimv_image_saver_is_saving (saver));
   g_return_if_fail (key && *key);
   g_return_if_fail (value);

   if (!saver->comments) {
      saver->comments = g_new (GimvImageSaverText, 1);
      saver->n_comments = 1;
   } else {
      saver->n_comments++;
      saver->comments
         = g_realloc (saver->comments,
                      sizeof (GimvImageSaverText) * saver->n_comments);
   }

   pos = saver->n_comments - 1;

   saver->comments[pos].key  = g_strdup(key);
   saver->comments[pos].text = g_strdup(value);
}


gboolean
gimv_image_saver_save (GimvImageSaver *saver)
{
   GimvImageSaverPlugin *saver_funcs;
   gboolean retval;

   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), FALSE);
   g_return_val_if_fail (saver->priv, FALSE);
   g_return_val_if_fail (!gimv_image_saver_is_saving (saver), FALSE);
   g_return_val_if_fail (saver->filename, FALSE);
   g_return_val_if_fail (saver->format, FALSE);
   g_return_val_if_fail (saver->image, FALSE);

   saver->priv->flags |= GIMV_IMAGE_SAVER_SAVING_FLAG;

   gtk_signal_emit (GTK_OBJECT(saver),
                    gimv_image_saver_signals[SAVE_START_SIGNAL]);

   g_timer_reset (saver->timer);
   g_timer_start (saver->timer);

   saver_funcs = g_hash_table_lookup (image_savers, saver->format);
   if (!saver_funcs) {
      gtk_signal_emit (GTK_OBJECT(saver),
                       gimv_image_saver_signals[SAVE_END_SIGNAL]);
      g_timer_stop (saver->timer);
      g_timer_reset (saver->timer);
      return FALSE;
   }

   retval = saver_funcs->save (saver,
                               saver->image,
                               saver->filename,
                               saver->format);

   saver->priv->flags &= ~GIMV_IMAGE_SAVER_CANCEL_FLAG;
   saver->priv->flags &= ~GIMV_IMAGE_SAVER_SAVING_FLAG;

   g_timer_stop (saver->timer);

   gtk_signal_emit (GTK_OBJECT(saver),
                    gimv_image_saver_signals[SAVE_END_SIGNAL]);

   return retval;
}


void
gimv_image_saver_save_start (GimvImageSaver *saver)
{
   g_return_if_fail (!gimv_image_saver_is_saving (saver));

   gimv_image_saver_save (saver);
}


void
gimv_image_saver_save_stop (GimvImageSaver *saver)
{
   g_return_if_fail (GIMV_IS_IMAGE_SAVER (saver));

   saver->priv->flags |= GIMV_IMAGE_SAVER_CANCEL_FLAG;
}


gboolean
gimv_image_saver_is_saving (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), FALSE);

   if (saver->priv && (saver->priv->flags & GIMV_IMAGE_SAVER_SAVING_FLAG))
      return TRUE;
   else
      return FALSE;
}




GimvImageInfo *
gimv_image_saver_get_image_info (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), NULL);

   return saver->info;
}


const gchar *
gimv_image_saver_get_path (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), NULL);

   return saver->filename;
}


const gchar *
gimv_image_saver_get_format (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), NULL);

   return saver->format;
}


GimvImage *
gimv_image_saver_get_image (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), NULL);

   return saver->image;
}


gint
gimv_image_saver_get_n_comments (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), 0);

   return saver->n_comments;
}


gboolean
gimv_image_saver_get_comment (GimvImageSaver *saver,
                              gint idx,
                              const gchar **key,
                              const gchar **value)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), FALSE);

   if (idx < 0 || idx >= saver->n_comments) return FALSE;

   if (key)
      *key = saver->comments[idx].key;
   if (value)
      *value = saver->comments[idx].text;

   return TRUE;
}


gboolean
gimv_image_saver_progress_update (GimvImageSaver *saver)
{
   g_return_val_if_fail (GIMV_IS_IMAGE_SAVER (saver), FALSE);
   g_return_val_if_fail (gimv_image_saver_is_saving (saver), FALSE);
   g_return_val_if_fail (saver->priv, FALSE);

   gtk_signal_emit (GTK_OBJECT(saver),
                    gimv_image_saver_signals[PROGRESS_UPDATE_SIGNAL]);

   if (saver->priv->flags & GIMV_IMAGE_SAVER_CANCEL_FLAG)
      return FALSE;
   else
      return TRUE;
}
