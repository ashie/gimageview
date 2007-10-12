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

#include "prefs_mplayer.h"

#ifdef ENABLE_MPLAYER

#include <stdlib.h>
#include <string.h>

#include "gtkutils.h"
#include "gimv_mplayer.h"
#include "gimv_prefs_ui_utils.h"

#define CONF_VO_DRIVER_KEY        "vo_driver"
#ifdef GDK_WINDOWING_FB
#  define CONF_VO_DRIVER          "fbdev"
#else /* GDK_WINDOWING_FB */
#  define CONF_VO_DRIVER          "default"
#endif /* GDK_WINDOWING_FB */
#define CONF_AO_DRIVER_KEY        "ao_driver"
#define CONF_AO_DRIVER            "default"
#define CONF_THUMBNAIL_ENABLE_KEY "thumbnail_enable"
#define CONF_THUMBNAIL_ENABLE     "TRUE"
#define CONF_THUMBNAIL_POS_KEY    "thumbnail_pos"
#define CONF_THUMBNAIL_POS        "1.0"
#define CONF_THUMBNAIL_FRAMES_KEY "thumbnail_frames"
#define CONF_THUMBNAIL_FRAMES     "5"


typedef struct MPlayerConf_Tag
{
   gchar    *vo_driver;
   gchar    *ao_driver;
   gboolean  thumb;
   gfloat    thumb_pos;
   gint      thumb_frames;
} MPlayerConf;

static MPlayerConf mconf, mconf_pre;


extern GimvPluginInfo *gimv_mplayer_plugin_get_info (void);
static GtkWidget      *prefs_mplayer_page           (void);
static gboolean        prefs_mplayer_apply          (GimvPrefsWinAction action);

static GimvPrefsWinPage mplayer_prefs_page =
{
   path:           N_("/Movie and Audio/MPlayer"),
   priority_hint:  0,
   icon:           NULL,
   icon_open:      NULL,
   create_page_fn: prefs_mplayer_page,
   apply_fn:       prefs_mplayer_apply,
};


gboolean
gimv_prefs_ui_mplayer_get_page (guint idx, GimvPrefsWinPage **page, guint *size)
{
   g_return_val_if_fail(page, FALSE);
   *page = NULL;
   g_return_val_if_fail(size, FALSE);
   *size = 0;

   if (idx == 0) {
      *page = &mplayer_prefs_page;
      *size = sizeof(mplayer_prefs_page);
      return TRUE;
   } else {
      return FALSE;
   }
}


gboolean
gimv_prefs_mplayer_get_thumb_enable (void)
{
   GimvPluginInfo *this = gimv_mplayer_plugin_get_info();
   gboolean enable = !strcasecmp("TRUE", CONF_THUMBNAIL_ENABLE) ? TRUE : FALSE;
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGE_LOADER,
                                           CONF_THUMBNAIL_ENABLE_KEY,
                                           GIMV_PLUGIN_PREFS_BOOL,
                                           (gpointer) &enable);
   if (!success) {
      enable = !strcasecmp("TRUE", CONF_THUMBNAIL_ENABLE) ? TRUE : FALSE;
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_IMAGE_LOADER,
                                    CONF_THUMBNAIL_ENABLE_KEY,
                                    CONF_THUMBNAIL_ENABLE);
   }

   return enable;
}


gfloat
gimv_prefs_mplayer_get_thumb_pos (void)
{
   GimvPluginInfo *this = gimv_mplayer_plugin_get_info();
   gfloat delay = atof (CONF_THUMBNAIL_POS);
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGE_LOADER,
                                           CONF_THUMBNAIL_POS_KEY,
                                           GIMV_PLUGIN_PREFS_FLOAT,
                                           (gpointer) &delay);
   if (!success) {
      delay = atof (CONF_THUMBNAIL_POS);
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_IMAGE_LOADER,
                                    CONF_THUMBNAIL_POS_KEY,
                                    CONF_THUMBNAIL_POS);
   }

   return delay;
}


gint
gimv_prefs_mplayer_get_thumb_frames (void)
{
   GimvPluginInfo *this = gimv_mplayer_plugin_get_info();
   gfloat frames = atoi (CONF_THUMBNAIL_FRAMES);
   gboolean success;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGE_LOADER,
                                           CONF_THUMBNAIL_FRAMES_KEY,
                                           GIMV_PLUGIN_PREFS_INT,
                                           (gpointer) &frames);
   if (!success) {
      frames = atoi (CONF_THUMBNAIL_FRAMES);
      gimv_plugin_prefs_save_value (this->name,
                                    GIMV_PLUGIN_IMAGE_LOADER,
                                    CONF_THUMBNAIL_FRAMES_KEY,
                                    CONF_THUMBNAIL_FRAMES);
   }

   return frames;
}


