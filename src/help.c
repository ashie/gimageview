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

#include "gimageview.h"

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <gdk-pixbuf/gdk-pixbuf-features.h>

#include "gimv_icon_stock.h"
#include "gimv_plugin.h"
#include "gimv_text_win.h"
#include "prefs.h"
#include "help.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_file_gtk.h"
#include "utils_menu.h"

#define DOC_HTML_DIR "html"
#define DOC_TEXT_DIR "text"
#define GIMV_MANUAL_FILE "index.html"

typedef struct GimvInfoWin_Tag
{
   GtkWidget *window;
   GtkWidget *scrolled_win;
   GtkWidget *text_box;
} GimvInfoWin;


/* callback functions */
static void cb_open_manual             (gpointer   data,
                                        guint      action,
                                        GtkWidget *widget);
static void cb_open_text               (GtkWidget *menuitem,
                                        gchar     *filename);
static void cb_gimv_info_win_ok_button (GtkWidget *widget,
                                        GtkWidget *window);
static void cb_open_info               (gpointer   data,
                                        guint      action,
                                        GtkWidget *widget);

/* other private functions */
static gchar       *get_doc_dir_name     (const gchar *lang,
                                          const gchar *type);
static GtkWidget   *get_dirlist_sub_menu (const gchar *dir,
                                          gpointer     func,
                                          GList      **filelist);


static GimvInfoWin info_win;

static gchar *license = 
N_("Copyright (C) 2001 %s <%s>\n\n"
   "This program is free software; you can redistribute it and/or modify\n"
   "it under the terms of the GNU General Public License as published by\n"
   "the Free Software Foundation; either version 2, or (at your option)\n"
   "any later version.\n\n"

   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
   "See the GNU General Public License for more details.\n\n"

   "You should have received a copy of the GNU General Public License\n"
   "along with this program; if not, write to the Free Software\n"
   "Foundation, Inc., 59 Temple Place - Suite 330, Boston,\n"
   "MA 02111-1307, USA.");

static gchar *authors = 
N_("Main Program :\n"
   "    Takuro Ashie <ashie@homa.ne.jp>\n"
   "Document :\n"
   "    Nyan2 <t-nyan2@nifty.com>\n"
   "Logo :\n"
   "    eins <eins@milk.freemail.ne.jp>\n"
   "Translate :\n"
   "\n"
   "Special Thanks :\n"
   "    horam\n"
   "    TAM\n"
   "    Hiroyuki Komatsu\n"
   "    Kazuki Iwamoto\n"
   "    katsu\n"
   "    kourin\n"
   "    jissama\n"
   "    shitamori\n"
   "    knee\n"
   "    matsu\n"
   "    Shlomi Fish\n"
   "    Jin Suh\n"
   "    sheepman\n"
   "    MINAMI Hirokazu\n"
   "    Brent Baccala\n"
   "    Christian Hammers\n"
   "    And all GImageView users");

static gchar *system_info = NULL;

static gchar *plugin_info = NULL;

GtkItemFactoryEntry gimvhelp_menu_items[] =
{
   {N_("/_Manual"),               NULL, cb_open_manual, 0, NULL},
   {N_("/_Document"),             NULL, NULL,           0, "<Branch>"},
   {N_("/_Document/_HTML"),       NULL, NULL,           0, "<Branch>"},
   {N_("/_Document/Plain _Text"), NULL, NULL,           0, "<Branch>"},
   {N_("/_About"),                NULL, cb_open_info,   0, NULL},
   {NULL, NULL, NULL, 0, NULL},
};


GList *html_filelist = NULL;
GList *text_filelist = NULL;


/******************************************************************************
 *
 *   Callback functions for menubar.
 *
 ******************************************************************************/
static void
cb_open_manual(gpointer data, guint action, GtkWidget *widget)
{
   gchar *dir, manual[MAX_PATH_LEN], *cmd, buf[BUF_SIZE];
   dir = get_doc_dir_name (NULL, DOC_HTML_DIR);
   g_snprintf (manual, MAX_PATH_LEN, "%s/%s", dir, GIMV_MANUAL_FILE);
   if (file_exists(manual)) {
      g_snprintf (buf, BUF_SIZE, conf.web_browser, manual);
      cmd = g_strconcat (buf, " &", NULL);
      system (cmd);
      g_free (cmd);
   }
   g_free (dir);
}


static void
cb_open_text (GtkWidget *menuitem, gchar *filename)
{
   gchar *cmd = NULL;

   g_return_if_fail (filename);

   if (!conf.text_viewer_use_internal && conf.text_viewer) {
      cmd = g_strconcat (conf.text_viewer, " ", filename, " &", NULL);
      system (cmd);
   } else {
      GtkWidget *text_win = gimv_text_win_new ();
      gimv_text_win_load_file (GIMV_TEXT_WIN (text_win), filename);
      gtk_widget_show (text_win);
   }

   g_free (cmd);
}


