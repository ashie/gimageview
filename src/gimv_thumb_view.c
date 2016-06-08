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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gimageview.h"

#include "fileload.h"
#include "gimv_comment_view.h"
#include "gimv_dir_view.h"
#include "gimv_dupl_finder.h"
#include "gimv_dupl_win.h"
#include "gimv_image.h"
#include "gimv_image_win.h"
#include "gimv_scrolled.h"
#include "gimv_thumb.h"
#include "gimv_thumb_cache.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "gimv_file_prop_win.h"
#include "prefs.h"
#include "utils_char_code.h"
#include "utils_dnd.h"
#include "utils_file.h"
#include "utils_file_gtk.h"
#include "utils_gtk.h"
#include "utils_menu.h"

#ifdef ENABLE_EXIF
#   include "gimv_exif_view.h"
#endif /* ENABLE_EXIF */


typedef enum
{
   ACTION_NONE,
   ACTION_POPUP,
   ACTION_OPEN_AUTO,
   ACTION_OPEN_AUTO_FORCE,
   ACTION_OPEN_IN_PREVIEW,
   ACTION_OPEN_IN_PREVIEW_FORCE,
   ACTION_OPEN_IN_NEW_WIN,
   ACTION_OPEN_IN_SHARED_WIN,
   ACTION_OPEN_IN_SHARED_WIN_FORCE
} ButtonActionType;


struct GimvThumbViewPriv_Tag
{
   gchar               *dirname;   /* store dir name if mode is dirmode */
   FRArchive           *archive;

   /* list of relation to other object */
   GList               *related_image_view;
   GList               *related_dupl_win;

   /* for mouse event */
   gint                 button_2pressed_queue; /* attach an action to
                                                  button release event */
};


static void gimv_thumb_view_dispose    (GObject       *object);


/* callback functions */
static void   cb_open_image            (GimvThumbView  *tv,
                                        GimvThumbViewOpenImageType action,
                                        GtkWidget      *menuitem);
static void   cb_open_image_by_external(GtkWidget      *menuitem,
                                        GimvThumbView  *tv);
static void   cb_open_image_by_script  (GtkWidget      *menuitem,
                                        GimvThumbView  *tv);
