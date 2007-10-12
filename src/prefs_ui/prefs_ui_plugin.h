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

#ifndef __PREFS_UI_PLUGIN_H__
#define __PREFS_UI_PLUGIN_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "gimv_prefs_win.h"

GtkWidget *prefs_ui_plugin              (void);
GtkWidget *prefs_ui_plugin_image_loader (void);
GtkWidget *prefs_ui_plugin_image_saver  (void);
GtkWidget *prefs_ui_plugin_io_stream    (void);
GtkWidget *prefs_ui_plugin_archiver     (void);
GtkWidget *prefs_ui_plugin_thumbnail    (void);
GtkWidget *prefs_ui_plugin_imageview    (void);
GtkWidget *prefs_ui_plugin_thumbview    (void);

gboolean prefs_ui_plugin_apply         (GimvPrefsWinAction action);
void     prefs_ui_plugin_set_sensitive (gboolean text_changed);

#endif /* __PREFS_UI_PLUGIN_H__ */
