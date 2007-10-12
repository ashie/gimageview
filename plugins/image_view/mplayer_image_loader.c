/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2004 Takuro Ashie
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

#include "mplayer_image_loader.h"

#ifdef ENABLE_MPLAYER

#include "gimv_mplayer.h"
#include "gimv_plugin.h"
#include "prefs_mplayer.h"

static GimvImageLoaderPlugin gimv_mplayer_image_loader =
{
   if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
   id:            "MPLAYER",
   priority_hint: GIMV_IMAGE_LOADER_PRIORITY_HIGH,
   check_type:    NULL,
   get_info:      NULL,
   loader:        gimv_mplayer_image_loader_load_file,
};


void
gimv_mplayer_image_loader_get_impl (gpointer *impl, guint *size)
{
   *impl = &gimv_mplayer_image_loader;
   *size = sizeof(gimv_mplayer_image_loader);
}


GimvImage *
gimv_mplayer_image_loader_load_file (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GimvImage *image = NULL;
   GimvMPlayer *mplayer;
   gfloat pos = 0.0, len;
   gchar *tmpfile;

   /* FIXME */
   if (!gimv_prefs_mplayer_get_thumb_enable()) return NULL;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

#warning FIXME
   if (!gimv_image_info_is_movie (loader->info)
       && !gimv_mime_types_extension_is (filename, "rm"))
   {
      return NULL;
   }

   mplayer = GIMV_MPLAYER (gimv_mplayer_new ());
#ifdef USE_GTK2
   g_object_ref (G_OBJECT (mplayer));
   gtk_object_sink (GTK_OBJECT (mplayer));
#endif /* USE_GTK2 */
   if (!gimv_mplayer_set_file (mplayer, filename)) goto ERROR0;

   len = gimv_mplayer_get_length (mplayer);
   if (len > 0.01)
      pos = len * gimv_prefs_mplayer_get_thumb_pos() / 100.0;

   tmpfile = gimv_mplayer_get_frame (mplayer, NULL, NULL,
                                     pos, gimv_prefs_mplayer_get_thumb_frames(),
                                     TRUE);
   if (!tmpfile) goto ERROR0;

   image = gimv_image_load_file (tmpfile, FALSE);

   g_free (tmpfile);
 ERROR0:
   gtk_widget_unref (GTK_WIDGET (mplayer));

   return image;
}

#endif /* ENABLE_MPLAYER */
