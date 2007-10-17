/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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

#include "gimv_dupl_finder.h"

#include <string.h>

#include "gimv_thumb.h"
#include "prefs.h"
#include "gimv_dupl_win.h"


typedef enum {
   START_SIGNAL,
   STOP_SIGNAL,
   PROGRESS_UPDATE_SIGNAL,
   FOUND_SIGNAL,
   LAST_SIGNAL
} GimvDuplFinderSignalType;


static void gimv_dupl_finder_dispose (GObject *object);

gboolean idle_duplicates_find    (gpointer user_data);
gboolean timeout_duplicates_find (gpointer data);


static gint gimv_dupl_finder_signals[LAST_SIGNAL] = {0};


extern GimvDuplCompFuncTable gimv_dupl_similar_funcs;
extern GimvDuplCompFuncTable gimv_dupl_file_size_funcs;
extern GimvDuplCompFuncTable gimv_dupl_md5_funcs;

GimvDuplCompFuncTable *duplicates_funcs_table[] =
{
   &gimv_dupl_similar_funcs,
   &gimv_dupl_md5_funcs,
   &gimv_dupl_file_size_funcs,
};


static const gchar **labels = NULL;

const gchar **
gimv_dupl_finder_get_algol_types (void)
{

   if (!labels) {
      gint i, num;

      num = sizeof (duplicates_funcs_table) / sizeof (GimvDuplCompFuncTable *);
      labels = g_malloc0 (sizeof (gchar*) * (num + 1));

      for (i = 0; i < num; i++) {
         GimvDuplCompFuncTable *funcs = duplicates_funcs_table[i];

         labels[i] = g_strdup (funcs->label);
      }
   }

   return labels;
}


G_DEFINE_TYPE (GimvDuplFinder, gimv_dupl_finder, G_TYPE_OBJECT)


static void
gimv_dupl_finder_class_init (GimvDuplFinderClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = (GObjectClass *) klass;

   gimv_dupl_finder_signals[START_SIGNAL]
      = g_signal_new ("start",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvDuplFinderClass, start),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_dupl_finder_signals[STOP_SIGNAL]
      = g_signal_new ("stop",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvDuplFinderClass, stop),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_dupl_finder_signals[PROGRESS_UPDATE_SIGNAL]
      = g_signal_new ("progress_update",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvDuplFinderClass,
                                       progress_update),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

   gimv_dupl_finder_signals[FOUND_SIGNAL]
      = g_signal_new ("found",
                      G_TYPE_FROM_CLASS (gobject_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimvDuplFinderClass, found),
                      NULL, NULL,
                      gtk_marshal_NONE__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

   gobject_class->dispose = gimv_dupl_finder_dispose;

   klass->start           = NULL;
   klass->stop            = NULL;
   klass->progress_update = NULL;
   klass->found           = NULL;
}


static void
gimv_dupl_finder_init (GimvDuplFinder *finder)
{
   finder->src_list     = NULL;
   finder->dest_list    = NULL;
   finder->cur1         = NULL;
   finder->cur2         = NULL;
   finder->table        = NULL;
   finder->funcs        = &gimv_dupl_similar_funcs;
   finder->progress     = 0.0;
   finder->pos          = 0;
   finder->len          = 0;
   finder->pairs_found  = 0;
   finder->refresh_rate = 1000;
   finder->timer_rate   = 10;
   finder->timer_id     = 0;
   finder->idle_id      = 0;
}


GimvDuplFinder *
gimv_dupl_finder_new (const gchar *type)
{
   GimvDuplFinder *finder;

   finder = GIMV_DUPL_FINDER (g_object_new (GIMV_TYPE_DUPL_FINDER, NULL));
   gimv_dupl_finder_set_algol_type (finder, type);

   return finder;
}


static void
gimv_dupl_finder_dispose (GObject *object)
{
   GimvDuplFinder *finder = GIMV_DUPL_FINDER (object);

   gimv_dupl_finder_stop (finder);

   g_list_foreach (finder->src_list,  (GFunc) g_object_unref, NULL);
   g_list_foreach (finder->dest_list, (GFunc) g_object_unref, NULL);
   g_list_free (finder->src_list);
   g_list_free (finder->dest_list);
   finder->src_list  = NULL;
   finder->dest_list = NULL;

   if (G_OBJECT_CLASS (gimv_dupl_finder_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_dupl_finder_parent_class)->dispose (object);
}


void
gimv_dupl_finder_set_algol_type (GimvDuplFinder *finder, const gchar *type)
{
   gint i, num = sizeof (duplicates_funcs_table) / sizeof (GimvDuplCompFuncTable *);

   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));

   finder->funcs = NULL;

   if (!type || !*type) {
      finder->funcs = duplicates_funcs_table[0];
      return;
   }

   for (i = 0; i < num; i++) {
      GimvDuplCompFuncTable *funcs = duplicates_funcs_table[i];

      if (!funcs->label || !*funcs->label) continue;
      if (!strcmp (funcs->label, type)) {
         finder->funcs = funcs;
         return;
      }
   }

   finder->funcs = duplicates_funcs_table[0];
}


