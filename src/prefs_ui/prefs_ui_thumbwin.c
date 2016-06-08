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

#include "gimv_dir_view.h"
#include "gimv_prefs_ui_utils.h"
#include "gimv_prefs_win.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "prefs.h"
#include "prefs_ui_thumbwin.h"
#include "utils_gtk.h"
#include "utils_menu.h"


typedef struct PrefsWin_Tag {
   /* thumbnail window */
   GtkWidget *thumbwin_width_spin;
   GtkWidget *thumbwin_height_spin;
   GtkWidget *thumbwin_layout;
   GtkWidget *thumbwin_dirview_toggle;
   GtkWidget *thumbwin_preview_toggle;
   GtkWidget *thumbwin_preview_tab_toggle;
   GtkWidget *thumbwin_menubar_toggle;
   GtkWidget *thumbwin_toolbar_toggle;
   GtkWidget *thumbwin_statusbar_toggle;
   GtkWidget *thumbwin_tab_toggle;
   GtkWidget *thumbwin_player_label;
   GtkWidget *thumbwin_player_radio[3];
   GtkWidget *thumbwin_tab_pos;

   /* thumbnail view */
   GtkWidget *thumbview_display_mode;
   GtkWidget *thumbview_thumb_size;

   /* Directory View */
   GtkWidget *dirview_show_current_dir;
   GtkWidget *dirview_show_parent_dir;

   /* Preview */
   GtkWidget *preview_fit_only_zoom_out;
   GtkWidget *preview_scale_spin;
   GtkWidget *preview_scrollbar_toggle;
} PrefsWin;

static PrefsWin prefs_win = {
   /* thumbnail window */
   thumbwin_width_spin:         NULL,
   thumbwin_height_spin:        NULL,
   thumbwin_layout:             NULL,
   thumbwin_dirview_toggle:     NULL,
   thumbwin_preview_toggle:     NULL,
   thumbwin_preview_tab_toggle: NULL,
   thumbwin_menubar_toggle:     NULL,
   thumbwin_toolbar_toggle:     NULL,
   thumbwin_statusbar_toggle:   NULL,
   thumbwin_tab_toggle:         NULL,
   thumbwin_tab_pos:            NULL,

   /* thumbnail view */
   thumbview_display_mode:      NULL,
   thumbview_thumb_size:        NULL,

   /* Directory View */
   dirview_show_current_dir:    NULL,
   dirview_show_parent_dir:     NULL,

   /* Preview */
   preview_fit_only_zoom_out:   NULL,
   preview_scale_spin:          NULL,
   preview_scrollbar_toggle:    NULL,
};


static void set_sensitive_thumbwin     (void);
static void set_sensitive_thumbview    (void);
static void set_sensitive_preview      (void);
static void set_sensitive_thumbwin_tab (void);


static const gchar *layout_items[] = {
   N_("Layout0"),
   N_("Layout1"),
   N_("Layout2"),
   N_("Layout3"),
   N_("Layout4"),
   N_("Custom"),
   NULL
};


static const gchar *toolbar_style_items[] = {
   N_("Icon"),
   N_("Text"),
   N_("Both"),
   NULL
};


static const gchar *tab_pos_items[] = {
   N_("Left"),
   N_("Right"),
   N_("Top"),
   N_("Bottom"),
   NULL
};


static const gchar *thumbview_mouse_items[] = {
   N_("None"),
   N_("Popup menu"),
   N_("Open (Auto select window)"),
   N_("Open (Auto & Force)"),
   N_("Open in preview"),
   N_("Open in preview (Force)"),
   N_("Open in new window"),
   N_("Open in shared window"),
   N_("Open in shared window (Force)"),
   NULL
};


static const gchar *dirview_mouse_items[] = {
   N_("None"),
   N_("Load thumbnails"),
   N_("Load thumbnails recursively"),
   N_("Popup menu"),
   N_("Change top directory"),
   N_("Load recursively in a tab"),
   NULL
};


static const gchar *preview_mouse_items[] = {
   N_("None"),
   N_("Next image"),
   N_("Previous image"),
   N_("Open in shared window"),
   N_("Open in new window"),
   N_("Popup menu"),
   N_("Zoom in"),
   N_("Zoom out"),
   N_("Fit image size to frame"),
   N_("Rotate CCW"),
   N_("Rotate CW"),
   N_("Open navigate window"),
   N_("Scroll up"),
   N_("Scroll down"),
   N_("Scroll left"),
   N_("Scroll right"),
   NULL
};

