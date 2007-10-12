/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 *  $Id$
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "gimv_image_info.h"
#include "fileutil.h"
#include "fr-archive.h"
#include "fr-command.h"
#include "fr-process.h"


#define MAX_CHUNK_LEN 16000 /* FIXME : what is the max length of a command 
                             * line ? */


enum {
   START,
   DONE,
   LAST_SIGNAL
};


static GHashTable *ext_archivers = NULL;
static GList *archive_ext_list = NULL;


static guint fr_archive_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (FRArchive, fr_archive, GTK_TYPE_OBJECT)


static void
fr_archive_destroy (GtkObject *object)
{
   FRArchive *archive;

   g_return_if_fail (object != NULL);
   g_return_if_fail (FR_IS_ARCHIVE (object));

   archive = FR_ARCHIVE (object);

   if (archive->command != NULL) 
      gtk_object_unref (GTK_OBJECT (archive->command));

   gtk_object_unref (GTK_OBJECT (archive->process));

   g_print (_("archive \"%s\" has been finalized.\n"), archive->filename);

   if (archive->filename != NULL)
      g_free (archive->filename);

   /* Chain up */
   if (GTK_OBJECT_CLASS (fr_archive_parent_class)->destroy)
      (* GTK_OBJECT_CLASS (fr_archive_parent_class)->destroy) (object);
}


static void
fr_archive_class_init (FRArchiveClass *class)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) class;

   fr_archive_signals[START] =
      gtk_signal_new ("start",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRArchiveClass, start),
                      gtk_marshal_NONE__INT,
                      GTK_TYPE_NONE, 1,
                      GTK_TYPE_INT);

   fr_archive_signals[DONE] =
      gtk_signal_new ("done",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRArchiveClass, done),
                      gtk_marshal_NONE__INT_INT,
                      GTK_TYPE_NONE, 2,
                      GTK_TYPE_INT,
                      GTK_TYPE_INT);

   object_class->destroy = fr_archive_destroy;
   class->start = NULL;
   class->done  = NULL;
}


static void
fr_archive_init (FRArchive *archive)
{
   archive->filename = NULL;
   archive->command = NULL;
   archive->process = fr_process_new ();

   gtk_object_ref (GTK_OBJECT (archive));
   gtk_object_sink (GTK_OBJECT (archive));
}


FRArchive *
fr_archive_new (void)
{
   FRArchive *archive;
   archive = FR_ARCHIVE (gtk_type_new (fr_archive_get_type ()));
   return archive;
}


FRArchive *
fr_archive_ref (FRArchive *archive)
{
   g_return_val_if_fail (archive != NULL, NULL);
   g_return_val_if_fail (FR_IS_ARCHIVE (archive), NULL);

   gtk_object_ref (GTK_OBJECT (archive));

   return archive;
}


void
fr_archive_unref (FRArchive *archive)
{
   g_return_if_fail (archive != NULL);
   g_return_if_fail (FR_IS_ARCHIVE (archive));

   gtk_object_unref (GTK_OBJECT (archive));
}


ExtArchiverPlugin *found_archiver = NULL;

static void
archiver_lookup (gpointer key, gpointer value, gpointer user_data)
{
   char *filename = user_data;

   if (filename && key && fileutil_extension_is (filename, key))
      found_archiver = value;
}


static gboolean
create_command_from_filename (FRArchive * archive, const char *filename)
{
   ExtArchiverPlugin *archiver = NULL;

   g_hash_table_foreach (ext_archivers,
                         archiver_lookup,
                         (gpointer) filename);

   if (!found_archiver) return FALSE;

   archiver = found_archiver;
   found_archiver = NULL;

   archive->command = archiver->archiver_new (archive->process,
                                              filename, archive);
   archive->is_compressed = archiver->is_compressed;

   return TRUE;
}


static void
action_started (FRCommand *command, 
                FRAction action,
                FRArchive *archive)
{
   gtk_signal_emit (GTK_OBJECT (archive), 
                    fr_archive_signals[START],
                    action);
}


static void
action_performed (FRCommand *command, 
                  FRAction action,
                  FRProcError error,
                  FRArchive *archive)
{
#ifdef DEBUG
   char *s_action = NULL;

   switch (action) {
   case FR_ACTION_LIST:
      s_action = "List";
      break;
   case FR_ACTION_ADD:
      s_action = "Add";
      break;
   case FR_ACTION_DELETE:
      s_action = "Delete";
      break;
   case FR_ACTION_EXTRACT:
      s_action = "Extract";
      break;
   }
   g_print ("%s [DONE]\n", s_action);
#endif

   gtk_signal_emit (GTK_OBJECT (archive), 
                    fr_archive_signals[DONE],
                    action,
                    error);
}


