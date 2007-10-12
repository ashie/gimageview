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
#include "gtk2-compat.h"
#include "prefs.h"
#include "gimv_dupl_win.h"


typedef enum {
   START_SIGNAL,
   STOP_SIGNAL,
   PROGRESS_UPDATE_SIGNAL,
   FOUND_SIGNAL,
   LAST_SIGNAL
} GimvDuplFinderSignalType;


static void gimv_dupl_finder_class_init (GimvDuplFinderClass *klass);
static void gimv_dupl_finder_init       (GimvDuplFinder      *finder);
static void gimv_dupl_finder_destroy    (GtkObject             *object);

gboolean idle_duplicates_find    (gpointer user_data);
gboolean timeout_duplicates_find (gpointer data);


static GtkObjectClass *parent_class = NULL;
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


GtkType
gimv_dupl_finder_get_type (void)
{
   static GtkType gimv_dupl_finder_type = 0;

   if (!gimv_dupl_finder_type) {
      static const GtkTypeInfo gimv_dupl_finder_info = {
         "GimvDuplFinder",
         sizeof (GimvDuplFinder),
         sizeof (GimvDuplFinderClass),
         (GtkClassInitFunc) gimv_dupl_finder_class_init,
         (GtkObjectInitFunc) gimv_dupl_finder_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_dupl_finder_type = gtk_type_unique (gtk_object_get_type (),
                                                &gimv_dupl_finder_info);
   }

   return gimv_dupl_finder_type;
}


static void
gimv_dupl_finder_class_init (GimvDuplFinderClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) klass;
   parent_class = gtk_type_class (gtk_object_get_type ());

   gimv_dupl_finder_signals[START_SIGNAL]
      = gtk_signal_new ("start",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvDuplFinderClass, start),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_dupl_finder_signals[STOP_SIGNAL]
      = gtk_signal_new ("stop",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvDuplFinderClass, stop),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_dupl_finder_signals[PROGRESS_UPDATE_SIGNAL]
      = gtk_signal_new ("progress_update",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvDuplFinderClass,
                                           progress_update),
                        gtk_signal_default_marshaller,
                        GTK_TYPE_NONE, 0);

   gimv_dupl_finder_signals[FOUND_SIGNAL]
      = gtk_signal_new ("found",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvDuplFinderClass, found),
                        gtk_marshal_NONE__POINTER,
                        GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

   gtk_object_class_add_signals (object_class,
                                 gimv_dupl_finder_signals, LAST_SIGNAL);

   object_class->destroy = gimv_dupl_finder_destroy;

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

   gtk_object_ref (GTK_OBJECT (finder));
   gtk_object_sink (GTK_OBJECT (finder));
}


GimvDuplFinder *
gimv_dupl_finder_new (const gchar *type)
{
   GimvDuplFinder *finder;

   finder = GIMV_DUPL_FINDER (gtk_type_new (gimv_dupl_finder_get_type ()));
   gimv_dupl_finder_set_algol_type (finder, type);

   return finder;
}


static void
gimv_dupl_finder_destroy (GtkObject *object)
{
   GimvDuplFinder *finder = GIMV_DUPL_FINDER (object);

   gimv_dupl_finder_stop (finder);

   g_list_foreach (finder->src_list,  (GFunc) gtk_object_unref, NULL);
   g_list_foreach (finder->dest_list, (GFunc) gtk_object_unref, NULL);
   g_list_free (finder->src_list);
   g_list_free (finder->dest_list);
   finder->src_list  = NULL;
   finder->dest_list = NULL;

   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

   gtk_object_ref(GTK_OBJECT(thumb));
   finder->src_list = g_list_append (finder->src_list, thumb);
}


void
gimv_dupl_finder_append_dest (GimvDuplFinder *finder,
                              GimvThumb *thumb)
{
   g_return_if_fail (GIMV_DUPL_FINDER (finder));
   g_return_if_fail (GIMV_IS_THUMB (thumb));

   gtk_object_ref(GTK_OBJECT(thumb));
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

         gtk_signal_emit (GTK_OBJECT (finder),
                          gimv_dupl_finder_signals[FOUND_SIGNAL],
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

   gtk_signal_emit (GTK_OBJECT (finder),
                    gimv_dupl_finder_signals[PROGRESS_UPDATE_SIGNAL]);

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

   gtk_signal_emit (GTK_OBJECT (finder), gimv_dupl_finder_signals[START_SIGNAL]);

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

   gtk_signal_emit (GTK_OBJECT (finder),
                    gimv_dupl_finder_signals[STOP_SIGNAL]);
}


gfloat
gimv_dupl_finder_get_progress (GimvDuplFinder *finder)
{
   g_return_val_if_fail (GIMV_IS_DUPL_FINDER (finder), 0.0);
   return finder->progress;
}