/* next two imported from prefs_ui_imagewin.c */

extern const gchar *rotate_menu_items[];
extern const gchar *zoom_menu_items[];

extern Config   *config_changed;
extern Config   *config_prechanged;


static void
cb_thumbwin_page_destroy (GtkWidget *widget, gpointer data)
{
   prefs_win.thumbwin_width_spin         = NULL;
   prefs_win.thumbwin_height_spin        = NULL;
   prefs_win.thumbwin_layout             = NULL;
   prefs_win.thumbwin_dirview_toggle     = NULL;
   prefs_win.thumbwin_preview_toggle     = NULL;
   prefs_win.thumbwin_preview_tab_toggle = NULL;
   prefs_win.thumbwin_menubar_toggle     = NULL;
   prefs_win.thumbwin_toolbar_toggle     = NULL;
   prefs_win.thumbwin_statusbar_toggle   = NULL;
   prefs_win.thumbwin_tab_toggle         = NULL;
}


static void
cb_thumbwin_tab_page_destroy (GtkWidget *widget, gpointer data)
{
   prefs_win.thumbwin_tab_pos = NULL;  
}


static void
cb_thumbview_page_destroy (GtkWidget *widget, gpointer data)
{
   prefs_win.thumbview_display_mode = NULL;
   prefs_win.thumbview_thumb_size   = NULL;
}


static void
cb_dirview_page_destroy (GtkWidget *widget, gpointer data)
{
   prefs_win.dirview_show_current_dir = NULL;
   prefs_win.dirview_show_parent_dir  = NULL;
}


static void
cb_preview_page_destroy (GtkWidget *widget, gpointer data)
{
   prefs_win.preview_fit_only_zoom_out = NULL;
   prefs_win.preview_scale_spin        = NULL;
   prefs_win.preview_scrollbar_toggle  = NULL;
}


static void
cb_player_visible (GtkWidget *radio, gpointer data)
{
   gint idx = GPOINTER_TO_INT (data);

   if (!GTK_TOGGLE_BUTTON (radio)->active) return;

   config_changed->preview_player_visible = idx;
}


static void
set_sensitive_thumbwin (void)
{
   GtkWidget *widgets[] = {
      prefs_win.thumbwin_width_spin,
      prefs_win.thumbwin_height_spin,
      prefs_win.thumbwin_layout,
      prefs_win.thumbwin_dirview_toggle,
      prefs_win.thumbwin_preview_toggle,
      prefs_win.thumbwin_preview_tab_toggle,
      prefs_win.thumbwin_menubar_toggle,
      prefs_win.thumbwin_toolbar_toggle,
      prefs_win.thumbwin_statusbar_toggle,
      prefs_win.thumbwin_tab_toggle,
      prefs_win.thumbwin_player_label,
      prefs_win.thumbwin_player_radio[0],
      prefs_win.thumbwin_player_radio[1],
      prefs_win.thumbwin_player_radio[2],
   };
   gint i, num = sizeof (widgets) / sizeof (widgets[0]);

   for (i = 0; i < num; i++) {
      if (widgets[i])
         gtk_widget_set_sensitive (widgets[i],
                                   !config_changed->thumbwin_save_win_state);
   }

   set_sensitive_thumbview    ();
   set_sensitive_preview      ();
   set_sensitive_thumbwin_tab ();
}


static void
set_sensitive_thumbwin_tab (void)
{
   if (prefs_win.thumbwin_tab_pos)
      gtk_widget_set_sensitive (prefs_win.thumbwin_tab_pos,
                                !config_changed->thumbwin_save_win_state);
}


static void
set_sensitive_thumbview (void)
{
   GtkWidget *widgets[] = {
      prefs_win.thumbview_display_mode,
      prefs_win.thumbview_thumb_size,
   };
   gint i, num = sizeof (widgets) / sizeof (widgets[0]);

   for (i = 0; i < num; i++) {
      if (widgets[i])
         gtk_widget_set_sensitive (widgets[i],
                                   !config_changed->thumbwin_save_win_state);
   }
}


static void
set_sensitive_preview (void)
{
   if (prefs_win.preview_scrollbar_toggle)
      gtk_widget_set_sensitive (prefs_win.preview_scrollbar_toggle,
                                !config_changed->thumbwin_save_win_state);
   if (prefs_win.preview_scale_spin)
      gtk_widget_set_sensitive (prefs_win.preview_scale_spin,
                                config_changed->preview_zoom == 0 ||
                                config_changed->preview_zoom == 1);
}