void
fr_archive_new_file (FRArchive *archive, char *filename)
{
   FRCommand *tmp_command;

   if (filename == NULL)
      return;

   archive->read_only = FALSE;

   if (archive->filename != NULL)
      g_free (archive->filename);	
   archive->filename = g_strdup (filename);

   tmp_command = archive->command;
   if (! create_command_from_filename (archive, filename))
      return;
   if (tmp_command != NULL) 
      gtk_object_unref (GTK_OBJECT (tmp_command));

   gtk_signal_connect (GTK_OBJECT (archive->command), "start",
                       GTK_SIGNAL_FUNC (action_started),
                       archive);
   gtk_signal_connect (GTK_OBJECT (archive->command), "done",
                       GTK_SIGNAL_FUNC (action_performed),
                       archive);
}


gboolean
fr_archive_load (FRArchive *archive, 
                 const char *filename)
{
   FRCommand *tmp_command;

   g_return_val_if_fail (archive != NULL, FALSE);

   if (access (filename, F_OK) != 0) /* file must exists. */
      return FALSE;

   archive->read_only = access (filename, W_OK) != 0;

   /* without this check you can't call 
    * archive_load (archive, archive->filename) */

   if (archive->filename != filename) {
      if (archive->filename != NULL)
         g_free (archive->filename);
      archive->filename = g_strdup (filename);
   }

   tmp_command = archive->command;
   if (!create_command_from_filename (archive, filename))
      return FALSE;
   if (tmp_command != NULL) 
      gtk_object_unref (GTK_OBJECT (tmp_command));

   gtk_signal_connect (GTK_OBJECT (archive->command), "start",
                       GTK_SIGNAL_FUNC (action_started),
                       archive);

   gtk_signal_connect (GTK_OBJECT (archive->command), "done",
                       GTK_SIGNAL_FUNC (action_performed),
                       archive);
	
   fr_command_list (archive->command);

   return TRUE;
}


void
fr_archive_reload (FRArchive *archive)
{
   g_return_if_fail (archive != NULL);
   g_return_if_fail (archive->filename != NULL);

   fr_command_list (archive->command);
}


void
fr_archive_rename (FRArchive *archive,
                   char *filename)
{
   g_return_if_fail (archive != NULL);

   if (archive->filename != NULL)
      g_free (archive->filename);
   archive->filename = g_strdup (filename);

   fr_command_set_filename (archive->command, filename);
}


/* -- add -- */


static GimvImageInfo *
find_file_in_archive (FRArchive *archive, char *path)
{
   GList *scan;

   for (scan = archive->command->file_list; scan; scan = scan->next) {
      GimvImageInfo *fdata = scan->data;
		
      if (strcmp (path, fdata->filename) == 0)
         return fdata;
   }

   return NULL;
}


#if 0
GList *
get_files_in_archive (FRArchive *archive,
                      GList *file_list,
                      gchar *base_dir,
                      gboolean skip_newer)
{
   GList *files_in_arch = NULL;
   GList *scan;

   for (scan = file_list; scan; scan = scan->next) {
      gchar *filename = scan->data;
      gchar *fullpath;
      GimvImageInfo *fdata;


      fdata = find_file_in_archive (archive, filename);

      if (fdata == NULL) 
         continue;

      fullpath = g_strconcat (base_dir, "/", filename, NULL);

      if (skip_newer
          && isfile (fullpath)
          && (fdata->st.st_mtime > get_file_mtime (fullpath))) {
         g_free (fullpath);
         continue;
      }

      files_in_arch = g_list_prepend (files_in_arch, 
                                      g_strdup (fdata->filename));
      g_free (fullpath);
   }
	
   return files_in_arch;
}
#endif


void
fr_archive_add (FRArchive *archive, 
                GList *file_list, 
                gchar *base_dir,
                gboolean update)
{
   if (archive->read_only)
      return;

   fr_process_clear (archive->process);

   if (file_list == NULL) {
      fr_command_add (archive->command, file_list, base_dir, update);
   } else {
      GList *scan;

      for (scan = file_list; scan != NULL; ) {
         GList *prev = scan->prev;
         GList *chunk_list;
         int l;

         chunk_list = scan;
         l = 0;
         while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
            if (l == 0)
               l = strlen (scan->data);
            prev = scan;
            scan = scan->next;
            if (scan != NULL)
               l += strlen (scan->data);
         }

         prev->next = NULL;
         fr_command_add (archive->command, 
                         chunk_list, 
                         base_dir, 
                         update);
         prev->next = scan;
      }
   }

   fr_process_start (archive->process, FALSE);
}


