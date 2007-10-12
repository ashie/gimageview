/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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

#ifndef __PREFS_UI_XINE_H__
#define __PREFS_UI_XINE_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef ENABLE_XINE

#include "gimv_plugin.h"
#include "gimv_prefs_win.h"

gboolean gimv_prefs_xine_get_thumb_enable (void);
gfloat   gimv_prefs_xine_get_thumb_pos    (void);
gfloat   gimv_prefs_xine_get_delay   (GimvPluginInfo    *this);
gboolean gimv_prefs_ui_xine_get_page (guint              idx,
                                      GimvPrefsWinPage **page,
                                      guint             *size);

#endif /* ENABLE_XINE */

#endif /* __PREFS_UI_XINE_H__ */
