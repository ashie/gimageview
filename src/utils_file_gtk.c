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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "fr-archive.h"
#include "gimv_comment.h"
#include "gimv_image.h"
#include "gimv_thumb.h"
#include "gimv_thumb_cache.h"
#include "prefs.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_file_gtk.h"
#include "utils_gtk.h"

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif


gchar *
add_slash (const gchar *path)
{
   g_return_val_if_fail (path, NULL);

   if (!*path) return g_strdup ("/");

   if (path [strlen (path) - 1] == '/')
      return g_strdup (path);
   else
      return g_strconcat (path, "/", NULL);
}


gchar *
remove_slash (const gchar *path)
{
   g_return_val_if_fail (path, NULL);
   g_return_val_if_fail (*path, g_strdup (path));

   if (path[strlen (path) - 1] == '/')
      return g_dirname (path);
   else
      return g_strdup (path);
}


gboolean getting_dir = FALSE;
gboolean stop_getting_dir = FALSE;
static gchar* tmpdir = NULL;


gchar *
get_temp_dir_name (void)
{
   if (tmpdir) return tmpdir;

   tmpdir = g_strdup_printf ("%s%s.%d",
                             g_get_tmp_dir (),
                             "/gimv",
                             getpid ());
   return tmpdir;
}


void
remove_dir (const gchar *dirname)
{
   GList *node, *list;

   if (!isdir(dirname)) return;

   list = get_dir_all (dirname);

   for (node = list; node; node = g_list_next (node))
      remove (node->data);

   if (list) {
      remove (dirname);
      g_list_foreach (list, (GFunc) g_free, NULL);
      g_list_free (list);
   }
}


void
remove_temp_dir (void)
{
   if (!tmpdir) return;

   remove_dir (tmpdir);

   g_free (tmpdir);
   tmpdir = NULL;
}


/*
 *  get_dir:
 *     @ Get image files in specified directory.
 *
 *  dirname  : Directory to scan.
 *  files    : Pointer to OpenFiles struct for store directory list.
 */
void
get_dir (const gchar *dirname, GetDirFlags flags,
         GList **filelist_ret, GList **dirlist_ret)
{
   DIR *dp;
   struct dirent *entry;
   gchar buf[MAX_PATH_LEN], *path;
   GList *filelist = NULL, *dirlist = NULL, *list;

   g_return_if_fail (dirname && *dirname);

   getting_dir = TRUE;

   if (flags & GETDIR_DISP_STDERR)
      fprintf (stderr, _("scandir = %s\n"), dirname);
   else if (flags & GETDIR_DISP_STDOUT)
      fprintf (stdout, _("scandir = %s\n"), dirname);

   if ((dp = opendir (dirname))) {
      while ((entry = readdir (dp))) {
         if (flags & GETDIR_ENABLE_CANCEL) {
            while (gtk_events_pending()) gtk_main_iteration();
            if (stop_getting_dir) break;
         }

         /* ignore dot file */
         if (!(flags & GETDIR_READ_DOT) && entry->d_name[0] == '.')
            continue;

         /* get full path */
         if (dirname [strlen (dirname) - 1] == '/')
            g_snprintf (buf, MAX_PATH_LEN, "%s%s", dirname, entry->d_name);
         else
            g_snprintf (buf, MAX_PATH_LEN, "%s/%s", dirname, entry->d_name);

         /* if path is file */
         if (!isdir (buf) || (!(flags & GETDIR_FOLLOW_SYMLINK) && islink (buf))) {
            if (!filelist_ret) continue;

            if (!(flags & GETDIR_DETECT_EXT)
                || gimv_image_detect_type_by_ext (buf)
                || ((flags & GETDIR_GET_ARCHIVE)
                    && fr_archive_utils_get_file_name_ext (buf)))
            {
               path = g_strdup (buf);

               if (flags & GETDIR_DISP_STDERR)
                  fprintf (stderr, _("filename = %s\n"), path);
               else if (flags & GETDIR_DISP_STDOUT)
                  fprintf (stdout, _("filename = %s\n"), path);

               filelist = g_list_append (filelist, path);
            }

         /* if path is dir */
         } else if (isdir(buf)) {
            if (dirlist_ret && strcmp(entry->d_name, ".")
                && strcmp(entry->d_name, ".."))
            {
               path = g_strdup (buf);

               if (flags & GETDIR_DISP_STDERR)
                  fprintf (stderr, _("dirname = %s\n"), path);
               else if (flags & GETDIR_DISP_STDOUT)
                  fprintf (stdout, _("dirname = %s\n"), path);

               dirlist = g_list_append (dirlist, path);
            }
         }
      }
      closedir (dp);
      if (filelist)
         filelist = g_list_sort (filelist, gtkutil_comp_spel);
      if (dirlist)
         dirlist = g_list_sort (dirlist, gtkutil_comp_spel);
   } else {
      g_warning ("cannot open directory: %s", dirname);
   }

   /* recursive get */
   if (flags & GETDIR_RECURSIVE) {
      GList *tmplist = g_list_copy (dirlist);
      gint tmp_flags = flags | GETDIR_RECURSIVE_IS_BRANCH;

      list = tmplist;
      while (list) {
         GList *tmp_filelist = NULL, *tmp_dirlist = NULL;
         if (flags & GETDIR_ENABLE_CANCEL) {
            while (gtk_events_pending()) gtk_main_iteration();
            if (stop_getting_dir) break;
         }
         get_dir ((const gchar *) list->data, tmp_flags,
                  &tmp_filelist, &tmp_dirlist);
         filelist = g_list_concat (filelist, tmp_filelist);
         dirlist = g_list_concat (dirlist, tmp_dirlist);
         list = g_list_next (list);
      }
      g_list_free (tmplist);
   }

   /* return value */
   if (filelist_ret)
      *filelist_ret = filelist;
   if (dirlist_ret)
      *dirlist_ret = dirlist;

   if (!(flags & GETDIR_RECURSIVE_IS_BRANCH)) {
      getting_dir = FALSE;
      stop_getting_dir = FALSE;
   }
}


void
get_dir_stop (void)
{
   if (getting_dir)
      stop_getting_dir = TRUE;
}


/*
 *  get_dir_all:
 *     @
 *
 *  dirname :
 *  Return  :
 */
