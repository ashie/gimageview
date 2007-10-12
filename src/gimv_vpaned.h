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

#ifndef __GIMV_VPANED_H__
#define __GIMV_VPANED_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk/gdk.h>
#include "gimv_paned.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef USE_NORMAL_PANED

#define GIMV_VPANED(obj)          GTK_CHECK_CAST (obj, gimv_vpaned_get_type (), GimvVPaned)
#define GIMV_VPANED_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gimv_vpaned_get_type (), GimvVPanedClass)
#define GIMV_IS_VPANED(obj)       GTK_CHECK_TYPE (obj, gimv_vpaned_get_type ())


typedef struct _GimvVPaned       GimvVPaned;
typedef struct _GimvVPanedClass  GimvVPanedClass;

struct _GimvVPaned
{
   GimvPaned paned;
};

struct _GimvVPanedClass
{
   GimvPanedClass parent_class;
};


GtkType    gimv_vpaned_get_type (void);
GtkWidget* gimv_vpaned_new      (void);

#else /* USE_NORMAL_PANED */

#include <gtk/gtkvpaned.h>

#define GimvVPaned GtkVPaned
#define gimv_vpaned_new() gtk_vpaned_new()

#endif /* USE_NORMAL_PANED */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMV_VPANED_H__ */