static void   cb_recreate_thumbnail    (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
static void   cb_remove_thumbnail      (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
static void   cb_file_property         (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
#ifdef ENABLE_EXIF
static void   cb_exif                  (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
#endif /* ENABLE_EXIF */
static void   cb_edit_comment          (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
static void   cb_file_operate          (GimvThumbView  *tv,
                                        FileOperateType type,
                                        GtkWidget      *widget);
static void   cb_rename_file           (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
static void   cb_remove_file           (GimvThumbView  *tv,
                                        guint           action,
                                        GtkWidget      *menuitem);
static void   cb_dupl_win_destroy      (GtkWidget      *window,
                                        GimvThumbView  *tv);
static void   cb_thumbview_scrollbar_value_changed
                                       (GtkWidget      *widget,
                                        GimvThumbView  *tv);


/* compare functions */
static int comp_func_spel  (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_size  (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_atime (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_mtime (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_ctime (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_type  (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_width (gconstpointer data1,
                            gconstpointer data2);
static int comp_func_height(gconstpointer data1,
                            gconstpointer data2);
static int comp_func_area  (gconstpointer data1,
                            gconstpointer data2);

/* other private functions */
static void       gimv_thumb_view_append_thumb_data(GimvThumbView  *tv,
                                                    GimvThumb      *thumb);
static void       gimv_thumb_view_remove_thumb_data(GimvThumbView  *tv,
                                                    GimvThumb      *thumb);
static gchar     *get_uri_list                     (GList          *thumblist);
static void       gimv_thumb_view_button_action    (GimvThumbView  *tv,
                                                    GimvThumb      *thumb,
                                                    GdkEventButton *event,
                                                    gint            num);
static GtkWidget *create_progs_submenu             (GimvThumbView  *tv);
static GtkWidget *create_scripts_submenu           (GimvThumbView  *tv);
static void       gimv_thumb_view_reset_load_priority
                                                   (GimvThumbView  *tv);
static void       gimv_thumb_view_set_scrollbar_callback
                                                   (GimvThumbView *tv);
static void       gimv_thumb_view_remove_scrollbar_callback
                                                   (GimvThumbView *tv);
static GCompareFunc
                  gimv_thumb_view_get_compare_func(GimvThumbView *tv,
                                                    gboolean      *reverse);


/* reference popup menu for each thumbnail */
static GtkItemFactoryEntry thumb_button_popup_items [] =
{
   {N_("/_Open"),                     NULL,  cb_open_image,    GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO,        NULL},
   {N_("/Open in New _Window"),       NULL,  cb_open_image,    GIMV_THUMB_VIEW_OPEN_IMAGE_NEW_WIN,     NULL},
   {N_("/Open in S_hared Window"),    NULL,  cb_open_image,    GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN,  NULL},
   {N_("/Open in E_xternal Program"), NULL,  NULL,             0,           "<Branch>"},
   {N_("/_Scripts"),                  NULL,  NULL,             0,           "<Branch>"},
   {N_("/---"),                       NULL,  NULL,             0,           "<Separator>"},
   {N_("/_Update Thumbnail"),         NULL,  cb_recreate_thumbnail,  0,     NULL},
   {N_("/Remo_ve from List"),         NULL,  cb_remove_thumbnail,    0,     NULL},
   {N_("/---"),                       NULL,  NULL,             0,           "<Separator>"},
   {N_("/_Property..."),              NULL,  cb_file_property, 0,           NULL},
#ifdef ENABLE_EXIF
   {N_("/Scan E_XIF Data..."),        NULL,  cb_exif,          0,           NULL},
#endif
   {N_("/_Edit Comment..."),          NULL,  cb_edit_comment,  0,           NULL},
   {N_("/---"),                       NULL,  NULL,             0,           "<Separator>"},
   {N_("/Re_name..."),                NULL,  cb_rename_file,   0,           NULL},
   {N_("/_Copy Files To..."),         NULL,  cb_file_operate,  FILE_COPY,   NULL},
   {N_("/_Move Files To..."),         NULL,  cb_file_operate,  FILE_MOVE,   NULL},
   {N_("/_Link Files To..."),         NULL,  cb_file_operate,  FILE_LINK,   NULL},
   {N_("/_Remove file..."),           NULL,  cb_remove_file,   0,           NULL},
   {NULL, NULL, NULL, 0, NULL},
};


static GList   *GimvThumbViewList = NULL;

static guint    total_tab_count = 0;

static guint    button = 0;
static gboolean pressed = FALSE;
static gboolean dragging = FALSE;
static gint     drag_start_x = 0;
static gint     drag_start_y = 0;



/****************************************************************************
 *
 *  Plugin Management
 *
 ****************************************************************************/
static GList                *plugin_list = NULL;
static GHashTable           *modes_table = NULL;
static gchar               **mode_labels = NULL;
extern GimvThumbViewPlugin   thumbalbum_modes[];
extern gint                  thumbalbum_modes_num;

gint
gimv_thumb_view_label_to_num(const gchar *label)
{
   GimvThumbViewPlugin *view, *default_view;
   gint pos;

   g_return_val_if_fail (modes_table, 0);

   if (!label || !*label) return 0;

   view = g_hash_table_lookup (modes_table, label);

   if (!view) {
      default_view = g_hash_table_lookup (modes_table,
                                          GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE);
      g_return_val_if_fail (view && default_view, 0);
      pos = g_list_index (plugin_list, view);
   } else {
      pos = g_list_index (plugin_list, view);
   }

   if (pos < 0)
      return 0;
   else
      return pos;
}


const gchar *
gimv_thumb_view_num_to_label (gint num)
{
   GimvThumbViewPlugin *view;

   view = g_list_nth_data (plugin_list, num);
   g_return_val_if_fail (view, NULL);

   return view->label;
}


GList *
gimv_thumb_view_get_summary_mode_list (void)
{
   gint i;

   if (!modes_table)
      modes_table = g_hash_table_new (g_str_hash, g_str_equal);

   if (!plugin_list) {
      for (i = 0; i < thumbalbum_modes_num; i++) {
         g_hash_table_insert (modes_table,
                              (gpointer) thumbalbum_modes[i].label,
                              &thumbalbum_modes[i]);
         plugin_list = g_list_append (plugin_list, &thumbalbum_modes[i]);
      }
   }

   return plugin_list;
}


gchar **
gimv_thumb_view_get_summary_mode_labels (gint *length_ret)
{
   gint i, num;
   gchar **labels;
   GList *list;

   gimv_thumb_view_get_summary_mode_list ();
   g_return_val_if_fail (plugin_list, NULL);

   if (mode_labels) {
      *length_ret = sizeof (mode_labels) / sizeof (gchar *);
      return mode_labels;
   }

   num = *length_ret = g_list_length (plugin_list);
   g_return_val_if_fail (num > 0, NULL);

   mode_labels = labels = g_new0 (gchar *, num + 1);
   list = plugin_list;
   for (i = 0; list && i < num; i++) {
      GimvThumbViewPlugin *view = list->data;
      if (!view) {
         g_free (labels);
         return NULL;
      }
      labels[i] = g_strdup (view->label);
      list = g_list_next (list);
   }
   labels[num] = NULL;

   return labels;
}


static gint
comp_func_priority (GimvThumbViewPlugin *plugin1,
                    GimvThumbViewPlugin *plugin2)
{
   g_return_val_if_fail (plugin1, 1);
   g_return_val_if_fail (plugin2, -1);

   return plugin1->priority_hint - plugin2->priority_hint;
}


gboolean
gimv_thumb_view_plugin_regist (const gchar *plugin_name,
                               const gchar *module_name,
                               gpointer impl,
                               gint     size)
{
   GimvThumbViewPlugin *plugin = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (plugin, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (plugin->if_version == GIMV_THUMBNAIL_VIEW_IF_VERSION,
                         FALSE);
   g_return_val_if_fail (plugin->label, FALSE);

   if (!plugin_list)
      gimv_thumb_view_get_summary_mode_list();

   g_hash_table_insert (modes_table,
                        (gpointer) plugin->label,
                        plugin);
   plugin_list = g_list_append (plugin_list, plugin);
   plugin_list = g_list_sort (plugin_list,
                              (GCompareFunc) comp_func_priority);

   return TRUE;
}


/******************************************************************************
 *
 *   Callback functions.
 *
 ******************************************************************************/
static void
cb_open_image (GimvThumbView *tv, GimvThumbViewOpenImageType action,
               GtkWidget *menuitem)
{
   GimvThumb *thumb;
   GList *thumblist, *node;
   gint listnum;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   listnum = g_list_length (thumblist);

   if (action == GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN) {
      thumb = thumblist->data;
      gimv_thumb_view_open_image (tv, thumb, action);
   } else {
      FilesLoader *files;

      files = files_loader_new ();
      if (listnum > 2)
         files_loader_create_progress_window (files);
      node = thumblist;

      while (node) {
         gint pos;
         gfloat progress;
         gchar buf[32];

         thumb = node->data;
         node = g_list_next (node);

         if (files->status == CANCEL || files->status == STOP) break;

         gimv_thumb_view_open_image (tv, thumb, action);

         pos = g_list_position (thumblist, node) + 1;
         progress = (gfloat) pos / (gfloat) listnum;
         g_snprintf (buf, 32, "%d/%d files", pos, listnum);
         files_loader_progress_update (files, progress, buf);
      }

      files_loader_destroy_progress_window (files);
      files_loader_delete (files);
   }

   g_list_free (thumblist);
}


static void
cb_open_image_by_external (GtkWidget *menuitem, GimvThumbView *tv)
{
   GimvThumb *thumb;
   GList *thumblist, *node;
   guint action;
   gchar *user_cmd, *cmd = NULL, *tmpstr = NULL, **pair;
   gboolean show_dialog = FALSE;

   g_return_if_fail (menuitem && GIMV_IS_THUMB_VIEW (tv));

   thumblist = node = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   action = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "num"));

   /* find command */
   if (action < sizeof (conf.progs) / sizeof (conf.progs[0])) {
      pair = g_strsplit (conf.progs[action], ",", 3);
      if (!pair[1]) {
         g_strfreev (pair);
         return;
      } else {
         cmd = g_strdup (pair[1]);
      }
      if (pair[2] && !g_ascii_strcasecmp (pair[2], "TRUE"))
         show_dialog = TRUE;
      g_strfreev (pair);
   } else {
      return;
   }

   /* create command string */
   while (node) {
      thumb = node->data;
      tmpstr = g_strconcat (cmd, " ",
                            "\"", gimv_image_info_get_path (thumb->info), "\"",
                            NULL);
      g_free (cmd);
      cmd = tmpstr;
      node = g_list_next (node);
   }
   tmpstr = g_strconcat (cmd, " &", NULL);
   g_free (cmd);
   cmd = tmpstr;
   tmpstr = NULL;

   tmpstr = cmd;
   cmd = charset_to_internal (cmd,
                              conf.charset_filename,
                              conf.charset_auto_detect_fn,
                              conf.charset_filename_mode);
   g_free (tmpstr);
   tmpstr = NULL;

   if (show_dialog) {
      user_cmd = gtkutil_popup_textentry (_("Execute command"),
                                          _("Please enter options:"),
                                          cmd, NULL, 400,
                                          TEXT_ENTRY_AUTOCOMP_PATH
                                          | TEXT_ENTRY_WRAP_ENTRY
                                          | TEXT_ENTRY_CURSOR_TOP,
                                          GTK_WINDOW (tv->tw));
   } else {
      user_cmd = g_strdup (cmd);
   }
   g_free (cmd);
   cmd = NULL;

   /* exec command */
   if (user_cmd) {
      tmpstr = user_cmd;
      user_cmd = charset_internal_to_locale (user_cmd);
      g_free (tmpstr);
      tmpstr = NULL;
      system (user_cmd);
      g_free (user_cmd);
   }

   g_list_free (thumblist);
}


static void
cb_open_image_by_script (GtkWidget *menuitem, GimvThumbView *tv)
{
   GimvThumb *thumb;
   GList *thumblist, *node;
   gchar *script, *cmd = NULL, *tmpstr = NULL, *user_cmd;

   g_return_if_fail (menuitem && GIMV_IS_THUMB_VIEW (tv));

   thumblist = node = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   script = g_object_get_data (G_OBJECT (menuitem), "script");
   if (!script || !script || !isexecutable (script)) goto ERROR;

   cmd = g_strdup (script);

   /* create command string */
   while (node) {
      thumb = node->data;
      tmpstr = g_strconcat (cmd, " ",
                            "\"", gimv_image_info_get_path (thumb->info), "\"",
                            NULL);
      g_free (cmd);
      cmd = tmpstr;
      node = g_list_next (node);
   }
   tmpstr = g_strconcat (cmd, " &", NULL);
   g_free (cmd);
   cmd = tmpstr;
   tmpstr = NULL;

   {
      tmpstr = cmd;
      cmd = charset_to_internal (cmd,
                                 conf.charset_filename,
                                 conf.charset_auto_detect_fn,
                                 conf.charset_filename_mode);
      g_free (tmpstr);
      tmpstr = NULL;

      if (conf.scripts_show_dialog) {
         user_cmd = gtkutil_popup_textentry (_("Execute script"),
                                             _("Please enter options:"),
                                             cmd, NULL, 400,
                                             TEXT_ENTRY_AUTOCOMP_PATH
                                             | TEXT_ENTRY_WRAP_ENTRY
                                             | TEXT_ENTRY_CURSOR_TOP,
                                             GTK_WINDOW (tv->tw));
      } else {
         user_cmd = g_strdup (cmd);
      }
      g_free (cmd);
      cmd = NULL;

      tmpstr = user_cmd;
      user_cmd = charset_internal_to_locale (user_cmd);
      g_free (tmpstr);
      tmpstr = NULL;
   }

   /* exec command */
   if (user_cmd) {
      {   /********** convert charset **********/
         tmpstr = user_cmd;
         user_cmd = charset_internal_to_locale (user_cmd);
         g_free (tmpstr);
         tmpstr = NULL;
      }
      system (user_cmd);
      g_free (user_cmd);
      user_cmd = NULL;
   }

ERROR:
   g_list_free (thumblist);
}


/* FIXME: context of progress should be displayed */
static void
cb_recreate_thumbnail (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   GimvThumb *thumb;
   GList *thumblist, *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   for (node = thumblist; node; node = g_list_next (node)) {
      thumb = node->data;
      if (!thumb) continue;

      gimv_thumb_view_refresh_thumbnail (tv, thumb, CREATE_THUMB);
   }

   g_list_free (thumblist);
}
/* END FIXME */


static void
cb_remove_thumbnail (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   GimvThumb *thumb;
   GList *thumblist, *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (tv->vfuncs);

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   if (tv->vfuncs->freeze)
      tv->vfuncs->freeze (tv);

   node = thumblist;
   while (node) {
      thumb = node->data;
      node = g_list_next (node);
      if (!GIMV_IS_THUMB (thumb)) continue;

      tv->vfuncs->remove_thumb (tv, thumb);
      gimv_thumb_view_remove_thumb_data (tv, thumb);
      g_object_unref (G_OBJECT(thumb));
   }

   if (tv->vfuncs->thaw)
      tv->vfuncs->thaw (tv);

   gimv_thumb_win_set_statusbar_page_info (tv->tw,
                                           GIMV_THUMB_WIN_CURRENT_PAGE);

   g_list_free (thumblist);
}


static void
cb_file_property (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   GimvThumb *thumb;
   GList *thumblist;
   GimvImageInfo *info;
   gint flags = 0;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist || g_list_length (thumblist) > 1) return;

   thumb = thumblist->data;
   if (!thumb) return;

   info = gimv_image_info_ref (thumb->info);
   if (!info) return;

   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      flags |= GTK_PROP_EDITABLE | GTK_PROP_NOT_DETECT_TYPE;
   }

   if (gimv_file_prop_win_run (info, flags))
      gimv_thumb_view_refresh_list (tv);

   gimv_image_info_unref (info);
}


#ifdef ENABLE_EXIF
static void
cb_exif (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   GimvThumb *thumb;
   GList *thumblist;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist || g_list_length (thumblist) > 1) return;

   thumb = thumblist->data;
   if (!thumb) return;

   gimv_exif_view_create_window (gimv_image_info_get_path (thumb->info),
                                 GTK_WINDOW (tv->tw));
}
#endif /* ENABLE_EXIF */


static void
cb_edit_comment (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   GList *thumblist, *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   while (node) {
      GimvThumb *thumb = node->data;
      node = g_list_next (node);

      if (!thumb) continue;

      gimv_comment_view_create_window (thumb->info);
   }
}


static gchar *previous_dir = NULL;


static void
cb_file_operate (GimvThumbView *tv, FileOperateType type, GtkWidget *widget)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   gimv_thumb_view_file_operate (tv, type);
}


static void
cb_rename_file (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   gimv_thumb_view_rename_file (tv);
}


static void
cb_remove_file (GimvThumbView *tv, guint action, GtkWidget *menuitem)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   gimv_thumb_view_delete_files (tv);
}


static void
cb_dupl_win_destroy (GtkWidget *window, GimvThumbView *tv)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (!tv->priv) return;

   tv->priv->related_dupl_win
      = g_list_remove (tv->priv->related_dupl_win, window);
}


static void
cb_thumbview_scrollbar_value_changed (GtkWidget *widget,
                                      GimvThumbView *tv)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   gimv_thumb_view_reset_load_priority (tv);
}


typedef struct GimvThumbViewButtonAction_Tag
{
   GimvThumbView *tv;
   GimvThumb *thumb;
   GdkEventButton event;
   gint action;
} GimvThumbViewButtonAction;


static gint
idle_button_action (gpointer data)
{
   GimvThumbViewButtonAction *act = data;
   g_return_val_if_fail (act, 0);
   gimv_thumb_view_button_action (act->tv, act->thumb,
                                  &act->event, act->action);
   return 0;
}


gboolean
gimv_thumb_view_thumb_button_press_cb (GtkWidget *widget,
                                       GdkEventButton *event,
                                       GimvThumb *thumb)
{
   GimvThumbView *tv;
   GimvThumbWin *tw;
   gint num;

   g_return_val_if_fail (event, FALSE);

   button = event->button;
   pressed = TRUE;
   drag_start_x = event->x;
   drag_start_y = event->y;

   if (!thumb) goto ERROR;
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv = gimv_thumb_get_parent_thumbview (thumb);
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tw = tv->tw;
   g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), FALSE);

   /* reset selection */
   if (event->type == GDK_BUTTON_PRESS
       && (event->button == 2 || event->button == 3))
   {
      if (!thumb->selected) {
         gimv_thumb_view_set_selection_all (tv, FALSE);
         gimv_thumb_view_set_selection (thumb, TRUE);
      }
   }

   while (gtk_events_pending()) gtk_main_iteration();

   num = prefs_mouse_get_num_from_event (event, conf.thumbview_mouse_button);
   if (event->type == GDK_2BUTTON_PRESS) {
      tv->priv->button_2pressed_queue = num;
   } else if (num > 0) {
      GimvThumbViewButtonAction *act = g_new0 (GimvThumbViewButtonAction, 1);

      act->tv = tv;
      act->thumb = thumb;
      act->event = *event;
      act->action = num;
      gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                         idle_button_action, NULL, act,
                         (GtkDestroyNotify) g_free);
   }

 ERROR:
   if (event->button == 3) /* for avoiding notebook's event */
      return TRUE;
   else
      return FALSE;
}


gboolean
gimv_thumb_view_thumb_button_release_cb (GtkWidget *widget,
                                         GdkEventButton *event,
                                         GimvThumb *thumb)
{
   GimvThumbView *tv;
   GimvThumbWin *tw;
   gint num;

   g_return_val_if_fail (event, FALSE);

   if (!thumb) goto ERROR;
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv = gimv_thumb_get_parent_thumbview (thumb);
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   tw = tv->tw;
   g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), FALSE);

   if(pressed && !dragging) {
      if (tv->priv->button_2pressed_queue) {
         num = tv->priv->button_2pressed_queue;
         if (num > 0)
            num = 0 - num;
         tv->priv->button_2pressed_queue = 0;
      } else {
         num = prefs_mouse_get_num_from_event (event,
                                               conf.thumbview_mouse_button);
      }
      if (num < 0) {
         GimvThumbViewButtonAction *act = g_new0 (GimvThumbViewButtonAction, 1);
         act->tv = tv;
         act->thumb = thumb;
         act->event = *event;
         act->action = num;
         gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                            idle_button_action, NULL, act,
                            (GtkDestroyNotify) g_free);
      }
   }

 ERROR:
   button   = 0;
   pressed  = FALSE;
   dragging = FALSE;

   if (event->button == 3)   /* for avoiding notebook's callback */
      return TRUE;
   else
      return FALSE;
}


gboolean
gimv_thumb_view_thumb_key_press_cb (GtkWidget *widget,
                                    GdkEventKey *event,
                                    GimvThumb *thumb)
{
   guint keyval, popup_key = 0;
   GdkModifierType modval, popup_mod = 0;

   g_return_val_if_fail (event, FALSE);

   keyval = event->keyval;
   modval = event->state;

   if (akey.common_popup_menu || *akey.common_popup_menu)
      gtk_accelerator_parse (akey.common_popup_menu, &popup_key, &popup_mod);

   if (keyval == popup_key && (!popup_mod || (modval & popup_mod))) {
      GimvThumbView *tv = gimv_thumb_get_parent_thumbview (thumb);
      g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
      gimv_thumb_view_popup_menu (tv, NULL, NULL);
      return TRUE;
   }

   return FALSE;
}


gboolean
gimv_thumb_view_thumb_key_release_cb (GtkWidget *widget,
                                      GdkEventKey *event,
                                      GimvThumb *thumb)
{
   g_return_val_if_fail (event, FALSE);
   return FALSE;
}


gboolean
gimv_thumb_view_motion_notify_cb (GtkWidget *widget,
                                  GdkEventMotion *event,
                                  GimvThumb *thumb)
{
   GimvThumbView *tv;
   GimvThumbWin *tw;
   gint dx, dy;

   g_return_val_if_fail (event, FALSE);

   if (thumb) {
      tv = gimv_thumb_get_parent_thumbview (thumb);
      g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

      tw = tv->tw;
      g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), FALSE);
   }

   if (!pressed)
      return FALSE;

   dx = event->x - drag_start_x;
   dy = event->y - drag_start_y;

   if (!dragging && (abs (dx) > 2 || abs (dy) > 2))
      dragging = TRUE;

   /* return TRUE; */
   return FALSE;
}


