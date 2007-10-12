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

#ifndef __GIMV_MIME_TYPES_H__
#define __GIMV_MIME_TYPES_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gmodule.h>
#include "gimv_icon_stock.h"

typedef struct GimvMimeTypeEntry_Taq
{
   const gchar  *mime_type;

   const gchar  *description;

   const gchar **extensions;
   const gint    extensions_len;
   const GimvIconStockEntry *icon;
} GimvMimeTypeEntry;

typedef struct GimvMimeTypePluginEntry_Tag
{
   const gchar *type;
   const gchar *name;
} GimvMimeTypePluginEntry;

typedef struct GimvMimeType_Tag
{
   const gchar  *mime_type;
   GList        *extensions_list;
   GList        *plugins_list;
   GList        *icons_list;
} GimvMimeType;

gboolean gimv_mime_types_regist         (GimvMimeTypeEntry *entry,
                                         GModule           *module);
gboolean gimv_mime_types_add_extension  (const gchar *mime_type,
                                         const gchar *extension);
gboolean gimv_mime_types_resolve_by_ext (const gchar       *ext,
                                         const gchar      **mime_type,
                                         const gchar      **plugin_type);

void         gimv_mime_types_update_known_enabled_ext_list (void);
void         gimv_mime_types_update_userdef_ext_list       (void);
GList       *gimv_mime_types_get_known_ext_list            (void);
GList       *gimv_mime_types_get_known_enabled_ext_list    (void);
GList       *gimv_mime_types_get_userdef_ext_list          (void);
const gchar *gimv_mime_types_get_type_from_ext             (const gchar *ext);

const gchar *gimv_mime_types_get_extension                 (const gchar *filename);
gboolean     gimv_mime_types_extension_is                  (const gchar *filename,
                                                            const gchar *ext);

#endif /* __GIMV_MIME_TYPES_H__ */
