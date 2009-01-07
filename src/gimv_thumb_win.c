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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "gimageview.h"

#include "gimv_comment_view.h"
#include "gimv_dir_view.h"
#include "gimv_dupl_finder.h"
#include "gimv_icon_stock.h"
#include "gimv_image_win.h"
#include "gimv_prefs_win.h"
#include "gimv_slideshow.h"
#include "gimv_thumb.h"
#include "gimv_thumb_view.h"
#include "gimv_thumb_win.h"
#include "help.h"
#include "prefs.h"
#include "utils_auto_comp.h"
#include "utils_char_code.h"
#include "utils_dnd.h"
#include "utils_file.h"
#include "utils_file_gtk.h"
#include "utils_menu.h"


typedef enum {
   OPEN_FILE,
   OPEN_IMAGEWIN,
   OPEN_THUMBWIN,
   OPEN_THUMBTAB,
   OPEN_PREFS,
   OPEN_GIMV_INFO
} OpenItem;


typedef enum {
   CLOSE_FILE,
   CLOSE_THUMBWIN,
   CLOSE_THUMBTAB,
   CLOSE_PREFS,
   CLOSE_GIMV_INFO,
   CLOSE_ALL
} CloseItem;


typedef enum {
   CURRENT,
   NEXT,
   PREV,
   FIRST,
   LAST
} SwitchPage;


typedef enum {
   DIR_VIEW,
   THUMBNAIL_VIEW,
   PREVIEW,
   LOCATION_ENTRY,
   THUMBNAIL_SIZE_ENTRY
} FocusItem;

typedef enum {
   MENUBAR,
   TOOLBAR,
   TAB,
   STATUSBAR,
   DIR_TOOLBAR,
   PREVIEW_TAB
} ShowItem;


typedef enum {
   LEFT,
   RIGHT
} MoveTabItem;


typedef enum {
   PERVIEW_NONE,
   PREVIEW_NEXT,
   PREVIEW_PREV,
   PREVIEW_SHAREDWIN,
   PREVIEW_NEWWIN,
   PREVIEW_POPUP,
   PREVIEW_ZOOM_IN,
   PREVIEW_ZOOM_OUT,
   PREVIEW_FIT,
   PREVIEW_ROTATE_CCW,
   PREVIEW_ROTATE_CW,
   PREVIEW_NAVWIN,
   PREVIEW_UP,
   PREVIEW_DOWN,
   PREVIEW_LEFT,
   PREVIEW_RIGHT
} PreviewPressType;


typedef enum {
   CLEAR_CACHE_NONE,
   CLEAR_CACHE_TAB,
   CLEAR_CACHE_ALL
} CacheClearType;


typedef enum {
   SLIDESHOW_START_FROM_FIRST,
   SLIDESHOW_START_FROM_SELECTED,
   SLIDESHOW_RANDOM_ORDER,
} SlideshowOrder;


typedef enum {
   SLIDESHOW_IMAGES_AND_MOVIES,
   SLIDESHOW_IMAGES_ONLY,
   SLIDESHOW_MOVIES_ONLY
} SlideshowFileType;


struct GimvThumbWinPriv_Tag {
   SlideshowOrder    slideshow_order;
   gboolean          slideshow_selected_only;
   SlideshowFileType slideshow_file_type;
};


static void       gimv_thumb_win_dispose             (GObject           *object);
static void       gimv_thumb_win_realize             (GtkWidget         *widget);
static gboolean   gimv_thumb_win_delete_event        (GtkWidget         *widget,
                                                      GdkEventAny       *event);

/* Parts of Thumbnail Window and utilities */
static void       create_gimv_thumb_win_menus        (GimvThumbWin   *tw);
static void       sync_widget_state_to_menu          (GimvThumbWin   *tw);
static GtkWidget *create_toolbar                     (GimvThumbWin   *tw,
                                                      GtkWidget      *container);
static void       gimv_thumb_win_get_layout          (GimvThumbwinComposeType *compose,
                                                      gint            layout);
static void       gimv_thumb_win_pane_set_visible    (GimvThumbWin   *tw,
                                                      GimvComponentType item);
static GtkWidget *thumbnail_window_contents_new      (GimvThumbWin   *tw);
static GtkWidget *thumbnail_view_new                 (GimvThumbWin   *tw);
static gint       image_preview_action               (GimvImageView  *iv,
                                                      GdkEventButton *event,
                                                      gint            num);
static gint       cb_image_preview_pressed           (GimvImageView  *iv,
                                                      GdkEventButton *event,
                                                      gpointer        data);
static gint       cb_image_preview_clicked           (GimvImageView  *iv,
                                                      GdkEventButton *event,
                                                      gpointer        data);
static GtkWidget *image_preview_new                  (GimvThumbWin   *tw);
static void       location_entry_set_text            (GimvThumbWin   *tw,
                                                      GimvThumbView  *tv,
                                                      const gchar    *location);


