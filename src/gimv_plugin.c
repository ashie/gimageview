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
#include <gmodule.h>

#include "gimageview.h"

#include "fileutil.h"
#include "gfileutil.h"
#include "gimv_plugin.h"
#include "prefs.h"

#define PLUGIN_RC_DIR "pluginrc"

typedef struct PluginTypeEntry_Tag
{
   const gchar        *type;
   GimvPluginRegistFn  regist_fn;
   GList              *plugin_list;
   const gchar        *prefsrc_filename;
   GHashTable         *prefs_table;
} PluginTypeEntry;

#define PLUGIN_REGIST_FUNC(type) \
extern gboolean type##_plugin_regist (const gchar *plugin_name, \
                                      const gchar *module_name, \
                                      gpointer     impl, \
                                      gint         size)

PLUGIN_REGIST_FUNC(gimv_image_loader);
PLUGIN_REGIST_FUNC(gimv_image_saver);
PLUGIN_REGIST_FUNC(gimv_io);
PLUGIN_REGIST_FUNC(fr_archive);
PLUGIN_REGIST_FUNC(gimv_thumb_cache);
PLUGIN_REGIST_FUNC(gimv_image_view);
PLUGIN_REGIST_FUNC(gimv_thumb_view);

static PluginTypeEntry plugin_types[] = {
   {GIMV_PLUGIN_IMAGE_LOADER,      gimv_image_loader_plugin_regist, NULL, "image_loader",   NULL},
   {GIMV_PLUGIN_IMAGE_SAVER,       gimv_image_saver_plugin_regist,  NULL, "image_saver",    NULL},
   {GIMV_PLUGIN_IO_STREAMER,       gimv_io_plugin_regist,           NULL, "io_stream",      NULL},
   {GIMV_PLUGIN_EXT_ARCHIVER,      fr_archive_plugin_regist,        NULL, "ext_archiver",   NULL},
   {GIMV_PLUGIN_THUMB_CACHE,       gimv_thumb_cache_plugin_regist,  NULL, "thumbnail",      NULL},
   {GIMV_PLUGIN_IMAGEVIEW_EMBEDER, gimv_image_view_plugin_regist,   NULL, "image_view",     NULL},
   {GIMV_PLUGIN_THUMBVIEW_EMBEDER, gimv_thumb_view_plugin_regist,   NULL, "thumbnail_view", NULL},
};
static guint plugin_types_num = sizeof (plugin_types) / sizeof (PluginTypeEntry);

static GList      *search_dir_list    = NULL;
static GHashTable *plugin_types_table = NULL;
static GList      *all_plugin_list    = NULL;


/******************************************************************************
 *
 *   Private functions.
 *
 ******************************************************************************/
static void
plugin_clear_search_dir_list (void)
{
   g_list_foreach (search_dir_list, (GFunc) g_free, NULL);
   g_list_free (search_dir_list);
   search_dir_list = NULL;
}


static const gchar *
get_plugin_prefs_filename (const gchar *type)
{
   PluginTypeEntry *entry = g_hash_table_lookup (plugin_types_table, type);

   g_return_val_if_fail(entry, NULL);

   return entry->prefsrc_filename;
}


static gchar *
get_plugin_prefs_path (const gchar *type)
{
   const gchar *filename;
   gchar *path;

   filename = get_plugin_prefs_filename (type);
   g_return_val_if_fail (filename, NULL);

   path = g_strconcat (g_getenv("HOME"), "/", GIMV_RC_DIR, "/",
                       PLUGIN_RC_DIR, "/", filename, NULL);

   return path;
}


static GHashTable *
get_prefs_table (const gchar *type)
{
   PluginTypeEntry *entry = g_hash_table_lookup (plugin_types_table, type);

   g_return_val_if_fail(entry, NULL);

   if (!entry->prefs_table)
      entry->prefs_table = g_hash_table_new (g_str_hash, g_str_equal);
   return entry->prefs_table;
}