GList *
get_dir_all (const gchar *dirname)
{
   GList *filelist, *dirlist, *sub_dirlist, *node;
   gchar *sub_dirname;

   get_dir (dirname, GETDIR_READ_DOT, &filelist, &dirlist);

   if (dirlist) {
      node = dirlist;
      while (node) {
         sub_dirname = node->data;
         sub_dirlist = get_dir_all (sub_dirname);
         if (sub_dirlist)
            filelist = g_list_concat (sub_dirlist, filelist);
         node = g_list_next (node);
      }
      filelist = g_list_concat (filelist, dirlist);      
   }
   return filelist;
}


/*
 *  get_dir_all_file:
 *     @
 *
 *  dirname :
 *  Return  :
 */
static GList *
get_dir_all_file (const gchar *dirname)
{
   GList *filelist, *dirlist, *sub_dirlist, *node;
   gchar *sub_dirname;

   get_dir (dirname, GETDIR_READ_DOT, &filelist, &dirlist);

   if (dirlist) {
      node = dirlist;
      while (node) {
         sub_dirname = node->data;
         sub_dirlist = get_dir_all_file (sub_dirname);
         if (sub_dirlist)
            filelist = g_list_concat (sub_dirlist, filelist);
         node = g_list_next (node);
      }
   }
   return filelist;
}


/*
 *  merge from misc/misc.c in Text maid (Copyright(C) Kazuki Iwamoto).
 */
gchar *
relpath2abs (const gchar *path)
{
   gchar *dir, *dest;
   gint i,j,len;

   g_return_val_if_fail (path && *path, NULL);

   if (path[0] != '/') {
      dir = g_get_current_dir ();
      dest = g_strjoin ("/", dir, path, NULL);
      g_free (dir);
   } else {
	   dest = g_strdup (path);
   }

   len = strlen (dest) + 1;
   i = 0;
   while (i < len - 2) {
      if (dest[i] == '/' && dest[i + 1] == '.'
          && (dest[i + 2] == '/' || dest[i + 2] == '\0'))
      {
         len -= 2;
         memmove (dest + i, dest + i + 2, len - i);
      }

      i++;
   }

   i = 0;
   while (i < len - 3) {
      if (dest[i] == '/' && dest[i + 1] == '.' && dest[i + 2] == '.'
          && (dest[i + 3] == '/' || dest[i + 3] == '\0'))
      {
         len -= 3;
         memmove (dest + i, dest + i + 3, len - i);
         for (j = i - 1; j >= 0; j--) {
            if (dest[j] == '/')
               break;
         }
         if (j >= 0) {
            memmove (dest + j, dest + i, len - i);
            len -= i - j;
            i = j;
         }
      }

      i++;
   }

   return dest;
}


gchar *
link2abs (const gchar *path)
{
   gchar *retval = NULL, **dirs, buf[MAX_PATH_LEN], *tmpstr;
   gint i, num;

   g_return_val_if_fail (path && *path, NULL);
   g_return_val_if_fail (path[0] == '/', g_strdup (path));

   if (!strcmp (path, "/")) return g_strdup (path);

   dirs = g_strsplit (path, "/", -1);
   g_return_val_if_fail (dirs, g_strdup (path));

   retval = g_strdup("");
   for (i = 0; dirs[i]; i++) {
      gchar *endchr;

      if (!*dirs[i]) continue;

      tmpstr = g_strconcat (retval, "/", dirs[i], NULL);
      g_free (retval);
      retval = tmpstr;

      num = readlink (retval, buf, MAX_PATH_LEN);
      if (num < 1) continue;

      buf[num] = '\0';
      if (buf[0] == '/') {
         g_free (retval);
         retval = g_strdup (buf);
      } else {
         endchr = strrchr (retval, '/');
         if (!endchr) {
            g_free (retval);
            retval = g_strdup (path);
            break;
         }
         *endchr = '\0';
         /* FIXME: what about link to link? */
         tmpstr = g_strconcat (retval, "/", buf, NULL);
         g_free (retval);
         retval = tmpstr;
      }
   }

   g_strfreev (dirs);

   if (!retval) {
      g_warning ("invalid link: %s\n", path);
   } else {
      tmpstr = relpath2abs (retval);
      g_free (retval);
      retval = tmpstr;
   }

   return retval;
}


static gboolean
move_file_check_path (const gchar *from_path,
                      struct stat *from_st,
                      const gchar *dir,
                      gboolean show_error,
                      GtkWindow *window)
{
   gchar *from_dir, error_message[BUF_SIZE];
   gchar *from_path_internal, *dir_internal;
   gboolean retval = FALSE;

   from_path_internal   = charset_to_internal (from_path,
                                               conf.charset_filename,
                                               conf.charset_auto_detect_fn,
                                               conf.charset_filename_mode);
   dir_internal         = charset_to_internal (dir,
                                               conf.charset_filename,
                                               conf.charset_auto_detect_fn,
                                               conf.charset_filename_mode);

   /********************
    * check source file
    ********************/
   if (lstat (from_path, from_st)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't find source file :\n%s"),
                     from_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;
   }

   /*****************
    * check dest dir
    *****************/
   if (!iswritable (dir)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't move file : %s\n"
                       "Permission denied: %s\n"),
                     from_path_internal, dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;
   }

   /*******************
    * check source dir
    *******************/
   from_dir = g_dirname (from_path);
   if (!iswritable (from_dir)) {
      if (show_error) {
         gchar *from_dir_internal;

         from_dir_internal   = charset_to_internal (from_dir,
                                                    conf.charset_filename,
                                                    conf.charset_auto_detect_fn,
                                                    conf.charset_filename_mode);

         g_snprintf (error_message, BUF_SIZE,
                     _("Can't move file : %s\n"
                       "Permission denied: %s\n"),
                     from_path_internal, from_dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);

         g_free (from_dir_internal);
      }
      g_free (from_dir);
      goto ERROR;
   }
   g_free (from_dir);

   retval = TRUE;

ERROR:
   g_free (from_path_internal);
   g_free (dir_internal);
   return retval;
}