/* Callback Functions for main menu */
static void cb_open                 (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_close                (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_select_all           (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_refresh_list         (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_reload               (GimvThumbWin  *tw,
                                     ThumbLoadType  type,
                                     GtkWidget     *widget);
static void cb_clear_cache          (GimvThumbWin  *tw,
                                     CacheClearType type,
                                     GtkWidget     *widget);
static void cb_file_operate         (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *menuitem);
static void cb_find_similar         (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_move_tab             (GimvThumbWin  *tw,
                                     MoveTabItem    sortitem,
                                     GtkWidget     *widget);
static void cb_cut_out_tab          (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_switch_layout        (GimvThumbWin  *tw,
                                     gint          action,
                                     GtkWidget     *widget);
static void cb_switch_tab_pos       (GimvThumbWin  *tw,
                                     GtkPositionType pos,
                                     GtkWidget     *widget);
static void cb_slideshow            (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_slideshow_order      (GimvThumbWin  *tw,
                                     SlideshowOrder action,
                                     GtkWidget     *widget);
static void cb_slideshow_selected   (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_slideshow_file_type  (GimvThumbWin  *tw,
                                     SlideshowFileType action,
                                     GtkWidget     *widget);
static void cb_switch_page          (GimvThumbWin  *tw,
                                     SwitchPage     action,
                                     GtkWidget     *widget);
static void cb_focus                (GimvThumbWin  *tw,
                                     FocusItem      item,
                                     GtkWidget     *widget);
static void cb_toggle_view          (GimvThumbWin  *tw,
                                     GimvComponentType item,
                                     GtkWidget     *widget);
static void cb_toggle_show          (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
static void cb_switch_player        (GimvThumbWin  *tw,
                                     GimvImageViewPlayerVisibleType visible,
                                     GtkWidget     *widget);
static void cb_toggle_maximize      (GimvThumbWin  *tw,
                                     guint          action,
                                     GtkWidget     *widget);
/*
  static void cb_toggle_fullscreen    (GimvThumbWin   *tw,
  guint          action,
  GtkWidget     *widget);
*/
static void cb_sort_item            (GimvThumbWin  *tw,
                                     GimvSortItem   sortitem,
                                     GtkWidget     *widget);
static void cb_sort_flag            (GimvThumbWin  *tw,
                                     GimvSortFlag   sortflag,
                                     GtkWidget     *widget);
static void cb_win_composition_menu (GtkMenuItem   *item,
                                     gpointer       data);

static void cb_file_submenu_show    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_file_submenu_hide    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_edit_submenu_show    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_edit_submenu_hide    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_view_submenu_show    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_view_submenu_hide    (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_tab_submenu_show     (GtkWidget     *widget,
                                     GimvThumbWin  *tw);
static void cb_tab_submenu_hide     (GtkWidget     *widget,
                                     GimvThumbWin  *tw);

/* callback function for toolbar buttons */
static void cb_location_entry_enter              (GtkEditable  *entry,
                                                  GimvThumbWin *tw);
static void cb_location_entry_drag_data_received (GtkWidget *widget,
                                                  GdkDragContext  *context,
                                                  gint x, gint y,
                                                  GtkSelectionData *seldata,
                                                  guint     info,
                                                  guint     time,
                                                  gpointer  data);
static gboolean cb_location_entry_key_press (GtkWidget    *widget, 
                                             GdkEventKey  *event,
                                             GimvThumbWin *tw);
static void cb_open_button                  (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_prefs_button                 (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_previous_button              (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_next_button                  (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_refresh_button               (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_skip_button                  (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_stop_button                  (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static gboolean cb_size_spinner_key_press   (GtkWidget    *widget, 
                                             GdkEventKey  *event,
                                             GimvThumbWin *tw);
static void cb_quit_button                  (GtkWidget    *widget,
                                             GimvThumbWin *tw);
static void cb_display_mode_menu            (GtkWidget    *widget,
                                             GimvThumbWin *tw);

/* other callback functions */
static void cb_thumb_notebook_switch_page (GtkNotebook      *notebook,
                                           GtkNotebookPage  *page,
                                           gint              pagenum,
                                           GimvThumbWin     *tw);
static void cb_tab_close_button_clicked   (GtkWidget        *button,
                                           GimvThumbWin     *tw);
static void cb_com_drag_begin             (GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           gpointer          data);
static void cb_com_drag_data_get          (GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           GtkSelectionData *seldata,
                                           guint             info,
                                           guint             time,
                                           gpointer          data);
static void cb_notebook_drag_data_received(GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           gint x, gint y,
                                           GtkSelectionData *seldata,
                                           guint             info,
                                           guint             time,
                                           gpointer          data);
static void cb_tab_drag_data_received     (GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           gint x, gint y,
                                           GtkSelectionData *seldata,
                                           guint             info,
                                           guint             time,
                                           gpointer          data);
static void cb_com_swap_drag_data_received(GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           gint x, gint y,
                                           GtkSelectionData *seldata,
                                           guint             info,
                                           guint             time,
                                           gpointer          data);
#if 1   /* FIXME */
static gboolean cb_focus_in               (GtkWidget        *widget,
                                           GdkEventFocus    *event,
                                           GimvThumbWin     *tw);
static gboolean cb_focus_out              (GtkWidget        *widget,
                                           GdkEventFocus    *event,
                                           GimvThumbWin     *tw);
#endif


/******************************************************************************
 *
 *   Parts of thumbnail window.
 *
 ******************************************************************************/
/* for main manu */
GtkItemFactoryEntry gimv_thumb_win_menu_items[] =
{
   {N_("/_File"),                       NULL,         NULL,     0,               "<Branch>"},
   {N_("/_File/_Open..."),              "<control>F", cb_open,  OPEN_FILE,       NULL},
   {N_("/_File/New _Image Window"),     "<control>I", cb_open,  OPEN_IMAGEWIN,   NULL},
   {N_("/_File/New _Window"),           "<control>W", cb_open,  OPEN_THUMBWIN,   NULL},
   {N_("/_File/New _Tab"),              "<control>T", cb_open,  OPEN_THUMBTAB,   NULL},
   {N_("/_File/---"),                   NULL,         NULL,     0,               "<Separator>"},
   {N_("/_File/Close Ta_b"),            "Q",          cb_close, CLOSE_THUMBTAB,  NULL},
   {N_("/_File/_Close Window"),         "<control>Q", cb_close, CLOSE_THUMBWIN,  NULL},
   {N_("/_File/_Quit"),                 "<control>C", cb_close, CLOSE_ALL,       NULL},

   {N_("/_Edit"),                       NULL,         NULL,             0,            "<Branch>"},
   {N_("/_Edit/_Select All"),           "<control>A", cb_select_all,    TRUE,         NULL},
   {N_("/_Edit/U_nselect All"),         "<control>N", cb_select_all,    FALSE,        NULL},
   {N_("/_Edit/---"),                   NULL,         NULL,             0,            "<Separator>"},
   {N_("/_Edit/_Refresh List"),         "<shift><control>R", cb_refresh_list,   0,    NULL},
   {N_("/_Edit/Reload _Cache"),         "<shift>R",   cb_reload,        LOAD_CACHE,   NULL},
   {N_("/_Edit/_Update All Thumbnail"), "<control>R", cb_reload,        CREATE_THUMB, NULL},
   {N_("/_Edit/---"),                   NULL,         NULL,             0,            "<Separator>"},
   /*
   {N_("/_Edit/Clear cache for _this tab"), NULL,     cb_clear_cache,   CLEAR_CACHE_TAB, NULL},
   */
   {N_("/_Edit/R_ename..."),            "<shift>E",   cb_file_operate,  FILE_RENAME, NULL},
   {N_("/_Edit/Co_py Files To..."),     "<shift>C",   cb_file_operate,  FILE_COPY,   NULL},
   {N_("/_Edit/_Move Files To..."),     "<shift>M",   cb_file_operate,  FILE_MOVE,   NULL},
   {N_("/_Edit/_Link Files To..."),     "<shift>L",   cb_file_operate,  FILE_LINK,   NULL},
   {N_("/_Edit/Remo_ve file..."),       "<del>",      cb_file_operate,  FILE_REMOVE, NULL},
   {N_("/_Edit/---"),                   NULL,         NULL,             0,            "<Separator>"},
   {N_("/_Edit/_Options..."),           "<control>O", cb_open,          OPEN_PREFS,   NULL},

   {N_("/_View"), NULL, NULL, 0, "<Branch>"},

   {N_("/_Tab"),                       NULL,         NULL,              0,              "<Branch>"},
   {N_("/_Tab/_Tab Position"),         NULL,         NULL,              0,              "<Branch>"},  
   {N_("/_Tab/_Tab Position/_Top"),    NULL,         cb_switch_tab_pos, GTK_POS_TOP,    "<RadioItem>"},  
   {N_("/_Tab/_Tab Position/_Bottom"), NULL,         cb_switch_tab_pos, GTK_POS_BOTTOM, "/Tab/Tab Position/Top"},  
   {N_("/_Tab/_Tab Position/_Left"),   NULL,         cb_switch_tab_pos, GTK_POS_LEFT,   "/Tab/Tab Position/Bottom"},  
   {N_("/_Tab/_Tab Position/_Right"),  NULL,         cb_switch_tab_pos, GTK_POS_RIGHT,  "/Tab/Tab Position/Left"},  
   {N_("/_Tab/---"),                   NULL,         NULL,              0,              "<Separator>"},
   {N_("/_Tab/_First Page"),           "k",          cb_switch_page,    FIRST,          NULL},
   {N_("/_Tab/_Last Page"),            "J",          cb_switch_page,    LAST,           NULL},
   {N_("/_Tab/_Next Page"),            "L",          cb_switch_page,    NEXT,           NULL},
   {N_("/_Tab/_Previous Page"),        "H",          cb_switch_page,    PREV,           NULL},
   {N_("/_Tab/---"),                   NULL,         NULL,              0,              "<Separator>"},
   {N_("/_Tab/Move tab for_ward"),     "<shift>F",   cb_move_tab,       LEFT,           NULL},
   {N_("/_Tab/Move tab _backward"),    "<shift>B",   cb_move_tab,       RIGHT,          NULL},
   {N_("/_Tab/_Detach tab"),           "<control>D", cb_cut_out_tab,    0,              NULL},

   {N_("/_Action"),                        NULL,            NULL,     0,              "<Branch>"},
   {N_("/_Action/_Focus"),                 NULL,            NULL,     0,              "<Branch>"},
   {N_("/_Action/_Focus/_Directory View"), "<shift><alt>D", cb_focus, DIR_VIEW,       NULL},
   {N_("/_Action/_Focus/_Thumbnail View"), "<shift><alt>T", cb_focus, THUMBNAIL_VIEW, NULL},
   {N_("/_Action/_Focus/_Preview"),        "<shift><alt>P", cb_focus, PREVIEW,        NULL},
   {N_("/_Action/_Focus/_Location Entry"), "<shift><alt>L", cb_focus, LOCATION_ENTRY, NULL},
#if 0
   {N_("/_Action/_Directory View"),        NULL,            NULL,     0,              "<Branch>"},
   {N_("/_Action/_Thumbnail View"),        NULL,            NULL,     0,              "<Branch>"},
   {N_("/_Action/_Preview"),               NULL,            NULL,     0,              "<Branch>"},
   {N_("/_Action/_Preview/_Next"),         NULL,            cb_preview_move,     0,   NULL},
   {N_("/_Action/_Preview/_Previous"),     NULL,            cb_preview_move,     0,   NULL},
#endif

   {N_("/Too_l"),                      NULL,         NULL,              0,              "<Branch>"},
   {N_("/Too_l/_Clear all cache"),     NULL,         cb_clear_cache,    CLEAR_CACHE_ALL, NULL},
   {N_("/Too_l/---"),                  NULL,         NULL,              0,              "<Separator>"},
   {N_("/Too_l/_Find duplicates"),     NULL,         NULL,              0,              "<Branch>"},
   {N_("/Too_l/---"),                  NULL,         NULL,              0,              "<Separator>"},
   {N_("/Too_l/_Wallpaper setting"),   NULL,         NULL,              0,              "<Branch>"},

   {N_("/_Help"), NULL, NULL, 0, "<Branch>"},
   {NULL, NULL, NULL, 0, NULL},
};


/* for "View" sub menu */
GtkItemFactoryEntry gimv_thumb_win_view_items [] =
{
   {N_("/_Sort File List"),        NULL,         NULL,                 0,              "<Branch>"},
   {N_("/_Layout"),                NULL,         NULL,                 0,              "<Branch>"},  
   {N_("/_Layout/Layout_0"),       "<control>0", cb_switch_layout,     0,              "<RadioItem>"},  
   {N_("/_Layout/Layout_1"),       "<control>1", cb_switch_layout,     1,              "/Layout/Layout0"},  
   {N_("/_Layout/Layout_2"),       "<control>2", cb_switch_layout,     2,              "/Layout/Layout1"},  
   {N_("/_Layout/Layout_3"),       "<control>3", cb_switch_layout,     3,              "/Layout/Layout2"},  
   {N_("/_Layout/Layout_4"),       "<control>4", cb_switch_layout,     4,              "/Layout/Layout3"},  
   {N_("/_Layout/_Custom"),        NULL,         cb_switch_layout,     -1,             "/Layout/Layout4"},  
   {N_("/_Layout/---"),            NULL,         NULL,                 0,              "<Separator>"},
   {N_("/_Layout/Window _Composition"), NULL,    NULL,                 0,              "<Branch>"},  

   {N_("/---"),                    NULL,         NULL,                 0,              "<Separator>"},

   {N_("/S_lideshow"),            "<control>S",  cb_slideshow,         0,              NULL},
   {N_("/Slideshow Opt_ions"),    NULL,          NULL,                 0,                       "<Branch>"},  
   {N_("/Slideshow Opt_ions/Start from the _first"),   NULL, cb_slideshow_order,     SLIDESHOW_START_FROM_FIRST,    "<RadioItem>"},
   {N_("/Slideshow Opt_ions/Start from the se_lected"),NULL, cb_slideshow_order,     SLIDESHOW_START_FROM_SELECTED, "/Slideshow Options/Start from the first"},
   {N_("/Slideshow Opt_ions/_Random order"),           NULL, cb_slideshow_order,     SLIDESHOW_RANDOM_ORDER,        "/Slideshow Options/Start from the selected"},
   {N_("/Slideshow Opt_ions/---"),                     NULL, NULL,                   0,                             "<Separator>"},
   {N_("/Slideshow Opt_ions/_Selected only"),          NULL, cb_slideshow_selected,  0,                             "<CheckItem>"},
   {N_("/Slideshow Opt_ions/---"),                     NULL, NULL,                   0,                             "<Separator>"},
   {N_("/Slideshow Opt_ions/Images _and movies"),      NULL, cb_slideshow_file_type, SLIDESHOW_IMAGES_AND_MOVIES,   "<RadioItem>"},
   {N_("/Slideshow Opt_ions/_Images only"),            NULL, cb_slideshow_file_type, SLIDESHOW_IMAGES_ONLY,         "/Slideshow Options/Images and movies"},
   {N_("/Slideshow Opt_ions/_Movies only"),            NULL, cb_slideshow_file_type, SLIDESHOW_MOVIES_ONLY,         "/Slideshow Options/Images only"},

   {N_("/---"),                    NULL,         NULL,                 0,              "<Separator>"},

   {N_("/_Directory View"),        "<shift>D",   cb_toggle_view,       GIMV_COM_DIR_VIEW,   "<ToggleItem>"},
   {N_("/_Preview"),               "<shift>P",   cb_toggle_view,       GIMV_COM_IMAGE_VIEW, "<ToggleItem>"},
   {N_("/_Menu Bar"),              "M",          cb_toggle_show,       MENUBAR,        "<ToggleItem>"},
   {N_("/T_ool Bar"),              "N",          cb_toggle_show,       TOOLBAR,        "<CheckItem>"},
   {N_("/Dir_View Tool Bar"),      "<shift>N",   cb_toggle_show,       DIR_TOOLBAR,    "<CheckItem>"},
   {N_("/St_atus Bar"),            "V",          cb_toggle_show,       STATUSBAR,      "<CheckItem>"},
   {N_("/_Tab"),                   "B",          cb_toggle_show,       TAB,            "<CheckItem>"},
   {N_("/Ta_b (Preview)"),         "<shift>Z",   cb_toggle_show,       PREVIEW_TAB,    "<CheckItem>"},
   {N_("/_Player"),                NULL,         NULL,                 0,              "<Branch>"},  
   {N_("/_Player/_Show"),          NULL,         cb_switch_player,     GimvImageViewPlayerVisibleShow, "<RadioItem>"},  
   {N_("/_Player/_Hide"),          NULL,         cb_switch_player,     GimvImageViewPlayerVisibleHide, "/Player/Show"},  
   {N_("/_Player/_Auto"),          NULL,         cb_switch_player,     GimvImageViewPlayerVisibleAuto, "/Player/Hide"},  

   {N_("/---"),                    NULL,         NULL,                 0,              "<Separator>"},

   {N_("/Ma_ximize"),              "F",          cb_toggle_maximize,   0,              "<ToggleItem>"},
   /* {N_("/_Full Screen"),        "F",          cb_toggle_fullscreen, 0,              "<ToggleItem>"}, */
   {NULL, NULL, NULL, 0, NULL},
};


/* for "Sort File List" sub menu */
GtkItemFactoryEntry gimv_thumb_win_sort_items [] =
{
   {N_("/by _Name"),               NULL, cb_sort_item,      GIMV_SORT_NAME,   "<RadioItem>"},
   {N_("/by _Access Time"),        NULL, cb_sort_item,      GIMV_SORT_ATIME,  "/by Name"},
   {N_("/by _Modification Time"),  NULL, cb_sort_item,      GIMV_SORT_MTIME,  "/by Access Time"},
   {N_("/by _Change Time"),        NULL, cb_sort_item,      GIMV_SORT_CTIME,  "/by Modification Time"},
   {N_("/by _Size"),               NULL, cb_sort_item,      GIMV_SORT_SIZE,   "/by Change Time"},
   {N_("/by _Type"),               NULL, cb_sort_item,      GIMV_SORT_TYPE,   "/by Size"},
   {N_("/by Image _Width"),        NULL, cb_sort_item,      GIMV_SORT_WIDTH,  "/by Type"},
   {N_("/by Image _Height"),       NULL, cb_sort_item,      GIMV_SORT_HEIGHT, "/by Image Width"},
   {N_("/by Image Ar_ea"),         NULL, cb_sort_item,      GIMV_SORT_AREA,   "/by Image Height"},
   {N_("/---"),                    NULL, NULL,              0,                "<Separator>"},
   {N_("/_Reverse Order"),         NULL, cb_sort_flag,      GIMV_SORT_REVERSE,"<CheckItem>"},
   {N_("/---"),                    NULL, NULL,              0,                "<Separator>"},
   {N_("/Case _insensitive"),      NULL, cb_sort_flag,      GIMV_SORT_CASE_INSENSITIVE, "<CheckItem>"},
   {N_("/_Directory insensitive"), NULL, cb_sort_flag,      GIMV_SORT_DIR_INSENSITIVE,  "<CheckItem>"},
   {NULL, NULL, NULL, 0, NULL},
};


/* for layout */
GimvThumbwinComposeType compose_type[] =
{
   {TRUE,  FALSE, FALSE, {GIMV_COM_DIR_VIEW,   GIMV_COM_THUMB_VIEW, GIMV_COM_IMAGE_VIEW}},
   {TRUE,  FALSE, TRUE,  {GIMV_COM_IMAGE_VIEW, GIMV_COM_DIR_VIEW,   GIMV_COM_THUMB_VIEW}},
   {FALSE, TRUE,  TRUE,  {GIMV_COM_IMAGE_VIEW, GIMV_COM_DIR_VIEW,   GIMV_COM_THUMB_VIEW}},
   {TRUE,  FALSE, TRUE,  {GIMV_COM_THUMB_VIEW, GIMV_COM_DIR_VIEW,   GIMV_COM_IMAGE_VIEW}},
   {FALSE, TRUE,  TRUE,  {GIMV_COM_THUMB_VIEW, GIMV_COM_DIR_VIEW,   GIMV_COM_IMAGE_VIEW}},
};
gint compose_type_num = sizeof (compose_type) / sizeof (compose_type[0]);


static gboolean win_comp[][3] = {
   {TRUE,  FALSE, FALSE},
   {FALSE, TRUE,  FALSE},
   {TRUE,  FALSE, TRUE},
   {FALSE, TRUE,  TRUE},
   {FALSE, FALSE, FALSE},
   {TRUE,  TRUE,  FALSE},
};


static gint win_comp_icon[][12] = {
   {0, 1, 0, 2, 1, 2, 0, 1, 1, 2, 1, 2},
   {0, 2, 0, 1, 0, 1, 1, 2, 1, 2, 1, 2},
   {0, 1, 0, 1, 1, 2, 0, 2, 0, 1, 1, 2},
   {0, 1, 0, 1, 1, 2, 0, 1, 0, 2, 1, 2},
   {0, 1, 0, 1, 0, 1, 1, 2, 0, 1, 2, 3},
   {0, 1, 0, 1, 1, 2, 0, 1, 2, 3, 0, 1},
};


static GList *ThumbWinList = NULL;


G_DEFINE_TYPE (GimvThumbWin, gimv_thumb_win, GTK_TYPE_WINDOW)


static void
gimv_thumb_win_class_init (GimvThumbWinClass *klass)
{
   GObjectClass *gobject_class;
   GtkWidgetClass *widget_class;

   gobject_class = (GObjectClass *) klass;
   widget_class  = (GtkWidgetClass *) klass;

   gobject_class->dispose        = gimv_thumb_win_dispose;

   widget_class->realize         = gimv_thumb_win_realize;
   widget_class->delete_event    = gimv_thumb_win_delete_event;
}


static void
gimv_thumb_win_init (GimvThumbWin *tw)
{
   GtkWidget *vbox, *hbox;
   GtkWidget *separator;
   GtkWidget *entry;
   gint num, summary_mode;
   gboolean loading;
   gchar **summary_mode_labels = NULL;

   tw->dv            = NULL;
   tw->iv            = NULL;
   tw->cv            = NULL;
   tw->filenum       = 0;
   tw->pagenum       = 0;
   tw->filesize      = 0;
   tw->newpage_count = 0;
   tw->open_dialog   = NULL;
   tw->pane_size1    = conf.thumbwin_pane_size1;
   tw->pane_size2    = conf.thumbwin_pane_size2;
   tw->show_dirview  = conf.thumbwin_show_dir_view;
   tw->show_preview  = conf.thumbwin_show_preview;
   tw->player_visible= conf.preview_player_visible;
   tw->layout_type   = conf.thumbwin_layout_type;
   tw->tab_pos       = conf.thumbwin_tab_pos;
   tw->sortitem      = conf.thumbwin_sort_type;
   tw->sortflags     = 0;
   if (conf.thumbwin_sort_reverse)
      tw->sortflags |= GIMV_SORT_REVERSE;
   if (conf.thumbwin_sort_ignore_case)
      tw->sortflags |= GIMV_SORT_CASE_INSENSITIVE;
   if (conf.thumbwin_sort_ignore_dir)
      tw->sortflags |= GIMV_SORT_DIR_INSENSITIVE;
   tw->changing_layout = FALSE;

   tw->priv = g_new0 (GimvThumbWinPriv, 1);
   tw->priv->slideshow_order         = SLIDESHOW_START_FROM_FIRST;
   tw->priv->slideshow_selected_only = FALSE;
   tw->priv->slideshow_file_type     = SLIDESHOW_IMAGES_AND_MOVIES;

   ThumbWinList = g_list_append (ThumbWinList, tw);

   tw->accel_group_list = NULL;

   /* window */
   gtk_widget_set_name (GTK_WIDGET (tw), "ThumbWin");
   gtk_window_set_title (GTK_WINDOW (tw),
                         _(GIMV_PROG_NAME" -Thumbnail Window-"));
   gtk_window_set_wmclass(GTK_WINDOW(tw), "thumbwin", "GImageView");
   gtk_window_set_default_size (GTK_WINDOW(tw),
                                conf.thumbwin_width, conf.thumbwin_height);
   gtk_window_set_policy(GTK_WINDOW(tw), TRUE, TRUE, FALSE);

   /* Main vbox */
   vbox = gtk_vbox_new (FALSE, 0);
   tw->main_vbox = vbox;
   gtk_container_add (GTK_CONTAINER (tw), vbox);
   gtk_widget_show (vbox);

   /* Menu Bar */
   tw->menubar_handle = gtk_handle_box_new();
   gtk_widget_set_name (tw->menubar_handle, "MenuBarContainer");
   gtk_container_set_border_width (GTK_CONTAINER(tw->menubar_handle), 2);
   gtk_box_pack_start(GTK_BOX(vbox), tw->menubar_handle, FALSE, FALSE, 0);
   gtk_widget_show (tw->menubar_handle);

   /* toolbar */
   tw->toolbar_handle = gtk_handle_box_new();
   gtk_widget_set_name (tw->toolbar_handle, "ToolBarContainer");
   gtk_container_set_border_width (GTK_CONTAINER(tw->toolbar_handle), 2);
   gtk_box_pack_start(GTK_BOX(vbox), tw->toolbar_handle, FALSE, FALSE, 0);
   gtk_widget_show (tw->toolbar_handle);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_container_add(GTK_CONTAINER(tw->toolbar_handle), hbox);
   gtk_widget_show (hbox);

   tw->toolbar = create_toolbar (tw, tw->toolbar_handle);
   gtk_box_pack_start (GTK_BOX (hbox), tw->toolbar, FALSE, FALSE, 0);
   gtk_widget_show (tw->toolbar);
   gtk_toolbar_set_style (GTK_TOOLBAR(tw->toolbar), conf.thumbwin_toolbar_style);

   /* location entry */
   tw->location_entry = entry = gtk_entry_new ();
   gtk_widget_set_name (tw->location_entry, "LocationEntry");
   gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
   g_signal_connect_after (G_OBJECT(entry), "key-press-event",
                           G_CALLBACK(cb_location_entry_key_press), tw);
   dnd_dest_set (tw->location_entry, dnd_types_uri, dnd_types_uri_num);
   g_signal_connect(G_OBJECT (tw->location_entry), "drag_data_received",
                    G_CALLBACK (cb_location_entry_drag_data_received), tw);
   g_signal_connect(G_OBJECT (tw->location_entry), "activate",
                    G_CALLBACK (cb_location_entry_enter), tw);
#if 1 /* FIXME */
   g_signal_connect(G_OBJECT (tw->location_entry), "focus_in_event",
                    G_CALLBACK (cb_focus_in), tw);
   g_signal_connect(G_OBJECT (tw->location_entry), "focus_out_event",
                    G_CALLBACK (cb_focus_out), tw);
#endif
   gtk_widget_show (entry);

   /* option menu for display mode */
   if (!summary_mode_labels)
      summary_mode_labels = gimv_thumb_view_get_summary_mode_labels (&num);

   summary_mode = gimv_thumb_view_label_to_num (conf.thumbwin_disp_mode);
   tw->thumbview_summary_mode = gimv_thumb_view_num_to_label (summary_mode);
   if (summary_mode < 0)
      summary_mode = gimv_thumb_view_label_to_num (GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE);

   tw->summary_mode_menu = create_option_menu ((const gchar **)summary_mode_labels,
                                               summary_mode,
                                               cb_display_mode_menu, tw);
   gtk_widget_set_name (tw->summary_mode_menu, "DispModeOptionMenu");
   gtk_box_pack_end (GTK_BOX (hbox), tw->summary_mode_menu, FALSE, FALSE, 0);
   gtk_widget_show (tw->summary_mode_menu);

   /* separator */
   separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
   gtk_widget_show (separator);

   /* main contents */
   tw->main_contents = thumbnail_window_contents_new (tw);
   gtk_widget_set_name (tw->main_contents, "MainArea");
   gtk_container_add (GTK_CONTAINER (vbox), tw->main_contents);
   gtk_widget_show (tw->main_contents);

   /* status bar */
   hbox = gtk_hbox_new (FALSE, 0);
   gtk_widget_set_name (hbox, "StatusBarContainer");
   tw->status_bar_container = hbox;
   gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show (hbox);

   tw->status_bar1 = gtk_statusbar_new ();
   gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(tw->status_bar1), FALSE);
   gtk_widget_set_name (tw->status_bar1, "StatusBar1");
   gtk_container_border_width (GTK_CONTAINER (tw->status_bar1), 1);
   gtk_widget_set_size_request(tw->status_bar1, 150, -1);
   gtk_box_pack_start (GTK_BOX (hbox), tw->status_bar1, TRUE, TRUE, 0);
   gtk_widget_show (tw->status_bar1);
   gtk_statusbar_push(GTK_STATUSBAR (tw->status_bar1), 1, _("New Window"));

   tw->status_bar2 = gtk_statusbar_new ();
   gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(tw->status_bar2), FALSE);
   gtk_widget_set_name (tw->status_bar2, "StatusBar2");
   gtk_container_border_width (GTK_CONTAINER (tw->status_bar2), 1);
   gtk_widget_set_size_request(tw->status_bar2, 150, -1);
   gtk_box_pack_start (GTK_BOX (hbox), tw->status_bar2, TRUE, TRUE, 0);
   gtk_widget_show (tw->status_bar2);

   tw->progressbar = gtk_progress_bar_new();
   gtk_widget_set_name (tw->progressbar, "ProgressBar");
   gtk_box_pack_end (GTK_BOX (hbox), tw->progressbar, FALSE, FALSE, 0);
   gtk_widget_show (tw->progressbar);

   /* create menus */
   create_gimv_thumb_win_menus (tw);
   sync_widget_state_to_menu (tw);

   /* create new tab */
   /* gimv_thumb_win_create_new_tab (tw); */

   loading = files_loader_query_loading ();
   if (loading) gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING);

#if 1   /* FIXME */
   if (tw->accel_group_list)
      g_slist_free (tw->accel_group_list);
   tw->accel_group_list
      = g_slist_copy (gtk_accel_groups_from_object (G_OBJECT (tw)));
   tw->accel_group_list = g_slist_reverse (tw->accel_group_list);
#endif
}


GtkWidget *
gimv_thumb_win_new (void)
{
   GimvThumbWin *tw = g_object_new (GIMV_TYPE_THUMB_WIN, NULL);

   return GTK_WIDGET (tw);
}


GimvThumbWin *
gimv_thumb_win_open_window (void)
{
   GimvThumbWin *tw = GIMV_THUMB_WIN (gimv_thumb_win_new());

   gtk_widget_show (GTK_WIDGET (tw));

   return tw;
}


static void
gimv_thumb_win_dispose (GObject *object)
{
   GimvThumbWin *tw = GIMV_THUMB_WIN (object);
   GimvThumbView *tv;
   GList *node;

   /* for inhibiting Gtk-WARNING */
   tw->location_entry = NULL;
   tw->button.size_spin = NULL;
   tw->summary_mode_menu = NULL;

   node = g_list_first (gimv_thumb_view_get_list());
   while (node) {
      tv = node->data;
      node = g_list_next (node);

      if (tv->tw == tw) {
         if (tv->progress) {
            tv->progress->status = WINDOW_DESTROYED;
         } else {
            g_object_unref (G_OBJECT (tv));
         }
      }

   }

   ThumbWinList = g_list_remove (ThumbWinList, tw);

   if (tw->priv) {
      g_free(tw->priv);
      tw->priv = NULL;
   }

   if (G_OBJECT_CLASS (gimv_thumb_win_parent_class)->dispose)
      G_OBJECT_CLASS (gimv_thumb_win_parent_class)->dispose (object);

   /* quit when last window */
   if (!gimv_image_win_get_list() && !gimv_thumb_win_get_list()) {
      gimv_quit();
   }
}


static void
gimv_thumb_win_realize (GtkWidget *widget)
{
   GimvThumbWin *tw = GIMV_THUMB_WIN (widget);

   if (GTK_WIDGET_CLASS (gimv_thumb_win_parent_class)->realize)
      GTK_WIDGET_CLASS (gimv_thumb_win_parent_class)->realize (widget);

   gimv_icon_stock_set_window_icon (GTK_WIDGET(tw)->window, "gimv_icon");
   gtk_widget_grab_focus (tw->dv->dirtree);
}


static gboolean
gimv_thumb_win_delete_event (GtkWidget *widget, GdkEventAny *event)
{
   GimvThumbWin *tw = GIMV_THUMB_WIN (widget);
   GimvThumbView *tv;
   GList *node;

   if (GTK_WIDGET_CLASS (gimv_thumb_win_parent_class)->delete_event)
      if (GTK_WIDGET_CLASS (gimv_thumb_win_parent_class)->delete_event (widget, event))
         return TRUE;

   if (tw->status == GIMV_THUMB_WIN_STATUS_CHECKING_DUPLICATE) return TRUE;

   node = g_list_first (gimv_thumb_view_get_list());
   while (node) {
      tv = node->data;
      node = g_list_next (node);

      if (tv->tw == tw) {
         /* do not destroy this window while loading */
         if (tv->progress) {
            return TRUE;
         }
      }
   }

   if (g_list_length (ThumbWinList) == 1 && conf.thumbwin_save_win_state)
      gimv_thumb_win_save_state (tw);

   return FALSE;
}


static GtkWidget *
create_composition_icon (gint type)
{
   GtkWidget *hbox, *table, *button, *label;
   gint i, comp[4], n_types;
   gchar buf[128];

   n_types = sizeof (win_comp_icon) / sizeof (gint) / 12;
   if (type > n_types || type < 0)
      return NULL;

   if (type < 4)
      table = gtk_table_new (2, 2, FALSE);
   else if (type == 4)
      table = gtk_table_new (3, 1, FALSE);
   else if (type == 5)
      table = gtk_table_new (1, 3, FALSE);
   else
      return NULL;

   gtk_widget_set_size_request (table, 24, 24);
   gtk_widget_show (table);

   hbox = gtk_hbox_new (FALSE, 0);
   gtk_widget_show (hbox);
   gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 2);

   for (i = 0; i < 3; i++) {
      comp[0] = win_comp_icon[type][i * 4];
      comp[1] = win_comp_icon[type][i * 4 + 1];
      comp[2] = win_comp_icon[type][i * 4 + 2];
      comp[3] = win_comp_icon[type][i * 4 + 3];
      button = gtk_button_new ();
      gtk_widget_set_sensitive (button, FALSE);
      GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
      gtk_table_attach_defaults (GTK_TABLE (table), button,
                                 comp[0], comp[1], comp[2], comp[3]);
      gtk_widget_show (button);
   }

   g_snprintf (buf, 128, _("Composition %d"), type);
   label = gtk_label_new (buf);
   gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

   return hbox;
}


static GtkWidget *
create_wincomp_sub_menu (GimvThumbWin *tw)
{
   GtkWidget *menu, *item, *icon;
   /* GSList *group = NULL; */
   gint i;

   menu = gtk_menu_new();
   gtk_widget_show (menu);

   for (i = 0; i < 6; i++) {
      /* item = gtk_menu_item_new (group); */
      item = gtk_menu_item_new ();
      g_object_set_data (G_OBJECT (item), "num", GINT_TO_POINTER (i));
      icon = create_composition_icon (i);
      gtk_container_add (GTK_CONTAINER (item), icon);
      g_signal_connect (G_OBJECT (item), "activate",
                        G_CALLBACK (cb_win_composition_menu),
                        tw);
      /* group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item)); */
      gtk_menu_append (GTK_MENU (menu), item);
      gtk_widget_show (item);
   }

   return menu;
}


static void
cb_file_submenu_show (GtkWidget *widget, GimvThumbWin *tw)
{
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;

   g_return_if_fail (tw);

   if (tw->pagenum < 1) {
      ifactory = gtk_item_factory_from_widget (tw->menubar);
      menuitem = gtk_item_factory_get_item (ifactory, "/File/Close Tab");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }
}


static void
cb_file_submenu_hide (GtkWidget *widget, GimvThumbWin *tw)
{
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;

   g_return_if_fail (tw);

   ifactory = gtk_item_factory_from_widget (tw->menubar);
   menuitem = gtk_item_factory_get_item (ifactory, "/File/Close Tab");
   gtk_widget_set_sensitive (menuitem, TRUE);
}


static void
cb_edit_submenu_show (GtkWidget *widget, GimvThumbWin *tw)
{
   GimvThumbView *tv;
   GList *list = NULL;
   gint len;

   g_return_if_fail (tw);
   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv || tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      if (!tv) {
         gtk_widget_set_sensitive (tw->menuitem.select,   FALSE);
         gtk_widget_set_sensitive (tw->menuitem.unselect, FALSE);
         gtk_widget_set_sensitive (tw->menuitem.refresh,  FALSE);
         gtk_widget_set_sensitive (tw->menuitem.reload,   FALSE);
         gtk_widget_set_sensitive (tw->menuitem.recreate, FALSE);
      }
      gtk_widget_set_sensitive (tw->menuitem.rename,   FALSE);
      gtk_widget_set_sensitive (tw->menuitem.copy,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.move,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.link,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.delete,   FALSE);
   }

   if (!tv) return;

   list = gimv_thumb_view_get_selection_list (tv);

   if (list) {
      len = g_list_length (list);
      if (len > 1) {
         gtk_widget_set_sensitive (tw->menuitem.rename,   FALSE);
      }
      if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
         const gchar *dirname = gimv_thumb_view_get_path (tv);
         if (!iswritable (dirname)) {
            gtk_widget_set_sensitive (tw->menuitem.rename,   FALSE);
            gtk_widget_set_sensitive (tw->menuitem.move,     FALSE);
            gtk_widget_set_sensitive (tw->menuitem.delete,   FALSE);
         } else if (tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
            /* FIXME */
         }
      }
   } else {
      gtk_widget_set_sensitive (tw->menuitem.rename,   FALSE);
      gtk_widget_set_sensitive (tw->menuitem.copy,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.move,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.link,     FALSE);
      gtk_widget_set_sensitive (tw->menuitem.delete,   FALSE);
   }

   if (list)
      g_list_free (list);
}


static void
cb_edit_submenu_hide (GtkWidget *widget, GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_widget_set_sensitive (tw->menuitem.select,   TRUE);
   gtk_widget_set_sensitive (tw->menuitem.unselect, TRUE);
   gtk_widget_set_sensitive (tw->menuitem.refresh,  TRUE);
   gtk_widget_set_sensitive (tw->menuitem.reload,   TRUE);
   gtk_widget_set_sensitive (tw->menuitem.recreate, TRUE);
   gtk_widget_set_sensitive (tw->menuitem.rename,   TRUE);
   gtk_widget_set_sensitive (tw->menuitem.copy,     TRUE);
   gtk_widget_set_sensitive (tw->menuitem.move,     TRUE);
   gtk_widget_set_sensitive (tw->menuitem.link,     TRUE);
   gtk_widget_set_sensitive (tw->menuitem.delete,   TRUE);
}


static void
cb_tool_submenu_show (GtkWidget *widget, GimvThumbWin *tw)
{
   GimvThumbView *tv;
   GtkItemFactory *ifactory;
   GtkWidget /* *menuitem1, */ *menuitem2;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) {
      /* gtk_widget_set_sensitive (tw->menuitem.find_sim, FALSE); */
   }

   ifactory = gtk_item_factory_from_widget (tw->menubar);
   /* menuitem1 = gtk_item_factory_get_item (ifactory, "/Tool/Clear cache for this tab"); */
   menuitem2 = gtk_item_factory_get_item (ifactory, "/Tool/Clear all cache");

   if (!gimv_thumb_cache_can_clear (NULL, conf.cache_write_type))
       gtk_widget_set_sensitive (menuitem2, FALSE);

   /*
   if (tw->pagenum < 1 || !tv) {
      gtk_widget_set_sensitive (menuitem1, FALSE);
   } else {
      const gchar *path = thumbview_get_path (tv);

      if (path && *path && !thumbsupport_can_clear_cache (path, conf.cache_write_type))
          gtk_widget_set_sensitive (menuitem1, FALSE);
   }
   */
}


static void
cb_tool_submenu_hide (GtkWidget *widget, GimvThumbWin *tw)
{
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;

   g_return_if_fail (tw);

   /* gtk_widget_set_sensitive (tw->menuitem.find_sim, TRUE); */

   ifactory = gtk_item_factory_from_widget (tw->menubar);
   /*
   menuitem = gtk_item_factory_get_item (ifactory, "/Edit/Clear cache for this tab");
   gtk_widget_set_sensitive (menuitem,     TRUE);
   */
   menuitem = gtk_item_factory_get_item (ifactory, "/Tool/Clear all cache");
   gtk_widget_set_sensitive (menuitem, TRUE);
}


static void
cb_view_submenu_show (GtkWidget *widget, GimvThumbWin *tw)
{
   GimvThumbView *tv;
   /*
     GtkItemFactory *ifactory;
     GtkWidget *menuitem;
   */

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);

   /*
     if (!tv || tv->mode == THUMB_VIEW_MODE_ARCHIVE) {
     ifactory = gtk_item_factory_from_widget (tw->view_menu);
     menuitem = gtk_item_factory_get_item (ifactory, "/Slideshow");
     gtk_widget_set_sensitive (menuitem, FALSE);
     }
   */
}


static void
cb_view_submenu_hide (GtkWidget *widget, GimvThumbWin *tw)
{
   /*
     GtkItemFactory *ifactory;
     GtkWidget *menuitem;

     ifactory = gtk_item_factory_from_widget (tw->view_menu);
     menuitem = gtk_item_factory_get_item (ifactory, "/Slideshow");
     gtk_widget_set_sensitive (menuitem, TRUE);
   */
}


static void
cb_tab_submenu_show (GtkWidget *widget, GimvThumbWin *tw)
{
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;

   g_return_if_fail (tw);

   ifactory = gtk_item_factory_from_widget (tw->menubar);

   if (tw->pagenum < 1 || !gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE)) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Detach tab");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }

   if (tw->pagenum < 2) {
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/First Page");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Last Page");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Next Page");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Previous Page");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Move tab forward");
      gtk_widget_set_sensitive (menuitem, FALSE);
      menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Move tab backward");
      gtk_widget_set_sensitive (menuitem, FALSE);
   }
}


static void
cb_tab_submenu_hide (GtkWidget *widget, GimvThumbWin *tw)
{
   GtkItemFactory *ifactory;
   GtkWidget *menuitem;

   g_return_if_fail (tw);

   ifactory = gtk_item_factory_from_widget (tw->menubar);

   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/First Page");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Last Page");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Next Page");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Previous Page");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Move tab forward");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Move tab backward");
   gtk_widget_set_sensitive (menuitem, TRUE);
   menuitem = gtk_item_factory_get_item (ifactory, "/Tab/Detach tab");
   gtk_widget_set_sensitive (menuitem, TRUE);
}


#warning should be implemented as customizable launcher.
static void
cb_wallpaper_setting (GtkWidget *menuitem, GimvThumbWin *tw)
{
   const gchar *str;
   gchar *cmd;

   str = g_object_get_data (G_OBJECT (menuitem), "command");
   if (!str) return;

   cmd = g_strconcat (str, " &", NULL);
   system (cmd);

   g_free (cmd);
}


static GtkWidget *
create_wallpaper_submenu (GimvThumbWin *tw)
{
   GtkWidget *menu;
   GtkWidget *menu_item;
   gint i;
   gchar **menus, **pair;

   menus = g_strsplit (conf.wallpaper_menu, ";", -1);
   if (!menus) return NULL;

   menu = gtk_menu_new();

   /* count items num */
   for (i = 0; menus[i]; i++) {
      if (!menus[i] || !*menus[i]) continue;

      pair = g_strsplit (menus[i], ",", 2);

      if (pair && pair[0] && pair[1]) {
         menu_item = gtk_menu_item_new_with_label (pair[0]);
         g_object_set_data_full (G_OBJECT (menu_item), "command",
                                 g_strdup (pair[1]),
                                 (GtkDestroyNotify) g_free);
         g_signal_connect (G_OBJECT (menu_item), "activate",
                           G_CALLBACK (cb_wallpaper_setting), tw);
         gtk_menu_append (GTK_MENU (menu), menu_item);
         gtk_widget_show (menu_item);
      }

      g_strfreev (pair);
   }

   g_strfreev (menus);

   return menu;
}


static void
create_gimv_thumb_win_menus (GimvThumbWin *tw)
{
   static GtkItemFactoryEntry *entries = NULL;
   GtkWidget *item, *dupmenu;
   GtkItemFactory *ifactory;
   guint n_menu_items, layout;
   GtkWidget *submenu, *help;
   const gchar **labels;
   gint i = 0;

   /* main menu */
   n_menu_items = sizeof (gimv_thumb_win_menu_items)
      / sizeof (gimv_thumb_win_menu_items[0]) - 1;
   tw->menubar = menubar_create (GTK_WIDGET (tw), gimv_thumb_win_menu_items,
                                 n_menu_items, "<ThumbWinMainMenu>", tw);

   gtk_container_add(GTK_CONTAINER(tw->menubar_handle), tw->menubar);
   gtk_widget_show (tw->menubar);

   /* sub menu */
   n_menu_items = sizeof(gimv_thumb_win_view_items)
      / sizeof(gimv_thumb_win_view_items[0]) - 1;
   tw->view_menu = menu_create_items(GTK_WIDGET (tw), gimv_thumb_win_view_items,
                                     n_menu_items, "<ThumbWinViewSubMenu>", tw);
   n_menu_items = sizeof(gimv_thumb_win_sort_items)
      / sizeof(gimv_thumb_win_sort_items[0]) - 1;
   tw->sort_menu = menu_create_items(GTK_WIDGET (tw), gimv_thumb_win_sort_items,
                                     n_menu_items, "<ThumbWinSortSubMenu>", tw);
   tw->comp_menu = create_wincomp_sub_menu (tw);
   help = gimvhelp_create_menu (GTK_WIDGET (tw));

   /* duplicates finder sub menu */
   labels = gimv_dupl_finder_get_algol_types ();
   for (i = 0; labels && labels[i]; i++) ;
   if (!entries && i > 0) {
      entries = g_new0 (GtkItemFactoryEntry, i);
      for (i = 0; labels[i]; i++) {
         gchar tempstr[BUF_SIZE];

         g_snprintf (tempstr, BUF_SIZE, "/%s", _(labels[i]));
         entries[i].path            = g_strdup (tempstr);
         entries[i].accelerator     = NULL;
         entries[i].callback        = cb_find_similar;
         entries[i].callback_action = i;
         entries[i].item_type       = NULL;
      }
   }
   if (entries) {
      dupmenu = menu_create_items (GTK_WIDGET (tw), entries,
                                   i, "<ThumbWinDuplicatesFinderSubMenu>", tw);
      menu_set_submenu (tw->menubar, "/Tool/Find duplicates", dupmenu);
   }

   /* attach sub menus to parent menu */
   menu_set_submenu (tw->menubar,   "/View", tw->view_menu);
   menu_set_submenu (tw->menubar,   "/Help", help);
   menu_set_submenu (tw->view_menu, "/Sort File List", tw->sort_menu);
   menu_set_submenu (tw->view_menu, "/Layout/Window Composition", tw->comp_menu);
   submenu = create_wallpaper_submenu (tw);
   if (submenu)
      menu_set_submenu (tw->menubar, "/Tool/Wallpaper setting", submenu);

   ifactory = gtk_item_factory_from_widget (tw->menubar);
   tw->menuitem.file = gtk_item_factory_get_item (ifactory, "/File");
   tw->menuitem.edit = gtk_item_factory_get_item (ifactory, "/Edit");
   tw->menuitem.view = gtk_item_factory_get_item (ifactory, "/View");
   tw->menuitem.tool = gtk_item_factory_get_item (ifactory, "/Tool");

   tw->menuitem.select     = gtk_item_factory_get_item (ifactory, "/Edit/Select All");
   tw->menuitem.unselect   = gtk_item_factory_get_item (ifactory, "/Edit/Unselect All");
   tw->menuitem.refresh    = gtk_item_factory_get_item (ifactory, "/Edit/Refresh List");
   tw->menuitem.reload     = gtk_item_factory_get_item (ifactory, "/Edit/Reload Cache");
   tw->menuitem.recreate   = gtk_item_factory_get_item (ifactory, "/Edit/Update All Thumbnail");

   tw->menuitem.rename     = gtk_item_factory_get_item (ifactory, "/Edit/Rename...");
   tw->menuitem.copy       = gtk_item_factory_get_item (ifactory, "/Edit/Copy Files To...");
   tw->menuitem.move       = gtk_item_factory_get_item (ifactory, "/Edit/Move Files To...");
   tw->menuitem.link       = gtk_item_factory_get_item (ifactory, "/Edit/Link Files To...");
   tw->menuitem.delete     = gtk_item_factory_get_item (ifactory, "/Edit/Remove file...");

   tw->menuitem.find_sim   = gtk_item_factory_get_item (ifactory, "/Tool/Find duplicates");

   item =  gtk_item_factory_get_item (ifactory, "/Tab");

   ifactory = gtk_item_factory_from_widget (tw->view_menu);
   tw->menuitem.layout[0]   = gtk_item_factory_get_item (ifactory, "/Layout/Layout0");
   tw->menuitem.layout[1]   = gtk_item_factory_get_item (ifactory, "/Layout/Layout1");
   tw->menuitem.layout[2]   = gtk_item_factory_get_item (ifactory, "/Layout/Layout2");
   tw->menuitem.layout[3]   = gtk_item_factory_get_item (ifactory, "/Layout/Layout3");
   tw->menuitem.layout[4]   = gtk_item_factory_get_item (ifactory, "/Layout/Layout4");
   tw->menuitem.layout[5]   = gtk_item_factory_get_item (ifactory, "/Layout/Custom");
   tw->menuitem.dirview     = gtk_item_factory_get_item (ifactory, "/Directory View");
   tw->menuitem.preview     = gtk_item_factory_get_item (ifactory, "/Preview");
   tw->menuitem.menubar     = gtk_item_factory_get_item (ifactory, "/Menu Bar");
   tw->menuitem.toolbar     = gtk_item_factory_get_item (ifactory, "/Tool Bar");
   tw->menuitem.dir_toolbar = gtk_item_factory_get_item (ifactory, "/DirView Tool Bar");
   tw->menuitem.statusbar   = gtk_item_factory_get_item (ifactory, "/Status Bar");
   tw->menuitem.tab         = gtk_item_factory_get_item (ifactory, "/Tab");
   tw->menuitem.preview_tab = gtk_item_factory_get_item (ifactory, "/Tab (Preview)");
   tw->menuitem.fullscr     = gtk_item_factory_get_item (ifactory, "/Full Screen");

   ifactory = gtk_item_factory_from_widget (tw->sort_menu);
   tw->menuitem.sort_name   = gtk_item_factory_get_item (ifactory, "/by Name");
   tw->menuitem.sort_access = gtk_item_factory_get_item (ifactory, "/by Access Time");
   tw->menuitem.sort_time   = gtk_item_factory_get_item (ifactory, "/by Modification Time");
   tw->menuitem.sort_change = gtk_item_factory_get_item (ifactory, "/by Change Time");
   tw->menuitem.sort_size   = gtk_item_factory_get_item (ifactory, "/by Size");
   tw->menuitem.sort_type   = gtk_item_factory_get_item (ifactory, "/by Type");
   tw->menuitem.sort_width  = gtk_item_factory_get_item (ifactory, "/by Image Width");
   tw->menuitem.sort_height = gtk_item_factory_get_item (ifactory, "/by Image Height");
   tw->menuitem.sort_area   = gtk_item_factory_get_item (ifactory, "/by Image Area");
   tw->menuitem.sort_rev    = gtk_item_factory_get_item (ifactory, "/Reverse Order");
   tw->menuitem.sort_case   = gtk_item_factory_get_item (ifactory, "/Case insensitive");
   tw->menuitem.sort_dir    = gtk_item_factory_get_item (ifactory, "/Directory insensitive");

   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.file)->submenu),
                     "show", G_CALLBACK (cb_file_submenu_show), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.file)->submenu),
                     "hide", G_CALLBACK (cb_file_submenu_hide), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.edit)->submenu),
                     "show", G_CALLBACK (cb_edit_submenu_show), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.edit)->submenu),
                     "hide", G_CALLBACK (cb_edit_submenu_hide), tw);
   g_signal_connect (G_OBJECT (tw->view_menu),
                     "show", G_CALLBACK (cb_view_submenu_show), tw);
   g_signal_connect (G_OBJECT (tw->view_menu),
                     "hide", G_CALLBACK (cb_view_submenu_hide), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (item)->submenu),
                     "show", G_CALLBACK (cb_tab_submenu_show), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (item)->submenu),
                     "hide", G_CALLBACK (cb_tab_submenu_hide), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.tool)->submenu),
                     "show", G_CALLBACK (cb_tool_submenu_show), tw);
   g_signal_connect (G_OBJECT (GTK_MENU_ITEM (tw->menuitem.tool)->submenu),
                     "hide", G_CALLBACK (cb_tool_submenu_hide), tw);

   /* initialize check menu items */
   if (tw->layout_type < 0)
      layout = compose_type_num;
   else if (tw->layout_type >compose_type_num)
      layout = 0;
   else
      layout = tw->layout_type;
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM
                                   (tw->menuitem.layout[layout]),
                                   TRUE);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.dirview),
                                   conf.thumbwin_show_dir_view);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.preview),
                                   conf.thumbwin_show_preview);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.menubar),
                                   conf.thumbwin_show_menubar);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.toolbar),
                                   conf.thumbwin_show_toolbar);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.dir_toolbar),
                                   conf.dirview_show_toolbar);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.statusbar),
                                   conf.thumbwin_show_statusbar);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.tab),
                                   conf.thumbwin_show_tab);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.preview_tab),
                                   conf.thumbwin_show_preview_tab);

   switch (tw->player_visible) {
   case GimvImageViewPlayerVisibleShow:
      menu_check_item_set_active (tw->view_menu, "/Player/Show", TRUE);
      break;
   case GimvImageViewPlayerVisibleHide:
      menu_check_item_set_active (tw->view_menu, "/Player/Hide", TRUE);
      break;
   case GimvImageViewPlayerVisibleAuto:
   default:
      menu_check_item_set_active (tw->view_menu, "/Player/Auto", TRUE);
      break;
   }

   switch (tw->tab_pos) {
   case GTK_POS_LEFT:
      menu_check_item_set_active (tw->menubar, "/Tab/Tab Position/Left", TRUE);
      break;
   case GTK_POS_RIGHT:
      menu_check_item_set_active (tw->menubar, "/Tab/Tab Position/Right", TRUE);
      break;
   case GTK_POS_BOTTOM:
      menu_check_item_set_active (tw->menubar, "/Tab/Tab Position/Bottom", TRUE);
      break;
   case GTK_POS_TOP:
   default:
      menu_check_item_set_active (tw->menubar, "/Tab/Tab Position/Top", TRUE);
      break;
   }

   switch (tw->sortitem) {
   case GIMV_SORT_NAME:
      item = tw->menuitem.sort_name;
      break;
   case GIMV_SORT_ATIME:
      item = tw->menuitem.sort_access;
      break;
   case GIMV_SORT_MTIME:
      item = tw->menuitem.sort_time;
      break;
   case GIMV_SORT_CTIME:
      item = tw->menuitem.sort_change;
      break;
   case GIMV_SORT_SIZE:
      item = tw->menuitem.sort_size;
      break;
   case GIMV_SORT_TYPE:
      item = tw->menuitem.sort_type;
      break;
   case GIMV_SORT_WIDTH:
      item = tw->menuitem.sort_width;
      break;
   case GIMV_SORT_HEIGHT:
      item = tw->menuitem.sort_height;
      break;
   case GIMV_SORT_AREA:
      item = tw->menuitem.sort_area;
      break;
   default:
      item = tw->menuitem.sort_name;
      break;
   }
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.sort_rev),
                                   tw->sortflags & GIMV_SORT_REVERSE);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.sort_case),
                                   tw->sortflags & GIMV_SORT_CASE_INSENSITIVE);
   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tw->menuitem.sort_dir),
                                   tw->sortflags & GIMV_SORT_DIR_INSENSITIVE);
}