static void
cb_save_thumbwin_state (GtkWidget *toggle)
{
   config_changed->thumbwin_save_win_state =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));

   set_sensitive_thumbwin ();
}


static void
cb_default_disp_mode (GtkWidget *widget)
{
   const gchar *tmpstr;
   gint disp_mode;

   disp_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "num"));
   tmpstr = gimv_thumb_view_num_to_label (disp_mode);
   if (tmpstr)
      config_changed->thumbwin_disp_mode = (gchar *) tmpstr;
   else
      config_changed->thumbwin_disp_mode = GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE;
}


static void
cb_zoom_menu (GtkWidget *menu)
{
   config_changed->preview_zoom =
      GPOINTER_TO_INT (g_object_get_data(G_OBJECT(menu), "num"));
   set_sensitive_preview();
}



/*******************************************************************************
 *  prefs_thumbwin_page:
 *     @ Create Thumbnail View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_thumbwin_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *vbox, *vbox2, *hbox, *hbox2, *hbox3;
   GtkWidget *label;
   GtkAdjustment *adj;
   GtkWidget *spinner;
   GtkWidget *option_menu;
   GtkWidget *toggle, *radio[3];

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
   g_signal_connect (G_OBJECT (main_vbox), "destroy",
                     G_CALLBACK (cb_thumbwin_page_destroy), NULL);


   /**********************************************
    * Window Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Window"), frame, vbox, main_vbox, FALSE);

   /* Save window state on exit */
   toggle = gtkutil_create_check_button (_("Save window state on exit"),
                                         conf.thumbwin_save_win_state,
                                         cb_save_thumbwin_state,
                                         &config_changed->thumbwin_save_win_state);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Window width and height spinner */
   hbox = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("width"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbwin_width,
                                               1.0, 10000.0, 1.0, 5.0, 0.0);
   spinner = prefs_win.thumbwin_width_spin = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 50, -1);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_int_cb),
                     &config_changed->thumbwin_width);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   label = gtk_label_new (_("height"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbwin_height,
                                               1.0, 10000.0, 1.0, 5.0, 0.0);
   spinner = prefs_win.thumbwin_height_spin = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 50, -1);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_int_cb),
                     &config_changed->thumbwin_height);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   /* Toolbar Style */
   label = gtk_label_new (_("Default Layout"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

   option_menu = create_option_menu_simple (layout_items,
                                            conf.thumbwin_layout_type,
                                            (gint *) &config_changed->thumbwin_layout_type);
   prefs_win.thumbwin_layout = option_menu;
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);

   /* Show/Hide frame */
   gimv_prefs_ui_create_frame (_("Show/Hide"), frame, vbox2, vbox, FALSE);

   hbox = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 0);
   gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

   hbox2 = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER(hbox2), 0);
   gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);

   hbox3 = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER(hbox3), 0);
   gtk_box_pack_start (GTK_BOX (vbox2), hbox3, FALSE, FALSE, 0);

   /* Show Directory or not */
   toggle = gtkutil_create_check_button (_("Directory view"),
                                         conf.thumbwin_show_dir_view,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_dir_view);
   prefs_win.thumbwin_dirview_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

   /* Show Preview or not */
   toggle = gtkutil_create_check_button (_("Preview"),
                                         conf.thumbwin_show_preview,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_preview);
   prefs_win.thumbwin_preview_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

   /* Show Preview Tab or not */
   toggle = gtkutil_create_check_button (_("Preview Tab"),
                                         conf.thumbwin_show_preview_tab,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_preview_tab);
   prefs_win.thumbwin_preview_tab_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

   /* Show Menubar or not */
   toggle = gtkutil_create_check_button (_("Menubar"),
                                         conf.thumbwin_show_menubar,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_menubar);
   prefs_win.thumbwin_menubar_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);

   /* Show Toolbar or not */
   toggle = gtkutil_create_check_button (_("Toolbar"),
                                         conf.thumbwin_show_toolbar,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_toolbar);
   prefs_win.thumbwin_toolbar_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);

   /* Show Statusbar or not */
   toggle = gtkutil_create_check_button (_("Statusbar"),
                                         conf.thumbwin_show_statusbar,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_statusbar);
   prefs_win.thumbwin_statusbar_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);

   /* Show Tab or not */
   toggle = gtkutil_create_check_button (_("Tab"),
                                         conf.thumbwin_show_tab,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_tab);
   prefs_win.thumbwin_tab_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (hbox2), toggle, FALSE, FALSE, 0);

   /* player */
   label = gtk_label_new (_("Player toolbar :"));
   gtk_box_pack_start (GTK_BOX (hbox3), label, FALSE, FALSE, 0);
   prefs_win.thumbwin_player_label = label;

   radio[0] = gtk_radio_button_new_with_label (NULL, _("Show"));
   gtk_box_pack_start (GTK_BOX (hbox3), radio[0], FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (radio[0]), "toggled",
                     G_CALLBACK (cb_player_visible),
                     GINT_TO_POINTER(GimvImageViewPlayerVisibleShow));
   prefs_win.thumbwin_player_radio[0] = radio[0];

   radio[1] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio[0]),
                                                           _("Hide"));
   gtk_box_pack_start (GTK_BOX (hbox3), radio[1], FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (radio[1]), "toggled",
                     G_CALLBACK (cb_player_visible),
                     GINT_TO_POINTER(GimvImageViewPlayerVisibleHide));
   prefs_win.thumbwin_player_radio[1] = radio[1];

   radio[2] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio[1]),
                                                           _("Auto"));
   gtk_box_pack_start (GTK_BOX (hbox3), radio[2], FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (radio[2]), "toggled",
                     G_CALLBACK (cb_player_visible),
                     GINT_TO_POINTER(GimvImageViewPlayerVisibleAuto));
   prefs_win.thumbwin_player_radio[2] = radio[2];

   switch (config_changed->preview_player_visible) {
   case GimvImageViewPlayerVisibleShow:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio[0]), TRUE);
      break;
   case GimvImageViewPlayerVisibleHide:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio[1]), TRUE);
      break;
   case GimvImageViewPlayerVisibleAuto:
   default:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio[2]), TRUE);
      break;
   }

   /* Raise window or not */
   toggle = gtkutil_create_check_button (_("Raise window when opening thumbnails"),
                                         conf.thumbwin_raise_window,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_raise_window);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   /* Toolbar Style */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   label = gtk_label_new (_("Toolbar Style"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

   option_menu = create_option_menu_simple (toolbar_style_items,
                                            conf.thumbwin_toolbar_style,
                                            (gint *) &config_changed->thumbwin_toolbar_style);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);


   /* "Loading" frame */
   gimv_prefs_ui_create_frame (_("Loading"), frame, vbox, main_vbox, FALSE);

   /* GUI redraw interval */
   hbox = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   label = gtk_label_new (_("GUI redraw interval while loading: Every"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbwin_redraw_interval,
                                               1.0, 1000.0, 1.0, 5.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 50, -1);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_int_cb),
                     &config_changed->thumbwin_redraw_interval);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);

   label = gtk_label_new (_("files"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

   /* show detail of progress */
   toggle = gtkutil_create_check_button (_("Show detail of loading progress"),
                                         conf.thumbwin_show_progress_detail,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_progress_detail);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);


   /* set sensitive */
   set_sensitive_thumbwin ();

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


GtkWidget *
prefs_thumbview_toolbar_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *label;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   label = gtk_label_new (_("Not implemented yet."));
   gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 5);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_thumbwin_tab_page:
 *     @ Create Thumbnail View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_thumbwin_tab_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *vbox, *hbox;
   GtkWidget *label;
   GtkWidget *option_menu;
   GtkWidget *toggle;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
   g_signal_connect (G_OBJECT (main_vbox), "destroy",
                     G_CALLBACK (cb_thumbwin_tab_page_destroy), NULL);


   /**********************************************
    * Tab Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Tab"), frame, vbox, main_vbox, FALSE);

   /* Tab Position Selection */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Tab Position"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

   option_menu = create_option_menu_simple (tab_pos_items,
                                            conf.thumbwin_tab_pos,
                                            (gint *) &config_changed->thumbwin_tab_pos);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   prefs_win.thumbwin_tab_pos = option_menu;

   /* Move to new tab automatically or not */
   toggle = gtkutil_create_check_button (_("Move to new tab automatically"),
                                         conf.thumbwin_move_to_newtab,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_move_to_newtab);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);

   /* show tab close button */
   toggle = gtkutil_create_check_button (_("Show close button"),
                                         conf.thumbwin_show_tab_close,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_show_tab_close);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   /* show full path in tab */
   toggle = gtkutil_create_check_button (_("Show full path"),
                                         conf.thumbwin_tab_fullpath,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_tab_fullpath);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   /* force open tab */
   toggle = gtkutil_create_check_button (_("Open new tab whether image is exist or not "
                                           "in the directory"),
                                         conf.thumbwin_force_open_tab,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbwin_force_open_tab);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);


   set_sensitive_thumbwin_tab ();

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


