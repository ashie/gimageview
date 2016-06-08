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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gimageview.h"

#include "fr-archive.h"
#include "gimv_comment.h"
#include "gimv_anim.h"
#include "gimv_image.h"
#include "gimv_image_info.h"
#include "gimv_mime_types.h"
#include "gimv_thumb.h"
#include "gimv_thumb_cache.h"
#include "prefs.h"
#include "utils_file.h"
#include "utils_file_gtk.h"

G_DEFINE_TYPE (GimvImageInfo, gimv_image_info, G_TYPE_OBJECT);

static GHashTable *gimv_image_info_table = NULL;
static GHashTable *comment_table    = NULL;
static GHashTable *archive_table    = NULL;
static GHashTable *link_table       = NULL;


/******************************************************************************
 *
 *   Private Functions.
 *
 ******************************************************************************/
static void
gimv_image_info_class_init (GimvImageInfoClass *klass)
{
}


static void
gimv_image_info_init (GimvImageInfo *info)
{
}

static GimvImageInfo *
gimv_image_info_new (const gchar *filename)
{
   GimvImageInfo *info;

   info = g_new0 (GimvImageInfo, 1);
   g_return_val_if_fail (info, NULL);

   info->filename     = g_strdup (filename);
   info->format       = gimv_image_detect_type_by_ext (filename);
   info->width        = -1;
   info->height       = -1;
   info->depth        = -1;
   info->flags        = 0;

   info->ref_count = 1;

   return info;
}


/******************************************************************************
 *
 *   Public Functions.
 *
 ******************************************************************************/
GimvImageInfo *
gimv_image_info_get (const gchar *filename)
{
   GimvImageInfo *info;
   struct stat st;

   g_return_val_if_fail (filename, NULL);

   if (!gimv_image_info_table) {
      gimv_image_info_table = g_hash_table_new (g_str_hash, g_str_equal);
   }

   info = g_hash_table_lookup (gimv_image_info_table, filename);
   if (!info) {
      if (!file_exists (filename)) return NULL;
      info = gimv_image_info_new (filename);
      if (!info) return NULL;
      stat (filename, &info->st);
      g_hash_table_insert (gimv_image_info_table, info->filename, info);
   } else {
      if (stat (filename, &st)) return NULL;
      if (info->st.st_size != st.st_size
          || info->st.st_mtime != st.st_mtime
          || info->st.st_ctime != st.st_ctime)
      {
         info->st = st;
         info->width = -1;
         info->height = -1;
         info->flags &= ~GIMV_IMAGE_INFO_SYNCED_FLAG;
      }
      gimv_image_info_ref (info);
   }

   return info;
}


GimvImageInfo *
gimv_image_info_get_url (const gchar *url)
{
   GimvImageInfo *info;

   g_return_val_if_fail (url, NULL);

   if (!gimv_image_info_table) {
      gimv_image_info_table = g_hash_table_new (g_str_hash, g_str_equal);
   }

   info = g_hash_table_lookup (gimv_image_info_table, url);
   if (!info) {
      info = gimv_image_info_new (url);
      if (!info) return NULL;
      info->flags |= GIMV_IMAGE_INFO_URL_FLAG;
      g_hash_table_insert (gimv_image_info_table, info->filename, info);
   } else {
      gimv_image_info_ref (info);
   }

   return info;
}


GimvImageInfo *
gimv_image_info_get_with_archive (const gchar *filename,
                                  FRArchive   *archive,
                                  struct stat *st)
{
   GimvImageInfo *info;
   gchar buf[MAX_PATH_LEN];

   g_return_val_if_fail (filename, NULL);
   g_return_val_if_fail (archive, NULL);

   if (!gimv_image_info_table)
      gimv_image_info_table = g_hash_table_new (g_str_hash, g_str_equal);

   if (!archive_table)
      archive_table = g_hash_table_new (g_direct_hash, g_direct_equal);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s", archive->filename, filename);

   info = g_hash_table_lookup (gimv_image_info_table, buf);
   if (!info) {
      info = gimv_image_info_new (filename);
      g_hash_table_insert (gimv_image_info_table, g_strdup (buf), info);
      g_hash_table_insert (archive_table, info, archive);
   } else {
      /* FIXME!! update if needed */
      /* gimv_image_info_ref (info); */
   }

   if (st)
      info->st = *st;

   info->flags |= GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG;

   return info;
}


GimvImageInfo *
gimv_image_info_lookup (const gchar *filename)
{
   GimvImageInfo *info;

   info = g_hash_table_lookup (gimv_image_info_table, filename);

   if (info)
      gimv_image_info_ref (info);

   return info;
}


