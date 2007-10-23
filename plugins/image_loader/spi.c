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

#include "spi.h"

#ifdef ENABLE_SPI

#include <stdio.h>
#include <string.h>

#include "prefs_spi.h"
#include "utils_file.h"
#include "utils_file_gtk.h"

#ifdef G_PLATFORM_WIN32
static
void* peimage_resolve (PE_image* image, const char* symbol_name)
{
   void* symbol;
   if (g_module_symbol (image, symbol_name, &symbol))
      return symbol;
   else
      return NULL;
}
#endif

static void         spi_clear_search_dir_list    (void);
static void         spi_create_search_dir_list   (void);
static void         spi_search_directory         (const gchar *dirname);
static gboolean     spi_regist                   (PE_image    *pe);
static SusiePlugin *spi_regist_import_filter     (PE_image    *pe);
static SusiePlugin *spi_regist_export_filter     (PE_image    *pe);
static SusiePlugin *spi_regist_archive_extractor (PE_image    *pe);


const char *spi_errormsg[] = {
  /* -1: Unimplemented */
  "Success",
  "Callback returned non-zero",
  "Unknown format",
  "Data broken",
  "No enough memory",
  "Memory error",
  "File read error",
  "Reserved",
  "Internal error"
};
int spi_errormsg_num = sizeof (spi_errormsg) / sizeof (char *);


static gboolean spi_inited               = FALSE;
static GList *spi_search_dir_list        = NULL;
static GList *spi_import_filter_list     = NULL;
static GList *spi_export_filter_list     = NULL;
static GList *spi_archive_extractor_list = NULL;


/*****************************************************************************
 *
 *   Private functions.
 *
 *****************************************************************************/
static void
spi_clear_search_dir_list (void)
{
   g_list_foreach (spi_search_dir_list, (GFunc) g_free, NULL);
   g_list_free (spi_search_dir_list);
   spi_search_dir_list = NULL;
}


static void
spi_create_search_dir_list (void)
{
   const gchar *dirlist;
   gchar **dirs;
   gint i;

   spi_clear_search_dir_list ();

   if (gimv_prefs_spi_get_use_default_dirlist ())
      dirlist = SPI_DEFAULT_SEARCH_DIR_LIST;
   else
      dirlist = gimv_prefs_spi_get_dirconf ();

   if (!dirlist || !*dirlist) return;

   dirs = g_strsplit (dirlist, ",", -1);

   for (i = 0; dirs && dirs[i]; i++) {
      if (*dirs[i])
         spi_search_dir_list = g_list_append (spi_search_dir_list,
                                              g_strdup (dirs[i]));
   }

   g_strfreev (dirs);
}


static void
spi_search_directory (const gchar *dirname)
{
   GList *filelist = NULL, *node;

   g_return_if_fail (dirname);
   if (!isdir (dirname)) return;

   get_dir (dirname, GETDIR_FOLLOW_SYMLINK, &filelist, NULL);
   if (!filelist) return;

   node = filelist;

   while (node) {
      gchar *filename = node->data;
      gboolean success;
      gint len;
      PE_image *pe;

      node = g_list_next (node);

      len = strlen (filename);
      if (len < 4 || strcasecmp (filename + len - 4, ".spi")) continue;

      g_print ("PluginName: %s\n", filename);

      pe = peimage_create ();
      if (!peimage_load (pe, filename)) {
         peimage_destroy (pe);
         g_print ("peimage_load() failed: %s\n", filename);
         continue;
      }

      success = spi_regist (pe);
      if (success) {
         /* hoge */
      } else {
         peimage_destroy(pe);
      }
   }

   g_list_foreach (filelist, (GFunc) g_free, NULL);
   g_list_free (filelist);
}


