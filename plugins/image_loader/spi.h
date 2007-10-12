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

#ifndef __SPI_H__
#define __SPI_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef ENABLE_SPI

#include <glib.h>

#define SPI_DEFAULT_SEARCH_DIR_LIST \
   PLUGINDIR "/" "spi"

#ifndef G_PLATFORM_WIN32
#include "dllloader/pe_image.h"
#else
#include "gmodule.h"
#define PE_image GModule
#define peimage_create() (NULL)
#define peimage_load(image, filename) \
	(image = g_module_open(filename, G_MODULE_BIND_LAZY))
#define peimage_destroy(image) (g_module_close(image))
#endif

#include "spi-private.h"

typedef struct SusiePlugin_Tag
{
   PE_image           *pe;
   GetPluginInfoFunc   get_plugin_info;
   gchar              *description;
   GList              *formats;
} SusiePlugin;


typedef struct SpiImportFilter_Tag
{
   SusiePlugin         parent;

   IsSupportedFunc     is_supported;
   GetPictureInfoFunc  get_pic_info;
   GetPictureFunc      get_pic;
} SpiImportFilter;


typedef struct SpiExportFilter_Tag
{
   SusiePlugin         parent;
} SpiExportFilter;


typedef struct SpiArchiveExtractor_Tag
{
   SusiePlugin         parent;

   IsSupportedFunc     is_supported;
   GetArchiveInfoFunc  get_archive_info;
   GetFileInfoFunc     get_file_info;
   GetFileFunc         get_file;
} SpiArchiveExtractor;


void       spi_init                       (void);
GList     *spi_get_search_dir_list        (void);
GList     *spi_get_import_filter_list     (void);
GList     *spi_get_export_filter_list     (void);
GList     *spi_get_archive_extractor_list (void);

#endif

#endif /* __SPI_H__ */
