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

#ifndef __GIMV_IO_MEM_H__
#define __GIMV_IO_MEM_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gimv_io.h>


#ifndef GIMV_IO_MEM_BUF_SIZE
#  define GIMV_IO_MEM_BUF_SIZE  4096
#endif

#define GIMV_IO_MEM_BLOCK_SIZE  65536   /* 64kB/1block */
#define GIMV_IO_MEM_BLOCKS_STEP 64


typedef struct GimvIOMem_Tag GimvIOMem;

typedef enum {
   GimvIOMemModeStack,
   GimvIOMemModeWrap
} GimvIOMemMode;


struct GimvIOMem_Tag
{
   GimvIO          parent;

   GimvIOMemMode   memio_mode;
   gboolean        free_buf;
   gpointer        buf;
   glong           bufsize;
   glong           pos;
};


extern GimvIOFuncs gimv_io_mem_funcs;


void       gimv_io_mem_init  (GimvIOMem     *memio,
                              const gchar   *url,
                              const gchar   *mode,
                              GimvIOMemMode  memio_mode);
GimvIO    *gimv_io_mem_new   (const gchar   *url,
                              const gchar   *mode,
                              GimvIOMemMode  memio_mode);
void       gimv_io_mem_stack (GimvIOMem     *memio,
                              const gchar   *buf,
                              guint          buf_size);
void       gimv_io_mem_wrap  (GimvIOMem     *memio,
                              const gchar   *buf,
                              guint          bufsize,
                              gboolean       free);

#endif /* __GIMV_IO_MEM_H__ */
