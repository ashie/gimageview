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

#ifndef __GIMAGEVIEW_H__
#define __GIMAGEVIEW_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* global constants */
#define GIMV_PROG_NAME     "GImageView"
#define GIMV_PROG_VERSION  "GImageView Version "VERSION
#define GIMV_PROG_AUTHOR   "Takuro Ashie"
#define GIMV_PROG_ADDRESS  "ashie@homa.ne.jp"
#define GIMV_PROG_URI      "http://gtkmmviewer.sourceforge.net/"
#define GIMV_RC_DIR        ".gimv"
#define GIMV_RC            "gimvrc"
#define GIMV_GTK_RC        "gtkrc"
#define GIMV_KEYCONF_RC    "keyconf"
#define GIMV_KEYACCEL_RC   "keyaccelrc"
#define BUF_SIZE      4096
#define MAX_PATH_LEN  1024

/* type defs */
typedef struct FilesLoader_Tag     FilesLoader;
typedef struct GimvImageInfo_Tag   GimvImageInfo;
typedef struct GimvImageView_Tag   GimvImageView;
typedef struct GimvImageWin_Tag    GimvImageWin;
typedef struct GimvThumb_Tag       GimvThumb;
typedef struct GimvThumbView_Tag   GimvThumbView;
typedef struct GimvThumbWin_Tag    GimvThumbWin;
typedef struct DirView_Tag         DirView;
typedef struct GimvComment_Tag     GimvComment;
typedef struct GimvCommentView_Tag GimvCommentView;
typedef struct GimvSlideShow_Tag   GimvSlideShow;

/* will be replaced to GimvVFS */
typedef struct _FRArchive       FRArchive;

typedef enum
{
   GIMV_COM_UNKNOWN,
   GIMV_COM_DIR_VIEW,
   GIMV_COM_THUMB_VIEW,
   GIMV_COM_IMAGE_VIEW,
   GIMV_COM_THUMB_WIN,
   GIMV_COM_IMAGE_WIN,
   GIMV_COM_LAST
} GimvComponentType;

void         gimv_charset_init         (void);
gchar       *gimv_filename_to_locale   (const gchar *filename);
gchar       *gimv_filename_to_internal (const gchar *filename);
void         gimv_quit                 (void);

#endif /* __GIMAGEVIEW_H__ */
