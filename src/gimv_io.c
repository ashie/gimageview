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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gimv_io.h"
#include "gimv_io_file.h"


/****************************************************************************
 *
 *  Plugin Management
 *
 ****************************************************************************/
static GList *gimv_io_list = NULL;


static gint
comp_func_priority (GimvIOPlugin *plugin1,
                    GimvIOPlugin *plugin2)
{
   g_return_val_if_fail (plugin1, 1);
   g_return_val_if_fail (plugin2, -1);

   return plugin1->priority_hint - plugin2->priority_hint;
}


gboolean
gimv_io_plugin_regist (const gchar *plugin_name,
                       const gchar *module_name,
                       gpointer impl,
                       gint     size)
{
   GimvIOPlugin *streamer = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (streamer, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (streamer->id, FALSE);
   g_return_val_if_fail (streamer->if_version == GIMV_IO_IF_VERSION, FALSE);
   g_return_val_if_fail (streamer->new_func, FALSE);

   gimv_io_list = g_list_append (gimv_io_list, streamer);
   gimv_io_list = g_list_sort (gimv_io_list,
                               (GCompareFunc) comp_func_priority);

   return TRUE;
}


/****************************************************************************
 *
 *
 *
 ****************************************************************************/
void
gimv_io_init (GimvIO *gio, const gchar *url)
{
   g_return_if_fail (gio);
   /* g_return_if_fail (url); */

   gio->funcs = NULL;
   gio->url = g_strdup (url);
   gio->ref_count = 1;
}


GimvIO *
gimv_io_new (const gchar *url, const gchar *mode)
{
   GList *node;

   g_return_val_if_fail (url, NULL);

   for (node = gimv_io_list; node; node = g_list_next (node)) {
      GimvIOPlugin *plugin = node->data;
      GimvIO *gio;

      gio = plugin->new_func (url, mode);
      if (gio) return gio;
   }

   return gimv_io_file_new (url, mode);
}


GimvIO *
gimv_io_ref (GimvIO *gio)
{
   g_return_val_if_fail (gio, NULL);

   gio->ref_count++;

   return gio;
}


void
gimv_io_unref (GimvIO *gio)
{
   g_return_if_fail (gio);

   gio->ref_count--;

   if (gio->ref_count < 1)
      gimv_io_close (gio);
}


GimvIOStatus
gimv_io_read (GimvIO *gio, 
              gchar  *buf, 
              guint   count,
              guint  *bytes_read)
{
   g_return_val_if_fail (gio, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (bytes_read, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs->read, GIMV_IO_STATUS_ERROR);

   return gio->funcs->read (gio, buf, count, bytes_read);
}


GimvIOStatus
gimv_io_write (GimvIO      *gio, 
               const gchar *buf, 
               guint        count,
               guint       *bytes_written)
{
   g_return_val_if_fail (gio, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (bytes_written, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs->write, GIMV_IO_STATUS_ERROR);

   return gio->funcs->write (gio, buf, count, bytes_written);
}


GimvIOStatus
gimv_io_seek  (GimvIO    *gio,
               glong      offset, 
               gint       whence)
{
   g_return_val_if_fail (gio, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs->seek, GIMV_IO_STATUS_ERROR);

   switch (whence) {
   case SEEK_CUR:
   case SEEK_SET:
   case SEEK_END:
      break;
   default:
      return GIMV_IO_STATUS_ERROR;
   }

   return gio->funcs->seek (gio, offset, whence);
}


GimvIOStatus
gimv_io_tell  (GimvIO    *gio,
               glong     *offset)
{
   g_return_val_if_fail (gio, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (offset, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (gio->funcs->tell, GIMV_IO_STATUS_ERROR);

   return gio->funcs->tell (gio, offset);
}


void
gimv_io_close (GimvIO *gio)
{
   g_return_if_fail (gio);
   g_return_if_fail (gio->funcs);
   g_return_if_fail (gio->funcs->close);

   gio->funcs->close (gio);

   g_free (gio->url);
   g_free (gio);
}


gint
gimv_io_getc (GimvIO *gio, GimvIOStatus *status)
{
   guint bytes_read;
   guchar c;

   gimv_io_read (gio, &c, 1, &bytes_read);

   if (bytes_read < 1) {
      if (status)
         *status = GIMV_IO_STATUS_ERROR;
      return EOF;
   } else {
      if (status)
         *status = GIMV_IO_STATUS_NORMAL;
      return (gint) c;
   }
}


GimvIOStatus
gimv_io_fgets (GimvIO *gio, gchar *buf, guint count)
{
   guint i;
   guint bytes_read;
   guint len = 0;

   g_return_val_if_fail (gio, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (buf, GIMV_IO_STATUS_ERROR);

   for (i = 0; i < count - 2; i++) {
      gimv_io_read (gio, &buf[i], 1, &bytes_read);
      if (bytes_read < 1) {
         buf[i] = '\0';
         return GIMV_IO_STATUS_ERROR;
      }

      len = i + 1;

      if (buf[i] == '\n') break;
   }

   buf[len] = '\0';

   return GIMV_IO_STATUS_NORMAL;
}