void
gimv_thumb_view_drag_begin_cb (GtkWidget *widget,
                               GdkDragContext *context,
                               gpointer data)
{
   GimvThumbView *tv = data;
   GimvThumb *thumb;
   GList *thumblist;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GdkColormap *colormap;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv) && widget);

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   thumb = thumblist->data;
   gimv_thumb_get_icon (thumb, &pixmap, &mask);
   if (g_list_length (thumblist) == 1 && pixmap) {
      colormap = gdk_colormap_get_system ();
      gtk_drag_set_icon_pixmap (context, colormap, pixmap, mask, -7, -7);
   }
}


void
gimv_thumb_view_drag_data_get_cb (GtkWidget *widget,
                                  GdkDragContext *context,
                                  GtkSelectionData *seldata,
                                  guint info,
                                  guint time,
                                  gpointer data)
{
   GimvThumbView *tv = data;
   GList *thumblist;
   gchar *uri_list;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv) && widget);

   thumblist = gimv_thumb_view_get_selection_list (tv);

   if (!thumblist) {
      gtkutil_message_dialog (_("Error!!"), _("No files specified!!"),
                              GTK_WINDOW (tv->tw));
      return;
   }

   switch (info) {
   case TARGET_URI_LIST:
      if (!thumblist) return;
      uri_list = get_uri_list (thumblist);
      gtk_selection_data_set(seldata, seldata->target,
                             8, uri_list, strlen(uri_list));
      g_free (uri_list);
      break;
   default:
      break;
   }
}


void
gimv_thumb_view_drag_data_received_cb (GtkWidget *widget,
                                       GdkDragContext *context,
                                       gint x, gint y,
                                       GtkSelectionData *seldata,
                                       guint info,
                                       guint time,
                                       gpointer data)
{
   GimvThumbView *tv = data;
   FilesLoader *files;
   GList *list;
   GtkWidget *src_widget;
   GimvThumbView *src_tab = NULL, *dest_tab = NULL;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv) && widget);

   src_widget = gtk_drag_get_source_widget (context);
   if (src_widget == widget) return;

   if (src_widget)
      src_tab  = g_object_get_data (G_OBJECT (src_widget), "gimv-tab");
   dest_tab = g_object_get_data (G_OBJECT (widget), "gimv-tab");
   if (src_tab == dest_tab) return;

   if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
      gchar *dirname;

      if (tv->dnd_destdir)
         dirname = tv->dnd_destdir;
      else
         dirname = tv->priv->dirname;

      if (iswritable (dirname)) {
         dnd_file_operation (dirname, context, seldata,
                             time, tv->tw);
      } else {
         gchar error_message[BUF_SIZE], *dir_internal;

         dir_internal = charset_to_internal (dirname,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
         g_snprintf (error_message, BUF_SIZE,
                     _("Permission denied: %s"),
                     dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message,
                                 GTK_WINDOW (tv->tw));

         g_free (dir_internal);
      }
      tv->dnd_destdir = NULL;

   } else if (tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
      list = dnd_get_file_list (seldata->data, seldata->length);
      files = files_loader_new ();
      files->filelist = list;
      gimv_thumb_view_append_thumbnail (tv, files, FALSE);
      files_loader_delete (files);
   }
}


void
gimv_thumb_view_drag_end_cb (GtkWidget *widget, GdkDragContext *drag_context,
                             gpointer data)
{
   GimvThumbView *tv = data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (conf.dnd_refresh_list_always) {
      /* gimv_thumb_view_refresh_list (tv); */
      /* to avoid gtk's bug, exec redraw after exit this callback function */
      gtk_idle_add (gimv_thumb_view_refresh_list_idle, tv);
   }
}


void
gimv_thumb_view_drag_data_delete_cb (GtkWidget *widget,
                                     GdkDragContext *drag_context,
                                     gpointer data)
{
   GimvThumbView *tv = data;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (!conf.dnd_refresh_list_always)
      gimv_thumb_view_refresh_list (tv);
}



/******************************************************************************
 *
 *   Compare functions.
 *
 ******************************************************************************/
/* sort by spel */
static int
comp_func_spel (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   GimvThumbView *tv;
   const gchar *filename1, *filename2;
   gboolean ignore_dir;
   gint comp;
   GimvSortFlag flags;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   filename1 = gimv_image_info_get_path (thumb1->info);
   filename2 = gimv_image_info_get_path (thumb2->info);

   tv = gimv_thumb_get_parent_thumbview (thumb1);
   g_return_val_if_fail(GIMV_IS_THUMB_VIEW (tv), 0);
   gimv_thumb_win_get_sort_type (tv->tw, &flags);
   ignore_dir = flags & GIMV_SORT_DIR_INSENSITIVE;

   if (!filename1 || !*filename1)
      return -1;
   else if (!filename2 || !*filename2)
      return 1;

   if (filename1 && !strcmp ("..", g_basename (filename1))) {
      comp = -1;
   } else if (filename2 && !strcmp ("..", g_basename (filename2))) {
      comp = 1;
   } else if (!ignore_dir && isdir (filename1) && !isdir (filename2)) {
      comp = -1;
   } else if (!ignore_dir && !isdir (filename1) && isdir (filename2)) {
      comp = 1;
   } else {
      if (flags & GIMV_SORT_CASE_INSENSITIVE)
         comp = g_ascii_strcasecmp ((gchar *) filename1, (gchar *) filename2);
      else
         comp = strcmp ((gchar *) filename1, (gchar *) filename2);
   }

   return comp;
}


/* sort by file size */
static int
comp_func_size (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->st.st_size - thumb2->info->st.st_size;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by time of las access */
static int
comp_func_atime (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->st.st_atime - thumb2->info->st.st_atime;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by time of last modification */
static int
comp_func_mtime (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->st.st_mtime - thumb2->info->st.st_mtime;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by time of last change */
static int
comp_func_ctime (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->st.st_ctime - thumb2->info->st.st_ctime;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by time of file name extension */
static int
comp_func_type (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   const gchar *file1, *file2, *ext1, *ext2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   file1 = gimv_image_info_get_path (thumb1->info);
   file2 = gimv_image_info_get_path (thumb2->info);

   if (!file1 || !*file1)
      return -1;
   else if (!file2 || !*file2)
      return 1;

   ext1 = strrchr (file1, '.');
   if (ext1 && *ext1)
      ext1++;
   else
      ext1 = NULL;

   ext2 = strrchr (file2, '.');
   if (ext2 && *ext2)
      ext2++;
   else
      ext2 = NULL;

   if ((!ext1 || !*ext1) && (!ext2 || !*ext2)) {
      return g_ascii_strcasecmp (file1, file2);
   } else if (!ext1 || !*ext1) {
      return -1;
   } else if (!ext2 || !*ext2) {
      return 1;
   }

   retval = g_ascii_strcasecmp (ext1, ext2);

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by image width */
static int
comp_func_width (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->width - thumb2->info->width;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by image height */
static int
comp_func_height (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   retval = thumb1->info->height - thumb2->info->height;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}


/* sort by image width */
static int
comp_func_area (gconstpointer data1, gconstpointer data2)
{
   GimvThumb *thumb1, *thumb2;
   int retval, area1, area2;

   thumb1 = (GimvThumb *) data1;
   thumb2 = (GimvThumb *) data2;

   if (!thumb1)
      return -1;
   else if (!thumb2)
      return 1;

   area1 = thumb1->info->width * thumb1->info->height;
   if (thumb1->info->width < 0 && thumb1->info->height < 0)
      area1 = 0 - area1;
   area2 = thumb2->info->width * thumb2->info->height;
   if (thumb2->info->width < 0 && thumb2->info->height < 0)
      area2 = 0 - area2;

   retval = area1 - area2;

   if (retval == 0)
      return comp_func_spel (data1, data2);
   else
      return retval;
}



/******************************************************************************
 *
 *   Other Private Functions.
 *
 ******************************************************************************/
static gint
progress_timeout (gpointer data)
{
   gfloat new_val;
   GtkAdjustment *adj;

   adj = GTK_PROGRESS (data)->adjustment;

   new_val = adj->value + 1;
   if (new_val > adj->upper)
      new_val = adj->lower;

   gtk_progress_set_value (GTK_PROGRESS (data), new_val);

   return (TRUE);
}


static gboolean
gimv_thumb_view_extract_archive_file (GimvThumb *thumb)
{
   GimvThumbWin *tw;
   GimvThumbView *tv;
   FilesLoader *files;
   guint timer;
   gboolean success;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv = gimv_thumb_get_parent_thumbview (thumb);
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   if (tv->mode != GIMV_THUMB_VIEW_MODE_ARCHIVE) return FALSE;

   tw = tv->tw;
   g_return_val_if_fail (tw, FALSE);

   files = files_loader_new ();
   tv->status = GIMV_THUMB_VIEW_STATUS_LOADING;
   files->archive = gimv_image_info_get_archive (thumb->info);
   if (files->archive)
      g_object_ref (G_OBJECT (files->archive));

   /* set progress bar */
   gtk_progress_set_activity_mode (GTK_PROGRESS (tw->progressbar), TRUE);
   timer = gtk_timeout_add (50, (GtkFunction)progress_timeout, tw->progressbar);

   /* extract */
   success = gimv_image_info_extract_archive (thumb->info);

   /* unset progress bar */
   gtk_timeout_remove (timer);
   gtk_progress_set_activity_mode (GTK_PROGRESS (tw->progressbar), FALSE);
   gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar), 0.0);

   tv->status = GIMV_THUMB_VIEW_STATUS_NORMAL;
   files_loader_delete (files);

   return success;
}


static void
gimv_thumb_view_append_thumb_data (GimvThumbView *tv, GimvThumb *thumb)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tv->thumblist = g_list_append (tv->thumblist, thumb);

   /* update file num info */
   tv->tw->filenum++;
   tv->tw->filesize += thumb->info->st.st_size;
   tv->filenum++;
   tv->filesize += thumb->info->st.st_size;
}


static void
gimv_thumb_view_remove_thumb_data (GimvThumbView *tv, GimvThumb *thumb)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (g_list_find (tv->thumblist, thumb));

   /* update file num info */
   tv->tw->filenum--;
   tv->tw->filesize -= thumb->info->st.st_size;
   tv->filenum--;
   tv->filesize -= thumb->info->st.st_size;

   tv->thumblist = g_list_remove (tv->thumblist, thumb);
}


static void
gimv_thumb_view_insert_thumb_frames (GimvThumbView *tv, GList *filelist)
{
   GimvThumb  *thumb;
   GList *node, *new_thumb_list = NULL;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   if (!filelist) return;
   g_return_if_fail (tv->vfuncs);

   for (node = filelist; node; node = g_list_next (node)) {
      gchar *filename = node->data;

      if (file_exists(node->data)) {
         GimvImageInfo *info = gimv_image_info_get (filename);

         thumb = gimv_thumb_new (info);
         gimv_image_info_unref (info);
         gimv_thumb_set_parent_thumbview (thumb, tv);

         gimv_thumb_view_append_thumb_data (tv, thumb);

         new_thumb_list = g_list_append (new_thumb_list, thumb);
      }
   }

   gimv_thumb_view_sort_data (tv);

   for (node = new_thumb_list; node; node = g_list_next (node))
      tv->vfuncs->insert_thumb (tv, node->data, tv->summary_mode);
   g_list_free(new_thumb_list);
}


static void
gimv_thumb_view_insert_thumb_frames_from_archive (GimvThumbView *tv,
                                                  FRArchive *archive)
{
   GimvThumb  *thumb;
   FRCommand *command;
   GList *node, *new_thumb_list = NULL;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   if (!archive) return;
   g_return_if_fail (tv->vfuncs);

   command = archive->command;
   g_return_if_fail (command);

   for (node = command->file_list; node; node = g_list_next (node)) {
      GimvImageInfo *info = node->data;

      if (!info) continue;

      /* detect filename extension */
      if (conf.detect_filetype_by_ext
          && !gimv_image_detect_type_by_ext (info->filename)
          && !fr_archive_utils_get_file_name_ext (info->filename))
      {
         continue;
      }
      
      thumb = gimv_thumb_new (info);
      gimv_thumb_set_parent_thumbview (thumb, tv);

      gimv_thumb_view_append_thumb_data (tv, thumb);

      new_thumb_list = g_list_append (new_thumb_list, thumb);
   }

   gimv_thumb_view_sort_data (tv);

   for (node = new_thumb_list; node; node = g_list_next (node))
      tv->vfuncs->insert_thumb (tv, node->data, tv->summary_mode);
   g_list_free(new_thumb_list);
}


static gchar *
get_uri_list (GList *thumblist)
{
   gchar *path;
   gchar *uri;
   GList *node;
   GimvThumb *thumb;
 
   if (!thumblist) return NULL;

   uri = g_strdup ("");
   node = thumblist;
   while (node) {
      gchar *filename = NULL;

      thumb = node->data;

      filename = gimv_image_info_get_path_with_archive (thumb->info);
      path = g_strconcat (uri, "file://", filename, "\r\n", NULL);
      g_free (filename);
      g_free (uri);
      uri = g_strdup (path);
      g_free (path);

      node = g_list_next (node);
   }

   return uri;
}


typedef struct ChangeImageData_Tag
{
   GimvThumbWin  *tw;
   GimvImageWin  *iw;
   GimvImageView *iv;
   GimvThumb     *thumb;
} ChangeImageData;


static gboolean
change_image_idle (gpointer user_data)
{
   ChangeImageData *data = user_data;

   if (data->iw) {
      gimv_image_win_change_image (data->iw, data->thumb->info);
   } else {
      gimv_image_view_change_image (data->iv, data->thumb->info);
   }

   return FALSE;
}


static GList *
next_image (GimvImageView *iv, gpointer list_owner,
            GList *current, gpointer data)
{
   GimvImageWin *iw = data;
   GList *next, *node;
   GimvThumb *thumb;
   GimvThumbView *tv = list_owner;
   GimvThumbWin *tw;
   ChangeImageData *change_data;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (GimvThumbViewList, tv);
   g_return_val_if_fail (node, NULL);

   node = g_list_find (tv->thumblist, current->data);
   g_return_val_if_fail (node, NULL);

   next = g_list_next (current);
   if (!next) next = g_list_first (current);
   g_return_val_if_fail (next, NULL);

   while (next && next != current) {
      GimvThumb *nthumb = next->data;
      GList *temp;

      if (!gimv_image_info_is_dir (nthumb->info)
          && !gimv_image_info_is_archive (nthumb->info))
      {
         break;
      }

      temp = g_list_next (next);
      if (!temp)
         next = g_list_first (next);
      else
         next = temp;
   }
   if (!next) next = current;

   thumb = next->data;
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   tw = tv->tw;
   g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), NULL);

   gimv_thumb_view_set_selection_all (tv, FALSE);
   gimv_thumb_view_set_selection (thumb, TRUE);
   gimv_thumb_view_adjust (tv, thumb);

   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      gboolean success =  gimv_thumb_view_extract_archive_file (thumb);
      g_return_val_if_fail (success, next);
   }

   change_data = g_new0 (ChangeImageData, 1);
   change_data->tw = tw;
   change_data->iw = iw;
   change_data->iv = iv;
   change_data->thumb = thumb;
   gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                      change_image_idle,
                      NULL,
                      change_data,
                      (GtkDestroyNotify) g_free);

   if (!iw)
      gimv_comment_view_change_file (tw->cv, thumb->info);

   return next;
}


