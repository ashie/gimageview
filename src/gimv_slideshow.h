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

typedef enum {
   GimvSlideshowWinModeFullScreen,
   GimvSlideshowWinModeMaximize,
   GimvSlideshowWinModeNormal
} GimvSlideshowWinMode;

struct GimvSlideshow_Tag
{
   GimvImageWin *iw;
   GList        *filelist;
   GList        *current;
   gboolean      repeat;
};

GimvSlideshow *gimv_slideshow_new               (void);
GimvSlideshow *gimv_slideshow_new_with_filelist (GList     *filelist,
                                                 GList     *start);
void           gimv_slideshow_play              (GimvSlideshow *slideshow);
void           gimv_slideshow_stop              (GimvSlideshow *slideshow);
void           gimv_slideshow_set_interval      (GimvSlideshow *slideshow,
                                                 guint          interval); /* [msec] */

#endif /* __GIMV_SLIDESHOW_H__ */