static void
sync_widget_state_to_menu (GimvThumbWin *tw)
{
   gimv_thumb_win_pane_set_visible (tw, GIMV_COM_DIR_VIEW);
   gimv_thumb_win_pane_set_visible (tw, GIMV_COM_IMAGE_VIEW);

   if (!GTK_CHECK_MENU_ITEM (tw->menuitem.menubar)->active)
      gtk_widget_hide (tw->menubar_handle);
   if (!GTK_CHECK_MENU_ITEM (tw->menuitem.toolbar)->active)
      gtk_widget_hide (tw->toolbar_handle);
   if (!GTK_CHECK_MENU_ITEM (tw->menuitem.statusbar)->active)
      gtk_widget_hide (tw->status_bar_container);
   gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook),
                               GTK_CHECK_MENU_ITEM (tw->menuitem.tab)->active);
   if (tw->cv)
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->cv->notebook),
                                  GTK_CHECK_MENU_ITEM (tw->menuitem.preview_tab)->active);
}


static GtkWidget *
create_toolbar (GimvThumbWin *tw, GtkWidget *container)
{
   GtkWidget *toolbar;
   GtkWidget *button;
   GtkWidget *iconw;   
   GtkAdjustment *adj;
   GtkWidget *spinner;

   toolbar = gtkutil_create_toolbar ();

   /* file open button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_OPEN,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Open"),
                                    _("File Open"),
                                    _("File Open"),
                                    iconw,
                                    G_CALLBACK (cb_open_button),
                                    tw);
   tw->button.fileopen = button;

   /* preference button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Prefs"),
                                    _("Preference"),
                                    _("Preference"),
                                    iconw,
                                    G_CALLBACK (cb_prefs_button),
                                    tw);
   tw->button.prefs = button;

   gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

   /* previous button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_GO_BACK,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Prev"),
                                    _("Go to previous page"), 
                                    _("Go to previous page"),
                                    iconw,
                                    G_CALLBACK (cb_previous_button),
                                    tw);
   tw->button.prev = button;

   /* next button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Next"),
                                    _("Go to next page"), 
                                    _("Go to next page"),
                                    iconw,
                                    G_CALLBACK (cb_next_button),
                                    tw);
   tw->button.next = button;

   gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

   /* refresh button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_REFRESH,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Reload"),
                                    _("Reload Cache"), 
                                    _("Reload Cache"),
                                    iconw,
                                    G_CALLBACK (cb_refresh_button),
                                    tw);
   tw->button.refresh = button;

   /* skip button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Skip"),
                                    _("Skip creating current thumbnail table"), 
                                    _("Skip creating current thumbnail table"),
                                    iconw,
                                    G_CALLBACK (cb_skip_button),
                                    tw);
   gtk_widget_set_sensitive (button, FALSE);
   tw->button.skip = button;

   /* stop button */
   iconw = gtk_image_new_from_stock(GTK_STOCK_STOP,
                                    GTK_ICON_SIZE_SMALL_TOOLBAR);
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Stop"),
                                    _("Stop creating thumbnails"), 
                                    _("Stop creating thumbnails"),
                                    iconw,
                                    G_CALLBACK (cb_stop_button),
                                    tw);
   gtk_widget_set_sensitive (button, FALSE);
   tw->button.stop = button;

   /* thumbnail size spinner */
   adj = (GtkAdjustment *) gtk_adjustment_new (conf.thumbwin_thumb_size,
                                               GIMV_THUMB_WIN_MIN_THUMB_SIZE,
                                               GIMV_THUMB_WIN_MAX_THUMB_SIZE,
                                               1.0, 4.0, 0.0);
   spinner = gtkutil_create_spin_button (adj);
   gtk_widget_set_name (tw->menubar_handle, "ThumbnailSizeSpinner");
   tw->button.size_spin = spinner;
   gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), spinner,
                              _("Thumbnail Size"), _("Thumbnail Size"));
   g_signal_connect (G_OBJECT(spinner), "key-press-event",
                     G_CALLBACK(cb_size_spinner_key_press), tw);
   gtk_widget_show (spinner);

   gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

   /* quit button */
   iconw = gimv_icon_stock_get_widget ("close");
   button = gtk_toolbar_append_item(GTK_TOOLBAR (toolbar),
                                    _("Quit"),
                                    _("Exit this program"),
                                    _("Exit this program"),
                                    iconw,
                                    G_CALLBACK (cb_quit_button),
                                    NULL);
   tw->button.quit = button;
   gtk_widget_hide (tw->button.quit);

   return toolbar;
}