static void
cb_open_html (GtkWidget *menuitem, gchar *filename)
{
   gchar buf[BUF_SIZE], *cmd;

   g_return_if_fail (filename);

   if (conf.web_browser && *conf.web_browser) {
      g_snprintf (buf, BUF_SIZE, conf.web_browser, filename);
      cmd = g_strconcat (buf, " &", NULL);
      system (cmd);
      g_free (cmd);
   }
}


static void
cb_progurl_clicked (GtkWidget *widget, gchar *url)
{
   gchar buf[BUF_SIZE], *cmd;

   g_return_if_fail (url);

   if (conf.web_browser && *conf.web_browser) {
      g_snprintf (buf, BUF_SIZE, conf.web_browser, url);
      cmd = g_strconcat (buf, " &", NULL);
      system (cmd);
      g_free (cmd);
   }
}


static void
set_copyleft_str (void)
{
   GtkTextBuffer *buffer;
   gchar buf[BUF_SIZE];

   g_snprintf (buf, BUF_SIZE, _(license),
               GIMV_PROG_AUTHOR, GIMV_PROG_ADDRESS);

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (info_win.text_box));
   gtk_text_buffer_set_text (buffer, "\0", -1);

   if (buf && *buf)
      gtk_text_buffer_set_text (buffer, buf, -1);
}


static void
cb_gimv_info_change_text (GtkWidget *widget, gchar *text)
{
   GtkTextBuffer *buffer;

   g_return_if_fail (info_win.text_box);

   if (!text) {
      set_copyleft_str ();
      return;
   }

   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (info_win.text_box));
   gtk_text_buffer_set_text (buffer, "\0", -1);

   if (text && *text)
         gtk_text_buffer_set_text (buffer, text, -1);
}


static void
cb_gimv_info_win_ok_button (GtkWidget *widget, GtkWidget *window)
{
   gtk_widget_destroy (window);
}


static void
cb_open_info (gpointer data, guint action, GtkWidget *widget)
{
   gimvhelp_open_info_window ();
}


/******************************************************************************
 *
 *   Private functions.
 *
 ******************************************************************************/
static gchar *
get_doc_dir_name (const gchar *lang, const gchar *type)
{
   gchar *dir, *lang_fallback, *tmp;

   g_return_val_if_fail (type && *type, NULL);

   if (!lang)
      lang = get_lang ();

   dir = g_strconcat (DOCDIR, "/", type, "/", lang, NULL);

   if (isdir (dir)) return dir;

   lang_fallback = g_strdup (lang);
   tmp = strchr (lang_fallback, '.');
   if (tmp) {
      *tmp = '\0';
      g_free (dir);
      dir = g_strconcat (DOCDIR, "/", type, "/", lang_fallback, NULL);
      g_free (lang_fallback);
      if (isdir (dir)) {
         return dir;
      }
   } else {
      g_free (lang_fallback);
   }

   if (strlen (lang) > 2 && lang[2] == '_') {
      lang_fallback = g_strdup (lang);
      lang_fallback[2] = '\0';
      g_free (dir);
      dir = g_strconcat (DOCDIR, "/", type, "/", lang_fallback, NULL);
      g_free (lang_fallback);
      if (isdir (dir)) {
         return dir;
      }
   }

   return NULL;
}


static GtkWidget *
get_dirlist_sub_menu (const gchar *dir, gpointer func, GList **filelist)
{
   GtkWidget *menu = NULL, *menuitem;
   GList *node;

   g_return_val_if_fail (filelist, NULL);

   if (!dir) return NULL;;

   menu = gtk_menu_new();

   if (!*filelist)
      get_dir (dir, GETDIR_FOLLOW_SYMLINK, filelist, NULL);

   node = *filelist;
   while (node) {
      gchar *filename = node->data;

      node = g_list_next (node);

      if (!filename) continue;

      menuitem = gtk_menu_item_new_with_label (g_basename(filename));
      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (func), filename);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);

   }

   return menu;
}


static void
help_plugin_info_append (GList *list)
{
   GList *node;
   gchar buf[BUF_SIZE];

   /* g_return_if_fail (list); */

   if (!plugin_info)
      plugin_info = g_strdup ("");

   if (!list) return;

   node = list;
   while (node) {
      GModule *module = node->data;
      gchar *tmpstr;
      g_snprintf (buf, BUF_SIZE,
                  _("Plugin Name : %s\n"
                    "Version : %s\n"
                    "Author : %s\n\n"),
                  gimv_plugin_get_name (module),
                  gimv_plugin_get_version_string (module),
                  gimv_plugin_get_author (module));
      tmpstr = plugin_info;
      plugin_info = g_strconcat (plugin_info, buf, NULL);
      g_free (tmpstr);

      node = g_list_next (node);
   }
}


