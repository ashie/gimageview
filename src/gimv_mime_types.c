/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2002 Takuro Ashie
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

#include "gimv_mime_types.h"
#include "prefs.h"

#include <string.h>

#define MAX_EXT_LEN 32

static GHashTable *mime_types_table  = NULL;

/* global extensions list */
static GList      *known_ext_list             = NULL;
static GList      *known_enabled_ext_list     = NULL;
static GHashTable *known_ext_mimetype_table   = NULL;

static GList      *userdef_ext_list           = NULL;
static GHashTable *userdef_ext_mimetype_table = NULL;


static GimvMimeType *
gimv_mime_type_new_from_entry (GimvMimeTypeEntry *entry)
{
   GimvMimeType *mtype;

   g_return_val_if_fail (entry, FALSE);

   mtype = g_new0 (GimvMimeType, 1);
   mtype->mime_type = g_strdup (entry->mime_type);
   mtype->extensions_list = NULL;
   mtype->plugins_list    = NULL;
   mtype->icons_list      = NULL;

   /* FIXME!! add icon */

   return mtype;
}


gboolean
gimv_mime_types_regist (GimvMimeTypeEntry *entry,
                        GModule           *module)
{
   GimvMimeType *mtype;
   gint i;

   g_return_val_if_fail (entry, FALSE);

   if (!mime_types_table)
      mime_types_table = g_hash_table_new (g_str_hash, g_str_equal);

   mtype = g_hash_table_lookup (mime_types_table, entry->mime_type);
   if (!mtype) {
      mtype = gimv_mime_type_new_from_entry (entry);
      mtype->plugins_list = g_list_append (mtype->plugins_list, module);
      g_hash_table_insert (mime_types_table,
                           (gchar *) mtype->mime_type, mtype);
   }

   for (i = 0; i < entry->extensions_len; i++) {
      gimv_mime_types_add_extension (entry->mime_type,
                                     entry->extensions[i]);
   }

   return TRUE;
}


/* FIXME: use hash? */
static gboolean
sysdef_check_enable (const gchar *ext)
{
   static gchar *old_conf = NULL, **sysdef_disables = NULL;
   gint i;

   /* create disabled list */
   if ((old_conf != conf.imgtype_disables)
       && conf.imgtype_disables && *conf.imgtype_disables)
   {
      if (sysdef_disables)
         g_strfreev (sysdef_disables);
      sysdef_disables = g_strsplit (conf.imgtype_disables, ",", -1);
      if (sysdef_disables)
         for (i = 0; sysdef_disables[i]; i++)
            if (*sysdef_disables[i]) {
               g_strstrip (sysdef_disables[i]);
               g_ascii_strdown (sysdef_disables[i], -1);
            }
      old_conf = conf.imgtype_disables;
   }

   /* check */
   for (i = 0; sysdef_disables && sysdef_disables[i]; i++) {
      if (sysdef_disables[i] && *sysdef_disables[i]
          && !strcmp (ext, sysdef_disables[i]))
      {
         return FALSE;
      }
   }

   return TRUE;
}


gboolean
gimv_mime_types_add_extension (const gchar *mime_type,
                               const gchar *extension)
{
   GimvMimeType *mtype = g_hash_table_lookup (mime_types_table, mime_type);
   gchar ext[MAX_EXT_LEN];
   gint len;

   g_return_val_if_fail (mtype, FALSE);

   if (!extension || !*extension) return FALSE;

   len = strlen (extension);
   g_return_val_if_fail (len < MAX_EXT_LEN, FALSE);

   strcpy(ext, extension);
   g_ascii_strdown (ext, -1);

   if (!g_list_find_custom (mtype->extensions_list,
                            (gpointer) ext,
                            (GCompareFunc) g_ascii_strcasecmp))
   {
      const gchar *known_mimetype;

      mtype->extensions_list = g_list_append (mtype->extensions_list,
                                              g_strdup (ext));

      /* add to known exteions list */
      /* FIXME */
      if (!known_ext_mimetype_table)
         known_ext_mimetype_table = g_hash_table_new (g_str_hash, g_str_equal);
      known_mimetype = g_hash_table_lookup (known_ext_mimetype_table, ext);
      if (!known_mimetype) {
         gchar *str = g_strdup (ext);

         g_hash_table_insert (known_ext_mimetype_table,
                              str, g_strdup (mime_type));

         known_ext_list = g_list_append (known_ext_list, str);
         g_list_sort (known_ext_list, (GCompareFunc) strcmp);

         if (sysdef_check_enable (ext)) {
            known_enabled_ext_list = g_list_append (known_enabled_ext_list,
                                                    str);
            g_list_sort (known_enabled_ext_list, (GCompareFunc) strcmp);
         }
      }
      return TRUE;
   }

   return FALSE;
}