static gboolean
move_file_check_over_write (const gchar *from_path,
                            struct stat *from_st,
                            const gchar *to_path,
                            struct stat *to_st,
                            gchar *new_path, gint new_path_len,
                            ConfirmType *action,
                            gboolean show_error,
                            GtkWindow *window)
{
   gchar error_message[BUF_SIZE], *to_path_internal;
   gint exist;
   gboolean retval = FALSE;

   if (new_path)
      new_path[0] = '\0';

   to_path_internal   = charset_to_internal (to_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);

   exist = !lstat(to_path, to_st);
   if (exist && (!strcmp (from_path, to_path)
                 || from_st->st_ino == to_st->st_ino))
   {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Same file :\n%s"), to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;

   }  else if (exist && *action == CONFIRM_ASK) {
      if (isdir (from_path)) {
         g_snprintf (error_message, BUF_SIZE,
                     _("File exist : %s"), to_path_internal);
         gtkutil_message_dialog (_("ERROR!!"), error_message, window);

      } else {
         g_snprintf (error_message, BUF_SIZE,
                     _("The file exists : %s\n"
                       "Overwrite?"),
                     to_path_internal);
         *action = gtkutil_overwrite_confirm_dialog (_("File exist!!"), error_message,
                                                     to_path, from_path,
                                                     new_path, MAX_PATH_LEN,
                                                     ConfirmDialogMultipleFlag,
                                                     window);
      }
   }

   if (new_path && *new_path) {
      retval = TRUE;
   } else {
      switch (*action) {
      case CONFIRM_YES:
      case CONFIRM_YES_TO_ALL:
         retval = TRUE;
      break;
      case CONFIRM_NO:
      case CONFIRM_CANCEL:
         retval = FALSE;
         break;
      case CONFIRM_NO_TO_ALL:
         if (exist)
            retval = FALSE;
         else
            retval = TRUE;
         break;
      default:
         if (!exist)
            retval = TRUE;
         else
            retval = FALSE;
         break;
      }
   }

ERROR:
   g_free (to_path_internal);
   return retval;
}


/*
 *  move_file:
 *     @
 *
 *  from_path  :
 *  dir        :
 *  action     :
 *  show_error :
 *  Return     : TRUE if success to move file.
 */
gboolean
move_file (const gchar *from_path, const gchar *dir,
           ConfirmType *action, gboolean show_error,
           GtkWindow *window)
{
   gchar *to_path, error_message[BUF_SIZE];
   struct stat from_st, to_st, todir_st;
   struct utimbuf ut;
   gboolean move_file = FALSE, move_faild = FALSE, copy_success = FALSE, retval;
   gchar *from_path_internal, *to_path_internal, new_path[MAX_PATH_LEN];

   new_path[0] = '\0';

   g_return_val_if_fail (action, FALSE);

   retval = move_file_check_path (from_path, &from_st, dir, show_error, window);
   if (!retval) return FALSE;

   /* set dest path */
   to_path = g_strconcat (dir, g_path_get_basename (from_path), NULL);

   move_file = move_file_check_over_write (from_path, &from_st,
                                           to_path, &to_st,
                                           new_path, MAX_PATH_LEN,
                                           action, show_error,
                                           window);
   if (!move_file) {
      retval = FALSE;
      goto ERROR0;
   }

   from_path_internal = charset_to_internal (from_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
   if (*new_path) {
      g_free (to_path);
      to_path = g_strdup (new_path);
   }
   to_path_internal   = charset_to_internal (to_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);

   /**************
    * move file!!
    **************/
   stat (dir, &todir_st);
   if (from_st.st_dev != todir_st.st_dev) {
      copy_success = copy_file_to_file (from_path, to_path, action, show_error,
                                        window);
      if (copy_success) {
         /* reset new file's time info */
         ut.actime = from_st.st_atime;
         ut.modtime = from_st.st_mtime;
         utime(to_path, &ut);

         /* remove old file */
         if (remove (from_path) < 0) {   /* faild to remove file */
            if (show_error) {
               g_snprintf (error_message, BUF_SIZE,
                           _("Faild to remove file :\n"
                             "%s"),
                           from_path_internal);
               gtkutil_message_dialog (_("Error!!"), error_message, window);
            }
            retval = FALSE;
            goto ERROR1;
         }
      } else {
         move_faild = TRUE;
      }
   } else {
      move_faild = rename (from_path, to_path);
   }

   /************************
    * if faild to move file
    ************************/
   if (move_faild) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Faild to move file :\n"
                       "From : %s\n"
                       "To : %s"),
                     from_path_internal, to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
   }

ERROR1:
   g_free (from_path_internal);
   g_free (to_path_internal);
ERROR0:
   g_free (to_path);
   return retval;
}


static gboolean
copy_dir_check_source (const gchar *from_dir, gboolean show_error,
                       GtkWindow *window)
{
   gchar error_message[BUF_SIZE], *from_dir_internal;
   gboolean retval = TRUE;

   from_dir_internal = charset_to_internal (from_dir,
                                            conf.charset_filename,
                                            conf.charset_auto_detect_fn,
                                            conf.charset_filename_mode);

   if (islink (from_dir)) {   /* check path is link or not */
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("%s is link!!.\n"),
                     from_dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
      goto ERROR;
   }

   if (!file_exists (from_dir)) {   /* check path exists or not */
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't find source file :\n%s"),
                     from_dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
      goto ERROR;
   }

   if (!isdir (from_dir)) {   /* check path is directory or not */
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("%s is not directory!!.\n"),
                     from_dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
      goto ERROR;
   }

ERROR:
   g_free (from_dir_internal);
   return TRUE;
}