static GList *
prev_image (GimvImageView *iv, gpointer list_owner,
            GList *current, gpointer data)
{
   GimvImageWin *iw = data;
   GList *prev, *node;
   GimvThumb *thumb;
   GimvThumbView *tv = list_owner;
   GimvThumbWin *tw;
   ChangeImageData *change_data;

   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (GimvThumbViewList, tv);
   g_return_val_if_fail (node, NULL);

   node = g_list_find (tv->thumblist, current->data);
   g_return_val_if_fail (node, NULL);

   prev = g_list_previous (current);
   if (!prev) prev = g_list_last (current);
   g_return_val_if_fail (prev, NULL);

   while (prev && prev != current) {
      GimvThumb *nthumb = prev->data;
      GList *temp;

      if (!gimv_image_info_is_dir (nthumb->info)
          && !gimv_image_info_is_archive (nthumb->info))
      {
         break;
      }

      temp = g_list_previous (prev);
      if (!temp)
         prev = g_list_last (prev);
      else
         prev = temp;
   }
   if (!prev) prev = current;

   thumb = prev->data;
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   tw = tv->tw;
   g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), NULL);

   gimv_thumb_view_set_selection_all (tv, FALSE);
   gimv_thumb_view_set_selection (thumb, TRUE);
   gimv_thumb_view_adjust (tv, thumb);

   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      gboolean success = gimv_thumb_view_extract_archive_file (thumb);
      g_return_val_if_fail (success, prev);
   }

   change_data = g_new0 (ChangeImageData, 1);
   change_data->tw = tw;
   change_data->iw = iw;
   change_data->iv = iv;
   change_data->thumb = thumb;
   gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                      change_image_idle,
                      NULL,
                      change_data,
                      (GtkDestroyNotify) g_free);

   if (!iw)
      gimv_comment_view_change_file (tw->cv, thumb->info);

   return prev;
}


static GList *
nth_image (GimvImageView *iv,
           gpointer list_owner,
           GList *current,
           guint nth,
           gpointer data)
{
   GimvImageWin *iw = data;
   GList *node;
   GimvThumb *thumb;
   GimvThumbView *tv = list_owner;
   GimvThumbWin *tw;
   ChangeImageData *change_data;

   /* check check chek ... */
   g_return_val_if_fail (GIMV_IS_IMAGE_VIEW (iv), NULL);
   g_return_val_if_fail (current, NULL);

   node = g_list_find (GimvThumbViewList, tv);
   g_return_val_if_fail (node, NULL);

   node = g_list_find (tv->thumblist, current->data);
   g_return_val_if_fail (node, NULL);

   tw = tv->tw;
   g_return_val_if_fail (tw, NULL);

   /* get nth of list */
   node = g_list_nth (tv->thumblist, nth);
   g_return_val_if_fail (node, NULL);

   thumb = node->data;
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), NULL);

   gimv_thumb_view_set_selection_all (tv, FALSE);
   gimv_thumb_view_set_selection (thumb, TRUE);
   gimv_thumb_view_adjust (tv, thumb);

   /* show image */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      gboolean success = gimv_thumb_view_extract_archive_file (thumb);
      g_return_val_if_fail (success, node);
   }

   change_data = g_new0 (ChangeImageData, 1);
   change_data->tw = tw;
   change_data->iw = iw;
   change_data->iv = iv;
   change_data->thumb = thumb;
   gtk_idle_add_full (GTK_PRIORITY_DEFAULT,
                      change_image_idle,
                      NULL,
                      change_data,
                      (GtkDestroyNotify) g_free);

   if (!iw)
      gimv_comment_view_change_file (tw->cv, thumb->info);

   return node;
}


static void
cb_imageview_thumbnail_created (GimvImageView *iv,
                                GimvImageInfo *info,
                                GimvThumbView *tv)
{
   GList *node;
   GimvThumb *thumb = NULL;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (info);
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = tv->thumblist;
   while (node) {
      thumb = node->data;
      node = g_list_next (node);

      if (gimv_image_info_is_same (thumb->info, info))
         break;
      else
         thumb = NULL;
   }

   if (thumb)
      gimv_thumb_view_refresh_thumbnail (tv, thumb, LOAD_CACHE);
}


static void
remove_list (GimvImageView *iv, gpointer list_owner, gpointer data)
{
   GimvThumbView *tv = list_owner;
   GList *node;

   g_return_if_fail (GIMV_IS_IMAGE_VIEW (iv));
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   g_signal_handlers_disconnect_by_func (
      G_OBJECT (iv),
      G_CALLBACK (cb_imageview_thumbnail_created),
      tv);

   node = g_list_find (GimvThumbViewList, tv);
   if (!node) return;

   if (tv->priv)
      tv->priv->related_image_view
         = g_list_remove (tv->priv->related_image_view, iv);
}


static void
gimv_thumb_view_button_action (GimvThumbView *tv,
                               GimvThumb *thumb,
                               GdkEventButton *event,
                               gint num)
{
   GimvThumbWin *tw;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tw = tv->tw;
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));

   if (gimv_image_info_is_dir (thumb->info)
       || gimv_image_info_is_archive (thumb->info))
   {
      switch (abs (num)) {
      case ACTION_OPEN_AUTO:
         num = ACTION_OPEN_AUTO_FORCE;
         break;
      case ACTION_OPEN_IN_PREVIEW:
         num = ACTION_OPEN_IN_PREVIEW_FORCE;
         break;
      case ACTION_OPEN_IN_SHARED_WIN:
         num = ACTION_OPEN_IN_SHARED_WIN_FORCE;
         break;
      default:
         break;
      }
   }

   switch (abs (num)) {
   case ACTION_POPUP:
      gimv_thumb_view_popup_menu (tv, thumb, event);

      /* FIXME */
      if (GIMV_IS_SCROLLED (GTK_BIN (tv->container)->child))
         gimv_scrolled_stop_auto_scroll (GIMV_SCROLLED (GTK_BIN (tv->container)->child));

      break;

   case ACTION_OPEN_AUTO:
      if (tw->show_preview || gimv_image_win_get_shared_window())
         gimv_thumb_view_open_image (tv, thumb,
                                     GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO);
      break;

   case ACTION_OPEN_AUTO_FORCE:
      gimv_thumb_view_open_image (tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO);
      break;

   case ACTION_OPEN_IN_PREVIEW:
      if (tw->show_preview)
         gimv_thumb_view_open_image (tv, thumb,
                                     GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW);
      break;

   case ACTION_OPEN_IN_PREVIEW_FORCE:
      if (!tw->show_preview)
         gimv_thumb_win_open_preview (tw);
      gimv_thumb_view_open_image (tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW);
      break;

   case ACTION_OPEN_IN_NEW_WIN:
      gimv_thumb_view_open_image (tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_NEW_WIN);
      break;

   case ACTION_OPEN_IN_SHARED_WIN:
      if (gimv_image_win_get_shared_window())
         gimv_thumb_view_open_image (tv, thumb,
                                     GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN);
      break;

   case ACTION_OPEN_IN_SHARED_WIN_FORCE:
      gimv_thumb_view_open_image (tv, thumb,
                                  GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN);
      break;

   default:
      break;
   }
}


static GtkWidget *
create_progs_submenu (GimvThumbView *tv)
{
   GtkWidget *menu;
   GtkWidget *menu_item;
   gint i, conf_num = sizeof (conf.progs) / sizeof (conf.progs[0]);
   gchar **pair;

   menu = gtk_menu_new();

   /* count items num */
   for (i = 0; i < conf_num; i++) {
      if (!conf.progs[i]) continue;

      pair = g_strsplit (conf.progs[i], ",", 3);

      if (pair[0] && pair[1]) {
         gchar *label;

         if (pair[2] && !strcasecmp (pair[2], "TRUE"))
            label = g_strconcat (pair[0], "...", NULL);
         else
            label = g_strdup (pair[0]);

         menu_item = gtk_menu_item_new_with_label (label);
         g_object_set_data (G_OBJECT (menu_item), "num", GINT_TO_POINTER (i));
         g_signal_connect (G_OBJECT (menu_item), "activate",
                           G_CALLBACK (cb_open_image_by_external), tv);
         gtk_menu_append (GTK_MENU (menu), menu_item);
         gtk_widget_show (menu_item);

         g_free (label);
      }

      g_strfreev (pair);
   }

   return menu;
}


