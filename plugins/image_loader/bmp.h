/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GTK See -- a image viewer based on GTK+
 * Copyright (C) 1998 Hotaru Lee <jkhotaru@mail.sti.com.cn> <hotaru@163.net>
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

#ifndef __BMP_H__
#define __BMP_H__

#include <gtk/gtk.h>
#include "gimv_image.h"
#include "gimv_image_loader.h"

typedef struct bmp_info_struct
{
   gulong sizeHeader;
   gulong width;		/* 12 */
   gulong height;		/* 16 */
   guint planes;		/* 1A */  /* Planes */
   guint bitCnt;		/* 1C */  /* Bits Per Pixels */
   gulong compr;		/* 1E */  /* Compression */
   gulong sizeIm;		/* 22 */  /* Bitmap Data Size */
   gulong xPels;		/* 26 */  /* HResolution */
   gulong yPels;		/* 2A */  /* VResolution */
   gulong clrUsed;		/* 2E */  /* Colors */
   gulong clrImp;		/* 32 */  /* Important Colors */
   /* 36 */
} bmp_info;

gboolean   bmp_get_header (GimvIO          *gio,
                           bmp_info        *info);
GimvImage *bmp_load       (GimvImageLoader *loader,
                           gpointer         data);

#endif /* BMP_H */