static gboolean
copy_dir_check_dest (const gchar *from_path, const gchar *dirname,
                     const gchar *to_dir, gboolean show_error,
                     GtkWindow *window)
{
   gchar error_message[BUF_SIZE];
   gchar *from_path_internal, *dirname_internal, *to_dir_internal;
   gboolean retval = TRUE;

   from_path_internal = charset_to_internal (from_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
   dirname_internal   = charset_to_internal (dirname,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
   to_dir_internal    = charset_to_internal (to_dir,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);

   if (!iswritable (dirname)) {   /* check permission */
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't copy directory : %s\n"
                       "Permission denied: %s\n"),
                     from_path_internal,
                     dirname_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
      goto ERROR;
   }

   if (file_exists (to_dir)) {   /* check dest path */
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("File exists!! : %s\n"),
                     to_dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      retval = FALSE;
      goto ERROR;
   }

ERROR:
   g_free (from_path_internal);
   g_free (dirname_internal);
   g_free (to_dir_internal);

   return retval;
}


/*
 *  copy_dir:
 *     @
 *
 *  from_path  :
 *  dir        :
 *  action     :
 *  show_error :
 *  Return     : TRUE if success to copy directory.
 */
static gboolean
copy_dir (const gchar *from_path, const gchar *dir,
          ConfirmType *action, gboolean show_error,
          GtkWindow *window)
{
   GtkWidget *progress_win;
   gchar message[BUF_SIZE];
   GList *filelist, *node;
   gchar *from_dir, *to_dir, *to_path, *dirname;
   ConfirmType confirm;
   gboolean result, cancel = FALSE;
   gfloat progress;
   gint pos, length;

   from_dir = g_strdup (from_path);
   if (from_dir[strlen (from_dir) - 1] == '/')
      from_dir[strlen (from_dir) - 1] = '\0';

   dirname = g_strdup (dir);
   if (dirname[strlen (dirname) - 1] == '/')
      dirname[strlen (dirname) - 1] = '\0';

   to_dir = g_strconcat (dirname, "/", g_path_get_basename (from_dir), NULL);

   /*******************
    * check source dir
    *******************/
   result = copy_dir_check_source (from_dir, show_error, window);
   if (!result) goto ERROR;

   /*****************
    * check dest dir
    *****************/
   result = copy_dir_check_dest (from_path, dirname, to_dir, show_error, window);
   g_free (dirname);
   if (!result) goto ERROR;


   /****************
    * do copy files
    ****************/
   filelist = node = get_dir_all_file (from_path);
   confirm = CONFIRM_YES_TO_ALL;

   progress_win = gtkutil_create_progress_window (_("Copy directory"), "...",
                                                  &cancel, 300, -1, window);
   gtk_grab_add (progress_win);
   length = g_list_length (filelist);

   while (node) {
      guint len;
      gchar *filename = node->data;
      gchar *tmpstr;

      while (gtk_events_pending()) gtk_main_iteration();

      pos = g_list_position (filelist, node);
      progress = (gfloat) pos / (gfloat) length;

      {   /********** convert charset **********/
         gchar *filename_internal;

         filename_internal = charset_to_internal (filename,
                                                  conf.charset_filename,
                                                  conf.charset_auto_detect_fn,
                                                  conf.charset_filename_mode);

         g_snprintf (message, BUF_SIZE, _("Copying %s ..."),
                     filename_internal);

         g_free (filename_internal);
      }

      gtkutil_progress_window_update (progress_win, _("Copying directory"),
                                      message, NULL, progress);

      len = strlen (from_path);

      if (strlen (filename) > len) {
         tmpstr = filename + len;
         if (tmpstr[0] == '/') tmpstr++;
         to_path = g_strconcat (to_dir, "/", tmpstr, NULL);

         /* realy do copy :-) */
         mkdirs (to_path);
         copy_file_to_file (filename, to_path, &confirm, show_error, window);

         g_free (to_path);
      }

      node = g_list_next (node);
   }

   gtk_grab_remove (progress_win);
   gtk_widget_destroy (progress_win);

   g_list_foreach (filelist, (GFunc) g_free, NULL);
   g_list_free (filelist);

   g_free (from_dir);
   g_free (to_dir);

   return TRUE;

ERROR:
   g_free (from_dir);
   g_free (to_dir);
   return FALSE;
}


/*
 *  copy_file_to_file:
 *     @
 *
 *  from_path  :
 *  to_path    :
 *  action     :
 *  show_error :
 *  Return     : TRUE if success to copy file.
 */
gboolean
copy_file_to_file (const gchar *from_path, const gchar *to_path,
                   ConfirmType *action, gboolean show_error,
                   GtkWindow *window)
{
   gint b;
   gchar buf[BUFSIZ], *from_path_internal, *to_path_internal;
   gchar error_message[BUF_SIZE];
   FILE *from, *to;
   struct stat from_st, to_st;
   gint exist;
   gchar new_path[MAX_PATH_LEN];

   new_path[0] = '\0';

   g_return_val_if_fail (action, FALSE);

   /******************** 
    * check source file
    ********************/
   if (isdir (from_path)) {
      return copy_dir (from_path, to_path, action, show_error, window);
   }

   from_path_internal = charset_to_internal (from_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
   to_path_internal   = charset_to_internal (to_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);

   if (lstat (from_path, &from_st)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't find source file :\n%s"),
                     from_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;
   }

   /******************
    * check dest file
    ******************/
   exist = !lstat(to_path, &to_st);
   if (exist && (!strcmp (from_path, to_path)
                 || from_st.st_ino == to_st.st_ino))
   {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Same file :\n%s"),
                     to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;

   } else if (exist && *action == CONFIRM_ASK) {
      g_snprintf (error_message, BUF_SIZE,
                  _("The file exists : %s\n"
                    "Overwrite?"),
                  to_path_internal);
      *action = gtkutil_overwrite_confirm_dialog (_("File exist!!"), error_message,
                                                  to_path, from_path,
                                                  new_path, MAX_PATH_LEN,
                                                  ConfirmDialogMultipleFlag,
                                                  window);
   }

   if (*new_path) {
      g_free (to_path_internal);
      to_path = new_path;
      to_path_internal = charset_to_internal (to_path,
                                              conf.charset_filename,
                                              conf.charset_auto_detect_fn,
                                              conf.charset_filename_mode);
   } else {
      switch (*action) {
      case CONFIRM_YES:
      case CONFIRM_YES_TO_ALL:
         break;
      case CONFIRM_NO:
      case CONFIRM_CANCEL:
         goto ERROR;
         break;
      case CONFIRM_NO_TO_ALL:
         if (exist)
            goto ERROR;
         break;
      default:
         if (exist)
            goto ERROR;
         break;
      }
   }

   /**********
    * do copy
    **********/
   from = fopen (from_path, "rb");
   if (!from) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't open file for read :\n%s"),
                     to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;
   }

   to = fopen (to_path, "wb");
   if (!to) {
      fclose (from);
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't open file for write :\n%s"),
                     to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR;
   }

   while ((b = fread (buf, sizeof (char), BUFSIZ, from)) > 0) {
      fwrite (buf, sizeof (char), b, to);
      if (ferror (to)) {
         fclose (from);
         fclose (to);

         if (show_error) {
            g_snprintf (error_message, BUF_SIZE,
                        _("An error occured while copying file :\n%s"),
                        to_path_internal);
            gtkutil_message_dialog (_("Error!!"), error_message, window);
         }
         goto ERROR;
      }
   }

   fclose (from);
   fclose (to);

   g_free (to_path_internal);
   g_free (from_path_internal);
   return TRUE;

ERROR:
   g_free (to_path_internal);
   g_free (from_path_internal);
   return FALSE;
}


/*
 *  copy_file:
 *     @
 *
 *  from_path  :
 *  dir        :
 *  action     :
 *  show_error :
 *  Return     : TRUE if success to copy file.
 */
gboolean
copy_file (const gchar *from_path, const gchar *dir,
           ConfirmType *action, gboolean show_error,
           GtkWindow *window)
{
   gchar *to_path;
   gboolean retval;
   gchar error_message[BUF_SIZE];

   g_return_val_if_fail (action, FALSE);

   /* check source file is directory or not */
   if (isdir (from_path)) {
      return copy_dir (from_path, dir, action, show_error, window);
   }

   /*****************
    * check dest dir
    *****************/
   if (!iswritable (dir)) {
      if (show_error) {
         gchar *from_path_internal, *dir_internal;

         from_path_internal = charset_to_internal (from_path,
                                                   conf.charset_filename,
                                                   conf.charset_auto_detect_fn,
                                                   conf.charset_filename_mode);
         dir_internal = charset_to_internal (dir,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't copy file : %s\n"
                       "Permission denied: %s\n"),
                     from_path_internal, dir_internal);

         gtkutil_message_dialog (_("Error!!"), error_message, window);

         g_free (from_path_internal);
         g_free (dir_internal);
      }
      return FALSE;
   }

   to_path = g_strconcat (dir, g_path_get_basename (from_path), NULL);
   retval = copy_file_to_file (from_path, to_path, action, show_error, window);
   g_free (to_path);

   return retval;
}


