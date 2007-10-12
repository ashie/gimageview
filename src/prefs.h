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

#ifndef __PREFS_H__
#define __PREFS_H__

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define SCRIPTS_DEFAULT_SEARCH_DIR_LIST \
   DATADIR"/scripts"
#define PLUGIN_DEFAULT_SEARCH_DIR_LIST \
   PLUGINDIR "/" ARCHIVER_PLUGIN_DIR "," \
   PLUGINDIR "/" IMAGE_LOADER_PLUGIN_DIR "," \
   PLUGINDIR "/" IMAGE_SAVER_PLUGIN_DIR "," \
   PLUGINDIR "/" IO_STREAM_PLUGIN_DIR "," \
   PLUGINDIR "/" THUMBNAIL_PLUGIN_DIR "," \
   PLUGINDIR "/" IMAGE_VIEW_PLUGIN_DIR "," \
   PLUGINDIR "/" THUMBNAIL_VIEW_PLUGIN_DIR

typedef enum
{
   WINTYPE_DEFAULT = -1,
   IMAGE_VIEW_WINDOW = 0,
   THUMBNAIL_WINDOW  = 1
} ImgWinType;


typedef struct _Config {
   /* General Option */
   gint       default_file_open_window;
   gint       default_dir_open_window;
   gint       default_arc_open_window;
   gint       scan_dir_recursive;
   gboolean   recursive_follow_link;
   gboolean   recursive_follow_parent;
   gboolean   read_dotfile;
   gboolean   detect_filetype_by_ext;
   gboolean   disp_filename_stdout;
   gint       interpolation;
   gboolean   fast_scale_down;
   gboolean   conv_rel_path_to_abs;
   gchar     *iconset;
   gchar     *textentry_font;

   /* Charset related common option */
   gchar     *charset_locale;
   gchar     *charset_internal;
   gint       charset_auto_detect_lang;
   gint       charset_filename_mode;
   gchar     *charset_filename;
   gpointer   charset_auto_detect_fn;   /* initialized in gimageview.c */

   /* Start up Option */
   gboolean   startup_read_dir;
   gboolean   startup_open_thumbwin;
   gboolean   startup_no_warning;

   /* filter */
   gchar     *imgtype_disables;
   gchar     *imgtype_user_defs;

   /* Image View */
   gboolean   imgwin_save_win_state;
   guint      imgwin_width;
   guint      imgwin_height;
   gboolean   imgwin_fit_to_image;
   gboolean   imgwin_open_new_win;
   gboolean   imgwin_raise_window;
   gboolean   imgwin_show_menubar;
   gboolean   imgwin_show_toolbar;
   gboolean   imgwin_show_player;
   gboolean   imgwin_show_statusbar;
   gint       imgwin_toolbar_style;
   gboolean   imgwin_set_bg;
   gint       imgwin_bg_color[3];             /* red green blue */
   gboolean   imgwin_fullscreen_set_bg;
   gint       imgwin_fullscreen_bg_color[3];  /* red green blue */

   gfloat     imgview_scale;
   gboolean   imgview_keep_aspect;
   gint       imgview_default_zoom;
   gint       imgview_default_rotation;
   gboolean   imgview_buffer;
   gboolean   imgview_scrollbar;
   gint       imgview_player_visible;
   gboolean   imgview_scroll_nolimit;
   gboolean   imgview_movie_continuance;

   gchar     *imgview_mouse_button;

   /* Thumbnail View */
   gboolean   thumbwin_save_win_state;
   guint      thumbwin_width;
   guint      thumbwin_height;
   gint       thumbwin_layout_type;
   gboolean   thumbwin_pane1_horizontal;
   gboolean   thumbwin_pane2_horizontal;
   gboolean   thumbwin_pane2_attach_1;
   gint       thumbwin_widget[3];
   guint      thumbwin_pane_size1;
   guint      thumbwin_pane_size2;

   gboolean   thumbwin_show_dir_view;
   gboolean   thumbwin_show_preview;
   gboolean   thumbwin_show_menubar;
   gboolean   thumbwin_show_toolbar;
   gboolean   thumbwin_show_statusbar;
   gboolean   thumbwin_show_tab;
   gboolean   thumbwin_show_preview_tab;

   gboolean   thumbwin_raise_window;
   gint       thumbwin_toolbar_style;
   gchar     *thumbwin_disp_mode;

   gint       thumbwin_tab_pos;
   gboolean   thumbwin_move_to_newtab;
   gboolean   thumbwin_show_tab_close;
   gboolean   thumbwin_tab_fullpath;
   gboolean   thumbwin_force_open_tab;

   guint      thumbwin_thumb_size;
   guint      thumbwin_redraw_interval;
   gboolean   thumbwin_show_progress_detail;

   guint      thumbwin_sort_type;
   gboolean   thumbwin_sort_reverse;
   gboolean   thumbwin_sort_ignore_case;
   gboolean   thumbwin_sort_ignore_dir;

   gboolean   thumbview_show_dir;
   gboolean   thumbview_show_archive;

   gchar     *thumbview_mouse_button;

   /* Thumbnail Album */
   guint      thumbalbum_row_space;
   guint      thumbalbum_col_space;

   /* Directory View */
   gboolean   dirview_show_toolbar;
   gboolean   dirview_show_dotfile;
   gboolean   dirview_show_current_dir;
   gboolean   dirview_show_parent_dir;
   gint       dirview_line_style;
   gint       dirview_expander_style;
   gboolean   dirview_auto_scroll;
   gint       dirview_auto_scroll_time;
   gboolean   dirview_auto_expand;
   gint       dirview_auto_expand_time;
   gchar     *dirview_mouse_button;

   /* Preview */
   gfloat     preview_scale;
   gint       preview_zoom;
   gint       preview_rotation;
   gboolean   preview_keep_aspect;
   gboolean   preview_scrollbar;
   gint       preview_player_visible;
   gboolean   preview_buffer;
   gchar     *preview_mouse_button;

   /* Thumbnail Cache */
   gchar     *cache_read_list;
   gchar     *cache_write_type;

   /* comment */
   gchar     *comment_key_list;

   gint       comment_charset_read_mode;
   gint       comment_charset_write_mode;
   gchar     *comment_charset;

   /* Movie */
   gchar     *movie_default_view_mode;

   /* Slide Show */
   gint       slideshow_zoom;
   gint       slideshow_rotation;
   gfloat     slideshow_img_scale;
   gint       slideshow_screen_mode;
   gboolean   slideshow_menubar;
   gboolean   slideshow_toolbar;
   gboolean   slideshow_player;
   gboolean   slideshow_statusbar;
   gboolean   slideshow_scrollbar;
   gboolean   slideshow_keep_aspect;
   gfloat     slideshow_interval;
   gboolean   slideshow_repeat;
   gboolean   slideshow_set_bg;
   gint       slideshow_bg_color[3];  /* red green blue */

   /* search */
   gfloat     search_similar_accuracy;
   gboolean   search_similar_open_collection;

   /* similar window */
   gboolean   simwin_sel_thumbview;
   gboolean   simwin_sel_preview;
   gboolean   simwin_sel_new_win;
   gboolean   simwin_sel_shared_win;

   /* drag and drop */
   gboolean   dnd_enable_to_external;
   gboolean   dnd_enable_from_external;
   gboolean   dnd_refresh_list_always;

   /* Wallpaper */
   gchar     *wallpaper_menu;

   /* External Program */
   gchar     *progs[16];
   gchar     *web_browser;
   gchar     *text_viewer;
   gboolean   text_viewer_use_internal;
   gboolean   scripts_use_default_search_dir_list;
   gchar     *scripts_search_dir_list;
   gboolean   scripts_show_dialog;

   /* plugin */
   gboolean   plugin_use_default_search_dir_list;
   gchar     *plugin_search_dir_list;
} Config;