const gchar *
gimv_prefs_mplayer_get_driver (const gchar *type)
{
   GimvPluginInfo *this = gimv_mplayer_plugin_get_info();
   const gchar *key;
   const gchar *driver;
   gboolean success;

   key = type && !strcasecmp("ao", type) ? CONF_AO_DRIVER_KEY
                                         : CONF_VO_DRIVER_KEY;

   success = gimv_plugin_prefs_load_value (this->name,
                                           GIMV_PLUGIN_IMAGEVIEW_EMBEDER,
                                           key,
                                           GIMV_PLUGIN_PREFS_STRING,
                                           (gpointer) &driver);
   if (success)
      return driver;

   driver = type && !strcasecmp("ao", type) ? CONF_AO_DRIVER : CONF_VO_DRIVER;
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGEVIEW_EMBEDER,
                                 key,
                                 driver);

   return driver;
}


static void
cb_vo_combo_changed (GtkEditable *editable, gpointer data)
{
   const gchar *text = gtk_entry_get_text (GTK_ENTRY (editable));

   g_free (mconf.vo_driver);
   mconf.vo_driver = NULL;

   if (text && *text)
      mconf.vo_driver = g_strdup (text);
}


static void
cb_ao_combo_changed (GtkEditable *editable, gpointer data)
{
   const gchar *text = gtk_entry_get_text (GTK_ENTRY (editable));

   g_free (mconf.ao_driver);
   mconf.ao_driver = NULL;

   if (text && *text)
      mconf.ao_driver = g_strdup (text);
}


static GtkWidget *
prefs_mplayer_page (void)
{
   GtkWidget *main_vbox, *frame, *vbox, *hbox, *table, *alignment;
   GtkWidget *label, *vo_combo, *ao_combo, *mplayer, *spinner, *toggle;
   GtkAdjustment *adj;
   const GList *list;

   main_vbox = gtk_vbox_new (FALSE, 0);

   mconf.vo_driver     = (gchar *) gimv_prefs_mplayer_get_driver("vo");
   mconf.vo_driver     = mconf.vo_driver ? g_strdup(mconf.vo_driver)
                                         : g_strdup("");
   mconf_pre.vo_driver = g_strdup(mconf.vo_driver);

   mconf.ao_driver     = (gchar *) gimv_prefs_mplayer_get_driver("ao");
   mconf.ao_driver     = mconf.ao_driver ? g_strdup(mconf.ao_driver)
                                         : g_strdup("");
   mconf_pre.ao_driver = g_strdup(mconf.ao_driver);

   mconf.thumb = mconf_pre.thumb
      = gimv_prefs_mplayer_get_thumb_enable();
   mconf.thumb_pos = mconf_pre.thumb_pos
      = gimv_prefs_mplayer_get_thumb_pos();
   mconf.thumb_frames = mconf_pre.thumb_frames
      = gimv_prefs_mplayer_get_thumb_frames();

   /**********************************************
    * Driver Frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Driver"), frame, vbox, main_vbox, FALSE);

   alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
   gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
   gtk_widget_show (alignment);

   table = gtk_table_new (2, 2, FALSE);
   gtk_container_add (GTK_CONTAINER (alignment), table);
   gtk_widget_show (table);

   /* video driver combo */
   label = gtk_label_new (_("Video driver : "));
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_widget_show (alignment);
   gtk_widget_show (label);

   vo_combo = gtk_combo_new ();
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), vo_combo);
   gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 0, 1,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_widget_show (alignment);
   gtk_widget_show (vo_combo);
   gtk_widget_set_usize (vo_combo, 100, -1);

   /* audio driver combo */
   label = gtk_label_new (_("Audio driver : "));
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), label);
   gtk_table_attach (GTK_TABLE (table), alignment, 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_widget_show (alignment);
   gtk_widget_show (label);

   ao_combo = gtk_combo_new ();
   alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
   gtk_container_add (GTK_CONTAINER (alignment), ao_combo);
   gtk_table_attach (GTK_TABLE (table), alignment, 1, 2, 1, 2,
                     GTK_EXPAND | GTK_FILL, GTK_FILL, 5, 1);
   gtk_widget_show (alignment);
   gtk_widget_show (ao_combo);
   gtk_widget_set_usize (ao_combo, 100, -1);


   /**********************************************
    * Thumbnail Frame
    **********************************************/
   gimv_prefs_ui_create_frame(_("Thumbnail"), frame, vbox, main_vbox, FALSE);

   /* use this feature or not */
   toggle = gtkutil_create_check_button (_("Enable creating thumbnail of movie using MPlayer"),
                                         mconf.thumb,
                                         gtkutil_get_data_from_toggle_cb,
                                         &mconf.thumb);
   gtk_container_set_border_width (GTK_CONTAINER(toggle), 5);
   gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
   gtk_widget_show (toggle);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_container_set_border_width (GTK_CONTAINER(hbox), 5);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show (hbox);
   label = gtk_label_new (_("Stream position : "));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_show (label);
   adj = (GtkAdjustment *) gtk_adjustment_new (mconf.thumb_pos,
                                               0.0, 100.0, 0.01, 0.1, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_usize(spinner, 70, -1);
   gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinner), 2);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                       GTK_SIGNAL_FUNC (gtkutil_get_data_from_adjustment_by_float_cb),
                       &mconf.thumb_pos);
   gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
   gtk_widget_show (spinner);

   label = gtk_label_new (_("[%]"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_show (label);



   /* for detecting drivers */
   mplayer = gimv_mplayer_new ();
#ifdef USE_GTK2
   g_object_ref (G_OBJECT (mplayer));
   gtk_object_sink (GTK_OBJECT (mplayer));
#endif /* USE_GTK2 */

   /* set drivers list */
   list = gimv_mplayer_get_video_out_drivers (GIMV_MPLAYER (mplayer), FALSE);
   gtk_combo_set_popdown_strings (GTK_COMBO (vo_combo), (GList *) list);
   list = gimv_mplayer_get_audio_out_drivers (GIMV_MPLAYER (mplayer), FALSE);
   gtk_combo_set_popdown_strings (GTK_COMBO (ao_combo), (GList *) list);

   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(vo_combo)->entry),
                       mconf.vo_driver);
   gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(ao_combo)->entry),
                       mconf.ao_driver);

   gtk_signal_connect (GTK_OBJECT (GTK_COMBO(vo_combo)->entry), "changed",
                       GTK_SIGNAL_FUNC (cb_vo_combo_changed), NULL);
   gtk_signal_connect (GTK_OBJECT (GTK_COMBO(ao_combo)->entry), "changed",
                       GTK_SIGNAL_FUNC (cb_ao_combo_changed), NULL);

   gtk_widget_unref (mplayer);

   return main_vbox;
}


