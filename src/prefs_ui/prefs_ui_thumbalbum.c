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

#include "gimv_prefs_ui_utils.h"
#include "gtkutils.h"
#include "intl.h"
#include "prefs.h"
#include <prefs_ui/prefs_ui_thumbalbum.h>

extern Config *config_changed;

GtkWidget *
prefs_ui_thumbalbum (void)
{
   GtkWidget *main_vbox;
   GtkWidget *frame, *frame_vbox, *table, *hbox;
   GtkWidget *label;
   GtkAdjustment *adj;
   GtkWidget *spinner;

   main_vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 5);

   /********************************************** 
    * Thumbnail Album Frame
    **********************************************/
   gimv_prefs_ui_create_frame (_("Album"),
                               frame, frame_vbox, main_vbox, FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);

   table = gtk_table_new (2, 4, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 5);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);
   gtk_container_set_border_width(GTK_CONTAINER(table), 5);
   gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

   /* Row Spacing spinner */
   label = gtk_label_new (_("Row Spacing"));
   gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                     GTK_EXPAND, GTK_FILL, 0, 0);

   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbalbum_row_space,
                                               0.0, 255.0, 1.0, 5.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                       GTK_SIGNAL_FUNC (gtkutil_get_data_from_adjustment_by_int_cb),
                       &config_changed->thumbalbum_row_space);
   gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

   /* Column Spacing spinner */
   label = gtk_label_new (_("Column Spacing"));
   gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                     GTK_EXPAND, GTK_FILL, 0, 0);

   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbalbum_col_space,
                                               0.0, 255.0, 1.0, 5.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                       GTK_SIGNAL_FUNC (gtkutil_get_data_from_adjustment_by_int_cb),
                       &config_changed->thumbalbum_col_space);
   gtk_table_attach (GTK_TABLE (table), spinner, 3, 4, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

   gtk_widget_show_all (main_vbox);

   return main_vbox;
}
