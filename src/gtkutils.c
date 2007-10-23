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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "utils_auto_comp.h"
#include "charset.h"
#include "fileutil.h"
#include "gtkutils.h"
#include "gimv_icon_stock.h"
#include "prefs.h"

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

/* callback functions for confirm dialog */
static gint cb_dummy                 (GtkWidget   *button,
                                      gpointer     data);
static void cb_confirm_yes           (GtkWidget   *button,
                                      ConfirmType *type);
static void cb_confirm_yes_to_all    (GtkWidget   *button,
                                      ConfirmType *type);
static void cb_confirm_no            (GtkWidget   *button,
                                      ConfirmType *type);
static void cb_confirm_cancel        (GtkWidget   *button,
                                      ConfirmType *type);

/* callback functions for message dialog */
static void cb_message_dialog_quit   (GtkWidget   *button,
                                      gpointer     data);

/* callback functions for progress bar window */
static void cb_progress_win_cancel   (GtkWidget   *button,
                                      gboolean    *cancel_pressed);

/* callback functions for text entry window */
static void cb_textpop_enter         (GtkWidget   *button,
                                      gboolean    *ok_pressd);
static void cb_textpop_ok_button     (GtkWidget   *button,
                                      gboolean    *ok_pressd);
static void cb_textpop_cancel_button (GtkWidget   *button,
                                      gboolean    *ok_pressd);



/******************************************************************************
 *
 *   misc
 *
 ******************************************************************************/
const gchar *
boolean_to_text (gboolean boolval)
{
   if (boolval)
      return "TRUE";
   else
      return "FALSE";
}


gboolean
text_to_boolean (gchar *text)
{
   g_return_val_if_fail (text && *text, FALSE);

   if (!g_strcasecmp (text, "TRUE") || !g_strcasecmp (text, "ENABLE"))
      return TRUE;
   else
      return FALSE;
}


void
gtkutil_get_widget_area (GtkWidget    *widget,
                         GdkRectangle *area)
{
   g_return_if_fail (widget);
   g_return_if_fail (area);

   area->x       = widget->allocation.x;
   area->y       = widget->allocation.y;
   area->width   = widget->allocation.width;
   area->height  = widget->allocation.height;

   /* FIXME? */
   area->x = 0;
   area->y = 0;
   /* END FIXME? */
}


/*
 *  create_toggle_button:
 *     @ Create toggle button widget.
 *
 *  label   : Label text for toggle button.
 *  def_val : Default value.
 *  Return  : Toggle button widget.
 */
GtkWidget *
gtkutil_create_check_button (const gchar *label_text, gboolean def_val,
                             gpointer func, gpointer data)
{
   GtkWidget *toggle;

   toggle = gtk_check_button_new_with_label (_(label_text));
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(toggle), def_val);

   if (func)
      g_signal_connect (G_OBJECT (toggle), "toggled",
                        G_CALLBACK (func), data);

   return toggle;
}


GtkWidget *
gtkutil_create_toolbar (void)
{
   GtkWidget *toolbar;

   toolbar = gtk_toolbar_new ();

   return toolbar;
}


GtkWidget *
gtkutil_create_spin_button (GtkAdjustment *adj)
{
   GtkWidget *spinner = gtk_spin_button_new (adj, 0, 0);
   gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);

   return spinner;
}


GtkWidget *
gtkutil_option_menu_get_current (GtkWidget *option_menu)
{
   g_return_val_if_fail (GTK_IS_OPTION_MENU (option_menu), NULL);

   {
      GtkWidget *menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
      gint nth = gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu));
      GList *node = g_list_nth (GTK_MENU_SHELL (menu)->children, nth);
      if (!node) return NULL;
      return node->data;
   }
}


