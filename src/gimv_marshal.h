/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * marshal.c - custom marshals collection.
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
 * Foundation, Inc., 59 Temple Place - Suite 3S30, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */


#ifndef __MARSHAL_H__
#define __MARSHAL_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>

#if (GTK_MAJOR_VERSION >= 2)

#include	<glib-object.h>

extern void gtk_marshal_INT__INT_INT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

#else /* (GTK_MAJOR_VERSION >= 2) */

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>

void gtk_marshal_INT__INT_INT (GtkObject * object,
                               GtkSignalFunc func,
                               gpointer func_data, GtkArg * args);

#endif /* (GTK_MAJOR_VERSION >= 2) */

#endif /* __MARSHAL_H__ */
