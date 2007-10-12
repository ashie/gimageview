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

/*
 * These codes are taken from gThumb.
 * gThumb code Copyright (C) 2001 The Free Software Foundation, Inc.
 * gThumb author: Paolo Bacchilega
 */

#ifndef __GIMV_PANED_H__
#define __GIMV_PANED_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef USE_NORMAL_PANED

#define GIMV_TYPE_PANED                  (gimv_paned_get_type ())
#define GIMV_PANED(obj)                  (GTK_CHECK_CAST ((obj), GIMV_TYPE_PANED, GimvPaned))
#define GIMV_PANED_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GIMV_TYPE_PANED, GimvPanedClass))
#define GIMV_IS_PANED(obj)               (GTK_CHECK_TYPE ((obj), GIMV_TYPE_PANED))
#define GIMV_IS_PANED_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_PANED))
	
typedef struct _GimvPaned       GimvPaned;
typedef struct _GimvPanedClass  GimvPanedClass;

struct _GimvPaned
{
   GtkContainer container;

   GtkWidget *child1;
   GtkWidget *child2;

   GdkWindow *handle;
   GdkGC *xor_gc;

   /*< public >*/
   guint16 gutter_size;

   /*< private >*/
   gint child1_size;
   gint last_allocation;
   gint min_position;
   gint max_position;

   guint position_set : 1;
   guint in_drag : 1;
   guint child1_shrink : 1;
   guint child1_resize : 1;
   guint child2_shrink : 1;
   guint child2_resize : 1;

   gint16 handle_xpos;
   gint16 handle_ypos;

   /* whether it is an horizontal or a vertical paned. */
   guint horizontal : 1;    

   /* minimal sizes for child1 and child2. (default value : 0) */
   gint child1_minsize;
   gint child2_minsize;

   /* whether the minimal size option is enabled or not. */
   guint child1_use_minsize : 1;
   guint child2_use_minsize : 1;

   /* stores what child is hiden, if any: 
    * 0   : no child collapsed.
    * 1,2 : index of the collapsed child. */
   guint child_hidden : 2;
};

struct _GimvPanedClass
{
   GtkContainerClass parent_class;

   void (*xor_line) (GimvPaned *);
};


GtkType gimv_paned_get_type        (void);
void    gimv_paned_add1            (GimvPaned *paned,
                                    GtkWidget *child);
void    gimv_paned_add2            (GimvPaned *paned,
                                    GtkWidget *child);
void    gimv_paned_pack1           (GimvPaned *paned,
                                    GtkWidget *child,
                                    gboolean   resize,
                                    gboolean   shrink);
void    gimv_paned_pack2           (GimvPaned *paned,
                                    GtkWidget *child,
                                    gboolean   resize,
                                    gboolean   shrink);
void    gimv_paned_set_position    (GimvPaned *paned,
                                    gint       position);
gint    gimv_paned_get_position    (GimvPaned *paned);

void    gimv_paned_set_gutter_size (GimvPaned *paned,
                                    guint16    size);

void    gimv_paned_xor_line        (GimvPaned *paned);

/* Set a minimal size for a child. Unset the collapse option if setted. */
void    gimv_paned_child1_use_minsize     (GimvPaned *paned,
                                           gboolean   use_minsize,
                                           gint       minsize);
void    gimv_paned_child2_use_minsize     (GimvPaned *paned,
                                           gboolean   use_minsize,
                                           gint       minsize);

void    gimv_paned_hide_child1            (GimvPaned *paned);
void    gimv_paned_hide_child2            (GimvPaned *paned);
void    gimv_paned_split                  (GimvPaned *paned);
guint   gimv_paned_which_hidden           (GimvPaned *paned);

/* Internal function */
void    gimv_paned_compute_position (GimvPaned *paned,
                                     gint       allocation,
                                     gint       child1_req,
                                     gint       child2_req);

#define gray50_width 2
#define gray50_height 2
extern char gray50_bits[];

#else /* USE_NORMAL_PANED */

#include <gtk/gtkpaned.h>

#define GimvPaned GtkPaned
#define GIMV_TYPE_PANED                      GTK_TYPE_PANED
#define GIMV_PANED(paned)                    GTK_PANED(paned)
#define GIMV_IS_PANED(obj)                   GTK_IS_PANED(paned)
#define GIMV_IS_PANED_CLASS(klass)           GTK_IS_PANED_CLASS(paned)
#define gimv_paned_add1(paned, widget)       gtk_paned_add1(paned, widget) 
#define gimv_paned_add2(paned, widget)       gtk_paned_add2(paned, widget) 
#define gimv_paned_set_position(paned, size) gtk_paned_set_position(paned, size)
#define gimv_paned_get_position(paned)       gtk_paned_get_position(paned)
#define gimv_paned_split(paned) \
{ \
   gtk_widget_show (paned->child1); \
   gtk_widget_show (paned->child2); \
}
#define gimv_paned_hide_child1(paned)        gtk_widget_hide (paned->child1)
#define gimv_paned_hide_child2(paned)        gtk_widget_hide (paned->child2)

guint gimv_paned_which_hidden (GimvPaned *paned);

#endif /* USE_NORMAL_PANED */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMV_PANED_H__ */
