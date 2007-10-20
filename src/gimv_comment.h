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

#ifndef __GIMV_COMMENT_H__
#define __GIMV_COMMENT_H__

#include "gimageview.h"

#define GIMV_TYPE_COMMENT            (gimv_comment_get_type ())
#define GIMV_COMMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_COMMENT, GimvComment))
#define GIMV_COMMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_COMMENT, GimvCommentClass))
#define GIMV_IS_COMMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_COMMENT))
#define GIMV_IS_COMMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_COMMENT))
#define GIMV_COMMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_COMMENT, GimvCommentClass))

typedef struct GimvCommentClass_Tag GimvCommentClass;

struct GimvComment_Tag
{
   GObject      parent;

   gchar         *filename;
   GimvImageInfo *info;        /* 1:1 relation */

   GList         *data_list;   /* comment data list */
   gchar         *note;
};

struct GimvCommentClass_Tag {
   GObjectClass parent_class;

   /* -- Signals -- */
   void (*file_saved)   (GimvComment   *comment,
                         GimvImageInfo *info); 
   void (*file_deleted) (GimvComment   *comment,
                         GimvImageInfo *info); 
};

typedef gchar *(*GimvCommentDataGetDefValFn) (GimvImageInfo *info, gpointer data);

typedef struct GimvCommentDataEntry_Tag
{
   gchar *key;
   gchar *display_name;
   gchar *value;
   gboolean enable;
   gboolean auto_val;
   gboolean display;
   gboolean userdef;
   GimvCommentDataGetDefValFn def_val_fn;
} GimvCommentDataEntry;

GType        gimv_comment_get_type                 (void);

GimvComment *gimv_comment_get_from_image_info      (GimvImageInfo *info);

void         gimv_comment_update_data_entry_list   (void);
GList       *gimv_comment_get_data_entry_list      (void);

gchar       *gimv_comment_get_path                 (const gchar *img_path);
gchar       *gimv_comment_find_file                (const gchar *img_path);
GimvCommentDataEntry
            *gimv_comment_data_entry_find_template_by_key (const gchar *key);
GimvCommentDataEntry
            *gimv_comment_find_data_entry_by_key   (GimvComment *comment,
                                                    const gchar *key);
GimvCommentDataEntry
            *gimv_comment_append_data              (GimvComment *comment,
                                                    const gchar *key,
                                                    const gchar *value);
void         gimv_comment_data_entry_remove        (GimvComment *comment,
                                                    GimvCommentDataEntry *entry);
void         gimv_comment_data_entry_remove_by_key (GimvComment *comment,
                                                    const gchar *key);
GimvCommentDataEntry
             *gimv_comment_data_entry_dup           (GimvCommentDataEntry *src);
void          gimv_comment_data_entry_delete        (GimvCommentDataEntry *entry);
gboolean      gimv_comment_update_note              (GimvComment *comment,
                                                     gchar       *note);
gboolean      gimv_comment_save_file                (GimvComment *comment);
void          gimv_comment_delete_file              (GimvComment *comment);

#endif /* __GIMV_COMMENT_H__ */
