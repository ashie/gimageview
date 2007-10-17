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


#ifndef __GIMV_DUPL_WIN_H__
#define __GIMV_DUPL_WIN_H__

#include "gimageview.h"

#include <gtk/gtkdialog.h>

#include "gimv_dupl_finder.h"
#include "gimv_thumb_view.h"

#define GIMV_TYPE_DUPL_WIN            (gimv_dupl_win_get_type ())
#define GIMV_DUPL_WIN(obj)            (GTK_CHECK_CAST (obj, gimv_dupl_win_get_type (), GimvDuplWin))
#define GIMV_DUPL_WIN_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_dupl_win_get_type, GimvDuplWinClass))
#define GIMV_IS_DUPL_WIN(obj)         (GTK_CHECK_TYPE (obj, gimv_dupl_win_get_type ()))
#define GIMV_IS_DUPL_WIN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_DUPL_WIN))

typedef struct GimvDuplWin_Tag      GimvDuplWin;
typedef struct GimvDuplWinClass_Tag GimvDuplWinClass;
typedef struct GimvDuplWinPriv_Tag  GimvDuplWinPriv;


struct GimvDuplWin_Tag
{
   GtkDialog         parent;

   GtkWidget        *ctree;
   GtkWidget        *radio_thumb;
   GtkWidget        *radio_icon;
   GtkWidget        *select_button;
   GtkWidget        *stop_button;
   GtkWidget        *progressbar;

   GimvDuplFinder   *finder;
   GimvThumbView    *tv;

   GimvDuplWinPriv   *priv;
};


struct GimvDuplWinClass_Tag
{
   GtkDialogClass parent_class;
};


/* result window */
GType                gimv_dupl_win_get_type       (void);
GimvDuplWin         *gimv_dupl_win_new            (gint thumbnail_size);
void                 gimv_dupl_win_set_relation   (GimvDuplWin         *sw,
                                                   GimvThumbView       *tv);
void                 gimv_dupl_win_unset_relation (GimvDuplWin         *sw);
void                 gimv_dupl_win_set_thumb      (GimvDuplWin         *sw,
                                                   GimvThumb           *thumb1,
                                                   GimvThumb           *thumb2,
                                                   gfloat               similar);

#endif /* __GIMV_DUPL_WIN_H__ */
