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
#include "fileload.h"
/* #include "gimv_image.h" */
#include "gimv_mime_types.h"
#include "gtkutils.h"
#include "menu.h"
#include "prefs.h"
#include "prefs_ui_common.h"
#include "gimv_prefs_ui_utils.h"
#include "gimv_prefs_win.h"
#include "utils.h"


typedef struct PrefsWin_Tag {
   /* filtering */
   GtkWidget *filter_editlist;
   GtkWidget *filter_ext_entry;
   GtkWidget *filter_type_entry;
   GtkWidget *filter_disable_toggle;

   GtkWidget *filter_clist;
   GtkWidget *filter_new;
   GtkWidget *filter_add;
   GtkWidget *filter_change;
   GtkWidget *filter_del;
   gint       filter_selected;
} PrefsWin;


typedef struct _FilterRowData
{
   gboolean enable;
   gboolean user_def;
} FilterRowData;


static PrefsWin  prefs_win;
extern Config   *config_changed;
extern Config   *config_prechanged;


static const gchar *open_window_items[] = {
   N_("Image View"),
   N_("Thumbnail View"),
   NULL
};


static const gchar *interpolation_items[] = {
   N_("Nearest"),
   N_("Tiles"),
   N_("Bilinear"),
   N_("Hyperbolic"),
   NULL
};


static const gchar *charset_to_internal_items[] = {
   N_("Never convert"),
   N_("Locale"),
   N_("Auto detect"),
   N_("Any"),
   NULL
};


static void
cb_scan_sub_dir (GtkToggleButton *button, gpointer data)
{
   ScanSubDirType *type = data;

   if (button->active)
      *type = SCAN_SUB_DIR;
   else
      *type = SCAN_SUB_DIR_NONE;
}


static void
cb_recursive_follow_symlink (GtkWidget *radio, gpointer data)
{
   gint idx = GPOINTER_TO_INT (data);

   if (!GTK_TOGGLE_BUTTON (radio)->active) return;

   switch (idx) {
   case 0:
      config_changed->recursive_follow_link   = TRUE;
      config_changed->recursive_follow_parent = TRUE;
      break;
   case 1:
      config_changed->recursive_follow_link   = FALSE;
      config_changed->recursive_follow_parent = FALSE;
      break;
   case 2:
      config_changed->recursive_follow_link   = TRUE;
      config_changed->recursive_follow_parent = FALSE;
      break;
   default:
      break;
   }
}


