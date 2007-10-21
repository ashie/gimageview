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


#ifndef __GIMV_TEXT_WIN_H__
#define __GIMV_TEXT_WIN_H__

#include <gtk/gtk.h>

#define GIMV_TYPE_TEXT_WIN            (gimv_text_win_get_type ())
#define GIMV_TEXT_WIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_TEXT_WIN, GimvTextWin))
#define GIMV_TEXT_WIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_TEXT_WIN, GimvTextWinClass))
#define GIMV_IS_TEXT_WIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_TEXT_WIN))
#define GIMV_IS_TEXT_WIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_TEXT_WIN))
#define GIMV_TEXT_WIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_TEXT_WIN, GimvTextWinClass))

typedef struct GimvTextWin_Tag      GimvTextWin;
typedef struct GimvTextWinClass_Tag GimvTextWinClass;

struct GimvTextWin_Tag
{
   GtkWindow parent;
};

struct GimvTextWinClass_Tag
{
   GtkWindowClass parent_class;
};

GType      gimv_text_win_get_type  (void);
GtkWidget *gimv_text_win_new       (void);
gboolean   gimv_text_win_load_file (GimvTextWin *text_viewer,
                                    gchar       *filename);

#endif /* __GIMV_TEXT_WIN_H__ */
