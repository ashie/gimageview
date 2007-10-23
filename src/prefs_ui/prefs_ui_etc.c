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

#include "gimv_comment.h"
#include "gimv_elist.h"
#include "prefs.h"
#include "prefs_ui_etc.h"
#include "gimv_prefs_ui_utils.h"
#include "gimv_prefs_win.h"
#include "gimv_thumb_cache.h"
#include "utils.h"
#include "utils_char_code.h"
#include "utils_gtk.h"
#include "utils_menu.h"


typedef struct PrefsWin_Tag {
   /* cache */
   GtkWidget *cache_write_type;
   GtkWidget *cache_write_prefs;

   /* comment */
   GtkWidget *comment_editlist;
   GtkWidget *comment_key_entry;
   GtkWidget *comment_name_entry;
   GtkWidget *comment_status_toggle;
   GtkWidget *comment_auto_toggle;
   GtkWidget *comment_disp_toggle;

   /* slideshow */
   GtkWidget *slideshow_scale_spin;
   GtkWidget *slideshow_fit_only_zoom_out;
} PrefsWin;

static PrefsWin  prefs_win;
extern Config   *config_changed;
extern Config   *config_prechanged;

/* next two imported from prefs_ui_imagewin.c */

extern const gchar *rotate_menu_items[];
extern const gchar *zoom_menu_items[];

static gint
cb_dummy (GtkWidget *button, gpointer data)
{
   return TRUE;
}


static void
set_sensitive_cache_write (void)
{
   GtkWidget *option_menu = prefs_win.cache_write_type;
   GtkWidget *button = prefs_win.cache_write_prefs;
   GtkWidget *menu_item;
   gchar *label;

   g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

   menu_item = gtkutil_option_menu_get_current (option_menu);
   if (!menu_item) return;

   label = g_object_get_data (G_OBJECT (menu_item), "label");
   if (!label) return;

   if (gimv_thumb_cache_has_save_prefs (label)) {
      gtk_widget_set_sensitive (button, TRUE);
   } else {
      gtk_widget_set_sensitive (button, FALSE);
   }
}


static void
cb_cache_write_type (GtkWidget *widget, gchar *data)
{
   if (!data) return;

   if (config_changed->cache_write_type != config_prechanged->cache_write_type)
      g_free (config_changed->cache_write_type);
   config_changed->cache_write_type = g_strdup (data);

   set_sensitive_cache_write ();
}


static void
cb_cache_write_prefs_ok (GtkWidget *widget)
{
   g_return_if_fail (GTK_IS_WINDOW (widget));
   gtk_main_quit ();
}


static void
cb_cache_write_prefs_button_pressed (GtkWidget *option_menu)
{
   GtkWidget *dialog, *widget, *button, *menu_item;
   gchar *label;

   g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

   menu_item = GTK_OPTION_MENU (option_menu)->menu_item;
   if (!menu_item) return;

   label = g_object_get_data (G_OBJECT (menu_item), "label");
   if (!label) return;

   widget = gimv_thumb_cache_get_save_prefs (label, NULL);
   if (!widget) return;

   /* create dialog window */
   dialog = gtk_dialog_new ();
   gtk_window_set_title (GTK_WINDOW (dialog), _("Preference - Cache Writing -")); 
   gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
   g_signal_connect (G_OBJECT (dialog), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
                       widget, TRUE, TRUE, 0);

   /* ok buttons */
   button = gtk_button_new_with_label (_("OK"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_cache_write_prefs_ok),
                     dialog);

   gtk_widget_show_all (dialog);

   gtk_grab_add (dialog);
   gtk_main ();
   gtk_grab_remove (dialog);
   gtk_widget_destroy (dialog);
}


