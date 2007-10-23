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

#include "gimv_plugin.h"
#include "gimv_prefs_ui_utils.h"
#include "gimv_prefs_win.h"
#include "utils_char_code.h"
#include "utils_gtk.h"
#include "prefs_spi.h"
#include "spi.h"

#ifdef ENABLE_SPI

#define CONF_DIRLIST_KEY          "search_dir_list"
#define CONF_DIRLIST              SPI_DEFAULT_SEARCH_DIR_LIST
#define CONF_USE_DEF_DIRLIST_KEY "use_default_search_dir_list"
#define CONF_USE_DEF_DIRLIST     "TRUE"

extern GimvPluginInfo *gimv_spi_plugin_get_info  (void);
static GtkWidget *prefs_plugin_spi_page          (void);
static GtkWidget *prefs_plugin_spi_loader_page   (void);
static GtkWidget *prefs_plugin_spi_archiver_page (void);
static gboolean   prefs_spi_apply            (GimvPrefsWinAction action);


typedef struct SPIConf_Tag
{
   gboolean  use_def_dirlist;
   gchar    *dirlist;
} SPIConf;
SPIConf spiconf, spiconf_pre;


static GimvPrefsWinPage gimv_prefs_page_spi[] = {
   {
      path:           N_("/Plugin/Susie plugin"),
      priority_hint:  0,
      icon:           NULL,
      icon_open:      NULL,
      create_page_fn: prefs_plugin_spi_page,
      apply_fn:       prefs_spi_apply,
   },
   {
      path:           N_("/Plugin/Susie plugin/Import filter"),
      priority_hint:  0,
      icon:           NULL,
      icon_open:      NULL,
      create_page_fn: prefs_plugin_spi_loader_page,
      apply_fn:       NULL,
   },
   {
      path:           N_("/Plugin/Susie plugin/Archive extractor"),
      priority_hint:  0,
      icon:           NULL,
      icon_open:      NULL,
      create_page_fn: prefs_plugin_spi_archiver_page,
      apply_fn:       NULL,
   },
};


gboolean
gimv_prefs_ui_spi_get_page (guint idx,
                            GimvPrefsWinPage **page,
                            guint *size)
{
   g_return_val_if_fail(page, FALSE);
   *page = NULL;
   g_return_val_if_fail(size, FALSE);
   *size = 0;
   if (idx < sizeof (gimv_prefs_page_spi) / sizeof (GimvPrefsWinPage)) {
      *size = sizeof (GimvPrefsWinPage);
      *page = &gimv_prefs_page_spi[idx];
   } else {
      return FALSE;
   }
   return TRUE;
}


gboolean
gimv_prefs_spi_get_use_default_dirlist (void)
{
   GimvPluginInfo *this = gimv_spi_plugin_get_info();
   gboolean use = !strcasecmp("TRUE", CONF_USE_DEF_DIRLIST) ? TRUE : FALSE;
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGE_LOADER,
                                           CONF_USE_DEF_DIRLIST_KEY,
                                           GIMV_PLUGIN_PREFS_BOOL,
                                           (gpointer) &use);
   if (!success) {
      use = !strcasecmp("TRUE", CONF_USE_DEF_DIRLIST) ? TRUE : FALSE;
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_IMAGE_LOADER,
                                    CONF_USE_DEF_DIRLIST_KEY,
                                    CONF_USE_DEF_DIRLIST);
   }

   return use;
}


const gchar *
gimv_prefs_spi_get_dirconf (void)
{
   GimvPluginInfo *this = gimv_spi_plugin_get_info();
   const gchar *driver;
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGE_LOADER,
                                           CONF_DIRLIST_KEY,
                                           GIMV_PLUGIN_PREFS_STRING,
                                           (gpointer) &driver);
   if (success)
      return driver;

   driver = CONF_DIRLIST;
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_DIRLIST_KEY,
                                 driver);

   return driver;
}


static void
cb_spi_use_default_dir_list(GtkToggleButton *button, GtkWidget *widget)
{
   g_return_if_fail (GTK_IS_WIDGET(widget));

   if (button->active) {
      spiconf.use_def_dirlist = TRUE;
      gtk_widget_hide (widget);
   } else {
      spiconf.use_def_dirlist = FALSE;
      gtk_widget_show (widget);
   }
}


/*******************************************************************************
 *
 *  for Susie plugins
 *
 *******************************************************************************/
#include "spi.h"

