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

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gimageview.h"

#include "argparse.h"
#include "fileload.h"
#include "gimv_dir_view.h"
#include "gimv_image.h"
#include "gimv_icon_stock.h"
#include "gimv_image_win.h"
#include "gimv_plugin.h"
#include "gimv_slideshow.h"
#include "gimv_thumb_cache.h"
#include "gimv_thumb_win.h"
#include "prefs.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_file_gtk.h"

#include "fr-archive.h"

/* private functions */
void                gimageview_init         (gint    *argc,
                                             gchar   *argv[]);
static FilesLoader *get_files_from_argument (gint     argc,
                                             gchar   *argv[],
                                             gint     remaining);
static gint         idle_open_image_startup (gpointer data);
static gint         idle_slideshow_startup  (gpointer data);


extern ArgsVal args_val;


void
gimv_charset_init (void)
{
   charset_set_locale_charset (conf.charset_locale);
   charset_set_internal_charset (conf.charset_internal);
   conf.charset_auto_detect_fn
      = charset_get_auto_detect_func (conf.charset_auto_detect_lang);
}


gchar *
gimv_filename_to_locale (const gchar *filename)
{
	return charset_internal_to_locale (filename);
}


gchar *
gimv_filename_to_internal (const gchar *filename)
{
	return charset_to_internal (filename,
                               conf.charset_filename,
                               conf.charset_auto_detect_fn,
                               conf.charset_filename_mode);
}


void
gimageview_init (gint *argc, gchar *argv[])
{
   gchar buf[MAX_PATH_LEN];

   /* set locale */
   setlocale (LC_ALL, "");
   bindtextdomain (GETTEXT_PACKAGE, GIMV_LOCALEDIR);
   bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
   textdomain (GETTEXT_PACKAGE);

   /* Gtk Initialize */
   gtk_set_locale();
   gtk_init(argc, &argv);
   g_snprintf (buf, MAX_PATH_LEN, "%s/%s", DATADIR, GIMV_GTK_RC);
   gtk_rc_parse (buf);
   g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s",
               g_getenv ("HOME"), GIMV_RC_DIR, GIMV_GTK_RC);
   gtk_rc_parse (buf);

   /* load config */
   prefs_load_config ();
   g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s", g_getenv("HOME"),
               GIMV_RC_DIR, GIMV_KEYACCEL_RC);
   gtk_accel_map_load (buf);

   /* gimv_image_init (); */

   /* load plugins */
   gimv_plugin_init ();

   /* load icons */
   gimv_icon_stock_init (conf.iconset);

   /* charset conversion related setting */
   gimv_charset_init ();
}


void
gimv_quit (void)
{
   GimvThumbWin *tw;
   GimvImageWin *iw;
   GList *node;
   gchar buf[MAX_PATH_LEN];

   node = g_list_last (gimv_thumb_win_get_list());

   if (node) {
      tw = node->data;
      if (tw && conf.thumbwin_save_win_state)
         gimv_thumb_win_save_state (tw);
   }

   node = g_list_last (gimv_image_win_get_list());

   if (node) {
      iw = node->data;
      if (iw && conf.imgwin_save_win_state)
         gimv_image_win_save_state (iw);
   }

   /* save config to disk */
   prefs_save_config ();
   g_snprintf (buf, MAX_PATH_LEN, "%s/%s/%s",
               g_getenv("HOME"), GIMV_RC_DIR, GIMV_KEYACCEL_RC);
   gtk_accel_map_save (buf);

   remove_temp_dir ();

   gtk_main_quit ();
}


static FilesLoader *
get_files_from_argument (gint argc, gchar *argv[], gint remaining)
{
   FilesLoader *files = NULL;
   gchar *path;
   gint i;

   files = files_loader_new ();

#ifdef GIMV_DEBUG
   g_print ("Program start\n");
   file_disp_loading_status (files);
#endif /* GIMV_DEBUG */

   for (i = remaining; i < argc; i++) {
      if (conf.conv_rel_path_to_abs) {
         path = relpath2abs (argv[i]);
      } else {
         path = g_strdup (argv[i]);
      }

      /* store dir list */
      if (isdir (path)) {
         files->dirlist = g_list_append (files->dirlist, path);

         /* detect image file by file name extension */
      } else if (file_exists (path)
                 && (args_val.ignore_ext
                     || gimv_image_detect_type_by_ext (path)
                     || fr_archive_utils_get_file_name_ext (path)))
      {
         files->filelist = g_list_append (files->filelist, path);
      } else {
         g_free (path);
      }
   }

   return files;
}


static gint
idle_open_image_startup (gpointer data)
{
   gboolean quit_main = FALSE;
   FilesLoader *files = (FilesLoader *) data;
   LoadStatus status = FALSE;

   status = open_image_files(files);
   if (args_val.read_dir) {
      status = open_dirs (files, NULL, LOAD_CACHE, args_val.read_dir_recursive)
         || status;
   }

   if (!status) {
      if (!conf.startup_no_warning) {
         g_warning (_("No image file is specified!!"));
         if (!conf.startup_read_dir)
            g_warning (_("If you want to scan directory, use \"-d\" option."));
      }
      /* quit if no window opened */
      if (!gimv_thumb_win_get_list() && !gimv_image_win_get_list()) {
         if (!conf.startup_no_warning)
            g_warning (_("No window is opened!! Quiting..."));
         quit_main = TRUE;
      }
   }

   files_loader_delete (files);

   /* reset config */
   prefs_load_config ();

   if (quit_main)
      gtk_main_quit ();

   return FALSE;
}


