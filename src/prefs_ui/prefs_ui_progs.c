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

#include <string.h>

#include "gimageview.h"

#include "charset.h"
#include "gimv_elist.h"
#include "gtkutils.h"
#include "prefs.h"
#include "prefs_ui_progs.h"
#include "gimv_prefs_ui_utils.h"
#include "gimv_prefs_win.h"


typedef struct PrefsWin_Tag
{
   GtkWidget *text_viewer_command;
} PrefsWin;

static PrefsWin  prefs_win;
extern Config   *config_changed;
extern Config   *config_prechanged;


static void
set_default_progs_list (GimvEList *editlist)
{
   gint i, j, num;
   gchar **pair, *data[16];

   num = sizeof (conf.progs) / sizeof (gchar *);
   j = 0;

   gimv_elist_set_max_row (editlist, num);

   for (i = 0; i < num; i++) {
      if (!config_changed->progs[i]) continue;

      pair = g_strsplit (conf.progs[i], ",", 3);
      if (pair[0] && pair[1]) {
         data[0] = charset_locale_to_internal (pair[0]);
         data[1] = charset_locale_to_internal (pair[1]);
         if (pair[2] && !g_strcasecmp ("TRUE", pair[2]))
            data[2] = _("TRUE");
         else
            data[2] = _("FALSE");

         gimv_elist_append_row (GIMV_ELIST (editlist), data);

         if (config_changed->progs[i] == config_prechanged->progs[i]) {
            config_changed->progs[j] = g_strdup (config_changed->progs[i]);
         } else {
            config_changed->progs[j] = config_changed->progs[i];
         }
         if (j != i)
            config_changed->progs[i] = NULL;

         g_free (data[0]);
         g_free (data[1]);

         j++;
      }
      g_strfreev (pair);
   }
}


static void
cb_editlist_updated (GimvEList *editlist,
                     GimvEListActionType type,
                     gint selected_row,
                     gpointer data)
{
   gint i,  maxnum = sizeof (conf.progs) / sizeof (gchar *);

   for (i = 0; i < editlist->rows && i < maxnum; i++) {
      gchar **text, *confstring, *booltext, *locale[2];

      text = gimv_elist_get_row_text (editlist, i);
      if (!text) continue;

      if (text[2] && *text[2] && !strcmp (text[2], _("TRUE")))
         booltext = "TRUE";
      else
         booltext = "FALSE";

      locale[0] = charset_internal_to_locale (text[0]);
      locale[1] = charset_internal_to_locale (text[1]);

      confstring = g_strconcat (locale[0], ",", locale[1], ",",
                                booltext, NULL);

      g_free (locale[0]);
      g_free (locale[1]);

      g_strfreev (text);

      if (config_changed->progs[i] != config_prechanged->progs[i])
         g_free (config_changed->progs[i]);
      config_changed->progs[i] = confstring;
   }

   for (i = editlist->rows; i < maxnum; i++) {
      if (config_changed->progs[i] != config_prechanged->progs[i])
         g_free (config_changed->progs[i]);
      config_changed->progs[i] = NULL;
   }
}


static void
cb_use_internal_text_viewer (GtkWidget *toggle)
{
   config_changed->text_viewer_use_internal =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));

   gtk_widget_set_sensitive (prefs_win.text_viewer_command,
                             !config_changed->text_viewer_use_internal);
}


