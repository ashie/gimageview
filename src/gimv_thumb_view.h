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

#ifndef __GIMV_THUMB_VIEW_H__
#define __GIMV_THUMB_VIEW_H__

#include <sys/stat.h>
#include <unistd.h>

#include "gimageview.h"

#include "fileload.h"
#include "gfileutil.h"
#include "gimv_thumb_cache.h"

#define GIMV_TYPE_THUMB_VIEW            (gimv_thumb_view_get_type ())
#define GIMV_THUMB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_THUMB_VIEW, GimvThumbView))
#define GIMV_THUMB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_THUMB_VIEW, GimvThumbViewClass))
#define GIMV_IS_THUMB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_THUMB_VIEW))
#define GIMV_IS_THUMB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_THUMB_VIEW))
#define GIMV_THUMB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_THUMB_VIEW, GimvThumbViewClass))

#define GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE "Album"

typedef struct GimvThumbViewPriv_Tag   GimvThumbViewPriv;
typedef struct GimvThumbViewClass_Tag  GimvThumbViewClass;
typedef struct GimvThumbViewPlugin_Tag GimvThumbViewPlugin;

typedef enum
{
   GIMV_THUMB_VIEW_MODE_COLLECTION,
   GIMV_THUMB_VIEW_MODE_DIR,
   GIMV_THUMB_VIEW_MODE_RECURSIVE_DIR, /* implemented as
                                          THUMB_VIEW_MODE_COLLECTION */
   GIMV_THUMB_VIEW_MODE_ARCHIVE
} GimvThumbViewMode;

typedef enum
{
   GIMV_THUMB_VIEW_STATUS_NORMAL,
   GIMV_THUMB_VIEW_STATUS_LOADING,
   GIMV_THUMB_VIEW_STATUS_CHECK_DUPLICATE
} GimvThumbViewStatus;

typedef enum
{
   GIMV_THUMB_VIEW_OPEN_IMAGE_AUTO,
   GIMV_THUMB_VIEW_OPEN_IMAGE_PREVIEW,
   GIMV_THUMB_VIEW_OPEN_IMAGE_NEW_WIN,
   GIMV_THUMB_VIEW_OPEN_IMAGE_SHARED_WIN,
   GIMV_THUMB_VIEW_OPEN_IMAGE_EXTERNAL
} GimvThumbViewOpenImageType;

struct GimvThumbView_Tag
{
   GObject              parent;

   GList               *thumblist;

   GimvThumbWin        *tw;

   GtkWidget           *container;
   GtkWidget           *popup_menu;

   gchar               *tabtitle;
   gint                 thumb_size;

   GimvThumbViewMode    mode;

   gint                 filenum;
   gulong               filesize;

   const gchar         *summary_mode;
   GimvThumbViewPlugin *vfuncs;

   /* FIXME! */
   /* for DnD */
   gchar               *dnd_destdir;

   /* FIXME!! */
   /* progress infomation of thumbnail loading */
   GimvThumbViewStatus  status;
   FilesLoader         *progress;
   GList               *load_list;

   GimvThumbViewPriv   *priv;
};

struct GimvThumbViewClass_Tag
{
   GObjectClass parent_class;
};

#define GIMV_THUMBNAIL_VIEW_IF_VERSION 8

struct GimvThumbViewPlugin_Tag
{
   const guint32 if_version; /* plugin interface version */

   const gchar * const label;

   const gint priority_hint;

   GtkWidget *(*create)                 (GimvThumbView *tv,
                                         const gchar   *dest_mode);
   void       (*freeze)                 (GimvThumbView *tv);
   void       (*thaw)                   (GimvThumbView *tv);
   void       (*insert_thumb)           (GimvThumbView *tv,
                                         GimvThumb     *thumb,
                                         const gchar   *dest_mode);
   void       (*update_thumb)           (GimvThumbView *tv,
                                         GimvThumb     *thumb,
                                         const gchar   *dest_mode);
   void       (*remove_thumb)           (GimvThumbView *tv,
                                         GimvThumb     *thumb);
   GList     *(*get_load_list)          (GimvThumbView *tv);
   void       (*adjust_position)        (GimvThumbView *tv,
                                         GimvThumb     *thumb);
   gboolean   (*set_selection)          (GimvThumbView *tv,
                                         GimvThumb     *thumb,
                                         gboolean       select);
   void       (*set_focus)              (GimvThumbView *tv,
                                         GimvThumb     *thumb);
   GimvThumb *(*get_focus)              (GimvThumbView *tv);
   gboolean   (*is_in_view)             (GimvThumbView *tv,
                                         GimvThumb     *thumb);
};

gint           gimv_thumb_view_label_to_num       (const gchar      *label);
const gchar   *gimv_thumb_view_num_to_label       (gint              num);
gchar        **gimv_thumb_view_get_summary_mode_labels
                                                  (gint             *num_ret);
GList         *gimv_thumb_view_get_summary_mode_list
                                                  (void);

GType          gimv_thumb_view_get_type           (void);
GimvThumbView *gimv_thumb_view_new                (void);
gboolean       gimv_thumb_view_set_widget         (GimvThumbView    *tv,
                                                   GimvThumbWin     *tw,
                                                   GtkWidget        *container,
                                                   const gchar      *summary_mode);
void           gimv_thumb_view_reload             (GimvThumbView    *tv,
                                                   FilesLoader      *files,
                                                   GimvThumbViewMode mode);
