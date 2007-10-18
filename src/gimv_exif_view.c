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

#include "gimageview.h"

#ifdef ENABLE_EXIF

#include <libexif/jpeg-data.h>
#include <libexif/exif-data.h>

#include "gimv_exif_view.h"
#include "gimv_image.h"
#include "gimv_io_mem.h"
#include "gtkutils.h"
#include "gimv_icon_stock.h"
#include "gimv_image_loader.h"


typedef enum {
   COLUMN_TERMINATOR = -1,
   COLUMN_KEY,
   COLUMN_VALUE,
   N_COLUMN
} ListStoreColumn;


/******************************************************************************
 *
 *   Callback Functions.
 *
 ******************************************************************************/
static void
cb_exif_view_destroy (GtkWidget *widget, GimvExifView *ev)
{
   g_return_if_fail (ev);

   if (ev->exif_data)
      exif_data_unref (ev->exif_data);
   ev->exif_data = NULL;

   if (ev->jpeg_data)
      jpeg_data_unref (ev->jpeg_data);
   ev->jpeg_data = NULL;

   g_free (ev);
}


static void
cb_exif_window_close (GtkWidget *button, GimvExifView *ev)
{
   g_return_if_fail (ev);

   gtk_widget_destroy (ev->window);
}


/******************************************************************************
 *
 *   Other Private Functions.
 *
 ******************************************************************************/
static void
gimv_exif_view_content_list_set_data (GtkWidget *clist,
                                      ExifContent *content)
{
   const gchar *text[2];
   guint i;
   GtkTreeModel *model;

   g_return_if_fail (clist);
   g_return_if_fail (content);

   model = gtk_tree_view_get_model (GTK_TREE_VIEW (clist));
   gtk_list_store_clear (GTK_LIST_STORE (model));

   for (i = 0; i < content->count; i++) {
      GtkTreeModel *model;
      GtkTreeIter iter;

      text[0] = exif_tag_get_name (content->entries[i]->tag);
      if (text[0] && *text[0]) text[0] = _(text[0]);
      text[1] = exif_entry_get_value (content->entries[i]);
      if (text[1] && *text[1]) text[1] = _(text[1]);

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (clist));

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_KEY,       text[0],
                          COLUMN_VALUE,     text[1],
                          COLUMN_TERMINATOR);
   }
}


/******************************************************************************
 *
 *   Public Functions.
 *
 ******************************************************************************/
GimvExifView *
gimv_exif_view_create_window (const gchar *filename, GtkWindow *parent)
{
   GimvExifView *ev;
   GtkWidget *button;
   gchar buf[BUF_SIZE];

   g_return_val_if_fail (filename && *filename, NULL);

   ev = gimv_exif_view_create (filename, parent);
   if (!ev) return NULL;

   ev->window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (ev->window), parent);
   g_snprintf (buf, BUF_SIZE, _("%s EXIF data"), filename);
   gtk_window_set_title (GTK_WINDOW (ev->window), buf); 
   gtk_window_set_default_size (GTK_WINDOW (ev->window), 500, 400);
   gtk_window_set_position (GTK_WINDOW (ev->window), GTK_WIN_POS_CENTER);

   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ev->window)->vbox),
                       ev->container,
                       TRUE, TRUE, 0);

   gtk_widget_show_all (ev->window);

   /* button */
   button = gtk_button_new_with_label (_("Close"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ev->window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_exif_window_close), ev);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   gtk_widget_grab_focus (button);

   gimv_icon_stock_set_window_icon (ev->window->window, "gimv_icon");

   return ev;
}


static GtkWidget *
gimv_exif_view_get_thumbnail (ExifData *edata)
{
   GtkWidget *image;
   GimvImageLoader *loader;
   GimvIO *gio;
   GimvImage *gimvimage;
   GdkPixmap *pixmap = NULL;
   GdkBitmap *bitmap = NULL;


   g_return_val_if_fail (edata, NULL);

   if (!edata->data) return NULL;
   if (edata->size <= 0) return NULL;

   loader = gimv_image_loader_new ();
   if (!loader) return NULL;

   gio = gimv_io_mem_new (NULL, "rb", GimvIOMemModeWrap);
   gimv_io_mem_wrap ((GimvIOMem *) gio, edata->data, edata->size, FALSE);
   gimv_image_loader_set_gio (loader, gio);

   gimv_image_loader_load (loader);

   gimvimage = gimv_image_loader_get_image (loader);
   if (!gimvimage) {
      g_object_unref (G_OBJECT(loader));
      return NULL;
   }

   gimv_image_scale_get_pixmap (gimvimage,
                                gimv_image_width (gimvimage),
                                gimv_image_height (gimvimage),
                                &pixmap, &bitmap);

   g_object_unref (G_OBJECT (loader));
   gimv_io_unref (gio);

   image = gtk_image_new_from_pixmap (pixmap, bitmap);   

   if (pixmap)
      gdk_pixmap_unref (pixmap);

   return image;
}