GList *
gtkutil_list_insert_sorted (GList        *list,
                            gpointer      data,
                            GCompareFunc  func,
                            gboolean      reverse)
{
   GList *tmp_list = list;
   GList *new_list;
   gint cmp;

   g_return_val_if_fail (func != NULL, list);

   if (!list) {
      new_list = g_list_alloc();
      new_list->data = data;
      return new_list;
   }

   cmp = (*func) (data, tmp_list->data);

   while ((tmp_list->next) &&
          ((!reverse && cmp > 0) || (reverse && cmp <= 0)))
   {
      tmp_list = tmp_list->next;
      cmp = (*func) (data, tmp_list->data);
   }

   new_list = g_list_alloc();
   new_list->data = data;

   if ((!tmp_list->next) && (cmp > 0)) {
      tmp_list->next = new_list;
      new_list->prev = tmp_list;
      return list;
   }

   if (tmp_list->prev) {
      tmp_list->prev->next = new_list;
      new_list->prev = tmp_list->prev;
   }
   new_list->next = tmp_list;
   tmp_list->prev = new_list;

   if (tmp_list == list)
      return new_list;
   else
      return list;
}


guint
gtkutil_paned_which_is_hidden (GtkPaned *paned)
{
   g_return_val_if_fail (GTK_IS_PANED (paned), 0);
   g_return_val_if_fail (paned->child1 && paned->child2, 0);

   if (!GTK_WIDGET_VISIBLE (paned->child1)
       && GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 1;

   } else if (GTK_WIDGET_VISIBLE (paned->child1)
              && !GTK_WIDGET_VISIBLE (paned->child2))
   {
      return 2;

   } else {
      return 0;
   }
}



/******************************************************************************
 *
 *   Confirm Dialog Window
 *
 ******************************************************************************/
static gint
cb_dummy (GtkWidget *button, gpointer data)
{
   return TRUE;
}


static void
cb_confirm_yes (GtkWidget *button, ConfirmType *type)
{
   *type = CONFIRM_YES;
   gtk_main_quit ();
}


static void
cb_confirm_yes_to_all (GtkWidget *button, ConfirmType *type)
{
   *type = CONFIRM_YES_TO_ALL;
   gtk_main_quit ();
}


static void
cb_confirm_no (GtkWidget *button, ConfirmType *type)
{
   *type = CONFIRM_NO;
   gtk_main_quit ();
}


static void
cb_confirm_no_to_all (GtkWidget *button, ConfirmType *type)
{
   *type = CONFIRM_NO_TO_ALL;
   gtk_main_quit ();
}


static void
cb_confirm_cancel (GtkWidget *button, ConfirmType *type)
{
   *type = CONFIRM_CANCEL;
   gtk_main_quit ();
}


ConfirmType
gtkutil_confirm_dialog (const gchar *title, const gchar *message,
                        ConfirmDialogFlags flags, GtkWindow *parent)
{
   ConfirmType retval = CONFIRM_NO;
   GtkWidget *window;
   GtkWidget *vbox, *hbox, *button, *label;
   GtkWidget *icon;

   window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (window), parent);
   gtk_window_set_title (GTK_WINDOW (window), title); 
   gtk_window_set_default_size (GTK_WINDOW (window), 300, 120);
   /* gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), 5); */
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   g_signal_connect (G_OBJECT (window), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   /* message area */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 15);
   gtk_widget_show (vbox);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_widget_show (hbox);

   /* icon */
   icon = gimv_icon_stock_get_widget ("question");
   gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
   gtk_widget_show (icon);

   /* message */
   label = gtk_label_new (message);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
   gtk_widget_show (label);

   /* buttons */
   button = gtk_button_new_with_label (_("Yes"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_yes),
                     &retval);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);
   gtk_widget_grab_focus (button);

   if (flags & ConfirmDialogMultipleFlag) {
      button = gtk_button_new_with_label (_("Yes to All"));
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                          button, TRUE, TRUE, 0);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cb_confirm_yes_to_all),
                        &retval);
      GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
      gtk_widget_show (button);
   }

   button = gtk_button_new_with_label (_("No"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_no),
                     &retval);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   if (flags & ConfirmDialogMultipleFlag) {
      button = gtk_button_new_with_label (_("Cancel"));
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                          button, TRUE, TRUE, 0);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cb_confirm_cancel),
                        &retval);
      GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
      gtk_widget_show (button);
   }

   gtk_widget_show (window);

   gtk_grab_add (window);
   gtk_main ();
   gtk_grab_remove (window);
   gtk_widget_destroy (window);

   return retval;
}


#include "gimv_image_view.h"

