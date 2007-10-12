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

#ifndef __DETAILVIEW_PRIVATE_H__
#define __DETAILVIEW_PRIVATE_H__

/* for converting thumbnail data to string */
typedef gchar *(*DetailViewColDataStr) (GimvThumb *thumb);

typedef struct DetailViewColumn_Tag {
   gchar                *title;  /* column title */
   gint                  width;  /* Column width. Currentry, this value not used */
   gboolean              free;   /* Free after setting text data to columns or not */
   DetailViewColDataStr  func;   /* Pointer to function convert data to string */
   GtkJustification      justification;
   gboolean              need_sync;
} DetailViewColumn;


void detailview_create_title_idx_list ();


extern DetailViewColumn detailview_columns[];
extern gint             detailview_columns_num;


extern GtkTargetEntry detailview_dnd_targets[];
extern const gint detailview_dnd_targets_num;


extern GList           *detailview_title_idx_list;
extern gint             detailview_title_idx_list_num;

#endif /*__DETAILVIEW_PRIVATE_H__ */