static void
cb_text_view_command_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->text_viewer != config_prechanged->text_viewer)
      g_free (config_changed->text_viewer);

   config_changed->text_viewer
      = charset_internal_to_locale (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
cb_web_browser_command_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->web_browser != config_prechanged->web_browser)
      g_free (config_changed->web_browser);

   config_changed->web_browser
      = charset_internal_to_locale (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
cb_scripts_use_default_dir_list(GtkToggleButton *button, GtkWidget *widget)
{
   g_return_if_fail (GTK_IS_WIDGET(widget));

   if (button->active) {
      config_changed->scripts_use_default_search_dir_list = TRUE;
      gtk_widget_hide (widget);
   } else {
      config_changed->scripts_use_default_search_dir_list = FALSE;
      gtk_widget_show (widget);
   }
}



/*******************************************************************************
 *  prefs_progs_page:
 *     @ Create external programs preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_progs_page (void)
{
   gchar *titles[] = {
      _("Program Name"),
      _("Command"),
      _("Dialog"),
   };
   gint titles_num = sizeof (titles) / sizeof (gchar *);

   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *hbox2, *vbox;
   GtkWidget *editlist, *label, *entry, *toggle;
   gchar *tmpstr;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);


   /**********************************************
    *  Viewer/Editor frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Graphic Viewer/Editor"),
                               frame, frame_vbox, main_vbox, TRUE);

   editlist = gimv_elist_new_with_titles (titles_num, titles);
   set_default_progs_list (GIMV_ELIST (editlist));
   gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (editlist), "list_updated",
                     G_CALLBACK (cb_editlist_updated), NULL);

   /*
    *  create edit area
    */
   hbox = GIMV_ELIST (editlist)->edit_area;

   /* program name entry */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);

   label = gtk_label_new (_("Program Name: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 0,
                                    NULL, FALSE);
   gtk_widget_set_usize (entry, 100, -1);
   gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);

   /* command entry */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);

   label = gtk_label_new (_("Command: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

   hbox2 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 1,
                                    NULL, FALSE);
   gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);

   /* check box */
   toggle = gimv_elist_create_check_button (GIMV_ELIST (editlist), 2,
                                            _("Dialog"), FALSE,
                                            _("TRUE"), _("FALSE"));
   gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);


   /**********************************************
    *  Web Browser frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Web Browser"),
                               frame, frame_vbox, main_vbox, FALSE);

   /* web browser command */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);
   label = gtk_label_new (_("Command: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   entry = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
   tmpstr = charset_locale_to_internal (conf.web_browser);
   gtk_entry_set_text (GTK_ENTRY (entry), tmpstr);
   g_free (tmpstr);
   g_signal_connect (G_OBJECT (entry),"changed",
                     G_CALLBACK (cb_web_browser_command_changed), NULL);


   /**********************************************
    *  Text Viewer frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Text Viewer"),
                               frame, frame_vbox, main_vbox, FALSE);

   /* use internal text viewer or not */
   toggle = gtkutil_create_check_button (_("Use internal viewer"),
                                         conf.text_viewer_use_internal,
                                         cb_use_internal_text_viewer,
                                         &config_changed->text_viewer_use_internal);
   gtk_box_pack_start (GTK_BOX (frame_vbox), toggle, FALSE, FALSE, 0);

   /* text viewer command */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);
   label = gtk_label_new (_("Command: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   prefs_win.text_viewer_command = entry = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
   tmpstr = charset_locale_to_internal (conf.text_viewer);
   gtk_entry_set_text (GTK_ENTRY (entry), tmpstr);
   g_free (tmpstr);
   g_signal_connect (G_OBJECT (entry),"changed",
                     G_CALLBACK (cb_text_view_command_changed), NULL);

   gtk_widget_set_sensitive (prefs_win.text_viewer_command,
                             !config_changed->text_viewer_use_internal);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_scripts_page:
 *     @ Create List View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_scripts_page (void)
{
   GtkWidget *main_vbox, *frame, *toggle;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   frame = gimv_prefs_ui_dir_list_prefs (_("Directories list to search scripts"),
                                         _("Select scripts directory"),
                                         config_prechanged->scripts_search_dir_list,
                                         &config_changed->scripts_search_dir_list,
                                         ',');
   toggle = gtkutil_create_check_button (_("Use default directories list to search scripts"),
                                         conf.scripts_use_default_search_dir_list,
                                         cb_scripts_use_default_dir_list,
                                         frame);

   gtk_box_pack_start(GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

   /* Show dialog */
   toggle = gtkutil_create_check_button (_("Show dialog befor execute script"),
                                         conf.scripts_show_dialog,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->scripts_show_dialog);
   gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   if (config_changed->scripts_use_default_search_dir_list)
      gtk_widget_hide (frame);
   else
      gtk_widget_show (frame);

   return main_vbox;
}