typedef struct OverWriteDialog_Tag
{
   GimvImageInfo *info1, *info2;
   GtkWidget *window, *iv1, *iv2, *show_compare_button, *compare_area, *entry;
   gchar *new_path;
   gint new_path_len;
   ConfirmType retval;
} OverWriteDialog;


static void
cb_show_compare (GtkButton *button, OverWriteDialog *dialog)
{
   gimv_image_view_change_image (GIMV_IMAGE_VIEW (dialog->iv1), dialog->info1);
   gimv_image_view_change_image (GIMV_IMAGE_VIEW (dialog->iv2), dialog->info2);
}


static void
cb_show_image1 (GtkButton *button, OverWriteDialog *dialog)
{
   gimv_image_view_change_image (GIMV_IMAGE_VIEW (dialog->iv1), dialog->info1);
}



static void
cb_show_image2 (GtkButton *button, OverWriteDialog *dialog)
{
   gimv_image_view_change_image (GIMV_IMAGE_VIEW (dialog->iv2), dialog->info2);
}


static void
overwrite_confirm_rename (OverWriteDialog *dialog)
{
   const gchar *filename_internal
      = g_basename (gtk_entry_get_text (GTK_ENTRY (dialog->entry)));
   gchar *dirname, *filename;

   if (!filename_internal && *filename_internal) return;
   g_return_if_fail (dialog->new_path && dialog->new_path_len > 0);

   dirname = g_dirname (gimv_image_info_get_path (dialog->info1));
   g_return_if_fail (dirname);
   if (!*dirname) g_free (dirname);
   g_return_if_fail (*dirname);

   filename = charset_internal_to_locale (filename_internal);
   g_return_if_fail (filename);
   if (!*filename) g_free (filename);
   g_return_if_fail (*filename);

   g_snprintf (dialog->new_path, dialog->new_path_len, "%s/%s",
               dirname, filename);

   /* check the new path */
   if (file_exists (dialog->new_path)) {
      gchar error_message[BUF_SIZE];
      g_snprintf (error_message, BUF_SIZE,
                  _("The file exists : %s"),
                  dialog->new_path);
      gtkutil_message_dialog (_("Error!!"), error_message,
                              GTK_WINDOW (dialog->window));
      g_free (dirname);
      g_free (filename);
      return;
   }

   g_free (dirname);
   g_free (filename);

   dialog->retval = CONFIRM_NO;

   gtk_main_quit ();
}


static void
cb_confirm_rename (GtkButton *button, OverWriteDialog *dialog)
{
   overwrite_confirm_rename (dialog);
}


static void
cb_confirm_rename_enter (GtkEntry *entry, OverWriteDialog *dialog)
{
   overwrite_confirm_rename (dialog);
}


