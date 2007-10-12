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

#ifndef __GTKUTILS_H__
#define __GTKUTILS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>


typedef enum {
   CONFIRM_ERROR = -1,
   CONFIRM_YES,
   CONFIRM_YES_TO_ALL,
   CONFIRM_NO,
   CONFIRM_NO_TO_ALL,
   CONFIRM_CANCEL,
   CONFIRM_ASK
} ConfirmType;


typedef enum {
   ConfirmDialogMultipleFlag = 1 << 0
} ConfirmDialogFlags;


typedef enum {
   TEXT_ENTRY_AUTOCOMP_PATH = 1 << 1,
   TEXT_ENTRY_WRAP_ENTRY    = 1 << 2,
   TEXT_ENTRY_CURSOR_TOP    = 1 << 3,
   TEXT_ENTRY_NO_EDITABLE   = 1 << 4
} TextEntryFlags;


typedef enum {
   MODAL_FILE_DIALOG_DIR_ONLY    = 1 << 0,
   MODAL_FILE_DIALOG_HIDE_FILEOP = 1 << 1
} ModalFileDialogFlags;


const gchar  *boolean_to_text                (gboolean       boolval);
gboolean      text_to_boolean                (gchar         *text);
void          gtkutil_get_widget_area        (GtkWidget     *widget,
                                              GdkRectangle  *area);

GtkWidget    *gtkutil_create_check_button    (const gchar   *lebel_text,
                                              gboolean       def_val,
                                              gpointer       func,
                                              gpointer       data);
GtkWidget    *gtkutil_create_toolbar         (void);
GtkWidget    *gtkutil_create_spin_button     (GtkAdjustment *adj);
GtkWidget    *gtkutil_option_menu_get_current(GtkWidget     *option_menu);
GList        *gtkutil_list_insert_sorted     (GList         *list,
                                              gpointer       data,
                                              GCompareFunc   func,
                                              gboolean       reverse);
guint         gtkutil_paned_which_is_hidden  (GtkPaned      *paned);

/* confirm dialog window */
ConfirmType   gtkutil_confirm_dialog         (const gchar   *title,
                                              const gchar   *message,
                                              ConfirmDialogFlags flags,
                                              GtkWindow     *parent);
ConfirmType
gtkutil_overwrite_confirm_dialog (const gchar *title, const gchar *message,
                                  const gchar *dest_file, const gchar *src_file,
                                  gchar *new_path, gint new_path_len,
                                  ConfirmDialogFlags flags,
                                  GtkWindow *parent);

/* message dialog window */
void          gtkutil_message_dialog         (const gchar   *title,
                                              const gchar   *message,
                                              GtkWindow *parent);

/* progress bar window */
GtkWidget    *gtkutil_create_progress_window (gchar         *title,
                                              gchar         *initial_message,
                                              gboolean      *cancel_pressed,
                                              gint           width,
                                              gint           height,
                                              GtkWindow     *parent);
void          gtkutil_progress_window_update (GtkWidget     *window,
                                              gchar         *title,
                                              gchar         *message,
                                              gchar         *progress_text,
                                              gfloat         progress);

/* text entry window */
gchar        *gtkutil_popup_textentry        (const gchar   *title,
                                              const gchar   *label_text,
                                              const gchar   *entry_text,
                                              GList         *text_list,
                                              gint           entry_width,
                                              TextEntryFlags flags,
                                              GtkWindow     *parent);

/* modal file selection */
gchar        *gtkutil_modal_file_dialog      (const gchar   *title,
                                              const gchar   *default_path,
                                              ModalFileDialogFlags flags,
                                              GtkWindow     *parent);

/* color selection button */
GtkWidget    *gtkutil_color_sel_button       (const gchar *label,
                                              gint         color[3]);

/* compare functions */
gint          gtkutil_comp_spel              (gconstpointer  data1,
                                              gconstpointer  data2);
gint          gtkutil_comp_casespel          (gconstpointer  data1,
                                              gconstpointer  data2);

/* simple callback functions */
void          gtkutil_get_data_from_toggle_cb              (GtkWidget *toggle,
                                                            gboolean  *data);
void          gtkutil_get_data_from_toggle_negative_cb     (GtkWidget *toggle,
                                                            gboolean  *data);
void          gtkutil_get_data_from_adjustment_by_int_cb   (GtkWidget *widget,
                                                            gint      *data);
void          gtkutil_get_data_from_adjustment_by_float_cb (GtkWidget *widget,
                                                            gfloat    *data);

#endif /* __GTKUTILS_H__ */