/*
 *  link_file:
 *     @
 *
 *  from_path  :
 *  dir        :
 *  action     :
 *  show_error :
 *  Return     : TRUE if success to link file.
 */
gboolean
link_file (const gchar *from_path, const gchar *dir,
           gboolean show_error, GtkWindow *window)
{
   gchar *to_path, *to_path_internal, *from_path_internal, *dir_internal;
   struct stat from_st, to_st;
   gboolean link_faild;
   gchar error_message[BUF_SIZE];

   from_path_internal = charset_to_internal (from_path,
                                             conf.charset_filename,
                                             conf.charset_auto_detect_fn,
                                             conf.charset_filename_mode);
   dir_internal = charset_to_internal (dir,
                                       conf.charset_filename,
                                       conf.charset_auto_detect_fn,
                                       conf.charset_filename_mode);

   /********************
    * check source file
    ********************/
   if (lstat (from_path, &from_st)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't find source file :\n%s"),
                     from_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR0;
   }

   /*****************
    * check dest dir
    *****************/
   if (!iswritable (dir)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Can't create link : %s\n"
                       "Permission denied: %s\n"),
                     from_path_internal, dir_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR0;
   }

   to_path = g_strconcat(dir_internal, g_path_get_basename(from_path), NULL);
   to_path_internal = charset_to_internal (to_path,
                                           conf.charset_filename,
                                           conf.charset_auto_detect_fn,
                                           conf.charset_filename_mode);

   /******************
    * check dest path
    ******************/
   if (!lstat (to_path, &to_st)) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("File exist : %s"),
                     to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR1;
   }

   link_faild = symlink(from_path, to_path);

   if (link_faild) {
      if (show_error) {
         g_snprintf (error_message, BUF_SIZE,
                     _("Faild to create link :\n"
                       "From : %s\n"
                       "To : %s"),
                     from_path_internal, to_path_internal);
         gtkutil_message_dialog (_("Error!!"), error_message, window);
      }
      goto ERROR1;
   }

   g_free (to_path);
   g_free (to_path_internal);
   g_free (from_path_internal);
   g_free (dir_internal);
   return TRUE;

ERROR1:
   g_free (to_path);
   g_free (to_path_internal);
ERROR0:
   g_free (from_path_internal);
   g_free (dir_internal);
   return FALSE;
}



static gchar *
get_dest_cache_dir (const gchar *src_file,
                    const gchar *dest_dir,
                    const gchar *cache_type)
{
   gchar *dest_file, *dest_cache_dir;

   if (!src_file || !dest_dir || !cache_type) return NULL;

   dest_file = g_strconcat (dest_dir, g_path_get_basename (src_file), NULL);
   dest_cache_dir = gimv_thumb_cache_get_path (dest_file, cache_type);

   if (dest_cache_dir) {
      gchar *endchr;

      endchr = strrchr (dest_cache_dir, '/');
      if (endchr) {
         *(endchr + 1) = '\0';
         mkdirs (dest_cache_dir);
      } else {
         g_free (dest_cache_dir);
         dest_cache_dir = NULL;
      }
   }
   g_free (dest_file);

   return dest_cache_dir;
}


static gchar *
get_dest_comment_dir (const gchar *src_file,
                      const gchar *dest_dir)
{
   gchar *dest_file, *dest_comment_dir;

   if (!src_file || !dest_dir) return NULL;

   dest_file = g_strconcat (dest_dir, g_path_get_basename (src_file), NULL);
   dest_comment_dir = gimv_comment_get_path (dest_file);

   if (dest_comment_dir) {
      gchar *endchr;

      endchr = strrchr (dest_comment_dir, '/');
      if (endchr) {
         *(endchr + 1) = '\0';
         mkdirs (dest_comment_dir);
      } else {
         g_free (dest_comment_dir);
         dest_comment_dir = NULL;
      }
   }
   g_free (dest_file);

   return dest_comment_dir;
}


