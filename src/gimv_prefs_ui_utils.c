/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
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

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "gimv_prefs_ui_utils.h"

#include "charset.h"
#include "gimv_dlist.h"
#include "gimv_elist.h"
#include "gtkutils.h"
#include "prefs.h"
#include "gimv_prefs_win.h"

/*******************************************************************************
 *
 *  Mouse button preference
 *
 *******************************************************************************/
static void
cb_mouse_button (GtkWidget *menu_item, gpointer user_data)
{
   gchar **src = user_data, *dest, *tempstr, buf[128], *defval;
   gchar **buttons, **mods;
   gpointer bid_p, mid_p, num_p;
   gint i, j, bid, mid, num, data[4];
   GtkToggleButton *pressed, *released;

   g_return_if_fail (menu_item);
   g_return_if_fail (src && *src && **src);

   bid_p    = gtk_object_get_data (GTK_OBJECT (menu_item), "button-id");
   mid_p    = gtk_object_get_data (GTK_OBJECT (menu_item), "mod-id");
   num_p    = gtk_object_get_data (GTK_OBJECT (menu_item), "num");
   defval   = gtk_object_get_data (GTK_OBJECT (menu_item), "prefs-prechanged");
   pressed  = gtk_object_get_data (GTK_OBJECT (menu_item), "pressed");
   released = gtk_object_get_data (GTK_OBJECT (menu_item), "released");

   bid = GPOINTER_TO_INT (bid_p);
   mid = GPOINTER_TO_INT (mid_p);
   num = GPOINTER_TO_INT (num_p);

   if (!*src) return;
   buttons = g_strsplit (*src, ";", 6);
   if (!buttons) return;
   dest = g_strdup ("");

   for (i = 0; i < 6; i++) {
      mods = g_strsplit (buttons[i], ",", 4);
      if (!mods) {
         g_strfreev (buttons);
         g_free (dest);
         return;
      }
      for (j = 0; j < 4; j++) {
         if (i == bid && j == mid) {
            if (released && released->active)
               data[j] = 0 - num;
            else
               data[j] = num;
         } else{
            g_strstrip (mods[j]);
            data[j] = atoi (mods[j]);
         }
      }
      g_snprintf (buf, 128, "%d,%d,%d,%d; ",
                  data[0], data[1], data[2], data[3]);
      tempstr = g_strconcat (dest, buf, NULL);
      g_free (dest);
      dest = tempstr;
      g_strfreev (mods);
   }

   if (defval != *src)
      g_free (*src);

   *src = dest;

   g_strfreev (buttons);
}


static void
cb_mouse_prefs_pressed_radio (GtkWidget *radio, gpointer data)
{
   GtkOptionMenu *option_menu = data;

   g_return_if_fail (option_menu);

   if (option_menu->menu_item)
      g_signal_emit_by_name (G_OBJECT (option_menu->menu_item),
                             "activate");
}