void
gimv_dupl_finder_append_src (GimvDuplFinder *finder,
                             GimvThumb *thumb)
{
   g_return_if_fail (GIMV_DUPL_FINDER (finder));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   g_object_ref(G_OBJECT(thumb));
   finder->src_list = g_list_append (finder->src_list, thumb);
}


void
gimv_dupl_finder_append_dest (GimvDuplFinder *finder,
                              GimvThumb *thumb)
{
   g_return_if_fail (GIMV_DUPL_FINDER (finder));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   g_object_ref(G_OBJECT(thumb));
   finder->dest_list = g_list_append (finder->dest_list, thumb);
}


static void
free_image_sim (gpointer key, gpointer value, gpointer data)
{
   GimvDuplFinder *finder = data;

   if (finder->funcs->data_delete)
      finder->funcs->data_delete (value);
}


gboolean
timeout_duplicates_find (gpointer data)
{
   GimvDuplFinder *finder = data;

   finder->timer_id = 0;
   finder->idle_id  = gtk_idle_add (idle_duplicates_find, data);

   return FALSE;
}


gboolean
idle_duplicates_find (gpointer user_data)
{
   GimvDuplFinder *finder = user_data;
   gint i;

   if (!GIMV_IS_DUPL_FINDER (finder)) return FALSE;
   if (!finder->table) goto STOP;
   if (!finder->cur1 || !finder->cur2) goto STOP;

   for (i = 0; i < finder->refresh_rate; i++) {
      GimvThumb *thumb1, *thumb2;
      gpointer data1, data2;
      gfloat similar = 0.0;

      thumb1 = finder->cur1->data;
      thumb2 = finder->cur2->data;

      data1 = g_hash_table_lookup (finder->table, thumb1);
      if (!data1) {
         if (finder->funcs->get_data)
            data1 = finder->funcs->get_data (thumb1);
         else
            data1 = thumb1;
         g_hash_table_insert (finder->table, thumb1, data1);
      }
      data2 = g_hash_table_lookup (finder->table, thumb2);
      if (!data2) {
         if (finder->funcs->get_data)
            data2 = finder->funcs->get_data (thumb2);
         else
            data2 = thumb2;
         g_hash_table_insert (finder->table, thumb2, data2);
      }

      if (!finder->funcs->compare (data1, data2, &similar)) {
         GimvDuplPair pair;

         pair.thumb1 = thumb1;
         pair.thumb2 = thumb2;
         pair.similarity = similar;

         finder->pairs_found++;

         g_signal_emit (G_OBJECT (finder),
                        gimv_dupl_finder_signals[FOUND_SIGNAL], 0,
                        &pair);
      }

      finder->pos++;
      finder->progress = (gfloat) finder->pos / (gfloat) finder->len;

      finder->cur2 = g_list_next (finder->cur2);
      if (!finder->cur2) {
         finder->cur1 = g_list_next (finder->cur1);
         finder->cur2 = g_list_next (finder->cur1);
      }
      if (!finder->cur1 || !finder->cur2) goto STOP;
   }

   g_signal_emit (G_OBJECT (finder),
                  gimv_dupl_finder_signals[PROGRESS_UPDATE_SIGNAL], 0);

   finder->timer_id  = gtk_timeout_add (finder->timer_rate,
                                        idle_duplicates_find, finder);
   finder->idle_id = 0;

   return FALSE;

STOP:
   gimv_dupl_finder_stop (finder);

   return FALSE;
}


void
gimv_dupl_finder_start (GimvDuplFinder *finder)
{
   gint num;

   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));

   g_signal_emit (G_OBJECT (finder), gimv_dupl_finder_signals[START_SIGNAL], 0);

   if (!finder->table)
      finder->table = g_hash_table_new (g_direct_hash, g_direct_equal);

   finder->cur1 = finder->dest_list;
   finder->cur2 = g_list_next (finder->dest_list);

   num = g_list_length (finder->dest_list);
   finder->len = num * (num - 1) / 2;
   finder->pos = 0;
   finder->pairs_found = 0;

   finder->timer_id = gtk_idle_add (idle_duplicates_find, finder);
}


void
gimv_dupl_finder_stop (GimvDuplFinder *finder)
{
   g_return_if_fail (GIMV_IS_DUPL_FINDER (finder));

   if (finder->timer_id)
      gtk_timeout_remove (finder->timer_id);
   finder->timer_id = 0;

   if (finder->idle_id)
      gtk_idle_remove (finder->idle_id);
   finder->idle_id = 0;

   finder->progress = 0.0;
   finder->pos      = 0;

   if (finder->table) {
      g_hash_table_foreach (finder->table, (GHFunc) free_image_sim, finder);
      g_hash_table_destroy (finder->table);
      finder->table = NULL;
   }

   g_signal_emit (G_OBJECT (finder),
                  gimv_dupl_finder_signals[STOP_SIGNAL], 0);
}


gfloat
gimv_dupl_finder_get_progress (GimvDuplFinder *finder)
{
   g_return_val_if_fail (GIMV_IS_DUPL_FINDER (finder), 0.0);
   return finder->progress;
}