void
gimv_thumb_view_open_image (GimvThumbView *tv, GimvThumb *thumb, gint type)
{
   GimvThumbWin *tw;
   GimvImageWin *iw = NULL;
   GimvImageView   *iv = NULL;
   GList *current, *node;
   const gchar *image_name, *ext;
   gchar *filename, *tmpstr;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv) && thumb);

   tw = tv->tw;
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));

   image_name = gimv_image_info_get_path (thumb->info);
   g_return_if_fail (image_name && *image_name);
   filename = g_strdup (image_name);

   if (!strcmp ("..", g_basename (filename))) {
      tmpstr = filename;
      filename = g_dirname (filename);
      g_free (tmpstr);
      if (filename) {
         tmpstr = filename;
         filename = g_dirname (filename);
         g_free (tmpstr);
      }
      tmpstr = NULL;
   }

   /* open directory */
   if (isdir (filename)) {
#warning FIXME!!!! Use this tab?
      open_dir_images (filename, tw, NULL, LOAD_CACHE, conf.scan_dir_recursive);
      g_free (filename);
      return;
   }

   /* extract image in archive */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      gboolean success = gimv_thumb_view_extract_archive_file (thumb);
      if (!success) {
         g_free (filename);
         return;
      }
   }

   /* open archive */
   ext = fr_archive_utils_get_file_name_ext (filename);
   if (ext) {
      open_archive_images (filename, tw, NULL, LOAD_CACHE);
      g_free (filename);
      return;
   }

   /* open the image */
   if ((type == GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO && tw->show_preview)
       || type == GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW)
   {
      iv = tw->iv;
      gimv_comment_view_change_file (tw->cv, thumb->info);
      gimv_image_view_change_image (iv, thumb->info);

   } else if (gimv_image_win_get_shared_window() &&
              ((type == GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO && !conf.imgwin_open_new_win)
               ||type == GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN))
   {
      iw = gimv_image_win_open_shared_window (thumb->info);
      if (iw)
         iv = iw->iv;

   } else {
      if (type == GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN) {
         iw = gimv_image_win_open_shared_window (thumb->info);
      } else if (type == GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO) {
         iw = gimv_image_win_open_window_auto (thumb->info);
      } else {
         iw = gimv_image_win_open_window (thumb->info);
      }
      if (iw)
         iv = iw->iv;
   }

   if (iv && g_list_find (gimv_image_view_get_list(), iv)) {
      current = g_list_find (tv->thumblist, thumb);
      gimv_image_view_set_list (iv, tv->thumblist, current,
                                (gpointer) tv,
                                next_image,
                                prev_image,
                                nth_image,
                                remove_list,
                                iw);
      g_signal_connect (G_OBJECT (iv), "thumbnail_created",
                        G_CALLBACK (cb_imageview_thumbnail_created), tv);
      node = g_list_find (tv->priv->related_image_view, iv);
      if (!node) {
         tv->priv->related_image_view
            = g_list_append (tv->priv->related_image_view, iv);
      }
   }

   g_free (filename);
}


void
gimv_thumb_view_popup_menu (GimvThumbView *tv, GimvThumb *thumb,
                            GdkEventButton *event)
{
   GtkWidget *popup_menu, *progs_submenu, *scripts_submenu;
   GList *thumblist = NULL, *node;
   guint n_menu_items;
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;
   gchar *dirname;
   guint button;
   guint32 time;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (event) {
      button = event->button;
      time = gdk_event_get_time ((GdkEvent *) event);
   } else {
      button = 0;
      time = GDK_CURRENT_TIME;
   }

   if (tv->popup_menu) {
      gtk_widget_unref (tv->popup_menu);
      tv->popup_menu = NULL;
   }

   thumblist = node = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist) return;

   if (!thumb) thumb = thumblist->data;
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   /* create popup menu */
   n_menu_items = sizeof(thumb_button_popup_items)
      / sizeof(thumb_button_popup_items[0]) - 1;
   popup_menu = menu_create_items(GTK_WIDGET (tv->tw),
                                  thumb_button_popup_items, n_menu_items,
                                  "<ThumbnailButtonPop>", tv);

   /* set sensitive */
   ifactory = gtk_item_factory_from_widget (popup_menu);

   menuitem = gtk_item_factory_get_item (ifactory, "/Open");
   if (!gimv_image_info_is_dir (thumb->info)
       && !gimv_image_info_is_archive (thumb->info))
   {
      gtk_widget_hide (menuitem);
   }

   menuitem = gtk_item_factory_get_item (ifactory, "/Open in New Window");
   if (gimv_image_info_is_dir (thumb->info)
       || gimv_image_info_is_archive (thumb->info))
   {
      gtk_widget_hide (menuitem);
   }

   menuitem = gtk_item_factory_get_item (ifactory, "/Open in Shared Window");
   if (gimv_image_info_is_dir (thumb->info)
       || gimv_image_info_is_archive (thumb->info))
   {
      gtk_widget_hide (menuitem);
   } else if (g_list_length (thumblist) > 1) {
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   menuitem = gtk_item_factory_get_item (ifactory, "/Open in External Program");
   if (/* thumbnail_is_dir (thumb) || thumbnail_is_archive (thumb) */
      gimv_image_info_is_in_archive (thumb->info))
   {
      gtk_widget_hide (menuitem);
   } else {
      progs_submenu = create_progs_submenu (tv);
      menu_set_submenu (popup_menu, "/Open in External Program", progs_submenu);
   }

   menuitem = gtk_item_factory_get_item (ifactory, "/Scripts");
   if (/* thumbnail_is_dir (thumb) || thumbnail_is_archive (thumb) */
      gimv_image_info_is_in_archive (thumb->info))
   {
      gtk_widget_hide (menuitem);
   } else {
      scripts_submenu = create_scripts_submenu (tv);
      menu_set_submenu (popup_menu, "/Scripts", scripts_submenu);
   }

#warning FIXME!!
   menuitem = gtk_item_factory_get_item (ifactory, "/Update Thumbnail");
   if (gimv_image_info_is_dir (thumb->info)
       || gimv_image_info_is_archive (thumb->info)
       || gimv_image_info_is_movie (thumb->info))
   {
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   menuitem = gtk_item_factory_get_item (ifactory, "/Remove from List");
   if (tv->progress || tv->mode != GIMV_THUMB_VIEW_MODE_COLLECTION) {
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   dirname = g_dirname (gimv_image_info_get_path (thumb->info));
   if (g_list_length (thumblist) < 1 || !iswritable (dirname)
       || tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE)
   {
      if (g_list_length (thumblist) < 1) {
         menuitem = gtk_item_factory_get_item (ifactory, "/Property...");
         gtk_widget_set_sensitive (menuitem, FALSE);
      }
      menuitem = gtk_item_factory_get_item (ifactory, "/Move Files To...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Rename...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Remove file...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }
   g_free (dirname);

   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Copy Files To...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Link Files To...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   if (g_list_length (thumblist) > 1) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Property...");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Rename...");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

#ifdef ENABLE_EXIF
   {
      const gchar *img_name = gimv_image_info_get_path (thumb->info);
      const gchar *format = gimv_image_detect_type_by_ext (img_name);
      menuitem = gtk_item_factory_get_item (ifactory, "/Scan EXIF Data...");
#warning FIXME!!
      if (!format || !*format || g_ascii_strcasecmp(format, "image/jpeg")) {
         gtk_widget_hide (menuitem);
      }
      if (g_list_length (thumblist) > 1
          || tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE)
      {
         gtk_widget_set_sensitive (menuitem, FALSE);
      }
   }
#endif /* ENABLE_EXIF */

   /* popup */
   gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL,
                  NULL, NULL, button, time);

   tv->popup_menu = popup_menu;
   g_object_ref (G_OBJECT (tv->popup_menu));
   gtk_object_sink (GTK_OBJECT (tv->popup_menu));

   g_list_free (thumblist);
}


void
gimv_thumb_view_file_operate (GimvThumbView *tv, FileOperateType type)
{
   GList *list = NULL;
   gboolean success = FALSE;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   /* FIXME!! */
   /* not implemented yet */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) return;

   list = gimv_thumb_view_get_selected_file_list (tv);
   if (!list) return;

   if (!previous_dir && tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
      previous_dir = g_strdup (tv->priv->dirname);
   }

   success = files2dir_with_dialog (list, &previous_dir, type,
                                    GTK_WINDOW (tv->tw));

   if (success && type == FILE_MOVE) {
      gimv_thumb_view_refresh_list (tv);
   }

   /* update dest side file list */
   tv = gimv_thumb_view_find_opened_dir (previous_dir);
   if (tv) {
      gimv_thumb_view_refresh_list (tv);
   }

   g_list_foreach (list, (GFunc) g_free, NULL);
   g_list_free (list);
}


void
gimv_thumb_view_rename_file (GimvThumbView *tv)
{
   GimvThumb *thumb;
   const gchar *cache_type;
   GList *thumblist;
   const gchar *src_file;
   gchar *dest_file, *dest_path;
   gchar *src_cache_path, *dest_cache_path;
   gchar *src_comment, *dest_comment;
   gchar message[BUF_SIZE], *dirname;
   ConfirmType confirm;
   gboolean exist;
   struct stat dest_st;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   /* FIXME!! */
   /* not implemented yet */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) return;

   thumblist = gimv_thumb_view_get_selection_list (tv);
   if (!thumblist || g_list_length (thumblist) > 1) return;

   thumb = thumblist->data;
   if (!thumb) return;

   {   /********** convert charset **********/
      gchar *tmpstr, *src_file_internal;

      src_file = g_basename(gimv_image_info_get_path (thumb->info));
      src_file_internal = charset_to_internal (src_file,
                                               conf.charset_filename,
                                               conf.charset_auto_detect_fn,
                                               conf.charset_filename_mode);

      tmpstr = gtkutil_popup_textentry (_("Rename a file"),
                                        _("New file name: "),
                                        src_file_internal, NULL, -1, 0,
                                        GTK_WINDOW (tv->tw));
      g_free (src_file_internal);
      src_file_internal = NULL;

      if (!tmpstr) return;

      dest_file = charset_internal_to_locale (tmpstr);
      g_free (tmpstr);
   }

   if (!strcmp (src_file, dest_file)) goto ERROR0;

   dirname = g_dirname (gimv_image_info_get_path (thumb->info));
   dest_path = g_strconcat (dirname, "/", g_basename (dest_file), NULL);
   g_free (dirname);
   exist = !lstat(dest_path, &dest_st);
   if (exist) {
      {   /********** convert charset **********/
         gchar *tmpstr;

         tmpstr = charset_to_internal (src_file,
                                       conf.charset_filename,
                                       conf.charset_auto_detect_fn,
                                       conf.charset_filename_mode);

         g_snprintf (message, BUF_SIZE,
                     _("File exist : %s\n\n"
                       "Overwrite?"), tmpstr);

         g_free (tmpstr);
      }

      confirm = gtkutil_confirm_dialog (_("File exist!!"), message, 0,
                                        GTK_WINDOW (tv->tw));
      if (confirm == CONFIRM_NO) goto ERROR1;
   }

   /* rename file!! */
   if (rename (gimv_image_info_get_path (thumb->info), dest_path) < 0) {
      {   /********** convert charset **********/
         gchar *tmpstr;

         tmpstr = charset_to_internal (gimv_image_info_get_path (thumb->info),
                                       conf.charset_filename,
                                       conf.charset_auto_detect_fn,
                                       conf.charset_filename_mode);

         g_snprintf (message, BUF_SIZE,
                     _("Faild to rename file :\n%s"),
                     tmpstr);

         g_free (tmpstr);
      }

      gtkutil_message_dialog (_("Error!!"), message,
                              GTK_WINDOW (tv->tw));
   }

   /* rename cache */
   cache_type = gimv_thumb_get_cache_type (thumb);
   if (cache_type) {
      src_cache_path 
         = gimv_thumb_cache_get_path (gimv_image_info_get_path (thumb->info),
                                      cache_type);
      dest_cache_path
         = gimv_thumb_cache_get_path (dest_path, cache_type);
      if (rename (src_cache_path, dest_cache_path) < 0)
         g_print (_("Faild to rename cache file :%s\n"), dest_path);
      g_free (src_cache_path);
      g_free (dest_cache_path);
   }

   /* rename comment */
   src_comment = gimv_comment_find_file (gimv_image_info_get_path (thumb->info));
   if (src_comment) {
      dest_comment = gimv_comment_get_path (dest_path);
      if (rename (src_comment, dest_comment) < 0)
         g_print (_("Faild to rename comment file :%s\n"), dest_comment);
      g_free (src_comment);
      g_free (dest_comment);      
   }

   gimv_thumb_view_refresh_list (tv);

ERROR1:
   g_free (dest_path);
ERROR0:
   g_free (dest_file);
}


gboolean
gimv_thumb_view_delete_files (GimvThumbView *tv)
{
   GimvThumbWin *tw;
   GList *selection;
   GList *filelist = NULL, *list;
   gboolean retval;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   /* FIXME!! */
   /* not implemented yet */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) return FALSE;

   tw = tv->tw;
   g_return_val_if_fail (GIMV_IS_THUMB_WIN (tw), FALSE);

   selection = gimv_thumb_view_get_selection_list (tv);
   if (!selection) return FALSE;

   /* convert to filename list */
   for (list = selection; list; list = g_list_next (list)) {
      GimvThumb *thumb = list->data;
      const gchar *filename;

      if (!thumb) continue;
      filename = gimv_image_info_get_path (thumb->info);
      if (filename && *filename)
         filelist = g_list_append (filelist, (gpointer) filename);
   }

   /* do delete */
   retval = delete_files (filelist, CONFIRM_ASK,
                          GTK_WINDOW (tv->tw));

   if (retval) {
      gimv_thumb_view_refresh_list (tv);
      if (tw->show_dirview)
         gimv_dir_view_refresh_list (tw->dv);
   }

   g_list_free (filelist);
   g_list_free (selection);

   return retval;
}