typedef struct _KeyConf {
   gchar *common_open;
   gchar *common_imagewin;
   gchar *common_thumbwin;
   gchar *common_quit;
   gchar *common_prefs;
   gchar *common_toggle_menubar;
   gchar *common_toggle_toolbar;
   gchar *common_toggle_tab;
   gchar *common_toggle_statusbar;
   gchar *common_toggle_scrollbar;
   gchar *common_fullscreen;
   gchar *common_create_thumb;
   gchar *common_auto_completion1;
   gchar *common_auto_completion2;
   gchar *common_popup_menu;

   gchar *imgwin_toggle_player;
   gchar *imgwin_buffer;
   gchar *imgwin_close;

   gchar *imgwin_fit_win;

   gchar *imgwin_zoomin;
   gchar *imgwin_zoomout;
   gchar *imgwin_fit_img;
   gchar *imgwin_keep_aspect;
   gchar *imgwin_zoom10;
   gchar *imgwin_zoom25;
   gchar *imgwin_zoom50;
   gchar *imgwin_zoom75;
   gchar *imgwin_zoom100;
   gchar *imgwin_zoom125;
   gchar *imgwin_zoom150;
   gchar *imgwin_zoom175;
   gchar *imgwin_zoom200;

   gchar *imgwin_rotate_ccw;
   gchar *imgwin_rotate_cw;
   gchar *imgwin_rotate_180;

   gchar *imgwin_next;
   gchar *imgwin_prev;
   gchar *imgwin_first;
   gchar *imgwin_last;

   gchar *thumbwin_disp_mode[16];

   gchar *thumbwin_layout0;
   gchar *thumbwin_layout1;
   gchar *thumbwin_layout2;
   gchar *thumbwin_layout3;
   gchar *thumbwin_layout4;
   gchar *thumbwin_custom;
   gchar *thumbwin_slideshow;
   gchar *thumbwin_first_page;
   gchar *thumbwin_last_page;
   gchar *thumbwin_next_page;
   gchar *thumbwin_prev_page;
   gchar *thumbwin_toggle_dir_toolbar;
   gchar *thumbwin_toggle_dirview;
   gchar *thumbwin_toggle_preview;
   gchar *thumbwin_toggle_preview_tab;
   gchar *thumbwin_new_tab;
   gchar *thumbwin_new_collection;
   gchar *thumbwin_close_tab;
   gchar *thumbwin_close_win;

   gchar *thumbwin_select_all;
   gchar *thumbwin_unselect_all;
   gchar *thumbwin_refresh_list;
   gchar *thumbwin_reload_cache;
   gchar *thumbwin_recreate_thumb;
   gchar *thumbwin_rename;
   gchar *thumbwin_copy;
   gchar *thumbwin_move;
   gchar *thumbwin_link;
   gchar *thumbwin_find_similar;
   gchar *thumbwin_move_tab_forward;
   gchar *thumbwin_move_tab_backward;
   gchar *thumbwin_detach_tab;

   gchar *thumbwin_sort_name;
   gchar *thumbwin_sort_atime;
   gchar *thumbwin_sort_mtime;
   gchar *thumbwin_sort_ctime;
   gchar *thumbwin_sort_size;
   gchar *thumbwin_sort_type;
   gchar *thumbwin_sort_rev;
} KeyConf;

extern Config conf;
extern KeyConf akey;

void prefs_load_config              (void);
void prefs_save_config              (void);
gint prefs_mouse_get_num            (gint            button_id,
                                     gint            mod_id,
                                     const gchar    *string);
gint prefs_mouse_get_num_from_event (GdkEventButton *event,
                                     const gchar    *string);

#endif /* __PREFS_H__ */
