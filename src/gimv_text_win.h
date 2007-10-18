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

typedef struct GimvTextWin_Tag
{
   GtkWidget *window;
   GtkWidget *textbox;
   GtkWidget *menubar;
   GtkWidget *statusbar;

   gchar *filename;
} GimvTextWin;


gboolean     gimv_text_win_load_file (GimvTextWin *text_viewer,
                                      gchar       *filename);
GimvTextWin *gimv_text_win_create    (gchar       *filename);

#endif /* __GIMV_TEXT_WIN_H__ */
