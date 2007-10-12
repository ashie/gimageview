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

#ifndef __GIMV_IMAGE_SAVER_H__
#define __GIMV_IMAGE_SAVER_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtkobject.h>

#include "gimv_image.h"
#include "gimv_image_info.h"


#define GIMV_TYPE_IMAGE_SAVER            (gimv_image_saver_get_type ())
#define GIMV_IMAGE_SAVER(obj)            (GTK_CHECK_CAST (obj, gimv_image_saver_get_type (), GimvImageSaver))
#define GIMV_IMAGE_SAVER_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_image_saver_get_type, GimvImageSaverClass))
#define GIMV_IS_IMAGE_SAVER(obj)         (GTK_CHECK_TYPE (obj, gimv_image_saver_get_type ()))
#define GIMV_IS_IMAGE_SAVER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_IMAGE_SAVER))


typedef struct GimvImageSaver_Tag      GimvImageSaver;
typedef struct GimvImageSaverPriv_Tag  GimvImageSaverPriv;
typedef struct GimvImageSaverClass_Tag GimvImageSaverClass;
typedef struct GimvImageSaverText_Tag  GimvImageSaverText;


struct GimvImageSaver_Tag
{
   GtkObject parent;

   GimvImageInfo       *info;
   GimvImage           *image;
   gchar               *format;
   gchar               *filename;

   GimvImageSaverText  *comments;
   gint                 n_comments;

   gpointer             param;

   GTimer              *timer;

   GimvImageSaverPriv  *priv;
};


struct GimvImageSaverClass_Tag
{
   GtkObjectClass parent;

   void (*save_start)        (GimvImageSaver *saver);
   void (*progress_update)   (GimvImageSaver *saver);
   void (*save_end)          (GimvImageSaver *saver);
};


struct GimvImageSaverText_Tag
{
   gchar *key;
   gchar *text;
};


#define GIMV_IMAGE_SAVER_IF_VERSION 2


typedef struct GimvImageSaverPlugin_Tag
{
   const guint32  if_version; /* plugin interface version */
   gchar         *format;

   gboolean      (*save)             (GimvImageSaver *saver,
                                      GimvImage      *image,
                                      const gchar    *filename,
                                      const gchar    *format);
   GtkWidget    *(*save_with_dialog) (GimvImageSaver *saver,
                                      GimvImage      *image,
                                      const gchar    *filename,
                                      const gchar    *format);
} GimvImageSaverPlugin;


GtkType         gimv_image_saver_get_type            (void);
GimvImageSaver *gimv_image_saver_new                 (void);
GimvImageSaver *gimv_image_saver_new_with_attr       (GimvImage      *image,
                                                      const gchar    *path,
                                                      const gchar    *format);

GimvImageSaver *gimv_image_saver_ref                 (GimvImageSaver *saver);
void            gimv_image_saver_unref               (GimvImageSaver *saver);

void            gimv_image_saver_reset               (GimvImageSaver *saver);

void            gimv_image_saver_set_image           (GimvImageSaver *saver,
                                                      GimvImage      *image);
void            gimv_image_saver_set_path            (GimvImageSaver *saver,
                                                      const gchar    *path);
void            gimv_image_saver_set_format          (GimvImageSaver *saver,
                                                      const gchar    *format);
void            gimv_image_saver_set_image_info      (GimvImageSaver *saver,
                                                      GimvImageInfo  *info);
void            gimv_image_saver_set_param           (GimvImageSaver *saver,
                                                      gpointer        param);
void            gimv_image_saver_set_comment         (GimvImageSaver *saver,
                                                      const gchar    *key,
                                                      const gchar    *value);

gboolean        gimv_image_saver_save                (GimvImageSaver *saver);
void            gimv_image_saver_save_start          (GimvImageSaver *saver);
void            gimv_image_saver_save_stop           (GimvImageSaver *saver);
gboolean        gimv_image_saver_is_saving           (GimvImageSaver *saver);


/* used by saver */
GimvImage      *gimv_image_saver_get_image           (GimvImageSaver *saver);
const gchar    *gimv_image_saver_get_path            (GimvImageSaver *saver);
const gchar    *gimv_image_saver_get_format          (GimvImageSaver *saver);
GimvImageInfo  *gimv_image_saver_get_image_info      (GimvImageSaver *saver);
gint            gimv_image_saver_get_n_comments      (GimvImageSaver *saver);
gboolean        gimv_image_saver_get_comment         (GimvImageSaver *saver,
                                                      gint            idx,
                                                      const gchar **  key,
                                                      const gchar **  value);
gboolean        gimv_image_saver_progress_update     (GimvImageSaver *saver);

/*
 *  for plugin loader
 */
gboolean        gimv_image_saver_plugin_regist       (const gchar *plugin_name,
                                                      const gchar *module_name,
                                                      gpointer     impl,
                                                      gint         size);
#endif /* __GIMV_IMAGE_SAVER_H__ */