static gint
idle_slideshow_startup (gpointer data)
{
   GimvSlideshow *slideshow = (GimvSlideshow *) data;

   /* reset config */
   prefs_load_config ();

   gimv_slideshow_play (slideshow);

   return FALSE;
}


static void
startup_slideshow (FilesLoader *files)
{
   GimvSlideshow *slideshow;
   GList *list, *filelist;

   /* get file list from directories */
   if (args_val.read_dir) {
      GList *tmplist = g_list_copy (files->dirlist);
      gint flags = 0;

      if (!args_val.ignore_ext)
         flags = flags | GETDIR_DETECT_EXT;
      if (args_val.read_dot)
         flags = flags | GETDIR_READ_DOT;
      if (args_val.read_dir_recursive) {
         flags = flags | GETDIR_RECURSIVE;
         if (conf.recursive_follow_link)
            flags = flags | GETDIR_FOLLOW_SYMLINK;
      }
      if (conf.disp_filename_stdout)
         flags = flags | GETDIR_DISP_STDOUT;

      for (list = tmplist; list; list = g_list_next (list)) {
         gchar *dirname = list->data;
         GList *filelist = NULL, *dirlist = NULL;
         if (dirname && *dirname) {
            get_dir (dirname, flags, &filelist, &dirlist);
            files->filelist = g_list_concat (files->filelist, filelist);
            files->dirlist = g_list_concat (files->dirlist, dirlist);
         }
      }
      g_list_free (tmplist);
   }

   filelist = NULL;
   for (list = files->filelist; list; list = g_list_next (list)) {
      GimvImageInfo *info = gimv_image_info_get (list->data);

      if (info)
         filelist = g_list_append (filelist, info);
   }

   /* init slideshow */
   slideshow = gimv_slideshow_new_with_filelist (filelist, filelist);
   gimv_slideshow_set_interval (slideshow, args_val.slideshow_interval * 1000);
   /* files->filelist = NULL; */

   /*
   if (g_list_length (slideshow->filelist) < 1)
   arg_help (argv, stderr);
   */

   gtk_init_add (idle_slideshow_startup, slideshow);

   files_loader_delete (files);
}


#ifdef ENABLE_SPLASH
gint splash_timer_id;

static gboolean
timeout_splash (GtkWidget *splash)
{
   g_return_val_if_fail (splash, FALSE);

   gtk_widget_destroy (splash);
   splash_timer_id = -1;

   return FALSE;
}


static void
show_splash ()
{
   GtkWidget *window;
   GtkWidget *pixmap;

   pixmap = gimv_icon_stock_get_widget ("gimageview");

   window = gtk_window_new (GTK_WINDOW_POPUP);
   gtk_widget_realize(window);
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   gdk_window_set_decorations(window->window, 0);

   gtk_container_add(GTK_CONTAINER(window), pixmap);
   gimv_icon_stock_free_icon ("gimageview");

   gtk_widget_show(pixmap);
   gtk_widget_show(window);
   gdk_window_raise (window->window);

   while (gtk_events_pending()) gtk_main_iteration();

   splash_timer_id
      = gtk_timeout_add (1500,
                         (GtkFunction) timeout_splash,
                         (gpointer) window);
}
#endif



/******************************************************************************
 *
 *   main
 *
 ******************************************************************************/
gint
main (gint argc, gchar *argv[])
{
   GimvImageWin *iw = NULL;
   GimvThumbWin *tw = NULL;
   gint remaining;
   gboolean open_thumbwin = FALSE;
   FilesLoader *files = NULL;

   gimageview_init (&argc, argv);

   if (argc == 1) {
      open_thumbwin = TRUE;
   }

   /* override config by argument while start up*/
   arg_parse (argc, argv, &remaining);

#ifdef ENABLE_SPLASH
   show_splash ();
#endif

   /* open window if specified by config or argument */
   if (args_val.open_thumbwin || open_thumbwin) {
      tw = gimv_thumb_win_open_window ();
   }
   if (args_val.open_imagewin) {
      iw = gimv_image_win_open_window (NULL);
   }

   /* set FilesLoader struct data for opening files */
   files = get_files_from_argument (argc, argv, remaining);
   if (!files && !gimv_thumb_win_get_list() && !gimv_image_win_get_list()) {
      exit (1);
   } else if (tw && files && files->dirlist) {
      GList *list = g_list_last (files->dirlist);
      gchar *dirname = NULL;

      if (list) dirname = list->data;
      if (dirname) gimv_dir_view_chroot (tw->dv, dirname);
   }

   /* exec slide show if specified by argument */
   if (args_val.exec_slideshow) {
      startup_slideshow (files);

   /* check filelist & dirlist and open image files */
   } else {
      gtk_init_add (idle_open_image_startup, files);
   }

   /* main roop */
   gtk_main ();

   return 0;
}
