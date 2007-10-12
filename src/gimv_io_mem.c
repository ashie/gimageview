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

#include "gimv_io_mem.h"

#include <string.h>


static GimvIOStatus   gimv_io_mem_read  (GimvIO      *gio, 
                                         gchar       *buf, 
                                         guint        count,
                                         guint       *bytes_read);
static GimvIOStatus   gimv_io_mem_write (GimvIO      *gio, 
                                         const gchar *buf, 
                                         guint        count,
                                         guint       *bytes_written);
static GimvIOStatus   gimv_io_mem_seek  (GimvIO      *gio,
                                         glong        offset, 
                                         gint         whence);
static GimvIOStatus   gimv_io_mem_tell  (GimvIO      *gio,
                                         glong       *offset);
static void           gimv_io_mem_close (GimvIO      *gio);


GimvIOFuncs gimv_io_mem_funcs =
{
   gimv_io_mem_read,
   gimv_io_mem_write,
   gimv_io_mem_seek,
   gimv_io_mem_tell,
   gimv_io_mem_close,
};


void
gimv_io_mem_init (GimvIOMem *memio,
                  const gchar *url,
                  const gchar *mode,
                  GimvIOMemMode memio_mode)
{
   GimvIO *gio;
   gint i;

   g_return_if_fail (memio);

   gio  = (GimvIO *) memio;

   gimv_io_init (gio, url);
   gio->funcs = &gimv_io_mem_funcs;

   memio->memio_mode = memio_mode;
   switch (memio->memio_mode) {
   case GimvIOMemModeWrap:
      memio->free_buf = FALSE;
      memio->buf = NULL;
      break;
   case GimvIOMemModeStack:
   default:
      memio->buf = g_new0 (gchar *, GIMV_IO_MEM_BLOCKS_STEP);
      for (i = 0; i < GIMV_IO_MEM_BLOCKS_STEP; i++)
         ((gchar **)memio->buf)[i] = NULL;
      break;
   }
   memio->bufsize = 0;
   memio->pos     = 0;
}


GimvIO *
gimv_io_mem_new (const gchar *url,
                 const gchar *mode,
                 GimvIOMemMode memio_mode)
{
   GimvIOMem *memio;

   memio = g_new0 (GimvIOMem, 1);

   gimv_io_mem_init (memio, url, mode, memio_mode);

   return (GimvIO *) memio;
}


void
gimv_io_mem_stack (GimvIOMem *memio,
                   const gchar *buf, guint bufsize)
{
   gint i, src_blocks, dest_blocks, src_steps, dest_steps;
   guint bytes, src_size, dest_size, remain, offset;
   const gchar *src;

   g_return_if_fail (memio);
   g_return_if_fail (memio->memio_mode == GimvIOMemModeStack);
   g_return_if_fail (buf);

   src_size  = memio->bufsize;
   dest_size = memio->bufsize + bufsize;

   src_blocks  = src_size  / GIMV_IO_MEM_BLOCK_SIZE + 1;
   dest_blocks = dest_size / GIMV_IO_MEM_BLOCK_SIZE + 1;

   src_steps  = src_blocks  / GIMV_IO_MEM_BLOCKS_STEP + 1;
   dest_steps = dest_blocks / GIMV_IO_MEM_BLOCKS_STEP + 1;

   if (dest_steps > src_steps) {
      memio->buf
         = g_renew (gchar *, memio->buf,
                    GIMV_IO_MEM_BLOCKS_STEP * dest_steps);
      for (i = GIMV_IO_MEM_BLOCKS_STEP * src_steps; i < dest_blocks; i++) {
         ((gchar **) memio->buf)[i] = NULL;
      }
   }

   remain = bufsize;

   bytes = MIN (src_blocks * GIMV_IO_MEM_BLOCK_SIZE - src_size, remain);
   src = buf;
   if (!((gchar **) memio->buf)[src_blocks - 1])
      ((gchar **) memio->buf)[src_blocks - 1]
         = g_new0 (gchar, GIMV_IO_MEM_BLOCK_SIZE);
   offset = src_size - (src_blocks - 1) * GIMV_IO_MEM_BLOCK_SIZE;
   memcpy (((gchar **) memio->buf)[src_blocks - 1] + offset, src, bytes);
   remain -= bytes;

   for (i = src_blocks; i < dest_blocks; i++) {
      bytes = MIN (GIMV_IO_MEM_BLOCK_SIZE, remain);
      src = buf + (bufsize - remain);
      if (!((gchar **) memio->buf)[i])
         ((gchar **) memio->buf)[i] = g_new0 (gchar, GIMV_IO_MEM_BLOCK_SIZE);
      memcpy (((gchar **) memio->buf)[i], src, bytes);
      remain -= bytes;
   }

   memio->bufsize = dest_size;
}


