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

#ifndef __GIMV_SLIDESHOW_H__
#define __GIMV_SLIDESHOW_H__

#include "gimageview.h"

#define GIMV_TYPE_SLIDESHOW            (gimv_slideshow_get_type ())
#define GIMV_SLIDESHOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_SLIDESHOW, GimvSlideshow))
#define GIMV_SLIDESHOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_SLIDESHOW, GimvSlideshowClass))
#define GIMV_IS_SLIDESHOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_SLIDESHOW))
#define GIMV_IS_SLIDESHOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_SLIDESHOW))
#define GIMV_SLIDESHOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_SLIDESHOW, GimvSlideshowClass))

typedef struct GimvSlideshow_Tag      GimvSlideshow;
typedef struct GimvSlideshowClass_Tag GimvSlideshowClass;

typedef enum {
   GimvSlideshowWinModeFullScreen,
   GimvSlideshowWinModeMaximize,
   GimvSlideshowWinModeNormal
} GimvSlideshowWinMode;

struct GimvSlideshow_Tag
{
   GObject       parent;
};

struct GimvSlideshowClass_Tag
{
   GObjectClass parent_class;
};

GType          gimv_slideshow_get_type          (void);

GimvSlideshow *gimv_slideshow_new               (void);
GimvSlideshow *gimv_slideshow_new_with_filelist (GList     *filelist,
                                                 GList     *start);
void           gimv_slideshow_play              (GimvSlideshow *slideshow);
void           gimv_slideshow_stop              (GimvSlideshow *slideshow);
void           gimv_slideshow_set_interval      (GimvSlideshow *slideshow,
                                                 guint          interval); /* [msec] */

#endif /* __GIMV_SLIDESHOW_H__ */