ConfirmType
gtkutil_overwrite_confirm_dialog (const gchar *title, const gchar *message,
                                  const gchar *dest_file, const gchar *src_file,
                                  gchar *new_path, gint new_path_len,
                                  ConfirmDialogFlags flags,
                                  GtkWindow *parent)
{
   OverWriteDialog dialog;
   GtkWidget *window;
   GtkWidget *vbox, *hbox, *button, *label, *entry;
   GtkWidget *icon;
   GtkWidget *vbox2;
   gchar *filename;

   dialog.retval = CONFIRM_NO;
   dialog.info1 = gimv_image_info_get (dest_file);
   dialog.info2 = gimv_image_info_get (src_file);
   dialog.new_path = new_path;
   dialog.new_path_len = new_path_len;

   dialog.window = window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (window), parent);
   gtk_window_set_title (GTK_WINDOW (window), title); 
   gtk_window_set_default_size (GTK_WINDOW (window), 300, 120);
   /* gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), 5); */
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   g_signal_connect (G_OBJECT (window), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   /* message area */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 15);
   gtk_widget_show (vbox);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_widget_show (hbox);

   /* icon */
   icon = gimv_icon_stock_get_widget ("question");
   gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
   gtk_widget_show (icon);

   /* message */
   label = gtk_label_new (message);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
   gtk_widget_show (label);

   /* compare area */
   {
      /* show image buttons */
      hbox = gtk_hbox_new (TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox),
                          hbox, FALSE, FALSE, 2);
      gtk_widget_show (hbox);

      label = gtk_button_new_with_label (_("Show destination"));
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 2);
      g_signal_connect (G_OBJECT (label), "clicked",
                        G_CALLBACK (cb_show_image1),
                        &dialog);
      gtk_widget_show (label);

      button = gtk_button_new_with_label (_("Show both images"));
      dialog.show_compare_button = button;
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 2);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cb_show_compare),
                        &dialog);
      gtk_widget_show (button);

      label = gtk_button_new_with_label (_("Show source"));
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 2);
      g_signal_connect (G_OBJECT (label), "clicked",
                        G_CALLBACK (cb_show_image2),
                        &dialog);
      gtk_widget_show (label);

      /* view */
      dialog.compare_area = hbox = gtk_hbox_new (TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      /* destination */
      vbox2 = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 2);
      gtk_widget_show (vbox2);

      dialog.iv1 = gimv_image_view_new (NULL);
      g_object_set (G_OBJECT (dialog.iv1),
                    "default_zoom",     3,
                    "default_rotation", 0,
                    "keep_aspect",     TRUE,
                    NULL);
      gimv_image_view_hide_scrollbar (GIMV_IMAGE_VIEW (dialog.iv1));
      gtk_widget_set_size_request (dialog.iv1, -1, 150);
      gtk_box_pack_start (GTK_BOX (vbox2), dialog.iv1, TRUE, TRUE, 0);
      gtk_widget_show (dialog.iv1);

      /* gimv_image_view_set_text (dialog.iv1, information) */


      /* source */
      vbox2 = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 2);
      gtk_widget_show (vbox2);

      dialog.iv2 = gimv_image_view_new (NULL);
      g_object_set (G_OBJECT (dialog.iv2),
                    "default_zoom",     3,
                    "default_rotation", 0,
                    "keep_aspect",     TRUE,
                    NULL);
      gimv_image_view_hide_scrollbar (GIMV_IMAGE_VIEW (dialog.iv2));
      gtk_widget_set_size_request (dialog.iv2, -1, 150);
      gtk_box_pack_start (GTK_BOX (vbox2), dialog.iv2, TRUE, TRUE, 0);
      gtk_widget_show (dialog.iv2);

      /* gimv_image_view_set_text (dialog.iv2, information) */
   }

   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
                       vbox, FALSE, FALSE, 0);
   gtk_widget_show (vbox);

   /* rename entry */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show (hbox);

   dialog.entry = entry = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (entry), "activate",
                     G_CALLBACK (cb_confirm_rename_enter), &dialog);
   filename = charset_to_internal (g_basename (src_file),
                                   conf.charset_filename,
                                   conf.charset_auto_detect_fn,
                                   conf.charset_filename_mode);
   gtk_entry_set_text (GTK_ENTRY (entry), filename);
   g_free (filename);
   filename = NULL;
   gtk_widget_show (entry);

   button = gtk_button_new_with_label (_("Rename"));
   gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_rename),
                     &dialog);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   /* buttons */
   hbox = gtk_hbox_new (TRUE, 0);
   gtk_box_pack_start (GTK_BOX (vbox),
                       hbox, TRUE, TRUE, 0);
   gtk_widget_show (hbox);

   button = gtk_button_new_with_label (_("Yes"));
   gtk_box_pack_start (GTK_BOX (hbox),  button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_yes),
                     &dialog.retval);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);
   gtk_widget_grab_focus (button);

   if (flags & ConfirmDialogMultipleFlag) {
      button = gtk_button_new_with_label (_("Yes to All"));
      gtk_box_pack_start (GTK_BOX (hbox),button, TRUE, TRUE, 0);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cb_confirm_yes_to_all),
                        &dialog.retval);
      GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
      gtk_widget_show (button);
   }

   button = gtk_button_new_with_label (_("Skip"));
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_no),
                     &dialog.retval);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   button = gtk_button_new_with_label (_("Skip all"));
   gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_confirm_no_to_all),
                     &dialog.retval);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   if (flags & ConfirmDialogMultipleFlag) {
      button = gtk_button_new_with_label (_("Cancel"));
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cb_confirm_cancel),
                        &dialog.retval);
      GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
      gtk_widget_show (button);
   }

   gtk_widget_show (window);

   gtk_grab_add (window);
   gtk_main ();
   gtk_grab_remove (window);
   gtk_widget_destroy (window);

   if (dialog.info1) gimv_image_info_unref (dialog.info1);
   if (dialog.info2) gimv_image_info_unref (dialog.info2);

   return dialog.retval;
}



