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

#ifndef __GIMV_IMAGE_INFO_H__
#define __GIMV_IMAGE_INFO_H__

#include <sys/stat.h>
#include <unistd.h>

#include "gimageview.h"
#include "gimv_io.h"


typedef enum
{
   GIMV_IMAGE_INFO_SYNCED_FLAG               = 1 << 0,
   GIMV_IMAGE_INFO_DIR_FLAG                  = 1 << 1,
   GIMV_IMAGE_INFO_ARCHIVE_FLAG              = 1 << 2,
   GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG       = 1 << 3,
   GIMV_IMAGE_INFO_ANIMATION_FLAG            = 1 << 5,
   GIMV_IMAGE_INFO_VECTOR_FLAG               = 1 << 6,
   GIMV_IMAGE_INFO_MOVIE_FLAG                = 1 << 7,
   GIMV_IMAGE_INFO_URL_FLAG                  = 1 << 8,
   GIMV_IMAGE_INFO_MRL_FLAG                  = 1 << 9    /* used by xine */
} GimvImageInfoFlags;


struct GimvImageInfo_Tag
{
   gchar          *filename;   /* If the file is a member of a archive,
                                  this value sholud be set as relative path. */
   const gchar    *format;
   struct stat     st;

   gint            width;      /* if unknown, value is -1  */
   gint            height;     /* same with above */
   gint            depth;
   /* ColorSpace   color_space; */

   GimvImageInfoFlags  flags;

   /* reference count */
   guint           ref_count;
};


#define GIMV_IMAGE_INFO_IS_SYNCED(info) \
   (info ? (info->flags & GIMV_IMAGE_INFO_SYNCED_FLAG) : FALSE)


/*
 *  Get GimvImageInfo object for specified file. If file is not exist,
 *  allocate new GimvImageInfo.
 *  To get GimvImageInfo for archive member, simply treat archive as directory.
 */
GimvImageInfo  *gimv_image_info_get                  (const gchar   *filename); 
GimvImageInfo  *gimv_image_info_get_url              (const gchar   *url);

/*
 *  Same with gimv_image_info_get, but never allocate new GimvImageInfo.
 */
GimvImageInfo  *gimv_image_info_lookup               (const gchar   *filename);

/* setter/getter */
void            gimv_image_info_set_size             (GimvImageInfo *info,
                                                      gint           width,
                                                      gint           height);
void            gimv_image_info_set_comment          (GimvImageInfo *info,
                                                      GimvComment   *comment);
GimvComment    *gimv_image_info_get_comment          (GimvImageInfo *info);
void            gimv_image_info_set_link             (GimvImageInfo *info,
                                                      const gchar   *link);
FRArchive      *gimv_image_info_get_archive          (GimvImageInfo *info);
void            gimv_image_info_set_flags            (GimvImageInfo *info,
                                                      GimvImageInfoFlags flags);
void            gimv_image_info_unset_flags          (GimvImageInfo *info,
                                                      GimvImageInfoFlags flags);

/*  If the object is archive member, return relative path in archive. */
const gchar    *gimv_image_info_get_path             (GimvImageInfo *info);

/*  If the object is archive member, add prefix (archive path). */
gchar          *gimv_image_info_get_path_with_archive(GimvImageInfo *info);

/*  If the object is archive member, return the archive path. */
const gchar    *gimv_image_info_get_archive_path     (GimvImageInfo *info);

gboolean        gimv_image_info_is_dir               (GimvImageInfo *info);
gboolean        gimv_image_info_is_archive           (GimvImageInfo *info);
gboolean        gimv_image_info_is_in_archive        (GimvImageInfo *info);
gboolean        gimv_image_info_is_url               (GimvImageInfo *info);

gboolean        gimv_image_info_is_animation         (GimvImageInfo *info);
gboolean        gimv_image_info_is_movie             (GimvImageInfo *info);
gboolean        gimv_image_info_is_audio             (GimvImageInfo *info);
gboolean        gimv_image_info_is_same              (GimvImageInfo *info1,
                                                      GimvImageInfo *info2);

gboolean        gimv_image_info_extract_archive      (GimvImageInfo *info);
const gchar    *gimv_image_info_get_format           (GimvImageInfo *info);
void            gimv_image_info_get_image_size       (GimvImageInfo *info,
                                                      gint          *width_ret,
                                                      gint          *height_ret);

gboolean        gimv_image_info_rename_image         (GimvImageInfo *info,
                                                      const gchar   *filename);
gboolean        gimv_image_info_need_temp_file       (GimvImageInfo *info);
gchar          *gimv_image_info_get_temp_file_path   (GimvImageInfo *info);
gchar          *gimv_image_info_get_temp_file        (GimvImageInfo *info);
GimvIO         *gimv_image_info_get_gio              (GimvImageInfo *info);

/*
 *  memory management
 */
void            gimv_image_info_finalize             (GimvImageInfo *info);
GimvImageInfo  *gimv_image_info_ref                  (GimvImageInfo *info);
void            gimv_image_info_unref                (GimvImageInfo *info);


/*
 *  used by fr-command only
 */
GimvImageInfo  *gimv_image_info_get_with_archive     (const gchar   *filename,
                                                      FRArchive     *archive,
                                                      struct stat   *st);
void            gimv_image_info_unref_with_archive   (GimvImageInfo *info);

#endif /* __GIMV_IMAGE_INFO_H__ */
