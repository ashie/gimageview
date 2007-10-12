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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gimageview.h"

#include "fileutil.h"
#include "fr-archive.h"
#include "fr-command.h"
#include "gimv_image_info.h"
#include "gimv_plugin.h"
#include "zip-ext.h"


/* plugin implement definition */
static ExtArchiverPlugin plugin_impl[] =
{
   {GIMV_ARCHIVER_IF_VERSION, ".zip", fr_command_zip_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".ear", fr_command_zip_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".jar", fr_command_zip_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".war", fr_command_zip_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".cbz", fr_command_zip_new, TRUE},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_EXT_ARCHIVER)

/* plugin definition */
GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("ZIP archive support"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


static void fr_command_zip_class_init  (FRCommandZipClass *class);

static void fr_command_zip_init        (FRCommand *afile);

static void fr_command_zip_destroy     (GtkObject *object);


/* Parent Class */

static FRCommandClass *parent_class = NULL;


/* -- list -- */

static time_t
mktime_from_string (char *date_s, char *time_s)
{
   struct tm tm = {0, };
   char **fields;

   /* date */

   fields = g_strsplit (date_s, "-", 3);
   if (fields[0] != NULL) {
      tm.tm_mon = atoi (fields[0]) - 1;
      if (fields[1] != NULL) {
         tm.tm_mday = atoi (fields[1]);
         if (fields[2] != NULL)
            tm.tm_year = atoi (fields[2]);
      }
   }
   g_strfreev (fields);

   /* time */

   fields = g_strsplit (time_s, ":", 3);
   if (fields[0] != NULL) {
      tm.tm_hour = atoi (fields[0]) - 1;
      if (fields[1] != NULL) {
         tm.tm_min = atoi (fields[1]);
         if (fields[2] != NULL)
            tm.tm_sec = atoi (fields[2]);
      }
   }
   g_strfreev (fields);

   return mktime (&tm);
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
   int n = 8;

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
   /*
     size_t size;
     time_t mtime;
   */
   struct stat *st;

   g_return_if_fail (line != NULL);

   fields = split_line (line, 7);
   st = g_new0 (struct stat, 1);
   st->st_size  = atol (fields[0]);
   st->st_mtime = mktime_from_string (fields[4], fields[5]);
   st->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
   g_strfreev (fields);

   /* Full path */

   name_field = get_last_field (line);

   if (*name_field == '/') {
      filename = name_field + 1;
   } else {
      filename = name_field;
   }

   if (*filename && *comm->filename)
      fdata = gimv_image_info_get_with_archive (filename,
                                                FR_ARCHIVE (comm->archive),
                                                st);
   g_free (st);
   if (fdata)
      comm->file_list = g_list_prepend (comm->file_list, fdata);
}


static void
fr_command_zip_list (FRCommand *comm)
{
   fr_process_clear (comm->process);
   fr_process_begin_command (comm->process, "unzip");
   fr_process_add_arg (comm->process, "-qq");
   fr_process_add_arg (comm->process, "-v");
   fr_process_add_arg (comm->process, "-l");
   fr_process_add_arg (comm->process, comm->filename);
   fr_process_end_command (comm->process);
   fr_process_start (comm->process, TRUE);
}


static void
fr_command_zip_add (FRCommand *comm,
                    GList *file_list,
                    gchar *base_dir,
                    gboolean update)
{
   GList *scan;

   fr_process_begin_command (comm->process, "zip");

   if (base_dir != NULL) 
      fr_process_set_working_dir (comm->process, base_dir);

   /* preserve links. */
   fr_process_add_arg (comm->process, "-y");

   if (update)
      fr_process_add_arg (comm->process, "-u");

   fr_process_add_arg (comm->process, comm->filename);

   for (scan = file_list; scan; scan = scan->next) 
      fr_process_add_arg (comm->process, (gchar*) scan->data);

   fr_process_end_command (comm->process);
}


static void
fr_command_zip_delete (FRCommand *comm,
                       GList *file_list)
{
   GList *scan;

   fr_process_begin_command (comm->process, "zip");
   fr_process_add_arg (comm->process, "-d");
   fr_process_add_arg (comm->process, comm->filename);

   for (scan = file_list; scan; scan = scan->next)
      fr_process_add_arg (comm->process, scan->data);
   fr_process_end_command (comm->process);
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
fr_command_zip_extract (FRCommand *comm,
                        GList *file_list,
                        char *dest_dir,
                        gboolean overwrite,
                        gboolean skip_older,
                        gboolean junk_paths)
{
   GList *scan;

   fr_process_begin_command (comm->process, "unzip");

   if (dest_dir != NULL) {
      fr_process_add_arg (comm->process, "-d");
      fr_process_add_arg (comm->process, dest_dir);
   }

   if (overwrite)
      fr_process_add_arg (comm->process, "-o");
   else
      fr_process_add_arg (comm->process, "-n");

   if (skip_older)
      fr_process_add_arg (comm->process, "-u");

   if (junk_paths)
      fr_process_add_arg (comm->process, "-j");

   fr_process_add_arg (comm->process, comm->filename);

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
fr_command_zip_class_init (FRCommandZipClass *class)
{
   FRCommandClass *afc;
   GtkObjectClass *object_class;

   parent_class = gtk_type_class (FR_COMMAND_TYPE);
   object_class = (GtkObjectClass*) class;
   afc = (FRCommandClass*) class;

   object_class->destroy = fr_command_zip_destroy;

   afc->list         = fr_command_zip_list;
   afc->add          = fr_command_zip_add;
   afc->delete       = fr_command_zip_delete;
   afc->extract      = fr_command_zip_extract;
}

 
static void 
fr_command_zip_init (FRCommand *comm)
{
   comm->propAddCanUpdate             = TRUE; 
   comm->propExtractCanAvoidOverwrite = TRUE;
   comm->propExtractCanSkipOlder      = TRUE;
   comm->propExtractCanJunkPaths      = TRUE;

   comm->propHasRatio                 = TRUE;
}


static void 
fr_command_zip_destroy (GtkObject *object)
{
   g_return_if_fail (object != NULL);
   g_return_if_fail (IS_FR_COMMAND_ZIP (object));

   /* Chain up */
   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


GtkType
fr_command_zip_get_type ()
{
   static guint fr_command_zip_type = 0;

   if (!fr_command_zip_type) {
      GtkTypeInfo fr_command_zip_info = {
         "FRCommandZip",
         sizeof (FRCommandZip),
         sizeof (FRCommandZipClass),
         (GtkClassInitFunc) fr_command_zip_class_init,
         (GtkObjectInitFunc) fr_command_zip_init,
         /* reserved_1 */ NULL,
         /* reserved_2 */ NULL,
         (GtkClassInitFunc) NULL,
      };
      fr_command_zip_type = gtk_type_unique (fr_command_get_type(), 
                                             &fr_command_zip_info);
   }

   return fr_command_zip_type;
}


FRCommand *
fr_command_zip_new (FRProcess *process,
                    const char *filename,
                    FRArchive  *archive)
{
   FRCommand *comm;

   comm = FR_COMMAND (gtk_type_new (fr_command_zip_get_type ()));
   fr_command_construct (comm, process, filename);
   fr_process_set_proc_line_func (FR_COMMAND (comm)->process, 
                                  process_line,
                                  comm);
   comm->archive = archive;

   return comm;
}
