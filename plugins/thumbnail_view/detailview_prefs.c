/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2002 Takuro Ashie
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

#include "detailview_prefs.h"

#include "gimv_prefs_win.h"
#include "gimv_prefs_ui_utils.h"
#include "detailview.h"
#include "gimv_plugin.h"

typedef struct DetailViewConf_Tag
{
   gchar    *data_order;
   gboolean  show_title;
} DetailViewConf;

DetailViewConf detailview_conf, *detailview_conf_pre;

static GimvPluginPrefsEntry detailview_prefs_entry [] = {
   {"data_order", GIMV_PLUGIN_PREFS_STRING,
    DETAIL_DEFAULT_COLUMN_ORDER, &detailview_conf.data_order},
   {"show_title", GIMV_PLUGIN_PREFS_BOOL,
    "TRUE",  &detailview_conf.show_title},
};

GIMV_PLUGIN_PREFS_GET_VALUE(detailview, "Thumbnail View Detail Mode",
                            GIMV_PLUGIN_THUMBVIEW_EMBEDER,
                            detailview_prefs_entry)

static GtkWidget *detailview_prefs_ui       (void);
static gboolean   detailview_prefs_ui_apply (GimvPrefsWinAction action);

static GimvPrefsWinPage detailview_prefs_page =
{
   path:           N_("/Thumbnail Window/Thumbnail View/Detail View"),
   priority_hint:  10,
   icon:           NULL,
   icon_open:      NULL,
   create_page_fn: detailview_prefs_ui,
   apply_fn:       detailview_prefs_ui_apply,
};

gboolean
gimv_prefs_ui_detailview_get_page (guint idx,
                                   GimvPrefsWinPage **page,
                                   guint *size)
{
   g_return_val_if_fail(page, FALSE);
   *page = NULL;
   g_return_val_if_fail(size, FALSE);
   *size = 0;

   if (idx == 0) {
      *page = &detailview_prefs_page;
      *size = sizeof(detailview_prefs_page);
      return TRUE;
   } else {
      return FALSE;
   }
}


/*******************************************************************************
 *  prefs_detailview_page:
 *     @ Create Detail View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
static GtkWidget *
detailview_prefs_ui(void)
{
   GtkWidget *main_vbox, *frame, *toggle;
   gint i;
   GList *list = NULL;

   GIMV_PLUGIN_PREFS_GET_ALL (detailview, detailview_prefs_entry,
                              detailview_conf_pre, detailview_conf);

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   for (i = 1; i < detailview_get_titles_num (); i++) {
      gchar *text = detailview_get_title (i);
      list = g_list_append (list, text);
   }

   frame = gimv_prefs_ui_double_clist (_("Column Order"),
                                       _("Possible columns"),
                                       _("Displayed columns"),
                                       list,
                                       detailview_conf_pre->data_order,
                                       &detailview_conf.data_order,
                                       ',');
   g_list_free (list);

   gtk_box_pack_start(GTK_BOX (main_vbox), frame, FALSE, TRUE, 0);


   /* show/hide column title */
   toggle = gtkutil_create_check_button (_("Show Column Title"),
                                         detailview_conf.show_title,
                                         gtkutil_get_data_from_toggle_cb,
                                         &detailview_conf.show_title);
   gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


static gboolean
detailview_prefs_ui_apply (GimvPrefsWinAction action)
{
   GIMV_PLUGIN_PREFS_APPLY_ALL (detailview_prefs_entry,
                                "Thumbnail View Detail Mode",
                                GIMV_PLUGIN_THUMBVIEW_EMBEDER,
                                action,
                                detailview_conf_pre, detailview_conf);

   /* detail view */
   detailview_apply_config ();

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_CANCEL:
      GIMV_PLUGIN_PREFS_FREE_ALL (detailview_prefs_entry,
                                  detailview_conf_pre,
                                  detailview_conf);
      break;
   default:
      break;
   }

   return FALSE;
}
