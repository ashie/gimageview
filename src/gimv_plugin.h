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

#ifndef __GIMV_PLUGIN_H__
#define __GIMV_PLUGIN_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gmodule.h>
#include <gtk/gtkwidget.h>

#include "gimv_mime_types.h"
#include "gimv_prefs_win.h"

#ifdef USE_GTK2
#  define GIMV_PLUGIN_IF_VERSION    20004
#else
#  define GIMV_PLUGIN_IF_VERSION    4
#endif

#define GIMV_PLUGIN_IO_STREAMER       "IOStreamer"
#define GIMV_PLUGIN_IMAGE_LOADER      "ImageLoader"
#define GIMV_PLUGIN_IMAGE_SAVER       "ImageSaver"
#define GIMV_PLUGIN_MOVIE_LOADER      "MovieLoader"
#define GIMV_PLUGIN_MOVIE_SAVER       "MovieSaver"
#define GIMV_PLUGIN_IMAGE_EFFECTER    "ImageEfecter"
#define GIMV_PLUGIN_ARCHIVER          "Archiver"
#define GIMV_PLUGIN_EXT_ARCHIVER      "ExternalArchiver"
#define GIMV_PLUGIN_IMAGEVIEW_EMBEDER "ImageViewEmbeder"
#define GIMV_PLUGIN_THUMBVIEW_EMBEDER "ThumbnailViewEmbeder"
#define GIMV_PLUGIN_THUMB_CACHE       "ThumbnailCache"
#define GIMV_PLUGIN_DATABASE          "DataBase"
#define GIMV_PLUGIN_MISC              "Misc"

typedef enum {
   GIMV_PLUGIN_PREFS_STRING,
   GIMV_PLUGIN_PREFS_INT,
   GIMV_PLUGIN_PREFS_FLOAT,
   GIMV_PLUGIN_PREFS_BOOL,
} GimvPluginPrefsType;


typedef gboolean (*GimvPluginRegistFn) (const gchar *plugin_name,
                                        const gchar *module_name,
                                        gpointer     impl,
                                        gint         size);

typedef struct GimvPluginInfo_Tag
{
   const guint32 if_version; /* plugin interface version */

   const gchar * const name;
   const gchar * const version;
   const gchar * const author;
   const gchar * const description;

   /*
    * idx:    Sequencial index number which start from 0.
    * impl:   Pointer to implement struct.
    * size:   Size of implement struct.
    * Return: Plugin type if succeeded. If idx is invalid, return NULL.
    */
   const gchar *(*get_implement) (guint     idx,
                                  gpointer *impl,
                                  guint    *size);

   /*
    * idx:    Sequencial index number which start from 0.
    * entry:
    * size:
    * Return: TRUE if succeeded.
    */
   gboolean (*get_mime_type) (guint               idx,
                              GimvMimeTypeEntry **entry,
                              guint              *size);

   /*
    * idx:    Sequencial index number which start from 0.
    * page:   Entry.of preference interface page.
    * size:   Size of page struct.
    * Return: TRUE if succeeded.
    */
   gboolean (*get_prefs_ui)  (guint              idx,
                              GimvPrefsWinPage **page,
                              guint             *size);
} GimvPluginInfo;


/* plugin type */
const gchar *gimv_plugin_type_get           (guint idx);

/* load/unload */
gboolean gimv_plugin_load_module            (const gchar *filename,
                                             gint        *error);
gboolean gimv_plugin_unload_module          (const gchar *filename,
                                             gint        *error);
void     gimv_plugin_search_directory       (const gchar *dirname);

/* plugin list */
void   gimv_plugin_create_search_dir_list   (void);
GList *gimv_plugin_get_search_dir_list      (void);

/* get each infomation from plugin */
const gchar *gimv_plugin_get_name           (GModule *module);
const gchar *gimv_plugin_get_version_string (GModule *module);
const gchar *gimv_plugin_get_author         (GModule *module);
const gchar *gimv_plugin_get_module_name    (GModule *module);
GList       *gimv_plugin_get_list           (const gchar *type);

/* plugin preference */
gboolean gimv_plugin_prefs_save_value       (const gchar     *pname,
                                             const gchar     *ptype,
                                             const gchar     *key,
                                             const gchar     *value);
gboolean gimv_plugin_prefs_load_value       (const gchar     *name,
                                             const gchar     *ptype,
                                             const gchar     *key,
                                             GimvPluginPrefsType type,
                                             gpointer         data);