GtkWidget *
prefs_thumbview_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *vbox, *hbox;
   GtkWidget *toggle, *spinner, *label, *option_menu;
   GtkAdjustment *adj;
   gint num, disp_mode;
   gchar **disp_mode_labels;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
   g_signal_connect (G_OBJECT (main_vbox), "destroy",
                     G_CALLBACK (cb_thumbview_page_destroy), NULL);


   /**********************************************
    * Show/Hide Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Show/Hide"), frame, vbox, main_vbox, FALSE);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

   /* show directory */
   toggle = gtkutil_create_check_button (_("Directory"),
                                         conf.thumbview_show_dir,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbview_show_dir);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);

   /* show archive */
   toggle = gtkutil_create_check_button (_("Archive"),
                                         conf.thumbview_show_archive,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->thumbview_show_archive);
   gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 5);


   /* Default display mode */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

   disp_mode_labels = gimv_thumb_view_get_summary_mode_labels (&num);

   label = gtk_label_new (_("Default display mode"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

   disp_mode = gimv_thumb_view_label_to_num (conf.thumbwin_disp_mode);
   option_menu = create_option_menu ((const gchar **)disp_mode_labels,
                                     disp_mode,
                                     cb_default_disp_mode,
                                     NULL);
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 5);
   prefs_win.thumbview_display_mode = option_menu;

   /* Thumbnail size spinner */
   hbox = gtk_hbox_new (FALSE, 10);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

   label = gtk_label_new (_("Thumnail Size"));
   gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbwin_thumb_size,
                                               GIMV_THUMB_WIN_MIN_THUMB_SIZE,
                                               GIMV_THUMB_WIN_MAX_THUMB_SIZE,
                                               1.0, 5.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   prefs_win.thumbview_thumb_size = spinner;
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_int_cb),
                     &config_changed->thumbwin_thumb_size);
   gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, TRUE, 0);


   set_sensitive_thumbview  ();

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_thumbview_mouse_page:
 *     @ Create mouse button action on thumbnail view preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_thumbview_mouse_page (void)
{
   GtkWidget *vbox;

   vbox = gimv_prefs_ui_mouse_prefs (thumbview_mouse_items,
                                     conf.thumbview_mouse_button,
                                     &config_changed->thumbview_mouse_button);

   gtk_widget_show_all (vbox);

   return vbox;
}