static void
plugin_prefs_read_file (const gchar *type)
{
   FILE *pluginrc;
   gchar *path;
   gchar buf[BUF_SIZE], section[256], **pair = NULL;
   gint len;

   if (!plugin_types_table) {
      guint i;

      plugin_types_table = g_hash_table_new (g_str_hash, g_str_equal);

      for (i = 0; i < plugin_types_num; i++) {
         PluginTypeEntry *entry = &plugin_types[i];
         g_hash_table_insert (plugin_types_table, (gpointer) entry->type, entry);
      }
   }

   path = get_plugin_prefs_path (type);
   if (!path || !*path) return;

   pluginrc = fopen(path, "r");
   if (!pluginrc) {
      /* g_warning (_("Can't open plugin preference file for read.")); */
      goto ERROR;
   }

   section[0] = '\0';
   while (fgets (buf, sizeof(buf), pluginrc)) {
      g_strstrip (buf);
      if (buf[0] == '\0') continue;
      if (buf[0] == '\n') continue;

      len = strlen(buf);

      if (len > 2 && len < 256 && buf[0] == '[' && buf[len - 1] == ']') {
         strncpy (section, buf + 1, len - 2);
         section[len - 2] = '\0';
         continue;
      }

      if (!*section) continue;

      pair = g_strsplit (buf, "=", 2);
      if (!pair) continue;

      if (pair[0]) g_strstrip(pair[0]);
      if (pair[1]) g_strstrip(pair[1]);

      if (pair[0] && *pair[0]) {
         gimv_plugin_prefs_save_value (section, type,
                                       pair[0],
                                       pair[1] ? pair[1] : NULL);
      }

      g_strfreev (pair);
      pair = NULL;
   }

   fclose (pluginrc);
ERROR:
   g_free (path);
}


static void
write_value (gpointer key, gpointer value, gpointer user_data)
{
   FILE *pluginrc = user_data;
   gchar *key_string = key;
   gchar *value_string = value;

   g_return_if_fail (key_string && *key_string);
   g_return_if_fail (value_string);
   g_return_if_fail (pluginrc);

   fprintf (pluginrc, "%s=%s\n", key_string, value_string);
}


static void
write_section (gpointer key, gpointer value, gpointer user_data)
{
   GHashTable *htable = value;
   FILE *pluginrc = user_data;
   gchar *string = key;

   g_return_if_fail (string && *string);
   g_return_if_fail (pluginrc);

   if (!htable) return;
   if (g_hash_table_size (htable) < 1) return;

   fprintf (pluginrc, "[%s]\n", string);
   g_hash_table_foreach (htable, (GHFunc) write_value, pluginrc);
   fprintf (pluginrc, "\n");
}


static void
plugin_prefs_write_file (const gchar *type)
{
   GHashTable *prefs_table;
   gchar *path;
   FILE *pluginrc;

   prefs_table = get_prefs_table (type);
   if (!prefs_table) return;
   if (g_hash_table_size (prefs_table) < 1) return;

   path = get_plugin_prefs_path (type);
   if (!path) return;

   mkdirs (path);

   pluginrc = fopen (path, "w");
   if (!pluginrc) {
      g_warning (_("Can't open plugin preference file for write."));
      goto ERROR;
   }

   g_hash_table_foreach (prefs_table, (GHFunc) write_section, pluginrc);

   fclose (pluginrc);
ERROR:
   g_free (path);
}


/******************************************************************************
 *
 *   Public functions.
 *
 ******************************************************************************/
const gchar *
gimv_plugin_type_get (guint idx)
{
   if (idx >= plugin_types_num) return NULL;
   return plugin_types[idx].type;
}


