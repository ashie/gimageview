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

#ifndef __GIMV_DUPL_FINDER_H__
#define __GIMV_DUPL_FINDER_H__

#include "gimageview.h"

#include <glib.h>

#define GIMV_TYPE_DUPL_FINDER            (gimv_dupl_finder_get_type ())
#define GIMV_DUPL_FINDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_DUPL_FINDER, GimvDuplFinder))
#define GIMV_DUPL_FINDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_DUPL_FINDER, GimvDuplFinderClass))
#define GIMV_IS_DUPL_FINDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_DUPL_FINDER))
#define GIMV_IS_DUPL_FINDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_DUPL_FINDER))
#define GIMV_DUPL_FINDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_DUPL_FINDER, GimvDuplFinderClass))

typedef struct GimvDuplFinder_Tag         GimvDuplFinder;
typedef struct GimvDuplFinderPriv_Tag     GimvDuplFinderPriv;
typedef struct GimvDuplFinderClass_Tag    GimvDuplFinderClass;
typedef struct GimvDuplPair_Tag           GimvDuplPair;
typedef struct GimvDuplCompFuncTable_Tag  GimvDuplCompFuncTable;

struct GimvDuplFinder_Tag
{
   GObject parent;

   GList *src_list, *dest_list;
   GList *cur1, *cur2;

   GHashTable *table;

   GimvDuplCompFuncTable *funcs;

   gfloat progress;
   gint   len, pos;

   gint   pairs_found;

   gint   refresh_rate;
   gint   timer_rate;

   guint  timer_id;
   guint  idle_id;
};

struct GimvDuplFinderClass_Tag
{
   GObjectClass parent_class;

   void (*start)           (GimvDuplFinder *finder);
   void (*stop)            (GimvDuplFinder *finder);
   void (*progress_update) (GimvDuplFinder *finder);
   void (*found)           (GimvDuplFinder *finder,
                            GimvDuplPair   *pair);
};

struct GimvDuplPair_Tag
{
   GimvThumb *thumb1;
   GimvThumb *thumb2;
   gfloat similarity;
};

struct GimvDuplCompFuncTable_Tag
{
   const gchar * const label;

   gpointer (*get_data)    (GimvThumb *thumb);
   gint     (*compare)     (gpointer   data1,
                            gpointer   data2,
                            gfloat    *similarity);
   void     (*data_delete) (gpointer   data);
};

GType         gimv_dupl_finder_get_type           (void);

const gchar **gimv_dupl_finder_get_algol_types    (void);

GimvDuplFinder *
              gimv_dupl_finder_new                (const gchar *type);

void          gimv_dupl_finder_set_algol_type     (GimvDuplFinder *finder,
                                                   const gchar    *type);
/* souce doesn't work yet */
void          gimv_dupl_finder_append_source      (GimvDuplFinder *finder,
                                                   GimvThumb      *thumb1);
void          gimv_dupl_finder_append_src         (GimvDuplFinder *finder,
                                                   GimvThumb      *thumb);
void          gimv_dupl_finder_append_dest        (GimvDuplFinder *finder,
                                                   GimvThumb      *thumb1);
void          gimv_dupl_finder_start              (GimvDuplFinder *finder);
void          gimv_dupl_finder_stop               (GimvDuplFinder *finder);
gfloat        gimv_dupl_finder_get_progress       (GimvDuplFinder *finder);

#endif /* __GIMV_DUPL_FINDER_H__ */
