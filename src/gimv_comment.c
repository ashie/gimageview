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

/*
 *  If use GTK+-2.0, internal character code for comment will be UTF-8.
 *  Else, it will be locale encode.
 *  System entry list will be ASCII only.  Do not use any other character set.
 */

#include "gimv_comment.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "gimv_image_info.h"
#include "gimv_mime_types.h"
#include "prefs.h"
#include "utils_char_code.h"
#include "utils_file.h"
#include "utils_gtk.h"

#define GIMV_COMMENT_DIRECTORY ".gimv/comment"

typedef enum {
   FILE_SAVED,
   FILE_DELETED,
   LAST_SIGNAL
} GimvCommentSignalType;


static void gimv_comment_dispose (GObject *object);

static gchar *defval_time                 (GimvImageInfo *info, gpointer data);
static gchar *defval_file_url             (GimvImageInfo *info, gpointer data);
static gchar *defval_file_path_in_archive (GimvImageInfo *info, gpointer data);
static gchar *defval_file_mtime           (GimvImageInfo *info, gpointer data);
static gchar *defval_img_type             (GimvImageInfo *info, gpointer data);
static gchar *defval_img_size             (GimvImageInfo *info, gpointer data);
#if 0
static gchar *defval_img_depth            (GimvImageInfo *info, gpointer data);
static gchar *defval_img_cspace           (GimvImageInfo *info, gpointer data);
#endif


static gint gimv_comment_signals[LAST_SIGNAL] = {0};


GimvCommentDataEntry gimv_comment_data_entry[] = {
   {"X-IMG-Subject",              N_("Subject"),           NULL, TRUE, FALSE, TRUE,  FALSE, NULL},
   {"X-IMG-Date",                 N_("Date"),              NULL, TRUE, FALSE, TRUE,  FALSE, NULL},
   {"X-IMG-Location",             N_("Location"),          NULL, TRUE, FALSE, TRUE,  FALSE, NULL},
   {"X-IMG-Model",                N_("Model"),             NULL, TRUE, FALSE, TRUE,  FALSE, NULL},
   {"X-IMG-Comment-Time",         N_("Comment Time"),      NULL, TRUE, TRUE,  FALSE, FALSE, defval_time},
   {"X-IMG-File-URL",             N_("URL"),               NULL, TRUE, TRUE,  TRUE,  FALSE, defval_file_url},
   {"X-IMG-File-Path-In-Archive", N_("Path in Archive"),   NULL, TRUE, TRUE,  TRUE,  FALSE, defval_file_path_in_archive},
   {"X-IMG-File-MTime",           N_("Modification Time"), NULL, TRUE, TRUE,  TRUE,  FALSE, defval_file_mtime},
   {"X-IMG-Image-Type",           N_("Image Type"),        NULL, TRUE, TRUE,  TRUE,  FALSE, defval_img_type},
   {"X-IMG-Image-Size",           N_("Image Size"),        NULL, TRUE, TRUE,  TRUE,  FALSE, defval_img_size},
   /*
   {"X-IMG-Image-Depth",          N_("Color Depth"),       NULL, TRUE, TRUE,  TRUE,  FALSE, defval_img_depth},
   {"X-IMG-Image-ColorSpace",     N_("Color Space"),       NULL, TRUE, TRUE,  TRUE,  FALSE, defval_img_cspace},
   */
   {NULL, NULL, NULL, FALSE, FALSE, FALSE,  FALSE, NULL},
};

GList *gimv_comment_data_entry_list = NULL;



/******************************************************************************
 *
 *   GimvComment class funcs
 *
 ******************************************************************************/
G_DEFINE_TYPE (GimvComment, gimv_comment, G_TYPE_OBJECT)


