/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2002 Takuro Ashie
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

#ifndef __GIMV_ANIM_H__
#define __GIMV_ANIM_H__

#include "gimv_image.h"


#define GIMV_TYPE_ANIM            (gimv_anim_get_type ())
#define GIMV_ANIM(obj)            (GTK_CHECK_CAST (obj, gimv_anim_get_type (), GimvAnim))
#define GIMV_ANIM_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_anim_get_type, GimvAnimClass))
#define GIMV_IS_ANIM(obj)         (GTK_CHECK_TYPE (obj, gimv_anim_get_type ()))
#define GIMV_IS_ANIM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_ANIM))


typedef struct GimvAnim_Tag          GimvAnim;
typedef struct GimvAnimClass_Tag     GimvAnimClass;
typedef struct GimvAnimFuncTable_Tag GimvAnimFuncTable;


struct GimvAnim_Tag
{
   GimvImage          parent;

   gpointer           anim;
   gint               current_frame_idx;
   GimvAnimFuncTable *table;
};


struct GimvAnimClass_Tag
{
   GimvImageClass parent_class;
};


struct GimvAnimFuncTable_Tag
{
   gint     (*get_length)   (GimvAnim    *anim);
   gint     (*get_idx)      (GimvAnim    *anim);
   gint     (*get_interval) (GimvAnim    *anim);
   gint     (*iterate)      (GimvAnim    *anim);
   gboolean (*seek)         (GimvAnim    *anim,
                             gint         idx);
   void     (*delete)       (GimvAnim    *anim);
};


GtkType    gimv_anim_get_type     (void);
GimvAnim  *gimv_anim_new          (void);

/* public */
gint       gimv_anim_get_length   (GimvAnim    *anim);
gint       gimv_anim_iterate      (GimvAnim    *anim);
gboolean   gimv_anim_seek         (GimvAnim    *anim,
                                   gint         idx);
gint       gimv_anim_get_interval (GimvAnim    *anim);

/* used by child */
gboolean   gimv_anim_update_frame (GimvAnim *anim,
                                   guchar   *frame,
                                   gint      width,
                                   gint      height,
                                   gboolean  has_alpha);

#endif /* __GIMV_ANIM_H__ */
