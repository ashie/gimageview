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

#include "gimv_io.h"
#include "gimv_io_file.h"


static GimvIOStatus   gimv_io_file_read  (GimvIO      *gio, 
                                          gchar       *buf, 
                                          guint        count,
                                          guint       *bytes_read);
static GimvIOStatus   gimv_io_file_write (GimvIO      *gio, 
                                          const gchar *buf, 
                                          guint        count,
                                          guint       *bytes_written);
static GimvIOStatus   gimv_io_file_seek  (GimvIO      *gio,
                                          glong        offset, 
                                          gint         whence);
static GimvIOStatus   gimv_io_file_tell  (GimvIO      *gio,
                                          glong       *offset);
static void           gimv_io_file_close (GimvIO      *gio);


typedef struct GimvIOFile_Tag GimvIOFile;

struct GimvIOFile_Tag
{
   GimvIO parent;
   FILE *fd;
};

GimvIOFuncs gimv_io_file_funcs =
{
   gimv_io_file_read,
   gimv_io_file_write,
   gimv_io_file_seek,
   gimv_io_file_tell,
   gimv_io_file_close,
};


GimvIO *
gimv_io_file_new (const gchar *url, const gchar *mode)
{
   GimvIOFile *fio;
   GimvIO *gio;
   const gchar *filename = url;

   g_return_val_if_fail (url, NULL);

   fio = g_new0 (GimvIOFile, 1);
   gio  = (GimvIO *) fio;

   gimv_io_init (gio, url);
   gio->funcs = &gimv_io_file_funcs;

   fio->fd = fopen (filename, (char *) mode);
   if (!fio->fd) {
      gimv_io_close (gio);
      return NULL;
   }

   return gio;
}


static GimvIOStatus
gimv_io_file_read (GimvIO *gio, 
                   gchar  *buf, 
                   guint   count,
                   guint  *bytes_read)
{
   GimvIOFile *fio = (GimvIOFile *) gio;

   *bytes_read = fread (buf, 1, count, fio->fd);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_file_write (GimvIO      *gio, 
                    const gchar *buf, 
                    guint        count,
                    guint       *bytes_written)
{
   GimvIOFile *fio = (GimvIOFile *) gio;

   *bytes_written = fwrite (buf, 1, count, fio->fd);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_file_seek  (GimvIO *gio,
                    glong   offset, 
                    gint    whence)
{
   GimvIOFile *fio = (GimvIOFile *) gio;

   fseek (fio->fd, offset, whence);

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_file_tell  (GimvIO *gio,
                    glong  *offset)
{
   GimvIOFile *fio = (GimvIOFile *) gio;

   *offset = ftell (fio->fd);

   return GIMV_IO_STATUS_NORMAL;
}


static void
gimv_io_file_close  (GimvIO *gio)
{
   GimvIOFile *fio = (GimvIOFile *) gio;

   if (fio->fd)
      fclose (fio->fd);
}
