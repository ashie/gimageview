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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include "gimageview.h"

#include "fr-archive.h"
#include "fr-command.h"
#include "gimv_image_info.h"
#include "gimv_plugin.h"
#include "utils_file.h"
#include "tar-ext.h"


G_DEFINE_TYPE (FRCommandTar, fr_command_tar, FR_TYPE_COMMAND)


static void       fr_command_tar_dispose      (GObject    *object);


static FRCommand *fr_command_tar_none_new     (FRProcess  *process,
                                               const char *filename,
                                               FRArchive  *archive);
static FRCommand *fr_command_tar_gz_new       (FRProcess  *process,
                                               const char *filename,
                                               FRArchive  *archive);
static FRCommand *fr_command_tar_bz2_new      (FRProcess  *process,
                                               const char *filename,
                                               FRArchive  *archive);
static FRCommand *fr_command_tar_bz_new       (FRProcess  *process,
                                               const char *filename,
                                               FRArchive  *archive);
static FRCommand *fr_command_tar_compress_new (FRProcess  *process,
                                               const char *filename,
                                               FRArchive  *archive);

/* plugin implement definition */
static ExtArchiverPlugin plugin_impl[] =
{
   {GIMV_ARCHIVER_IF_VERSION, ".tar",     fr_command_tar_none_new,     FALSE},
   {GIMV_ARCHIVER_IF_VERSION, ".tar.gz",  fr_command_tar_gz_new,       TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tgz",     fr_command_tar_gz_new,       TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tar.bz2", fr_command_tar_bz2_new,      TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tbz2",    fr_command_tar_bz2_new,      TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tar.bz",  fr_command_tar_bz_new,       TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tbz",     fr_command_tar_bz_new,       TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tar.Z",   fr_command_tar_compress_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".taz",     fr_command_tar_compress_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tar.lzo", fr_command_tar_compress_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".tzo",     fr_command_tar_compress_new, TRUE},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_EXT_ARCHIVER)

/* plugin definition */
GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("TAR archive support"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


/* -- list -- */

static time_t
mktime_from_string (char *date_s, char *time_s)
{
   struct tm tm = {0, };
   char **fields;

   g_return_val_if_fail (date_s && *date_s, 0);
   g_return_val_if_fail (time_s && *time_s, 0);

   /* date */

   fields = g_strsplit (date_s, "-", 3);
   if (fields[0] != NULL) {
      tm.tm_year = atoi (fields[0]) - 1900;
      if (fields[1] != NULL) {
         tm.tm_mon = atoi (fields[1]) - 1;
         if (fields[2] != NULL)
            tm.tm_mday = atoi (fields[2]);
      }
   }
   g_strfreev (fields);

   /* time */

   fields = g_strsplit (time_s, ":", 3);
   if (fields[0] != NULL) {
      tm.tm_hour = atoi (fields[0]) - 1;
      if (fields[1] != NULL) {
         tm.tm_min  = atoi (fields[1]);
         if (fields[2] != NULL)
            tm.tm_sec  = atoi (fields[2]);
      }
   }
   g_strfreev (fields);

   return mktime (&tm);
}


static mode_t
mkmode_from_string (char *mode_s)
{
   mode_t mode = 0;
   gint len;

   g_return_val_if_fail (mode_s && *mode_s, mode);

   len = strlen (mode_s);
   g_return_val_if_fail (len > 9, mode);

   switch (mode_s[0]) {
   case '-':
      mode = mode | S_IFREG;
      break;
   case 'l':
      mode = mode | S_IFLNK;
      break;
   case 'd':
      mode = mode | S_IFDIR;
      break;
   default:
      break;
   }

   if (mode_s[1] == 'r')
      mode = mode | S_IRUSR;
   if (mode_s[2] == 'w')
      mode = mode | S_IWUSR;
   if (mode_s[3] == 'x')
      mode = mode | S_IXUSR;
   else if (mode_s[3] == 'S')
      mode = mode | S_ISUID;

   if (mode_s[4] == 'r')
      mode = mode | S_IRGRP;
   if (mode_s[5] == 'w')
      mode = mode | S_IWGRP;
   if (mode_s[6] == 'x')
      mode = mode | S_IXGRP;
   else if (mode_s[6] == 'S')
      mode = mode | S_ISGID;

   if (mode_s[7] == 'r')
      mode = mode | S_IROTH;
   if (mode_s[8] == 'w')
      mode = mode | S_IWOTH;
   if (mode_s[9] == 'x')
      mode = mode | S_IXOTH;
   else if (mode_s[9] == 'T')
      mode = mode | S_ISVTX;

   return mode;
}


static void
mkugid_from_string (char *ugid_s, uid_t *uid_ret, gid_t *gid_ret)
{
   gchar **ids;
   struct passwd *pw;
   struct group *gr;

   g_return_if_fail (ugid_s && *ugid_s);
   g_return_if_fail (uid_ret && gid_ret);

   ids = g_strsplit (ugid_s, "/", 2);
   g_return_if_fail (ids);

   pw = getpwnam (ids[0]);
   gr = getgrnam (ids[1]);

   if (pw)
      *uid_ret = pw->pw_uid;
   else
      *uid_ret = atoi (ids[0]);

   if (gr)
      *gid_ret = gr->gr_gid;
   else
      *gid_ret = atoi (ids[1]);

   g_strfreev (ids);
}


static char *
eat_spaces (char *line)
{
   while ((*line == ' ') && (*line != 0))
      line++;
   return line;
}


static char **
split_line (char *line, int n_fields)
{
   char **fields;
   char *scan, *field_end;
   int i;

   fields = g_new0 (char *, n_fields + 1);
   fields[n_fields] = NULL;

   scan = eat_spaces (line);
   for (i = 0; i < n_fields; i++) {
      field_end = strchr (scan, ' ');
      fields[i] = g_strndup (scan, field_end - scan);
      scan = eat_spaces (field_end);
   }

   return fields;
}


static char *
get_last_field (char *line)
{
   int i;
   char *field;
   int n = 6;

   n--;
   field = eat_spaces (line);
   for (i = 0; i < n; i++) {
      field = strchr (field, ' ');
      field = eat_spaces (field);
   }

   return field;
}


static void
process_line (char *line, gpointer data)
{
   GimvImageInfo *fdata = NULL;
   FRCommand *comm = FR_COMMAND (data);
   char **fields;
   char *name_field;
   char *filename;
   struct stat st;

   g_return_if_fail (line != NULL);

   fields = split_line (line, 5);
   memset (&st, 0, sizeof (struct stat));
   st.st_size = atol (fields[2]);
   st.st_mtime = mktime_from_string (fields[3], fields[4]);
   mkugid_from_string (fields[1], &st.st_uid, &st.st_gid);
   st.st_mode = mkmode_from_string (fields[0]);
   g_strfreev (fields);

   /* Full path */

   name_field = get_last_field (line);
   fields = g_strsplit (name_field, " -> ", 2);

   if (!fields[1]) {
      g_strfreev (fields);
      fields = g_strsplit (name_field, " link to ", 2);
   }

   if (*(fields[0]) == '/') {
      filename = fields[0] + 1;
   } else {
      filename = fields[0];
   }

   if (*filename && *comm->filename) {
      fdata = gimv_image_info_get_with_archive (filename,
                                                FR_ARCHIVE (comm->archive),
                                                &st);
   }
   if (fdata) {
      if (fields[1])
         gimv_image_info_set_link (fdata, fields[1]);
      comm->file_list = g_list_prepend (comm->file_list, fdata);
   }

   g_strfreev (fields);
}


static void
add_compress_arg (FRCommand *comm)
{
   FRCommandTar *comm_tar = FR_COMMAND_TAR (comm);

   switch (comm_tar->compress_prog) {
   case FR_COMPRESS_PROGRAM_NONE:
      break;

   case FR_COMPRESS_PROGRAM_GZIP:
      fr_process_add_arg (comm->process, "-z");
      break;

   case FR_COMPRESS_PROGRAM_BZIP:
      fr_process_add_arg (comm->process, 
                          "--use-compress-program bzip");
      break;

   case FR_COMPRESS_PROGRAM_BZIP2:
      fr_process_add_arg (comm->process, "--bzip");
      break;

   case FR_COMPRESS_PROGRAM_COMPRESS:
      fr_process_add_arg (comm->process, "-Z");
      break;
   case FR_COMPRESS_PROGRAM_LZO:
      fr_process_add_arg (comm->process, 
                          "--use-compress-program lzop");
      break;
   }
}


static gchar*
uncompress (FRCommand *comm, gboolean *name_modified)
{
   gchar *new_name;
   gint l;
   FRCommandTar *comm_tar = FR_COMMAND_TAR (comm);

   *name_modified = FALSE;
   new_name = g_strdup (comm->filename);
   l = strlen (new_name);

   switch (comm_tar->compress_prog) {
   case FR_COMPRESS_PROGRAM_NONE:
      break;

   case FR_COMPRESS_PROGRAM_GZIP:
      if (isfile (comm->filename)) {
         fr_process_begin_command (comm->process, "gzip");
         fr_process_add_arg (comm->process, "-d");
         fr_process_add_arg (comm->process, comm->filename);
         fr_process_end_command (comm->process);
      }

      /* X.tgz     -->  X.tar 
       * X.tar.gz  -->  X.tar */
      if (fileutil_extension_is (comm->filename, ".tgz")) {
         *name_modified = TRUE;
         new_name[l - 2] = 'a';
         new_name[l - 1] = 'r';
      } else if (fileutil_extension_is (comm->filename, ".tar.gz")) 
         new_name[l - 3] = 0;
      break;

   case FR_COMPRESS_PROGRAM_BZIP:
      if (isfile (comm->filename)) {
         fr_process_begin_command (comm->process, "bzip");
         fr_process_add_arg (comm->process, "-d");
         fr_process_add_arg (comm->process, comm->filename);
         fr_process_end_command (comm->process);
      }

      /* Remove the .bz extension. */
      new_name[l - 3] = 0;
      break;

   case FR_COMPRESS_PROGRAM_BZIP2:
      if (isfile (comm->filename)) {
         fr_process_begin_command (comm->process, "bzip2");
         fr_process_add_arg (comm->process, "-d");
         fr_process_add_arg (comm->process, comm->filename);
         fr_process_end_command (comm->process);
      }

      /* Remove the .bz2 extension. */
      new_name[l - 4] = 0;
      break;

   case FR_COMPRESS_PROGRAM_COMPRESS: 
      if (isfile (comm->filename)) {
         fr_process_begin_command (comm->process, "uncompress");
         fr_process_add_arg (comm->process, "-f");
         fr_process_add_arg (comm->process, comm->filename);
         fr_process_end_command (comm->process);
      }

      /* X.taz   -->  X.tar 
       * X.tar.Z -->  X.tar */
      if (fileutil_extension_is (comm->filename, ".taz")) {
         *name_modified = TRUE;
         new_name[l - 1] = 'r';
      } else if (fileutil_extension_is (comm->filename, ".tar.Z")) 
         new_name[l - 2] = 0;
      break;

   case FR_COMPRESS_PROGRAM_LZO:
      if (isfile (comm->filename)) {
         fr_process_begin_command (comm->process, "lzop");
         fr_process_add_arg (comm->process, "-d");
         fr_process_add_arg (comm->process, comm->filename);
         fr_process_end_command (comm->process);
      }

      /* X.tzo     -->  X.tar 
       * X.tar.lzo -->  X.tar */
      if (fileutil_extension_is (comm->filename, ".tzo")) {
         *name_modified = TRUE;
         new_name[l - 2] = 'a';
         new_name[l - 1] = 'r';
      } else if (fileutil_extension_is (comm->filename, ".tar.lzo")) 
         new_name[l - 4] = 0;
      break;
   }

   return new_name;
}


static char *
recompress (FRCommand *comm, char *uncompressed_name)
{
   gchar *new_name = NULL;
   FRCommandTar *comm_tar = FR_COMMAND_TAR (comm);

   switch (comm_tar->compress_prog) {
   case FR_COMPRESS_PROGRAM_NONE:
      break;

   case FR_COMPRESS_PROGRAM_GZIP:
      fr_process_begin_command (comm->process, "gzip");
      fr_process_add_arg (comm->process, uncompressed_name);
      fr_process_end_command (comm->process);

      new_name = g_strconcat (uncompressed_name, ".gz", NULL);
      break;

   case FR_COMPRESS_PROGRAM_BZIP:
      fr_process_begin_command (comm->process, "bzip");
      fr_process_add_arg (comm->process, uncompressed_name);
      fr_process_end_command (comm->process);

      new_name = g_strconcat (uncompressed_name, ".bz", NULL);
      break;

   case FR_COMPRESS_PROGRAM_BZIP2:
      fr_process_begin_command (comm->process, "bzip2");
      fr_process_add_arg (comm->process, uncompressed_name);
      fr_process_end_command (comm->process);

      new_name = g_strconcat (uncompressed_name, ".bz2", NULL);
      break;

   case FR_COMPRESS_PROGRAM_COMPRESS: 
      fr_process_begin_command (comm->process, "compress");
      fr_process_add_arg (comm->process, "-f");
      fr_process_add_arg (comm->process, uncompressed_name);
      fr_process_end_command (comm->process);

      new_name = g_strconcat (uncompressed_name, ".Z", NULL);
      break;

   case FR_COMPRESS_PROGRAM_LZO:
      fr_process_begin_command (comm->process, "lzop");
      fr_process_add_arg (comm->process, uncompressed_name);
      fr_process_end_command (comm->process);

      new_name = g_strconcat (uncompressed_name, ".lzo", NULL);
      break;
   }

   return new_name;
}


static void
fr_command_tar_list (FRCommand *comm)
{
   fr_process_clear (comm->process);
   fr_process_begin_command (comm->process, "tar");
   fr_process_add_arg (comm->process, "-tvf");
   fr_process_add_arg (comm->process, comm->filename);
   add_compress_arg (comm);
   fr_process_end_command (comm->process);
   fr_process_start (comm->process, TRUE);
}


static void
fr_command_tar_add (FRCommand *comm,
                    GList *file_list,
                    gchar *base_dir,
                    gboolean update)
{
   GList *scan;
   gchar *new_filename, *new_filename2;
   gboolean name_modified;

   new_filename = uncompress (comm, &name_modified);

   /* Add files. */

   fr_process_begin_command (comm->process, "tar");

   if (base_dir != NULL) {
      fr_process_add_arg (comm->process, "-C");
      fr_process_add_arg (comm->process, base_dir);
   }

   if (update)
      fr_process_add_arg (comm->process, "-uf");
   else
      fr_process_add_arg (comm->process, "-rf");

   fr_process_add_arg (comm->process, new_filename);
   for (scan = file_list; scan; scan = scan->next) 
      fr_process_add_arg (comm->process, scan->data);
   fr_process_end_command (comm->process);

   new_filename2 = recompress (comm, new_filename);

   /* Restore original name. */

   if (name_modified) {
      fr_process_begin_command (comm->process, "mv");
      fr_process_add_arg (comm->process, "-f");
      fr_process_add_arg (comm->process, new_filename2);
      fr_process_add_arg (comm->process, comm->filename);
      fr_process_end_command (comm->process);
   }
   g_free(new_filename);
   g_free(new_filename2);
}


static void
fr_command_tar_delete (FRCommand *comm,
                       GList *file_list)
{
   GList *scan;
   gchar *new_filename, *new_filename2;
   gboolean name_modified;

   new_filename = uncompress (comm, &name_modified);

   /* Delete files. */

   fr_process_begin_command (comm->process, "tar");
   fr_process_add_arg (comm->process, "--delete");
   fr_process_add_arg (comm->process, "-f");
   fr_process_add_arg (comm->process, new_filename);

   for (scan = file_list; scan; scan = scan->next)
      fr_process_add_arg (comm->process, scan->data);
   fr_process_end_command (comm->process);

   new_filename2 = recompress (comm, new_filename);

   /* Restore original name. */

   if (name_modified) {
      fr_process_begin_command (comm->process, "mv");
      fr_process_add_arg (comm->process, "-f");
      fr_process_add_arg (comm->process, new_filename2);
      fr_process_add_arg (comm->process, comm->filename);
      fr_process_end_command (comm->process);
   }
   g_free(new_filename);
   g_free(new_filename2);
}


static gchar *
escape_path (const gchar *path)
{
   guint len, i, j = 0;
   gchar *newpath;

   g_return_val_if_fail (path, NULL);

   len = strlen (path);

   for (i = 0; path[i]; i++) {
      switch (path[i]) {
      case '[':
      case '*':
      case '?':
      case '"':
      case '\'':
         len += 2;
         break;
      default:
         break;
      }
   }

   newpath = g_malloc(len + 1);

   for (i = 0; path[i]; i++) {
      switch (path[i]) {
      case '[':
         newpath[j] = '['; j++;
         newpath[j] = '['; j++;
         newpath[j] = ']'; j++;
         break;
      case '*':
         newpath[j] = '['; j++;
         newpath[j] = '*'; j++;
         newpath[j] = ']'; j++;
         break;
      case '?':
         newpath[j] = '['; j++;
         newpath[j] = '?'; j++;
         newpath[j] = ']'; j++;
         break;
      case '"':
         newpath[j] = '['; j++;
         newpath[j] = '"'; j++;
         newpath[j] = ']'; j++;
         break;
      case '\'':
         newpath[j] = '['; j++;
         newpath[j] = '\''; j++;
         newpath[j] = ']'; j++;
         break;
      default:
         newpath[j] = path[i]; j++;
         break;
      }
   }

   newpath[len] = '\0';

   return newpath;
}


static void
fr_command_tar_extract (FRCommand *comm,
                        GList *file_list,
                        char *dest_dir,
                        gboolean overwrite,
                        gboolean skip_older,
                        gboolean junk_paths)
{
   GList *scan;

   fr_process_begin_command (comm->process, "tar");
	
   fr_process_add_arg (comm->process, "-xf");
   fr_process_add_arg (comm->process, comm->filename);
   add_compress_arg (comm);

   if (dest_dir != NULL) {
      fr_process_add_arg (comm->process, "-C");
      fr_process_add_arg (comm->process, dest_dir);
   }

   /*
     if (! overwrite)
     fr_process_add_arg (comm->process, "-k");
   */

   for (scan = file_list; scan; scan = scan->next) {
      gchar *escaped = escape_path(scan->data);

      if (escaped) {
         fr_process_add_arg (comm->process, escaped);
         g_free(escaped);
      }
   }

   fr_process_end_command (comm->process);
}


static void 
fr_command_tar_class_init (FRCommandTarClass *class)
{
   FRCommandClass *afc;
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass*) class;
   afc = (FRCommandClass*) class;

   gobject_class->dispose = fr_command_tar_dispose;

   afc->list         = fr_command_tar_list;
   afc->add          = fr_command_tar_add;
   afc->delete       = fr_command_tar_delete;
   afc->extract      = fr_command_tar_extract;
}

 
static void 
fr_command_tar_init (FRCommandTar *tar_comm)
{
   FRCommand *comm = FR_COMMAND(tar_comm);

   comm->propAddCanUpdate             = TRUE; 
   comm->propExtractCanAvoidOverwrite = FALSE;
   comm->propExtractCanSkipOlder      = FALSE;
   comm->propExtractCanJunkPaths      = FALSE;

   comm->propHasRatio                 = FALSE;
}


static void 
fr_command_tar_dispose (GObject *object)
{
   g_return_if_fail (object != NULL);
   g_return_if_fail (FR_IS_COMMAND_TAR (object));

   /* Chain up */
   if (G_OBJECT_CLASS (fr_command_tar_parent_class)->dispose)
      G_OBJECT_CLASS (fr_command_tar_parent_class)->dispose (object);
}


FRCommand *
fr_command_tar_new (FRProcess *process,
                    const char *filename,
                    FRArchive  *archive,
                    FRCompressProgram prog)
{
   FRCommand *comm;

   comm = FR_COMMAND (g_object_new (FR_TYPE_COMMAND_TAR, NULL));
   fr_command_construct (comm, process, filename);
   fr_process_set_proc_line_func (FR_COMMAND (comm)->process, 
                                  process_line,
                                  comm);
   comm->archive = archive;
   FR_COMMAND_TAR (comm)->compress_prog = prog;

   return comm;
}


static FRCommand *
fr_command_tar_gz_new (FRProcess *process,
                       const char *filename,
                       FRArchive  *archive)
{
   return fr_command_tar_new (process,
                              filename,
                              archive,
                              FR_COMPRESS_PROGRAM_GZIP);
}


static FRCommand *
fr_command_tar_bz2_new (FRProcess *process,
                        const char *filename,
                        FRArchive  *archive)
{
   return fr_command_tar_new (process,
                              filename,
                              archive,
                              FR_COMPRESS_PROGRAM_BZIP2);
}


static FRCommand *
fr_command_tar_bz_new (FRProcess *process,
                       const char *filename,
                       FRArchive  *archive)
{
   return fr_command_tar_new (process,
                              filename,
                              archive,
                              FR_COMPRESS_PROGRAM_BZIP);
}


static FRCommand *
fr_command_tar_compress_new (FRProcess *process,
                             const char *filename,
                             FRArchive  *archive)
{
   return fr_command_tar_new (process,
                              filename,
                              archive,
                              FR_COMPRESS_PROGRAM_COMPRESS);
}


static FRCommand *
fr_command_tar_none_new (FRProcess *process,
                         const char *filename,
                         FRArchive  *archive)
{
   return fr_command_tar_new (process,
                              filename,
                              archive,
                              FR_COMPRESS_PROGRAM_NONE);
}
