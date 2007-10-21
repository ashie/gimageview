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

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "gimageview.h"

#include <glib.h>
#include "fr-process.h"
#include "fr-command.h"

#define FR_TYPE_ARCHIVE            (fr_archive_get_type ())
#define FR_ARCHIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_ARCHIVE, FRArchive))
#define FR_ARCHIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_ARCHIVE, FRArchiveClass))
#define FR_IS_ARCHIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_ARCHIVE))
#define FR_IS_ARCHIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_ARCHIVE))
#define FR_ARCHIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_ARCHIVE, FRArchiveClass))

typedef struct _FRArchiveClass  FRArchiveClass;

struct _FRArchive {
   GObject __parent;

   char *           filename;       /* The archive filename. */
   FRCommand *      command;
   FRProcess *      process;
   gboolean         is_compressed;
   gboolean         read_only;
};

struct _FRArchiveClass {
   GObjectClass __parent_class;

   /* -- Signals -- */
   void (*start) (FRArchive *archive,
                  FRAction   action); 

   void (*done)  (FRArchive   *archive,
                  FRAction     action, 
                  FRProcError  error);
};

#define GIMV_ARCHIVER_IF_VERSION 1

typedef FRCommand *(*ExtArchiverNewFn) (FRProcess  *process,
                                        const char *filename,
                                        FRArchive  *archive);

typedef struct ExtArchiverPlugin_Tag
{
   const guint32          if_version; /* plugin interface version */
   gchar                 *format;
   ExtArchiverNewFn       archiver_new;
   gboolean               is_compressed;
} ExtArchiverPlugin;

GType        fr_archive_get_type              (void);
FRArchive *  fr_archive_new                   (void);
void         fr_archive_free                  (FRArchive *archive);
void         fr_archive_new_file              (FRArchive *archive,
                                               char *filename);
gboolean     fr_archive_load                  (FRArchive *archive,
                                               const char *filename);
void         fr_archive_reload                (FRArchive *archive);
void         fr_archive_rename                (FRArchive *archive,
                                               char *filename);
void         fr_archive_add                   (FRArchive *archive, 
                                               GList * file_list,
                                               gchar *base_dir,
                                               gboolean update);

#if 0
void         fr_archive_add_with_wildcard     (FRArchive *archive, 
                                               const char *files,
                                               gchar *base_dir,
                                               gboolean update,
                                               gboolean recursive,
                                               gboolean follow_links,
                                               gboolean same_fs,
                                               gboolean no_backup_files,
                                               gboolean no_dot_files,
                                               gboolean ignore_case);
void         fr_archive_add_directory         (FRArchive *archive, 
                                               const char *directory,
                                               gchar *base_dir,
                                               gboolean update);
#endif

void         fr_archive_remove                (FRArchive *archive,
                                               GList * file_list);
void         fr_archive_extract               (FRArchive *archive,
                                               GList * file_list,
                                               char * dest_dir,
                                               gboolean skip_older,
                                               gboolean overwrite,
                                               gboolean junk_path);

/* utils */

gchar *      fr_archive_utils_get_file_name_ext (const char  *filename);

/* for plugin loader */

gboolean     fr_archive_plugin_regist         (const gchar *plugin_name,
                                               const gchar *module_name,
                                               gpointer     impl,
                                               gint         size);

#endif /* ARCHIVE_H */