static void
gimv_comment_class_init (GimvCommentClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass *) klass;

   gimv_comment_signals[FILE_SAVED]
      = g_signal_new ("file_saved",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvCommentClass, file_saved),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_comment_signals[FILE_DELETED]
      = g_signal_new ("file_deleted",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvCommentClass, file_deleted),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gobject_class->dispose = gimv_comment_dispose;

   klass->file_saved    = NULL;
   klass->file_deleted  = NULL;
}


static void
gimv_comment_init (GimvComment *comment)
{
   comment->filename  = NULL;
   comment->info      = NULL;

   comment->data_list = NULL;

   comment->note      = NULL;
}


static void
gimv_comment_dispose (GObject *object)
{
   GimvComment *comment = GIMV_COMMENT (object);
   GList *node;

   g_return_if_fail (comment);
   g_return_if_fail (GIMV_IS_COMMENT (comment));

   if (comment->filename) {
      g_free (comment->filename);
      comment->filename = NULL;
   }

   if (comment->info) {
      gimv_image_info_set_comment (comment->info, NULL);
      gimv_image_info_unref (comment->info);
      comment->info = NULL;
   }

   node = comment->data_list;
   while (node) {
      GimvCommentDataEntry *entry = node->data;
      node = g_list_next (node);
      comment->data_list =  g_list_remove (comment->data_list, entry);
      gimv_comment_data_entry_delete (entry);
   }

   if (comment->note) {
      g_free (comment->note);
      comment->note = NULL;
   }

   if (G_OBJECT_CLASS (gimv_comment_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_comment_parent_class)->dispose (object);
}


/******************************************************************************
 *
 *   private functions.
 *
 ******************************************************************************/
static const gchar *
get_file_charset ()
{
   const gchar *lang;

   if (conf.comment_charset && *conf.comment_charset
       && g_strcasecmp (conf.comment_charset, "default"))
   {
      return conf.comment_charset;
   }

   lang = get_lang ();

   if (!lang || !*lang)
      return charset_get_internal ();

   /* get default charset for each language */
   if (!strncmp (lang, "ja", 2)) {   /* japanese */
      return CHARSET_JIS;
   } else {
      return charset_get_internal ();
   }
}


static gchar *
defval_time (GimvImageInfo *info, gpointer data)
{
   time_t current_time;
   struct tm tm_buf;
   gchar buf[256];

   time(&current_time);
   tm_buf = *localtime (&current_time);
   strftime(buf, 256, "%Y-%m-%d %H:%M:%S %Z", &tm_buf);

   return charset_locale_to_internal (buf);
}


static gchar *
defval_file_url (GimvImageInfo *info, gpointer data)
{
   const gchar *filename;

   g_return_val_if_fail (info, NULL);

   if (gimv_image_info_is_in_archive (info)) {
      filename = gimv_image_info_get_archive_path (info);
   } else {
      filename = gimv_image_info_get_path (info);
   }

   if (filename) {
      return charset_to_internal (filename, conf.charset_filename,
                                  conf.charset_auto_detect_fn,
                                  conf.comment_charset_read_mode);
   } else {
      return NULL;
   }
}


static gchar *
defval_file_path_in_archive (GimvImageInfo *info, gpointer data)
{
   const gchar *path;

   g_return_val_if_fail (info, NULL);
   if (!gimv_image_info_is_in_archive (info)) return NULL;

   path = gimv_image_info_get_path (info);

   if (path)
      return charset_to_internal (path, conf.charset_filename,
                                  conf.charset_auto_detect_fn,
                                  conf.comment_charset_read_mode);
   else
      return NULL;
}


static gchar *
defval_file_mtime (GimvImageInfo *info, gpointer data)
{
   struct tm tm_buf;
   gchar buf[256];

   g_return_val_if_fail (info, NULL);

   tm_buf = *localtime (&info->st.st_mtime);
   strftime(buf, 256, "%Y-%m-%d %H:%M:%S %Z", &tm_buf);

   return charset_locale_to_internal (buf);
}


static gchar *
defval_img_type (GimvImageInfo *info, gpointer data)
{
   const gchar *filename;
   const gchar *type = NULL;

   g_return_val_if_fail (info, NULL);

   filename = gimv_image_info_get_path (info);
   if (filename) {
      const gchar *ext = gimv_mime_types_get_extension (filename);
      type = gimv_mime_types_get_type_from_ext (ext);
   }

   if (type)
      return g_strdup (type); /* FIXME: need convert? */
   else
      return NULL;
}


static gchar *
defval_img_size (GimvImageInfo *info, gpointer data)
{
   gchar buf[256];

   g_return_val_if_fail (info, NULL);

   g_snprintf (buf, 256, "%dx%d", info->width, info->height);

   return g_strdup (buf);
}


#if 0
static gchar *
defval_img_depth (GimvImageInfo *info, gpointer data)
{
   g_return_val_if_fail (info, NULL);
   return NULL;
}


static gchar *
defval_img_cspace (GimvImageInfo *info, gpointer data)
{
   g_return_val_if_fail (info, NULL);
   return NULL;
}
#endif


static GimvComment *
gimv_comment_new ()
{
   GimvComment *comment;

   comment = GIMV_COMMENT (g_object_new (GIMV_TYPE_COMMENT, NULL));
   g_return_val_if_fail (comment, NULL);

   return comment;
}


static void
parse_comment_file (GimvComment *comment)
{
   FILE *file;
   gchar buf[BUF_SIZE];
   gchar **pair, *tmpstr;
   gint i;
   gboolean is_note = FALSE;
   gchar *key_internal, *value_internal = NULL;

   g_return_if_fail (comment);
   g_return_if_fail (comment->filename);

   if (!file_exists (comment->filename)) return;

   file = fopen (comment->filename, "r");
   if (!file) {
      g_warning (_("Can't open comment file for read."));
      return;
   }

   while (fgets (buf, sizeof(buf), file)) {
      gchar *key, *value = NULL;

      if (is_note) {
         if (comment->note) {
            tmpstr = g_strconcat (comment->note, buf, NULL);
            g_free (comment->note);
            comment->note = tmpstr;
         } else {
            comment->note = g_strdup (buf);
         }
         continue;
      }

      if (buf[0] == '\n') {
         is_note = TRUE;
         continue;
      }

      pair = g_strsplit (buf, ":", -1);
      if (!pair[0]) continue;
      if (!*pair[0]) goto ERROR;
      key = g_strdup (pair[0]);
      g_strstrip (key);

      if (pair[1]) {
         value = g_strdup (pair[1]);
         for (i = 2; pair[i]; i++) {   /* concat all value to one string */
            gchar *tmpstr = value;
            value = g_strconcat (value, ":", pair[i], NULL);
            g_free (tmpstr);
         }
         g_strstrip (value);
         if (!*value) {
            g_free (value);
            value = NULL;
         }
      } else {
         value = NULL;
      }

      key_internal = charset_to_internal (key, get_file_charset (),
                                          conf.charset_auto_detect_fn,
                                          conf.comment_charset_read_mode);
      if (value && *value) {
         value_internal = charset_to_internal (value, get_file_charset (),
                                               conf.charset_auto_detect_fn,
                                               conf.comment_charset_read_mode);
      } else {
         value_internal = NULL;
      }

      gimv_comment_append_data (comment, key_internal, value_internal);

      g_free (key_internal);
      g_free (value_internal);

      g_free (key);
      g_free (value);

   ERROR:
      g_strfreev (pair);
   }

   fclose (file);

   if (comment->note && *comment->note) {
      gchar *note_internal;

      note_internal = charset_to_internal (comment->note, get_file_charset (),
                                           conf.charset_auto_detect_fn,
                                           conf.comment_charset_read_mode);

      g_free (comment->note);
      comment->note = note_internal;
   }
}


static void
gimv_comment_set_default_value (GimvComment *comment)
{
   GList *list = gimv_comment_get_data_entry_list ();

   while (list) {
      GimvCommentDataEntry *template = list->data, *entry;

      list = g_list_next (list);

      if (!template) continue;

      entry = g_new0 (GimvCommentDataEntry, 1);

      *entry = *template;
      entry->key = g_strdup (template->key);
      entry->display_name = g_strdup (template->display_name);

      if (comment->info && template->def_val_fn) {
         entry->value = entry->def_val_fn (comment->info, NULL);
      } else {
         entry->value = NULL;
      }

      comment->data_list = g_list_append (comment->data_list, entry);
   }
}


/******************************************************************************
 *
 *   public functions.
 *
 ******************************************************************************/
gchar *
gimv_comment_get_path (const gchar *img_path)
{
   gchar buf[MAX_PATH_LEN];

   g_return_val_if_fail (img_path && *img_path, NULL);
   g_return_val_if_fail (img_path[0] == '/', NULL);

   g_snprintf (buf, MAX_PATH_LEN, "%s/%s%s" ".txt",
               g_getenv("HOME"), GIMV_COMMENT_DIRECTORY, img_path);

   return g_strdup (buf);
}


gchar *
gimv_comment_find_file (const gchar *img_path)
{
   gchar *path = gimv_comment_get_path (img_path);

   if (!path) return NULL;

   if (file_exists (path)) {
      return path;
   } else {
      g_free (path);
      return NULL;
   }
}


GimvCommentDataEntry *
gimv_comment_data_entry_find_template_by_key (const gchar *key)
{
   GList *list;

   g_return_val_if_fail (key && *key, NULL);

   list = gimv_comment_get_data_entry_list ();
   while (list) {
      GimvCommentDataEntry *template = list->data;

      list = g_list_next (list);

      if (!template) continue;

      if (template->key && *template->key && !strcmp (key, template->key))
         return template;
   }

   return NULL;
}


GimvCommentDataEntry *
gimv_comment_find_data_entry_by_key (GimvComment *comment, const gchar *key)
{
   GList *list;

   g_return_val_if_fail (comment, NULL);
   g_return_val_if_fail (key, NULL);

   list = comment->data_list;
   while (list) {
      GimvCommentDataEntry *entry = list->data;

      list = g_list_next (list);

      if (!entry) continue;

      if (entry->key && !strcmp (key, entry->key))
         return entry;
   }

   return NULL;
}


/*
 *  gimv_comment_append_data:
 *     @ Append a key & value pair.
 *     @ Before enter this function, character set must be converted to internal
 *       code.
 *
 *  comment :
 *  key     :
 *  value   :
 *  Return  :
 */
GimvCommentDataEntry *
gimv_comment_append_data (GimvComment *comment, const gchar *key, const gchar *value)
{
   GimvCommentDataEntry *entry;

   g_return_val_if_fail (comment, NULL);
   g_return_val_if_fail (key, NULL);

   entry = gimv_comment_find_data_entry_by_key (comment, key);

   if (!entry) {
      GimvCommentDataEntry *template;

      entry = g_new0 (GimvCommentDataEntry, 1);
      g_return_val_if_fail (entry, NULL);
      comment->data_list = g_list_append (comment->data_list, entry);

      /* find template */
      template = gimv_comment_data_entry_find_template_by_key (key);
      if (template)  {
         *entry = *template;
         entry->key = g_strdup (template->key);
         entry->display_name = g_strdup (template->display_name);
      } else {
         entry->key          = g_strdup (key);
         entry->display_name = g_strdup (key);
         entry->value        = NULL;
         entry->enable       = TRUE;
         entry->auto_val     = FALSE;
         entry->display      = TRUE;
         entry->def_val_fn   = NULL;
      }
   }

   /* free old value */
   if (entry->value) {
      g_free (entry->value);
      entry->value = NULL;
   }

   /* set new value */
   if (entry->auto_val && entry->def_val_fn) {
      entry->value = entry->def_val_fn (comment->info, NULL);
   } else if (value) {
      entry->value = g_strdup (value);
   } else {
      entry->value = NULL;
   }

   return entry;
}


/*
 *  gimv_comment_update_note:
 *     @ Apply "note" value.
 *     @ Before enter this function, character set must be converted to internal
 *       code.
 *
 *  comment :
 *  note    :
 *  Return  :
 */
gboolean
gimv_comment_update_note (GimvComment *comment, gchar *note)
{
   g_return_val_if_fail (comment, FALSE);

   if (comment->note)
      g_free (comment->note);

   comment->note = g_strdup (note);

   return TRUE;
}


void
gimv_comment_data_entry_remove (GimvComment *comment, GimvCommentDataEntry *entry)
{
   GList *list;

   g_return_if_fail (comment);
   g_return_if_fail (entry);

   list = g_list_find (comment->data_list, entry);
   if (list) {
      comment->data_list =  g_list_remove (comment->data_list, entry);
      gimv_comment_data_entry_delete (entry);
   }
}


void
gimv_comment_data_entry_remove_by_key (GimvComment *comment, const gchar *key)
{
   GList *list;

   g_return_if_fail (comment);
   g_return_if_fail (key && *key);

   list = comment->data_list;
   while (list) {
      GimvCommentDataEntry *entry = list->data;

      list = g_list_next (list);

      if (!entry) continue;

      if (entry->key && !strcmp (key, entry->key)) {
         comment->data_list =  g_list_remove (comment->data_list, entry);
         gimv_comment_data_entry_delete (entry);
      }
   }
}


GimvCommentDataEntry *
gimv_comment_data_entry_dup (GimvCommentDataEntry *src)
{
   GimvCommentDataEntry *dest;
   g_return_val_if_fail (src, NULL);

   dest = g_new0 (GimvCommentDataEntry, 1);
   *dest = *src;
   if (src->key)
      dest->key = g_strdup (src->key);
   if (src->display_name)
      dest->display_name = g_strdup (src->display_name);

   return dest;
}


void
gimv_comment_data_entry_delete (GimvCommentDataEntry *entry)
{
   g_return_if_fail (entry);

   g_free (entry->key);
   entry->key = NULL;
   g_free (entry->display_name);
   entry->display_name = NULL;
   if (entry->value)
      g_free (entry->value);
   entry->value = NULL;
   g_free (entry);
}


gboolean
gimv_comment_save_file (GimvComment *comment)
{
   GList *node;
   FILE *file;
   gboolean success;
   gint n;

   g_return_val_if_fail (comment, FALSE);
   g_return_val_if_fail (comment->filename, FALSE);

   g_print ("save comments to file: %s\n", comment->filename);

   success = mkdirs (comment->filename);
   if (!success) {
      g_warning (_("cannot make dir\n"));
      return FALSE;
   }

   file = fopen (comment->filename, "w");
   if (!file) {
      g_warning (_("Can't open comment file for write."));
      return FALSE;
   }

   node = comment->data_list;
   while (node) {
      GimvCommentDataEntry *entry = node->data;

      node = g_list_next (node);

      if (!entry || !entry->key) continue;

      {   /********** convert charset **********/
         gchar *tmpstr, buf[BUF_SIZE];

         if (entry->value && *entry->value)
            g_snprintf (buf, BUF_SIZE, "%s: %s\n", entry->key, entry->value);
         else
            g_snprintf (buf, BUF_SIZE, "%s:\n", entry->key);

         tmpstr = charset_from_internal (buf, get_file_charset ());

         n = fprintf (file, "%s", tmpstr);

         g_free (tmpstr);
      }

      if (n < 0) goto ERROR;
   }

   /********** convert charset **********/
   if (comment->note && *comment->note) {
      gchar *tmpstr;

      tmpstr = charset_from_internal (comment->note, get_file_charset ());

      n = fprintf (file, "\n%s", tmpstr);
      if (n < 0) goto ERROR;

      g_free (tmpstr);
   }

   fclose (file);

   g_signal_emit (G_OBJECT (comment), gimv_comment_signals[FILE_SAVED], 0);

   return TRUE;

ERROR:
   fclose (file);

   g_signal_emit (G_OBJECT (comment), gimv_comment_signals[FILE_SAVED], 0);
   return FALSE;
}


void
gimv_comment_delete_file (GimvComment *comment)
{
   g_return_if_fail (comment);
   g_return_if_fail (comment->filename);

   remove (comment->filename);
}


GimvComment *
gimv_comment_get_from_image_info (GimvImageInfo *info)
{
   GimvComment *comment;
   gchar *image_name;

   g_return_val_if_fail (info, NULL);

   comment = gimv_image_info_get_comment (info);
   if (comment) {
      g_object_ref (G_OBJECT (comment));
      return comment;
   }

   comment = gimv_comment_new ();
   comment->info = gimv_image_info_ref (info);
   gimv_image_info_set_comment (info, comment);

   /* get comment file path */
   image_name = gimv_image_info_get_path_with_archive (info);

   comment->filename = gimv_comment_get_path (image_name);

   /* get comment */
   if (file_exists (comment->filename)) {
      parse_comment_file (comment);
   } else {
      gimv_comment_set_default_value (comment);
   }

   g_free (image_name);

   return comment;
}


/******************************************************************************
 *
 *   Key list related functions
 *
 ******************************************************************************/
static void
gimv_comment_prefs_set_default_key_list ()
{
   gchar *string, *tmp;
   gint i;

   if (conf.comment_key_list) return;

   string = g_strdup ("");

   for (i = 0; gimv_comment_data_entry[i].key; i++) {
      GimvCommentDataEntry *entry = &gimv_comment_data_entry[i];

      tmp = string;
      if (*string) {
         string = g_strconcat (string, "; ", NULL);
         g_free (tmp);
         tmp = string;
      }
      string = g_strconcat (string, entry->key, ",", entry->display_name,
                            ",", boolean_to_text (entry->enable),
                            ",", boolean_to_text (entry->auto_val),
                            ",", boolean_to_text (entry->display),
                            NULL);
      g_free (tmp);
      tmp = NULL;
   }

   conf.comment_key_list = string;
}


static GList *
gimv_comment_get_system_data_entry_list (void)
{
   GList *list = NULL;
   gint i;

   for (i = 0; gimv_comment_data_entry[i].key; i++) {
      GimvCommentDataEntry *entry = g_new0 (GimvCommentDataEntry, 1);

      list = g_list_append (list, entry);

      *entry = gimv_comment_data_entry[i];
      entry->key          = g_strdup (gimv_comment_data_entry[i].key);
      entry->display_name = g_strdup (gimv_comment_data_entry[i].display_name);
      if (entry->value && *entry->value)
         entry->value = g_strdup (entry->value);
   }

   return list;
}


static void
free_data_entry_list (void)
{
   if (!gimv_comment_data_entry_list) return;

   g_list_foreach (gimv_comment_data_entry_list,
                   (GFunc) gimv_comment_data_entry_delete,
                   NULL);
   g_list_free (gimv_comment_data_entry_list);
   gimv_comment_data_entry_list = NULL;
}


static GimvCommentDataEntry *
gimv_comment_data_entry_new (gchar *key, gchar *name,
                             gboolean enable,
                             gboolean auto_val,
                             gboolean display)
{
   GimvCommentDataEntry *entry = NULL;
   gint i, pos = -1;

   g_return_val_if_fail (key && *key, NULL);
   g_return_val_if_fail (name && *name, NULL);

   for (i = 0; gimv_comment_data_entry[i].key; i++) {
      if (!strcmp (key, gimv_comment_data_entry[i].key)) {
         pos = i;
         break;
      }
   }

   entry = g_new0 (GimvCommentDataEntry, 1);
   if (!entry) return NULL;

   if (pos < 0) {
      entry->key          = g_strdup (key);
      entry->display_name = g_strdup (name);
      entry->value        = NULL;
      entry->def_val_fn   = NULL;
      entry->userdef      = TRUE;
   } else {
      *entry              = gimv_comment_data_entry[pos];
      entry->key          = g_strdup (gimv_comment_data_entry[pos].key);
      entry->display_name = g_strdup (gimv_comment_data_entry[pos].display_name);
      entry->userdef      = FALSE;
   }

   entry->enable = enable;
   if (entry->def_val_fn)
      entry->auto_val = auto_val;
   else
      entry->auto_val = FALSE;
   entry->display = display;

   return entry;
}


static gint
compare_entry_by_key (gconstpointer a, gconstpointer b)
{
   const GimvCommentDataEntry *entry = a;
   const gchar *string = b;

   g_return_val_if_fail (entry, TRUE);
   g_return_val_if_fail (string, TRUE);

   if (!entry->key || !*entry->key) return TRUE;
   if (!string || !*string) return TRUE;

   if (!strcmp (entry->key, string))
      return FALSE;

   return TRUE;
}


void
gimv_comment_update_data_entry_list ()
{
   GList *node = NULL, *list;
   gchar **sections, **values;
   gint i, j;

   if (!conf.comment_key_list)
      gimv_comment_prefs_set_default_key_list ();

   if (gimv_comment_data_entry_list)
      free_data_entry_list ();

   sections = g_strsplit (conf.comment_key_list, ";", -1);
   if (!sections) return;

   /* for undefined system entry in prefs */
   list = gimv_comment_get_system_data_entry_list ();

   for (i = 0; sections[i]; i++) {
      GimvCommentDataEntry *entry;
      gchar *key, *name;

      if (!*sections[i]) continue;

      values = g_strsplit (sections[i], ",", 5);
      if (!values) continue;

      /* strip space characters */
      for (j = 0; j < 5; j++) {
         if (values[j] && *values[j])
            g_strstrip (values[j]);
      }
      if (!*values[0] || !*values[1]) goto ERROR1;

      key  = values[0];
      name = charset_locale_to_internal (values[1]);

      /* check duplicate */
      node = NULL;
      node = g_list_find_custom (gimv_comment_data_entry_list,
                                 key, compare_entry_by_key);
      if (node) {
         gimv_comment_data_entry_delete (node->data);
         list = g_list_remove (list, node->data);
      }

      /* add entry */
      entry = gimv_comment_data_entry_new (key, name,
                                           text_to_boolean (values[2]),
                                           text_to_boolean (values[3]),
                                           text_to_boolean (values[4]));
      g_free (name);

      if (!entry) goto ERROR1;

      gimv_comment_data_entry_list
         = g_list_append (gimv_comment_data_entry_list, entry);

      /* for undefined system entry in prefs */
      if (!entry->userdef) {
         node = NULL;
         if (list)
            node = g_list_find_custom (list, entry->key, compare_entry_by_key);
         if (node) {   /* this entry is already defined in prefs */
            gimv_comment_data_entry_delete (node->data);
            list = g_list_remove (list, node->data);
         }
      }

   ERROR1:
      g_strfreev (values);
      values = NULL;
   }

   /* append system defined entry if it isn't exist in prefs */
   for (node = list; node; node = g_list_next (node)) {
      if (node && node->data)
         gimv_comment_data_entry_list
            = g_list_append (gimv_comment_data_entry_list, node->data);
   }

   if (list) g_list_free (list);
   g_strfreev (sections);
}


GList *
gimv_comment_get_data_entry_list ()
{
   if (!gimv_comment_data_entry_list)
      gimv_comment_update_data_entry_list ();

   return gimv_comment_data_entry_list;
}
