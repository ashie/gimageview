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

#ifndef __GIMV_EXIF_VIEW_H__
#define __GIMV_EXIF_VIEW_H__

#include "gimageview.h"

#ifdef ENABLE_EXIF

#include <libexif/exif-data.h>
#include <libexif/jpeg-data.h>   /* FIXME!! */

typedef struct ExifView_Tag
{
   GtkWidget *window;     /* if open in stand alone window, use this */
   GtkWidget *container;  /* exif view */
   ExifData  *exif_data;
   JPEGData  *jpeg_data;
} ExifView;


ExifView *exif_view_create_window (const gchar *filename,
                                   GtkWindow   *parent);
ExifView *exif_view_create        (const gchar *filename,
                                   GtkWindow   *parent);

#endif /* ENABLE_EXIF */

#endif /* __GIMV_EXIF_VIEW_H__ */