static GtkWidget *
create_scripts_submenu (GimvThumbView *tv)
{
   GtkWidget *menu;
   GtkWidget *menu_item;
   GList *tmplist = NULL, *filelist = NULL, *list;
   const gchar *dirlist;
   gchar **dirs;
   gint i, flags;

   menu = gtk_menu_new();

   if (conf.scripts_use_default_search_dir_list)
      dirlist = SCRIPTS_DEFAULT_SEARCH_DIR_LIST;
   else
      dirlist = conf.scripts_search_dir_list;

   if (!dirlist || !*dirlist) return NULL;

   dirs = g_strsplit (dirlist, ",", -1);
   if (!dirs) return NULL;

   flags = 0 | GETDIR_FOLLOW_SYMLINK;
   for (i = 0; dirs[i]; i++) {
      if (!*dirs || !isdir (dirs[i])) continue;
      get_dir (dirs[i], flags, &tmplist, NULL);
      filelist = g_list_concat (filelist, tmplist);
   }
   g_strfreev (dirs);

   for (list = filelist; list; list = g_list_next (list)) {
      gchar *filename = list->data;
      gchar *label;

      if (!filename || !*filename || !isexecutable(filename)) continue;

      if (conf.scripts_show_dialog)
         label = g_strconcat (g_basename (filename), "...", NULL);
      else
         label = g_strdup (g_basename (filename));

      menu_item = gtk_menu_item_new_with_label (label);
      g_object_set_data_full (G_OBJECT (menu_item),
                              "script",
                              g_strdup (filename),
                              (GtkDestroyNotify) g_free);
      g_signal_connect (G_OBJECT (menu_item),
                        "activate",
                        G_CALLBACK (cb_open_image_by_script),
                        tv);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);

      g_free (label);
   }

   g_list_foreach (filelist, (GFunc) g_free, NULL);
   g_list_free (filelist);

   return menu;
}


static void
gimv_thumb_view_reset_load_priority (GimvThumbView *tv)
{
   GList *node, *tmp_list = NULL;
   gboolean in_view;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   if (!tv->load_list) return;
   node = g_list_last (tv->load_list);
   g_return_if_fail (tv->vfuncs);

   if (!tv->vfuncs->is_in_view) return;

   while (node) {
      GimvThumb *thumb = node->data;
      node = g_list_previous (node);

      in_view = tv->vfuncs->is_in_view (tv, thumb);
      if (in_view) {
         tv->load_list = g_list_remove (tv->load_list, thumb);
         tmp_list = g_list_prepend (tmp_list, thumb);
      }
   }

   if (tmp_list)
      tv->load_list = g_list_concat (tmp_list, tv->load_list);
}


static void
gimv_thumb_view_set_scrollbar_callback (GimvThumbView *tv)
{
   GtkAdjustment *hadj, *vadj;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   hadj = gtk_scrolled_window_get_hadjustment (
               GTK_SCROLLED_WINDOW (tv->container));
   vadj = gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (tv->container));

   g_signal_connect (G_OBJECT (hadj),
                     "value_changed",
                     G_CALLBACK (cb_thumbview_scrollbar_value_changed),
                     tv);
   g_signal_connect (G_OBJECT (vadj),
                     "value_changed",
                     G_CALLBACK (cb_thumbview_scrollbar_value_changed),
                     tv);
}


static void
gimv_thumb_view_remove_scrollbar_callback (GimvThumbView *tv)
{
   GtkAdjustment *hadj, *vadj;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (tv->container));
   vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tv->container));

   g_signal_handlers_disconnect_by_func (
      G_OBJECT (hadj),
      G_CALLBACK (cb_thumbview_scrollbar_value_changed),
      tv);
   g_signal_handlers_disconnect_by_func (
      G_OBJECT (vadj),
      G_CALLBACK (cb_thumbview_scrollbar_value_changed),
      tv);
}


static GCompareFunc
gimv_thumb_view_get_compare_func (GimvThumbView *tv, gboolean *reverse)
{
   GimvSortItem item;
   GimvSortFlag flags;
   GCompareFunc func = NULL;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   item = gimv_thumb_win_get_sort_type (tv->tw, &flags);

   /* sort thumbnail */
   if (item == GIMV_SORT_NAME)
      func = comp_func_spel;
   else if (item == GIMV_SORT_SIZE)
      func = comp_func_size;
   else if (item == GIMV_SORT_ATIME)
      func = comp_func_atime;
   else if (item == GIMV_SORT_MTIME)
      func = comp_func_mtime;
   else if (item == GIMV_SORT_CTIME)
      func = comp_func_ctime;
   else if (item == GIMV_SORT_TYPE)
      func = comp_func_type;
   else if (item == GIMV_SORT_WIDTH)
      func = comp_func_width;
   else if (item == GIMV_SORT_HEIGHT)
      func = comp_func_height;
   else if (item == GIMV_SORT_AREA)
      func = comp_func_area;

   if (reverse) {
      if ((flags & GIMV_SORT_REVERSE))
         *reverse = TRUE;
      else
         *reverse = FALSE;
   }

   return func;
}



/******************************************************************************
 *
 *   Public Functions.
 *
 ******************************************************************************/
GList *
gimv_thumb_view_get_list (void)
{
   return GimvThumbViewList;
}


static void
gimv_thumb_view_destroy_dupl_win_relation (GimvDuplWin *sw,GimvThumbView *tv)
{
   g_return_if_fail (GIMV_IS_DUPL_WIN (sw));
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   gimv_dupl_win_unset_relation (sw);
   g_signal_handlers_disconnect_by_func (
      G_OBJECT (sw),
      G_CALLBACK (cb_dupl_win_destroy),
      tv);
}


const gchar *
gimv_thumb_view_get_path (GimvThumbView *tv)
{
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
      return tv->priv->dirname;
   } else if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      return tv->priv->archive->filename;
   }

   return NULL;
}


GimvThumbView *
gimv_thumb_view_find_opened_dir (const gchar *path)
{
   GList *node;

   if (!path) return NULL;

   for (node = GimvThumbViewList; node; node = g_list_next (node)) {
      GimvThumbView *tv  = node->data;

      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR &&
          tv->priv && tv->priv->dirname &&
          !strcmp (path, tv->priv->dirname))
      {
         return tv;
      }
   }

   return NULL;
}


GimvThumbView *
gimv_thumb_view_find_opened_archive (const gchar *path)
{
   GimvThumbView *tv;
   GList *node;

   if (!path) return NULL;

   node = g_list_first (GimvThumbViewList);
   while (node) {
      tv = node->data;
      if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE
          && tv->priv->archive
          && !strcmp (path, tv->priv->archive->filename))
      {
         return tv;
      }
      node = g_list_next (node);
   }

   return NULL;
}


void
gimv_thumb_view_sort_data (GimvThumbView *tv)
{
   GCompareFunc func;
   gboolean reverse = FALSE;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   func = gimv_thumb_view_get_compare_func (tv, &reverse);
   g_return_if_fail (func);

   tv->thumblist = g_list_sort (tv->thumblist, func);
   if (reverse)
      tv->thumblist = g_list_reverse (tv->thumblist);
}


/* #define ENABLE_THUMB_LOADER_TIMER 0 */
gboolean
gimv_thumb_view_load_thumbnails (GimvThumbView *tv, GList *loadlist,
                                 const gchar *dest_mode)
{
#ifdef ENABLE_THUMB_LOADER_TIMER
   GTimer *timer;
   gdouble etime;
   gulong mtime;
#endif /* ENABLE_THUMB_LOADER_TIMER */
   FilesLoader *files = tv->progress;
   gint pos;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv) && loadlist, FALSE);
   g_return_val_if_fail (plugin_list, FALSE);

#ifdef ENABLE_THUMB_LOADER_TIMER
   timer = g_timer_new ();
   g_timer_start (timer);
   g_print ("timer started\n");
#endif /* ENABLE_THUMB_LOADER_TIMER */

   g_return_val_if_fail (tv->vfuncs, FALSE);

   /* load thumbnail!! */
   tv->load_list = g_list_copy (loadlist);
   gimv_thumb_view_reset_load_priority (tv);

   tv->progress->window = GTK_WIDGET (tv->tw);
   tv->progress->progressbar = tv->tw->progressbar;
   tv->progress->num = g_list_length (loadlist);
   tv->progress->pos = 0;
   gimv_thumb_win_set_statusbar_page_info (tv->tw,
                                           GIMV_THUMB_WIN_CURRENT_PAGE);

   for (pos = 1; tv->load_list && files->status < CANCEL; pos++) {
      GimvThumb *thumb;

      if (tv->progress->pos % conf.thumbwin_redraw_interval == 0)
         while (gtk_events_pending()) gtk_main_iteration();

      thumb = tv->load_list->data;

      gimv_thumb_load (thumb, tv->thumb_size, files->thumb_load_type);

      if (tv->vfuncs->update_thumb)
         tv->vfuncs->update_thumb (tv, thumb, dest_mode);

      if(files->status < 0) {
         GimvThumbViewList = g_list_remove (GimvThumbViewList, tv);
         g_object_unref (G_OBJECT (tv));
         break;
      } else {
         /* update progress info */
         tv->progress->now_file = gimv_image_info_get_path (thumb->info);
         tv->progress->pos = pos;
         gimv_thumb_win_loading_update_progress (tv->tw,
                                                 GIMV_THUMB_WIN_CURRENT_PAGE);
      }

      tv->load_list = g_list_remove (tv->load_list, thumb);
   }

   if (tv->load_list) {
      g_list_free (tv->load_list);
      tv->load_list = NULL;
   }

   gtk_progress_set_show_text(GTK_PROGRESS(files->progressbar), FALSE);
   gtk_progress_bar_update (GTK_PROGRESS_BAR(files->progressbar), 0.0);

#ifdef ENABLE_THUMB_LOADER_TIMER
   g_timer_stop (timer);
   g_print ("timer stopped\n");
   etime = g_timer_elapsed (timer, &mtime);
   g_print ("elapsed time = %f sec\n", etime);
   g_timer_destroy (timer);
#endif /* ENABLE_THUMB_LOADER_TIMER */

   return TRUE;
}


gboolean
gimv_thumb_view_append_thumbnail (GimvThumbView *tv, FilesLoader *files,
                                  gboolean force)
{
   GList *loadlist = NULL;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv) && files, FALSE);
   g_return_val_if_fail (tv->vfuncs, FALSE);

   if (!tv || !files || (tv->mode == GIMV_THUMB_VIEW_MODE_DIR && !force)) {
      return FALSE;
   }

   tv->progress = files;

   gimv_thumb_view_insert_thumb_frames (tv, files->filelist);

   if (tv->vfuncs->get_load_list)
      loadlist = tv->vfuncs->get_load_list (tv);

   if (loadlist) {
      gimv_thumb_view_load_thumbnails (tv, loadlist, tv->summary_mode);
      g_list_free (loadlist);
   }

   tv->progress = NULL;

   if (files->status >= 0)
      gimv_thumb_win_set_statusbar_page_info (tv->tw,
                                              GIMV_THUMB_WIN_CURRENT_PAGE);

   return TRUE;
}


void
gimv_thumb_view_clear (GimvThumbView *tv)
{
   GimvThumb *thumb;
   GList *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (tv->vfuncs);
   g_return_if_fail (!tv->progress);

   if (tv->vfuncs->freeze)
      tv->vfuncs->freeze (tv);

   node = tv->thumblist;
   while (node) {
      thumb = node->data;
      node = g_list_next (node);
      if (!GIMV_IS_THUMB (thumb)) continue;

      tv->vfuncs->remove_thumb (tv, thumb);
      gimv_thumb_view_remove_thumb_data (tv, thumb);
      g_object_unref (G_OBJECT(thumb));
   }

   /* destroy relation */
   node = tv->priv->related_image_view;
   while (node) {
      GimvImageView *iv = node->data;
      node = g_list_next(node);
      gimv_image_view_remove_list (iv, tv);
   }
   g_list_free (tv->priv->related_image_view);
   tv->priv->related_image_view = NULL;

   g_list_foreach (tv->priv->related_dupl_win,
                   (GFunc) gimv_thumb_view_destroy_dupl_win_relation,
                   tv);
   g_list_free (tv->priv->related_dupl_win);
   tv->priv->related_dupl_win = NULL;

   if (tv->vfuncs->thaw)
      tv->vfuncs->thaw (tv);

   gimv_thumb_win_set_statusbar_page_info (tv->tw,
                                           GIMV_THUMB_WIN_CURRENT_PAGE);
}


