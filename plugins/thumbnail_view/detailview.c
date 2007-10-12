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

#include "detailview.h"

#include <time.h>
#include <string.h>

#include "dnd.h"
#include "detailview_priv.h"
#include "detailview_prefs.h"
#include "fileutil.h"
#include "gimv_image_info.h"
#include "gimv_plugin.h"
#include "gimv_thumb.h"
#include "gimv_thumb_view.h"

static GimvThumbViewPlugin detailview_modes[] =
{
   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail"),
    100,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail + Icon"),
    110,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},

   {GIMV_THUMBNAIL_VIEW_IF_VERSION,
    N_("Detail + Thumbnail"),
    120,
    detailview_create,
    detailview_freeze,
    detailview_thaw,
    detailview_append_thumb_frame,
    detailview_update_thumbnail,
    detailview_remove_thumbnail,
    detailview_get_load_list,
    detailview_adjust,
    detailview_set_selection,
    detailview_set_focus,
    detailview_get_focus,
    detailview_thumbnail_is_in_viewport},
};

GIMV_PLUGIN_GET_IMPL(detailview_modes, GIMV_PLUGIN_THUMBVIEW_EMBEDER)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("Thumbnail View Detail Mode"),
   version:       "0.0.2",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: NULL,
   get_prefs_ui:  gimv_prefs_ui_detailview_get_page,
};


/* for setting clist text data */
static gchar *column_data_filename   (GimvThumb *thumb);
static gchar *column_data_image_size (GimvThumb *thumb);
static gchar *column_data_size       (GimvThumb *thumb);
static gchar *column_data_type       (GimvThumb *thumb);
static gchar *column_data_atime      (GimvThumb *thumb);
static gchar *column_data_mtime      (GimvThumb *thumb);
static gchar *column_data_ctime      (GimvThumb *thumb);
static gchar *column_data_uid        (GimvThumb *thumb);
static gchar *column_data_gid        (GimvThumb *thumb);
static gchar *column_data_mode       (GimvThumb *thumb);
static gchar *column_data_cache      (GimvThumb *thumb);


DetailViewColumn detailview_columns [] =
{
   {NULL,                    0,   FALSE, NULL,                  GTK_JUSTIFY_CENTER, FALSE},
   {N_("Name"),              100, TRUE,  column_data_filename,  GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Size (byte)"),       100, TRUE,  column_data_size,      GTK_JUSTIFY_RIGHT,  FALSE},
   {N_("Type"),              100, TRUE,  column_data_type,      GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Cache type"),        100, FALSE, column_data_cache,     GTK_JUSTIFY_LEFT,   TRUE},
   {N_("Access time"),       150, TRUE,  column_data_atime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Modification time"), 150, TRUE,  column_data_mtime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Change time"),       150, TRUE,  column_data_ctime,     GTK_JUSTIFY_LEFT,   FALSE},
   {N_("User"),              40,  TRUE,  column_data_uid,       GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Group"),             40,  TRUE,  column_data_gid,       GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Mode"),              100, TRUE,  column_data_mode,      GTK_JUSTIFY_LEFT,   FALSE},
   {N_("Image size"),        100, TRUE,  column_data_image_size,GTK_JUSTIFY_CENTER, TRUE},
};
gint detailview_columns_num = sizeof (detailview_columns) / sizeof (DetailViewColumn);


GtkTargetEntry detailview_dnd_targets[] = {
   {"text/uri-list", 0, TARGET_URI_LIST},
};
const gint detailview_dnd_targets_num = sizeof(detailview_dnd_targets) / sizeof(GtkTargetEntry);


GList *detailview_title_idx_list     = NULL;
gint   detailview_title_idx_list_num = 0;


/******************************************************************************
 *
 *   for title index list.
 *
 ******************************************************************************/
