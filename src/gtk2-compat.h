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

#ifndef __GTK2_COMPAT_H__
#define __GTK2_COMPAT_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#define gtk_object_class_add_signals(class, func, type)
#define OBJECT_CLASS_SET_FINALIZE_FUNC(klass, func) \
      G_OBJECT_CLASS(klass)->finalize = func
#define OBJECT_CLASS_FINALIZE_SUPER(parent_class, object) \
      if (G_OBJECT_CLASS(parent_class)->finalize) \
         G_OBJECT_CLASS(parent_class)->finalize (object);

gboolean gtk2compat_scroll_to_button_cb (GtkWidget      *widget,
					 GdkEventScroll *event,
					 gpointer        data);

#define SIGNAL_CONNECT_TRANSRATE_SCROLL(obj) \
      g_signal_connect (G_OBJECT(obj), "scroll-event", \
                        G_CALLBACK(gtk2compat_scroll_to_button_cb), \
                        NULL);

#endif /* __GTK2_COMPAT_H__ */
