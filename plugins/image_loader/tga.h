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

#ifndef __TGA_H__
#define __TGA_H__

#include <gtk/gtk.h>
#include "gimv_image.h"
#include "gimv_image_loader.h"

typedef struct tga_info_struct
{
   guint8 idLength;
   guint8 colorMapType;

   guint8 imageType;
   /* Known image types. */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3

   guint8 imageCompression;
   /* Only known compression is RLE */
#define TGA_COMP_NONE        0 
#define TGA_COMP_RLE         1 

   /* Color Map Specification. */
   /* We need to separately specify high and low bytes to avoid endianness
      and alignment problems. */

   guint16 colorMapIndex;
   guint16 colorMapLength;
   guint8 colorMapSize;

   /* Image Specification. */
   guint16 xOrigin;
   guint16 yOrigin;

   guint16 width;
   guint16 height;

   guint8 bpp;
   guint8 bytes;

   guint8 alphaBits;
   guint8 flipHoriz;
   guint8 flipVert;

   /* Extensions (version 2) */

/* Not all the structures described in the standard are transcribed here
   only those which seem applicable to Gimp */

   gchar authorName[41];
   gchar comment[324];
   guint month, day, year, hour, minute, second;
   gchar jobName[41];
   gchar softwareID[41];
   guint pixelWidth, pixelHeight;  /* write dpi? */
   gdouble gamma;
} tga_info;

GimvImage *tga_load       (GimvImageLoader *loader,
                           gpointer         data);

#endif /* __TGA_H__ */