GimvExifView *
gimv_exif_view_create (const gchar *filename, GtkWindow *parent)
{
   JPEGData *jdata;
   ExifData *edata;
   GimvExifView *ev = NULL;
   ExifContent *contents[EXIF_IFD_COUNT];
   GtkWidget *notebook, *label;
   GtkWidget *vbox, *pixmap;
   gint i;

   gchar *titles[] = {
      N_("Tag"), N_("Value"),
   };

   g_return_val_if_fail (filename && *filename, NULL);

   jdata = jpeg_data_new_from_file (filename);
   if (!jdata) {
      gtkutil_message_dialog (_("Error!!"), _("EXIF data not found."),
                              GTK_WINDOW (ev->window));
      return NULL;
   }

   edata = jpeg_data_get_exif_data (jdata);
   if (!edata) {
      gtkutil_message_dialog (_("Error!!"), _("EXIF data not found."),
                              GTK_WINDOW (parent));
      goto ERROR;
   }

   ev = g_new0 (GimvExifView, 1);
   ev->exif_data = edata;
   ev->jpeg_data = jdata;

#if 0
   contents[0] = edata->ifd0;
   contents[1] = edata->ifd1;
   contents[2] = edata->ifd_exif;
   contents[3] = edata->ifd_gps;
   contents[4] = edata->ifd_interoperability;
#else
   for (i = 0; i < EXIF_IFD_COUNT; i++)
      contents[i] = edata->ifd[i];
#endif

   ev->container = gtk_vbox_new (FALSE, 0);
   g_signal_connect (G_OBJECT (ev->container), "destroy",
                     G_CALLBACK (cb_exif_view_destroy), ev);
   gtk_widget_show (ev->container);

   notebook = gtk_notebook_new ();
   gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
   gtk_box_pack_start(GTK_BOX(ev->container), notebook, TRUE, TRUE, 0);
   gtk_widget_show (notebook);

   /* Tag Tables */
   for (i = 0; i < EXIF_IFD_COUNT; i++) {
      GtkWidget *scrolledwin, *clist;
      GtkListStore *store;
      GtkTreeViewColumn *col;
      GtkCellRenderer *render;

      /* scrolled window & clist */
      label = gtk_label_new (_(exif_ifd_get_name(i)));
      scrolledwin = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledwin),
                                      GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
      gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
                                          GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (scrolledwin), 5);
      gtk_notebook_append_page (GTK_NOTEBOOK(notebook),
                                scrolledwin, label);
      gtk_widget_show (scrolledwin);

      store = gtk_list_store_new (N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
      clist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

      gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (clist), TRUE);

      /* set column for key */
      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_resizable (col, TRUE);
      gtk_tree_view_column_set_title (col, _(titles[0]));
      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_KEY);
      gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

      /* set column for value */
      col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_resizable (col, TRUE);
      gtk_tree_view_column_set_title (col, _(titles[1]));
      render = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, render, TRUE);
      gtk_tree_view_column_add_attribute (col, render, "text", COLUMN_VALUE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (clist), col);

      gtk_container_add (GTK_CONTAINER (scrolledwin), clist);
      gtk_widget_show (clist);

      gimv_exif_view_content_list_set_data (clist, contents[i]);
   }

   /* Thumbnail page */
   label = gtk_label_new (_("Thumbnail"));
   vbox = gtk_vbox_new (TRUE, 0);
   gtk_notebook_append_page (GTK_NOTEBOOK(notebook),
                             vbox, label);
   gtk_widget_show (vbox);

   pixmap = gimv_exif_view_get_thumbnail (edata);

   if (pixmap)
      gtk_box_pack_start (GTK_BOX (vbox), pixmap, TRUE, TRUE, 0);

   return ev;

ERROR:
   jpeg_data_unref (jdata);
   return NULL;
}

#endif /* ENABLE_EXIF */