gboolean
gimv_plugin_load_module (const gchar *filename, gint *error)
{
#ifndef G_MODULE_SUFFIX
#  define G_MODULE_SUFFIX "so"
#endif /* G_MODULE_SUFFIX */
#define SO_EXT ("." G_MODULE_SUFFIX)
#define SO_EXT_LEN (strlen(SO_EXT))
   GModule *module;
   GimvPluginInfo *info;
   const gchar *type;
   gpointer impl;
   GimvPrefsWinPage *prefs_ui;
   GimvMimeTypeEntry *mime_type;
   guint size;
   GHashTable *hash = NULL;
   gboolean success, regist;
   gint i;

   success = FALSE;
   regist = FALSE;

   /* load module */
   module =  g_module_open (filename, G_MODULE_BIND_LAZY);
   if (!module) {
      /* FIXME: show message (return GError?) */
      return FALSE;
   }
   success = g_module_symbol (module, "gimv_plugin_info", (gpointer) &info);
   if (!success) {
      /* FIXME: show message (return GError?) */
      return FALSE;
   }

   if (info->if_version != GIMV_PLUGIN_IF_VERSION) {
      g_warning ("GimvPlugin: plugin interface version is invalid!: %s",
                 filename);
      g_module_close (module);
      return FALSE;
   }

   if (!info->get_implement) {
      g_warning ("GimvPlugin: get_implement() method is not found!");
      g_module_close (module);
      return FALSE;
   }

   /* get plugin implement */
   hash = g_hash_table_new (g_str_hash, g_str_equal);

   for (i = 0; (type = info->get_implement(i, &impl, &size)); i++) {
      PluginTypeEntry *type_entry;
 
      if (!impl || size <= 0) continue;

      /* regist each method to memory */
      type_entry = g_hash_table_lookup (plugin_types_table, type);
      if (!type_entry || !type_entry->regist_fn) continue;

      success =  type_entry->regist_fn (info->name,
                                        g_module_name (module),
                                        impl, size);
      regist = regist || success;

      if (success && !g_hash_table_lookup (hash, type)) {
         type_entry->plugin_list
            = g_list_append (type_entry->plugin_list, module);
         g_hash_table_insert (hash, (gpointer) type, (gpointer) type);
      }
   }

   g_hash_table_destroy(hash);
   hash = NULL;

   /* get mime types */
   for (i = 0;
        regist
           && info->get_mime_type
           && info->get_mime_type (i, &mime_type, &size);
        i++)
   {
      if (!mime_type || size <= 0) continue;
      gimv_mime_types_regist (mime_type, module);
   }

   /* get preference UI */
   for (i = 0;
        regist
           && info->get_prefs_ui
           && info->get_prefs_ui (i, &prefs_ui, &size);
        i++)
   {
      if (!prefs_ui || size <= 0) continue;
      gimv_prefs_win_add_page_entry (prefs_ui);
   }

   /* append to list */
   if (regist) {
      all_plugin_list = g_list_append (all_plugin_list, module);
   } else {
      g_module_close (module);
   }

   return regist;
}


gboolean
gimv_plugin_unload_module (const gchar *filename, gint *error)
{
   g_warning ("GimvPlugin: not imlemented yet!");
   return FALSE;
}


void
gimv_plugin_search_directory (const gchar *dirname)
{
   GList *filelist = NULL, *node;

   g_return_if_fail (dirname);
   if (!isdir (dirname)) return;

   get_dir (dirname, GETDIR_FOLLOW_SYMLINK, &filelist, NULL);
   if (!filelist) return;

   for (node = filelist; node; node = g_list_next (node)) {
      gchar *filename = node->data;
      size_t len;

      len = strlen (filename);
      if (len < SO_EXT_LEN || strcmp (filename + len - SO_EXT_LEN, SO_EXT))
         continue;

      gimv_plugin_load_module (filename, NULL);
   }

   g_list_foreach (filelist, (GFunc) g_free, NULL);
   g_list_free (filelist);
}


void
gimv_plugin_create_search_dir_list (void)
{
   const gchar *dirlist = NULL;
   gchar **dirs;
   gint i;

   plugin_clear_search_dir_list ();

   if (conf.plugin_use_default_search_dir_list)
      dirlist = PLUGIN_DEFAULT_SEARCH_DIR_LIST;
   else
      dirlist = conf.plugin_search_dir_list;

   if (!dirlist || !*dirlist) return;

   dirs = g_strsplit (dirlist, ",", -1);

   for (i = 0; dirs && dirs[i]; i++) {
      if (*dirs[i])
         search_dir_list = g_list_append (search_dir_list, g_strdup (dirs[i]));
   }

   g_strfreev (dirs);
}


GList *
gimv_plugin_get_search_dir_list (void)
{
   return search_dir_list;
}


const gchar *
gimv_plugin_get_name (GModule *module)
{
   gboolean success;
   GimvPluginInfo *info;

   g_return_val_if_fail (module, NULL);

   success = g_module_symbol (module, "gimv_plugin_info", (gpointer) &info);
   g_return_val_if_fail (success, NULL);

   return info->name;
}


const gchar *
gimv_plugin_get_version_string (GModule *module)
{
   gboolean success;
   GimvPluginInfo *info;

   g_return_val_if_fail (module, NULL);

   success = g_module_symbol (module, "gimv_plugin_info", (gpointer) &info);
   g_return_val_if_fail (success, NULL);

   return info->version;
}


const gchar *
gimv_plugin_get_author (GModule *module)
{
   gboolean success;
   GimvPluginInfo *info;

   g_return_val_if_fail (module, NULL);

   success = g_module_symbol (module, "gimv_plugin_info", (gpointer) &info);
   g_return_val_if_fail (success, NULL);

   return info->author;
}