/*******************************************************************************
 *  prefs_dirview_page:
 *     @ Create Directory View preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_dirview_page (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *vbox;
   GtkWidget *toggle;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
   g_signal_connect (G_OBJECT (main_vbox), "destroy",
                     G_CALLBACK (cb_dirview_page_destroy), NULL);


   /**********************************************
    * Shoe/Hide Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Show/Hide"), frame, vbox, main_vbox, FALSE);

   /* show/hide toolbar */
   toggle = gtkutil_create_check_button (_("Show toolbar"),
                                         conf.dirview_show_toolbar,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dirview_show_toolbar);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* show/hide dot file */
   toggle = gtkutil_create_check_button (_("Show dot file"),
                                         conf.dirview_show_dotfile,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dirview_show_dotfile);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* show/hide dot file */
   toggle = gtkutil_create_check_button (_("Show \".\" directory whether hide dotfile or not."),
                                         conf.dirview_show_current_dir,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dirview_show_current_dir);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* show/hide dot file */
   toggle = gtkutil_create_check_button (_("Show \"..\" directory whether hide dotfile or not."),
                                         conf.dirview_show_parent_dir,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->dirview_show_parent_dir);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_dirview_mouse_page:
 *     @ Create mouse button action on dirview preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_dirview_mouse_page (void)
{
   GtkWidget *vbox;

   vbox = gimv_prefs_ui_mouse_prefs (dirview_mouse_items,
                                     conf.dirview_mouse_button,
                                     &config_changed->dirview_mouse_button);

   gtk_widget_show_all (vbox);

   return vbox;
}