static gboolean
do_file_operate (const gchar *src_file,
                 const gchar *dest_dir,
                 ConfirmType *over_write,
                 FileOperateType action,
                 GtkWidget *progress_win,
                 gfloat progress,
                 GtkWindow *window)
{
   gchar *src_cache, *dest_cache_dir;
   gchar *src_comment, *dest_comment_dir;
   gchar *cache_type;
   gchar *src_file_internal;
   struct stat src_st, dest_st;
   ConfirmType cache_over_write = CONFIRM_YES_TO_ALL;
   gboolean success = TRUE, delete_src = FALSE, result;
   gchar message[BUF_SIZE];

   /* FIXME!! */ /* get cache file & dir */
   src_cache        = gimv_thumb_find_thumbcache (src_file, &cache_type);
   dest_cache_dir   = get_dest_cache_dir (src_file, dest_dir, cache_type);
   src_comment      = gimv_comment_find_file (src_file);
   dest_comment_dir = get_dest_comment_dir (src_file, dest_dir);

   /* if move to different file system, change to copy */
   lstat (src_file, &src_st);
   lstat (dest_dir, &dest_st);
   if (src_st.st_dev != dest_st.st_dev && action == FILE_MOVE ) {
      action = FILE_COPY;
      delete_src = TRUE;
   }

   src_file_internal = charset_to_internal (src_file,
                                            conf.charset_filename,
                                            conf.charset_auto_detect_fn,
                                            conf.charset_filename_mode);

   switch (action) {
   case FILE_MOVE:
      g_snprintf (message, BUF_SIZE,
                  _("Moving %s ..."), src_file_internal);
      gtkutil_progress_window_update (progress_win, _("Moving files"),
                                      message, NULL, progress);
      result = move_file (src_file, dest_dir, over_write, TRUE, window);

      if (!result)
         success = FALSE;

      /* FIXME!! */ /* move cache file */
      if (result && src_cache && dest_cache_dir)
         move_file (src_cache, dest_cache_dir, &cache_over_write, FALSE, window);
      if (result && src_comment && dest_comment_dir)
         move_file (src_comment, dest_comment_dir, &cache_over_write, FALSE, window);

      break;

   case FILE_COPY:
   {
      gboolean delete_src_cache = FALSE, delete_src_comment = FALSE;

      g_snprintf (message, BUF_SIZE,
                  _("Copying %s ..."), src_file_internal);
      gtkutil_progress_window_update (progress_win, _("Copying files"),
                                      message, NULL, progress);
      result = copy_file (src_file, dest_dir, over_write, TRUE, window);

      if (!result)
         success = FALSE;

      /* FIXME!! */ /* copy cache file */
      if (result && src_cache && dest_cache_dir) {
         if (copy_file (src_cache, dest_cache_dir, &cache_over_write, FALSE, window))
            delete_src_cache = TRUE;         
      }
      if (result && src_comment && dest_comment_dir) {
         if (copy_file (src_comment, dest_comment_dir, &cache_over_write, FALSE, window))
            delete_src_comment = TRUE;
      }

      /* FIXME: delete src? */
      if (result && delete_src) {
         GList *delete_file_list = NULL;
         delete_file_list = g_list_append (delete_file_list, (gpointer) src_file);
         if (delete_src_cache)
            delete_file_list = g_list_append (delete_file_list, src_cache);
         if (delete_src_comment)
            delete_file_list = g_list_append (delete_file_list, src_comment);
         if (delete_file_list)
            delete_files (delete_file_list, CONFIRM_YES, window);
         g_list_free (delete_file_list);
      }

      break;
   }
   case FILE_LINK:
      g_snprintf (message, BUF_SIZE,
                  _("Creating Link %s ..."), src_file_internal);
      gtkutil_progress_window_update (progress_win, _("Creating Links"),
                                      message, NULL, progress);
      result = link_file (src_file, dest_dir, TRUE, window);

      if (!result)
         success = FALSE;

      /* FIXME!! */ /* link cache file */
      if (result && src_cache && dest_cache_dir)
         link_file (src_cache, dest_cache_dir, FALSE, window);
      if (result && src_comment && dest_comment_dir)
         link_file (src_comment, dest_comment_dir, FALSE, window);

      break;

   default:
      success = FALSE;
      break;
   }

   g_free (src_file_internal);

   /* FIXME!! */ 
   g_free (src_cache);
   g_free (dest_cache_dir);
   g_free (src_comment);
   g_free (dest_comment_dir);

   return success;
}


gboolean
files2dir (GList *filelist, const gchar *dir, FileOperateType action, GtkWindow *window)
{
   GtkWidget *progress_win;
   GList *node;
   ConfirmType over_write = CONFIRM_ASK;
   gboolean success = TRUE, cancel = FALSE, result;
   gchar message[BUF_SIZE], *src_file, *dest_dir, *dir_internal;
   gint length, pos;
   gfloat progress;

   g_return_val_if_fail (filelist, FALSE);
   g_return_val_if_fail (dir, FALSE);

   dir_internal = charset_to_internal (dir,
                                       conf.charset_filename,
                                       conf.charset_auto_detect_fn,
                                       conf.charset_filename_mode);

   /*****************
    * check dest dir
    *****************/
   if (!file_exists (dir)) {
      g_snprintf (message, BUF_SIZE,
                  _("Directory doesn't exist!!: %s"),
                  dir_internal);
      gtkutil_message_dialog (_("Error!!"), message, window);
      goto ERROR;
   }
   if (!iswritable (dir)) {
      g_snprintf (message, BUF_SIZE,
                  _("Permission denied!!: %s"),
                  dir_internal);
      gtkutil_message_dialog (_("Error!!"), message, window);
      goto ERROR;
   }

   /* add "/" character */
   if (dir[strlen(dir) - 1] == '/')
      dest_dir = g_strdup (dir);
   else
      dest_dir = g_strconcat (dir, "/", NULL);

   /* create progress window */
   progress_win = gtkutil_create_progress_window ("File Operation", "...",
                                                  &cancel, 300, -1, window);
   gtk_grab_add (progress_win);

   /* do file operation */
   length = g_list_length (filelist);
   for (node = filelist; node; node = g_list_next (node)) {
      src_file = node->data;

      while (gtk_events_pending()) gtk_main_iteration();

      pos = g_list_position (filelist, node);
      progress = (gfloat) pos / (gfloat) length;

      result = do_file_operate (src_file, dest_dir, &over_write, action,
                                progress_win, progress, window);
      if (!result)
         success = FALSE;

      /* cancel */
      if (cancel || (over_write == CONFIRM_CANCEL)) break;

      /* reset to CONFIRM_ASK mode */
      if (over_write != CONFIRM_YES_TO_ALL && over_write != CONFIRM_NO_TO_ALL) {
         over_write = CONFIRM_ASK;
      }
   }

   gtk_grab_remove (progress_win);
   gtk_widget_destroy (progress_win);

   g_free (dest_dir);
   g_free (dir_internal);

   return success;

ERROR:
   g_free (dir_internal);
   return FALSE;
}