static GtkWidget *
prefs_plugin_spi_page (void)
{
   GtkWidget *main_vbox, *frame, *toggle;

   spiconf.use_def_dirlist = spiconf_pre.use_def_dirlist
      = gimv_prefs_spi_get_use_default_dirlist ();

   spiconf.dirlist = (gchar *) gimv_prefs_spi_get_dirconf ();
   spiconf.dirlist = spiconf.dirlist ? g_strdup (spiconf.dirlist)
                                     : g_strdup ("");
   spiconf_pre.dirlist = g_strdup (spiconf.dirlist);

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   frame = gimv_prefs_ui_dir_list_prefs (_("Directories list to search susie plugins"),
                                         _("Select susie plugin directory"),
                                         spiconf_pre.dirlist,
                                         &spiconf.dirlist,
                                    ',');
   toggle = gtkutil_create_check_button (_("Use default directories list to search susie plugins"),
                                         spiconf.use_def_dirlist,
                                         cb_spi_use_default_dir_list,
                                         frame);

   gtk_box_pack_start(GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

   gtk_widget_show (main_vbox);
   gtk_widget_show (toggle);
   gtk_widget_show_all (frame);

   if (spiconf.use_def_dirlist)
      gtk_widget_hide (frame);
   else
      gtk_widget_show (frame);

   return main_vbox;
}


static GtkWidget *
create_spi_admin_widget (GList *plugin_list)
{
   GtkWidget *hbox, *scrollwin;
   GtkWidget *clist;
   GList *list;
   gint i;
   gchar *titles[] = {N_("Plugin Info"), N_("File Name")};
   gint titles_num = sizeof (titles)/ sizeof (gchar *);
   const gchar *text[32];
   GtkListStore *store;

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);

   scrollwin = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 5);
   gtk_box_pack_start (GTK_BOX (hbox), scrollwin, TRUE, TRUE, 0);
   gtk_widget_set_size_request (scrollwin, -1, 200);

   store = gtk_list_store_new (titles_num,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING);
   clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);
   gtk_container_add (GTK_CONTAINER (scrollwin), clist);

   /* set columns */
   for (i = 0; i < titles_num; i++) {
      GtkTreeViewColumn *col;
      GtkCellRenderer *render;

      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title (col, _(titles[i]));
      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render, "text", i);
      gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);
   }

   for (list = plugin_list; list; list = g_list_next (list)) {
      SusiePlugin *spi = list->data;
      GtkTreeIter iter;

      text[0] = spi->description;
#ifdef G_PLATFORM_WIN32
      text[1] = g_module_name (spi->pe);
#else
      text[1] = spi->pe->filepath;
#endif

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, text[0],
                          1, text[1],
                          -1);
   }

   return hbox;
}


static GtkWidget *
prefs_plugin_spi_loader_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox;
   GList *list;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   gimv_prefs_ui_create_frame (_("Import filters"),
                               frame, frame_vbox, main_vbox, TRUE);

   /* clist */
   list = spi_get_import_filter_list ();
   hbox = create_spi_admin_widget (list);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


static GtkWidget *
prefs_plugin_spi_archiver_page (void)
{
   GtkWidget *main_vbox, *frame, *frame_vbox, *hbox;
   GList *list;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   gimv_prefs_ui_create_frame (_("Archive Extractor"),
                               frame, frame_vbox, main_vbox, TRUE);

   /* clist */
   list = spi_get_archive_extractor_list ();
   hbox = create_spi_admin_widget (list);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, TRUE, TRUE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


static gboolean
prefs_spi_apply (GimvPrefsWinAction action)
{
   gchar *dirlist, *use;
   GimvPluginInfo *this = gimv_spi_plugin_get_info();

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_APPLY:
      dirlist = spiconf.dirlist;
      use = spiconf.use_def_dirlist ? "TRUE" : "FALSE";
      break;
   default:
      dirlist = spiconf_pre.dirlist;
      use = spiconf_pre.use_def_dirlist ? "TRUE" : "FALSE";
      break;
   }

   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_USE_DEF_DIRLIST_KEY,
                                 use);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_DIRLIST_KEY,
                                 dirlist);

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_CANCEL:
      g_free(spiconf.dirlist);
      g_free(spiconf_pre.dirlist);
      spiconf.dirlist     = NULL;
      spiconf_pre.dirlist = NULL;
      break;
   default:
      break;
   }

   return FALSE;
}

#endif /* ENABLE_SPI */