GtkWidget *
gimv_prefs_ui_mouse_prefs (const gchar **items,
                           const gchar  *defval,
                           gchar       **dest)
{
   const gchar *mod_keys[] = {
      N_("Normal"),
      N_("Shift"),
      N_("Control"),
      N_("Mod1")
   };
   gint mod_keys_num = sizeof (mod_keys) / sizeof (gchar *);

   GtkWidget *main_vbox, *vbox, *hbox, *vbox1, *vbox2, *temp_vbox;
   GtkWidget *frame1, *frame_vbox1;
   GtkWidget *label, *notebook, *option_menu, *menu, *menu_item;
   GtkWidget *radio0, *radio1;
   gchar buf[128], **temp = NULL, *buttons[6], **mods;
   gint id, num, i, j, k, def;

   g_return_val_if_fail (items, NULL);
   g_return_val_if_fail (defval, NULL);
   g_return_val_if_fail (dest, NULL);

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   hbox = gtk_hbox_new (TRUE, 0);
   gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, TRUE, 0);
   vbox1 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, TRUE, 0);
   vbox2 = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, TRUE, 0);

   if (defval)
      temp = g_strsplit (defval, ";", 6);
   if (temp) {
      buttons[0] = temp[1];
      buttons[1] = temp[2];
      buttons[2] = temp[3];
      buttons[3] = temp[0];
      buttons[4] = temp[4];
      buttons[5] = temp[5];
   } else {
      for (i = 0; i < 6; i++)
         buttons[i] = NULL;
   }

   /* create frames */
   for (i = 0; i < 6; i++) {
      if (i < 3) {
         num = i + 1;
         id = num;
      } else if (i == 3) {
         num = 1;
         id = 0;
      } else {
         num = i;
         id = num;
      }

      if (i / 3 < 1) {
         temp_vbox = vbox1;
      } else {
         temp_vbox = vbox2;
      }

      /**********************************************
       * Mouse Button X Frame
       **********************************************/
      if (num == 4) {
         g_snprintf (buf, 128, _("Mouse Button %d (Wheel up)"), num);
      } else if (num == 5) {
         g_snprintf (buf, 128, _("Mouse Button %d (Wheel down)"), num);
      } else if (i == 3) {
         g_snprintf (buf, 128, _("Mouse Button %d Double Click"), num);
      } else {
         const gchar *button_num_str[] = {
            NULL, N_("Left"), N_("Middle"), N_("Right")
         };
         g_snprintf (buf, 128, _("Mouse Button %d (%s)"),
                     num, _(button_num_str[num]));
      }
      frame1 = gtk_frame_new (buf);
      gtk_container_set_border_width(GTK_CONTAINER(frame1), 5);
      gtk_box_pack_start(GTK_BOX(temp_vbox), frame1, FALSE, TRUE, 5);
      frame_vbox1 = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width(GTK_CONTAINER(frame1), 5);
      gtk_container_add (GTK_CONTAINER (frame1), frame_vbox1);

      notebook = gtk_notebook_new ();
      gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
      gtk_box_pack_start(GTK_BOX(frame_vbox1), notebook, FALSE, FALSE, 5);
      gtk_widget_show (notebook);

      if (buttons[i] && *buttons[i]) {
         mods = g_strsplit (buttons[i], ",", 4);
      } else {
         mods = NULL;
      }

      /* create notebook pages */
      for (j = 0; j < mod_keys_num; j++) {
         label = gtk_label_new (_(mod_keys[j]));
         gtk_widget_show (label);
         vbox = gtk_vbox_new (FALSE, 0);
         gtk_widget_show (vbox);
         gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                   vbox, label);

         if (mods[j] && *mods[j]) {
            g_strstrip (mods[j]);
            def = atoi (mods[j]);
         } else {
            def = 0;
         }

         option_menu = gtk_option_menu_new();
         menu = gtk_menu_new();

         gtk_box_pack_start(GTK_BOX(vbox), option_menu, FALSE, FALSE, 5);

         hbox = gtk_hbox_new (FALSE, 0);
         gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

         radio0 = gtk_radio_button_new_with_label (NULL, _("Pressed"));
         gtk_box_pack_start (GTK_BOX (hbox), radio0, FALSE, FALSE, 0);
         radio1 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio0),
                                                               _("Released"));
         gtk_box_pack_start (GTK_BOX (hbox), radio1, FALSE, FALSE, 0);
         g_signal_connect (G_OBJECT (radio0), "toggled",
                           G_CALLBACK (cb_mouse_prefs_pressed_radio),
                           option_menu);
         g_signal_connect (G_OBJECT (radio1), "toggled",
                           G_CALLBACK (cb_mouse_prefs_pressed_radio),
                           option_menu);

         if (def < 0)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), TRUE);
         else
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio0), TRUE);

         if (id == 0) {
            gtk_widget_set_sensitive (radio0, FALSE);
            gtk_widget_set_sensitive (radio1, FALSE);
         }

         /* create option menu items */
         for (k = 0; items[k]; k++) {
            menu_item = gtk_menu_item_new_with_label (_(items[k]));
            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "button-id",
                                 GINT_TO_POINTER(id));
            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "mod-id",
                                 GINT_TO_POINTER(j));
            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "num",
                                 GINT_TO_POINTER(k));
            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "prefs-prechanged",
                                 (gpointer) defval);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                             G_CALLBACK(cb_mouse_button),
                             dest);

            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "pressed",
                                 (gpointer) radio0);
            gtk_object_set_data (GTK_OBJECT (menu_item),
                                 "released",
                                 (gpointer) radio1);
            gtk_menu_append (GTK_MENU(menu), menu_item);
            gtk_widget_show (menu_item);
         }
         gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
         gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), abs (def));
      }

      g_strfreev (mods);
   }

   g_strfreev (temp);

   return main_vbox;
}



