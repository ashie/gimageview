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

#ifndef __GIMV_THUMB_WIN_H__
#define __GIMV_THUMB_WIN_H__

#include "gimageview.h"

#include "gimv_image_view.h"
#include "gimv_thumb_view.h"

#define GIMV_TYPE_THUMB_WIN            (gimv_thumb_win_get_type ())
#define GIMV_THUMB_WIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMV_TYPE_THUMB_WIN, GimvThumbWin))
#define GIMV_THUMB_WIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMV_TYPE_THUMB_WIN, GimvThumbWinClass))
#define GIMV_IS_THUMB_WIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMV_TYPE_THUMB_WIN))
#define GIMV_IS_THUMB_WIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_THUMB_WIN))
#define GIMV_THUMB_WIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMV_TYPE_THUMB_WIN, GimvThumbWinClass))

#define GIMV_THUMB_WIN_MIN_THUMB_SIZE 4
#define GIMV_THUMB_WIN_MAX_THUMB_SIZE 640

/* current page of notebook */
#define GIMV_THUMB_WIN_CURRENT_PAGE -1


typedef struct GimvThumbWinClass_Tag GimvThumbWinClass;
typedef struct GimvThumbWinPriv_Tag  GimvThumbWinPriv;


/* program status */
typedef enum {
   GIMV_THUMB_WIN_STATUS_NORMAL,
   GIMV_THUMB_WIN_STATUS_LOADING,
   GIMV_THUMB_WIN_STATUS_LOADING_BG,
   GIMV_THUMB_WIN_STATUS_CHECKING_DUPLICATE,
   GIMV_THUMB_WIN_STATUS_NO_SENSITIVE
} GimvThumbwinStatus;


typedef enum {
   GIMV_SORT_NAME,
   GIMV_SORT_ATIME,
   GIMV_SORT_MTIME,
   GIMV_SORT_CTIME,
   GIMV_SORT_SIZE,
   GIMV_SORT_TYPE,
   GIMV_SORT_WIDTH,
   GIMV_SORT_HEIGHT,
   GIMV_SORT_AREA,
   GIMV_SORT_NONE
} GimvSortItem;


typedef enum {
   GIMV_SORT_REVERSE          = 1 << 0,
   GIMV_SORT_CASE_INSENSITIVE = 1 << 1,
   GIMV_SORT_DIR_INSENSITIVE  = 1 << 2
} GimvSortFlag;


typedef struct GimvThumbwinComposeType_Tag {
   gboolean pane1_horizontal;
   gboolean pane2_horizontal;
   gboolean pane2_attach_to_child1;
   GimvComponentType widget_type[3];
} GimvThumbwinComposeType;

extern GimvThumbwinComposeType compose_type[];


/* thumbnail window composition */
struct GimvThumbWin_Tag
{
   GtkWindow  parent;

   GtkWidget *main_vbox;

   /* top of window (menu & toolbar) */
   GtkWidget *menubar;
   GtkWidget *menubar_handle;
   GtkWidget *toolbar;
   GtkWidget *toolbar_handle;
   GtkWidget *location_entry;
   GtkWidget *summary_mode_menu;

   /* middle of window (main contents) */
   GtkWidget *main_contents;
   GtkWidget *notebook;
   GtkWidget *preview;
   GtkWidget *pane1;
   GtkWidget *pane2;

   /* dir view module */
   GimvDirView   *dv;

   /* image view module */
   GimvImageView *iv;

   /* commnet view module */
   GimvCommentView *cv;

   /* bottom of window (status bar) */
   GtkWidget *status_bar_container;
   GtkWidget *status_bar1;
   GtkWidget *status_bar2;
   GtkWidget *status_bar3;
   GtkWidget *progressbar;

   /* popup menu */
   GtkWidget *thumbview_popup;

   /* sub menus */
   GtkWidget *view_menu;
   GtkWidget *sort_menu;
   GtkWidget *comp_menu;

   struct    /* menuitems */
   {
      GtkWidget *file;
      GtkWidget *edit;
      GtkWidget *view;
      GtkWidget *tool;

      GtkWidget *dirview;
      GtkWidget *preview;
      GtkWidget *menubar;
      GtkWidget *toolbar;
      GtkWidget *dir_toolbar;
      GtkWidget *statusbar;
      GtkWidget *tab;
      GtkWidget *preview_tab;
      GtkWidget *fullscr;

      GtkWidget *layout[6];

      GtkWidget *select;
      GtkWidget *unselect;
      GtkWidget *refresh;
      GtkWidget *reload;
      GtkWidget *recreate;

      GtkWidget *rename;
      GtkWidget *copy;
      GtkWidget *move;
      GtkWidget *link;
      GtkWidget *delete;

      GtkWidget *find_sim;