void
gimv_image_info_finalize (GimvImageInfo *info)
{
   guint num;
   gchar buf[MAX_PATH_LEN];

   g_return_if_fail (info);

   /* remove data from hash table */
   if (info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG) {
      gchar *orig_key;
      GimvImageInfo *value;
      gboolean success = FALSE;
      FRArchive *archive = g_hash_table_lookup (archive_table, info);

      if (archive) {
         g_snprintf (buf, MAX_PATH_LEN, "%s/%s",
                     archive->filename, info->filename);
         success = g_hash_table_lookup_extended (gimv_image_info_table, buf,
                                                 (gpointer) &orig_key,
                                                 (gpointer) &value);
         g_hash_table_remove (archive_table, info);
      }
      if (success) {
         g_hash_table_remove (gimv_image_info_table, buf);
         g_free (orig_key);
      }
   } else {
      g_hash_table_remove (gimv_image_info_table, info->filename);
   }

   gimv_image_info_set_comment (info, NULL);
   gimv_image_info_set_link (info, NULL);

   if (info->filename) {
      g_free (info->filename);
      info->filename = NULL;
   }
   g_free (info);

   /* destroy hash table if needed */
   num = g_hash_table_size (gimv_image_info_table);
   if (num < 1) {
      g_hash_table_destroy (gimv_image_info_table);
      gimv_image_info_table = NULL;
   }
}


GimvImageInfo *
gimv_image_info_ref (GimvImageInfo *info)
{
   g_return_val_if_fail (info, NULL);

   info->ref_count++;

   if (info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG) {
      FRArchive *archive = g_hash_table_lookup (archive_table, info);
      if (archive)
         g_object_ref (G_OBJECT (archive));
   }

   return info;
}


void
gimv_image_info_unref (GimvImageInfo *info)
{
   FRArchive *archive = NULL;

   g_return_if_fail (info);

   if (info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG)
      archive = g_hash_table_lookup (archive_table, info);

   info->ref_count--;

   if (info->ref_count < 1) {
      gimv_image_info_finalize (info);
   }

   if (archive)
      g_object_unref (G_OBJECT (archive));
}


/* used by fr-command */
void
gimv_image_info_unref_with_archive (GimvImageInfo *info)
{
   g_return_if_fail (info);

   info->ref_count--;
   /* info->archive = NULL; */

   if (info->ref_count < 1)
      gimv_image_info_finalize (info);
}


void
gimv_image_info_set_size (GimvImageInfo *info, gint width, gint height)
{
   g_return_if_fail (info);

   if (info->flags & GIMV_IMAGE_INFO_SYNCED_FLAG) return;

   info->width  = width;
   info->height = height;
}


void
gimv_image_info_set_comment (GimvImageInfo *info, GimvComment *comment)
{
   GimvComment *prev;

   g_return_if_fail (info);

   if (!comment_table)
      comment_table = g_hash_table_new (g_direct_hash, g_direct_equal);

   prev = g_hash_table_lookup (comment_table, info);
   if (prev) {
      g_hash_table_remove (comment_table, info);
      /* comment_unref (prev); */
   }

   if (comment) {
      /* comment_ref (comment); */
      g_hash_table_insert (comment_table, info, comment);
   }
}


GimvComment *
gimv_image_info_get_comment (GimvImageInfo *info)
{
   g_return_val_if_fail (info, NULL);

   if (!comment_table) return NULL;
   return g_hash_table_lookup (comment_table, info);
}


void
gimv_image_info_set_link (GimvImageInfo *info, const gchar *link)
{
   gchar *old_link;

   g_return_if_fail (info);

   if (!link_table)
      link_table = g_hash_table_new (g_direct_hash, g_direct_equal);

   old_link = g_hash_table_lookup (link_table, info);
   if (old_link) {
      g_hash_table_remove (link_table, info);
      g_free (old_link);
   }

   if (link && *link)
      g_hash_table_insert (link_table, info, g_strdup (link));
}


FRArchive *
gimv_image_info_get_archive (GimvImageInfo *info)
{
   g_return_val_if_fail (info, NULL);

   if (!archive_table) return NULL;
   return g_hash_table_lookup (archive_table, info);
}


void
gimv_image_info_set_flags (GimvImageInfo *info, GimvImageInfoFlags flags)
{
   g_return_if_fail (info);

   if (!(info->flags & GIMV_IMAGE_INFO_SYNCED_FLAG)
       || (flags & GIMV_IMAGE_INFO_SYNCED_FLAG))
   {
      info->flags |= flags;
   }
}


void
gimv_image_info_unset_flags (GimvImageInfo *info, GimvImageInfoFlags flags)
{
   g_return_if_fail (info);

   if (!(info->flags & GIMV_IMAGE_INFO_SYNCED_FLAG)
       || (flags & GIMV_IMAGE_INFO_SYNCED_FLAG))
   {
      info->flags &= ~flags;
   }
}