/*******************************************************************************
 *  prefs_preview_page:
 *     @ Create pewview preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_preview_page (void)
{
   GtkWidget *main_vbox, *frame, *vbox, *hbox, *table, *alignment;
   GtkWidget *spinner, *toggle, *label, *option_menu;
   GtkAdjustment *adj;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);
   g_signal_connect (G_OBJECT (main_vbox), "destroy",
                     G_CALLBACK (cb_preview_page_destroy), NULL);


   /********************************************** 
    * Image Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Image"), frame, vbox, main_vbox, FALSE);

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
                                     conf.preview_zoom,
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
                                            conf.preview_rotation,
                                            &config_changed->preview_rotation);
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), option_menu);
   gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
#if 0
   gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
#endif

   /* Keep Aspect Ratio */
   toggle = gtkutil_create_check_button (_("Keep aspect ratio"),
                                         conf.preview_keep_aspect,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->preview_keep_aspect);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* Default Image Scale Spinner */
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   label = gtk_label_new (_("Default Image Scale"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.preview_scale,
                                               1.0, 10000.0, 1.0, 5.0, 0.0);
   prefs_win.preview_scale_spin = spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_size_request(spinner, 50, -1);
   g_signal_connect (G_OBJECT (adj), "value_changed",
                     G_CALLBACK (gtkutil_get_data_from_adjustment_by_float_cb),
                     &config_changed->preview_scale);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
   label = gtk_label_new (_("%"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

   /* Show Scrollbar or not */
   toggle = gtkutil_create_check_button (_("Show scrollbar"),
                                         conf.preview_scrollbar,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->preview_scrollbar);
   prefs_win.preview_scrollbar_toggle = toggle;
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   /* keep original image on memory or not */
   toggle = gtkutil_create_check_button (_("Keep original image on memory"),
                                         conf.preview_buffer,
                                         gtkutil_get_data_from_toggle_cb,
                                         &config_changed->preview_buffer);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);

   set_sensitive_preview  ();

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}


/*******************************************************************************
 *  prefs_preview_mouse_page:
 *     @ Create mouse button action on preview preference page.
 *
 *  Return : Packed widget (GtkVbox)
 *******************************************************************************/
GtkWidget *
prefs_preview_mouse_page (void)
{
   GtkWidget *vbox;

   vbox = gimv_prefs_ui_mouse_prefs (preview_mouse_items,
                                     conf.preview_mouse_button,
                                     &config_changed->preview_mouse_button);

   gtk_widget_show_all (vbox);

   return vbox;
}


gboolean
prefs_ui_thumbwin_apply (GimvPrefsWinAction action)
{
   GimvThumbWin *tw;
   GList *node;
   Config *dest;

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_APPLY:
      dest = config_changed;
      break;
   default:
      dest = config_prechanged;
      break;
   }

   for (node = gimv_thumb_win_get_list(); node; node = g_list_next (node)) {
      tw = node->data;
      gtk_toolbar_set_style (GTK_TOOLBAR(tw->toolbar),
                             dest->thumbwin_toolbar_style);

      gtk_widget_queue_resize (tw->toolbar_handle);
   }

   return FALSE;
}


gboolean
prefs_ui_thumbwin_tab_apply (GimvPrefsWinAction action)
{
   GList *node;

   /* tab label */
   for (node = gimv_thumb_view_get_list(); node; node = g_list_next (node)) {
      GimvThumbView *tv = node->data;
      gimv_thumb_view_reset_tab_label (tv, NULL);
   }

   return FALSE;
}


gboolean
prefs_ui_dirview_apply (GimvPrefsWinAction action)
{
   return FALSE;
}


gboolean
prefs_ui_preview_apply (GimvPrefsWinAction action)
{
   GimvThumbWin *tw;
   GList *node;
   Config *dest;

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_APPLY:
      dest = config_changed;
      break;
   default:
      dest = config_prechanged;
      break;
   }

   for (node = gimv_thumb_win_get_list(); node; node = g_list_next (node)) {
      tw = node->data;

      if (tw->iv) {
         if (dest->preview_zoom == 0) {
            g_object_set (G_OBJECT (tw->iv),
                          "x_scale", dest->preview_scale,
                          "y_scale", dest->preview_scale,
                          NULL);
         }
         g_object_set (G_OBJECT (tw->iv),
                       "default_zoom",       dest->preview_zoom,
                       "default_rotation",   dest->preview_rotation,
                       "keep_aspect",        dest->preview_keep_aspect,
                       "keep_buffer",        dest->preview_buffer,
                       "show_scrollbar",     dest->preview_scrollbar,
                       NULL);
      }
   }

   return FALSE;
}