static gboolean
filter_check_value (const gchar *ext, const gchar *name)
{
   if (!string_is_ascii_printable (ext) || !string_is_ascii_printable (name)) {
      gtkutil_message_dialog (_("Error"),
                              _("String includes invalid character!\n"
                                "Only ASCII is available."),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   if (strchr(ext, ',') || strchr(name, ',')) {
      gtkutil_message_dialog (_("Error"),
                              _("Sorry, cannot include \",\" character!"),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   if (strchr(ext, ';') || strchr(name, ';')) {
      gtkutil_message_dialog (_("Error"),
                              _("Sorry, cannot include \";\" character!"),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   return TRUE;
}


static gboolean
filter_check_duplicate (const gchar *ext, gint row, gboolean dialog)
{
   GimvEList *editlist = GIMV_ELIST (prefs_win.filter_editlist);
   gchar *text, message[BUF_SIZE];
   gint i, rows = gimv_elist_get_n_rows (editlist);

   if (!ext || !*ext) return FALSE;

   for (i = 0; i < rows; i++) {
      if (i == row) continue;

      text = gimv_elist_get_cell_text (editlist, i, 0);
      if (!text) continue;

      if (text && *text && !g_strcasecmp (ext, text)) {
         if (dialog) {
            g_snprintf (message, BUF_SIZE, _("\"%s\" is already defined."), ext);
            gtkutil_message_dialog (_("Error!!"), message,
                                    GTK_WINDOW (gimv_prefs_win_get ()));
         }
         g_free (text);
         return TRUE;
      }

      g_free (text);
   }

   return FALSE;
}


static gboolean
filter_sysdef_check_enable (gchar *ext, gchar **list)
{
   gint i;

   if (!list) return TRUE;

   for (i = 0; list[i]; i++) {
      if (list[i] && *list[i] && !strcmp (ext, list[i]))
         return FALSE;
   }

   return TRUE;
}


static void
filter_set_value (void)
{
   GimvEList *editlist = GIMV_ELIST (prefs_win.filter_editlist);
   GList *list;
   FilterRowData *rowdata;
   gchar *text[16];
   gchar **sysdef_disables = NULL, **user_defs = NULL, **sections = NULL;
   gint i, row;

   /* system defined image types */

   if (conf.imgtype_disables && *conf.imgtype_disables) {
      sysdef_disables = g_strsplit (conf.imgtype_disables, ",", -1);
      if (sysdef_disables)
         for (i = 0; sysdef_disables[i]; i++)
            if (*sysdef_disables[i])
               g_strstrip (sysdef_disables[i]);
   }

   /* list = gimv_image_get_sysdef_ext_list_all (); */
   list = gimv_mime_types_get_known_ext_list();

   for (i = 0; list; list = g_list_next (list), i++) {
      rowdata = g_new0 (FilterRowData, 1);
      rowdata->enable = filter_sysdef_check_enable (list->data, sysdef_disables);
      rowdata->user_def = FALSE;

      text[0] = list->data;
      /* text[1] = gimv_image_get_type_from_ext (text[0]); */
      text[1] = (gchar *) gimv_mime_types_get_type_from_ext (text[0]);
      text[2] = rowdata->enable ? _("Enable") :  _("Disable");
      text[3] = _("System defined");

      row = gimv_elist_append_row (editlist, text);     

      gimv_elist_set_row_data_full (editlist, row, rowdata,
                                       (GtkDestroyNotify) g_free);
   }

   if (sysdef_disables)
      g_strfreev (sysdef_disables);


   /* user defined image types */

   if (conf.imgtype_user_defs && *conf.imgtype_user_defs)
      user_defs = g_strsplit (conf.imgtype_user_defs, ";", -1);
   if (!user_defs) return;

   for (i = 0; user_defs[i]; i++) {
      if (!*user_defs[i]) continue;

      sections = g_strsplit (user_defs[i], ",", 3);
      if (!sections) continue;
      g_strstrip (sections[0]);	 
      g_strstrip (sections[1]);	 
      g_strstrip (sections[2]);	 
      if (!*sections[0]) goto ERROR;
      if (filter_check_duplicate (sections[0], -1, FALSE)) goto ERROR;

      text[0] = sections[0];

      rowdata = g_new0 (FilterRowData, 1);
      rowdata->user_def = TRUE;

      text[1] = *sections[1] ? sections[1] : "UNKNOWN";

      if (*sections[2] && !g_strcasecmp (sections[2], "ENABLE")) {
         text[2] = _("Enable");
         rowdata->enable = TRUE;
      } else {
         text[2] = _("Disable");
         rowdata->enable = FALSE;
      }

      text[3] = _("User defined");

      row = gimv_elist_append_row (editlist, text);     

      gimv_elist_set_row_data_full (editlist, row, rowdata,
                                       (GtkDestroyNotify) g_free);

   ERROR:
      g_strfreev (sections);
   }

   g_strfreev (user_defs);
}


static void
cb_filter_editlist_confirm (GimvEList *editlist,
                            GimvEListActionType type,
                            gint selected_row,
                            GimvEListConfirmFlags *flags,
                            gpointer data)
{
   const gchar *ext, *name;
   gboolean duplicate = FALSE;
   FilterRowData *rowdata;

   ext  = gtk_entry_get_text (GTK_ENTRY (prefs_win.filter_ext_entry));
   name = gtk_entry_get_text (GTK_ENTRY (prefs_win.filter_type_entry));

   gtk_widget_set_sensitive (prefs_win.filter_ext_entry, TRUE);
   gtk_widget_set_sensitive (prefs_win.filter_type_entry, TRUE);

   if (selected_row >= 0) {
      rowdata = gimv_elist_get_row_data (editlist, selected_row);
      if (rowdata && !rowdata->user_def) {
         *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
         *flags |= GIMV_ELIST_CONFIRM_CANNOT_DELETE;
         gtk_widget_set_sensitive (prefs_win.filter_ext_entry, FALSE);
         gtk_widget_set_sensitive (prefs_win.filter_type_entry, FALSE);
      }
   }

   if (!ext || !*ext || !name || !*name) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
   }

   if (type != GIMV_ELIST_ACTION_ADD && type != GIMV_ELIST_ACTION_CHANGE)
      return;

   /* check intput value */
   if (!filter_check_value (ext, name)) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
      return;
   }

   /* check duplicate */
   if (type == GIMV_ELIST_ACTION_ADD)
      duplicate = filter_check_duplicate (ext, -1, TRUE);
   else if (type == GIMV_ELIST_ACTION_CHANGE)
      duplicate = filter_check_duplicate (ext, selected_row, TRUE);

   if (duplicate) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
   }
}


static void
cb_filter_editlist_updated (GimvEList *editlist)
{
   FilterRowData *rowdata;
   gint i, rows = gimv_elist_get_n_rows (editlist);
   gchar *ext, *type, *disable_list = NULL, *user_defs = NULL, *tmpstr, *status;

   for (i = 0; i < rows; i++) {
      rowdata = gimv_elist_get_row_data (editlist, i);
      if (!rowdata) continue;

      ext = gimv_elist_get_cell_text (editlist, i, 0);
      if (!ext) continue;
      type = gimv_elist_get_cell_text (editlist, i, 1);
      if (!type) type = "UNKNOWN";

      if (!rowdata->user_def) {   /* system define */
         if (rowdata->enable) continue;
         if (!disable_list) {
            disable_list = g_strdup (ext);
         } else {
            tmpstr = disable_list;
            disable_list = g_strconcat (disable_list, ",", ext, NULL);
            g_free (tmpstr);
         }
      } else {                    /* user define */
         status = rowdata->enable ? "ENABLE" : "DISABLE";

         if (!user_defs) {
            user_defs = g_strconcat (ext, ",", type, ",", status, NULL);
         } else {
            tmpstr = user_defs;
            user_defs = g_strconcat (user_defs, "; ",
                                     ext, ",", type, ",", status, NULL);
            g_free (tmpstr);
         }
      }
   }

   if (config_changed->imgtype_disables != config_prechanged->imgtype_disables)
      g_free (config_changed->imgtype_disables);
   config_changed->imgtype_disables = disable_list;

   if (config_changed->imgtype_user_defs != config_prechanged->imgtype_user_defs)
      g_free (config_changed->imgtype_user_defs);
   config_changed->imgtype_user_defs =  user_defs;
}


static gchar *
cb_filter_deftype_get_data (GimvEList *editlist,
                            GimvEListActionType type,
                            GtkWidget *widget,
                            gpointer coldata)
{
   FilterRowData *rowdata;
   gint selected;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);

   if (type == GIMV_ELIST_ACTION_ADD) goto USER_DEF;

   selected = gimv_elist_get_selected_row (editlist);
   if (selected < 0) goto USER_DEF;

   rowdata = gimv_elist_get_row_data (editlist, selected);
   if (!rowdata) goto USER_DEF;

   if (!rowdata->user_def)
      return g_strdup (_("System defined"));

 USER_DEF:
   return g_strdup (_("User defined"));
}


static gboolean
cb_filter_get_row_data (GimvEList *editlist,
                        GimvEListActionType type,
                        gpointer *rowdata_ret,
                        GtkDestroyNotify *destroy_fn_ret)
{
   GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (prefs_win.filter_disable_toggle);
   FilterRowData *rowdata = NULL;
   gint selected = gimv_elist_get_selected_row (editlist);
   gboolean retval = FALSE;

   g_return_val_if_fail (rowdata_ret && destroy_fn_ret, FALSE);

   if (type == GIMV_ELIST_ACTION_ADD) {
      rowdata = g_new0 (FilterRowData, 1);
      rowdata->user_def = TRUE;
      *rowdata_ret    = rowdata;
      *destroy_fn_ret = (GtkDestroyNotify) g_free;
      retval = TRUE;
   } else if (type == GIMV_ELIST_ACTION_CHANGE) {
      rowdata = gimv_elist_get_row_data (editlist, selected);
      g_return_val_if_fail (rowdata, FALSE);
   } else {
      return FALSE;
   }

   rowdata->enable = !toggle->active;

   return retval;
}


static void
cb_locale_charset_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->charset_locale != config_prechanged->charset_locale)
      g_free (config_changed->charset_locale);

   config_changed->charset_locale
      = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
cb_internal_charset_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->charset_internal != config_prechanged->charset_internal)
      g_free (config_changed->charset_internal);

   config_changed->charset_internal
      = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