void
detailview_create_title_idx_list (void)
{
   static gchar *config_columns_string = NULL;
   gchar **titles, *data_order;
   gint i = 0;

   detailview_prefs_get_value("data_order", (gpointer) &data_order);
 
   if (!data_order) {
      config_columns_string = NULL;
      if (detailview_title_idx_list)
         g_list_free (detailview_title_idx_list);
      detailview_title_idx_list_num = 0;
      return;
   }

   if (data_order == config_columns_string) return;

   if (detailview_title_idx_list) g_list_free (detailview_title_idx_list);
   detailview_title_idx_list = NULL;

   titles = g_strsplit (data_order, ",", -1);

   g_return_if_fail (titles);

   detailview_title_idx_list_num = 0;
   config_columns_string = data_order;

   while (titles[i]) {
      gint idx;
      idx = detailview_get_title_idx (titles[i]);
      if (idx > 0) {
         detailview_title_idx_list
            = g_list_append (detailview_title_idx_list, GINT_TO_POINTER (idx));
         detailview_title_idx_list_num++;
      }
      i++;
   }

   g_strfreev (titles);
}


gint
detailview_get_titles_num (void)
{
   return detailview_columns_num;
}


gchar *
detailview_get_title (gint idx)
{
   g_return_val_if_fail (idx > 0 && idx < detailview_columns_num, NULL);

   return detailview_columns[idx].title;
}


gint
detailview_get_title_idx (const gchar *title)
{
   gint i;

   g_return_val_if_fail (title, -1);

   for (i = 1; i < detailview_columns_num; i++) {
      if (!detailview_columns[i].title) continue;
      if (!strcmp (detailview_columns[i].title, title))
         return i;
   }

   return -1;
}


void
detailview_apply_config (void)
{
   GList *node;

   detailview_create_title_idx_list ();

   node = gimv_thumb_view_get_list();

   while (node) {
      GimvThumbView *tv = node->data;

      if (!strcmp (tv->summary_mode, DETAIL_VIEW_LABEL)
          || !strcmp (tv->summary_mode, DETAIL_ICON_LABEL)
          || !strcmp (tv->summary_mode, DETAIL_THUMB_LABEL))
      {
         gimv_thumb_view_set_widget (tv, tv->tw, tv->container, tv->summary_mode);
      }

      node = g_list_next (node);
   }
}



/******************************************************************************
 *
 *   for creating column text.
 *
 ******************************************************************************/
static gchar *
column_data_filename (GimvThumb *thumb)
{
   GimvThumbView *tv;

   if (!thumb) return NULL;
   tv = gimv_thumb_get_parent_thumbview (thumb);

   /*
   if (tv->mode == THUMB_VIEW_MODE_DIR)
      return g_strdup (g_basename (thumb->info->filename));
   else
      return g_strdup (thumb->info->filename);
   */

   {
      const gchar *filename;
      gchar *retval;

      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR)
         filename = g_basename (gimv_image_info_get_path (thumb->info));
      else
         filename = gimv_image_info_get_path (thumb->info);

      retval = gimv_filename_to_internal (filename);

      return retval;
   }
}


static gchar *
column_data_image_size (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   if (thumb->info->width > 0 && thumb->info->width > 0)
      return g_strdup_printf ("%d x %d", thumb->info->width, thumb->info->height);
   else
      return g_strdup (_("Unknwon"));

}


static gchar *
column_data_size (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_size2str (thumb->info->st.st_size, FALSE);
}


static gchar *
column_data_type (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return g_strdup (gimv_image_detect_type_by_ext (thumb->info->filename));
}


static gchar *
column_data_atime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_atime);
}


static gchar *
column_data_mtime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_mtime);
}


static gchar *
column_data_ctime (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_time2str (thumb->info->st.st_ctime);
}


static gchar *
column_data_uid (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_uid2str (thumb->info->st.st_uid);
}


static gchar *
column_data_gid (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_gid2str (thumb->info->st.st_gid);
}


static gchar *
column_data_mode (GimvThumb *thumb)
{
   if (!thumb) return NULL;

   return fileutil_mode2str (thumb->info->st.st_mode);
}


static gchar *
column_data_cache (GimvThumb *thumb)
{
   const gchar *cache_type;

   if (!thumb) return NULL;

   cache_type = gimv_thumb_get_cache_type (thumb);

   return (gchar *) cache_type;
}