/******************************************************************************
 *
 *   Message Dialog Window
 *
 ******************************************************************************/
static void
cb_message_dialog_quit (GtkWidget *button, gpointer data)
{
   gtk_main_quit ();
}


void
gtkutil_message_dialog (const gchar *title, const gchar *message, GtkWindow *parent)
{
   GtkWidget *window;
   GtkWidget *button, *label, *vbox, *hbox;
   GtkWidget *alert_icon;

   window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (window), parent);
   gtk_window_set_title (GTK_WINDOW (window), title); 
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   g_signal_connect (G_OBJECT (window), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   /* message area */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox,
                       TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 15);
   gtk_widget_show (vbox);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_widget_show (hbox);

   /* icon */
   alert_icon = gimv_icon_stock_get_widget ("alert");
   gtk_box_pack_start (GTK_BOX (hbox), alert_icon, TRUE, TRUE, 0);
   gtk_widget_show (alert_icon);

   /* message */
   label = gtk_label_new (message);
   gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
   gtk_widget_show (label);

   /* button */
   button = gtk_button_new_with_label (_("OK"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_message_dialog_quit), NULL);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   gtk_widget_grab_focus (button);

   gtk_widget_show (window);

   gtk_grab_add (window);
   gtk_main ();
   gtk_grab_remove (window);
   gtk_widget_destroy (window);
}



/******************************************************************************
 *
 *   Progress Bar Window
 *
 ******************************************************************************/
static void
cb_progress_win_cancel (GtkWidget *button, gboolean *cancel_pressed)
{
   *cancel_pressed = TRUE;
}


void
gtkutil_progress_window_update (GtkWidget *window,
                                gchar *title, gchar *message,
                                gchar *progress_text, gfloat progress)
{
   GtkWidget *label;
   GtkWidget *progressbar;

   g_return_if_fail (window);

   label = g_object_get_data (G_OBJECT (window), "label");
   progressbar = g_object_get_data (G_OBJECT (window), "progressbar");

   g_return_if_fail (label && progressbar);

   if (title)
      gtk_window_set_title (GTK_WINDOW (window), _(title));
   if (message)
      gtk_label_set_text (GTK_LABEL (label), message);
   if (progress_text)
      gtk_progress_set_format_string(GTK_PROGRESS (progressbar),
                                     progress_text);
   if (progress > 0.0 && progress < 1.0)
      gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar), progress);
}


GtkWidget *
gtkutil_create_progress_window (gchar *title, gchar *initial_message,
                                gboolean *cancel_pressed,
                                gint width, gint height, GtkWindow *parent)
{
   GtkWidget *window;
   GtkWidget *vbox;
   GtkWidget *label;
   GtkWidget *progressbar;
   GtkWidget *button;

   g_return_val_if_fail (title && initial_message && cancel_pressed, NULL);

   *cancel_pressed = FALSE;

   /* create dialog window */
   window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (window), parent);
   gtk_container_border_width (GTK_CONTAINER (window), 3);
   gtk_window_set_title (GTK_WINDOW (window), title);
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   gtk_window_set_default_size (GTK_WINDOW (window), width, height);
   g_signal_connect (G_OBJECT (window), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   /* message area */
   vbox = gtk_vbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox,
                       TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
   gtk_widget_show (vbox);

   /* label */
   label = gtk_label_new (initial_message);
   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

   /* progress bar */
   progressbar = gtk_progress_bar_new();
   gtk_progress_set_show_text(GTK_PROGRESS(progressbar), TRUE);
   gtk_box_pack_start (GTK_BOX (vbox), progressbar, FALSE, FALSE, 0);

   /* cancel button */
   button = gtk_button_new_with_label (_("Cancel"));
   gtk_container_border_width (GTK_CONTAINER (button), 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
                       TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_progress_win_cancel), cancel_pressed);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);

   g_object_set_data (G_OBJECT (window), "label", label);
   g_object_set_data (G_OBJECT (window), "progressbar", progressbar);

   gtk_widget_show_all (window);

   return window;
}



