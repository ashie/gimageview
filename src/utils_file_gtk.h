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

#ifndef __GFILEUTIL_H__
#define __GFILEUTIL_H__

#include <gtk/gtk.h>

#include "utils_gtk.h"


typedef enum
{
   GETDIR_READ_DOT       = 1 << 0,
   GETDIR_FOLLOW_SYMLINK = 1 << 1,
   GETDIR_DETECT_EXT     = 1 << 2,
   GETDIR_GET_ARCHIVE    = 1 << 3,  /* use with GETDIR_DETECT_EXT */
   GETDIR_DISP_STDOUT    = 1 << 4,
   GETDIR_DISP_STDERR    = 1 << 5,
   GETDIR_ENABLE_CANCEL  = 1 << 6,
   GETDIR_RECURSIVE      = 1 << 7,
   GETDIR_RECURSIVE_IS_BRANCH = 1 << 8 /* for internal use */
} GetDirFlags;


typedef enum
{
   FILE_COPY,
   FILE_MOVE,
   FILE_LINK,
   FILE_HLINK,
   FILE_REMOVE,
   FILE_RENAME
} FileOperateType;


gchar    *add_slash             (const gchar     *path);
gchar    *remove_slash          (const gchar     *path);

gchar    *get_temp_dir_name     (void);
void      remove_temp_dir       (void);
void      get_dir               (const gchar     *dirname,
                                 GetDirFlags      flags,
                                 GList          **filelist,
                                 GList          **dirlist);
void      get_dir_stop          (void);
GList    *get_dir_all           (const gchar     *dirname);
gchar    *relpath2abs           (const gchar     *path);
gchar    *link2abs              (const gchar     *path);


void      remove_dir            (const char    *dirname);
gboolean  move_file             (const gchar   *from_path,
                                 const gchar   *dir,
                                 ConfirmType   *action,
                                 gboolean       show_error,
                                 GtkWindow     *window);
gboolean  copy_file_to_file     (const gchar   *from_path,
                                 const gchar   *to_path,
                                 ConfirmType   *action,
                                 gboolean       show_error,
                                 GtkWindow     *window);
gboolean  copy_file             (const gchar   *from_path,
                                 const gchar   *dir,
                                 ConfirmType   *action,
                                 gboolean       show_error,
                                 GtkWindow     *window);
gboolean  link_file             (const gchar   *from_path,
                                 const gchar   *dir,
                                 gboolean       show_error,
                                 GtkWindow     *window);
gboolean  files2dir             (GList         *filelist,
                                 const gchar   *dir,
                                 FileOperateType type,
                                 GtkWindow     *window);
gboolean  files2dir_with_dialog (GList          *filelist,
                                 gchar         **dir,
                                 FileOperateType action,
                                 GtkWindow      *window);
gboolean  delete_dir            (const gchar    *path,
                                 GtkWindow      *window);
gboolean  delete_files          (GList          *filelist,
                                 ConfirmType     confirm,
                                 GtkWindow      *window);

gboolean  make_dir_dialog       (const gchar    *parent_dir,
                                 GtkWindow      *window);
gboolean  rename_dir_dialog     (const gchar    *dir,
                                 GtkWindow      *window);
#endif