      GtkWidget *sort_name;
      GtkWidget *sort_access;
      GtkWidget *sort_time;
      GtkWidget *sort_change;
      GtkWidget *sort_size;
      GtkWidget *sort_type;
      GtkWidget *sort_width, *sort_height, *sort_area;
      GtkWidget *sort_rev, *sort_case, *sort_dir;
   } menuitem;

   struct    /* buttons in toolbar */
   {
      GtkWidget *fileopen;
      GtkWidget *prefs;
      GtkWidget *refresh;
      GtkWidget *skip;
      GtkWidget *stop;
      GtkWidget *prev;
      GtkWidget *next;
      GtkWidget *quit;
      GtkWidget *size_spin;
   } button;

   /* window status */
   GimvThumbwinStatus status;
   gboolean     show_dirview;
   gboolean     show_preview;
   gboolean     fullscreen;
   gint         win_x;
   gint         win_y;
   gint         win_width;
   gint         win_height;
   gint         pane_size1;
   gint         pane_size2;
   const gchar *thumbview_summary_mode;
   GimvImageViewPlayerVisibleType player_visible;
   GimvSortItem sortitem;
   GimvSortFlag sortflags;
   gint         layout_type;
   GtkPositionType tab_pos;
   gboolean     changing_layout;

   gint   filenum; /* image files number in this window */
   gint   pagenum; /* notebook pages number in this window */
   gint   newpage_count;
   gulong filesize;

   /* File open dialog */
   GtkWidget *open_dialog;

   GSList *accel_group_list;

   GimvThumbWinPriv *priv;
};


struct GimvThumbWinClass_Tag
{
   GtkWindowClass parent_class;
};


GList       *gimv_thumb_win_get_list                  (void);
GType        gimv_thumb_win_get_type                  (void);

GtkWidget   *gimv_thumb_win_new                       (void);
GimvThumbWin
            *gimv_thumb_win_open_window              (void);

GimvThumbView
            *gimv_thumb_win_find_thumbtable           (GimvThumbWin   *tw,
                                                       gint            pagenum);
void         gimv_thumb_win_set_statusbar_page_info   (GimvThumbWin   *tw,
                                                       gint            pagenum);
void         gimv_thumb_win_loading_update_progress   (GimvThumbWin   *tw,
                                                       gint            pagenum);
void         gimv_thumb_win_set_sensitive             (GimvThumbWin   *tw,
                                                       GimvThumbwinStatus status);
void         gimv_thumb_win_set_tab_label_text        (GtkWidget *page_container,
                                                       const gchar    *title);
void         gimv_thumb_win_set_tab_label_state       (GtkWidget *page_container,
                                                       GtkStateType    state);
GimvThumbView
            *gimv_thumb_win_create_new_tab            (GimvThumbWin   *tw);
GtkWidget   *gimv_thumb_win_detach_tab                (GimvThumbWin   *tw_dest,
                                                       GimvThumbWin   *tw_src,
                                                       GimvThumbView  *tv);
void         gimv_thumb_win_close_tab                 (GimvThumbWin   *tw,
                                                       gint            page);
void         gimv_thumb_win_reload_thumbnail          (GimvThumbWin   *tw,
                                                       ThumbLoadType   type);
void         gimv_thumb_win_open_dirview              (GimvThumbWin   *tw);
void         gimv_thumb_win_close_dirview             (GimvThumbWin   *tw);
void         gimv_thumb_win_open_preview              (GimvThumbWin   *tw);
void         gimv_thumb_win_close_preview             (GimvThumbWin   *tw);
void         gimv_thumb_win_location_entry_set_text   (GimvThumbWin   *tw,
                                                       const gchar    *location);
GimvSortItem gimv_thumb_win_get_sort_type             (GimvThumbWin   *tw,
                                                       GimvSortFlag   *flags_ret);
/* currently, only GIMV_THUMB_WIN_CURRENT_PAGE will be accepted */
void         gimv_thumb_win_sort_thumbnail            (GimvThumbWin   *tw,
                                                       GimvSortItem    item,
                                                       GimvSortFlag    flags,
                                                       gint            page);
void         gimv_thumb_win_change_layout             (GimvThumbWin   *tw,
                                                       gint            layout);
void         gimv_thumb_win_swap_component            (GimvThumbWin   *tw,
                                                       GimvComponentType com1,
                                                       GimvComponentType com2);
void          gimv_thumb_win_save_state               (GimvThumbWin   *tw);

/* FIXMEEEEEEEEEEEEEEEEEE!! (TOT */
void         gimv_thumb_win_remove_key_accel          (GimvThumbWin   *tw);
void         gimv_thumb_win_reset_key_accel           (GimvThumbWin   *tw);
/* END FIXMEEEEEEEEEEEEEEEEEE!! (TOT */

#endif /* __GIMV_THUMB_WIN_H__ */
