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

#ifndef __DETAILVIEW_H__
#define __DETAILVIEW_H__

#include "gimageview.h"
#include "fileload.h"
#include "gimv_thumb_view.h"

#define DETAIL_VIEW_LABEL  "Detail"
#define DETAIL_ICON_LABEL  "Detail + Icon"
#define DETAIL_THUMB_LABEL "Detail + Thumbnail"

#define DETAIL_DEFAULT_COLUMN_ORDER \
   "Name,Image size,Size (byte),Type,Cache type,Modification time,User,Group,Mode"

void       detailview_freeze                   (GimvThumbView *tv);
void       detailview_thaw                     (GimvThumbView *tv);
void       detailview_append_thumb_frame       (GimvThumbView *tv,
                                                GimvThumb     *thumb,
                                                const gchar   *dest_mode);
void       detailview_update_thumbnail         (GimvThumbView *tv,
                                                GimvThumb     *thumb,
                                                const gchar   *dest_mode);
void       detailview_remove_thumbnail         (GimvThumbView *tv,
                                                GimvThumb     *thumb);
GList     *detailview_get_load_list            (GimvThumbView *tv);
GtkWidget *detailview_resize                   (GimvThumbView *tv);
void       detailview_adjust                   (GimvThumbView *tv,
                                                GimvThumb     *thumb);
GtkWidget *detailview_create                   (GimvThumbView *tv,
                                                const gchar   *dest_mode);
gboolean   detailview_set_selection            (GimvThumbView *tv,
                                                GimvThumb     *thumb,
                                                gboolean       select);
void       detailview_set_focus                (GimvThumbView *tv,
                                                GimvThumb     *thumb);
GimvThumb *detailview_get_focus                (GimvThumbView *tv);
gboolean   detailview_thumbnail_is_in_viewport (GimvThumbView *tv,
                                                GimvThumb     *thumb);

void       detailview_create_title_idx_list    (void);

/* for preference */
gint   detailview_get_titles_num (void);
gchar *detailview_get_title      (gint idx);
gint   detailview_get_title_idx  (const gchar *title);
void   detailview_apply_config   (void);

#endif /* __DETAILVIEW_H__ */