/******************************************************************************
 *
 *   Text Entry Window
 *
 ******************************************************************************/
static void
cb_textpop_enter (GtkWidget *button, gboolean *ok_pressd)
{
   *ok_pressd = TRUE;
   gtk_main_quit ();
}


static void
cb_textpop_ok_button (GtkWidget *button, gboolean *ok_pressd)
{
   *ok_pressd = TRUE;
   gtk_main_quit ();
}


static void
cb_textpop_cancel_button (GtkWidget *button, gboolean *ok_pressd)
{
   *ok_pressd = FALSE;
   gtk_main_quit ();
}


static gint
cb_textpop_key_press (GtkWidget   *widget, 
                      GdkEventKey *event,
                      gboolean *ok_pressd)
{
   const gchar *path;
   gchar *text;
   gint   n, len;
   guint comp_key1, comp_key2;
   GdkModifierType comp_mods1, comp_mods2;

   if (akey.common_auto_completion1)
      gtk_accelerator_parse (akey.common_auto_completion1,
                             &comp_key1, &comp_mods1);
   if (akey.common_auto_completion2)
      gtk_accelerator_parse (akey.common_auto_completion2,
                             &comp_key2, &comp_mods2);

   if (event->keyval == GDK_Tab
       || (event->keyval == comp_key1 && (!comp_mods1 || (event->state & comp_mods1)))
       || (event->keyval == comp_key2 && (!comp_mods1 || (event->state & comp_mods2))))
   {
      path = gtk_entry_get_text (GTK_ENTRY (widget));
      n = auto_compl_get_n_alternatives (path);

      if (n < 1) return TRUE;

      text = auto_compl_get_common_prefix ();

      if (n == 1) {
         auto_compl_hide_alternatives ();
         gtk_entry_set_text (GTK_ENTRY (widget), text);
         if (text[strlen(text) - 1] != '/')
            gtk_entry_append_text (GTK_ENTRY (widget), "/");
      } else {
         gtk_entry_set_text (GTK_ENTRY (widget), text);
         auto_compl_show_alternatives (widget);
      }
	 
      if (text)
         g_free (text);
      return TRUE;

   } else {
      switch (event->keyval) {
      case GDK_Return:
      case GDK_KP_Enter:
         path = gtk_entry_get_text (GTK_ENTRY (widget));

         if (!isdir (path)) return FALSE;

         len = strlen (path);
         if (path[len - 1] != '/') {
            text = g_strconcat (path, "/", NULL);
         } else {
            text = g_strdup (path);
         }
         g_free (text);
         break;
      case GDK_Right:
      case GDK_Left:
      case GDK_Up:
      case GDK_Down:
         break;
      case GDK_Escape:
         *ok_pressd = FALSE;
         gtk_main_quit ();
         break;
      default:
         break;
      }
   }

   return FALSE;
}