GimvThumbView *gimv_thumb_view_find_opened_dir    (const gchar      *path);
GimvThumbView *gimv_thumb_view_find_opened_archive(const gchar      *path);
const gchar   *gimv_thumb_view_get_path           (GimvThumbView    *tv);
void           gimv_thumb_view_sort_data          (GimvThumbView    *tv);
gboolean       gimv_thumb_view_load_thumbnails    (GimvThumbView    *tv,
                                                   GList            *loadlist,
                                                   const gchar      *dest_mode);
gboolean       gimv_thumb_view_append_thumbnail   (GimvThumbView    *tv,
                                                   FilesLoader      *files,
                                                   gboolean          force);
void           gimv_thumb_view_change_summary_mode(GimvThumbView    *tv,
                                                   const gchar      *mode);
void           gimv_thumb_view_clear              (GimvThumbView    *tv);
void           gimv_thumb_view_refresh_thumbnail  (GimvThumbView    *tv,
                                                   GimvThumb        *thumb,
                                                   ThumbLoadType     type);
gboolean       gimv_thumb_view_refresh_list       (GimvThumbView    *tv);
gint           gimv_thumb_view_refresh_list_idle  (gpointer          data);
void           gimv_thumb_view_adjust             (GimvThumbView    *tv,
                                                   GimvThumb        *thumb);
GList         *gimv_thumb_view_get_selection_list (GimvThumbView    *tv);
GList         *gimv_thumb_view_get_selected_file_list
                                                  (GimvThumbView    *tv);
gboolean       gimv_thumb_view_set_selection      (GimvThumb        *thumb,
                                                   gboolean          select);
gboolean       gimv_thumb_view_set_selection_all  (GimvThumbView    *tv,
                                                   gboolean          select);
gboolean       gimv_thumb_view_set_selection_multiple
                                                  (GimvThumbView    *tv,
                                                   GimvThumb        *thumb,
                                                   gboolean          reverse,
                                                   gboolean          clear);
void           gimv_thumb_view_set_focus          (GimvThumbView    *tv,
                                                   GimvThumb        *thumb);
GimvThumb     *gimv_thumb_view_get_focus          (GimvThumbView    *tv);
void           gimv_thumb_view_grab_focus         (GimvThumbView    *tv);
void           gimv_thumb_view_find_duplicates    (GimvThumbView    *tv,
                                                   GimvThumb        *thumb,
                                                   const gchar      *type);
void           gimv_thumb_view_reset_tab_label    (GimvThumbView    *tv,
                                                   const gchar      *title);


/* reference callback functions used by child module */
GList         *gimv_thumb_view_get_list           (void);
gboolean       gimv_thumb_view_thumb_button_press_cb
                                                  (GtkWidget        *widget,
                                                   GdkEventButton   *event,
                                                   GimvThumb        *thumb);
gboolean       gimv_thumb_view_thumb_button_release_cb
                                                  (GtkWidget        *widget,
                                                   GdkEventButton   *event,
                                                   GimvThumb        *thumb);
gboolean       gimv_thumb_view_thumb_key_press_cb (GtkWidget        *widget,
                                                   GdkEventKey      *event,
                                                   GimvThumb        *thumb);
gboolean       gimv_thumb_view_thumb_key_release_cb
                                                  (GtkWidget        *widget,
                                                   GdkEventKey      *event,
                                                   GimvThumb        *thumb);
gboolean       gimv_thumb_view_motion_notify_cb   (GtkWidget        *widget,
                                                   GdkEventMotion   *event,
                                                   GimvThumb        *thumb);
void           gimv_thumb_view_drag_begin_cb      (GtkWidget        *widget,
                                                   GdkDragContext   *context,
                                                   gpointer          data);
void           gimv_thumb_view_drag_data_received_cb
                                                  (GtkWidget        *widget,
                                                   GdkDragContext   *context,
                                                   gint              x,
                                                   gint              y,
                                                   GtkSelectionData *seldata,
                                                   guint             info,
                                                   guint32           time,
                                                   gpointer          data);
void           gimv_thumb_view_drag_data_get_cb   (GtkWidget        *widget,
                                                   GdkDragContext   *context,
                                                   GtkSelectionData *seldata,
                                                   guint             info,
                                                   guint             time,
                                                   gpointer          data);
void           gimv_thumb_view_drag_end_cb        (GtkWidget        *widget,
                                                   GdkDragContext   *drag_context,
                                                   gpointer          data);
void           gimv_thumb_view_drag_data_delete_cb(GtkWidget        *widget,
                                                   GdkDragContext   *drag_context,
                                                   gpointer          data);

void           gimv_thumb_view_open_image         (GimvThumbView    *tv,
                                                   GimvThumb        *thumb,
                                                   gint              type);
void           gimv_thumb_view_popup_menu         (GimvThumbView    *tv,
                                                   GimvThumb        *thumb,
                                                   GdkEventButton   *event);
void           gimv_thumb_view_file_operate       (GimvThumbView    *tv,
                                                   FileOperateType   type);
void           gimv_thumb_view_rename_file        (GimvThumbView    *tv);
gboolean       gimv_thumb_view_delete_files       (GimvThumbView    *tv);

/* for plugin loader */
gboolean       gimv_thumb_view_plugin_regist      (const gchar *plugin_name,
                                                   const gchar *module_name,
                                                   gpointer     impl,
                                                   gint         size);

#endif /* __GIMV_THUMB_VIEW_H__ */