static void
gimv_thumb_win_get_layout (GimvThumbwinComposeType *compose, gint layout)
{
   gint i;

   if (layout >= compose_type_num)
      layout = 0;

   if (layout > -1) {
      *compose = compose_type[layout];
   } else {
      compose->pane1_horizontal = conf.thumbwin_pane1_horizontal;
      compose->pane2_horizontal = conf.thumbwin_pane2_horizontal;
      compose->pane2_attach_to_child1 = conf.thumbwin_pane2_attach_1;
      for (i = 0; i < 3; i++)
         compose->widget_type[i] = conf.thumbwin_widget[i];
   }
}


static void
gimv_thumb_win_pane_set_visible (GimvThumbWin *tw,  GimvComponentType item)
{
   GimvThumbwinComposeType compose;
   GimvComponentType type;
   GtkWidget *gpane;
   gint i, val = -1, *size, pane, child;
   gboolean show, *current_state;

   g_return_if_fail (tw);

   if (tw->layout_type >= compose_type_num) return;
   gimv_thumb_win_get_layout (&compose, tw->layout_type);

   if (item == GIMV_COM_DIR_VIEW) {
      type = GIMV_COM_DIR_VIEW;
      current_state = &tw->show_dirview;
      show = GTK_CHECK_MENU_ITEM(tw->menuitem.dirview)->active;
   } else if (item == GIMV_COM_IMAGE_VIEW) {
      type = GIMV_COM_IMAGE_VIEW;
      current_state = &tw->show_preview;
      show = GTK_CHECK_MENU_ITEM(tw->menuitem.preview)->active;
   } else {
      return;
   }

   /* find attached pane and child */
   for (i = 0; i < 3; i++) {
      if (compose.widget_type[i] == type)
         val = i;
   }
   if (val < 0) return;

   if (val == 0) {
      pane = 1;
      if (compose.pane2_attach_to_child1)
         child = 2;
      else
         child = 1;
   } else if (val == 1) {
      pane = 2;
      child = 1;
   } else if (val == 2) {
      pane = 2;
      child = 2;
   } else {
      goto FUNC_END;
   }

   if (pane == 1) {
      gpane = tw->pane1;
      size = &tw->pane_size1;
   } else {
      gint pane1_hide_item = gtkutil_paned_which_is_hidden (GTK_PANED(tw->pane1));
      gint pane2_hide_item = gtkutil_paned_which_is_hidden (GTK_PANED(tw->pane2));

      /*
       *  directory view and preview are attach to same pane,
       *  and one of them is hidden.
       */
      if (compose.widget_type[0] == GIMV_COM_THUMB_VIEW
          && gtkutil_paned_which_is_hidden (GTK_PANED(tw->pane2)) != 0
          && ((pane1_hide_item == 0 && !show) || (pane1_hide_item != 0 && show)))
         {
            gpane = tw->pane1;
            size = &tw->pane_size1;
            if (GTK_PANED(tw->pane1)->child1 == tw->pane2) {
               child = 1;
            } else if (GTK_PANED(tw->pane1)->child2 == tw->pane2) {
               child = 2;
            } else {
               goto FUNC_END;
            }

            if (show && (val == pane2_hide_item)) {
               gtk_widget_show (GTK_PANED(tw->pane2)->child1);
               gtk_widget_show (GTK_PANED(tw->pane2)->child2);
               if (pane2_hide_item == 1)
                  gtk_widget_hide (GTK_PANED (tw->pane2)->child2);
               else if (pane2_hide_item == 2)
                  gtk_widget_hide (GTK_PANED (tw->pane2)->child1);
            }

         } else {
            gpane = tw->pane2;
            size = &tw->pane_size2;
         }
   }

   if (show) {
      gtk_widget_show (GTK_PANED (gpane)->child1);
      gtk_widget_show (GTK_PANED (gpane)->child2);
      gtk_paned_set_position (GTK_PANED (gpane), *size);
   } else {
      *size = gtk_paned_get_position (GTK_PANED (gpane));
      if (child == 1)
         gtk_widget_hide (GTK_PANED (gpane)->child1);
      else
         gtk_widget_hide (GTK_PANED (gpane)->child2);
   }

 FUNC_END:
   *current_state = show;
}


static GtkWidget *
thumbnail_window_contents_new (GimvThumbWin *tw)
{
   GtkWidget *widget[3], *thumbview;
   GimvThumbwinComposeType compose;
   gchar *dirname;
   gint i;

   if (tw->layout_type >= compose_type_num)
      tw->layout_type = 0;

   gimv_thumb_win_get_layout (&compose, tw->layout_type);

   /* create each widget */
   dirname = g_get_current_dir ();
   tw->dv = GIMV_DIR_VIEW (gimv_dir_view_new (dirname, tw));
   thumbview = thumbnail_view_new (tw);
   tw->preview = image_preview_new (tw);
   g_free (dirname);

   for (i = 0; i < 3; i++) {
      switch (compose.widget_type[i]) {
      case GIMV_COM_DIR_VIEW:
         widget[i] = GTK_WIDGET (tw->dv);
         break;
      case GIMV_COM_THUMB_VIEW:
         widget[i] = thumbview;
         break;
      case GIMV_COM_IMAGE_VIEW:
         widget[i] = tw->preview;
         break;
      default:
         break;
      }
   }

   /* compose */
   if (compose.pane2_horizontal)
      tw->pane2 = gtk_hpaned_new ();
   else
      tw->pane2 = gtk_vpaned_new ();
   if (compose.pane1_horizontal)
      tw->pane1 = gtk_hpaned_new ();
   else
      tw->pane1 = gtk_vpaned_new ();

   if (compose.pane2_attach_to_child1) {
      gtk_paned_add1 (GTK_PANED (tw->pane1), tw->pane2);
      gtk_paned_add2 (GTK_PANED (tw->pane1), widget[0]);
   } else {
      gtk_paned_add1 (GTK_PANED (tw->pane1), widget[0]);
      gtk_paned_add2 (GTK_PANED (tw->pane1), tw->pane2);
   }
   gtk_paned_add1 (GTK_PANED (tw->pane2), widget[1]);
   gtk_paned_add2 (GTK_PANED (tw->pane2), widget[2]);

   gtk_paned_set_position (GTK_PANED (tw->pane1),
                            tw->pane_size1);
   gtk_paned_set_position (GTK_PANED (tw->pane2),
                            tw->pane_size2);

   /* show widget */
   gtk_widget_show (widget[0]);
   gtk_widget_show (widget[1]);
   gtk_widget_show (widget[2]);
   gtk_widget_show (tw->pane2);

   return tw->pane1;
}


static GtkWidget *
thumbnail_view_new (GimvThumbWin *tw)
{
   GtkWidget *notebook, *vbox;

   vbox = gtk_vbox_new (FALSE, 0);

   notebook = gtk_notebook_new ();
   gtk_widget_set_name (notebook, "ThumbViewNoteBook");
   gtk_widget_show (notebook);
   gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
   gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

   gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook),
                             tw->tab_pos);
   gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
   gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));
   gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (notebook), 0);
   gtk_notebook_set_tab_vborder (GTK_NOTEBOOK (notebook), 0);
   g_signal_connect (G_OBJECT(notebook), "switch-page",
                     G_CALLBACK(cb_thumb_notebook_switch_page), tw);

   g_object_set_data (G_OBJECT (notebook), "thumbwin", tw);
   dnd_src_set  (notebook, dnd_types_tab_component, dnd_types_tab_component_num);
   g_signal_connect (G_OBJECT (notebook), "drag_begin",
                     G_CALLBACK (cb_com_drag_begin), tw);
   g_signal_connect (G_OBJECT (notebook), "drag_data_get",
                     G_CALLBACK (cb_com_drag_data_get), tw);

   dnd_dest_set (notebook, dnd_types_all, dnd_types_all_num);
   g_object_set_data (G_OBJECT (notebook),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_THUMB_VIEW));
   g_signal_connect (G_OBJECT (notebook), "drag_data_received",
                     G_CALLBACK (cb_notebook_drag_data_received), tw);

   dnd_dest_set (vbox, dnd_types_all, dnd_types_all_num);
   g_object_set_data (G_OBJECT (vbox),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_THUMB_VIEW));
   g_signal_connect (G_OBJECT (vbox), "drag_data_received",
                     G_CALLBACK (cb_notebook_drag_data_received), tw);

   tw->notebook = notebook;

   return vbox;
}