static gboolean
check_value (const gchar *key, const gchar *name)
{
   gchar message[BUF_SIZE];

   if (!string_is_ascii_graphable (key)) {
      gtkutil_message_dialog (_("Error"),
                              _("Key string includes invalid character!\n"
                                "Only ASCII (except space) is available."),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   if (strchr(key, ',') || strchr(name, ',')) {
      gtkutil_message_dialog (_("Error"),
                              _("Sorry, cannot include \",\" character!"),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   if (strchr(key, ';') || strchr(name, ';')) {
      gtkutil_message_dialog (_("Error"),
                              _("Sorry, cannot include \";\" character!"),
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   if (!key || !*key) {
      g_snprintf (message, BUF_SIZE, _("\"Key name\" must be defined."));
      gtkutil_message_dialog (_("Error!!"), message,
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }
   if (!name || !*name) {
      g_snprintf (message, BUF_SIZE, _("\"Display name\" must be defined."));
      gtkutil_message_dialog (_("Error!!"), message,
                              GTK_WINDOW (gimv_prefs_win_get ()));
      return FALSE;
   }

   return TRUE;
}


static gboolean
check_duplicate_comment_key (const gchar *key, gint this_row)
{
   GimvEList *editlist = GIMV_ELIST (prefs_win.comment_editlist);
   gint row, rows = editlist->rows;

   g_return_val_if_fail (key && *key, FALSE);

   for (row = 0; row < rows; row++) {
      GimvCommentDataEntry *entry;

      if (row == this_row) continue;

      entry = gimv_elist_get_row_data (editlist, row);
      if (!entry) continue;

      if (!strcmp (entry->key, key)) {
         /* FIXME!! add error handling */
         return TRUE;
      }
   }

   return FALSE;
}


static void
set_default_comment_key_list (void)
{
   GimvEList *editlist = GIMV_ELIST (prefs_win.comment_editlist);
   GList *list;
   gchar *text[16];
   gint row;

   for (list = gimv_comment_get_data_entry_list ();
        list;
        list = g_list_next (list))
   {
      GimvCommentDataEntry *dentry = list->data, *rowdata;

      if (!dentry) continue;

      rowdata = gimv_comment_data_entry_dup (dentry);

      text[0] = dentry->key;
      text[1] = _(dentry->display_name);
      text[2] = _(boolean_to_text (dentry->enable));
      text[3] = _(boolean_to_text (dentry->auto_val));
      text[4] = _(boolean_to_text (dentry->display));
      if (dentry->userdef)
         text[5] = _("User defined");
      else
         text[5] = _("System defined");
      row = gimv_elist_append_row (editlist, text);

      if (row >= 0) {
         gimv_elist_set_row_data_full (editlist, row, rowdata,
                                       (GtkDestroyNotify) gimv_comment_data_entry_delete);
      } else {
         gimv_comment_data_entry_delete (rowdata);
      }
   }
}


static void
cb_comment_editlist_updated (GimvEList *editlist)
{
   gchar *string, *tmp;
   gint row, rows = gimv_elist_get_n_rows (editlist);

   g_return_if_fail (GIMV_IS_ELIST (editlist));

   if (config_changed->comment_key_list != config_prechanged->comment_key_list)
      g_free (config_changed->comment_key_list);
   config_changed->comment_key_list = NULL;

   string = g_strdup ("");

   for (row = 0; row < rows; row++) {
      GimvCommentDataEntry *entry;
      gchar *dname_internal;

      entry = gimv_elist_get_row_data (editlist, row);     
      if (!entry) continue;
      if (!entry->key || !*entry->key) continue;
      if (!entry->display_name || !*entry->display_name) continue;

      tmp = string;
      if (*string) {
         string = g_strconcat (string, "; ", NULL);
         g_free (tmp);
         tmp = string;
      }
      dname_internal = charset_internal_to_locale (entry->display_name);
      string = g_strconcat (string, entry->key, ",", dname_internal,
                            ",", boolean_to_text (entry->enable),
                            ",", boolean_to_text (entry->auto_val),
                            ",", boolean_to_text (entry->display),
                            NULL);
      g_free (dname_internal);
      g_free (tmp);
      tmp = NULL;
   }

   config_changed->comment_key_list = string;
}


static void
cb_comment_editlist_confirm (GimvEList *editlist,
                             GimvEListActionType type,
                             gint selected_row,
                             GimvEListConfirmFlags *flags,
                             gpointer data)
{
   GtkWidget *t_auto = prefs_win.comment_auto_toggle;
   GtkWidget *key_entry  = prefs_win.comment_key_entry;
   GtkWidget *name_entry = prefs_win.comment_name_entry;
   GimvCommentDataEntry *entry = NULL;
   const gchar *key, *name;
   gchar message[BUF_SIZE];
   gboolean duplicate = FALSE;

   key  = gtk_entry_get_text (GTK_ENTRY (key_entry));
   name = gtk_entry_get_text (GTK_ENTRY (name_entry));

   gtk_widget_set_sensitive (key_entry,  TRUE);
   gtk_widget_set_sensitive (name_entry, TRUE);
   gtk_widget_set_sensitive (t_auto,     TRUE);

   if (selected_row < 0) {
      gtk_widget_set_sensitive (t_auto, FALSE);
   } else {
      entry = gimv_elist_get_row_data (editlist, selected_row);
      if (entry) {
         gtk_widget_set_sensitive (key_entry,  entry->userdef);
         gtk_widget_set_sensitive (name_entry, entry->userdef);

         if (!entry->def_val_fn)
            gtk_widget_set_sensitive (t_auto, FALSE);

         if (!entry->userdef) {
            *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
            *flags |= GIMV_ELIST_CONFIRM_CANNOT_DELETE;
         }
      }
   }

   if (!key || !*key || !name || !*name) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
   }

   if ((!key || !*key) && (!name || !*name) && selected_row < 0)
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_NEW;


   if (type != GIMV_ELIST_ACTION_ADD && type != GIMV_ELIST_ACTION_CHANGE)
      return;

   if (!check_value (key, name)) {
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
      g_signal_stop_emission_by_name (G_OBJECT (editlist), "action_confirm");
      return;
   }


#if 0
   if ((type == GIMV_ELIST_ACTION_CHANGE) && entry && !entry->userdef) {
      if (strcmp (entry->key, key)) {
         /* FIXME!! add error handling */
         *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
         return;
      }
      if (strcmp (_(entry->display_name), name)) {
         /* FIXME!! add error handling */
         *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
         return;
      }
   }
#endif


   if (type == GIMV_ELIST_ACTION_ADD)
      duplicate = check_duplicate_comment_key (key, -1);
   else if ((type == GIMV_ELIST_ACTION_CHANGE) && entry && entry->userdef)
      duplicate = check_duplicate_comment_key (key, selected_row);

   if (duplicate) {
      g_snprintf (message, BUF_SIZE, _("\"%s\" is already defined."), key);
      gtkutil_message_dialog (_("Error!!"), message,
                              GTK_WINDOW (gimv_prefs_win_get ()));
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_ADD;
      *flags |= GIMV_ELIST_CONFIRM_CANNOT_CHANGE;
      g_signal_stop_emission_by_name (G_OBJECT (editlist), "action_confirm");
   }
}


static gchar *
cb_editlist_deftype_get_data (GimvEList *editlist,
                              GimvEListActionType type,
                              GtkWidget *widget,
                              gpointer coldata)
{
   GimvCommentDataEntry *entry;
   gint selected;

   g_return_val_if_fail (GIMV_IS_ELIST (editlist), NULL);

   if (type == GIMV_ELIST_ACTION_ADD) goto USER_DEF;

   selected = gimv_elist_get_selected_row (editlist);
   if (selected < 0) goto USER_DEF;

   entry = gimv_elist_get_row_data (editlist, selected);
   if (!entry) goto USER_DEF;

   if (!entry->userdef)
      return g_strdup (_("System defined"));

 USER_DEF:
   return g_strdup (_("User defined"));
}


static gboolean
cb_editlist_get_row_data (GimvEList *editlist,
                          GimvEListActionType type,
                          gpointer *rowdata,
                          GtkDestroyNotify *destroy_fn)
{
   GtkEntry *entry1 = GTK_ENTRY (prefs_win.comment_key_entry);
   GtkEntry *entry2 = GTK_ENTRY (prefs_win.comment_name_entry);
   GtkToggleButton *t_ins  = GTK_TOGGLE_BUTTON (prefs_win.comment_status_toggle);
   GtkToggleButton *t_auto = GTK_TOGGLE_BUTTON (prefs_win.comment_auto_toggle);
   GtkToggleButton *t_disp = GTK_TOGGLE_BUTTON (prefs_win.comment_disp_toggle);
   GimvCommentDataEntry *entry;
   gint row = gimv_elist_get_selected_row (editlist);
   const gchar *key, *name;

   g_return_val_if_fail (rowdata && destroy_fn, FALSE);

   key  = gtk_entry_get_text (entry1);
   name = gtk_entry_get_text (entry2);

   if (type == GIMV_ELIST_ACTION_ADD) {
      entry = g_new0 (GimvCommentDataEntry, 1);
      entry->key          = NULL;
      entry->display_name = NULL;
      entry->value        = NULL;
      entry->userdef      = TRUE;
   } else if (type == GIMV_ELIST_ACTION_CHANGE) {
      entry = gimv_elist_get_row_data (editlist, row);
      g_return_val_if_fail (entry, FALSE);
   } else {
      return FALSE;
   }

   if (!entry->userdef) {
      if (!entry->def_val_fn)
         entry->auto_val = FALSE;
      else
         entry->auto_val = t_auto->active;
   } else {
      g_free (entry->key);
      g_free (entry->display_name);

      entry->key          = g_strdup (key);
      entry->display_name = g_strdup (name);
      entry->auto_val     = FALSE;
   }

   entry->enable  = t_ins->active;
   entry->display = t_disp->active;

   if (type == GIMV_ELIST_ACTION_ADD) {
      g_return_val_if_fail (rowdata && destroy_fn, FALSE);
      *rowdata = entry;
      *destroy_fn = (GtkDestroyNotify) gimv_comment_data_entry_delete;
      return TRUE;
   } else {
      return FALSE;
   }
}


static void
cb_comment_charset_changed (GtkEditable *entry, gpointer data)
{
   if (config_changed->comment_charset != config_prechanged->comment_charset)
      g_free (config_changed->comment_charset);

   config_changed->comment_charset
      = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
cb_zoom_menu (GtkWidget *menu)
{
   config_changed->slideshow_zoom =
      GPOINTER_TO_INT (g_object_get_data(G_OBJECT(menu), "num"));

   gtk_widget_set_sensitive (prefs_win.slideshow_scale_spin,
                             config_changed->slideshow_zoom == 0 ||
                             config_changed->slideshow_zoom == 1);
}



/*******************************************************************************
 *  prefs_cache_page:
 *     @ Create cache preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_cache_page (void)
{
   GtkWidget *main_vbox, *frame, *vbox, *hbox;
   GtkWidget *label, *button1, *menu, *menu_item, *option_menu;
   GList *list, *node;
   gint i, item = 0;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   list = gimv_thumb_cache_get_loader_list ();
   frame = gimv_prefs_ui_double_clist (_("Cache reading"),
                                       _("Available cache type"),
                                       _("Cache type to use"),
                                       list,
                                       config_prechanged->cache_read_list,
                                       &config_changed->cache_read_list,
                                       ',');
   g_list_free (list);
   gtk_box_pack_start(GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);


   /**********************************************
    *  Cache writing frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Cache writing"), frame, vbox, main_vbox, FALSE);

   /* Cache write type menu */
   list = node = gimv_thumb_cache_get_saver_list ();
   /* list = node = g_list_prepend (list, "NONE"); */

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   label = gtk_label_new (_("Cache type for save"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
   option_menu = prefs_win.cache_write_type = gtk_option_menu_new();
   menu = gtk_menu_new();
   for (i = 0; node; i++, node = g_list_next (node)) {
      gchar *text = node->data;

      if (!strcmp(text, conf.cache_write_type)) item = i;
      menu_item = gtk_menu_item_new_with_label (_(text));
      g_object_set_data (G_OBJECT (menu_item), "label", text);
      g_signal_connect(G_OBJECT(menu_item), "activate",
                       G_CALLBACK(cb_cache_write_type), text);
      gtk_menu_append (GTK_MENU(menu), menu_item);
      gtk_widget_show (menu_item);
   }
   gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
   gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
                                item);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);

   button1 = gtk_button_new_with_label (_("Preference"));
   prefs_win.cache_write_prefs = button1;
   gtk_box_pack_start (GTK_BOX (hbox), button1, FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (button1),"clicked",
                     G_CALLBACK (cb_cache_write_prefs_button_pressed),
                     option_menu);

   g_list_free (list);

   set_sensitive_cache_write ();

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_comment_page:
 *     @ Create comment preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_comment_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *hbox1, *vbox;
   GtkWidget *editlist, *entry, *toggle, *label, *combo;
   gchar *titles[] = {
      _("Key Name"),
      _("Displayed Name"),
      _("Status"),
      _("Auto"),
      _("Display"),
      NULL
   };
   gint titles_num = sizeof (titles) / sizeof (gchar *);

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /********************************************** 
    * Key Name definition frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Key Name Definition"),
                         frame, frame_vbox, main_vbox, TRUE);
   gtk_widget_show (frame_vbox);
   gtk_widget_show (frame);

   editlist = gimv_elist_new_with_titles (titles_num, titles);
   prefs_win.comment_editlist = editlist;
   gimv_elist_set_reorderable (GIMV_ELIST (editlist), FALSE);
   gtk_box_pack_start (GTK_BOX (frame_vbox), editlist, TRUE, TRUE, 0);
   gtk_widget_show (editlist);


   /*
    *  create edit area
    */
   hbox = GIMV_ELIST (editlist)->edit_area;

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
   gtk_widget_show (vbox);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   label = gtk_label_new (_("Key Name: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 0, NULL, FALSE);
   prefs_win.comment_key_entry = entry;
   gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
   gtk_widget_show (entry);

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
   gtk_widget_show (vbox);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   label = gtk_label_new (_("Displayed Name: "));
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
   gtk_widget_show (label);
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);
   gtk_widget_show (hbox1);

   entry = gimv_elist_create_entry (GIMV_ELIST (editlist), 1, NULL, FALSE);
   prefs_win.comment_name_entry = entry;
   gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);
   gtk_widget_show (entry);

   /* "insert" check box */
   toggle = gimv_elist_create_check_button (GIMV_ELIST (editlist), 2,
                                            _("Enable"), TRUE,
                                            _("TRUE"), _("FALSE"));
   prefs_win.comment_status_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   /* "Auto" check box */
   toggle = gimv_elist_create_check_button (GIMV_ELIST (editlist), 3,
                                            _("Auto"), FALSE,
                                            _("TRUE"), _("FALSE"));
   prefs_win.comment_auto_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   /* "display" check box */
   toggle = gimv_elist_create_check_button (GIMV_ELIST (editlist), 4,
                                            _("Display"), TRUE,
                                            _("TRUE"), _("FALSE"));
   prefs_win.comment_disp_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox1), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   /* definition type column */
   gimv_elist_set_column_funcs (GIMV_ELIST (editlist),
                                   NULL, 5, NULL,
                                   cb_editlist_deftype_get_data,
                                   NULL, NULL, NULL);

   /* for row data */
   gimv_elist_set_get_row_data_func (GIMV_ELIST (editlist),
                                     cb_editlist_get_row_data);

   set_default_comment_key_list ();
   g_signal_connect (G_OBJECT (editlist), "action_confirm",
                     G_CALLBACK (cb_comment_editlist_confirm),
                     NULL);
   g_signal_connect (G_OBJECT (editlist), "list_updated",
                     G_CALLBACK (cb_comment_editlist_updated),
                     NULL);


   /********************************************** 
    * Charset Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Character set"),
                          frame, frame_vbox, main_vbox, FALSE);

   /* locale charset */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, TRUE, 5);
   label = gtk_label_new (_("Character set: "));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 5);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   combo = gtk_combo_new ();
   gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 5);
   gtk_combo_set_popdown_strings (GTK_COMBO (combo),
                                  charset_get_known_list(NULL));
   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
                       conf.comment_charset);
   g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "changed",
                     G_CALLBACK (cb_comment_charset_changed), NULL);
   gtk_widget_show_all (frame);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_search_page:
 *     @ Create search preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_search_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox, *vbox;
   GtkWidget *label, *spinner, *toggle;
   GtkAdjustment *adj;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /**********************************************
    *  Find duplicates
    **********************************************/
   gimv_prefs_ui_create_frame(_("Find duplicates"),
                         frame, frame_vbox, main_vbox, FALSE);

   /* Accuracy Spinner */
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Accuracy"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.search_similar_accuracy,
                                               0.0, 1.0, 0.01, 0.1, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinner), 2);
   gtk_widget_set_size_request(spinner, 50, -1);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_float_cb),
                     &config_changed->search_similar_accuracy);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   /**********************************************
    *  Behabior of selecting
    **********************************************/
   gimv_prefs_ui_create_frame(_("Behabior when select thumbnail on result window"),
                         frame, frame_vbox, main_vbox, FALSE);

   vbox = gtk_vbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
   gtk_box_pack_start (GTK_BOX (frame_vbox), vbox, FALSE, FALSE, 0);

   toggle = gtkutil_create_check_button (_("Select the thumbnail on thumbnail view"),
                                         conf.simwin_sel_thumbview,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->simwin_sel_thumbview);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   toggle = gtkutil_create_check_button (_("Show the image in preview"),
                                         conf.simwin_sel_preview,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->simwin_sel_preview);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   toggle = gtkutil_create_check_button (_("Show the image in new window"),
                                         conf.simwin_sel_new_win,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->simwin_sel_new_win);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   toggle = gtkutil_create_check_button (_("Show the image in shared window"),
                                         conf.simwin_sel_shared_win,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->simwin_sel_shared_win);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_slideshow_page:
 *     @ Create slide show preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_slideshow_page (void)
{
   GtkWidget *main_vbox, *frame;
   GtkWidget *hbox, *vbox, *table, *alignment;
   GtkWidget *label, *option_menu;
   GtkWidget *spinner, *toggle, *button;
   GtkAdjustment *adj;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   gimv_prefs_ui_create_frame(NULL, frame, vbox, main_vbox, FALSE);

   /* Default Image Scale Spinner */
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Image change interval"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.slideshow_interval,
                                               0.0, 7200.0, 0.01, 0.1, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 70, -1);
   gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinner), 2);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_float_cb),
                     &config_changed->slideshow_interval);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   label = gtk_label_new (_("[sec]"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

   /* repeat */
   toggle = gtkutil_create_check_button (_("Repeat slide show"),
                                         conf.slideshow_repeat,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->slideshow_repeat);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

#if 0
   /********************************************** 
    * Show/Hide Frame
    **********************************************/
   prefs_create_frame(_("Show/Hide"), frame, vbox, main_vbox);

   /* Show Menubar or not */
   toggle = create_check_button (_("Show menubar"), conf.slideshow_menubar,
                                 cb_get_data_from_toggle,
                                 &config_changed->slideshow_menubar);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Show Toolbar or not */
   toggle = create_check_button (_("Show toolbar"), conf.slideshow_toolbar,
                                 cb_get_data_from_toggle,
                                 &config_changed->slideshow_toolbar);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Show Player toolbar or not */
   toggle = create_check_button (_("Show player toolbar"), conf.slideshow_player,
                                 cb_get_data_from_toggle,
                                 &config_changed->slideshow_player);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Show Statusbar or not */
   toggle = create_check_button (_("Show statusbar"), conf.slideshow_statusbar,
                                 cb_get_data_from_toggle,
                                 &config_changed->slideshow_statusbar);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
#endif

   /********************************************** 
    * Image Frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Image"), frame, vbox, main_vbox, FALSE);

   alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
   gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

   table = gtk_table_new (2, 2, FALSE);
   gtk_container_add (GTK_CONTAINER (alignment), table);

   /* Zoom menu */
   label = gtk_label_new (_("Zoom:"));
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);

   option_menu = create_option_menu (zoom_menu_items,
                                     conf.slideshow_zoom,
                                     cb_zoom_menu, NULL);
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), option_menu);
   gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);

   /* Rotate menu */
   label = gtk_label_new (_("Rotation:"));
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);

   option_menu = create_option_menu_simple (rotate_menu_items,
                                            conf.slideshow_rotation,
                                            &config_changed->slideshow_rotation);
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), option_menu);
   gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);

   /* Keep Aspect Ratio */
   toggle = gtkutil_create_check_button (_("Keep aspect rario"),
                                         conf.slideshow_keep_aspect,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->slideshow_keep_aspect);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Default Image Scale Spinner */
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Default Image Scale"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.slideshow_img_scale,
                                               1.0, 10000.0, 1.0, 5.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 50, -1);
   prefs_win.slideshow_scale_spin = spinner;
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_int_cb),
                     &config_changed->slideshow_img_scale);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
   label = gtk_label_new (_("%"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_set_sensitive (prefs_win.slideshow_scale_spin,
                             conf.slideshow_zoom == 0 ||
                             conf.slideshow_zoom == 1);


   /********************************************** 
    * Back Ground Frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Back Ground"), frame, vbox, main_vbox, FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   toggle = gtkutil_create_check_button (_("Use specified color"),
                                         conf.slideshow_set_bg,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->slideshow_set_bg);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

   button = gtkutil_color_sel_button (_("Choose Color"),
                                      config_changed->slideshow_bg_color);
   gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

   /* show all */
   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_dnd_page:
 *     @ Create Drag and Drop preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_dnd_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *vbox;
   GtkWidget *toggle;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /**********************************************
    *  File Operation frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("File operation"), frame, vbox, main_vbox, FALSE);

   /* Drag and Drop to external proccess */
   toggle = gtkutil_create_check_button (_("Enable DnD to external proccess (Experimental)"),
                                         conf.dnd_enable_to_external,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dnd_enable_to_external);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Drag and Drop from external proccess */
   toggle = gtkutil_create_check_button (_("Enable DnD from extenal proccess (Experimental)"),
                                         conf.dnd_enable_from_external,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dnd_enable_from_external);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Always refresh list when DnD end */
   toggle = gtkutil_create_check_button (_("Always refresh list when DnD end"),
                                         conf.dnd_refresh_list_always,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dnd_refresh_list_always);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


gboolean
prefs_comment_apply (GimvPrefsWinAction action)
{
   gimv_comment_update_data_entry_list ();
   return FALSE;
}