void
gimv_thumb_view_refresh_thumbnail (GimvThumbView *tv, GimvThumb *thumb,
                                   ThumbLoadType type)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (GIMV_IS_THUMB (thumb));
   g_return_if_fail (tv->vfuncs);

   gimv_thumb_load (thumb, tv->thumb_size, type);

   if (tv->vfuncs->update_thumb)
      tv->vfuncs->update_thumb (tv, thumb, tv->summary_mode);
}


typedef struct _SetCursorData
{
   GimvThumbView *tv;
   GimvThumb *cursor;
} SetCursorData;


static gboolean
idle_set_cursor(gpointer user_data)
{
   SetCursorData *data = user_data;

   if (data->cursor) {
      gimv_thumb_view_set_focus (data->tv, data->cursor);
      gimv_thumb_view_adjust (data->tv, data->cursor);
   }   

   g_free (data);

   return FALSE;
}


gboolean
gimv_thumb_view_refresh_list (GimvThumbView *tv)
{
   FilesLoader *files;
   GList *selection, *thumbnode, *node;
   GimvThumb *thumb, *dest_cur[4];
   gchar *filename;
   gboolean exist = FALSE;
   struct stat st;
   gint i, flags, n_dest_cur;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (tv->vfuncs, FALSE);

   if (tv->progress)
      return FALSE;

   /* get dest cursor candiates */
   n_dest_cur = sizeof (dest_cur) / sizeof (GimvThumb *);
   for (i = 0; i < n_dest_cur; i++)
      dest_cur[i] = NULL;

   dest_cur[0] = gimv_thumb_view_get_focus (tv);
   if (dest_cur[0]) {
      node = g_list_find (tv->thumblist, dest_cur[0]);
      if (node) {
         if (g_list_next (node))
            dest_cur[1] = g_list_next (node)->data;
         else if (g_list_previous (node))
            dest_cur[1] = g_list_previous (node)->data;
      }
   }

   selection = gimv_thumb_view_get_selection_list (tv);

   if (selection) {
      node = g_list_find (tv->thumblist, selection->data);
      if (node) {
         dest_cur[2] = node->data;
         if (g_list_previous (node))
            dest_cur[3] = g_list_previous (node)->data;
      }

      for (; node; node = g_list_next (node)) {
         if (!g_list_find (selection, node->data)) {
            dest_cur[3] = node->data;
            break;
         }
      }
   }

   g_list_free (selection);

   /* search */
   files = files_loader_new ();

   if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
      if (tv->vfuncs->freeze)
         tv->vfuncs->freeze (tv);

      flags = GETDIR_FOLLOW_SYMLINK;
      if (conf.read_dotfile)
         flags = flags | GETDIR_READ_DOT;
      if (conf.detect_filetype_by_ext)
         flags = flags | GETDIR_DETECT_EXT;
      if (conf.thumbview_show_archive)
         flags = flags | GETDIR_GET_ARCHIVE;

      get_dir (tv->priv->dirname, flags, &files->filelist, &files->dirlist);

      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR && conf.thumbview_show_dir) {
         gchar *parent = g_strconcat (tv->priv->dirname, "..", NULL);
         files->filelist = g_list_concat (files->dirlist, files->filelist);
         files->filelist = g_list_prepend (files->filelist, parent);
         files->dirlist = NULL;
      }

      thumbnode = g_list_first (tv->thumblist);
      while (thumbnode) {
         thumb = thumbnode->data;
         thumbnode = g_list_next (thumbnode);

         /* remove same file from files->filelist */
         node = g_list_first (files->filelist);
         while (node) {
            filename = node->data;
            if (!strcmp (filename, gimv_image_info_get_path (thumb->info))) {
               /* check modification time */
               if (!stat (filename, &st) &&
                   ((thumb->info->st.st_mtime != st.st_mtime) ||
                    (thumb->info->st.st_ctime != st.st_ctime)))
               {
                  exist = FALSE;
               } else {
                  exist = TRUE;
                  files->filelist = g_list_remove (files->filelist, filename);
                  g_free (filename);
               }
               break;
            }
            node = g_list_next (node);
         }

         /* remove obsolete data */
         if (!exist) {
            tv->vfuncs->remove_thumb (tv, thumb);
            gimv_thumb_view_remove_thumb_data (tv, thumb);
            g_object_unref (G_OBJECT (thumb));
         }

         exist = FALSE;
      }

      if (tv->vfuncs->thaw)
         tv->vfuncs->thaw (tv);

      /* append new files */
      if (files->filelist)
         gimv_thumb_view_append_thumbnail (tv, files, TRUE);

   } else if (tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
   }

   files_loader_delete (files);

   gimv_thumb_win_set_statusbar_page_info (tv->tw,
                                           GIMV_THUMB_WIN_CURRENT_PAGE);

   /* set cursor position */
   for (i = 0; i < n_dest_cur; i++) {
      thumb = dest_cur[i];

      if (thumb && g_list_find (tv->thumblist, thumb)) {
         SetCursorData *data = g_new0 (SetCursorData, 1);
         data->tv = tv;
         data->cursor = thumb;
         gtk_idle_add (idle_set_cursor, data);
         break;
      }
   }

   /* check relation */
   node = tv->priv->related_image_view;
   while (node) {
      GimvImageView *iv = node->data;
      GList *lnode;

      node = g_list_next (node);

      lnode = gimv_image_view_image_list_current (iv);
      if (!lnode) continue;

      if (!g_list_find (tv->thumblist, lnode->data)) {
         gimv_image_view_remove_list (iv, (gpointer) tv);
      }
   }

   return TRUE;
}


gint
gimv_thumb_view_refresh_list_idle (gpointer data)
{
   GimvThumbView *tv = data;
   gimv_thumb_view_refresh_list (tv);
   return FALSE;
}


void
gimv_thumb_view_change_summary_mode (GimvThumbView *tv, const gchar *mode)
{
   GimvThumbWin *tw;
   GList *node, *loadlist = NULL;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tw = tv->tw;
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));
   g_return_if_fail (tv->vfuncs);

   node = g_list_find (GimvThumbViewList, tv);
   if (!node) return;

   gimv_thumb_view_set_widget (tv, tw, tv->container, mode);

   if (tv->vfuncs->get_load_list)
   loadlist = tv->vfuncs->get_load_list (tv);

   /* reload images if needed */
   if (loadlist) {
      FilesLoader *files;

      files = files_loader_new ();
      tv->progress = files;

      gimv_thumb_view_load_thumbnails (tv, loadlist, tv->summary_mode);

      g_list_free (loadlist);
      files_loader_delete (files);
      tv->progress = NULL;

      gimv_thumb_win_set_statusbar_page_info (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   }
}


void
gimv_thumb_view_adjust (GimvThumbView *tv, GimvThumb *thumb)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (tv->vfuncs);

   if (tv->vfuncs->adjust_position)
      tv->vfuncs->adjust_position (tv, thumb);

   return;
}


GList *
gimv_thumb_view_get_selection_list (GimvThumbView *tv)
{
   GList *list = NULL, *node;
   GimvThumb *thumb;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   node = tv->thumblist;
   if (!node) return NULL;

   while (node) {
      thumb = node->data;

      if (thumb->selected)
         list = g_list_append (list, thumb);

      node = g_list_next (node);
   }

   return list;
}


GList *
gimv_thumb_view_get_selected_file_list (GimvThumbView *tv)
{
   GList *list = NULL, *filelist = NULL, *node;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);

   list = gimv_thumb_view_get_selection_list (tv);
   if (!list) return NULL;

   node = list;
   while (node) {
      GimvThumb *thumb = node->data;

      if (thumb->info)
         filelist
            = g_list_append (filelist,
                             g_strdup (gimv_image_info_get_path (thumb->info)));

      node = g_list_next (node);
   }

   return filelist;
}


gboolean
gimv_thumb_view_set_selection (GimvThumb *thumb, gboolean select)
{
   GimvThumbView *tv;

   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   tv = gimv_thumb_get_parent_thumbview (thumb);
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);

   if ((thumb->selected && select) || (!thumb->selected && !select))
      return TRUE;

   g_return_val_if_fail (tv->vfuncs, FALSE);

   if (tv->vfuncs->set_selection)
      tv->vfuncs->set_selection (tv, thumb, select);
   else
      return FALSE;

   return TRUE;
}


gboolean
gimv_thumb_view_set_selection_all (GimvThumbView *tv, gboolean select)
{
   GimvThumb *thumb;
   GList *node;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (tv->thumblist, FALSE);

   node = tv->thumblist;
   while (node) {
      thumb = node->data;
      gimv_thumb_view_set_selection (thumb, select);
      node = g_list_next (node);
   }

   return TRUE;
}


void
gimv_thumb_view_set_focus (GimvThumbView *tv, GimvThumb *thumb)
{
   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (tv->vfuncs);

   if (tv->vfuncs->set_focus)
      tv->vfuncs->set_focus (tv, thumb);
}


GimvThumb *
gimv_thumb_view_get_focus (GimvThumbView *tv)
{
   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), NULL);
   g_return_val_if_fail (tv->vfuncs, NULL);

   if (tv->vfuncs->get_focus)
      return tv->vfuncs->get_focus (tv);

   return NULL;
}


gboolean
gimv_thumb_view_set_selection_multiple (GimvThumbView *tv, GimvThumb *thumb,
                                        gboolean reverse, gboolean clear)
{
   GimvThumb *thumb_tmp;
   GList *node, *current_node;
   gboolean retval = FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (GIMV_IS_THUMB (thumb), FALSE);

   node = current_node = g_list_find (tv->thumblist, thumb);
   if (reverse)
      node = g_list_previous (node);
   else
      node = g_list_next (node);

   while (node) {
      thumb_tmp = node->data;
      if (thumb_tmp->selected) {
         if (clear)
            gimv_thumb_view_set_selection_all (tv, FALSE);
         while (TRUE) {
            thumb_tmp = node->data;
            gimv_thumb_view_set_selection (thumb_tmp, TRUE);
            if (node == current_node) break;
            if (reverse)
               node = g_list_next (node);
            else
               node = g_list_previous (node);
         }
         retval = TRUE;
         break;
      }
      if (reverse)
         node = g_list_previous (node);
      else
         node = g_list_next (node);
   }

   return retval;
}


void
gimv_thumb_view_grab_focus (GimvThumbView *tv)
{
   g_return_if_fail(GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail(GTK_IS_BIN(tv->container));
   gtk_widget_grab_focus (GTK_BIN (tv->container)->child);
}


void
gimv_thumb_view_find_duplicates (GimvThumbView *tv, GimvThumb *thumb,
                                 const gchar *type)
{
   GimvDuplFinder *finder;
   GimvDuplWin *sw = NULL;   
   GimvThumbWin *tw;
   GList *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   tw = tv->tw;
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));

   /* create window */
   sw = gimv_dupl_win_new (tv->thumb_size);
   g_signal_connect (G_OBJECT (sw), "destroy",
                     G_CALLBACK (cb_dupl_win_destroy),
                     tv);
   gimv_dupl_win_set_relation (sw, tv);
   tv->priv->related_dupl_win
      = g_list_append (tv->priv->related_dupl_win, sw);

   /* set finder */
   finder = sw->finder;
   gimv_dupl_finder_set_algol_type (finder, type);

   for (node = tv->thumblist; node; node = g_list_next (node)) {
      gimv_dupl_finder_append_dest (finder, node->data);
   }

   gimv_dupl_finder_start (finder);
}


void
gimv_thumb_view_reset_tab_label (GimvThumbView *tv, const gchar *title)
{
   gchar *tmpstr;
   const gchar *filename;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   /* set tab title */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
      if (title) {
         g_free (tv->tabtitle);
         tv->tabtitle = g_strdup (title);
      }

   } else {
      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
         g_return_if_fail (tv->priv->dirname && *tv->priv->dirname);
         filename = tv->priv->dirname;
      } else if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
         g_return_if_fail (tv->priv->archive);
         g_return_if_fail (tv->priv->archive->filename
                           && *tv->priv->archive->filename);
         filename = tv->priv->archive->filename;
      } else {
         return;
      }

      if (conf.thumbwin_tab_fullpath) {
         tmpstr = fileutil_home2tilde (filename);
      } else {
         if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
            tmpstr = g_strdup (g_basename (filename));
         } else {
            gchar *dirname = g_dirname (filename);
            tmpstr = fileutil_dir_basename (dirname);
            g_free (dirname);
         }
      }

      g_free (tv->tabtitle);
      tv->tabtitle = charset_to_internal (tmpstr,
                                          conf.charset_filename,
                                          conf.charset_auto_detect_fn,
                                          conf.charset_filename_mode);

      g_free (tmpstr);

   }

   gimv_thumb_win_set_tab_label_text (tv->container, tv->tabtitle);
}