static gint
image_preview_action (GimvImageView *iv, GdkEventButton *event, gint num)
{
   gint mx, my;

   gimv_image_view_get_view_position (iv, &mx, &my);

   switch (abs (num)) {
   case PREVIEW_NEXT:
      gimv_image_view_next (iv);
      break;
   case PREVIEW_PREV:
      gimv_image_view_prev (iv);
      break;
   case PREVIEW_SHAREDWIN:
      gimv_image_win_open_shared_window (iv->info);
      break;
   case PREVIEW_NEWWIN:
      if (iv->info)
         gimv_image_win_open_window (iv->info);
      break;
   case PREVIEW_POPUP:
      gimv_image_view_popup_menu (iv, event);
      break;
   case PREVIEW_ZOOM_IN:
      gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_IN, 0, 0);
      break;
   case PREVIEW_ZOOM_OUT:
      gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_OUT, 0, 0);
      break;
   case PREVIEW_FIT:
      gimv_image_view_zoom_image (iv, GIMV_IMAGE_VIEW_ZOOM_FIT, 0, 0);
      break;
   case PREVIEW_ROTATE_CCW:
      gimv_image_view_rotate_ccw (iv);
      break;
   case PREVIEW_ROTATE_CW:
      gimv_image_view_rotate_cw (iv);
      break;
   case PREVIEW_NAVWIN:
      gimv_image_view_open_navwin (iv, event->x_root, event->y_root);
      break;
   case PREVIEW_UP:
      my += 20;
      gimv_image_view_moveto (iv, mx, my);
      break;
   case PREVIEW_DOWN:
      my -= 20;
      gimv_image_view_moveto (iv, mx, my);
      break;
   case PREVIEW_LEFT:
      mx += 20;
      gimv_image_view_moveto (iv, mx, my);
      break;
   case PREVIEW_RIGHT:
      mx -= 20;
      gimv_image_view_moveto (iv, mx, my);
      break;
   default:
      break;
   }

   return TRUE;
}


static gint
cb_image_preview_pressed (GimvImageView *iv, GdkEventButton *event, gpointer data)
{
   gint num;

   g_return_val_if_fail (iv, TRUE);
   g_return_val_if_fail (event, TRUE);

   num = prefs_mouse_get_num_from_event (event, conf.preview_mouse_button);
   if (num > 0)
      return image_preview_action (iv, event, num);

   return TRUE;
}


static gint
cb_image_preview_clicked (GimvImageView *iv, GdkEventButton *event, gpointer data)
{
   gint num;

   g_return_val_if_fail (iv, TRUE);
   g_return_val_if_fail (event, TRUE);

   num = prefs_mouse_get_num_from_event (event, conf.preview_mouse_button);
   if (num < 0)
      return image_preview_action (iv, event, num);

   return TRUE;
}


static gboolean
cb_unset_com_dnd (GtkWidget *widget,
                  GdkEventButton *event,
                  GimvThumbWin *tw)
{
   g_return_val_if_fail (tw, FALSE);
   g_return_val_if_fail (tw->cv, FALSE);
   gtk_drag_source_unset (tw->cv->notebook);

   return FALSE;
}


static gboolean
cb_reset_com_dnd (GtkWidget *widget,
                  GdkEventButton *event,
                  GimvThumbWin *tw)
{
   g_return_val_if_fail (tw, FALSE);
   g_return_val_if_fail (tw->cv, FALSE);
   dnd_src_set  (tw->cv->notebook,
                 dnd_types_component,
                 dnd_types_component_num);
   return FALSE;
}


static gboolean
cb_comment_view_delete (GtkWidget *widget, GdkEventAny *event, GimvThumbWin *tw)
{
   if (tw->cv) {
      if (g_slist_find (tw->accel_group_list, tw->cv->accel_group))
         tw->accel_group_list
            = g_slist_remove (tw->accel_group_list, tw->cv->accel_group);
      else
         gtk_window_remove_accel_group (GTK_WINDOW (tw),
                                        tw->cv->accel_group);
   }
   return FALSE;
}


static GtkWidget *
image_preview_new (GimvThumbWin *tw)
{
   GimvCommentView *cv;
   GtkWidget *label;
   gboolean show_scrollbar;

   cv = tw->cv = gimv_comment_view_create ();
   if (cv->accel_group) {
      gtk_accel_group_ref (cv->accel_group);
      gtk_window_add_accel_group (GTK_WINDOW (tw), cv->accel_group);
   }
   g_signal_connect (G_OBJECT (cv->main_vbox), "delete_event",
                     G_CALLBACK (cb_comment_view_delete), tw);
#if 1 /* FIXME */
   g_signal_connect(G_OBJECT (cv->value_entry), "focus_in_event",
                    G_CALLBACK (cb_focus_in), tw);
   g_signal_connect(G_OBJECT (cv->value_entry), "focus_out_event",
                    G_CALLBACK (cb_focus_out), tw);
   g_signal_connect(G_OBJECT (cv->note_box), "focus_in_event",
                    G_CALLBACK (cb_focus_in), tw);
   g_signal_connect(G_OBJECT (cv->note_box), "focus_out_event",
                    G_CALLBACK (cb_focus_out), tw);
#endif

   /* create image view and attach to comment view notebook */
   label = gtk_label_new (_("Preview"));
   gtk_widget_set_name (label, "TabLabel");

   tw->iv = GIMV_IMAGE_VIEW (gimv_image_view_new (NULL));

   g_signal_connect (G_OBJECT (tw->iv), "image_pressed",
                     G_CALLBACK (cb_image_preview_pressed), tw);
   g_signal_connect (G_OBJECT (tw->iv), "image_clicked",
                     G_CALLBACK (cb_image_preview_clicked), tw);

   /* override prefs */
   gimv_image_view_set_player_visible (tw->iv, tw->player_visible);
   g_object_set(G_OBJECT(tw->iv),
                "x_scale",           conf.preview_scale,
                "y_scale",           conf.preview_scale,
                "default_zoom",      conf.preview_zoom,
                "default_rotation",  conf.preview_rotation,
                "keep_aspect",       conf.preview_keep_aspect,
                "keep_buffer",       conf.preview_buffer,
                "show_scrollbar",    conf.preview_scrollbar,
                NULL);

   gimv_image_view_create_popup_menu (GTK_WIDGET (tw),
                                      tw->iv, "<ThumbWinPreviewPop>");

   /* set component DnD */
   /* FIXME!! image view and comment view shuold be seperated */
   dnd_src_set  (cv->notebook, dnd_types_component, dnd_types_component_num);
   g_object_set_data (G_OBJECT (cv->notebook),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_IMAGE_VIEW));
   g_signal_connect (G_OBJECT (cv->notebook), "drag_begin",
                     G_CALLBACK (cb_com_drag_begin), tw);
   g_signal_connect (G_OBJECT (cv->notebook), "drag_data_get",
                     G_CALLBACK (cb_com_drag_data_get), tw);

   dnd_dest_set (cv->main_vbox, dnd_types_component, dnd_types_component_num);
   g_object_set_data (G_OBJECT (cv->main_vbox),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_IMAGE_VIEW));
   g_signal_connect (G_OBJECT (cv->main_vbox), "drag_data_received",
                     G_CALLBACK (cb_com_swap_drag_data_received), tw);
   /* END FIXME!! */

   /* FIXME!! */
   /* for avoiding gtk's bug */
   g_signal_connect (G_OBJECT (tw->iv), "button_press_event",
                     G_CALLBACK (cb_unset_com_dnd), tw);
   g_signal_connect (G_OBJECT (tw->iv), "button_release_event",
                     G_CALLBACK (cb_reset_com_dnd), tw);
   g_signal_connect (G_OBJECT (tw->cv->comment_clist->parent),
                     "button_press_event",
                     G_CALLBACK (cb_unset_com_dnd), tw);
   g_signal_connect (G_OBJECT (tw->cv->comment_clist->parent),
                     "button_release_event",
                     G_CALLBACK (cb_reset_com_dnd), tw);
   g_signal_connect (G_OBJECT (tw->cv->note_box->parent),
                     "button_press_event",
                     G_CALLBACK (cb_unset_com_dnd), tw);
   g_signal_connect (G_OBJECT (tw->cv->note_box->parent),
                     "button_release_event",
                     G_CALLBACK (cb_reset_com_dnd), tw);
   /* END FIXME!! */

   gtk_widget_show (GTK_WIDGET (tw->iv));
   g_object_get (G_OBJECT (tw->iv),
                 "show_scrollbar", &show_scrollbar,
                 NULL);
   if (!show_scrollbar)
      gimv_image_view_hide_scrollbar (tw->iv);
   gtk_notebook_prepend_page (GTK_NOTEBOOK(cv->notebook), GTK_WIDGET (tw->iv), label);
   gtk_notebook_set_page (GTK_NOTEBOOK(cv->notebook), 0);

   return cv->main_vbox;
}



/******************************************************************************
 *
 *   Callback functions for menubar.
 *
 ******************************************************************************/
static void
cb_open (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   switch (action) {
   case OPEN_FILE:
      if (tw->open_dialog)
         gdk_window_raise (tw->open_dialog->window);
      else
         tw->open_dialog = (GtkWidget *) create_filebrowser (tw);
      break;
   case OPEN_IMAGEWIN:
      gimv_image_win_open_window (NULL);   
      break;
   case OPEN_THUMBWIN:
      gimv_thumb_win_open_window();
      break;
   case OPEN_THUMBTAB:
      gimv_thumb_win_create_new_tab (tw);
      break;
   case OPEN_PREFS:
      gimv_prefs_win_open_idle ("/Thumbnail Window", GTK_WINDOW (tw));
      break;
   case OPEN_GIMV_INFO:
      gimvhelp_open_info_window ();
      break;
   default:
      break;
   }
}


static void
cb_close (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   switch (action) {
   case CLOSE_THUMBWIN:
      if (g_list_length (ThumbWinList) == 1 && conf.thumbwin_save_win_state)
         gimv_thumb_win_save_state (tw);
      gtk_widget_destroy (GTK_WIDGET (tw));
      break;
   case CLOSE_THUMBTAB:
      gimv_thumb_win_close_tab (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
      break;
   case CLOSE_ALL:
      gimv_quit ();
      break;
   default:
      break;
   }
}


static void
cb_select_all (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GimvThumbView *tv;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   gimv_thumb_view_set_selection_all (tv, action);
}


static void
cb_refresh_list (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GimvThumbView *tv;

   if (!tw) return;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   gimv_thumb_view_refresh_list (tv);
}

static void
cb_reload (GimvThumbWin *tw, ThumbLoadType type, GtkWidget *widget)
{
   gimv_thumb_win_reload_thumbnail (tw, type);
}


static void
cb_clear_cache (GimvThumbWin *tw, CacheClearType type, GtkWidget *widget)
{
   const gchar *path;

   switch (type) {
   case CLEAR_CACHE_TAB:
   {
      GimvThumbView *tv;

      tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
      if (!tv) return;

      path = gimv_thumb_view_get_path (tv);
      gimv_thumb_cache_clear (path, conf.cache_write_type, 0, NULL);
      break;
   }
   case CLEAR_CACHE_ALL:
      gimv_thumb_cache_clear (NULL, conf.cache_write_type, 0, NULL);
      break;
   default:
      break;
   }
}


static void
cb_file_operate (GimvThumbWin *tw, guint action, GtkWidget *menuitem)
{
   GimvThumbView  *tv;

   g_return_if_fail (tw);
   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   switch (action) {
   case FILE_COPY:
      gimv_thumb_view_file_operate (tv, FILE_COPY);
      break;
   case FILE_MOVE:
      gimv_thumb_view_file_operate (tv, FILE_MOVE);
      break;
   case FILE_LINK:
      gimv_thumb_view_file_operate (tv, FILE_LINK);
      break;
   case FILE_RENAME:
      gimv_thumb_view_rename_file (tv);
      break;
   case FILE_REMOVE:
      gimv_thumb_view_delete_files (tv);
      break;
   default:
      break;
   }
}


static void
cb_find_similar (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GimvThumbView *tv;
   const gchar **types;

   g_return_if_fail (tw);

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   types = gimv_dupl_finder_get_algol_types();
   gimv_thumb_view_find_duplicates (tv, NULL, types[action]);
}


static void
cb_move_tab (GimvThumbWin *tw, MoveTabItem item, GtkWidget *widget)
{
   GtkWidget *page_container;
   gint current_page;

   current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (tw->notebook));
   if (current_page < 0) return;

   page_container = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tw->notebook),
                                               current_page);

   switch (item) {
   case LEFT:
      gtk_notebook_reorder_child (GTK_NOTEBOOK (tw->notebook),
                                  page_container, current_page - 1);
      break;
   case RIGHT:
      gtk_notebook_reorder_child (GTK_NOTEBOOK (tw->notebook),
                                  page_container, current_page + 1);
      break;
   default:
      break;
   }
}


static void
cb_cut_out_tab (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GimvThumbWin *tw_new;
   GimvThumbView  *tv;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   /* open new window */
   tw_new = gimv_thumb_win_open_window ();

   gimv_thumb_win_detach_tab (tw_new, tw, tv);
}


static void
cb_sort_item (GimvThumbWin *tw, GimvSortItem sortitem, GtkWidget *widget)
{
   GimvThumbView  *tv;

   tw->sortitem = sortitem;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   gimv_thumb_view_set_widget (tv, tv->tw, tv->container, tv->summary_mode);
}


static void
cb_sort_flag (GimvThumbWin *tw, GimvSortFlag sortflag, GtkWidget *widget)
{
   GimvThumbView  *tv;

   if (GTK_CHECK_MENU_ITEM (widget)->active) {
      tw->sortflags |= sortflag;
   } else {
      tw->sortflags &= ~sortflag;
   }

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   gimv_thumb_view_set_widget (tv, tv->tw, tv->container, tv->summary_mode);
}


static void
cb_switch_layout (GimvThumbWin *tw, gint action, GtkWidget *widget)
{
   gimv_thumb_win_change_layout (tw, action);
}


static void
cb_switch_tab_pos (GimvThumbWin *tw, GtkPositionType pos, GtkWidget *widget)
{
   g_return_if_fail (tw);

   tw->tab_pos = pos;
   gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook),
                             tw->tab_pos);
}


static void
g_list_randomize (GList **lix)
{
   GList *newlist = NULL;

   gint num_elements;
   while ((num_elements = g_list_length(*lix)) > 0) {
        GList *element = g_list_nth(*lix, g_random_int_range(0, num_elements));

        newlist = g_list_append(newlist, element->data);
        *lix = g_list_remove_link(*lix, element);
   }

   *lix = newlist;
}


static void
cb_slideshow (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GimvThumbView *tv;
   GimvThumb *thumb;
   GimvSlideshow *slideshow;
   GList *filelist = NULL, *list, *selection, *node, *start = NULL;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;
   if (!tv->priv) return;

   selection = gimv_thumb_view_get_selection_list (tv);

   if (tw->priv->slideshow_selected_only)
      list = selection;
   else
      list = tv->thumblist;

   for (node = list; node; node = g_list_next (node)) {
      thumb = node->data;

      if (gimv_image_info_is_dir (thumb->info)
          || gimv_image_info_is_archive (thumb->info))
      {
         continue;
      }

      if (gimv_image_info_is_movie (thumb->info)) {
         if (tw->priv->slideshow_file_type == SLIDESHOW_IMAGES_ONLY)
            continue;
      } else {
         if (tw->priv->slideshow_file_type == SLIDESHOW_MOVIES_ONLY)
            continue;
      }

      filelist = g_list_append (filelist, gimv_image_info_ref (thumb->info));

      if (!start) {
         if (tw->priv->slideshow_order == SLIDESHOW_START_FROM_SELECTED) {
            if (g_list_find (selection, thumb))
               start = g_list_find (filelist, thumb->info);
         } else {
            start = filelist;
         }
      }
   }

   if (filelist) {
      if (tw->priv->slideshow_order == SLIDESHOW_RANDOM_ORDER) {
         g_list_randomize(&filelist);
         start = filelist;
      }

      slideshow = gimv_slideshow_new_with_filelist (filelist, start);
      if (slideshow)
         gimv_slideshow_play (slideshow);
   }

   g_list_free (selection);
}


static void
cb_slideshow_order (GimvThumbWin  *tw, SlideshowOrder action, GtkWidget *widget)
{
   if (tw && tw->priv)
      tw->priv->slideshow_order = action;
}


static void
cb_slideshow_selected (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   if (tw && tw->priv)
      tw->priv->slideshow_selected_only
         = GTK_CHECK_MENU_ITEM (widget)->active;
}


static void
cb_slideshow_file_type (GimvThumbWin *tw, SlideshowFileType action,
                        GtkWidget *widget)
{
   if (tw && tw->priv)
      tw->priv->slideshow_file_type = action;
}


static void
cb_switch_page (GimvThumbWin *tw, SwitchPage action, GtkWidget *widget)
{
   switch (action) {
   case FIRST:
      gtk_notebook_set_page (GTK_NOTEBOOK (tw->notebook), 0);
      break;
   case LAST:
      gtk_notebook_set_page (GTK_NOTEBOOK (tw->notebook), -1);
      break;
   case NEXT:
      gtk_notebook_next_page (GTK_NOTEBOOK (tw->notebook));
      break;
   case PREV:
      gtk_notebook_prev_page (GTK_NOTEBOOK (tw->notebook));
      break;
   default:
      break;
   }
}


static void
cb_focus (GimvThumbWin *tw, FocusItem item, GtkWidget *menuitem)
{
   GtkWidget *widget;

   switch (item) {
   case DIR_VIEW:
      widget = tw->dv->dirtree;
      break;
   case THUMBNAIL_VIEW:
   {
      GimvThumbView *tv;

      tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
      if (tv)
         gimv_thumb_view_grab_focus (tv);
      return;
   }
   case PREVIEW:
      widget = tw->iv->draw_area;
      break;
   case LOCATION_ENTRY:
      widget = tw->location_entry;
      break;
   default:
      return;
   }

   if (GTK_IS_WIDGET (widget))
      gtk_widget_grab_focus (widget);
}


static void
cb_toggle_view (GimvThumbWin *tw, GimvComponentType item, GtkWidget *widget)
{
   g_return_if_fail (tw && (tw->layout_type < compose_type_num));
   gimv_thumb_win_pane_set_visible (tw, item);
}


static void
cb_toggle_show (GimvThumbWin *tw, ShowItem sitem, GtkWidget *widget)
{
   GtkWidget *item = NULL;

   switch (sitem) {
   case MENUBAR:
      item = tw->menubar_handle;
      break;
   case TOOLBAR:
      item = tw->toolbar_handle;
      break;
   case TAB:
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->notebook), 
                                  GTK_CHECK_MENU_ITEM(widget)->active);
      break;
   case STATUSBAR:
      item = tw->status_bar_container;
      break;
   case DIR_TOOLBAR:
      if (!tw->dv) return;
      if (GTK_CHECK_MENU_ITEM(widget)->active) {
         gimv_dir_view_show_toolbar (tw->dv);
      } else {
         gimv_dir_view_hide_toolbar (tw->dv);
      }
      return;
   case PREVIEW_TAB:
      if (tw->cv)
         gtk_notebook_set_show_tabs (GTK_NOTEBOOK(tw->cv->notebook), 
                                     GTK_CHECK_MENU_ITEM(widget)->active);
      break;
   default:
      return;
      break;
   }

   if (sitem == TAB || sitem == PREVIEW_TAB)
      return;

   if (GTK_CHECK_MENU_ITEM(widget)->active) {
      gtk_widget_show (item);
   } else {
      gtk_widget_hide (item);
   }
}


