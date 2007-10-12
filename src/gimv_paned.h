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

/*
 * These codes are taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#ifndef __GIMV_PANED_H__
#define __GIMV_PANED_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtk/gtkpaned.h>

#define GimvPaned GtkPaned
#define GIMV_TYPE_PANED                      GTK_TYPE_PANED
#define GIMV_PANED(paned)                    GTK_PANED(paned)
#define GIMV_IS_PANED(obj)                   GTK_IS_PANED(paned)
#define GIMV_IS_PANED_CLASS(klass)           GTK_IS_PANED_CLASS(paned)
#define gimv_paned_add1(paned, widget)       gtk_paned_add1(paned, widget) 
#define gimv_paned_add2(paned, widget)       gtk_paned_add2(paned, widget) 
#define gimv_paned_set_position(paned, size) gtk_paned_set_position(paned, size)
#define gimv_paned_get_position(paned)       gtk_paned_get_position(paned)
#define gimv_paned_split(paned) \
{ \
   gtk_widget_show (paned->child1); \
   gtk_widget_show (paned->child2); \
}
#define gimv_paned_hide_child1(paned)        gtk_widget_hide (paned->child1)
#define gimv_paned_hide_child2(paned)        gtk_widget_hide (paned->child2)

guint gimv_paned_which_hidden (GimvPaned *paned);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMV_PANED_H__ */