cb_filename_charset_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->charset_filename != config_prechanged->charset_filename)
      g_free (config_changed->charset_filename);

   config_changed->charset_filename
      = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
set_sensitive_filename_charset_mode (gint num, GtkWidget *combo)
{
   g_return_if_fail (combo);

   if (num == CHARSET_TO_LOCALE_ANY) {
      gtk_widget_set_sensitive (combo, TRUE);
   } else {
      gtk_widget_set_sensitive (combo, FALSE);
   }
}


static void
cb_filename_charset_conv (GtkWidget *widget, gpointer data)
{
   GtkWidget *combo = data;

   config_changed->charset_filename_mode
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "num"));

   set_sensitive_filename_charset_mode (config_changed->charset_filename_mode,
                                        combo);
}


/*******************************************************************************
 *  prefs_common_page:
 *     @ Create common preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_common_page (void)
{
   GtkWidget *main_vbox, *frame, *table, *vbox, *alignment;
   GtkWidget *hbox;
   GtkWidget *label;
   GtkWidget *option_menu;
   GtkWidget *toggle, *radio0, *radio1, *radio2;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);


   /**********************************************
    * Default Open Window Frame
    **********************************************/
   frame = gtk_frame_new (_("Default Open Window"));
   gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
   gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, TRUE, 0);

   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (frame), alignment);

   table = gtk_table_new (3, 2, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 5);
   gtk_container_add (GTK_CONTAINER (alignment), table);

   /* File Open Window Selection */
   label = gtk_label_new (_("File Open Window"));
   alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   option_menu = create_option_menu_simple (open_window_items,
                                            conf.default_file_open_window,
                                            &config_changed->default_file_open_window);
   gtk_table_attach (GTK_TABLE (table), option_menu, 1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 1);

   /* Directory Open Window Selection */
   label = gtk_label_new (_("Directory Open Window"));
   alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   option_menu = create_option_menu_simple (open_window_items,
                                            conf.default_dir_open_window,
                                            &config_changed->default_dir_open_window);
   gtk_table_attach (GTK_TABLE (table), option_menu, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 1);

   /* Archive Open Window Selection */
   label = gtk_label_new (_("Archive Open Window"));
   alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 2, 3,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   option_menu = create_option_menu_simple (open_window_items,
                                            conf.default_arc_open_window,
                                            &config_changed->default_arc_open_window);
   gtk_table_attach (GTK_TABLE (table), option_menu, 1, 2, 2, 3,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 1);
   gtk_widget_set_sensitive (option_menu, FALSE);


   /**********************************************
    * Scan Directory Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Directory scan"), frame, vbox, main_vbox, FALSE);

   /* Ignore filename extension or not */
   toggle = gtkutil_create_check_button (_("Scan directories recursively"),
                                         conf.scan_dir_recursive,
                                         cb_scan_sub_dir,
                                         &config_changed->scan_dir_recursive);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* follow symbolic link */
   radio0 = gtk_radio_button_new_with_label (NULL, _("Follow symbolic link"));
   gtk_box_pack_start (GTK_BOX (vbox), radio0, FALSE, FALSE, 0);

   radio1 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio0),
                                                         _("Do not follow symbolic link"));
   gtk_box_pack_start (GTK_BOX (vbox), radio1, FALSE, FALSE, 0);

   radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
                                                         _("Do not follow link to parent directory"));
   /* not implemented yet
      gtk_box_pack_start (GTK_BOX (vbox), radio2, FALSE, FALSE, 0);
      gtk_widget_set_uposition (GTK_WIDGET(radio2), 40, -1);
   */

   if (conf.recursive_follow_link && conf.recursive_follow_parent)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio0), TRUE);
   else if (conf.recursive_follow_link && !conf.recursive_follow_parent)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);
   else
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), TRUE);

   g_signal_connect (G_OBJECT (radio0), "toggled",
                     G_CALLBACK (cb_recursive_follow_symlink),
                     GINT_TO_POINTER(0));
   g_signal_connect (G_OBJECT (radio1), "toggled",
                     G_CALLBACK (cb_recursive_follow_symlink),
                     GINT_TO_POINTER(1));
   g_signal_connect (G_OBJECT (radio2), "toggled",
                     G_CALLBACK (cb_recursive_follow_symlink),
                     GINT_TO_POINTER(2));


   /********************************************** 
    * Image Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Image"), frame, vbox, main_vbox, FALSE);

   /* Interpolation type */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Interpolation type for scaling"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
   option_menu = create_option_menu_simple (interpolation_items,
                                            conf.interpolation,
                                            (gint *) &config_changed->interpolation);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);

   /********************************************** 
    * Start up Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Start up"), frame, vbox, main_vbox, FALSE);

   /* Open thumbnail window or not */
   toggle = gtkutil_create_check_button (_("Open thumbnail window"),
                                         conf.startup_open_thumbwin,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->startup_open_thumbwin);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Scan (or not) Directory */
   toggle = gtkutil_create_check_button (_("Ignore directory"),
                                         !(conf.startup_read_dir),
                                         gtkutil_get_data_from_toggle_negative_cb,
                                         &config_changed->startup_read_dir);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* No warning */
   toggle = gtkutil_create_check_button (_("No warning"),
                                         conf.startup_no_warning,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->startup_no_warning);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_filter_page:
 *     @ Create filter preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_filter_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *vbox;
   GtkWidget *editlist, *entry;
   GtkWidget *toggle, *label;
   gchar *titles[] = {_("Extension"), _("File Type"), _("Status"), NULL};
   gint titles_num = sizeof (titles) / sizeof (gchar *);

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /********************************************** 
    * Image Types Frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("File Types"), frame, frame_vbox, main_vbox, TRUE);
   gtk_widget_show (frame_vbox);
   gtk_widget_show (frame);

   editlist = gimv_elist_new_with_titles (titles_num, titles);
   prefs_win.filter_editlist = editlist;
   gimv_elist_set_reorderable (GIMV_ELIST (editlist), FALSE);
   gimv_elist_set_auto_sort (GIMV_ELIST (editlist), 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
   gtk_widget_show (editlist);

   /* entry area */
   hbox = GIMV_ELIST (editlist)->edit_area;

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
   gtk_widget_show (vbox);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   label = gtk_label_new (_("Extension: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 0, NULL, FALSE);
   prefs_win.filter_ext_entry = entry;
   gtk_widget_set_size_request (entry, 100, -1);
   gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
   gtk_widget_show (entry);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
   gtk_widget_show (vbox);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   label = gtk_label_new (_("File Type: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
   gtk_widget_show (label);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 1,
                                       "UNKNOWN", FALSE);
   prefs_win.filter_type_entry = entry;
   gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);
   gtk_widget_show (entry);

   /* disable check box */
   toggle = gimv_elist_create_check_button (GIMV_ELIST (editlist), 2,
                                            _("Disable"), FALSE,
                                            _("Disable"), _("Enable"));
   prefs_win.filter_disable_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   /* definition type column */
   gimv_elist_set_column_funcs (GIMV_ELIST (editlist),
                                NULL, 3, NULL,
                                cb_filter_deftype_get_data,
                                NULL, NULL, NULL);

   /* for row data */
   gimv_elist_set_get_row_data_func (GIMV_ELIST (editlist),
                                     cb_filter_get_row_data);

   filter_set_value ();

   g_signal_connect (G_OBJECT (editlist), "action_confirm",
                     G_CALLBACK (cb_filter_editlist_confirm),
                     NULL);
   g_signal_connect (G_OBJECT (editlist), "list_updated",
                     G_CALLBACK (cb_filter_editlist_updated),
                     NULL);


   /* Ignore filename extension or not */
   toggle = gtkutil_create_check_button (_("Read dotfiles"),
                                         conf.read_dotfile,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->read_dotfile);
   gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   /* Ignore filename extension or not */
   toggle = gtkutil_create_check_button (_("Ignore file name extension"),
                                         !(conf.detect_filetype_by_ext),
                                         gtkutil_get_data_from_toggle_negative_cb,
                                         &config_changed->detect_filetype_by_ext);
   gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_charset_page:
 *     @ Create character set preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_charset_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *table, *hbox;
   GtkWidget *label, *combo, *option_menu;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /********************************************** 
    * Common Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Common"), frame, frame_vbox, main_vbox, FALSE);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);
   table = gtk_table_new (2, 2, FALSE);
   gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, TRUE, 5);

   /* locale charset */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
   label = gtk_label_new (_("Locale character set: "));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   combo = gtk_combo_new ();
   gtk_combo_set_popdown_strings (GTK_COMBO (combo),
                                  charset_get_known_list(NULL));
   gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
                       conf.charset_locale);
   g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                     G_CALLBACK (cb_locale_charset_changed), NULL);

   /* internal charset */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
   label = gtk_label_new (_("Internal character set: "));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   combo = gtk_combo_new ();
   gtk_combo_set_popdown_strings (GTK_COMBO (combo),
                                  charset_get_known_list(NULL));
   gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
                       conf.charset_internal);
   g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                     G_CALLBACK (cb_internal_charset_changed), NULL);

   /* Language to detect */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 0);
   label = gtk_label_new (_("Language for auto detecting"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);
   option_menu = create_option_menu_simple (charset_auto_detect_labels,
                                            conf.charset_auto_detect_lang,
                                            &config_changed->charset_auto_detect_lang);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, TRUE, 5);


   /********************************************** 
    * Filename Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("File name"), frame, frame_vbox, main_vbox, FALSE);

   /* charset for filename */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 0);
   label = gtk_label_new (_("Character set of file name"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);

   /* filename charset */
   combo = gtk_combo_new ();
   gtk_widget_set_size_request (combo, 120, -1);
   gtk_combo_set_popdown_strings (GTK_COMBO (combo),
                                  charset_get_known_list(NULL));
   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
                       conf.charset_filename);
   g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                     G_CALLBACK (cb_filename_charset_changed), NULL);

   option_menu = create_option_menu (charset_to_internal_items,
                                     conf.charset_filename_mode,
                                     cb_filename_charset_conv,
                                     combo);

   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, TRUE, 5);
   gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);

   set_sensitive_filename_charset_mode (conf.charset_filename_mode, combo);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


gboolean
prefs_filter_apply (GimvPrefsWinAction action)
{
   /* gimv_image_update_image_types (); */
   gimv_mime_types_update_known_enabled_ext_list ();
   gimv_mime_types_update_userdef_ext_list       ();
   return FALSE;
}


gboolean
prefs_charset_apply (GimvPrefsWinAction action)
{
   gimv_charset_init ();
   return FALSE;
}
