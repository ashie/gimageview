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

#ifndef __GIMV_IO__
#define __GIMV_IO__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>


typedef struct GimvIO_Tag GimvIO;
typedef struct GimvIOFuncs_Tag GimvIOFuncs;
typedef struct GimvIOPlugin_Tag GimvIOPlugin;


typedef enum
{
   GIMV_IO_STATUS_NORMAL,
   GIMV_IO_STATUS_ERROR,
   GIMV_IO_STATUS_EOF,
   GIMV_IO_STATUS_AGAIN
} GimvIOStatus;


struct GimvIO_Tag
{
   GimvIOFuncs *funcs;
   gchar       *url;
   gint         ref_count;
};


struct GimvIOFuncs_Tag
{
   GimvIOStatus (*read)   (GimvIO         *gio, 
                           gchar          *buf, 
                           guint           count,
                           guint          *bytes_read);
   GimvIOStatus (*write)  (GimvIO         *gio, 
                           const gchar    *buf, 
                           guint           count,
                           guint          *bytes_written);
   GimvIOStatus (*seek)   (GimvIO         *gio, 
                           glong           offset, 
                           gint            whence);
   GimvIOStatus (*tell)   (GimvIO         *gio, 
                           glong          *offset);
   void         (*close)  (GimvIO         *gio);
};


#define GIMV_IO_IF_VERSION 1

typedef GimvIO *(*GimvIONewFn) (const gchar *url,
                                const gchar *mode);

#define GIMV_IO_PLUGIN_PRIORITY_DEFAULT 0


struct GimvIOPlugin_Tag
{
   const guint32       if_version; /* plugin interface version */
   const gchar * const id;
   gint                priority_hint;
   GimvIONewFn         new_func;
};


void           gimv_io_init  (GimvIO      *gio,
                              const gchar *url);
GimvIO        *gimv_io_new   (const gchar *url,
                              const gchar *mode);
GimvIO        *gimv_io_ref   (GimvIO      *gio);
void           gimv_io_unref (GimvIO      *gio);
GimvIOStatus   gimv_io_read  (GimvIO      *gio, 
                              gchar       *buf, 
                              guint        count,
                              guint       *bytes_read);
GimvIOStatus   gimv_io_write (GimvIO      *gio, 
                              const gchar *buf, 
                              guint        count,
                              guint       *bytes_written);
GimvIOStatus   gimv_io_seek  (GimvIO      *gio,
                              glong        offset, 
                              gint         whence);
GimvIOStatus   gimv_io_tell  (GimvIO      *gio,
                              glong       *offset);
void           gimv_io_close (GimvIO      *gio);


gint           gimv_io_getc  (GimvIO       *gio,
                              GimvIOStatus *status);
GimvIOStatus   gimv_io_fgets (GimvIO       *gio,
                              gchar        *buf,
                              guint         count);

/* for plugin loader */
gboolean       gimv_io_plugin_regist (const gchar *plugin_name,
                                      const gchar *module_name,
                                      gpointer     impl,
                                      gint         size);

#endif /* __GIMV_IO__ */
