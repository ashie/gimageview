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

#include "fr-archive.h"
#include "fr-command.h"
#include "gimv_image_info.h"
#include "gimv_plugin.h"
#include "utils_file.h"
#include "rar-ext.h"


G_DEFINE_TYPE (FRCommandRar, fr_command_rar, FR_TYPE_COMMAND)


/* plugin implement definition */
static ExtArchiverPlugin plugin_impl[] =
{
   {GIMV_ARCHIVER_IF_VERSION, ".rar", fr_command_rar_new, TRUE},
   {GIMV_ARCHIVER_IF_VERSION, ".cbr", fr_command_rar_new, TRUE},
};

GIMV_PLUGIN_GET_IMPL(plugin_impl, GIMV_PLUGIN_EXT_ARCHIVER)

/* plugin definition */
GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("RAR archive support"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  NULL,
};


static void fr_command_rar_dispose (GObject *object);


/* -- list -- */

static time_t
mktime_from_string (char *date_s, char *time_s)
{
   struct tm tm = {0, };
   char **fields;

   /* date */

   fields = g_strsplit (date_s, "-", 3);
   if (fields[0] != NULL) {
      tm.tm_mday = atoi (fields[0]);
      if (fields[1] != NULL) {
         tm.tm_mon = atoi (fields[1]) - 1;
         if (fields[2] != NULL)
            tm.tm_year = 100 + atoi (fields[2]);
      }
   }
   g_strfreev (fields);

   /* time */

   fields = g_strsplit (time_s, ":", 2);
   if (fields[0] != NULL) {
      tm.tm_hour = atoi (fields[0]) - 1;
      if (fields[1] != NULL) 
         tm.tm_min = atoi (fields[1]);
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
      if (field_end != NULL) {
         fields[i] = g_strndup (scan, field_end - scan);
         scan = eat_spaces (field_end);
      }
   }

   return fields;
}


static char *
get_last_field (char *line)
{
   int i;
   char *field;
   int n = 1;

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
   FRCommand *comm = FR_COMMAND (data);
   FRCommandRar *rar_comm = FR_COMMAND_RAR (comm);
   char **fields;
   char *name_field;
   char *filename;

   g_return_if_fail (line != NULL);

   if (! rar_comm->list_started) {
      if (strncmp (line, "--------", 8) == 0) {
         rar_comm->list_started = TRUE;
         rar_comm->odd_line = TRUE;
      }
      return;
   }

   if (strncmp (line, "--------", 8) == 0) {
      rar_comm->list_started = FALSE;
      return;
   }

   if (rar_comm->odd_line) {
      /* read file name. */

      name_field = get_last_field (line);

      if (*name_field == '/') {
         filename = name_field + 1;
      } else {
         filename = name_field;
      }

      rar_comm->fdata = gimv_image_info_get_with_archive (filename,
                                                          FR_ARCHIVE (comm->archive),
                                                          NULL);
   } else {
      GimvImageInfo *fdata;
      struct stat *st;
      /*
        size_t size;
        time_t mtime;
      */

      fdata = rar_comm->fdata;

      /* read file info. */

      fields = split_line (line, 5);
      st = g_new0 (struct stat, 1);
      st->st_size  = atol (fields[0]);
      st->st_mtime = mktime_from_string (fields[3], fields[4]); 
      st->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
      g_strfreev (fields);

      rar_comm->fdata
         = gimv_image_info_get_with_archive (fdata->filename,
                                             gimv_image_info_get_archive (fdata),
                                             st);
      g_free (st);

      if (*fdata->filename == 0)
         gimv_image_info_unref (fdata);
      else
         comm->file_list = g_list_prepend (comm->file_list, fdata);

      rar_comm->fdata = NULL;
   }

   rar_comm->odd_line = ! rar_comm->odd_line;
}


static void
fr_command_rar_list (FRCommand *comm)
{
   FR_COMMAND_RAR(comm)->list_started = FALSE;

   fr_process_clear (comm->process);
   fr_process_begin_command (comm->process, "rar");
   fr_process_add_arg (comm->process, "v");
   fr_process_add_arg (comm->process, "-c-");

   /* stop switches scanning */
   fr_process_add_arg (comm->process, "--"); 

   fr_process_add_arg (comm->process, comm->filename);
   fr_process_end_command (comm->process);
   fr_process_start (comm->process, TRUE);
}


