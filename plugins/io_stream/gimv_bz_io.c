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

#include "gimv_bz_io.h"

#ifdef HAVE_BZLIB

#include <stdio.h>
#include <string.h>
#include <bzlib.h>

#include "gimv_io_mem.h"
#include "gimv_plugin.h"
#include "utils_file.h"


#ifndef BUF_SIZE
#  define BUF_SIZE 4096
#endif /* BUF_SIZE */


static GimvIOPlugin plugin_impl[] = {
   {
      if_version:    GIMV_IO_IF_VERSION,
      id:            "BZ",
      priority_hint: GIMV_IO_PLUGIN_PRIORITY_DEFAULT,
      new_func:      gimv_bz_io_new,
   }
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_IO_STREAMER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("BZIP2 Compressed Stream Support"),
   version:       "0.1.0",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


typedef struct GimvBzIO_Tag GimvBzIO;

struct GimvBzIO_Tag
{
   GimvIOMem  parent;
   BZFILE    *file;
};


#if 0
GimvIOFuncs gimv_bz_io_funcs =
{
   gimv_bz_io_read,
   gimv_bz_io_write,
   gimv_bz_io_seek,
   gimv_bz_io_tell,
   gimv_bz_io_close,
};
#endif


GimvIO *
gimv_bz_io_new (const gchar *url, const gchar *mode)
{
   GimvBzIO *bio;
   GimvIO *gio;
   GimvIOMem *memio;
   const gchar *filename = url;
   guint bytes;
   gchar buf[BUF_SIZE];

   g_return_val_if_fail (url, NULL);

   if (!fileutil_extension_is (url, "bz2"))
      return NULL;

   bio = g_new0 (GimvBzIO, 1);
   gio   = (GimvIO *) bio;
   memio = (GimvIOMem *) bio;

   gimv_io_mem_init (memio, url, mode, GimvIOMemModeStack);
   /* gio->funcs = &gimv_bz_io_funcs; */

   bio->file = BZOPEN (filename, (char *) mode);
   if (!bio->file) {
      gimv_io_close (gio);
      return NULL;
   }

   while (1) {
      bytes = BZREAD (bio->file, buf, BUF_SIZE);
      if (bytes > 0) {
         gimv_io_mem_stack (memio, buf, bytes);
      } else {
         break;
      }
   }

   if (bio->file)
      BZCLOSE (bio->file);
   bio->file = NULL;

   return gio;
}

#endif /* HAVE_BZLIB */