/*******************************************************************************
 *
 *  Directory list preference
 *
 *******************************************************************************/
typedef struct DirListPrefs_Tag {
   GtkWidget *editlist, *entry, *select_button;
   const gchar *defval;
   gchar **prefsval;
   gchar sepchar;
   const gchar *dialog_title;
} DirListPrefs;


static void
set_default_dir_list (DirListPrefs *dirprefs)
{
   gchar **strs;
   gint i;

   if (!dirprefs->defval) return;

   strs = g_strsplit (dirprefs->defval, &dirprefs->sepchar, -1);
   for (i = 0; strs && strs[i]; i++) {
      gchar *text[1];

      text[0] = charset_to_internal (strs[i],
                                     conf.charset_filename,
                                     conf.charset_auto_detect_fn,
                                     conf.charset_filename_mode);

      gimv_elist_append_row (GIMV_ELIST (dirprefs->editlist), text);

      g_free (text[0]);
   }
   g_strfreev (strs);
}


static void
cb_dirprefs_editlist_updated (GimvEList *editlist, gpointer data)
{
   DirListPrefs *dirprefs = data;
   gchar *string = NULL;
   gint i;

   for (i = 0; i < editlist->rows; i++) {
      gchar **text, *text_locale, *tmpstr;

      text = gimv_elist_get_row_text (editlist, i);
      if (!text) continue;

      if (string) {
         tmpstr = string;
         string = g_strconcat (string, &dirprefs->sepchar, NULL);
         g_free (tmpstr);
      } else {
         string = g_strdup ("");
      }

      tmpstr = string;
      text_locale = charset_internal_to_locale (text[0]);
      string = g_strconcat (tmpstr, text_locale, NULL);
      g_free (text_locale);
      g_free (tmpstr);
   }

   if (dirprefs->defval != *dirprefs->prefsval) {
      g_free (*dirprefs->prefsval);
   }
   *dirprefs->prefsval = string;
}


static void
cb_dirprefs_dirsel_pressed (GtkButton *button, gpointer data)
{
   DirListPrefs *dirprefs = data;
   gchar *path;
   const gchar *default_path, *dialog_title = _("Select directory");

   if (dirprefs->dialog_title)
      dialog_title = dirprefs->dialog_title;

   default_path = gtk_entry_get_text (GTK_ENTRY (dirprefs->entry));
   path = gtkutil_modal_file_dialog (dialog_title,
                                     default_path,
                                     MODAL_FILE_DIALOG_DIR_ONLY,
                                     GTK_WINDOW (gimv_prefs_win_get ()));
   if (path)
      gtk_entry_set_text (GTK_ENTRY (dirprefs->entry), path);
   g_free (path);
}


static void
cb_dirprefs_destroy (GtkWidget *widget, gpointer data)
{
   g_free (data);
}