static gboolean
spi_regist (PE_image *pe)
{
   gchar buf[256];
   gboolean success;
   GetPluginInfoFunc get_plugin_info;
   SusiePlugin *plugin;

   g_return_val_if_fail (pe, FALSE);

   get_plugin_info = peimage_resolve (pe, "GetPluginInfo");
   if (!get_plugin_info) {
      g_print ("Cannot resolve GetPluginInfo.\n");
      goto ERROR;
   }

   success = get_plugin_info (0, buf, 256);
   if (!success) {
      g_print ("GetPluginInfo returns 0\n");
      goto ERROR;
   }

   switch (buf[2]) {
   case 'I':
      g_print ("Detected as Import filter.\n");
      plugin = spi_regist_import_filter (pe);
      break;

   case 'X':
      g_print ("Export filter is not supported yet.\n");
      plugin = spi_regist_export_filter (pe);
      break;

   case 'A':
      g_print ("Archiver extractor is not supported yet.\n");
      plugin = spi_regist_archive_extractor (pe);
      break;

   default:
      g_print ("Unknown susie plugin type %c.\n", buf[2]);
      plugin = NULL;
      break;
   }

   if (!plugin) goto ERROR;

   plugin->pe = pe;
   plugin->get_plugin_info = get_plugin_info;
   success = plugin->get_plugin_info (1, buf, 256);
   if (success) {
      plugin->description = g_strdup (buf);
      g_print ("description: %s\n\n", plugin->description);
   } else {
      plugin->description = NULL;
   }

   /* FIXME */
   plugin->formats = NULL;


   return TRUE;

ERROR:
   return FALSE;
}


static SusiePlugin *
spi_regist_import_filter (PE_image *pe)
{
   SpiImportFilter *loader;

   g_return_val_if_fail (pe, NULL);

   loader = g_new0 (SpiImportFilter, 1);

   loader->is_supported = peimage_resolve (pe, "IsSupported");
   if (!loader->is_supported) {
      g_print ("Cannot resolve IsSupported.\n");
      goto ERROR;
   }

   loader->get_pic_info = peimage_resolve (pe, "GetPictureInfo");
   if (!loader->get_pic_info) {
      g_print ("Cannot resolve GetPictureInfo.\n");
      goto ERROR;
   }

   loader->get_pic = peimage_resolve (pe, "GetPicture");
   if (!loader->get_pic) {
      g_print ("Cannot resolve GetPicture.\n");
      goto ERROR;
   }

   spi_import_filter_list = g_list_append (spi_import_filter_list, loader);

   return (SusiePlugin *) loader;

ERROR:
   g_free (loader);
   return NULL;
}


static SusiePlugin *
spi_regist_export_filter (PE_image *pe)
{
   return NULL;
}


static SusiePlugin *
spi_regist_archive_extractor (PE_image *pe)
{
   SpiArchiveExtractor *extractor;

   g_return_val_if_fail (pe, NULL);

   extractor = g_new0 (SpiArchiveExtractor, 1);

   extractor->is_supported = peimage_resolve (pe, "IsSupported");
   if (!extractor->is_supported) {
      g_print ("Cannot resolve IsSupported.\n");
      goto ERROR;
   }

   extractor->get_archive_info = peimage_resolve (pe, "GetArchiveInfo");
   if (!extractor->get_archive_info) {
      g_print ("Cannot resolve GetArchiveInfo.\n");
      goto ERROR;
   }

   extractor->get_file_info = peimage_resolve (pe, "GetFileInfo");
   if (!extractor->get_file_info) {
      g_print ("Cannot resolve GetFileInfo.\n");
      goto ERROR;
   }

   extractor->get_file = peimage_resolve (pe, "GetFile");
   if (!extractor->get_file) {
      g_print ("Cannot resolve GetFile.\n");
      goto ERROR;
   }

   spi_archive_extractor_list = g_list_append (spi_archive_extractor_list,
                                               extractor);

   return (SusiePlugin *) extractor;

ERROR:
   g_free (extractor);
   return NULL;
}



/*****************************************************************************
 *
 *   Public functions.
 *
 *****************************************************************************/
void
spi_init ()
{
   GList *list;

   spi_create_search_dir_list ();

   /* load spi from disk */
   list = g_list_last (spi_search_dir_list);
   for (; list; list = g_list_previous (list)) {
      gchar *dir = list->data;

      if (dir && *dir)
         spi_search_directory (dir);
   }

   spi_inited = TRUE;
}


GList *
spi_get_search_dir_list (void)
{
   if (!spi_inited)
      spi_init();
   return spi_search_dir_list;
}


GList *
spi_get_import_filter_list (void)
{
   if (!spi_inited)
      spi_init();
   return spi_import_filter_list;
}


GList *
spi_get_export_filter_list (void)
{
   if (!spi_inited)
      spi_init();
   return spi_export_filter_list;
}


GList *
spi_get_archive_extractor_list (void)
{
   if (!spi_inited)
      spi_init();
   return spi_archive_extractor_list;
}

#endif /* ENABLE_SPI */