void     gimv_plugin_prefs_read_files       (void);
void     gimv_plugin_prefs_write_files      (void);

void     gimv_plugin_init                   (void);




/* utility for plugin */
#if 1 /* FIXME!!! uma shika dana ore ha... */
#define GIMV_PLUGIN_GET_IMPL(impl_array, type)                                  \
static const gchar *                                                            \
gimv_plugin_get_impl (guint idx, gpointer *impl, guint *size)                   \
{                                                                               \
   g_return_val_if_fail(impl, NULL);                                            \
   *impl = NULL;                                                                \
   g_return_val_if_fail(size, NULL);                                            \
   *size = 0;                                                                   \
   if (idx < sizeof (impl_array) / sizeof (impl_array[0])) {                    \
      *size = sizeof (impl_array[0]);                                           \
      *impl = &impl_array[idx];                                                 \
   } else {                                                                     \
      return NULL;                                                              \
   }                                                                            \
   return type;                                                                 \
}


#define GIMV_PLUGIN_GET_MIME_TYPE(mime_types_array)                             \
static gboolean                                                                 \
gimv_plugin_get_mime_type (guint idx, GimvMimeTypeEntry **entry, guint *size)   \
{                                                                               \
   g_return_val_if_fail(entry, FALSE);                                          \
   *entry = NULL;                                                               \
   g_return_val_if_fail(size, FALSE);                                           \
   *size = 0;                                                                   \
   if (idx < sizeof (mime_types_array) / sizeof (GimvMimeTypeEntry)) {          \
      *size  = sizeof (mime_types_array[0]);                                    \
      *entry = &mime_types_array[idx];                                          \
   } else {                                                                     \
      return FALSE;                                                             \
   }                                                                            \
   return TRUE;                                                                 \
}


#include <string.h>

typedef struct PluginPrefsEntry_Tag
{
   const gchar         *key;
   GimvPluginPrefsType  type;
   const gchar         *defval;
   gpointer             value;
} GimvPluginPrefsEntry;


#define GIMV_PLUGIN_PREFS_GET_VALUE(prefix, pname, ptype, table)                \
gboolean                                                                        \
prefix##_prefs_get_value (const gchar *key, gpointer *value)                    \
{                                                                               \
   GimvPluginPrefsEntry *entry = NULL;                                          \
   gboolean success;                                                            \
   guint i;                                                                     \
                                                                                \
   g_return_val_if_fail(key && value, FALSE);                                   \
                                                                                \
   *value = NULL;                                                               \
                                                                                \
   for (i = 0; i < sizeof (table) / sizeof (GimvPluginPrefsEntry); i++) {       \
      if (table[i].key && !strcmp(key, table[i].key)) {                         \
         entry = &table[i];                                                     \
         break;                                                                 \
      }                                                                         \
   }                                                                            \
   if (!entry) {                                                                \
      g_warning ("GimvPluginPrefs: key \"%s\" not found!\n", key);              \
      return FALSE;                                                             \
   }                                                                            \
                                                                                \
   success = gimv_plugin_prefs_load_value (pname,                               \
                                           ptype,                               \
                                           entry->key,                          \
                                           entry->type,                         \
                                           value);                              \
   if (!success) {                                                              \
      gimv_plugin_prefs_save_value (pname,                                      \
                                    ptype,                                      \
                                    entry->key,                                 \
                                    entry->defval);                             \
      success = gimv_plugin_prefs_load_value (pname,                            \
                                              ptype,                            \
                                              key,                              \
                                              entry->type,                      \
                                              value);                           \
      g_return_val_if_fail(success, FALSE);                                     \
   }                                                                            \
                                                                                \
   return TRUE;                                                                 \
}


#define GIMV_PLUGIN_PREFS_GET_ALL(prefix, table, src_struct_p, dest_struct)     \
{                                                                               \
   guint i;                                                                     \
   src_struct_p = g_malloc0(sizeof(dest_struct));                               \
   for (i = 0; i < sizeof (table) / sizeof (GimvPluginPrefsEntry); i++) {       \
      prefix##_prefs_get_value(table[i].key, table[i].value);                   \
   }                                                                            \
   *src_struct_p = dest_struct;                                                 \
   for (i = 0; i < sizeof (table) / sizeof (GimvPluginPrefsEntry); i++) {       \
      if (table[i].type == GIMV_PLUGIN_PREFS_STRING) {                          \
         gpointer valp_src, valp_dest;                                          \
         gulong offset;                                                         \
         gchar *str;                                                            \
                                                                                \
         valp_dest = table[i].value;                                            \
         offset = (char *) table[i].value - (char *) &dest_struct;              \
         valp_src = (G_STRUCT_MEMBER_P (src_struct_p, offset));                 \
         str = *(gchar **)valp_dest;                                            \
         if (str) {                                                             \
            *(gchar **)valp_dest = g_strdup (str);                              \
            *(gchar **)valp_src  = g_strdup (str);                              \
         }                                                                      \
      }                                                                         \
   }                                                                            \
}