#if 0
void
fr_archive_add_with_wildcard (FRArchive *archive, 
                              const char *files,
                              gchar *base_dir,
                              gboolean update,
                              gboolean recursive,
                              gboolean follow_links,
                              gboolean same_fs,
                              gboolean no_backup_files,
                              gboolean no_dot_files,
                              gboolean ignore_case)
{
   GList *file_list;

   if (archive->read_only)
      return;

   file_list = get_wildcard_file_list (base_dir, 
                                       files, 
                                       recursive, 
                                       follow_links, 
                                       same_fs,
                                       no_backup_files, 
                                       no_dot_files, 
                                       ignore_case);
   fr_archive_add (archive,
                   file_list,
                   base_dir,
                   update);
   g_list_foreach (file_list, (GFunc) g_free, NULL);
   g_free (file_list);
}


void
fr_archive_add_directory (FRArchive *archive, 
                          const char *directory,
                          gchar *base_dir,
                          gboolean update)

{
   GList *file_list;

   if (archive->read_only)
      return;

   file_list = get_directory_file_list (directory, base_dir);
   fr_archive_add (archive,
                   file_list,
                   base_dir,
                   update);
   g_list_foreach (file_list, (GFunc) g_free, NULL);
   g_list_free (file_list);
}
#endif


/* -- remove -- */


void
fr_archive_remove (FRArchive *archive,
                   GList *file_list)
{
   g_return_if_fail (archive != NULL);

   if (archive->read_only)
      return;

   fr_process_clear (archive->process);

   if (file_list == NULL) {
      fr_command_delete (archive->command, file_list);
   } else {
      GList *scan;

      for (scan = file_list; scan != NULL; ) {
         GList *prev = scan->prev;
         GList *chunk_list;
         int l;
			
         chunk_list = scan;
         l = 0;
         while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
            if (l == 0)
               l = strlen (scan->data);
            prev = scan;
            scan = scan->next;
            if (scan != NULL)
               l += strlen (scan->data);
         }
			
         prev->next = NULL;
         fr_command_delete (archive->command, chunk_list);
         prev->next = scan;
      }
   }

   fr_process_start (archive->process, FALSE);
}


/* -- extract -- */


static void
move_files_to_dir (FRArchive *archive,
                   GList *file_list,
                   char *source_dir,
                   char *dest_dir)
{
   GList *scan;

   fr_process_begin_command (archive->process, "mv");
   fr_process_add_arg (archive->process, "-f");
   for (scan = file_list; scan; scan = scan->next) {
      char path[4096];
      char *filename = scan->data;

      if (filename[0] == '/')
         sprintf (path, "%s%s", source_dir, filename);
      else
         sprintf (path, "%s/%s", source_dir, filename);
		
      fr_process_add_arg (archive->process, path);
   }
   fr_process_add_arg (archive->process, dest_dir);
   fr_process_end_command (archive->process);
}


static void
move_files_in_chunks (FRArchive *archive,
                      GList *file_list,
                      char *temp_dir,
                      char *dest_dir)
{
   GList *scan;
   int temp_dir_l = strlen (temp_dir);

   for (scan = file_list; scan != NULL; ) {
      GList *prev = scan->prev;
      GList *chunk_list;
      int l;

      chunk_list = scan;
      l = 0;
      while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
         if (l == 0)
            l = temp_dir_l + 1 + strlen (scan->data);
         prev = scan;
         scan = scan->next;
         if (scan != NULL)
            l += temp_dir_l + 1 + strlen (scan->data);
      }

      prev->next = NULL;
      move_files_to_dir (archive,
                         chunk_list,
                         temp_dir, dest_dir);
      prev->next = scan;
   }	
}


static void
extract_in_chunks (FRCommand *command,
                   GList *file_list,
                   char *dest_dir,
                   gboolean overwrite,
                   gboolean skip_older,
                   gboolean junk_paths)
{
   GList *scan;

   if (file_list == NULL) {
      fr_command_extract (command,
                          file_list,
                          dest_dir,
                          overwrite,
                          skip_older,
                          junk_paths);
      return;
   }

   for (scan = file_list; scan != NULL; ) {
      GList *prev = scan->prev;
      GList *chunk_list;
      int l;

      chunk_list = scan;
      l = 0;
      while ((scan != NULL) && (l < MAX_CHUNK_LEN)) {
         if (l == 0)
            l = strlen (scan->data);
         prev = scan;
         scan = scan->next;
         if (scan != NULL)
            l += strlen (scan->data);
      }

      prev->next = NULL;
      fr_command_extract (command,
                          chunk_list,
                          dest_dir,
                          overwrite,
                          skip_older,
                          junk_paths);
      prev->next = scan;
   }
}