gchar *
gtkutil_popup_textentry (const gchar   *title,
                         const gchar   *label_text,
                         const gchar   *entry_text,
                         GList         *text_list,
                         gint           entry_width,
                         TextEntryFlags flags,
                         GtkWindow *parent)
{
   GtkWidget *window, *box, *hbox, *vbox, *button, *label, *combo, *entry;
   gboolean ok_pressed = FALSE;
   gchar *str = NULL;

   /* dialog window */
   window = gtk_dialog_new ();
   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (window), parent);
   gtk_window_set_title (GTK_WINDOW (window), title); 
   gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
   g_signal_connect (G_OBJECT (window), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   /* main area */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
   gtk_widget_show (vbox);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
   gtk_widget_show (hbox);

   /* label */
   label = gtk_label_new (label_text);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_show (label);

   /* entry */
   if (flags & TEXT_ENTRY_WRAP_ENTRY)
      box = vbox;
   else
      box = hbox;

   combo = gtk_combo_new();
   entry = GTK_COMBO (combo)->entry;

   if (text_list)
      gtk_combo_set_popdown_strings (GTK_COMBO (combo), text_list);
   else
      gtk_widget_hide (GTK_COMBO (combo)->button);

   gtk_box_pack_start (GTK_BOX (box), combo, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT (entry), "activate",
                     G_CALLBACK (cb_textpop_enter), &ok_pressed);
   if (entry_text)
      gtk_entry_set_text (GTK_ENTRY (entry), entry_text);
   if (flags & TEXT_ENTRY_CURSOR_TOP)
      gtk_entry_set_position (GTK_ENTRY (entry), 0);
   if (entry_width > 0)
      gtk_widget_set_size_request (combo, entry_width, -1);
   if (flags & TEXT_ENTRY_AUTOCOMP_PATH)
      g_signal_connect_after (G_OBJECT(entry), "key-press-event",
                              G_CALLBACK(cb_textpop_key_press),
                              &ok_pressed);
   gtk_widget_show (combo);

   if (flags & TEXT_ENTRY_NO_EDITABLE)
      gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);

   gtk_widget_grab_focus (entry);

   /* button */
   button = gtk_button_new_with_label (_("OK"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_textpop_ok_button), &ok_pressed);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   button = gtk_button_new_with_label (_("Cancel"));
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
                       button, TRUE, TRUE, 0);
   g_signal_connect (G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_textpop_cancel_button), &ok_pressed);
   GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
   gtk_widget_show (button);

   gtk_widget_show (window);

   gtk_grab_add (window);
   gtk_main ();
   gtk_grab_remove (window);

   if (ok_pressed)
      str = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

   gtk_widget_destroy (window);

   return str;
}


/******************************************************************************
 *
 *   modal file dialog
 *
 ******************************************************************************/
static void
cb_filesel_ok (GtkWidget *button, gboolean *retval)
{
   *retval = TRUE;
   gtk_main_quit ();
}


static void
cb_filesel_cancel (GtkWidget *button, gboolean *retval)
{
   *retval = FALSE;
   gtk_main_quit ();
}


gchar *
gtkutil_modal_file_dialog (const gchar   *title,
                           const gchar   *default_path,
                           ModalFileDialogFlags flags,
                           GtkWindow *parent)
{
   GtkWidget *filesel = gtk_file_selection_new (title);
   GtkWidget *button;
   gchar *filename = NULL;
   gboolean retval = FALSE;

   if (parent)
      gtk_window_set_transient_for (GTK_WINDOW (filesel), parent);

   gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_CENTER);
   g_signal_connect (G_OBJECT (filesel), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   button = GTK_FILE_SELECTION (filesel)->ok_button;
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_filesel_ok),
                     &retval);
   button = GTK_FILE_SELECTION (filesel)->cancel_button;
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_filesel_cancel),
                     &retval);

   if (default_path && *default_path)
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                       default_path);

   if (flags & MODAL_FILE_DIALOG_DIR_ONLY)
      gtk_widget_hide (GTK_FILE_SELECTION (filesel)->file_list->parent);

   if (flags & MODAL_FILE_DIALOG_HIDE_FILEOP)
      gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (filesel));

   gtk_widget_show (filesel);

   gtk_grab_add (filesel);
   gtk_main ();

   if (retval) {
      const gchar *tmpstr;
      tmpstr = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
      filename = g_strdup (tmpstr);
   }

   gtk_grab_remove (filesel);
   gtk_widget_destroy (filesel);

   return filename;
}



/******************************************************************************
 *
 *   Color selection button
 *
 ******************************************************************************/
static void
cb_colorsel_ok (GtkWidget *button, gboolean *retval)
{
   *retval = TRUE;
   gtk_main_quit ();
}


static void
cb_colorsel_cancel (GtkWidget *button, gboolean *retval)
{
   *retval = FALSE;
   gtk_main_quit ();
}