/******************************************************************************
 *
 *   Public functions.
 *
 ******************************************************************************/
GtkWidget *
gimvhelp_create_menu (GtkWidget *window)
{
   GtkWidget *menu = NULL, *menuitem = NULL, *html_submenu, *text_submenu;
   GtkItemFactory *ifactory;
   guint n_menu_items;
   gchar *dir, manual[MAX_PATH_LEN];

   n_menu_items = sizeof (gimvhelp_menu_items)
      / sizeof (gimvhelp_menu_items[0]) - 1;
   menu = menu_create_items(window, gimvhelp_menu_items,
                            n_menu_items, "<HelpSubMenu>", NULL);

   dir = get_doc_dir_name (NULL, DOC_HTML_DIR);
   html_submenu = get_dirlist_sub_menu (dir, (gpointer) cb_open_html,
                                        &html_filelist);
   if (html_submenu)
      menu_set_submenu (menu, "/Document/HTML", html_submenu);
   g_free (dir);
   dir = NULL;

   dir = get_doc_dir_name (NULL, DOC_TEXT_DIR);
   text_submenu = get_dirlist_sub_menu (dir, (gpointer) cb_open_text,
                                        &text_filelist);
   if (text_submenu)
      menu_set_submenu (menu, "/Document/Plain Text", text_submenu);
   g_free (dir);
   dir = NULL;

   dir = get_doc_dir_name (NULL, DOC_HTML_DIR);
   g_snprintf (manual, MAX_PATH_LEN, "%s/%s", dir, GIMV_MANUAL_FILE);
   g_free (dir);
   if (!file_exists(manual)) {
      ifactory = gtk_item_factory_from_widget (menu);
      menuitem  = gtk_item_factory_get_item (ifactory, "/Manual");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   return menu;
}


GtkWidget *
gimvhelp_create_info_widget (void)
{
   GtkWidget *vbox, *hbox1, *hbox2, *tmpvbox;
   GtkWidget *label;
   GtkWidget *button, *radio;
   GtkWidget *scrolledwin, *text;
   GtkWidget *frame, *frame_vbox;
   GtkWidget *logo;
   gchar buf[BUF_SIZE] /* , alt_string[BUF_SIZE] */;
   struct utsname utsbuf;

   /* create info string */
   if (!system_info) {
      uname(&utsbuf);
      g_snprintf (buf, BUF_SIZE,
                  _("Operating System : %s %s %s\n"
                    "GTK+ version : %d.%d.%d\n"
                    /* "libpng version : %s\n" */),
                  utsbuf.sysname, utsbuf.release, utsbuf.machine,
                  gtk_major_version, gtk_minor_version, gtk_micro_version
                  /*, png_get_header_ver (NULL)*/);
#if 0
#ifdef ENABLE_MNG
      g_snprintf (alt_string, sizeof (alt_string) / sizeof (gchar),
                  _("libmng version : %s\n"),
                  mng_version_text ());
      strncat (buf, alt_string, BUF_SIZE - strlen (buf));
#endif
      g_snprintf (alt_string, sizeof (alt_string) / sizeof (gchar),
                  _("gdk-pixbuf version : %s\n"),
                  gdk_pixbuf_version);
      strncat (buf, alt_string, BUF_SIZE - strlen (buf));
#ifdef ENABLE_SVG
      g_snprintf (alt_string, sizeof (alt_string) / sizeof (gchar),
                  _("librsvg version : %s\n"),
                  librsvg_version);
      strncat (buf, alt_string, BUF_SIZE - strlen (buf));
#endif
#ifdef ENABLE_XINE
      /*
      g_snprintf (alt_string, sizeof (alt_string) / sizeof (gchar),
                  _("Xine version : %s\n"),
                  xine_get_str_version());
      strncat (buf, alt_string, BUF_SIZE - strlen (buf));
      */
#endif
#endif
      system_info = g_strdup (buf);
   }

   if (!plugin_info) {
#if 0
      gint idx;
      const gchar *type;

      for (idx = 0; (type = gimv_plugin_type_get (idx)) != NULL; idx++) {
         GList *list = NULL;

         list = gimv_plugin_get_list (type);
         help_plugin_info_append (list);
      }
#else
      {
         GList *list = NULL;

         list = gimv_plugin_get_list (NULL);
         help_plugin_info_append (list);
      }
#endif
   }

   /* create content widget */
   vbox = gtk_vbox_new (FALSE, 0);

   frame = gtk_frame_new (NULL);
   gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
   gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
   gtk_widget_show (frame);

   frame_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
   gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
   gtk_widget_show (frame_vbox);

   logo = gimv_icon_stock_get_widget ("gimageview");
   gtk_box_pack_start (GTK_BOX (frame_vbox), 
                       logo, FALSE, TRUE, 5);
   gimv_icon_stock_free_icon ("gimageview");
   gtk_widget_show (logo);

   tmpvbox = frame_vbox;

   /* Program name & Copyright */ 
   label = gtk_label_new (_(GIMV_PROG_VERSION));
   gtk_box_pack_start (GTK_BOX (tmpvbox), 
                       label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   /* Web Site Button */
   hbox1 = gtk_hbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (tmpvbox), 
                       hbox1, FALSE, FALSE, 0);
   gtk_widget_show (hbox1);
   hbox2 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox1), 
                       hbox2, TRUE, FALSE, 0);
   gtk_widget_show (hbox2);

   label = gtk_label_new (_("Web Site: "));
   gtk_box_pack_start (GTK_BOX (hbox2), 
                       label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   button = gtk_button_new ();
   gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
   label = gtk_label_new (GIMV_PROG_URI);
   gtk_container_add (GTK_CONTAINER (button), label);
   gtk_widget_show (label);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_progurl_clicked),
                     GIMV_PROG_URI);
   gtk_box_pack_start (GTK_BOX (hbox2), 
                       button, FALSE, FALSE, 0);
   gtk_widget_show (button);

   /* Infomation Text Box */
   scrolledwin = gtk_scrolled_window_new (NULL, NULL);
   info_win.scrolled_win = scrolledwin;
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
                                       GTK_SHADOW_IN);
   gtk_box_pack_start (GTK_BOX (vbox),
                       scrolledwin, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (scrolledwin), 5);
   gtk_widget_show (scrolledwin);

   text = gtk_text_view_new ();
   gtk_container_add (GTK_CONTAINER (scrolledwin), text);
   info_win.text_box = text;
   set_copyleft_str ();
   gtk_widget_show (text);

   /* Radio Button */
   hbox1 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), 
                       hbox1, FALSE, FALSE, 0);
   gtk_widget_show (hbox1);
   hbox2 = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (hbox1), 
                       hbox2, TRUE, FALSE, 0);
   gtk_widget_show (hbox2);

   radio = gtk_radio_button_new_with_label (NULL, _("License"));
   g_signal_connect (G_OBJECT (radio), "clicked",
                     G_CALLBACK (cb_gimv_info_change_text), NULL);
   gtk_box_pack_start (GTK_BOX (hbox2), radio, FALSE, FALSE, 0);
   gtk_widget_show (radio);

   radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio),
                                                        _("Authors"));
   g_signal_connect (G_OBJECT (radio), "clicked",
                     G_CALLBACK (cb_gimv_info_change_text),
                     _(authors));
   gtk_box_pack_start (GTK_BOX (hbox2), radio, FALSE, FALSE, 0);
   gtk_widget_show (radio);

   radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio),
                                                        _("System Info"));
   g_signal_connect (G_OBJECT (radio), "clicked",
                     G_CALLBACK (cb_gimv_info_change_text), system_info);
   gtk_box_pack_start (GTK_BOX (hbox2), radio, FALSE, FALSE, 0);
   gtk_widget_show (radio);

   radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio),
                                                        _("Plugin Info"));
   g_signal_connect (G_OBJECT (radio), "clicked",
                     G_CALLBACK (cb_gimv_info_change_text), plugin_info);
   gtk_box_pack_start (GTK_BOX (hbox2), radio, FALSE, FALSE, 0);
   gtk_widget_show (radio);

   return vbox;
}


void
gimvhelp_open_info_window (void)
{
   GtkWidget *widget, *window, *button;
   gchar buf[BUF_SIZE];

   /* window */
   window = gtk_dialog_new ();
   info_win.window = window;
   gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), 0);
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   g_snprintf (buf, BUF_SIZE, _("About %s"), GIMV_PROG_NAME);
   gtk_window_set_wmclass(GTK_WINDOW(window), "about", "GImageView");
   gtk_window_set_title (GTK_WINDOW (window), buf); 
   gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);

   /* main content */
   widget = gimvhelp_create_info_widget ();
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), 
                       widget, TRUE, TRUE, 0);
   gtk_widget_show (widget);

   /* OK Button */
   button = gtk_button_new_with_label (_("OK"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_gimv_info_win_ok_button),
                     window);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   gtk_widget_show (window);
   gimv_icon_stock_set_window_icon (window->window, "gimv_icon");

   gtk_widget_grab_focus (button);

   gtk_grab_add (window);
}