static void
cb_switch_player (GimvThumbWin *tw, GimvImageViewPlayerVisibleType visible, GtkWidget *widget)
{
   g_return_if_fail (tw);
   g_return_if_fail (GIMV_IS_IMAGE_VIEW (tw->iv));

   gimv_image_view_set_player_visible (tw->iv, visible);
   tw->player_visible = gimv_image_view_get_player_visible (tw->iv);
}


static void
cb_toggle_maximize (GimvThumbWin *tw, guint action, GtkWidget *widget)
{
   GdkWindow *gdk_window = GTK_WIDGET (tw)->window;
   gint client_x, client_y, root_x, root_y;
   gint width, height;

   if (GTK_CHECK_MENU_ITEM(widget)->active) {
      gdk_window_get_origin (gdk_window, &root_x, &root_y);
      gdk_window_get_geometry (gdk_window, &client_x, &client_y,
                               &width, &height, NULL);

      gdk_window_move_resize (gdk_window, -client_x, -client_y,
                              gdk_screen_width (), gdk_screen_height ());

      tw->fullscreen = TRUE;
      tw->win_x = root_x - client_x;
      tw->win_y = root_y - client_y;
      tw->win_width  = width;
      tw->win_height = height;

   } else {
      gdk_window_move_resize (gdk_window, tw->win_x, tw->win_y,
                              tw->win_width, tw->win_height);
      tw->fullscreen = FALSE;
   }
}


static void
cb_win_composition_menu (GtkMenuItem *item, gpointer data)
{
   GimvThumbWin *tw = data;
   GtkCheckMenuItem *menuitem;
   gpointer p;
   gint num, n_win_comps;
   gboolean active;

   g_return_if_fail (tw);

   p = g_object_get_data (G_OBJECT (item), "num");
   num = GPOINTER_TO_INT (p);
   n_win_comps = sizeof (win_comp) / sizeof (gboolean) / 3;
   if (num < 0 || num > n_win_comps) return;

   conf.thumbwin_pane1_horizontal = win_comp[num][0];
   conf.thumbwin_pane2_horizontal = win_comp[num][1];
   conf.thumbwin_pane2_attach_1   = win_comp[num][2];

   menuitem = GTK_CHECK_MENU_ITEM(tw->menuitem.layout[5]);
   active = menuitem->active;
   gtk_check_menu_item_set_active (menuitem, TRUE);
   if (active)
      g_signal_emit_by_name (G_OBJECT (menuitem), "activate");
}



/******************************************************************************
 *
 *  Callback functions for toolbar buttons.
 *
 ******************************************************************************/
static void
cb_location_entry_enter (GtkEditable *entry, GimvThumbWin *tw)
{
   const gchar *path_internal;
   gchar *path, *dir;

   g_return_if_fail (tw);

   path_internal = gtk_entry_get_text (GTK_ENTRY (entry));
   path = charset_internal_to_locale (path_internal);

   if (isdir (path)) {
      dir = add_slash (path);
      open_dir_images (dir, tw, NULL, LOAD_CACHE, conf.scan_dir_recursive);
      g_free (dir);
   } else if (file_exists (path)) {
      if (fr_archive_utils_get_file_name_ext (path)) {
         open_archive_images (path, tw, NULL, LOAD_CACHE);
      } else {
         GimvImageInfo *info = gimv_image_info_get (path);
         if (info)
            gimv_image_view_change_image (tw->iv, info);
      }
   }

   g_free (path);
}


static gboolean
cb_location_entry_key_press (GtkWidget *widget, 
                             GdkEventKey *event,
                             GimvThumbWin *tw)
{
   const gchar *path_internal;
   guint comp_key1 = 0, comp_key2 = 0;
   GdkModifierType comp_mods1 = 0, comp_mods2 = 0;

   g_return_val_if_fail (tw, FALSE);

   if (akey.common_auto_completion1)
      gtk_accelerator_parse (akey.common_auto_completion1,
                             &comp_key1, &comp_mods1);
   if (akey.common_auto_completion2)
      gtk_accelerator_parse (akey.common_auto_completion2,
                             &comp_key2, &comp_mods2);

   if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter) {
      gchar *path;

      path_internal = gtk_entry_get_text (GTK_ENTRY (widget));
      path = charset_internal_to_locale (path_internal);

      if (isdir (path)) {
         gchar *dir = add_slash (path);
         open_dir_images (dir, tw, NULL, LOAD_CACHE, conf.scan_dir_recursive);
         g_free (dir);
      } else if (file_exists (path)) {
         if (fr_archive_utils_get_file_name_ext (path)) {
            open_archive_images (path, tw, NULL, LOAD_CACHE);
         } else {
            GimvImageInfo *info = gimv_image_info_get (path);
            if (info)
               gimv_image_view_change_image (tw->iv, info);
         }
      }

      g_free (path);

      return TRUE;

   } else if (event->keyval == GDK_Tab
              || (event->keyval == comp_key1 && (!comp_mods1 || (event->state & comp_mods1)))
              || (event->keyval == comp_key2 && (!comp_mods2 || (event->state & comp_mods2))))
      {
         gchar *text, *dirname;
         gint n;

         path_internal = gtk_entry_get_text (GTK_ENTRY (widget));
         n = auto_compl_get_n_alternatives (path_internal);

         if (n < 1) return TRUE;

         text = auto_compl_get_common_prefix ();

         if (n == 1) {
            auto_compl_hide_alternatives ();
            gtk_entry_set_text (GTK_ENTRY (widget), text);
            if (text[strlen(text) - 1] != '/')
               gtk_entry_append_text (GTK_ENTRY (widget), "/");
            gimv_dir_view_change_dir (tw->dv, text);
         } else {
            gtk_entry_set_text (GTK_ENTRY (widget), text);
            dirname = g_dirname (text);
            if (tw->show_dirview)
               gimv_dir_view_change_dir (tw->dv, dirname);
            g_free (dirname);
            auto_compl_show_alternatives (widget);
         }

         gtk_editable_set_position (GTK_EDITABLE (widget), -1);

         g_free (text);

         return TRUE;

      } else if (event->keyval == GDK_Escape) {
         gtk_window_set_focus (GTK_WINDOW (tw), NULL);
         return TRUE;

      } else if (event->keyval == GDK_Right || event->keyval == GDK_Left
                 || event->keyval == GDK_Up || event->keyval == GDK_Down)
         {
            return TRUE;
         }

   return TRUE;
}


static void
cb_location_entry_drag_data_received (GtkWidget *widget,
                                      GdkDragContext *context,
                                      gint x, gint y,
                                      GtkSelectionData *seldata,
                                      guint info,
                                      guint time,
                                      gpointer data)
{
   GimvThumbWin *tw = data;
   GList *list;

   switch (info) {
   case TARGET_URI_LIST:
      list = dnd_get_file_list (seldata->data, seldata->length);
      gimv_thumb_win_location_entry_set_text (tw, list->data);
      g_list_foreach (list, (GFunc) g_free, NULL);
      g_list_free (list);
      break;

   default:
      break;
   }
}


static void
cb_open_button (GtkWidget *widget, GimvThumbWin *tw)
{
   if (tw->open_dialog)
      gdk_window_raise (tw->open_dialog->window);
   else
      tw->open_dialog = (GtkWidget *) create_filebrowser (tw);
}


static void
cb_refresh_button (GtkWidget *widget, GimvThumbWin *tw)
{
   /* ThumbView *tv; */

   if (!tw) return;

   g_signal_handlers_block_by_func (G_OBJECT (widget),
                                    G_CALLBACK (cb_refresh_button),
                                    tw);

   gimv_thumb_win_reload_thumbnail (tw, LOAD_CACHE);

   g_signal_handlers_unblock_by_func (G_OBJECT (widget),
                                      G_CALLBACK (cb_refresh_button),
                                      tw);
}


static void
cb_previous_button (GtkWidget *widget, GimvThumbWin *tw)
{
   gtk_notebook_prev_page (GTK_NOTEBOOK (tw->notebook));
}



static void
cb_next_button (GtkWidget *widget, GimvThumbWin *tw)
{
   gtk_notebook_next_page (GTK_NOTEBOOK (tw->notebook));
}


static void
cb_skip_button (GtkWidget *widget, GimvThumbWin *tw)
{
   GimvThumbView *tv;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);

   if (tv && tv->progress) {
      tv->progress->status = CANCEL;
   }
}


static gboolean
cb_size_spinner_key_press (GtkWidget *widget, 
                           GdkEventKey *event,
                           GimvThumbWin *tw)
{
   g_return_val_if_fail (tw, FALSE);

   switch (event->keyval) {
   case GDK_Escape:
      gtk_window_set_focus (GTK_WINDOW (tw), NULL);
      return TRUE;
   }

   return TRUE;
}


static void
cb_stop_button (GtkWidget *widget, GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   /*
   for (i = 0; i < tw->pagenum; i++) {
      tv = gimv_thumb_win_find_thumbtable (tw, i);

      if (tv && tv->thumb_window == tw && tv->progress) {
         tv->progress->status = STOP;
      }
   }
   */

   files_loader_stop ();
}


static void
cb_prefs_button (GtkWidget *widget, GimvThumbWin *tw)
{
   gimv_prefs_win_open_idle ("/Thumbnail Window", GTK_WINDOW (tw));
}


static void
cb_quit_button (GtkWidget *widget, GimvThumbWin *tw)
{
   gimv_quit ();
}


static void
cb_display_mode_menu (GtkWidget *widget, GimvThumbWin *tw)
{
   GimvThumbView *tv;
   gint summary_mode;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   summary_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "num"));
   tw->thumbview_summary_mode = gimv_thumb_view_num_to_label (summary_mode);

   g_signal_handlers_block_by_func (G_OBJECT (widget),
                                    G_CALLBACK (cb_display_mode_menu),
                                    tw);

   gimv_thumb_view_change_summary_mode (
      tv, gimv_thumb_view_num_to_label (summary_mode));
   gtk_option_menu_set_history (GTK_OPTION_MENU (tw->summary_mode_menu),
                                summary_mode);

   g_signal_handlers_unblock_by_func (G_OBJECT (widget),
                                      G_CALLBACK(cb_display_mode_menu),
                                      tw);
}



/******************************************************************************
 *
 *   other collback functions for thumbnail window.
 *
 ******************************************************************************/
static void
cb_thumb_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
                               gint pagenum, GimvThumbWin *tw)
{
   GimvThumbView *tv = NULL;
   gint summary_mode
      = gimv_thumb_view_label_to_num (GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE);
   GList *node;

   g_return_if_fail (notebook && tw);
   node = g_list_find (ThumbWinList, tw);
   if (!node) return;

   tv = gimv_thumb_win_find_thumbtable (tw, pagenum);

   if (files_loader_query_loading ()) {
      gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING);
      /*
      if (tv && tv->progress)
         tw->status = THUMB_WIN_STATUS_LOADING;
      else
         tw->status = THUMB_WIN_STATUS_LOADING_BG;
      */
   }

   gimv_thumb_win_set_statusbar_page_info (tw, pagenum);

   if (tv) {
      location_entry_set_text (tw, tv, NULL);

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tw->button.size_spin),
                                 tv->thumb_size);
      gtk_option_menu_set_history (GTK_OPTION_MENU (tw->summary_mode_menu),
                                   gimv_thumb_view_label_to_num (tv->summary_mode));
      summary_mode = gimv_thumb_view_label_to_num (tv->summary_mode);

      if (tw->status == GIMV_THUMB_WIN_STATUS_LOADING
          || tw->status == GIMV_THUMB_WIN_STATUS_LOADING_BG
          || files_loader_query_loading ())
         {
            if (tv->progress)
               gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING);
            else
               gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING_BG);
         }

      if (tw->show_dirview) {
         const gchar *path;

         path = gimv_thumb_view_get_path (tv);
         if (path && isdir (path)) {
            gimv_dir_view_change_dir (tw->dv, path);
         } else if (path && isfile (path)) {
            gchar *dirname = g_dirname (path);
            gimv_dir_view_change_dir (tw->dv, dirname);
            g_free(dirname);
         }
      }

   } else {   /* if empty page */
      if (tw->location_entry)
         gimv_thumb_win_location_entry_set_text (tw, g_getenv("HOME"));

      if (tw->button.size_spin)
         gtk_spin_button_set_value (GTK_SPIN_BUTTON (tw->button.size_spin),
                                    conf.thumbwin_thumb_size);
      if (tw->summary_mode_menu) {
         summary_mode = gimv_thumb_view_label_to_num (tw->thumbview_summary_mode);
         if (summary_mode < 0)
            summary_mode
               = gimv_thumb_view_label_to_num (
                     GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE);
         gtk_option_menu_set_history (GTK_OPTION_MENU (tw->summary_mode_menu),
                                      summary_mode);
      }

      if (tw->status == GIMV_THUMB_WIN_STATUS_LOADING
          || tw->status == GIMV_THUMB_WIN_STATUS_LOADING_BG)
         {
            gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_LOADING_BG);
         }
   }

   tw->thumbview_summary_mode = gimv_thumb_view_num_to_label (summary_mode);
}


static void
cb_tab_close_button_clicked (GtkWidget *button, GimvThumbWin *tw)
{
   GtkWidget *container;
   gint num;

   container = g_object_get_data (G_OBJECT (button), "page-container");
   num = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), container);

   gimv_thumb_win_close_tab (tw, num);
   gimv_thumb_win_set_statusbar_page_info (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
}


static void
cb_com_drag_begin (GtkWidget *widget,
                   GdkDragContext *context,
                   gpointer data)
{
   GdkColormap *colormap;
   GimvIcon *icon;

   icon = gimv_icon_stock_get_icon ("paper");
   colormap = gdk_colormap_get_system ();
   gtk_drag_set_icon_pixmap (context, colormap,
                             icon->pixmap, icon->mask,
                             0, 0);
}


static void
cb_com_drag_data_get (GtkWidget *widget,
                      GdkDragContext *context,
                      GtkSelectionData *seldata,
                      guint info,
                      guint time,
                      gpointer data)
{
   switch (info) {
   case TARGET_GIMV_TAB:
   case TARGET_GIMV_COMPONENT:
      gtk_selection_data_set(seldata, seldata->target,
                             8, "dummy", strlen("dummy"));
      break;
   }
}


typedef struct SwapCom_Tag
{
   GimvThumbWin *tw;
   gint src;
   gint dest;
} SwapCom;


static gint
idle_gimv_thumb_win_swap_component (gpointer data)
{
   SwapCom *swap = data;
   gimv_thumb_win_swap_component (swap->tw, swap->src, swap->dest);
   return FALSE;
}


static void
cb_notebook_drag_data_received (GtkWidget *widget,
                                GdkDragContext *context,
                                gint x, gint y,
                                GtkSelectionData *seldata,
                                guint info,
                                guint time,
                                gpointer data)
{
   GimvThumbWin *tw = data;
   GimvThumbView *tv;
   GtkWidget *src_widget;
   GList *list;
   gpointer p;
   gint src, dest;

   switch (info) {
   case TARGET_URI_LIST:
      list = dnd_get_file_list (seldata->data, seldata->length);
      open_images_dirs (list, tw, LOAD_CACHE, FALSE);
      g_list_foreach (list, (GFunc) g_free, NULL);
      g_list_free (list);
      break;

   case TARGET_GIMV_TAB:
      src_widget = gtk_drag_get_source_widget (context);
      if (src_widget && GTK_IS_NOTEBOOK (src_widget)) {
         GimvThumbWin *tw_src, *tw_dest = tw;

         tw_src = g_object_get_data (G_OBJECT (src_widget), "thumbwin");
         if (!tw_src) return;

         /* if deferent window, detach tab */
         if (tw_src != tw_dest) {
            tv = gimv_thumb_win_find_thumbtable (tw_src, GIMV_THUMB_WIN_CURRENT_PAGE);
            if (!tv) return;

            gimv_thumb_win_detach_tab (tw_dest, tw_src, tv);
         }
      }
      break;

   case TARGET_GIMV_COMPONENT:
      src_widget = gtk_drag_get_source_widget (context);
      if (!src_widget) return;
      if (gdk_window_get_toplevel (src_widget->window)
          != gdk_window_get_toplevel (widget->window))
         {
            return;
         }

      p = g_object_get_data (G_OBJECT (src_widget), "gimv-component");
      src = GPOINTER_TO_INT (p);
      if (!src) return;

      p = g_object_get_data (G_OBJECT (widget), "gimv-component");
      dest = GPOINTER_TO_INT (p);
      if (!dest) return;

      /* to avoid gtk's bug, exec redraw after exit this callback function */
      {
         SwapCom *swap = g_new0 (SwapCom, 1);
         swap->tw = tw;
         swap->src = src;
         swap->dest = dest;
         gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                            idle_gimv_thumb_win_swap_component, NULL, swap,
                            (GtkDestroyNotify) g_free);
      }

      break;
   }
}