gboolean
gimv_image_info_is_dir (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);

   if (isdir (gimv_image_info_get_path (info)))
      return TRUE;

   return FALSE;
}


gboolean
gimv_image_info_is_archive (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);

   if (fr_archive_utils_get_file_name_ext (gimv_image_info_get_path (info)))
      return TRUE;

   return FALSE;
}


gboolean
gimv_image_info_is_in_archive (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);

   if (info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG)
      return TRUE;
   else
      return FALSE;
}


gboolean
gimv_image_info_is_url (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);

   if (info->flags & GIMV_IMAGE_INFO_URL_FLAG)
      return TRUE;
   else
      return FALSE;
}


gboolean
gimv_image_info_is_animation (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);  

   if (info->flags & GIMV_IMAGE_INFO_ANIMATION_FLAG)
      return TRUE;

   return FALSE;
}


gboolean
gimv_image_info_is_movie (GimvImageInfo *info)
{
   const gchar *ext;
   const gchar *type;

   g_return_val_if_fail (info, FALSE);

   if ((info->flags & GIMV_IMAGE_INFO_MOVIE_FLAG)
       || (info->flags & GIMV_IMAGE_INFO_MRL_FLAG))
   {
      return TRUE;
   }

   ext  = fileutil_get_extention (info->filename);
   type = gimv_mime_types_get_type_from_ext (ext);

   if (type 
       && (!g_ascii_strncasecmp (type, "video", 5))
       && g_ascii_strcasecmp (type, "video/x-mng")) /* FIXME!! */
   {
      return TRUE;
   }

   return FALSE;
}


gboolean
gimv_image_info_is_audio (GimvImageInfo *info)
{
   const gchar *ext;
   const gchar *type;

   g_return_val_if_fail (info, FALSE);

   ext  = gimv_mime_types_get_extension (info->filename);
   type = gimv_mime_types_get_type_from_ext (ext);

   if (type && !g_ascii_strncasecmp (type, "audio", 5))
      return TRUE;

   return FALSE;
}


gboolean
gimv_image_info_is_same (GimvImageInfo *info1, GimvImageInfo *info2)
{
   FRArchive *arc1 = NULL, *arc2 = NULL;

   g_return_val_if_fail (info1, FALSE);
   g_return_val_if_fail (info2, FALSE);
   g_return_val_if_fail (info1->filename && *info1->filename, FALSE);
   g_return_val_if_fail (info2->filename && *info2->filename, FALSE);

   if (strcmp (info1->filename, info2->filename))
      return FALSE;

   if (!(info1->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG)
       && !(info2->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG))
   {
      return TRUE;
   }

   if (archive_table) {
      arc1 = g_hash_table_lookup (archive_table, info1);
      arc2 = g_hash_table_lookup (archive_table, info2);
   }

   if (arc1 && arc1->filename && *arc1->filename
       && arc2 && arc2->filename && *arc2->filename
       && !strcmp (arc1->filename, arc2->filename))
   {
      return TRUE;
   }

   return FALSE;
}


const gchar *
gimv_image_info_get_path (GimvImageInfo *info)
{
   g_return_val_if_fail (info, NULL);

   return info->filename;
}


gchar *
gimv_image_info_get_path_with_archive (GimvImageInfo *info)
{
   FRArchive *arc;

   g_return_val_if_fail (info, NULL);

   if (!(info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG))
      return g_strdup (info->filename);

   arc = g_hash_table_lookup (archive_table, info);
   g_return_val_if_fail (arc, NULL);

   return g_strconcat (arc->filename, "/", info->filename, NULL);
}


const gchar *
gimv_image_info_get_archive_path (GimvImageInfo *info)
{
   FRArchive *arc;

   g_return_val_if_fail (info, NULL);
   g_return_val_if_fail (info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG,
                         NULL);

   arc = g_hash_table_lookup (archive_table, info);

   return arc->filename;
}


gboolean
gimv_image_info_need_temp_file (GimvImageInfo *info)
{
   g_return_val_if_fail (info, FALSE);

   if (gimv_image_info_is_in_archive (info))
      return TRUE;
   else
      return FALSE;
}


gchar *
gimv_image_info_get_temp_file_path (GimvImageInfo *info)
{
   FRArchive *archive;
   gchar *temp_dir;
   gchar *filename, buf[MAX_PATH_LEN];

   g_return_val_if_fail (info, NULL);
   g_return_val_if_fail (archive_table, FALSE);

   archive = g_hash_table_lookup (archive_table, info);
   g_return_val_if_fail (archive, NULL);

   filename = info->filename;

   temp_dir = g_object_get_data (G_OBJECT (archive), "temp-dir");

   g_return_val_if_fail (temp_dir && *temp_dir, NULL);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s",
               temp_dir, filename);

   return g_strdup (buf);
}