static gboolean
prefs_mplayer_apply (GimvPrefsWinAction action)
{
   GimvPluginInfo *this = gimv_mplayer_plugin_get_info();
   gchar *vo_driver, *ao_driver, pos_str[32], frames_str[32], *enable;

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_APPLY:
      vo_driver = mconf.vo_driver;
      ao_driver = mconf.ao_driver;
      enable = mconf.thumb ? "TRUE" : "FALSE";
      g_snprintf(pos_str, 32, "%f", mconf.thumb_pos);
      g_snprintf(frames_str, 32, "%d", mconf.thumb_frames);
      break;
   default:
      vo_driver = mconf_pre.vo_driver;
      ao_driver = mconf_pre.ao_driver;
      enable = mconf_pre.thumb ? "TRUE" : "FALSE";
      g_snprintf(pos_str, 32, "%f", mconf_pre.thumb_pos);
      g_snprintf(frames_str, 32, "%d", mconf_pre.thumb_frames);
      break;
   }

   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGEVIEW_EMBEDER,
                                 CONF_VO_DRIVER_KEY,
                                 vo_driver);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGEVIEW_EMBEDER,
                                 CONF_AO_DRIVER_KEY,
                                 ao_driver);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_THUMBNAIL_ENABLE_KEY,
                                 enable);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_THUMBNAIL_POS_KEY,
                                 pos_str);
   gimv_plugin_prefs_save_value (this->name,
                                 GIMV_PLUGIN_IMAGE_LOADER,
                                 CONF_THUMBNAIL_FRAMES_KEY,
                                 frames_str);

   switch (action) {
   case GIMV_PREFS_WIN_ACTION_OK:
   case GIMV_PREFS_WIN_ACTION_CANCEL:
      g_free(mconf.vo_driver);
      g_free(mconf_pre.vo_driver);
      g_free(mconf.ao_driver);
      g_free(mconf_pre.ao_driver);
      mconf.vo_driver     = NULL;
      mconf_pre.ao_driver = NULL;
      mconf.vo_driver     = NULL;
      mconf_pre.ao_driver = NULL;
      break;
   default:
      break;
   }

   return FALSE;
}

#endif /* ENABLE_MPLAYER */
