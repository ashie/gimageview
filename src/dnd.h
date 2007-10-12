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

#ifndef __DND_H__
#define __DND_H__

#include "gimageview.h"

typedef enum
{
   TARGET_URI_LIST,
   TARGET_TEXT,
   TARGET_GIMV_TAB,
   TARGET_GIMV_COMPONENT,
   TARGET_GIMV_ARCHIVE_MEMBER_LIST
} TargetType;


extern GtkTargetEntry dnd_types_all[];
extern const gint dnd_types_all_num;

extern GtkTargetEntry *dnd_types_uri;
extern const gint dnd_types_uri_num;

extern GtkTargetEntry *dnd_types_archive;
extern const gint dnd_types_archive_num;

extern GtkTargetEntry *dnd_types_tab_component;
extern const gint dnd_types_tab_component_num;

extern GtkTargetEntry *dnd_types_component;
extern const gint dnd_types_component_num;

/*
  extern GtkTargetEntry dnd_com_types[];
  extern const gint dnd_com_types_num;
*/


GList *dnd_get_file_list     (const gchar          *string,
                              gint                  len);
void   dnd_src_set           (GtkWidget            *widget,
                              const GtkTargetEntry *entry,
                              gint                  num);
void   dnd_dest_set          (GtkWidget            *widget,
                              const GtkTargetEntry *entry,
                              gint                  num);
void   dnd_file_operation    (const gchar          *dest_dir,
                              GdkDragContext       *context,
                              GtkSelectionData     *seldata,
                              guint                 time,
                              GimvThumbWin         *tw);

#endif /* __DND_H__ */