void
fr_archive_extract (FRArchive *archive,
                    GList * file_list,
                    char * dest_dir,
                    gboolean skip_older,
                    gboolean overwrite,
                    gboolean junk_paths)
{
   GList *filtered;
   GList *scan;
   gboolean extract_all;
   gboolean move_to_dest_dir;

   g_return_if_fail (archive != NULL);

   /* if a command supports all the requested options... */

   if (! (! archive->command->propExtractCanAvoidOverwrite && ! overwrite)
       && ! (! archive->command->propExtractCanSkipOlder && skip_older)
       && ! (! archive->command->propExtractCanJunkPaths && junk_paths)) {

      fr_process_clear (archive->process);
      extract_in_chunks (archive->command,
                         file_list,
                         dest_dir,
                         overwrite,
                         skip_older,
                         junk_paths);

      fr_process_start (archive->process, FALSE);
      return;
   }

   /* .. else we have to implement the unsupported ones. */

   fr_process_clear (archive->process);

   move_to_dest_dir = (junk_paths 
                       && ! archive->command->propExtractCanJunkPaths);

   extract_all = (file_list == NULL);
   if (extract_all) {
      GList *scan;

      scan = archive->command->file_list;
      for (; scan; scan = scan->next) {
         GimvImageInfo *fdata = scan->data;
         file_list = g_list_prepend (file_list, g_strdup (fdata->filename));
      }
   }

   filtered = NULL;
   for (scan = file_list; scan; scan = scan->next) {
      GimvImageInfo *fdata;
      char *filename = scan->data;
      char full_name[4096];

      if (filename[0] == '/')
         sprintf (full_name, "%s%s", dest_dir, filename);
      else
         sprintf (full_name, "%s/%s", dest_dir, filename);
      fdata = find_file_in_archive (archive, filename);

      if (fdata == NULL)
         continue;

      if (skip_older 
          && isfile (full_name)
          && (fdata->st.st_mtime < get_file_mtime (full_name)))
         continue;

      if (! overwrite && isfile (full_name))
         continue;

      filtered = g_list_prepend (filtered, fdata->filename);
   }

   if (filtered != NULL) 
      filtered = g_list_reverse (filtered);
   else {
      /* all files got filtered, do nothing. */
      g_list_foreach (file_list, (GFunc) g_free, NULL);
      g_list_free (file_list);
      return;
   } 

   if (move_to_dest_dir) {
      char *temp_dir;

      temp_dir = g_strdup_printf ("%s%s%d",
                                  g_get_tmp_dir (),
                                  "/gimv.",
                                  getpid ());
      ensure_dir_exists (temp_dir);
      extract_in_chunks (archive->command,
                         filtered,
                         temp_dir,
                         overwrite,
                         skip_older,
                         junk_paths);

      move_files_in_chunks (archive, filtered, temp_dir, dest_dir);

      /* remove the temp dir. */
      fr_process_begin_command (archive->process, "rm");
      fr_process_add_arg (archive->process, "-rf");
      fr_process_add_arg (archive->process, temp_dir);
      fr_process_end_command (archive->process);
		
      g_free (temp_dir);
   } else {
      extract_in_chunks (archive->command,
                         filtered,
                         dest_dir,
                         overwrite,
                         skip_older,
                         junk_paths);
   }

   if (filtered != NULL)
      g_list_free (filtered);

   if (extract_all) {
      /* the list has been created in this function. */
      g_list_foreach (file_list, (GFunc) g_free, NULL);
      g_list_free (file_list);
   }

   fr_process_start (archive->process, FALSE);
}


gchar *
fr_archive_utils_get_file_name_ext (const char *filename)
{
   GList *node;

   node = archive_ext_list;
   while (node) {
      gchar *ext = node->data;

      if (ext && *ext && fileutil_extension_is (filename, ext)) {
         return ext;
      }
      node = g_list_next (node);
   }

   return NULL;
}


gboolean
fr_archive_plugin_regist (const gchar *plugin_name,
                          const gchar *module_name,
                          gpointer     impl,
                          gint         size)
{
   ExtArchiverPlugin *archiver = impl;

   g_return_val_if_fail (module_name, FALSE);
   g_return_val_if_fail (archiver, FALSE);
   g_return_val_if_fail (size > 0, FALSE);
   g_return_val_if_fail (archiver->if_version == GIMV_ARCHIVER_IF_VERSION, FALSE);
   g_return_val_if_fail (archiver->format && archiver->archiver_new, FALSE);

   if (!ext_archivers)
      ext_archivers = g_hash_table_new (g_str_hash, g_str_equal);

   g_hash_table_insert (ext_archivers, archiver->format, archiver);
   archive_ext_list = g_list_append (archive_ext_list,
                                     g_strdup (archiver->format));

   return TRUE;
}
