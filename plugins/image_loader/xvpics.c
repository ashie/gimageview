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

#include <stdio.h>
#include <string.h>

#include "xvpics.h"
#include "gimv_plugin.h"


static GimvImageLoaderPlugin gimv_xvpics_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "XVPICS",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        xvpics_load,
   }
};

GIMV_PLUGIN_GET_IMPL(gimv_xvpics_loader, GIMV_PLUGIN_IMAGE_LOADER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("XV thumbnail Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


GimvImage *
xvpics_load (GimvImageLoader *loader, gpointer user_data)
{
   GimvIO *gio;
   GimvImage *image;
   gchar buffer[BUF_SIZE];
   guchar *line, *rgb_data;
   gint width, height, depth;
   gint i, j;
   guint bytes_read;
   gint bytes_count = 0, bytes_count_tmp = 0, step = 65536;
   glong pos;

   gint orig_width, orig_height, orig_size;
   gchar orig_color[16];
   gboolean has_orig_info = FALSE;

   g_return_val_if_fail (loader, NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   gimv_io_fgets (gio, buffer, BUF_SIZE);
   if (strncmp (buffer, "P7 332", 6) != 0) return NULL;

   while (gimv_io_fgets (gio, buffer, BUF_SIZE) == GIMV_IO_STATUS_NORMAL && buffer[0] == '#') {
      gint ret;

      ret = sscanf(buffer, "#IMGINFO:%dx%d %4s (%d bytes)",
                   &orig_width, &orig_height, orig_color, &orig_size);
      orig_color[sizeof(orig_color) / sizeof(gchar) - 1] = '\0';
      if (ret == 4) {
         has_orig_info = TRUE;
      }
   }

   if (sscanf(buffer, "%d %d %d", &width, &height, &depth) != 3) return NULL;
   if (!gimv_image_loader_progress_update (loader)) return NULL;

   line = g_new0 (guchar, width * height);
   rgb_data = g_new(guchar, width * height * 3);

   for(j = 0; j < height; j++) {
      gimv_io_read (gio, line, width, &bytes_read);

      for (i = 0; i < width; i++) {
         rgb_data[(i + j * width) * 3 + 0] = (line[i] >> 5) * 36;
         rgb_data[(i + j * width) * 3 + 1] = ((line[i] & 28) >> 2) * 36;
         rgb_data[(i + j * width) * 3 + 2] = (line[i] & 3) * 85;
      }

      if (bytes_read < (guint) width) break;

      gimv_io_tell (gio, &pos);
      bytes_count_tmp = pos / step;
      if (bytes_count_tmp > bytes_count) {
         bytes_count = bytes_count_tmp;
         if (!gimv_image_loader_progress_update (loader)) goto ERROR;
      }
   }

   g_free (line);

   image = gimv_image_create_from_data (rgb_data, width, height, FALSE);

   if (has_orig_info) {
      gchar val[32];
      gint val_len = sizeof (val) / sizeof (gchar);

      g_snprintf (val, val_len, "%d", orig_width);
      gimv_image_add_comment (image, "OriginalWidth",      val);

      g_snprintf (val, val_len, "%d", orig_height);
      gimv_image_add_comment (image, "OriginalHeight",     val);

      gimv_image_add_comment (image, "OriginalColorSpace", orig_color);

      g_snprintf (val, val_len, "%d", orig_size);
      gimv_image_add_comment (image, "OriginalSize",       val);
   }

   return image;

ERROR:
   g_free (line);
   g_free (rgb_data);
   return NULL;
}