gboolean
files2dir_with_dialog (GList *filelist, gchar **default_dir, FileOperateType action,
                       GtkWindow *window)
{
   gchar *dir;
   gboolean retval = FALSE;
   gchar *title, *label, *tmpstr;

   g_return_val_if_fail (filelist, FALSE);
   g_return_val_if_fail (default_dir, FALSE);

   if (!*default_dir)
      *default_dir = g_strdup (g_getenv("HOME"));

   switch (action) {
   case FILE_MOVE:
      title = _("Move files to...");
      label = _("Move files to: ");
      break;
   case FILE_COPY:
      title = _("Copy files to...");
      label = _("Copy files to: ");
      break;
   case FILE_LINK:
      title = _("Link files to...");
      label = _("Link files to: ");
      break;
   default:
      dir = *default_dir;
      goto FUNC_END;
      break;
   }

   tmpstr = charset_to_internal (*default_dir,
                                 conf.charset_filename,
                                 conf.charset_auto_detect_fn,
                                 conf.charset_filename_mode);
   dir = gtkutil_modal_file_dialog (title, tmpstr, MODAL_FILE_DIALOG_DIR_ONLY, window);
   g_free (tmpstr);
   tmpstr = NULL;

   if (dir) {
      if (dir[strlen(dir) - 1] != '/') {
         tmpstr = dir;
         dir = g_strconcat (dir, "/", NULL);
         g_free (tmpstr);
      }

      tmpstr = dir;
      dir = charset_internal_to_locale (dir);
      g_free (tmpstr);

      retval = files2dir (filelist, dir, action, window);
   }

FUNC_END:
   if (*default_dir != dir)
      g_free (*default_dir);
   *default_dir = dir;

   return retval;
}


gboolean
delete_dir (const gchar *path, GtkWindow *window)
{
   gboolean exist, cancel, not_empty = FALSE;
   struct stat st;
   gchar message[BUF_SIZE], *path_internal;
   GList *filelist, *listnode;
   gint length, pos;
   gfloat progress;
   GtkWidget *progress_win;
   ConfirmType action;

   g_return_val_if_fail (path && *path, FALSE);

   path_internal = charset_to_internal (path,
                                        conf.charset_filename,
                                        conf.charset_auto_detect_fn,
                                        conf.charset_filename_mode);

   /* check direcotry exist or not */
   exist = !lstat (path, &st);
   if (!exist) {
      g_snprintf (message, BUF_SIZE,
                  _("Directory not exist : %s"), path_internal);
      gtkutil_message_dialog (_("Error!!"), message, window);
      goto ERROR;
   }

   /* check path is link or not */
   if (islink (path)) {
      g_snprintf (message, BUF_SIZE,
                  _("%s is symbolic link.\n"
                    "Remove link ?"), path_internal);
      action = gtkutil_confirm_dialog (_("Confirm Deleting Directory"),
                                       message, 0, window);
      if (action == CONFIRM_YES) {
         remove (path);
      }
      goto SUCCESS;
   }

   /* confirm */
   g_snprintf (message, BUF_SIZE,
               _("Delete %s\n"
                 "OK?"), path_internal);
   action = gtkutil_confirm_dialog (_("Confirm Deleting Directory"),
                                    message, 0, window);
   if (action != CONFIRM_YES) goto ERROR;

   /* remove sub directories recursively */
   filelist = get_dir_all (path);
   if (filelist) {
      g_snprintf (message, BUF_SIZE,
                  _("%s is not empty\n"
                    "Delete all files under %s ?"),
                  path_internal, path_internal);
      action = gtkutil_confirm_dialog (_("Confirm Deleting Directory"),
                                       message, 0, window);
      if (action != CONFIRM_YES) goto ERROR;

      /* create progress bar */
      progress_win = gtkutil_create_progress_window ("Delete File", "Deleting Files",
                                                     &cancel, 300, -1, window);
      gtk_grab_add (progress_win);

      length = g_list_length (filelist);
      listnode = filelist;

      while (listnode) {

         /* update progress */
         pos = g_list_position (filelist, listnode);
         if ((pos % 50) == 0) {
            while (gtk_events_pending()) gtk_main_iteration();

            pos = g_list_position (filelist, listnode);
            progress = (gfloat) pos / (gfloat) length;

            {   /********** convert charset **********/
               gchar *tmpstr;
               tmpstr =  charset_to_internal (listnode->data,
                                              conf.charset_filename,
                                              conf.charset_auto_detect_fn,
                                              conf.charset_filename_mode);
               g_snprintf (message, BUF_SIZE,
                           _("Deleting %s ..."),
                           tmpstr);
               g_free (tmpstr);
            }

            gtkutil_progress_window_update (progress_win, NULL,
                                            message, NULL, progress);
         }

         /* remove a file */
         if (remove ((gchar *) listnode->data) < 0)
            not_empty = TRUE;
         listnode = g_list_next (listnode);

         /* cancel */
         if (cancel) break;
      }
      gtk_grab_remove (progress_win);
      gtk_widget_destroy (progress_win);

      g_list_foreach (filelist, (GFunc) g_free, NULL);
      g_list_free (filelist);
   }

   /* remove the directory */
   if (not_empty) {
      g_snprintf (message, BUF_SIZE,
                  _("Faild to remove directory :\n"
                    "%s is not empty."), path_internal);
      gtkutil_message_dialog (_("Error!!"), message, window);
   } else if (remove (path) < 0) {
      g_snprintf (message, BUF_SIZE,
                  _("Faild to remove directory : %s"), path_internal);
      gtkutil_message_dialog (_("Error!!"), message, window);
   }

SUCCESS:
   g_free (path_internal);
   return TRUE;

ERROR:
   g_free (path_internal);
   return FALSE;
}