gchar *
gimv_image_info_get_temp_file (GimvImageInfo *info)
{
   gboolean success;
   gchar *filename;

   /* load the image */
   if (!gimv_image_info_need_temp_file (info))
      return NULL;

   filename = gimv_image_info_get_temp_file_path (info);
   g_return_val_if_fail (filename, NULL);

   if (file_exists (filename)) {
      return filename;
   }

   success = gimv_image_info_extract_archive (info);
   if (success)
      return filename;

   g_free (filename);
   return NULL;
}


/*
 *  FIXME!!
 *  If archive was already destroyed, create FRArchive object first.
 *  To detect whether archive was destroyed or not, check info->archive.
 *  When archive is destroyed, fr-archive class will set this value to NULL
 *  by using gimv_image_info_unref_with_archive ().
 */
gboolean
gimv_image_info_extract_archive (GimvImageInfo *info)
{
   FRArchive *archive;
   GList *filelist = NULL;
   gchar *temp_dir;
   gchar *filename, *temp_file, buf[MAX_PATH_LEN];

   g_return_val_if_fail (info, FALSE);
   g_return_val_if_fail (archive_table, FALSE);

   archive = g_hash_table_lookup (archive_table, info);
   g_return_val_if_fail (archive, FALSE);

   filename = info->filename;

   temp_dir = g_object_get_data (G_OBJECT (archive), "temp-dir");

   g_return_val_if_fail (temp_dir && *temp_dir, FALSE);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s",
               temp_dir, filename);
   temp_file = buf;

   if (!file_exists (temp_file) && !archive->process->running) {
      ensure_dir_exists (temp_dir);
      filelist = g_list_append (filelist, filename);

      fr_archive_extract (archive, filelist, temp_dir,
                          FALSE, TRUE, FALSE);

      gtk_main ();   /* will be quited by callback function
                        of archive (see fileload.c) */
   }

   if (file_exists (temp_file)) {
      return TRUE;
   } else {
      return FALSE;
   }
}


GimvIO *
gimv_image_info_get_gio (GimvImageInfo *info)
{
   gboolean need_temp;
   const gchar *filename;
   gchar *temp_file = NULL;
   GimvIO *gio = NULL;

   g_return_val_if_fail (info, NULL);

   need_temp = gimv_image_info_need_temp_file (info);
   if (need_temp) {
      temp_file = gimv_image_info_get_temp_file (info);
      filename = temp_file;
   } else {
      filename = gimv_image_info_get_path (info);
   }

   if (filename)
      gio = gimv_io_new (filename, "rb");

   g_free (temp_file);

   return gio;
}


const gchar *
gimv_image_info_get_format (GimvImageInfo *info)
{
   g_return_val_if_fail (info, NULL);

   return info->format;
}


void
gimv_image_info_get_image_size (GimvImageInfo *info,
                           gint *width_ret, gint *height_ret)
{
   g_return_if_fail (width_ret && height_ret);
   *width_ret = -1;
   *height_ret = -1;

   g_return_if_fail (info);

   *width_ret  = info->width;
   *height_ret = info->height;
}


/* FIXME */
gboolean
gimv_image_info_rename_image (GimvImageInfo *info, const gchar *filename)
{
   struct stat st;
   gboolean success;
   gchar *cache_type;
   gchar *src_cache_path, *dest_cache_path;
   gchar *src_comment, *dest_comment;

   g_return_val_if_fail (info, FALSE);
   g_return_val_if_fail (info->filename, FALSE);
   g_return_val_if_fail (!(info->flags & GIMV_IMAGE_INFO_ARCHIVE_MEMBER_FLAG),
                         FALSE);
   g_return_val_if_fail (filename, FALSE);

   /* rename the file */
   success = !rename(info->filename, filename);
   if (!success) return FALSE;

   /* rename cache */
   src_cache_path = gimv_thumb_find_thumbcache (gimv_image_info_get_path (info),
                                                &cache_type);
   if (src_cache_path) {
      dest_cache_path
         = gimv_thumb_cache_get_path (filename, cache_type);
      if (rename (src_cache_path, dest_cache_path) < 0)
         g_print (_("Faild to rename cache file :%s\n"), filename);
      g_free (src_cache_path);
      g_free (dest_cache_path);
   }

   /* rename comment */
   src_comment = gimv_comment_find_file (gimv_image_info_get_path (info));
   if (src_comment) {
      dest_comment = gimv_comment_get_path (filename);
      if (rename (src_comment, dest_comment) < 0)
         g_print (_("Faild to rename comment file :%s\n"), dest_comment);
      g_free (src_comment);
      g_free (dest_comment);      
   }

   if (stat (filename, &st)) return FALSE;
   info->st = st;
   g_free (info->filename);
   info->filename = (gchar *) g_strdup (filename);

   return success;
}