static void
cb_choose_color (GtkWidget *widget, gint color[3])
{
   GtkWidget *dialog, *button, *csel;
   gboolean retval = FALSE;
   gdouble selcol[4];

   g_return_if_fail (color);

   dialog = gtk_color_selection_dialog_new (_("Choose Color"));
   selcol[0] = (gdouble) color[0] / 0xffff;
   selcol[1] = (gdouble) color[1] / 0xffff;
   selcol[2] = (gdouble) color[2] / 0xffff;
   selcol[3] = 0.0;
   csel = GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel;     
   gtk_color_selection_set_color (GTK_COLOR_SELECTION (csel), selcol);
   g_signal_connect (G_OBJECT (dialog), "delete_event",
                     G_CALLBACK (cb_dummy), NULL);

   button = GTK_COLOR_SELECTION_DIALOG (dialog)->ok_button;
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_colorsel_ok),
                     &retval);
   button = GTK_COLOR_SELECTION_DIALOG (dialog)->cancel_button;
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_colorsel_cancel),
                     &retval);
   button = GTK_COLOR_SELECTION_DIALOG (dialog)->help_button;
   gtk_widget_hide (button);
   gtk_widget_show (dialog);

   gtk_grab_add (dialog);
   gtk_main ();
   if (retval) {
      gtk_color_selection_get_color (GTK_COLOR_SELECTION (csel), selcol);
      color[0] = selcol[0] * 0xffff;
      color[1] = selcol[1] * 0xffff;
      color[2] = selcol[2] * 0xffff;
   }
   gtk_grab_remove (dialog);
   gtk_widget_destroy (dialog);
}


GtkWidget *
gtkutil_color_sel_button (const gchar *label, gint color[3])
{
   GtkWidget *button;

   button = gtk_button_new_with_label (label);
   g_signal_connect (G_OBJECT (button),"clicked",
                     G_CALLBACK (cb_choose_color),
                     color);

   return button;
}


/******************************************************************************
 *
 *   Compare functions
 *
 ******************************************************************************/
gint
gtkutil_comp_spel (gconstpointer data1, gconstpointer data2)
{
   const gchar *str1 = data1;
   const gchar *str2 = data2;

   return strcmp (str1, str2);
}


gint
gtkutil_comp_casespel (gconstpointer data1, gconstpointer data2)
{
   const gchar *str1 = data1;
   const gchar *str2 = data2;

   return g_strcasecmp (str1, str2);
}


/******************************************************************************
 *
 *   simple callback functions
 *
 ******************************************************************************/
void
gtkutil_get_data_from_toggle_cb (GtkWidget *toggle, gboolean *data)
{
   g_return_if_fail (data);

   *data = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle));
}


void
gtkutil_get_data_from_toggle_negative_cb (GtkWidget *toggle, gboolean *data)
{
   g_return_if_fail (data);

   *data = !(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(toggle)));
}


void
gtkutil_get_data_from_adjustment_by_int_cb (GtkWidget *widget, gint *data)
{
   g_return_if_fail (data);

   *data = GTK_ADJUSTMENT(widget)->value;
}


void
gtkutil_get_data_from_adjustment_by_float_cb (GtkWidget *widget, gfloat *data)
{
   g_return_if_fail (data);

   *data = GTK_ADJUSTMENT(widget)->value;
}

gboolean
gtkutil_scroll_to_button_cb (GtkWidget *widget,
                             GdkEventScroll *se,
                             gpointer data)
{
   GdkEventButton be;
   gboolean retval;

   g_return_val_if_fail (GTK_IS_WIDGET(widget), FALSE);

   be.type       = GDK_BUTTON_PRESS;
   be.window     = se->window;
   be.send_event = se->send_event;
   be.time       = se->time;
   be.x          = se->x;
   be.y          = se->y;
   be.axes       = NULL;
   be.state      = se->state;
   be.device     = se->device;
   be.x_root     = se->x_root;
   be.y_root     = se->y_root;
   switch ((se)->direction) {
   case GDK_SCROLL_UP:
      be.button = 4;
      break;
   case GDK_SCROLL_DOWN:
      be.button = 5;
      break;
   case GDK_SCROLL_LEFT:
      be.button = 6;
      break;
   case GDK_SCROLL_RIGHT:
      be.button = 7;
      break;
   default:
      g_warning ("invalid scroll direction!");
      be.button = 0;
      break;
   }

   g_signal_emit_by_name (G_OBJECT(widget), "button-press-event",
                          &be, &retval);
   be.type = GDK_BUTTON_RELEASE;
   g_signal_emit_by_name (G_OBJECT(widget), "button-release-event",
                          &be, &retval);

   return retval;
}
