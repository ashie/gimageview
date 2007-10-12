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
#include <zlib.h>

#include "gimv_io.h"
#include "gimv_gzip_io.h"
#include "fileutil.h"
#include "gimv_plugin.h"


static GimvIOStatus   gimv_gzip_io_read  (GimvIO      *gio, 
                                          gchar       *buf, 
                                          guint        count,
                                          guint       *bytes_read);
static GimvIOStatus   gimv_gzip_io_write (GimvIO      *gio, 
                                          const gchar *buf, 
                                          guint        count,
                                          guint       *bytes_written);
static GimvIOStatus   gimv_gzip_io_seek  (GimvIO      *gio,
                                          glong        offset, 
                                          gint         whence);
static GimvIOStatus   gimv_gzip_io_tell  (GimvIO      *gio,
                                          glong       *offset);
static void           gimv_gzip_io_close (GimvIO      *gio);


static GimvIOPlugin plugin_impl[] = {
   {
      if_version:    GIMV_IO_IF_VERSION,
      id:            "GZIP",
      priority_hint: GIMV_IO_PLUGIN_PRIORITY_DEFAULT,
      new_func:      gimv_gzip_io_new,
   }
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_IO_STREAMER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("GZIP Compressed Stream Support"),
   version:       "0.1.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


typedef struct GimvGZipIO_Tag GimvGZipIO;

struct GimvGZipIO_Tag
{
   GimvIO parent;
   gzFile file;
};

GimvIOFuncs gimv_gzip_io_funcs =
{
   gimv_gzip_io_read,
   gimv_gzip_io_write,
   gimv_gzip_io_seek,
   gimv_gzip_io_tell,
   gimv_gzip_io_close,
};


GimvIO *
gimv_gzip_io_new (const gchar *url, const gchar *mode)
{
   GimvGZipIO *zio;
   GimvIO *gio;
   const gchar *filename = url;

   g_return_val_if_fail (url, NULL);

   if (!fileutil_extension_is (url, "gz"))
      return NULL;

   zio = g_new0 (GimvGZipIO, 1);
   gio  = (GimvIO *) zio;

   gimv_io_init (gio, url);
   gio->funcs = &gimv_gzip_io_funcs;

   zio->file = gzopen (filename, (char *) mode);
   if (zio->file < 0) {
      gimv_io_close (gio);
      return NULL;
   }

   return gio;
}


static GimvIOStatus
gimv_gzip_io_read (GimvIO *gio, 
                   gchar  *buf, 
                   guint   count,
                   guint  *bytes_read)
{
   GimvGZipIO *zio = (GimvGZipIO *) gio;

   *bytes_read = gzread (zio->file, buf, count);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_gzip_io_write (GimvIO      *gio, 
                    const gchar *buf, 
                    guint        count,
                    guint       *bytes_written)
{
   GimvGZipIO *zio = (GimvGZipIO *) gio;

   *bytes_written = gzwrite (zio->file, (const voidp) buf, count);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_gzip_io_seek  (GimvIO *gio,
                    glong   offset, 
                    gint    whence)
{
   GimvGZipIO *zio = (GimvGZipIO *) gio;

   gzseek (zio->file, offset, whence);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_gzip_io_tell  (GimvIO *gio,
                    glong  *offset)
{
   GimvGZipIO *zio = (GimvGZipIO *) gio;

   *offset = gztell (zio->file);

   return GIMV_IO_STATUS_NORMAL;
}


static void
gimv_gzip_io_close  (GimvIO *gio)
{
   GimvGZipIO *zio = (GimvGZipIO *) gio;

   if (zio->file)
      gzclose (zio->file);
}