#define GIMV_PLUGIN_PREFS_APPLY_ALL(table, pname, ptype, action, src_struct_p, dest_struct)\
{                                                                               \
   guint i;                                                                     \
                                                                                \
   for (i = 0;                                                                  \
        i < sizeof (table) / sizeof (GimvPluginPrefsEntry);                     \
        i++)                                                                    \
   {                                                                            \
      GimvPluginPrefsEntry *entry = &table[i];                                  \
      gpointer valp, valp_src, valp_dest;                                       \
      gchar *valstr, buf[256];                                                  \
      gulong offset;                                                            \
                                                                                \
      valp_dest = entry->value;                                                 \
      offset = (gchar *) entry->value - (gchar *) &dest_struct;                 \
      valp_src = (G_STRUCT_MEMBER_P (src_struct_p, offset));                    \
                                                                                \
      switch (action) {                                                         \
      case GIMV_PREFS_WIN_ACTION_OK:                                            \
      case GIMV_PREFS_WIN_ACTION_APPLY:                                         \
         valp = valp_dest;                                                      \
         break;                                                                 \
      default:                                                                  \
         valp = valp_src;                                                       \
         break;                                                                 \
      }                                                                         \
                                                                                \
      switch (entry->type) {                                                    \
      case GIMV_PLUGIN_PREFS_STRING:                                            \
         valstr = *((gchar **) valp);                                           \
         break;                                                                 \
      case GIMV_PLUGIN_PREFS_INT:                                               \
         g_snprintf (buf, sizeof(buf) / sizeof(gchar),                          \
                     "%d", *((gint *) valp));                                   \
         valstr = buf;                                                          \
         break;                                                                 \
      case GIMV_PLUGIN_PREFS_FLOAT:                                             \
         g_snprintf (buf, sizeof(buf) / sizeof(gchar),                          \
                     "%f", *((gfloat *) valp));                                 \
         valstr = buf;                                                          \
         break;                                                                 \
      case GIMV_PLUGIN_PREFS_BOOL:                                              \
         valstr = *((gboolean *) valp) ? "TRUE" : "FALSE";                      \
         break;                                                                 \
      default:                                                                  \
         valstr = NULL;                                                         \
         break;                                                                 \
      }                                                                         \
                                                                                \
      if (valstr)                                                               \
         gimv_plugin_prefs_save_value (pname, ptype, entry->key, valstr);       \
   }                                                                            \
}

#define GIMV_PLUGIN_PREFS_FREE_ALL(table, src_struct_p, dest_struct)            \
{                                                                               \
   guint i;                                                                     \
                                                                                \
   for (i = 0;                                                                  \
        i < sizeof (table) / sizeof (GimvPluginPrefsEntry);                     \
        i++)                                                                    \
   {                                                                            \
      GimvPluginPrefsEntry *entry = &table[i];                                  \
      gpointer valp_src, valp_dest;                                             \
      gulong offset;                                                            \
                                                                                \
      valp_dest = entry->value;                                                 \
      offset = (char *) entry->value - (char *) &dest_struct;                   \
      valp_src = (G_STRUCT_MEMBER_P (src_struct_p, offset));                    \
                                                                                \
      if (entry->type == GIMV_PLUGIN_PREFS_STRING) {                            \
         g_free (*((gchar **) valp_src));                                       \
         g_free (*((gchar **) valp_dest));                                      \
         *((gchar **) valp_src)  = NULL;                                        \
         *((gchar **) valp_dest) = NULL;                                        \
      }                                                                         \
   }                                                                            \
   g_free(src_struct_p);                                                        \
   src_struct_p = NULL;                                                         \
}
#endif /* FIXME!!! uma shika dana ore ha... */

#endif /* __GIMV_PLUGIN_H__ */