gboolean
delete_files (GList *filelist, ConfirmType confirm, GtkWindow *window)
{
   GtkWidget *progress_win = NULL;
   GList *node;
   gboolean cancel = FALSE, dialog = FALSE;
   gint pos, length;
   gfloat progress;
   gchar message[BUF_SIZE], *dirname;

   g_return_val_if_fail (filelist, FALSE);

   length = g_list_length (filelist);

   if (confirm == CONFIRM_ASK)
      dialog = TRUE;

   if (dialog) {
      g_snprintf (message, BUF_SIZE,
                  _("Delete these %d files.\n"
                    "OK?"),
                  length);
      confirm = gtkutil_confirm_dialog (_("Confirm Deleting Files"),
                                        message, 0, window);
   }

   if (!(confirm == CONFIRM_YES || confirm == CONFIRM_YES_TO_ALL)) {
      return FALSE;
   }

   if (dialog) {
      progress_win = gtkutil_create_progress_window (_("Delete File"),
                                                     _("Deleting Files"),
                                                     &cancel, 300, -1, window);
      gtk_grab_add (progress_win);
   }

   node = filelist;
   while (node) {
      gchar *filename = node->data, *filename_internal, *dirname_internal;

      node = g_list_next (node);
      if (!filename || !*filename) continue;

      filename_internal =  charset_to_internal (filename,
                                                conf.charset_filename,
                                                conf.charset_auto_detect_fn,
                                                conf.charset_filename_mode);

      while (gtk_events_pending()) gtk_main_iteration();

      pos = g_list_position (filelist, node);
      progress = (gfloat) pos / (gfloat) length;

      if (dialog && progress_win) {
         g_snprintf (message, BUF_SIZE, _("Deleting %s ..."),
                     filename_internal);
         gtkutil_progress_window_update (progress_win, NULL,
                                         message, NULL, progress);
      }

      dirname = g_dirname (filename);
      dirname_internal =  charset_to_internal (dirname,
                                               conf.charset_filename,
                                               conf.charset_auto_detect_fn,
                                               conf.charset_filename_mode);

      if (!iswritable (dirname)) {
         g_snprintf (message, BUF_SIZE,
                     _("Permission denied : %s"), dirname_internal);
         gtkutil_message_dialog (_("Error!!"), message, window);

      } else { /* remove file!! */
         gboolean success;
         gint ret;

         if (isdir (filename)) {
            success = delete_dir (filename, window);
         } else {
            ret = remove (filename);
            if (ret < 0)
               success = FALSE;
            else
               success = TRUE;
         }

         if (!success) {
            g_snprintf (message, BUF_SIZE,
                        _("Faild to delete file :\n%s"),
                        filename_internal);
            gtkutil_message_dialog (_("Error!!"), message, window);
         }
      }

      g_free (dirname);
      g_free (dirname_internal);
      g_free (filename_internal);
      dirname = NULL;
      dirname_internal  = NULL;
      filename_internal = NULL;

      /* cancel */
      if (cancel) break;
   }

   if (dialog && progress_win) {
      gtk_grab_remove (progress_win);
      gtk_widget_destroy (progress_win);
   }

   return TRUE;
}

gboolean 
make_dir_dialog (const gchar *parent_dir, GtkWindow *window)
{
   gchar *dirname, *path, *parent_path, *tmpstr;
   gboolean success = FALSE, exist;
   struct stat st;
   gchar error_message[BUF_SIZE];

   g_return_val_if_fail (parent_dir && *parent_dir, FALSE);

   parent_path = add_slash (parent_dir);

   if (!iswritable (parent_path)) {
      g_snprintf (error_message, BUF_SIZE,
                  _("Permission denied : %s"), parent_path);
      gtkutil_message_dialog (_("Error!!"), error_message, window);
      goto ERROR0;
   }

   dirname = gtkutil_popup_textentry (_("Make directory"),
                                      _("New directory name: "),
                                      NULL, NULL, -1, 0, window);
   if (!dirname) goto ERROR0;

   tmpstr = charset_internal_to_locale (dirname);
   g_free (dirname);
   dirname = tmpstr;
   path = g_strconcat (parent_path, dirname, NULL);

   exist = !lstat (path, &st);
   if (exist) {
      if (isdir (path))
         g_snprintf (error_message, BUF_SIZE,
                     _("Directory exist : %s"), path);
      else
         g_snprintf (error_message, BUF_SIZE,
                     _("File exist : %s"), path);
      gtkutil_message_dialog (_("Error!!"), error_message, window);
      g_free (path);
      goto ERROR1;
   }

   success = makedir (path);
   if (!success) {
      g_snprintf (error_message, BUF_SIZE,
                  _("Faild to create directory : %s"), path);
      gtkutil_message_dialog (_("Error!!"), error_message, window);
   }

 ERROR1:
   g_free (path);
 ERROR0:
   g_free (parent_path);

   return success;
}


gboolean
rename_dir_dialog (const gchar *dir, GtkWindow *window)
{
   gboolean exist, success = FALSE;
   struct stat st;
   gchar message[BUF_SIZE];
   gchar *path, *parent_dir, *dirname, *src_path, *src_file_internal;
   gchar *dest_path, *tmpstr;

   g_return_val_if_fail (dir && *dir, FALSE);

   path = add_slash (dir);

   /* check direcotry */
   exist = !lstat (path, &st);
   if (!exist) {
      g_snprintf (message, BUF_SIZE,
                  _("Directory not exist :%s"), path);
      gtkutil_message_dialog (_("Error!!"), message, window);
      goto ERROR0;
   }

   src_path = remove_slash (path);
   parent_dir = g_dirname (src_path);

   /* popup rename directory dialog */
   src_file_internal = charset_locale_to_internal (g_path_get_basename (src_path));
   dirname = gtkutil_popup_textentry (_("Rename directory"),
                                      _("New directory name: "),
                                      src_file_internal,
                                      NULL, -1, 0, window);
   g_free (src_file_internal);
   if (!dirname) goto ERROR1;

   tmpstr = charset_internal_to_locale (dirname);
   g_free (dirname);
   dirname = tmpstr;

   dest_path = g_strconcat (parent_dir, "/", dirname, NULL);

   if (rename (src_path, dest_path) < 0) {
      g_snprintf (message, BUF_SIZE,
                  _("Faild to rename directory : %s"), src_path);
      gtkutil_message_dialog (_("Error!!"), message, window);
   } else {
      success = TRUE;
   }

   g_free (dirname);
   g_free (dest_path);
 ERROR1:
   g_free (src_path);
   g_free (parent_dir);
 ERROR0:
   g_free (path);

   return success;
}
