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

#include <string.h>

#include "gimageview.h"

#include "dnd.h"
#include "fileutil.h"
#include "gfileutil.h"
#include "gtkutils.h"
#include "gimv_thumb.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "menu.h"
#include "prefs.h"


GtkTargetEntry dnd_types_all[] = {
   {"GIMV_TAB",                   0, TARGET_GIMV_TAB},
   {"GIMV_COMPONENT",             0, TARGET_GIMV_COMPONENT},
   {"GIMV_ARCHIVE_MEMBER_LIST",   0, TARGET_GIMV_ARCHIVE_MEMBER_LIST},
   {"text/uri-list",              0, TARGET_URI_LIST},
   {"property/bgimage",           0, TARGET_URI_LIST},
};
const gint dnd_types_all_num = sizeof(dnd_types_all) / sizeof(GtkTargetEntry);

GtkTargetEntry *dnd_types_uri = &dnd_types_all[3];
const gint dnd_types_uri_num = 1;

GtkTargetEntry *dnd_types_archive = &dnd_types_all[2];
const gint dnd_types_archive_num = 2;

GtkTargetEntry *dnd_types_tab_component = &dnd_types_all[0];
const gint dnd_types_tab_component_num = 2;

GtkTargetEntry *dnd_types_component = &dnd_types_all[1];
const gint dnd_types_component_num = 1;


GtkItemFactoryEntry dnd_file_popup_items [] =
{
   {N_("/Open in new tab"), NULL, menu_modal_cb, GDK_ACTION_PRIVATE, NULL},
   {N_("/---"),             NULL, NULL,          0,                  "<Separator>"},
   {N_("/Move"),            NULL, menu_modal_cb, GDK_ACTION_MOVE,    NULL},
   {N_("/Copy"),            NULL, menu_modal_cb, GDK_ACTION_COPY,    NULL},
   {N_("/Symbolic Link"),   NULL, menu_modal_cb, GDK_ACTION_LINK ,   NULL},
   {N_("/---"),             NULL, NULL,          0,                  "<Separator>"},
   {N_("/Cancel"),          NULL, NULL,          0,                  NULL},
   {NULL, NULL, NULL, 0, NULL},
};
 

/*
 *  dnd_get_file_list:
 *     @ convert string URI list to GList format.
 *
 *  string : file list (string format)
 *  Return : file list (GList format)
 */
GList *
dnd_get_file_list (const gchar *string, gint len)
{
   gchar *file;
   gchar *ptr, *uri;
   GList *list = NULL;
   gint pos = 0;

   uri = ptr = g_memdup (string, len);

   while (*ptr && (pos < len)) {
      if (!strncmp(ptr, "file:", 5)) {
         ptr += 5;
         pos += 5;
      }
      if (!strncmp(ptr, "//", 2)){
         ptr += 2;
         pos += 2;
      }

      file = ptr;

      while (*ptr != '\r' && *ptr != '\n' && *ptr != '\0') {
         ptr++;
         pos++;
      }
      *ptr++ = '\0';
      pos++;

      while (*ptr == '\r' || *ptr == '\n') {
         ptr++;
         pos++;
      }

      if (file && file[0] != '\r' && file[0] != '\n' && file[0] != '\0')
         list = g_list_append (list, g_strdup(file));
   }

   g_free (uri);

   return list;
}


/*
 *  dnd_src_set:
 *     @
 *
 *  widget : widget to set DnD (source side).
 */
void
dnd_src_set (GtkWidget *widget, const GtkTargetEntry *entry, gint num)
{
   /* FIXME */
   if (conf.dnd_enable_to_external)
      dnd_types_all[3].flags = 0;
   else
      dnd_types_all[3].flags = GTK_TARGET_SAME_APP;

   gtk_drag_source_set(widget,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK,
                       entry, num,
                       GDK_ACTION_ASK  | GDK_ACTION_COPY
                       | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}


/*
 *  dnd_dest_set:
 *     @
 *
 *  widget : widget to set DnD (destination side).
 */
void
dnd_dest_set (GtkWidget *widget, const GtkTargetEntry *entry, gint num)
{
   if (conf.dnd_enable_from_external)
      dnd_types_all[3].flags = 0;
   else
      dnd_types_all[3].flags = GTK_TARGET_SAME_APP;

   gtk_drag_dest_set(widget,
                     GTK_DEST_DEFAULT_ALL,
                     /* GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, */
                     entry, num,
                     GDK_ACTION_ASK  | GDK_ACTION_COPY
                     | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}


/*
 *  dnd_file_operation:
 *     @ Open DnD context menu and do specified file operation when data
 *       received. This function will called by "drag_data_received" signal's
 *       callback function.
 *
 *  dest_dir : destination directory to move/copy/link specified files.
 *  context  : DnD context.
 *  seldata  : URI list (string format).
 *  time     : time when drag data received.
 *  tw       : Pointer to parent ThumbWindow.
 */
void
dnd_file_operation (const gchar *dest_dir, GdkDragContext *context,
                    GtkSelectionData *seldata, guint time, GimvThumbWin *tw)
{
   GtkWidget *dnd_popup;
   GList *list;
   GimvThumbView *tv;
   gboolean dnd_success = TRUE, dnd_delete = FALSE;
   gint n_menu_items, action;

   g_return_if_fail (dest_dir && context && seldata);

   list = dnd_get_file_list (seldata->data, seldata->length);

   /* create popup menu */
   n_menu_items = sizeof (dnd_file_popup_items)
      / sizeof (dnd_file_popup_items[0]) - 1;
   dnd_popup = menu_create_items (NULL, dnd_file_popup_items,
                                  n_menu_items, "<DnDPop>",
                                  NULL);

   gtk_object_ref (GTK_OBJECT (dnd_popup));
   gtk_object_sink (GTK_OBJECT (dnd_popup));

   /* popup menu */
   action = menu_popup_modal (dnd_popup, NULL, NULL, NULL, NULL);

   gtk_widget_unref (dnd_popup);
 
   if (action == GDK_ACTION_PRIVATE) {
      open_images_dirs (list, tw, LOAD_CACHE, FALSE);
   } else {
      GtkWindow *window = tw ? GTK_WINDOW (tw) : NULL;
      switch (action) {
      case GDK_ACTION_MOVE:
         files2dir (list, dest_dir, FILE_MOVE, window);
         dnd_delete = TRUE;
         break;
      case GDK_ACTION_COPY:
         files2dir (list, dest_dir, FILE_COPY, window);
         break;
      case GDK_ACTION_LINK:
         files2dir (list, dest_dir, FILE_LINK, window);
         break;
      default:
         dnd_success = FALSE;
         break;
      }
   }

   gtk_drag_finish(context, dnd_success, dnd_delete, time);

   /* update dest side file list */
   tv = gimv_thumb_view_find_opened_dir (dest_dir);
   if (tv) {
      gimv_thumb_view_refresh_list (tv);
      gtk_idle_add (gimv_thumb_view_refresh_list_idle, tv);
   }

   g_list_foreach (list, (GFunc) g_free, NULL);
   g_list_free (list);
}
