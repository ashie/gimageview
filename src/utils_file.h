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

#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

#include <stdlib.h>
#include <sys/types.h>

typedef enum
{
   SIZE_BYTE,
   SIZE_KBYTE,
   SIZE_MBYTE,
   SIZE_GBYTE,
   SIZE_BIT,
   SIZE_KBIT,
   SIZE_MBIT,
   SIZE_GBIT
} SIZE_UNIT;

int file_exists  (const char *path);
int isfile       (const char *path);
int isdir        (const char *path);
int islink       (const char *path);
int iswritable   (const char *path);
int isexecutable (const char *path);

int has_subdirs  (const char *path);
int makedir      (const char *dir);
int mkdirs       (const char *path);

int ensure_dir_exists    (const char *a_path);
int get_file_mtime       (const char *path);

char *fileutil_size2str  (size_t      size,
                          int         space);
char *fileutil_time2str  (time_t      time);
char *fileutil_uid2str   (uid_t       uid);
char *fileutil_gid2str   (gid_t       gid);
char *fileutil_mode2str  (mode_t      mode);

char *fileutil_home2tilde    (const char *path);
char *fileutil_dir_basename  (const char *path);
char *fileutil_get_extention (const char *path);
int   fileutil_extension_is  (const char *filename,
                              const char *ext);

#endif /* __FILEUTIL_H__ */
