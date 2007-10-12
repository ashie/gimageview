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

#include "gtk2-compat.h"
#include <gtk/gtk.h>


gboolean
gtk2compat_scroll_to_button_cb (GtkWidget *widget,
                                GdkEventScroll *se,
                                gpointer data)
{
   GdkEventButton be;
   gboolean retval;

   g_return_val_if_fail (GTK_IS_WIDGET(widget), FALSE);

   be.type       = GDK_BUTTON_PRESS;
   be.window     = se->window;
   be.send_event = se->send_event;
   be.time       = se->time;
   be.x          = se->x;
   be.y          = se->y;
   be.axes       = NULL;
   be.state      = se->state;
   be.device     = se->device;
   be.x_root     = se->x_root;
   be.y_root     = se->y_root;
   switch ((se)->direction) {
   case GDK_SCROLL_UP:
      be.button = 4;
      break;
   case GDK_SCROLL_DOWN:
      be.button = 5;
      break;
   case GDK_SCROLL_LEFT:
      be.button = 6;
      break;
   case GDK_SCROLL_RIGHT:
      be.button = 7;
      break;
   default:
      g_warning ("invalid scroll direction!");
      be.button = 0;
      break;
   }

   g_signal_emit_by_name (G_OBJECT(widget), "button-press-event",
                          &be, &retval);
   be.type = GDK_BUTTON_RELEASE;
   g_signal_emit_by_name (G_OBJECT(widget), "button-release-event",
                          &be, &retval);

   return retval;
}

#include "gimv_paned.h"
guint
gimv_paned_which_hidden (GimvPaned *paned)
{
   g_return_val_if_fail (GIMV_IS_PANED (paned), 0);
   g_return_val_if_fail (paned->child1 && paned->child2, 0);

   if (!GTK_WIDGET_VISIBLE (paned->child1)
       && GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 1;

   } else if (GTK_WIDGET_VISIBLE (paned->child1)
              && !GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 2;

   } else {
      return 0;
   }
}
