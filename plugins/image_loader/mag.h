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

#ifndef __MAG_H__
#define __MAG_H__

#include <gtk/gtk.h>
#include "gimv_image.h"
#include "gimv_image_loader.h"

typedef struct mag_info_tag {
   guchar mcode;
   guchar dcode;
   guchar screen;
   guint lx;
   guint uy;
   guint rx;
   guint dy;
   gulong off_a;
   gulong off_b;
   gulong size_b;
   gulong off_p;
   gulong size_p;
   gulong offset;
   guint width;
   guint height;
   guint ncolors;
   guint lpad;
   guint rpad;
   guint flagperline;
   guchar palette[256][3];
} mag_info;

GimvImage *mag_load       (GimvImageLoader *loader,
                           gpointer         data);

#endif /* __MAG_H__ */