static void
cb_tab_drag_data_received (GtkWidget *widget,
                           GdkDragContext *context,
                           gint x, gint y,
                           GtkSelectionData *seldata,
                           guint info,
                           guint time,
                           gpointer data)
{
   GimvThumbWin *tw = data;
   GimvThumbView *tv;
   GtkWidget *page_container;
   GtkWidget *src_widget;
   gint pagenum;
   gpointer p;
   gint src, dest;

   g_return_if_fail (widget && tw);

   page_container = g_object_get_data (G_OBJECT (widget), "page-container");
   if (!page_container) return;

   switch (info) {
   case TARGET_URI_LIST:
      pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook),
                                       page_container);

      tv = gimv_thumb_win_find_thumbtable (tw, pagenum);
      if (!tv) return;

      gimv_thumb_view_drag_data_received_cb (widget, context, x, y,
                                             seldata, info, time, tv);
      break;

   case TARGET_GIMV_TAB:
      src_widget = gtk_drag_get_source_widget (context);
      if (src_widget && GTK_IS_NOTEBOOK (src_widget)) {
         GtkNotebook *notebook = GTK_NOTEBOOK (src_widget);
         GtkWidget *src_page, *newtab;
         GimvThumbWin *tw_src, *tw_dest = tw;
         gint src_pagenum, dest_pagenum;

         tw_src = g_object_get_data (G_OBJECT (src_widget), "thumbwin");
         if (!tw_src) return;

         dest_pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (tw_dest->notebook),
                                               page_container);

         /* if same window, reorder pages */
         if (tw_src == tw_dest) {
            src_pagenum = gtk_notebook_get_current_page (notebook);
            if (src_pagenum < 0) return;

            src_page = gtk_notebook_get_nth_page (notebook, src_pagenum);
            if (!src_page) return;

            gtk_notebook_reorder_child (notebook, src_page, dest_pagenum);

            /* if deferent window, detach tab */
         } else {
            tv = gimv_thumb_win_find_thumbtable (tw_src, GIMV_THUMB_WIN_CURRENT_PAGE);
            if (!tv) return;

            if (dest_pagenum < 0) return;

            newtab = gimv_thumb_win_detach_tab (tw_dest, tw_src, tv);
            gtk_notebook_reorder_child (GTK_NOTEBOOK (tw_dest->notebook),
                                        newtab, dest_pagenum);	    
         }
      }
      break;

   case TARGET_GIMV_COMPONENT:
      src_widget = gtk_drag_get_source_widget (context);
      if (!src_widget) return;
      if (gdk_window_get_toplevel (src_widget->window)
          != gdk_window_get_toplevel (widget->window))
         {
            return;
         }

      p = g_object_get_data (G_OBJECT (src_widget), "gimv-component");
      src = GPOINTER_TO_INT (p);
      if (!src) return;

      p = g_object_get_data (G_OBJECT (widget), "gimv-component");
      dest = GPOINTER_TO_INT (p);
      if (!dest) return;

      {
         SwapCom *swap = g_new0 (SwapCom, 1);
         swap->tw = tw;
         swap->src = src;
         swap->dest = dest;
         gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                            idle_gimv_thumb_win_swap_component, NULL, swap,
                            (GtkDestroyNotify) g_free);
      }

      break;
   }
}


static void
cb_com_swap_drag_data_received (GtkWidget *widget,
                                GdkDragContext *context,
                                gint x, gint y,
                                GtkSelectionData *seldata,
                                guint info,
                                guint time,
                                gpointer data)
{
   GimvThumbWin *tw = data;
   GtkWidget *src_widget;
   gpointer p;
   gint src, dest;

   switch (info) {
   case TARGET_GIMV_COMPONENT:
      src_widget = gtk_drag_get_source_widget (context);
      if (!src_widget) return;
      if (gdk_window_get_toplevel (src_widget->window)
          != gdk_window_get_toplevel (widget->window))
         {
            return;
         }

      p = g_object_get_data (G_OBJECT (src_widget), "gimv-component");
      src = GPOINTER_TO_INT (p);
      if (!src) return;

      p = g_object_get_data (G_OBJECT (widget), "gimv-component");
      dest = GPOINTER_TO_INT (p);
      if (!dest) return;

      {
         SwapCom *swap = g_new0 (SwapCom, 1);
         swap->tw = tw;
         swap->src = src;
         swap->dest = dest;
         gtk_idle_add_full (GTK_PRIORITY_REDRAW,
                            idle_gimv_thumb_win_swap_component, NULL, swap,
                            (GtkDestroyNotify) g_free);
      }

      break;

   default:
      break;
   }
}


#if 1   /* FIXME */
void
gimv_thumb_win_remove_key_accel (GimvThumbWin *tw)
{
   GSList *node;

   for (node = tw->accel_group_list; node; node = g_slist_next (node)) {
      GtkAccelGroup *accel_group = node->data;
      gtk_window_remove_accel_group (GTK_WINDOW (tw), accel_group);
   }
}


void
gimv_thumb_win_reset_key_accel (GimvThumbWin *tw)
{
   GSList *node;

   for (node = tw->accel_group_list; node; node = g_slist_next (node)) {
      GtkAccelGroup *accel_group = node->data;
      gtk_window_add_accel_group (GTK_WINDOW (tw), accel_group);
   }
}


static gboolean
cb_focus_in (GtkWidget *widget, GdkEventFocus *event, GimvThumbWin *tw)
{
   gimv_thumb_win_remove_key_accel (tw);
   return FALSE;
}


static gboolean
cb_focus_out (GtkWidget *widget, GdkEventFocus *event, GimvThumbWin *tw)
{
   gimv_thumb_win_reset_key_accel (tw);
   return FALSE;
}
#endif



/******************************************************************************
 *
 *   Utility.
 *
 ******************************************************************************/
GList *
gimv_thumb_win_get_list (void)
{
   return ThumbWinList;
}


GimvThumbView *
gimv_thumb_win_find_thumbtable (GimvThumbWin *tw, gint pagenum)
{
   GimvThumbView *tv;
   GtkWidget *tab;
   GList *node;
   gint i, num;

   g_return_val_if_fail (tw, NULL);
   node = g_list_find (ThumbWinList, tw);
   if (!node) return NULL;

   if (pagenum == GIMV_THUMB_WIN_CURRENT_PAGE)
      pagenum = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
   tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK(tw->notebook), pagenum);
   node = gimv_thumb_view_get_list();
   num = g_list_length (gimv_thumb_view_get_list());
   for (i = 0; i < num; i++) {
      tv = node->data;
      if (tv->container == tab) {
         return tv;
      }
      node = g_list_next (node);
   }
   return NULL;
}


void
gimv_thumb_win_set_statusbar_page_info (GimvThumbWin *tw, gint pagenum)
{
   GimvThumbView *tv;
   gint page, tv_filenum;
   gulong tv_filesize;
   gchar buf[BUF_SIZE];
   gfloat progress = 0.0;
   GList *node;
   gboolean update_statusbar1 = FALSE;

   g_return_if_fail (tw);
   node = g_list_find (ThumbWinList, tw);
   if (!node) return;

   if (pagenum == GIMV_THUMB_WIN_CURRENT_PAGE)
      page = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
   else
      page = pagenum;

   tv = gimv_thumb_win_find_thumbtable (tw, page);

   if (!tv) {
      gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), FALSE);
      g_snprintf (buf, BUF_SIZE, _("Empty"));
      tv_filenum = 0;
      tv_filesize = 0;
      progress = 0.0;
   } else if (tv->progress){
      if (conf.thumbwin_show_progress_detail) {
         gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), TRUE);
         g_snprintf (buf, BUF_SIZE, _("%d/%d files"),
                     tv->progress->pos, tv->progress->num);

         gtk_progress_set_format_string(GTK_PROGRESS(tw->progressbar), buf);
         if (tv->progress->now_file) {
            gchar *tmpstr;

            tmpstr = charset_to_internal (tv->progress->now_file,
                                          conf.charset_filename,
                                          conf.charset_auto_detect_fn,
                                          conf.charset_filename_mode);
            g_snprintf (buf, BUF_SIZE, "%s", tmpstr);
            g_free (tmpstr);
            update_statusbar1 = TRUE;
         } else {
            gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), FALSE);
         }
      }
      progress = (gfloat) tv->progress->pos / (gfloat) tv->progress->num;
      tv_filenum = tv->filenum;
      tv_filesize = tv->filesize;
   } else {
      const gchar *real_path;

      gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), FALSE);
      tv_filenum = tv->filenum;
      tv_filesize = tv->filesize;

      real_path = gimv_thumb_view_get_path (tv);
      if (real_path) {
         const gchar *format_str = NULL;
         gchar *path, *tmpstr;

         if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR)
            format_str = _("Dir View: %s");
         else if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE)
            format_str = _("Archive View: %s");

         if (format_str) {
            path = fileutil_home2tilde (real_path);
            tmpstr = charset_to_internal (path,
                                          conf.charset_filename,
                                          conf.charset_auto_detect_fn,
                                          conf.charset_filename_mode);

            g_snprintf (buf, BUF_SIZE, format_str, tmpstr);

            g_free (tmpstr);
            g_free (path);
         }
      } else {
         g_snprintf (buf, BUF_SIZE, _("Collection View"));
      }

      progress = 0.0;
      update_statusbar1 = TRUE;
   }

   if ((!tv || !tv->progress) && tw->status == GIMV_THUMB_WIN_STATUS_LOADING_BG) {
      gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), TRUE);
      gtk_progress_set_format_string(GTK_PROGRESS(tw->progressbar),
                                     _("Loading in another tab..."));
   }

   /* update status bar */
   if (update_statusbar1) {
      gtk_statusbar_pop (GTK_STATUSBAR (tw->status_bar1), 1);
      gtk_statusbar_push (GTK_STATUSBAR (tw->status_bar1), 1, buf);
   }
   g_snprintf (buf, BUF_SIZE, _("%d/%d page  %d/%d files  %ld/%ld kB"),
               page + 1, tw->pagenum, tv_filenum, tw->filenum,
               tv_filesize / 1024, tw->filesize / 1024);
   gtk_statusbar_pop (GTK_STATUSBAR (tw->status_bar2), 1);
   gtk_statusbar_push (GTK_STATUSBAR (tw->status_bar2), 1, buf);
   /* update progress bar */
   gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar), progress);
}


void
gimv_thumb_win_loading_update_progress (GimvThumbWin *tw, gint pagenum)
{
   GimvThumbView *tv;
   gint page;
   gchar buf[BUF_SIZE], *tmpstr;
   gfloat progress = 0.0;
   GList *node;

   g_return_if_fail (tw);
   node = g_list_find (ThumbWinList, tw);
   if (!node) return;

   if (pagenum == GIMV_THUMB_WIN_CURRENT_PAGE)
      page = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
   else
      page = pagenum;

   tv = gimv_thumb_win_find_thumbtable (tw, page);
   if (!tv) return;
   if (!tv->progress) return;

   if (conf.thumbwin_show_progress_detail) {
      gtk_progress_set_show_text(GTK_PROGRESS(tw->progressbar), TRUE);
      g_snprintf (buf, BUF_SIZE, _("%d/%d files"),
                  tv->progress->pos, tv->progress->num);
      gtk_progress_set_format_string(GTK_PROGRESS(tw->progressbar), buf);
      tmpstr = charset_to_internal (tv->progress->now_file,
                                    conf.charset_filename,
                                    conf.charset_auto_detect_fn,
                                    conf.charset_filename_mode);
      g_snprintf (buf, BUF_SIZE, "%s", tmpstr);
      g_free (tmpstr);
      gtk_statusbar_pop (GTK_STATUSBAR (tw->status_bar1), 1);
      gtk_statusbar_push (GTK_STATUSBAR (tw->status_bar1), 1, buf);
   }
   progress = (gfloat) tv->progress->pos / (gfloat) tv->progress->num;

   /* update progress bar */
   gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar), progress);
}


#define SET_SENSITIVE_WIDGETS_NUM 23

void
gimv_thumb_win_set_sensitive (GimvThumbWin *tw, GimvThumbwinStatus status)
{
   gboolean gimv_thumb_win_status [][SET_SENSITIVE_WIDGETS_NUM] =
      {
         /* status: normal */
         {TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
          TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE,
          TRUE,  TRUE,  TRUE},
         /* status: loading */
         {TRUE,  FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
          FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,
          FALSE, FALSE, FALSE},
         /* status: loading on BG */
         {TRUE,  FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
          FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,
          FALSE, FALSE, TRUE},
         /* status: checking duplicate images */
         {TRUE,  FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
          FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,
          FALSE, FALSE, TRUE},
         /* status: take all sensitive */
         {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
          FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
          FALSE, FALSE, FALSE},
      };
   GtkWidget *widget[SET_SENSITIVE_WIDGETS_NUM], *menu;
   GList *list, *node;
   guint i, num, summary_mode_menu_num;

   num = sizeof (gimv_thumb_win_status) / sizeof (gimv_thumb_win_status[0]);
   if (status > num - 1 || status < 0)
      status = 0;

   widget[0] = tw->menubar;
   widget[1] = tw->menuitem.file;
   widget[2] = tw->menuitem.edit;
   widget[3] = tw->view_menu;
   widget[4] = tw->menuitem.sort_name;
   widget[5] = tw->menuitem.sort_access;
   widget[6] = tw->menuitem.sort_time;
   widget[7] = tw->menuitem.sort_change;
   widget[8] = tw->menuitem.sort_size;
   widget[9] = tw->menuitem.sort_type;

   widget[10] = tw->menuitem.sort_width;
   widget[11] = tw->menuitem.sort_height;
   widget[12] = tw->menuitem.sort_area;
   widget[13] = tw->menuitem.sort_rev;
   widget[14] = tw->menuitem.sort_case;
   widget[15] = tw->menuitem.sort_dir;
   widget[16] = tw->button.fileopen;
   widget[17] = tw->button.refresh;
   widget[18] = tw->button.skip;
   widget[19] = tw->button.stop;

   widget[20] = tw->button.prefs;
   widget[21] = tw->button.quit;

   summary_mode_menu_num = 22;
   widget[summary_mode_menu_num] = tw->summary_mode_menu;

   for (i = 0; i < SET_SENSITIVE_WIDGETS_NUM; i++) {
      gtk_widget_set_sensitive (widget[i], gimv_thumb_win_status[status][i]);
   }

   /* set sensitivity of option menu items */
   menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (tw->summary_mode_menu));
   if (!menu) return;

   list = gtk_container_children (GTK_CONTAINER (menu));
   if (!list) return;

   for (node = list; node; node = g_list_next (node)) {
      GtkWidget *item = node->data;
      if (!item) continue;
      gtk_widget_set_sensitive (item, gimv_thumb_win_status[status][summary_mode_menu_num]);
   }

   tw->status = status;
}

#undef SET_SENSITIVE_WIDGETS_NUM


void
gimv_thumb_win_set_tab_label_text (GtkWidget *page_container,
                                   const gchar *title)
{
   GtkWidget *tablabel, *button;

   tablabel = g_object_get_data (G_OBJECT (page_container), "tab-label");

   if (tablabel) {
      button =   g_object_get_data (G_OBJECT (tablabel), "close-button");
      gtk_label_set_text (GTK_LABEL (tablabel), title);

      if (conf.thumbwin_show_tab_close) {
         gtk_widget_show (button);
      } else {
         gtk_widget_hide (button);
      }
   }

   gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (page_container->parent),
                                     page_container, title);
}


void
gimv_thumb_win_set_tab_label_state (GtkWidget *page_container,
                                    GtkStateType state)
{
   GtkWidget *tablabel;

   tablabel = g_object_get_data (G_OBJECT (page_container), "tab-label");

   if (tablabel) {
      gtk_widget_set_state (tablabel, state);
   }
}


static void
cb_pagecontainer_button_press (GtkWidget *widget,
                               GdkEventButton *event,
                               GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gimv_thumb_win_notebook_drag_src_unset (tw);   /* FIXMEEEEEEEE!! */
}


static void
cb_pagecontainer_thumb_button_release (GtkWidget *widget,
                                       GdkEventButton *event,
                                       GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gimv_thumb_win_notebook_drag_src_reset (tw);   /* FIXMEEEEEEEEE!!! */
}


static GtkWidget *
gimv_thumb_win_create_tab_container (GimvThumbWin *tw)
{
   GtkWidget *hbox, *pixmap, *button;
   GtkWidget *tablabel;
   GtkWidget *scrolled_window;
   GtkScrolledWindow *scrollwin;
   gint pagenum;
   gchar buf[BUF_SIZE];

   /* page container */
   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   scrollwin = GTK_SCROLLED_WINDOW (scrolled_window);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show (scrolled_window);
   g_signal_connect_after (G_OBJECT(scrolled_window), "button_press_event",
                           G_CALLBACK(cb_pagecontainer_button_press), tw);
   g_signal_connect_after (G_OBJECT(scrolled_window), "button_release_event",
                           G_CALLBACK(cb_pagecontainer_thumb_button_release), tw);
   g_signal_connect_after (G_OBJECT(scrollwin->hscrollbar), "button_press_event",
                           G_CALLBACK(cb_pagecontainer_button_press), tw);
   g_signal_connect_after (G_OBJECT(scrollwin->hscrollbar), "button_release_event",
                           G_CALLBACK(cb_pagecontainer_thumb_button_release), tw);
   g_signal_connect_after (G_OBJECT(scrollwin->vscrollbar), "button_press_event",
                           G_CALLBACK(cb_pagecontainer_button_press), tw);
   g_signal_connect_after (G_OBJECT(scrollwin->vscrollbar), "button_release_event",
                           G_CALLBACK(cb_pagecontainer_thumb_button_release), tw);

   /* page label widget */
   hbox = gtk_hbox_new (FALSE, 0);
   g_object_set_data (G_OBJECT (hbox), "page-container", scrolled_window);

   tablabel = gtk_label_new (_("New Tab"));
   gtk_widget_set_name (tablabel, "TabLabel");
   gtk_misc_set_alignment (GTK_MISC (tablabel), 0.00, 0.5);
   gtk_misc_set_padding (GTK_MISC (tablabel), 4, 0);
   gtk_box_pack_start(GTK_BOX(hbox), tablabel, TRUE, TRUE, 0);

   button = gtk_button_new();
   gtk_container_set_border_width (GTK_CONTAINER (button), 1);
   gtk_widget_set_size_request (button, 16, 16);
   gtk_button_set_relief((GtkButton *) button, GTK_RELIEF_NONE);
   pixmap = gimv_icon_stock_get_widget ("small_close");
   g_object_set_data (G_OBJECT (button), "page-container", scrolled_window);
   g_object_set_data (G_OBJECT (tablabel), "close-button", button);
   g_signal_connect (G_OBJECT (button), "clicked",
                     G_CALLBACK (cb_tab_close_button_clicked), tw);

   gtk_container_add(GTK_CONTAINER(button), pixmap);
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

   gtk_widget_show_all (hbox);

   if (conf.thumbwin_show_tab_close) {
      gtk_widget_show (button);
   } else {
      gtk_widget_hide (button);
   }

   dnd_dest_set (hbox, dnd_types_all, dnd_types_all_num);
   g_object_set_data (G_OBJECT (hbox),
                      "gimv-component",
                      GINT_TO_POINTER (GIMV_COM_THUMB_VIEW));
   g_signal_connect (G_OBJECT (hbox), "drag_data_received",
                     G_CALLBACK (cb_tab_drag_data_received), tw);

   /* create page */
   gtk_notebook_append_page (GTK_NOTEBOOK(tw->notebook), scrolled_window, hbox);
   g_object_set_data (G_OBJECT (scrolled_window), "tab-label", tablabel);

   tw->pagenum++;

   /* set label text */
   pagenum = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), scrolled_window);
   g_snprintf (buf, BUF_SIZE, _("NewTab %d"), tw->newpage_count++);
   gtk_label_set_text (GTK_LABEL (tablabel), buf);
   gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (tw->notebook),
                                     scrolled_window, buf);

   /* switch to this page */
   if (conf.thumbwin_move_to_newtab || tw->pagenum == 1)
      gtk_notebook_set_page (GTK_NOTEBOOK(tw->notebook), pagenum);

   return scrolled_window;
}


