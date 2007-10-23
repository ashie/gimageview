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
#include "gimv_text_win.h"
#include "prefs.h"
#include "utils_char_code.h"

G_DEFINE_TYPE (GimvTextWin, gimv_text_win, GTK_TYPE_WINDOW)

#define GIMV_TEXT_WIN_GET_PRIVATE(obj) \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMV_TYPE_TEXT_WIN, GimvTextWinPrivate))

typedef struct GimvTextWinPrivate_Tag
{
   GtkWidget *textbox;
   GtkWidget *menubar;
   GtkWidget *statusbar;
   gchar     *filename;
} GimvTextWinPrivate;

static void gimv_text_win_dispose (GObject *object);

static void
gimv_text_win_class_init (GimvTextWinClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass *) klass;
   gobject_class->dispose = gimv_text_win_dispose;

   g_type_class_add_private (gobject_class, sizeof (GimvTextWinPrivate));
}

static void
gimv_text_win_init (GimvTextWin *text_win)
{
   GimvTextWinPrivate *priv = GIMV_TEXT_WIN_GET_PRIVATE (text_win);
   GtkWidget *vbox;
   GtkWidget *scrolledwin, *text;
   GtkWidget *statusbar;

   priv->filename = NULL;

   /* window */
   gtk_window_set_title (GTK_WINDOW (text_win), GIMV_PROG_NAME" -Text Viewer-");
   gtk_window_set_default_size (GTK_WINDOW (text_win), 600, 500);

   /* main vbox */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (text_win), vbox);
   gtk_widget_show (vbox);

   /* text box */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
   gtk_box_pack_start (GTK_BOX (vbox), scrolledwin, TRUE, TRUE, 0);
   gtk_widget_show (scrolledwin);

   text = gtk_text_view_new ();
   priv->textbox = text;
   gtk_container_add (GTK_CONTAINER (scrolledwin), text);
   gtk_widget_show (text);

   /* statusbar */
   priv->statusbar = statusbar = gtk_statusbar_new ();
   gtk_container_border_width (GTK_CONTAINER (statusbar), 1);
   gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
   gtk_statusbar_push(GTK_STATUSBAR (statusbar), 1, "New Window");
   gtk_widget_show (statusbar);
}

GtkWidget *
gimv_text_win_new (void)
{
   return GTK_WIDGET (g_object_new (GIMV_TYPE_TEXT_WIN, NULL));
}

static void
gimv_text_win_dispose (GObject *object)
{
   GimvTextWin *text_win = GIMV_TEXT_WIN (object);
   GimvTextWinPrivate *priv = GIMV_TEXT_WIN_GET_PRIVATE (text_win);

   if (priv->filename) {
      g_free (priv->filename);
      priv->filename = NULL;
   }

   if (G_OBJECT_CLASS (gimv_text_win_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_text_win_parent_class)->dispose (object);
}

gboolean
gimv_text_win_load_file (GimvTextWin *text_win, gchar *filename)
{
   GimvTextWinPrivate *priv;
   FILE *textfile;
   gchar *tmpstr;
   gchar buf[BUF_SIZE];
   GtkTextBuffer *buffer;
   gchar *text;

   g_return_val_if_fail (GIMV_IS_TEXT_WIN (text_win) && filename, FALSE);

   priv = GIMV_TEXT_WIN_GET_PRIVATE (text_win);

   if (priv->filename) {
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textbox));
      gtk_text_buffer_set_text (buffer, "\0", -1);
      g_free (priv->filename);
      priv->filename = NULL;
   }

   textfile = fopen (filename, "r");
   if (!textfile) {
      g_warning (_("Can't open text file: %s\n"), filename);
      return FALSE;
   }

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textbox));

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
   gtk_statusbar_push(GTK_STATUSBAR (priv->statusbar), 1, tmpstr);
   g_free (tmpstr);

   priv->filename = g_strdup (filename);

   return TRUE;
}
