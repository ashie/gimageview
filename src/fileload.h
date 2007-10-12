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

#ifndef __FILE_H__
#define __FILE_H__

#include "gimageview.h"
#include "fr-archive.h"

typedef enum
{
   LOAD_CACHE,
   CREATE_THUMB
} ThumbLoadType;


typedef enum
{
   SCAN_SUB_DIR_NONE,
   SCAN_SUB_DIR,
   SCAN_SUB_DIR_ONE_TAB
} ScanSubDirType;


typedef enum
{
   NO_OPEN_FILE,
   END,
   GETTING_FILELIST,
   IMAGE_LOADING,
   IMAGE_LOAD_DONE,
   THUMB_LOADING,
   THUMB_LOAD_DONE,
   CANCEL              = 30,
   STOP                = 31,
   CONTAINER_DESTROYED = -1,
   WINDOW_DESTROYED    = -2
} LoadStatus;


struct FilesLoader_Tag
{
   GList        *filelist;
   GList        *dirlist;
   const gchar  *dirname;
   FRArchive    *archive;

   GtkWidget    *window;      /* window which containes progress bar */
   GtkWidget    *progressbar;

   /* for thumbnail */
   ThumbLoadType thumb_load_type;

   /* cancel */
   LoadStatus    status;

   /* progress infomation */
   const gchar  *now_file;
   gint          pos;
   gint          num;
};


FilesLoader *files_loader_new                     (void);
gboolean     files_loader_query_loading           (void);
void         files_loader_delete                  (FilesLoader   *files);
void         files_loader_stop                    (void);
void         files_loader_create_progress_window  (FilesLoader   *files);
void         files_loader_destroy_progress_window (FilesLoader   *files);
void         files_loader_progress_update         (FilesLoader   *files,
                                                   gfloat         progress,
                                                   const gchar   *text);

gint         open_image_files_in_image_view     (FilesLoader     *files);
gint         open_image_files_in_thumbnail_view (FilesLoader     *files,
                                                 GimvThumbWin    *tw,
                                                 GimvThumbView   *tv);
gint         open_image_files                   (FilesLoader     *files);
gint         open_dir_images                    (const gchar     *dirname,
                                                 GimvThumbWin    *tw,
                                                 GimvThumbView   *tv,
                                                 ThumbLoadType    type,
                                                 ScanSubDirType   scan_subdir);
gint         open_archive_images                (const gchar     *filename,
                                                 GimvThumbWin    *tw,
                                                 GimvThumbView   *tv,
                                                 ThumbLoadType    type);
gint         open_dirs                          (FilesLoader     *files,
                                                 GimvThumbWin    *tw,
                                                 ThumbLoadType    type,
                                                 gboolean         scan_subdir);
gint         open_images_dirs                   (GList           *list,
                                                 GimvThumbWin    *tw,
                                                 ThumbLoadType    type,
                                                 gboolean         scan_subdir);

GtkWidget   *create_filebrowser                 (gpointer         parent);

#ifdef GIMV_DEBUG
void file_disp_loading_status (FilesLoader *files);
#endif /* GIMV_DEBUG */

#endif /* __FILE_H__ */