G_DEFINE_TYPE (GimvThumbView, gimv_thumb_view, G_TYPE_OBJECT)


static void
gimv_thumb_view_class_init (GimvThumbViewClass *klass)
{
   GObjectClass *gobject_class;

   gimv_thumb_view_get_summary_mode_list ();

   gobject_class = (GObjectClass *) klass;

   gobject_class->dispose = gimv_thumb_view_dispose;
}


static void
gimv_thumb_view_init (GimvThumbView *tv)
{
   tv->thumblist          = NULL;

   tv->tw                 = NULL;

   tv->container          = NULL;
   tv->popup_menu         = NULL;

   tv->tabtitle           = NULL;

   tv->thumb_size         = 96;

   tv->mode               = 0;

   tv->filenum            = 0;
   tv->filesize           = 0;

   tv->summary_mode       = NULL;
   tv->vfuncs             = NULL;

   tv->dnd_destdir        = NULL;

   tv->status             = 0;
   tv->progress           = NULL;
   tv->load_list          = NULL;

   tv->priv = g_new0 (GimvThumbViewPriv, 1);
   tv->priv->dirname               = NULL;
   tv->priv->archive               = NULL;
   tv->priv->related_image_view    = NULL;
   tv->priv->related_dupl_win      = NULL;
   tv->priv->button_2pressed_queue = 0;

   GimvThumbViewList = g_list_append (GimvThumbViewList, tv);
   total_tab_count++;
}


GimvThumbView *
gimv_thumb_view_new (void)
{
   GimvThumbView *tv;

   tv = GIMV_THUMB_VIEW (g_object_new (GIMV_TYPE_THUMB_VIEW, NULL));

   return tv;
}


static void
gimv_thumb_view_redraw (GimvThumbView *tv,
                        const gchar *mode,
                        GtkWidget *scroll_win)
{
   GList *node;
   GtkAdjustment *hadj, *vadj;
   GtkWidget *widget;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));

   node = g_list_find (GimvThumbViewList, tv);
   if (!node) return;

   if (!scroll_win)
      scroll_win = tv->container;

   gimv_thumb_view_remove_scrollbar_callback (tv);

   hadj = gtk_scrolled_window_get_hadjustment (
               GTK_SCROLLED_WINDOW (tv->container));
   vadj = gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (tv->container));
   hadj->value = 0.0;
   vadj->value = 0.0;
   g_signal_emit_by_name (G_OBJECT(hadj), "value_changed"); 
   g_signal_emit_by_name (G_OBJECT(vadj), "value_changed"); 

   /* sort thumbnail list */
   gimv_thumb_view_sort_data (tv);

   /* remove current widget */
   if (GTK_BIN (tv->container)->child)
      gtk_widget_destroy (GTK_BIN (tv->container)->child);   

   /* create new widget */
   tv->vfuncs = g_hash_table_lookup (modes_table, mode);
   tv->summary_mode = mode;
   g_return_if_fail (tv->vfuncs);

   widget = tv->vfuncs->create (tv, mode);

   gtk_container_add (GTK_CONTAINER (scroll_win), widget);
   tv->container = scroll_win;

   gimv_thumb_view_set_scrollbar_callback (tv);

   gimv_thumb_view_reset_load_priority (tv);

   gtk_widget_grab_focus (GTK_BIN (tv->container)->child);
}


gboolean
gimv_thumb_view_set_widget (GimvThumbView *tv, GimvThumbWin *tw,
                            GtkWidget *container, const gchar *summary_mode)
{
   gint page;
   gboolean is_newtab = FALSE;

   g_return_val_if_fail (GIMV_IS_THUMB_VIEW (tv), FALSE);
   g_return_val_if_fail (container || tv->container, FALSE);

   /* check parent widget */
   page = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), container);
   if (page < 0)
      return FALSE;

   if (!tv->container)
      is_newtab = TRUE;

   if (is_newtab) {
      gchar buf[BUF_SIZE];

      tv->tw = tw;
      tv->container = container;
      tv->summary_mode = summary_mode;
      tv->mode = GIMV_THUMB_VIEW_MODE_COLLECTION;
      g_snprintf (buf, BUF_SIZE, _("Collection %d"), total_tab_count);
      gimv_thumb_view_reset_tab_label (tv, buf);
      gimv_thumb_view_set_scrollbar_callback (tv);
      gimv_thumb_view_redraw (tv, tv->summary_mode, tv->container);
   } else {
      gimv_thumb_view_redraw (tv, summary_mode, container);
      tv->tw = tw;
   }

   return TRUE;
}


void
gimv_thumb_view_reload (GimvThumbView *tv, FilesLoader *files, GimvThumbViewMode mode)
{
   gint current_page, this_page;
   GList *loadlist = NULL;
   gchar buf[BUF_SIZE];

   g_return_if_fail (GIMV_IS_THUMB_VIEW (tv));
   g_return_if_fail (files);
   g_return_if_fail ((mode == GIMV_THUMB_VIEW_MODE_COLLECTION) ||
                     (mode == GIMV_THUMB_VIEW_MODE_DIR && files->dirname) ||
                     (mode == GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR && files->dirname) ||
                     (mode == GIMV_THUMB_VIEW_MODE_ARCHIVE && files->archive));
   g_return_if_fail (!tv->progress);

   gimv_thumb_view_clear (tv);
   g_free (tv->priv->dirname);
   tv->priv->dirname = NULL;
   if (tv->priv->archive)
      g_object_unref (G_OBJECT (tv->priv->archive));
   tv->priv->archive = NULL;

   /* set mode specific data */
   if (mode == GIMV_THUMB_VIEW_MODE_DIR ||
       mode == GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR)
   {
      if (files->dirname[strlen (files->dirname) - 1] != '/')
         tv->priv->dirname = g_strconcat (files->dirname, "/", NULL);
      else
         tv->priv->dirname = g_strdup (files->dirname);

   } else if (mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      g_object_ref (G_OBJECT (files->archive));
      tv->priv->archive = files->archive;

   } else if (mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
   }

   /* set struct member */
   tv->progress = files;

   if (mode == GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR)
      tv->mode = GIMV_THUMB_VIEW_MODE_COLLECTION;
   else
      tv->mode = mode;

   tv->thumb_size =
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(tv->tw->button.size_spin));

   /* set tab label */
   if (mode == GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR) {
      gchar *tmpstr, *dirname_internal;
      tmpstr = fileutil_home2tilde (tv->priv->dirname);
      dirname_internal = charset_to_internal (tmpstr,
                                              conf.charset_filename,
                                              conf.charset_auto_detect_fn,
                                              conf.charset_filename_mode);
      g_snprintf (buf, BUF_SIZE, _("%s (Collection)"), dirname_internal);
      g_free (dirname_internal);
      g_free (tmpstr);
      gimv_thumb_view_reset_tab_label (tv, buf);
   } else if (mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
      if (!tv->tabtitle) {
         g_snprintf (buf, BUF_SIZE, _("Collection %d"), total_tab_count);
         gimv_thumb_view_reset_tab_label (tv, buf);
      }
   } else {
      gimv_thumb_view_reset_tab_label (tv, NULL);
   }
   gimv_thumb_win_set_tab_label_state (tv->container, GTK_STATE_SELECTED);

   gimv_thumb_win_location_entry_set_text (tv->tw, NULL);

   /* fetch infomation about image files */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      gimv_thumb_view_insert_thumb_frames_from_archive (tv, files->archive);
   } else {
      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR && conf.thumbview_show_dir) {
         gchar *parent = g_strconcat (tv->priv->dirname, "..", NULL);
         GList *dirlist = g_list_copy (files->dirlist);

         dirlist = g_list_prepend (dirlist, parent);
         gimv_thumb_view_insert_thumb_frames (tv, dirlist);
         g_list_free(dirlist);
         g_free (parent);
      }
      gimv_thumb_view_insert_thumb_frames (tv, files->filelist);
   }

   /* set window status */
   if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR && tv->priv->dirname)
      gimv_dir_view_set_opened_mark (tv->tw->dv, tv->priv->dirname);

   current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tv->tw->notebook));
   this_page = gtk_notebook_page_num (GTK_NOTEBOOK (tv->tw->notebook),
                                      tv->container);

   if (current_page == this_page) {
      tv->tw->status = GIMV_THUMB_WIN_STATUS_LOADING;
      gimv_thumb_win_set_sensitive (tv->tw, GIMV_THUMB_WIN_STATUS_LOADING);
      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR && tv->priv->dirname)
         gimv_dir_view_change_dir (tv->tw->dv, tv->priv->dirname);

   } else {
      tv->tw->status = GIMV_THUMB_WIN_STATUS_LOADING_BG;
      gimv_thumb_win_set_sensitive (tv->tw, GIMV_THUMB_WIN_STATUS_LOADING_BG);
   }

   /* load thumbnails */
   if (tv->vfuncs->get_load_list)
      loadlist = tv->vfuncs->get_load_list (tv);
   if (loadlist) {
      gimv_thumb_view_load_thumbnails (tv, loadlist, tv->summary_mode);
      g_list_free (loadlist);
   }

   /* reset window status */
   if (files->status > 0 && files->status < CANCEL) {
      files->status = THUMB_LOAD_DONE;
   }

   if (files->status > 0) {
      tv->progress = NULL;
      /* reset status bar and tab label */
      tv->tw->status = GIMV_THUMB_WIN_STATUS_NORMAL;
      gimv_thumb_win_set_statusbar_page_info (tv->tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   }

   gimv_thumb_win_set_tab_label_state (tv->container, GTK_STATE_NORMAL);
}


static void
gimv_thumb_view_dispose (GObject *object)
{
   GimvThumbView *tv;
   GList *node;

   g_return_if_fail (GIMV_IS_THUMB_VIEW (object));

   tv = GIMV_THUMB_VIEW (object);

   gimv_thumb_view_remove_scrollbar_callback (tv);

   GimvThumbViewList = g_list_remove (GimvThumbViewList, tv);

   if (GTK_BIN (tv->container)->child) {
      gtk_widget_destroy (GTK_BIN (tv->container)->child);   
      GTK_BIN (tv->container)->child = NULL;
   }

   if (tv->priv) {
      if (tv->tw && tv->tw->dv &&
          tv->mode == GIMV_THUMB_VIEW_MODE_DIR && tv->priv->dirname) {
         gimv_dir_view_unset_opened_mark (tv->tw->dv, tv->priv->dirname);
      }

      g_free (tv->priv->dirname);
      tv->priv->dirname = NULL;

      /* remove archive */
      if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE && tv->priv->archive) {
         g_object_unref (G_OBJECT (tv->priv->archive));
         tv->priv->archive = NULL;
      }

      /* destroy relation */
      node = tv->priv->related_image_view;
      while (node) {
         GimvImageView *iv = node->data;
         node = g_list_next(node);
         gimv_image_view_remove_list(iv, tv);
      }
      g_list_free (tv->priv->related_image_view);
      tv->priv->related_image_view = NULL;

      g_list_foreach (tv->priv->related_dupl_win,
                      (GFunc) gimv_thumb_view_destroy_dupl_win_relation,
                      tv);
      g_list_free (tv->priv->related_dupl_win);
      tv->priv->related_dupl_win = NULL;

      g_free (tv->priv);
      tv->priv = NULL;
   }

   if (tv->tw) {
      tv->tw->filenum -= tv->filenum;
      tv->tw->filesize -= tv->filesize;
      tv->tw = NULL;
   }

   if (tv->progress && tv->progress->status != WINDOW_DESTROYED) {
      tv->progress->status = CONTAINER_DESTROYED;
      tv->progress = NULL;
   }

   /* remove thumbnails */
   g_list_foreach (tv->thumblist, (GFunc) g_object_unref, NULL);
   g_list_free(tv->thumblist);
   tv->thumblist = NULL;

   /* remove popup menu */
   if (tv->popup_menu) {
      gtk_widget_unref (tv->popup_menu);
      tv->popup_menu = NULL;
   }

   g_free (tv->tabtitle);
   tv->tabtitle = NULL;

   if (G_OBJECT_CLASS (gimv_thumb_view_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_thumb_view_parent_class)->dispose (object);
}