static void
fr_command_rar_add (FRCommand *comm,
                    GList *file_list,
                    gchar *base_dir,
                    gboolean update)
{
   GList *scan;

   fr_process_begin_command (comm->process, "rar");

   if (base_dir != NULL) 
      fr_process_set_working_dir (comm->process, base_dir);

   if (update)
      fr_process_add_arg (comm->process, "u");
   else
      fr_process_add_arg (comm->process, "a");

   /* stop switches scanning */
   fr_process_add_arg (comm->process, "--"); 

   fr_process_add_arg (comm->process, comm->filename);

   for (scan = file_list; scan; scan = scan->next) 
      fr_process_add_arg (comm->process, (gchar*) scan->data);

   fr_process_end_command (comm->process);
}


static void
fr_command_rar_delete (FRCommand *comm,
                       GList *file_list)
{
   GList *scan;

   fr_process_begin_command (comm->process, "rar");
   fr_process_add_arg (comm->process, "d");

   /* stop switches scanning */
   fr_process_add_arg (comm->process, "--"); 

   fr_process_add_arg (comm->process, comm->filename);

   for (scan = file_list; scan; scan = scan->next)
      fr_process_add_arg (comm->process, scan->data);
   fr_process_end_command (comm->process);
}


static void
fr_command_rar_extract (FRCommand *comm,
                        GList *file_list,
                        char *dest_dir,
                        gboolean overwrite,
                        gboolean skip_older,
                        gboolean junk_paths)
{
   GList *scan;

   fr_process_begin_command (comm->process, "rar");
   fr_process_add_arg (comm->process, "x");

   if (overwrite)
      fr_process_add_arg (comm->process, "-o+");
   else
      fr_process_add_arg (comm->process, "-o-");	

   if (skip_older)
      fr_process_add_arg (comm->process, "-u");

   if (junk_paths)
      fr_process_add_arg (comm->process, "-ep");

   /* stop switches scanning */
   fr_process_add_arg (comm->process, "--"); 

   fr_process_add_arg (comm->process, comm->filename);

   for (scan = file_list; scan; scan = scan->next)
      fr_process_add_arg (comm->process, scan->data);

   if (dest_dir != NULL) 
      fr_process_add_arg (comm->process, dest_dir);

   fr_process_end_command (comm->process);
}


static void 
fr_command_rar_class_init (FRCommandRarClass *class)
{
   FRCommandClass *afc;
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass*) class;
   afc = (FRCommandClass*) class;

   gobject_class->dispose = fr_command_rar_dispose;

   afc->list         = fr_command_rar_list;
   afc->add          = fr_command_rar_add;
   afc->delete       = fr_command_rar_delete;
   afc->extract      = fr_command_rar_extract;
}

 
static void 
fr_command_rar_init (FRCommandRar *rar_comm)
{
   FRCommand *comm = FR_COMMAND(rar_comm);

   comm->propAddCanUpdate             = TRUE; 
   comm->propExtractCanAvoidOverwrite = TRUE;
   comm->propExtractCanSkipOlder      = TRUE;
   comm->propExtractCanJunkPaths      = TRUE;

   comm->propHasRatio                 = TRUE;
}


static void 
fr_command_rar_dispose (GObject *object)
{
   g_return_if_fail (object != NULL);
   g_return_if_fail (FR_IS_COMMAND_RAR (object));

   /* Chain up */
   if (G_OBJECT_CLASS (fr_command_rar_parent_class)->dispose)
      G_OBJECT_CLASS (fr_command_rar_parent_class)->dispose (object);
}


FRCommand *
fr_command_rar_new (FRProcess  *process,
                    const char *filename,
                    FRArchive  *archive)
{
   FRCommand *comm;

   comm = FR_COMMAND (g_object_new (FR_TYPE_COMMAND_RAR, NULL));
   fr_command_construct (comm, process, filename);
   fr_process_set_proc_line_func (FR_COMMAND (comm)->process, 
                                  process_line,
                                  comm);
   comm->archive = archive;

   return comm;
}