void
gimv_io_mem_wrap (GimvIOMem *memio,
                  const gchar *buf, guint bufsize,
                  gboolean free)
{
   g_return_if_fail (memio);
   g_return_if_fail (memio->memio_mode == GimvIOMemModeWrap);
   g_return_if_fail (buf);

   if (memio->buf && memio->free_buf)
      g_free (memio->buf);

   memio->free_buf = free;
   memio->buf = (gpointer) buf;
   memio->bufsize = bufsize;
}


static GimvIOStatus
io_mem_read_stack_mode (GimvIOMem *memio, 
                        gchar     *buf, 
                        guint      count,
                        guint     *bytes_read)
{
   gchar *dest;
   guint idx, offset;
   guint bytes, remain;

   g_return_val_if_fail (memio->pos <= memio->bufsize, GIMV_IO_STATUS_ERROR);
   g_return_val_if_fail (bytes_read, GIMV_IO_STATUS_ERROR);

   remain = count;
   *bytes_read = 0;

   idx = memio->pos / GIMV_IO_MEM_BLOCK_SIZE;
   offset = memio->pos % GIMV_IO_MEM_BLOCK_SIZE;
   bytes = MIN (remain, GIMV_IO_MEM_BLOCK_SIZE - offset);
   dest = buf;

   memcpy (dest, ((gchar **) memio->buf)[idx] + offset, bytes);

   *bytes_read += bytes;
   memio->pos += bytes;
   remain -= bytes;
   dest += bytes;

   while (remain > 0) {
      idx++;

      bytes = MIN (remain, GIMV_IO_MEM_BLOCK_SIZE);

      memcpy (dest, ((gchar **) memio->buf)[idx], bytes);

      *bytes_read += bytes;
      memio->pos += bytes;
      remain -= bytes;
      dest += bytes;
   }

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_mem_read (GimvIO *gio, 
                  gchar  *buf, 
                  guint   count,
                  guint  *bytes_read)
{
   GimvIOMem *memio = (GimvIOMem *) gio;

   *bytes_read = 0;

   if (memio->pos == memio->bufsize) {
      return GIMV_IO_STATUS_NORMAL;
   } else if (memio->pos > memio->bufsize) {
      memio->pos = memio->bufsize;
      return GIMV_IO_STATUS_ERROR;
   }

   switch (memio->memio_mode) {
   case GimvIOMemModeWrap:
      *bytes_read = MIN (count, ((guint) memio->bufsize - memio->pos));
      memcpy (buf, ((char *) memio->buf) + memio->pos, *bytes_read);
      memio->pos += *bytes_read;
      break;
   case GimvIOMemModeStack:
   default:
      io_mem_read_stack_mode (memio, buf, count, bytes_read);
      break;
   }

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_mem_write (GimvIO      *gio, 
                   const gchar *buf, 
                   guint        count,
                   guint       *bytes_written)

{
   /* GimvIOMem *memio = (GimvIOMem *) gio; */

   /* FIXME */

   return GIMV_IO_STATUS_ERROR;
}


static GimvIOStatus
gimv_io_mem_seek  (GimvIO *gio,
                   glong   offset, 
                   gint    whence)
{
   GimvIOMem *memio = (GimvIOMem *) gio;
   glong pos;

   switch (whence) {
   case SEEK_CUR:
      pos = memio->pos + offset;
      break;
   case SEEK_SET:
      pos = offset;
      break;
   case SEEK_END:
      pos = memio->bufsize + offset;
      break;
   default:
      return GIMV_IO_STATUS_ERROR;
      break;
   }

   if (pos > memio->bufsize) {
      memio->pos = memio->bufsize;
      return GIMV_IO_STATUS_ERROR;
   } else if (pos < 0) {
      memio->pos = 0;
      return GIMV_IO_STATUS_ERROR;
   }

   memio->pos = pos;

   return GIMV_IO_STATUS_NORMAL;
}


static GimvIOStatus
gimv_io_mem_tell  (GimvIO *gio,
                   glong  *offset)
{
   GimvIOMem *memio = (GimvIOMem *) gio;

   g_return_val_if_fail (offset, GIMV_IO_STATUS_ERROR);

   *offset = memio->pos;

   return GIMV_IO_STATUS_NORMAL;
}


static void
gimv_io_mem_close  (GimvIO *gio)
{
   GimvIOMem *memio = (GimvIOMem *) gio;
   gint block_num, i;

   switch (memio->memio_mode) {
   case GimvIOMemModeWrap:
      if (memio->free_buf)
         g_free (memio->buf);
      memio->buf = NULL;
      break;
   case GimvIOMemModeStack:
   default:
      block_num = memio->bufsize / GIMV_IO_MEM_BLOCK_SIZE + 1;
      for (i = 0; i < block_num; i++) {
         g_free (((gchar **)memio->buf)[i]);
      }
      g_free (memio->buf);
      break;
   }
}
