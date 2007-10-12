/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/* 
 * Copyright (C) 2000-2003 the xine project
 * 
 * This file is part of xine, a unix video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 */

/*
 *  2003-04-11 Takuro Ashie <ashie@homa.ne.jp>
 *
 *    * merged from xine-ui-0.9.18 and adapt to GimvXine.
 */

#ifndef __GIMV_XINE_POST_H__
#define __GIMV_XINE_POST_H__

#include "gimv_xine.h"

void post_init                        (GimvXine      *gxine);
void post_rewire_visual_anim          (GimvXine      *gxine);
void post_rewire_video_post           (GimvXine      *gxine);
int  post_rewire_audio_port_to_stream (GimvXine      *gxine,
                                       xine_stream_t *stream);
int  post_rewire_audio_post_to_stream (GimvXine      *gxine,
                                       xine_stream_t *stream);
int  post_rewire_video_post_to_stream (GimvXine      *gxine,
                                       xine_stream_t *stream);

#endif /* __GIMV_XINE_POST_H__ */
