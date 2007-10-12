/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  Copyright (C) 2001 The Free Software Foundation
 *
 *  This program is released under the terms of the GNU General Public 
 *  License version 2, you can find a copy of the lincense in the file
 *  COPYING.
 *
 *  $Id$
 */

#ifndef PIXBUF_UTILS_H
#define PIXBUF_UTILS_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_GDK_PIXBUF

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

GdkPixbuf *pixbuf_copy_rotate_90  (GdkPixbuf *src, 
                                   gboolean counter_clockwise);

GdkPixbuf *pixbuf_copy_mirror     (GdkPixbuf *src, 
                                   gboolean mirror, 
                                   gboolean flip);

void       pixmap_from_xpm        (const char **data, 
                                   GdkPixmap **pixmap, 
                                   GdkBitmap **mask);

#endif /* HAVE_GDK_PIXBUF */
#endif /* PIXBUF_UTILS_H */
