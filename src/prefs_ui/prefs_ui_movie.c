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

#include "prefs_ui_movie.h"

#include <string.h>
#include "gimv_image_view.h"
#include "prefs.h"
#include "utils_menu.h"

extern Config   *config_changed;
extern Config   *config_prechanged;

static const gchar **movie_view_modes = NULL;
static gint movie_view_modes_len = 0;


static void
cb_movie_view_mode (GtkWidget *widget, gpointer data)
{
   gint idx;

   idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "num"));
   g_return_if_fail (idx >=0 && idx < movie_view_modes_len);

   if (config_changed->movie_default_view_mode != config_prechanged->movie_default_view_mode)
      g_free (config_changed->movie_default_view_mode);
   config_changed->movie_default_view_mode = g_strdup (movie_view_modes[idx]);
}


GtkWidget *
prefs_movie_page (void)
{
   GtkWidget *main_vbox, *hbox, *vbox;
   GtkWidget *label, *option_menu;
   gint i, defval = 0;

   main_vbox = gtk_vbox_new (FALSE, 0);

   /* create label array */
   if (!movie_view_modes) {
      GList *node, *list = gimv_image_view_plugin_get_list();

      movie_view_modes_len = g_list_length(list);
      if (movie_view_modes_len > 1) {
         movie_view_modes = g_new0(const gchar *, movie_view_modes_len + 1);
         for (i = 0, node = list;
              i < movie_view_modes_len && node;
              i++, node = g_list_next (node))
         {
            GimvImageViewPlugin *plugin = node->data;
            if (!plugin || !plugin->label || !*plugin->label) continue;
            movie_view_modes[i] = g_strdup(plugin->label);
         }
         movie_view_modes[movie_view_modes_len] = NULL;
      }
   }

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show (hbox);

   if (movie_view_modes_len > 1) {
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width(GTK_CONTAINER (vbox), 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Default view mode for movie and audio: "));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 2);
      gtk_widget_show (label);

      for (i = 0; conf.movie_default_view_mode && movie_view_modes[i]; i++) {
         if (!strcmp (conf.movie_default_view_mode, movie_view_modes[i])) {
            defval = i;
            break;
         }
      }

      option_menu = create_option_menu (movie_view_modes, defval,
                                        cb_movie_view_mode, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), option_menu, FALSE, FALSE, 2);
      gtk_widget_show (option_menu);

   } else {
      label = gtk_label_new (_("No movie plugins are available."));
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 2);
      gtk_widget_show (label);
   }

   return main_vbox;
}