GimvThumbView *
gimv_thumb_win_create_new_tab (GimvThumbWin *tw)
{
   GtkWidget *scrolled_window;
   GimvThumbView *tv;
   FilesLoader *files;

   scrolled_window = gimv_thumb_win_create_tab_container (tw);

   /* create new collection */
   files = files_loader_new ();
   tv = gimv_thumb_view_new ();
   gimv_thumb_view_set_widget (tv, tw, scrolled_window,
                               tw->thumbview_summary_mode);
   gimv_thumb_win_set_statusbar_page_info (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   files_loader_delete (files);

   return tv;
}


GtkWidget *
gimv_thumb_win_detach_tab (GimvThumbWin *tw_dest, GimvThumbWin *tw_src,
                           GimvThumbView *tv)
{
   GtkWidget *newtab;
   const gchar *path;

   g_return_val_if_fail (tw_dest && tw_src && tv, FALSE);
   g_return_val_if_fail (tw_dest != tw_src, FALSE);
   g_return_val_if_fail (tw_src == tv->tw, FALSE);

   newtab = gimv_thumb_win_create_tab_container (tw_dest);
   gimv_thumb_win_set_tab_label_text (newtab, tv->tabtitle);

   /* move the table to new window */
   gimv_thumb_view_set_widget (tv, tw_dest, newtab, tv->summary_mode);

   /* reset widget */
   gtk_spin_button_set_value (GTK_SPIN_BUTTON (tw_dest->button.size_spin),
                              tv->thumb_size);
   gtk_option_menu_set_history (GTK_OPTION_MENU (tw_dest->summary_mode_menu),
                                gimv_thumb_view_label_to_num (tv->summary_mode));

   /* update struct data */
   tw_src->filenum -= tv->filenum;
   tw_src->filesize -= tv->filesize;
   tw_dest->filenum += tv->filenum;
   tw_dest->filesize += tv->filesize;

   gimv_thumb_win_close_tab (tw_src, GIMV_THUMB_WIN_CURRENT_PAGE);

   gimv_thumb_win_set_statusbar_page_info (tw_dest, GIMV_THUMB_WIN_CURRENT_PAGE);

   /* change current directory of dirview */
   path = gimv_thumb_view_get_path (tv);
   if (tv && tw_dest->show_dirview && path)
   {
      if (isdir(path)) {
         gimv_dir_view_change_dir (tw_dest->dv, path);
      } else if (isfile(path)) {
         gchar *dirname = g_dirname (path);
         gimv_dir_view_change_dir (tw_dest->dv, dirname);
         g_free(dirname);
      }
   }

   return newtab;
}


void
gimv_thumb_win_close_tab (GimvThumbWin *tw, gint page)
{
   gint page_tmp;
   GimvThumbView *tv;

   tv = gimv_thumb_win_find_thumbtable (tw, page);

   if (tv && tv->progress) return;
   if (tv && (tv->status == GIMV_THUMB_VIEW_STATUS_LOADING)) return;
   if (tv && (tv->status == GIMV_THUMB_VIEW_STATUS_CHECK_DUPLICATE)) return;

   if (tv)
      g_object_unref (G_OBJECT (tv));

   if (page == GIMV_THUMB_WIN_CURRENT_PAGE)
      page_tmp = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
   else
      page_tmp = page;

   gtk_notebook_remove_page (GTK_NOTEBOOK(tw->notebook), page_tmp);

   if (tw->pagenum > 0)
      tw->pagenum--;

   gimv_thumb_win_set_statusbar_page_info (tw, page);
}


void
gimv_thumb_win_reload_thumbnail (GimvThumbWin *tw, ThumbLoadType type)
{
   GimvThumbView *tv;
   gchar *path = NULL;

   tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   if (!tv) return;

   path = g_strdup (gimv_thumb_view_get_path (tv));

   if (tv->mode == GIMV_THUMB_VIEW_MODE_DIR) {
      open_dir_images (path, tw, tv, type, conf.scan_dir_recursive);

   } else if (tv->mode == GIMV_THUMB_VIEW_MODE_ARCHIVE) {
      open_archive_images (path, tw, tv, type);

   } else if (tv->mode == GIMV_THUMB_VIEW_MODE_COLLECTION) {
      FilesLoader *files = files_loader_new ();
      GList *node;

      for (node = tv->thumblist; node; node = g_list_next (node)) {
         GimvThumb  *thumb;
         gchar *filename;

         thumb = GIMV_THUMB (node->data);
         filename = g_strdup (gimv_image_info_get_path (thumb->info));
         files->filelist = g_list_append (files->filelist, filename);
      }
      files->status = END;
      files->thumb_load_type = type;

      open_image_files_in_thumbnail_view (files, tw, tv);
      files_loader_delete (files);
   }

   g_free (path);
}

void
gimv_thumb_win_open_dirview (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(tw->menuitem.dirview),
                                   TRUE);
}


void
gimv_thumb_win_close_dirview (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(tw->menuitem.dirview),
                                   FALSE);
}


void
gimv_thumb_win_open_preview (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(tw->menuitem.preview),
                                   TRUE);
}


void
gimv_thumb_win_close_preview (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(tw->menuitem.preview),
                                   FALSE);
}


static void
location_entry_set_text (GimvThumbWin  *tw,
                         GimvThumbView *tv,
                         const gchar   *location)
{
   gchar *text;
   gchar *internal_str;

   g_return_if_fail (tw);

   if (!location) {
      if (tv) {
         const gchar *path = gimv_thumb_view_get_path (tv);
         if (path && isdir (path))
            text = g_strdup (path);
         else if (path && isfile (path))
            text = g_dirname (path);
         else
            text = g_strdup (g_getenv ("HOME"));
      } else {
         text = g_strdup (g_getenv ("HOME"));
      }
   } else {
      text = g_strdup (location);
   }

   internal_str = charset_to_internal (text,
                                       conf.charset_filename,
                                       conf.charset_auto_detect_fn,
                                       conf.charset_filename_mode);

   gtk_entry_set_text (GTK_ENTRY (tw->location_entry), internal_str);

   g_free (internal_str);

   gtk_editable_set_position (GTK_EDITABLE (tw->location_entry), -1);

   g_free (text);
}


void
gimv_thumb_win_location_entry_set_text (GimvThumbWin *tw, const gchar *location)
{
   GimvThumbView *tv;

   g_return_if_fail (tw);

   if (!location) {
      tv = gimv_thumb_win_find_thumbtable (tw, GIMV_THUMB_WIN_CURRENT_PAGE);
   } else {
      tv = NULL;
   }

   location_entry_set_text (tw, tv, location);
}


GimvSortItem
gimv_thumb_win_get_sort_type (GimvThumbWin *tw, GimvSortFlag *flags_ret)
{
   g_return_val_if_fail (tw, GIMV_SORT_NONE);
   if (flags_ret)
      *flags_ret = tw->sortflags;
   return tw->sortitem;
}


void
gimv_thumb_win_sort_thumbnail (GimvThumbWin *tw,
                               GimvSortItem item, GimvSortFlag flags,
                               gint page)
{
   GtkWidget *sort_item = NULL;
   GtkCheckMenuItem *toggle;
   gboolean bool1, bool2;

   g_return_if_fail (tw);

   switch (item) {
   case GIMV_SORT_NAME:
      sort_item = tw->menuitem.sort_name;
      break;
   case GIMV_SORT_ATIME:
      sort_item = tw->menuitem.sort_access;
      break;
   case GIMV_SORT_MTIME:
      sort_item = tw->menuitem.sort_time;
      break;
   case GIMV_SORT_CTIME:
      sort_item = tw->menuitem.sort_change;
      break;
   case GIMV_SORT_SIZE:
      sort_item = tw->menuitem.sort_size;
      break;
   case GIMV_SORT_WIDTH:
      sort_item = tw->menuitem.sort_width;
      break;
   case GIMV_SORT_HEIGHT:
      sort_item = tw->menuitem.sort_height;
      break;
   case GIMV_SORT_AREA:
      sort_item = tw->menuitem.sort_area;
      break;
   case GIMV_SORT_TYPE:
      sort_item = tw->menuitem.sort_type;
      break;
   default:
      return;
   }

   if (!sort_item) return;

   if (!GTK_CHECK_MENU_ITEM(sort_item)->active)
      gtk_check_menu_item_set_active
         (GTK_CHECK_MENU_ITEM(sort_item), TRUE);

   toggle = GTK_CHECK_MENU_ITEM(tw->menuitem.sort_rev);
   bool1 = flags & GIMV_SORT_REVERSE;
   bool2 = tw->sortflags & GIMV_SORT_REVERSE;
   if (bool1 != bool2)
      gtk_check_menu_item_set_active (toggle, flags & GIMV_SORT_REVERSE);

   toggle = GTK_CHECK_MENU_ITEM(tw->menuitem.sort_case);
   bool1 = flags & GIMV_SORT_CASE_INSENSITIVE;
   bool2 = tw->sortflags & GIMV_SORT_CASE_INSENSITIVE;
   if (bool1 != bool2)
      gtk_check_menu_item_set_active (toggle, flags & GIMV_SORT_REVERSE);

   toggle = GTK_CHECK_MENU_ITEM(tw->menuitem.sort_dir);
   bool1 = flags & GIMV_SORT_DIR_INSENSITIVE;
   bool2 = tw->sortflags & GIMV_SORT_DIR_INSENSITIVE;
   if (bool1 != bool2)
      gtk_check_menu_item_set_active (toggle, flags & GIMV_SORT_REVERSE);
}


void
gimv_thumb_win_change_layout (GimvThumbWin *tw, gint layout)
{
   GimvThumbView *tv;
   GimvImageInfo *info = NULL;
   GtkWidget *tab, *new_main_contents;
   GList *pages_list = NULL, *node;
   gint i, pagenum, current_page;
   gfloat progress;

   if (tw->layout_type > -1 && tw->layout_type == layout) return;

   if (tw->changing_layout) return;
   tw->changing_layout = TRUE;

   gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_NO_SENSITIVE);

   if (tw->iv->info)
      info = gimv_image_info_ref (tw->iv->info);

   if (layout >= compose_type_num)
      tw->layout_type = 0;
   else
      tw->layout_type = layout;

   /* get pages list in this window */
   for (i = 0; i < tw->pagenum; i++) {
      tv = gimv_thumb_win_find_thumbtable (tw, i);
      pages_list = g_list_append (pages_list, tv);
   }

   current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK(tw->notebook));
   pagenum = tw->pagenum;

   /* create new main contents */
   if (tw->iv) {
      tw->iv = NULL;
      tw->preview = NULL;
   }
   if (tw->dv) {
      tw->dv = NULL;
   }
   new_main_contents = thumbnail_window_contents_new (tw);

   /* move pages */
   if (pages_list) {
      node = g_list_first (pages_list);

      for (i = 0; i < pagenum; i++) {
         tv = node->data;
         tab = gimv_thumb_win_create_tab_container (tw);
         if (tv && tab) {
            /* move to new tab */ 
            gimv_thumb_view_set_widget (tv, tv->tw, tab, tv->summary_mode);
            gimv_thumb_win_set_tab_label_text (tv->container, tv->tabtitle);
         }
         node = g_list_next (node);
         if (pagenum > 4) {
            progress = (gfloat) i / (gfloat) pagenum;
            gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar),
                                     progress);
            while (gtk_events_pending()) gtk_main_iteration();
         }
      }
      gtk_progress_bar_update (GTK_PROGRESS_BAR(tw->progressbar), 0.0);
   }

   gtk_widget_destroy (tw->main_contents);

   /* attach new main contents */
   gtk_container_add (GTK_CONTAINER (tw->main_vbox), new_main_contents);
   sync_widget_state_to_menu (tw);  /* reset widget state */
   gtk_widget_show (new_main_contents);
   tw->main_contents = new_main_contents;
   tw->pagenum = pagenum;

   gtk_notebook_set_page (GTK_NOTEBOOK (tw->notebook), current_page);
   gimv_thumb_win_set_statusbar_page_info (tw, GIMV_THUMB_WIN_CURRENT_PAGE);

   tv = gimv_thumb_win_find_thumbtable (tw, current_page);
   if (tv) {
      tw->thumbview_summary_mode = tv->summary_mode;
      gtk_option_menu_set_history (GTK_OPTION_MENU (tw->summary_mode_menu),
                                   gimv_thumb_view_label_to_num (tw->thumbview_summary_mode));
   }

   /* reset preview image */
   if (tw->show_preview && info)
      gimv_image_view_change_image (tw->iv, info);

   if (info) {
      gimv_image_info_unref (info);
      info = NULL;
   }

   while (gtk_events_pending()) gtk_main_iteration();

   if (!GIMV_IS_THUMB_WIN (tw)) return;

   if (GIMV_IS_THUMB_VIEW (tv)) {
      const gchar *path = gimv_thumb_view_get_path (tv);

      /* adjust dirview */
      if (tw->show_dirview && path) {
         if (path && isdir (path)) {
            gimv_dir_view_change_dir (tw->dv, path);
         } else if (path && isfile (path)) {
            gchar *dirname = g_dirname (path);
            gimv_dir_view_change_dir (tw->dv, dirname);
            g_free (dirname);
         }
      }
   }

   gimv_thumb_win_set_sensitive (tw, GIMV_THUMB_WIN_STATUS_NORMAL);

#if 1   /* FIXME */
   if (tw->accel_group_list)
      g_slist_free (tw->accel_group_list);
   tw->accel_group_list
      = g_slist_copy (gtk_accel_groups_from_object (G_OBJECT (tw)));
   tw->accel_group_list = g_slist_reverse (tw->accel_group_list);
#endif

   tw->changing_layout = FALSE;
}


void
gimv_thumb_win_swap_component (GimvThumbWin *tw,
                               GimvComponentType com1,
                               GimvComponentType com2)
{
   GimvThumbwinComposeType compose;
   GtkCheckMenuItem *menuitem;
   gboolean active;
   gint tmp = 0, *p1 = NULL, *p2 = NULL, i;

   g_return_if_fail (tw);
   g_return_if_fail (com1 > 0 && com1 < 4);
   g_return_if_fail (com2 > 0 && com2 < 4);

   if (com1 == com2) return;

   gimv_thumb_win_get_layout (&compose, tw->layout_type);

   for (i = 0; i < 3; i++) {
      if (compose.widget_type[i] == com1) {
         tmp = compose.widget_type[i];
         p1 = &conf.thumbwin_widget[i];
      } else if (compose.widget_type[i] == com2) {
         p2 = &conf.thumbwin_widget[i];
      }
   }

   g_return_if_fail (p1 && p2 && p1 != p2);

   conf.thumbwin_pane1_horizontal = compose.pane1_horizontal;
   conf.thumbwin_pane2_horizontal = compose.pane2_horizontal;
   conf.thumbwin_pane2_attach_1 = compose.pane2_attach_to_child1;
   for (i = 0; i < 3; i++)
      conf.thumbwin_widget[i] = compose.widget_type[i];

   *p1 = *p2;
   *p2 = tmp;

   menuitem = GTK_CHECK_MENU_ITEM(tw->menuitem.layout[5]);
   active = menuitem->active;
   gtk_check_menu_item_set_active (menuitem, TRUE);
   if (active)
      g_signal_emit_by_name (G_OBJECT (menuitem), "activate");
}


void
gimv_thumb_win_save_state (GimvThumbWin *tw)
{
   g_return_if_fail (GIMV_IS_THUMB_WIN (tw));

   if (!tw->fullscreen) {
      conf.thumbwin_width  = GTK_WIDGET (tw)->allocation.width;
      conf.thumbwin_height = GTK_WIDGET (tw)->allocation.height;
   } else {
      conf.thumbwin_width  = tw->win_width;
      conf.thumbwin_height = tw->win_height;
   }

   if (tw->show_preview)
      conf.thumbwin_pane_size2
         = gtk_paned_get_position (GTK_PANED (tw->pane2));
   if (tw->show_dirview)
      conf.thumbwin_pane_size1
         = gtk_paned_get_position (GTK_PANED (tw->pane1));

   conf.thumbwin_show_preview  = tw->show_preview;
   conf.thumbwin_show_dir_view = tw->show_dirview;

   conf.thumbwin_show_menubar
      = GTK_CHECK_MENU_ITEM (tw->menuitem.menubar)->active;
   conf.thumbwin_show_toolbar
      = GTK_CHECK_MENU_ITEM (tw->menuitem.toolbar)->active;
   conf.thumbwin_show_statusbar
      = GTK_CHECK_MENU_ITEM (tw->menuitem.statusbar)->active;
   conf.thumbwin_show_tab
      = GTK_CHECK_MENU_ITEM (tw->menuitem.tab)->active;
   conf.thumbwin_show_preview_tab
      = GTK_CHECK_MENU_ITEM (tw->menuitem.preview_tab)->active;

   g_object_get (G_OBJECT (tw->iv),
                 "show_scrollbar",   &conf.preview_scrollbar,
                 "continuance_play", &conf.imgview_movie_continuance,
                 NULL);
   conf.preview_player_visible = tw->player_visible;

   conf.thumbwin_layout_type = tw->layout_type;
   conf.thumbwin_tab_pos = tw->tab_pos;

   conf.thumbwin_thumb_size =
      gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(tw->button.size_spin));

   conf.thumbwin_sort_type        = tw->sortitem;
   conf.thumbwin_sort_reverse     = tw->sortflags & GIMV_SORT_REVERSE;
   conf.thumbwin_sort_ignore_case = tw->sortflags & GIMV_SORT_CASE_INSENSITIVE;
   conf.thumbwin_sort_ignore_dir  = tw->sortflags & GIMV_SORT_DIR_INSENSITIVE;

   if (!gimv_prefs_win_is_opened ())
      g_free (conf.thumbwin_disp_mode);
   conf.thumbwin_disp_mode = g_strdup (tw->thumbview_summary_mode);
}


/* FIXMEEEEEEEEEEE!!! (TOT */
void
gimv_thumb_win_notebook_drag_src_unset (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   gtk_drag_source_unset (tw->notebook);
}


void
gimv_thumb_win_notebook_drag_src_reset (GimvThumbWin *tw)
{
   g_return_if_fail (tw);

   dnd_src_set  (tw->notebook,
                 dnd_types_tab_component,
                 dnd_types_tab_component_num);
}
/* END FIXMEEEEEEEEEEE!!! (TOT */