GtkWidget *
gimv_prefs_ui_dir_list_prefs (const gchar *title,
                              const gchar *dialog_title,
                              const gchar *defval,
                              gchar **prefsval,
                              gchar sepchar)
{
   DirListPrefs *dirprefs = g_new0 (DirListPrefs, 1);
   GtkWidget *frame, *frame_vbox, *hbox;
   GtkWidget *editlist, *label, *entry, *button;
   gchar *titles[] = {
      N_("Directory"),
   };
   gint titles_num = sizeof (titles)/ sizeof (gchar *);

   g_return_val_if_fail (prefsval, NULL);

   dirprefs->defval = defval;
   dirprefs->prefsval = prefsval;
   dirprefs->sepchar = sepchar;
   dirprefs->dialog_title = dialog_title;

   /* frame */
   frame = gtk_frame_new (title);
   gtk_container_set_border_width(GTK_CONTAINER (frame), 0);
   frame_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
   gtk_container_add (GTK_CONTAINER (frame), frame_vbox);

   g_signal_connect (G_OBJECT (frame),"destroy",
                     G_CALLBACK (cb_dirprefs_destroy),
                     dirprefs);

   /* clist */
   dirprefs->editlist = editlist
      = gimv_elist_new_with_titles (titles_num, titles);
   gimv_elist_set_column_title_visible (GIMV_ELIST (editlist), FALSE);
   gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
   set_default_dir_list (dirprefs);
   g_signal_connect (G_OBJECT (editlist), "list_updated",
                     G_CALLBACK (cb_dirprefs_editlist_updated), dirprefs);

   /* entry area */
   hbox = GIMV_ELIST (editlist)->edit_area;

   label = gtk_label_new (_("Directory to add : "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

   entry = dirprefs->entry
      = gimv_elist_create_entry (GIMV_ELIST (editlist), 0,
                                 NULL, FALSE);
   gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

   button = dirprefs->select_button = gtk_button_new_with_label (_("Select"));
   gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
   g_signal_connect (G_OBJECT (button),"clicked",
                     G_CALLBACK (cb_dirprefs_dirsel_pressed), dirprefs);

   return frame;
}



/*******************************************************************************
 *
 *  Double Stack List.
 *
 *******************************************************************************/
typedef struct GimvDListData_Tag
{
   GtkWidget *dslist;
   const gchar *defval;
   gchar **prefsval;
   gchar sepchar;
} GimvDListData;


static void
cb_dslist_destroy (GtkWidget *dslist, gpointer data)
{
   GimvDListData *dsdata = data;
   g_free (dsdata);
}


static void
cb_gimv_dlist_updated (GimvDList *dslist, GimvDListData *dsdata)
{
   gchar *string = NULL, *tmpstr;
   gint i, rows;

   g_return_if_fail (dsdata);

   rows = gimv_dlist_get_n_enabled_items (dslist);

   for (i = 0; i < rows; i++) {
      gchar *text;
      text = gimv_dlist_get_enabled_row_text (dslist, i);
      if (!text) continue;

      if (string) {
         tmpstr = string;
         string = g_strconcat (tmpstr, &dsdata->sepchar, NULL);
         g_free (tmpstr);
      } else {
         string = g_strdup ("");
      }
      tmpstr = string;
      string = g_strconcat (tmpstr, text, NULL);
      g_free (tmpstr);
   }

   if (!dsdata->prefsval) return;

   if (*dsdata->prefsval != dsdata->defval) {
      g_free (*dsdata->prefsval);
   }
   *dsdata->prefsval = string;
}


GtkWidget *
gimv_prefs_ui_double_clist (const gchar *title,
                            const gchar *clist1_title,
                            const gchar *clist2_title,
                            GList *available_list,
                            const gchar *defval,
                            gchar **prefsval,
                            gchar sepchar)
{
   GimvDListData *dsdata;
   GtkWidget *frame, *frame_vbox, *dslist;
   GList *node;
   gchar **titles;
   gint i;

   g_return_val_if_fail (prefsval, NULL);

   dsdata = g_new0 (GimvDListData, 1);
   dsdata->defval = defval;
   dsdata->prefsval = prefsval;
   dsdata->sepchar = sepchar;

   /* frame */
   frame = gtk_frame_new (title);
   gtk_container_set_border_width(GTK_CONTAINER (frame), 0);
   frame_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
   gtk_container_add (GTK_CONTAINER (frame), frame_vbox);


   /* double stack list */
   dslist = gimv_dlist_new (clist1_title, clist2_title);
   gtk_box_pack_start (GTK_BOX (frame_vbox), dslist, TRUE, TRUE, 0);
   gtk_widget_show (dslist);

   /* set available list */
   for (node = available_list; node; node = g_list_next (node)) {
      const gchar *text = node->data;
      gimv_dlist_append_available_item (GIMV_DLIST (dslist), text);
   }

   /* set default data */
   if (dsdata->defval) {
      titles = g_strsplit (dsdata->defval, &dsdata->sepchar, -1);
      for (i = 0; titles[i]; i++) {
         gimv_dlist_column_add_by_label (GIMV_DLIST (dslist), titles[i]);
      }
      g_strfreev (titles);
   }

   g_signal_connect (G_OBJECT (dslist), "destroy",
                     G_CALLBACK (cb_dslist_destroy), dsdata);
   g_signal_connect (G_OBJECT (dslist), "enabled_list_updated",
                     G_CALLBACK (cb_gimv_dlist_updated),
                     dsdata);

   return frame;
}
