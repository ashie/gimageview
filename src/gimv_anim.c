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

#include "gimv_anim.h"

#ifdef HAVE_GDK_PIXBUF
#  include <gdk-pixbuf/gdk-pixbuf.h>
#elif defined (HAVE_GDK_IMLIB)
#  include <gdk_imlib.h>
#endif


static void gimv_anim_class_init    (GimvAnimClass *klass);
static void gimv_anim_init          (GimvAnim      *anim);
static void gimv_anim_destroy       (GtkObject     *object);


static GimvImageClass *parent_class = NULL;


GtkType
gimv_anim_get_type (void)
{
   static GtkType gimv_anim_type = 0;

   if (!gimv_anim_type) {
      static const GtkTypeInfo gimv_anim_info = {
         "GimvAnim",
         sizeof (GimvAnim),
         sizeof (GimvAnimClass),
         (GtkClassInitFunc) gimv_anim_class_init,
         (GtkObjectInitFunc) gimv_anim_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_anim_type = gtk_type_unique (gimv_image_get_type (),
                                        &gimv_anim_info);
   }

   return gimv_anim_type;
}


static void
gimv_anim_class_init (GimvAnimClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gimv_image_get_type ());

   object_class->destroy  = gimv_anim_destroy;
}


static void
gimv_anim_init (GimvAnim *anim)
{
   anim->anim = NULL;
   anim->current_frame_idx = -1;
   anim->table = NULL;
}


static void
gimv_anim_destroy (GtkObject *object)
{
   GimvAnim *anim;

   g_return_if_fail (GIMV_IS_ANIM (object));

   anim = GIMV_ANIM (object);

   g_return_if_fail (anim->table);
   g_return_if_fail (anim->table->delete);

   if (anim->anim && anim->table && anim->table->delete) {
      anim->table->delete (anim);
      anim->anim  = NULL;
   }
   anim->table = NULL;

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


GimvAnim *
gimv_anim_new (void)
{
   GimvAnim *anim = GIMV_ANIM (gtk_type_new (GIMV_TYPE_ANIM));
   return anim;
}


gint
gimv_anim_get_length (GimvAnim *anim)
{
   g_return_val_if_fail (anim, -1);
   g_return_val_if_fail (anim->table, -1);
   
   if (anim->table->get_length)
      return anim->table->get_length (anim);

   return -1;
}


gint
gimv_anim_iterate (GimvAnim *anim)
{
   g_return_val_if_fail (anim, -1);
   g_return_val_if_fail (anim->table, -1);
   g_return_val_if_fail (anim->table->iterate, -1);

   return anim->table->iterate (anim);
}


gboolean
gimv_anim_seek (GimvAnim *anim, gint idx)
{
   g_return_val_if_fail (anim, FALSE);
   g_return_val_if_fail (anim->table, FALSE);

   if (anim->table->seek)
      return anim->table->seek (anim, idx);

   return FALSE;
}


gint
gimv_anim_get_interval (GimvAnim *anim)
{
   g_return_val_if_fail (anim, -1);
   g_return_val_if_fail (anim->table, -1);
   g_return_val_if_fail (anim->table->get_interval, -1);

   return anim->table->get_interval (anim);
}


#if HAVE_GDK_PIXBUF
static void
free_rgb_buffer (guchar *pixels, gpointer data)
{
   g_free(pixels);
}
#endif /* HAVE_GDK_PIXBUF */

gboolean
gimv_anim_update_frame (GimvAnim *anim,
                        guchar   *frame,
                        gint      width,
                        gint      height,
                        gboolean  has_alpha)
{
   GimvImage *image = (GimvImage *) anim;

   g_return_val_if_fail (anim, FALSE);

#if HAVE_GDK_PIXBUF
   {
      gint bytes = 3;

      if (has_alpha)
         bytes = 4;

      if (image->image)
         gdk_pixbuf_unref (image->image);

      image->image = gdk_pixbuf_new_from_data (frame, GDK_COLORSPACE_RGB, FALSE, 8,
                                               width, height, bytes * width,
                                               free_rgb_buffer, NULL);
   }
#elif defined (HAVE_GDK_IMLIB)
   if (image->image)
      gdk_imlib_kill_image (image->image);

   image->image = gdk_imlib_create_image_from_data (frame, NULL, width, height);
   g_free (frame);
#endif /* HAVE_GDK_PIXBUF */

   if (image->image)
      return TRUE;
   else
      return FALSE;
}
