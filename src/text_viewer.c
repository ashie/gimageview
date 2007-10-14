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

#include <stdio.h>

#include "gimageview.h"
#include "charset.h"
#include "prefs.h"
#include "text_viewer.h"

static void
cb_text_viewer_destroy (GtkWidget *widget, TextViewer *text_viewer)
{
   g_return_if_fail (text_viewer);

   if (text_viewer->filename)
      g_free (text_viewer->filename);

   g_free (text_viewer);
}


gboolean
text_viewer_load_file (TextViewer *text_viewer, gchar *filename)
{
   FILE *textfile;
   gchar *tmpstr;
   gchar buf[BUF_SIZE];
   GtkTextBuffer *buffer;
   gchar *text;

   g_return_val_if_fail (text_viewer && filename, FALSE);

   if (text_viewer->filename) {
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_viewer->textbox));
      gtk_text_buffer_set_text (buffer, "\0", -1);
      g_free (text_viewer->filename);
      text_viewer->filename = NULL;
   }

   textfile = fopen (filename, "r");
   if (!textfile) {
      g_warning (_("Can't open text file: %s\n"), filename);
      return FALSE;
   }

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_viewer->textbox));

   text = g_strdup ("");
   while (fgets (buf, sizeof(buf), textfile)) {
      gchar  *tmpstr, *prev;
      tmpstr = charset_to_internal (buf, NULL, NULL,
                                    CHARSET_TO_INTERNAL_LOCALE);
      prev = text;
      text = g_strconcat (text, tmpstr, NULL);
      g_free (prev);
   }
   gtk_text_buffer_set_text (buffer, text, -1);
   g_free (text);

   fclose (textfile);

   tmpstr = g_strconcat (_("File Name: "), filename, NULL);
   gtk_statusbar_push(GTK_STATUSBAR (text_viewer->statusbar), 1, tmpstr);
   g_free (tmpstr);

   text_viewer->filename = g_strdup (filename);

   return TRUE;
}


TextViewer *
text_viewer_create (gchar *filename)
{
   TextViewer *text_viewer;
   GtkWidget *window, *vbox;
   GtkWidget *scrolledwin, *text;
   GtkWidget *statusbar;

   text_viewer = g_new0 (TextViewer, 1);
   text_viewer->filename = NULL;

   /* window */
   text_viewer->window = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title (GTK_WINDOW (window), GIMV_PROG_NAME" -Text Viewer-");
   gtk_window_set_default_size (GTK_WINDOW(window), 600, 500);
   gtk_widget_show (window);
   g_signal_connect (G_OBJECT (window), "destroy",
                     G_CALLBACK (cb_text_viewer_destroy), text_viewer);

   /* main vbox */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (window), vbox);
   gtk_widget_show (vbox);

   /* text box */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
   gtk_box_pack_start (GTK_BOX (vbox), scrolledwin, TRUE, TRUE, 0);
   gtk_widget_show (scrolledwin);

   text = gtk_text_view_new ();
   text_viewer->textbox = text;
   gtk_container_add (GTK_CONTAINER (scrolledwin), text);
   gtk_widget_show (text);

   /* statusbar */
   text_viewer->statusbar = statusbar = gtk_statusbar_new ();
   gtk_container_border_width (GTK_CONTAINER (statusbar), 1);
   gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
   gtk_statusbar_push(GTK_STATUSBAR (statusbar), 1, "New Window");
   gtk_widget_show (statusbar);

   /* set text from file */
   if (filename)
      text_viewer_load_file (text_viewer, filename);

   return text_viewer;
}