const gchar *
gimv_plugin_get_module_name (GModule *module)
{
   gboolean success;
   GimvPluginInfo *info;

   g_return_val_if_fail (module, NULL);

   success = g_module_symbol (module, "gimv_plugin_info", (gpointer) &info);
   g_return_val_if_fail (success, NULL);

   return g_module_name (module);
}


GList *
gimv_plugin_get_list (const gchar *type)
{
   PluginTypeEntry *entry;

   if (!type || !g_strcasecmp(type, "all")) {
      return all_plugin_list;
   } else {
      entry = g_hash_table_lookup(plugin_types_table, type);
      g_return_val_if_fail (entry, NULL);
      return entry->plugin_list;
   }
}


gboolean
gimv_plugin_prefs_save_value (const gchar *pname,
                              const gchar *ptype,
                              const gchar *key,
                              const gchar *value)
{
   GHashTable *prefs_table, *htable;
   gchar *old_value, *orig_key;
   gboolean success;
   gchar *new_key, *new_value;

   g_return_val_if_fail (pname && *pname, FALSE);
   g_return_val_if_fail (key && *key, FALSE);

   prefs_table = get_prefs_table (ptype);
   g_return_val_if_fail (prefs_table, FALSE);

   new_key   = g_strdup(key);
   new_value = value ? g_strdup(value) : g_strdup("");

   htable = g_hash_table_lookup (prefs_table, pname);
   if (!htable) {
      htable = g_hash_table_new (g_str_hash, g_str_equal);
      g_hash_table_insert (prefs_table, g_strdup (pname), htable);
   }

   success = g_hash_table_lookup_extended (htable, key,
                                           (gpointer) &orig_key,
                                           (gpointer) &old_value);
   if (success) {
      g_hash_table_remove (htable, key);
      g_free (orig_key);
      g_free (old_value);
   }

   g_hash_table_insert (htable, new_key, new_value);

   return TRUE;
}


gboolean
gimv_plugin_prefs_load_value (const gchar *pname,
                              const gchar *ptype,
                              const gchar *key,
                              GimvPluginPrefsType type,
                              gpointer data)
{
   GHashTable *prefs_table, *htable;
   gchar *value;

   g_return_val_if_fail (pname && *pname, FALSE);
   g_return_val_if_fail (key && *key, FALSE);
   g_return_val_if_fail (data, FALSE);

   prefs_table = get_prefs_table (ptype);
   g_return_val_if_fail (prefs_table, FALSE);

   htable = g_hash_table_lookup (prefs_table, pname);
   if (!htable) return FALSE;

   value = g_hash_table_lookup (htable, key);
   if (!value) return FALSE;
   /* g_return_val_if_fail (!*value, FALSE); */

   switch (type) {
   case GIMV_PLUGIN_PREFS_STRING:
      *((gchar **) data) = value;
      break;
   case GIMV_PLUGIN_PREFS_INT:
      *((gint *) data) = atoi (value);
      break;
   case GIMV_PLUGIN_PREFS_FLOAT:
      *((gfloat *) data) = atof (value);
      break;
   case GIMV_PLUGIN_PREFS_BOOL:
      if (!g_strcasecmp (value, "TRUE"))
         *((gboolean *) data) = TRUE;
      else
         *((gboolean *) data) = FALSE;
      break;
   default:
      return FALSE;
   }

   return TRUE;
}


void
gimv_plugin_prefs_read_files (void)
{
   guint i;
   for (i = 0; i < plugin_types_num; i ++)
      plugin_prefs_read_file (plugin_types[i].type);
}


void
gimv_plugin_prefs_write_files (void)
{
   guint i;
   for (i = 0; i < plugin_types_num; i ++)
      plugin_prefs_write_file (plugin_types[i].type);
}


void
gimv_plugin_init (void)
{
   GList *list;

   gimv_plugin_create_search_dir_list ();

   /* check whether platform supports dynamic loading or not */
   if (!g_module_supported ()) return;

   if (!plugin_types_table) {
      guint i;
      
      plugin_types_table = g_hash_table_new (g_str_hash, g_str_equal);

      for (i = 0; i < plugin_types_num; i++) {
         PluginTypeEntry *entry = &plugin_types[i];
         g_hash_table_insert (plugin_types_table, (gpointer) entry->type, entry);
      }
   }

   /* load module from disk */
   list = g_list_last (search_dir_list);
   for (; list; list = g_list_previous (list)) {
      gchar *dir = list->data;

      if (dir && *dir)
         gimv_plugin_search_directory (dir);
   }
}