void
gimv_mime_types_update_known_enabled_ext_list (void)
{
   GList *node;

   g_list_free (known_enabled_ext_list);
   known_enabled_ext_list = NULL;

   for (node = known_ext_list; node; node = g_list_next (node)) {
      if (sysdef_check_enable (node->data)) {
         known_enabled_ext_list = g_list_append (known_enabled_ext_list,
                                                 node->data);
      }
   }
}


void
gimv_mime_types_update_userdef_ext_list (void)
{
   gchar **user_defs = NULL, **sections = NULL;
   gchar *ext, *type;
   gint i;
   GList *node;

   /* free old value */
   for (node = userdef_ext_list; node; node = g_list_next (node)) {
      gboolean found = FALSE;
      gchar *key, *value;
      if (node->data)
         found = g_hash_table_lookup_extended (userdef_ext_mimetype_table,
                                               node->data,
                                               (gpointer) &key,
                                               (gpointer) &value);
      if (found) {
         g_hash_table_remove (userdef_ext_mimetype_table, key);
         g_free (key);
         g_free (value);
      }
   }
   g_list_free (userdef_ext_list);
   userdef_ext_list = NULL;

   /* create new list */
   if (conf.imgtype_user_defs && *conf.imgtype_user_defs)
      user_defs = g_strsplit (conf.imgtype_user_defs, ";", -1);
   if (!user_defs) return;

   if (!userdef_ext_mimetype_table)
      userdef_ext_mimetype_table = g_hash_table_new (g_str_hash, g_str_equal);

   for (i = 0; user_defs[i]; i++) {
      if (!*user_defs[i]) continue;

      sections = g_strsplit (user_defs[i], ",", 3);
      if (!sections) continue;
      g_strstrip (sections[0]);	 
      g_strstrip (sections[1]);	 
      g_strstrip (sections[2]);	 
      if (!*sections[0]) goto ERROR;
      /* if (filter_check_duplicate (sections[0], -1, FALSE)) goto ERROR; */

      if (!*sections[2] || g_ascii_strcasecmp (sections[2], "ENABLE")) goto ERROR;

      ext = g_strdup (sections[0]);
      g_ascii_strdown (ext, -1);
      if (!*sections[1])
         type = g_strdup ("UNKNOWN");
      else
         type = g_strdup (sections[1]);

      userdef_ext_list
         = g_list_append (userdef_ext_list, ext);
      g_hash_table_insert (userdef_ext_mimetype_table, ext, type);

   ERROR:
      g_strfreev (sections);
   }
}


GList *
gimv_mime_types_get_known_ext_list (void)
{
   return known_ext_list;
}


GList *
gimv_mime_types_get_known_enabled_ext_list (void)
{
   if (!known_enabled_ext_list
       && conf.imgtype_disables && *conf.imgtype_disables)
   {
      gimv_mime_types_update_known_enabled_ext_list ();
   }

   return known_enabled_ext_list;
}


GList *
gimv_mime_types_get_userdef_ext_list (void)
{
   if (!userdef_ext_list)
      gimv_mime_types_update_userdef_ext_list ();
   return NULL;
}


const gchar *
gimv_mime_types_get_type_from_ext (const gchar *extension)
{
   const gchar *type = NULL;
   gchar ext[MAX_EXT_LEN];
   gint len;

   if (!extension || !*extension) return NULL;

   len = strlen (extension);
   g_return_val_if_fail (len < MAX_EXT_LEN, NULL);

   strcpy(ext, extension);
   g_ascii_strdown (ext, -1);

   if (!known_enabled_ext_list)
      gimv_mime_types_update_known_enabled_ext_list ();
   if (known_ext_mimetype_table)
      type = g_hash_table_lookup (known_ext_mimetype_table, ext);
   if (type) return type;

   if (!userdef_ext_mimetype_table
       && conf.imgtype_user_defs && *conf.imgtype_user_defs)
   {
      gimv_mime_types_update_userdef_ext_list ();
   }

   if (userdef_ext_mimetype_table)
      return g_hash_table_lookup (userdef_ext_mimetype_table, ext);

   return NULL;
}


const gchar *
gimv_mime_types_get_extension (const gchar *filename)
{
   gchar *ext;

   if (!filename)
      return NULL;

   ext = strrchr(filename, '.');
   if (ext)
      ext = ext + 1;
   else
      return NULL;

   if (ext == "\0")
      return NULL;
   else
      return ext;
}


gboolean
gimv_mime_types_extension_is (const gchar *filename, const gchar *ext)
{
   gint len1, len2;

   if (!filename) return FALSE;
   if (!*filename) return FALSE;
   if (!ext) return FALSE;
   if (!*ext) return FALSE;

   len1 = strlen (filename);
   len2 = strlen (ext);

   if (len1 < len2) return FALSE;

   return !g_ascii_strcasecmp (filename + len1 - len2, ext);
}
