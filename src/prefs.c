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
#include <sys/stat.h>

#include "gimageview.h"

#include "gimv_icon_stock.h"
#include "gimv_image_win.h"
#include "gimv_plugin.h"
#include "gimv_thumb_win.h"
#include "prefs.h"

Config conf;
KeyConf akey;


typedef enum {
   D_NULL,
   D_STRING,
   D_INT,
   D_FLOAT,
   D_BOOL,
   D_ENUM
} DataType;


typedef struct _ConfParm
{
   gchar    *keyname;
   DataType  data_type;
   gchar    *default_val;
   gpointer  data;
} ConfParam;


typedef struct _KeyBindIfactory
{
   gpointer keyconf;
   GtkItemFactoryEntry *ifactory;
   gchar   *path;
} KeyBindIfactory;


typedef struct _PrefsSection
{
   gchar     *section_name;
   ConfParam *param;
} PrefsSection;


typedef struct _PrefsFile
{
   gchar        *filename;
   PrefsSection *sections;
} PrefsFile;


/***************************
 * default config value
 ***************************/
static ConfParam param_common [] = {
   {"default_file_open_window", D_ENUM, "0",     &conf.default_file_open_window},
   {"default_dir_open_window",  D_ENUM, "1",     &conf.default_dir_open_window},
   {"default_arc_open_window",  D_ENUM, "1",     &conf.default_arc_open_window},
   {"scan_dir_recursive",       D_BOOL, "FALSE", &conf.scan_dir_recursive},
   {"recursive_follow_link",    D_BOOL, "FALSE", &conf.recursive_follow_link},
   {"recursive_follow_parent",  D_BOOL, "FALSE", &conf.recursive_follow_parent},
   {"read_dotfile",             D_BOOL, "FALSE", &conf.read_dotfile},

   {"detect_filetype_by_ext",   D_BOOL,   "TRUE",  &conf.detect_filetype_by_ext},
   {"disp_filename_stdout",     D_BOOL,   "TRUE",  &conf.disp_filename_stdout},
   {"interpolation",            D_ENUM,   "1",     &conf.interpolation},
   {"fast_scale_down",          D_BOOL,   "FALSE", &conf.fast_scale_down},
   {"conv_rel_path_to_abs",     D_BOOL,   "TRUE", &conf.conv_rel_path_to_abs},
   {"iconset",                  D_STRING, DEFAULT_ICONSET, &conf.iconset},
   {"textentry_font",           D_STRING,
    "-alias-fixed-medium-r-normal--14-*-*-*-*-*-*-*", &conf.textentry_font},

   /* charset */
   {"charset_locale",           D_STRING, "default", &conf.charset_locale},
   {"charset_internal",         D_STRING, "default", &conf.charset_internal},
   {"charset_auto_detect_lang", D_ENUM,   "0",       &conf.charset_auto_detect_lang},
   {"charset_filename_mode",    D_ENUM,   "1",       &conf.charset_filename_mode},
   {"charset_filename",         D_STRING, "default", &conf.charset_filename},

   /* filter */
   {"imgtype_disables",         D_STRING, NULL,  &conf.imgtype_disables},
   {"imgtype_user_defs",        D_STRING, NULL,  &conf.imgtype_user_defs},

   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_startup [] = {
   {"startup_read_dir",           D_BOOL, "FALSE", &conf.startup_read_dir},
   {"startup_open_thumbwin",      D_BOOL, "FALSE", &conf.startup_open_thumbwin},
   {"startup_no_warning",         D_BOOL, "FALSE", &conf.startup_no_warning},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_imageview [] = {
   {"imgwin_save_win_state",      D_BOOL, "TRUE",  &conf.imgwin_save_win_state},
   {"imgwin_width",               D_INT,  "600",   &conf.imgwin_width},
   {"imgwin_height",              D_INT,  "450",   &conf.imgwin_height},
   {"imgwin_fit_to_image",        D_BOOL, "FALSE", &conf.imgwin_fit_to_image},
   {"imgwin_open_new_win",        D_BOOL, "FALSE", &conf.imgwin_open_new_win},
   {"imgwin_raise_window",        D_BOOL, "TRUE",  &conf.imgwin_raise_window},

   {"imgwin_show_menubar",        D_BOOL, "TRUE",  &conf.imgwin_show_menubar},
   {"imgwin_show_toolbar",        D_BOOL, "TRUE",  &conf.imgwin_show_toolbar},
   {"imgwin_show_player",         D_BOOL, "FALSE", &conf.imgwin_show_player},
   {"imgwin_show_statusbar",      D_BOOL, "TRUE",  &conf.imgwin_show_statusbar},
   {"imgwin_toolbar_style",       D_ENUM, "0",     &conf.imgwin_toolbar_style},
   {"imgwin_set_bg",              D_BOOL, "FALSE", &conf.imgwin_set_bg},
   {"imgwin_bg_red",              D_INT,  "0",     &conf.imgwin_bg_color[0]},
   {"imgwin_bg_green",            D_INT,  "0",     &conf.imgwin_bg_color[1]},
   {"imgwin_bg_blue",             D_INT,  "0",     &conf.imgwin_bg_color[2]},
   {"imgwin_fullscreen_set_bg",   D_BOOL, "FALSE", &conf.imgwin_fullscreen_set_bg},
   {"imgwin_fullscreen_bg_red",   D_INT,  "0",     &conf.imgwin_fullscreen_bg_color[0]},
   {"imgwin_fullscreen_bg_green", D_INT,  "0",     &conf.imgwin_fullscreen_bg_color[1]},
   {"imgwin_fullscreen_bg_blue",  D_INT,  "0",     &conf.imgwin_fullscreen_bg_color[2]},

   {"imgview_default_zoom",       D_ENUM,   "3",       &conf.imgview_default_zoom},
   {"imgview_default_rotation",   D_ENUM,   "0",       &conf.imgview_default_rotation},
   {"imgview_keep_aspect",        D_BOOL,   "TRUE",    &conf.imgview_keep_aspect},
   {"imgview_scale",              D_FLOAT,  "100.0",   &conf.imgview_scale},
   {"imgview_buffer",             D_BOOL,   "FALSE",   &conf.imgview_buffer},
   {"imgview_scrollbar",          D_BOOL,   "TRUE",    &conf.imgview_scrollbar},
   {"imgview_player_visible",     D_ENUM,   "2",       &conf.imgview_player_visible},
   {"imgview_scroll_nolimit",     D_BOOL,   "FALSE",   &conf.imgview_scroll_nolimit},
   {"imgview_movie_continuance",  D_BOOL,   "FALSE",   &conf.imgview_movie_continuance},
   {"movie_default_view_mode",    D_STRING, NULL,      &conf.movie_default_view_mode},
   {"imgview_mouse_button",       D_STRING,
    "0,0,0,0; -1,-2,0,0; -2,0,0,0; 3,0,0,0; 2,0,0,0; 1,0,0,0",
    &conf.imgview_mouse_button},
   {NULL, D_NULL, NULL, NULL}
};


static ConfParam param_thumbview [] = {
   {"thumbwin_save_win_state",  D_BOOL,   "TRUE",  &conf.thumbwin_save_win_state},
   {"thumbwin_width",           D_INT,    "600",   &conf.thumbwin_width},
   {"thumbwin_height",          D_INT,    "450",   &conf.thumbwin_height},
   {"thumbwin_layout_type",     D_INT,    "0",     &conf.thumbwin_layout_type},
   {"thumbwin_pane1_horizontal",D_BOOL,   "TRUE",  &conf.thumbwin_pane1_horizontal},
   {"thumbwin_pane2_horizontal",D_BOOL,   "TRUE",  &conf.thumbwin_pane2_horizontal},
   {"thumbwin_pane2_attach_1",  D_BOOL,   "FALSE", &conf.thumbwin_pane2_attach_1},
   {"thumbwin_widget[0]",       D_INT,    "1",     &conf.thumbwin_widget[0]},
   {"thumbwin_widget[1]",       D_INT,    "2",     &conf.thumbwin_widget[1]},
   {"thumbwin_widget[2]",       D_INT,    "3",     &conf.thumbwin_widget[2]},
   {"thumbwin_pane_size1",      D_INT,    "200",   &conf.thumbwin_pane_size1},
   {"thumbwin_pane_size2",      D_INT,    "200",   &conf.thumbwin_pane_size2},

   {"thumbwin_show_dir_view",   D_BOOL,   "TRUE",  &conf.thumbwin_show_dir_view},
   {"thumbwin_show_preview",    D_BOOL,   "FALSE", &conf.thumbwin_show_preview},
   {"thumbwin_show_menubar",    D_BOOL,   "TRUE",  &conf.thumbwin_show_menubar},
   {"thumbwin_show_toolbar",    D_BOOL,   "TRUE",  &conf.thumbwin_show_toolbar},
   {"thumbwin_show_statusbar",  D_BOOL,   "TRUE",  &conf.thumbwin_show_statusbar},
   {"thumbwin_show_tab",        D_BOOL,   "TRUE",  &conf.thumbwin_show_tab},
   {"thumbwin_show_preview_tab",D_BOOL,   "TRUE",  &conf.thumbwin_show_preview_tab},

   {"thumbwin_raise_window",    D_BOOL,   "TRUE",  &conf.thumbwin_raise_window},
   {"thumbwin_toolbar_style",   D_ENUM,   "0",     &conf.thumbwin_toolbar_style},

   {"thumbwin_disp_mode",       D_STRING, GIMV_THUMB_VIEW_DEFAULT_SUMMARY_MODE, &conf.thumbwin_disp_mode},
   {"thumbwin_tab_pos",         D_ENUM,   "2",     &conf.thumbwin_tab_pos},
   {"thumbwin_move_to_newtab",  D_BOOL,   "TRUE",  &conf.thumbwin_move_to_newtab},
   {"thumbwin_show_tab_close",  D_BOOL,   "TRUE",  &conf.thumbwin_show_tab_close},
   {"thumbwin_tab_fullpath",    D_BOOL,   "TRUE",  &conf.thumbwin_tab_fullpath},
   {"thumbwin_force_open_tab",  D_BOOL,   "FALSE", &conf.thumbwin_force_open_tab},

   {"thumbwin_thumb_size",      D_INT,    "96",    &conf.thumbwin_thumb_size},
   {"thumbwin_redraw_interval", D_INT,    "1",     &conf.thumbwin_redraw_interval},
   {"thumbwin_show_progress_detail", D_BOOL, "TRUE", &conf.thumbwin_show_progress_detail},

   {"thumbwin_sort_type",       D_ENUM,   "0",     &conf.thumbwin_sort_type},
   {"thumbwin_sort_reverse",    D_ENUM,   "FALSE", &conf.thumbwin_sort_reverse},
   {"thumbwin_sort_ignore_case",D_ENUM,   "FALSE", &conf.thumbwin_sort_ignore_case},
   {"thumbwin_sort_ignore_dir", D_ENUM,   "FALSE", &conf.thumbwin_sort_ignore_dir},

   {"thumbview_show_dir",       D_BOOL,   "FALSE", &conf.thumbview_show_dir},
   {"thumbview_show_archive",   D_BOOL,   "TRUE",  &conf.thumbview_show_archive},

   {"thumbview_mouse_button", D_STRING,
    "3,0,0,0; -2,0,0,0; -3,0,0,0; 1,0,0,0; 0,0,0,0; 0,0,0,0",
    &conf.thumbview_mouse_button},
};

static ConfParam param_thumbalbum [] = {
   {"thumbalbum_row_space",     D_INT,    "8",     &conf.thumbalbum_row_space},
   {"thumbalbum_col_space",     D_INT,    "8",     &conf.thumbalbum_col_space},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_preview [] = {
   {"preview_zoom",               D_ENUM, "3",     &conf.preview_zoom},
   {"preview_rotation",           D_ENUM, "0",     &conf.preview_rotation},
   {"preview_keep_aspect",        D_BOOL, "TRUE",  &conf.preview_keep_aspect},
   {"preview_scale",              D_FLOAT,"100.0", &conf.preview_scale},
   {"preview_scrollbar",          D_BOOL, "TRUE",  &conf.preview_scrollbar},
   {"preview_player_visible",     D_ENUM, "2",     &conf.preview_player_visible},
   {"preview_buffer",             D_BOOL, "FALSE", &conf.preview_buffer},
   {"preview_mouse_button",       D_STRING,
    "0,0,0,0; -1,-2,0,0; -4,0,0,0; 5,0,0,0; 2,0,0,0; 1,0,0,0",
    &conf.preview_mouse_button},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_dirview [] = {
   {"dirview_show_toolbar",     D_BOOL,   "TRUE",   &conf.dirview_show_toolbar},
   {"dirview_show_dotfile",     D_BOOL,   "FALSE",  &conf.dirview_show_dotfile},
   {"dirview_show_current_dir", D_BOOL,   "FALSE",  &conf.dirview_show_current_dir},
   {"dirview_show_parent_dir",  D_BOOL,   "FALSE",  &conf.dirview_show_parent_dir},
   {"dirview_line_style",       D_ENUM,   "2",      &conf.dirview_line_style},
   {"dirview_expander_style",   D_ENUM,   "1",      &conf.dirview_expander_style},
   {"dirview_auto_scroll",      D_BOOL,   "TRUE",   &conf.dirview_auto_scroll},
   {"dirview_auto_scroll_time", D_INT,    "50",     &conf.dirview_auto_scroll_time},
   {"dirview_auto_expand",      D_BOOL,   "FALSE",  &conf.dirview_auto_expand},
   {"dirview_auto_expand_time", D_INT,    "500",    &conf.dirview_auto_expand_time},
   {"dirview_mouse_button",     D_STRING,
    "1,0,0,0; 0,0,0,0; 1,0,0,0; 3,0,0,0; 0,0,0,0; 0,0,0,0",
    &conf.dirview_mouse_button},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_cache [] = {
   {"cache_read_list",          D_STRING,
    "GImageView,Nautilus,Nautilus-2.0,"
    "Konqueror (Large),Konqueror (Medium),Konqueror (Small),"
    "GQview,Electric Eyes (Preview),Electric Eyes (Icon),XV thumbnail",
    &conf.cache_read_list},
   {"cache_write_type",         D_STRING, "GImageView", &conf.cache_write_type},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_comment [] = {
   {"comment_key_list",           D_STRING, NULL,      &conf.comment_key_list},
   {"comment_charset_read_mode",  D_ENUM,   "3",       &conf.comment_charset_read_mode},
   {"comment_charset_write_mode", D_ENUM,   "3",       &conf.comment_charset_write_mode},
   {"comment_charset",            D_STRING, "default", &conf.comment_charset},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_slideshow [] = {
   {"slideshow_zoom",             D_ENUM, "3",     &conf.slideshow_zoom},
   {"slideshow_rotation",         D_ENUM, "0",     &conf.slideshow_rotation},
   {"slideshow_img_scale",        D_FLOAT,"100.0", &conf.slideshow_img_scale},
   {"slideshow_screen_mode",      D_ENUM, "0",     &conf.slideshow_screen_mode},
   {"slideshow_menubar",          D_BOOL, "FALSE", &conf.slideshow_menubar},
   {"slideshow_toolbar",          D_BOOL, "FALSE", &conf.slideshow_toolbar},
   {"slideshow_player",           D_BOOL, "FALSE", &conf.slideshow_player},
   {"slideshow_statusbar",        D_BOOL, "FALSE", &conf.slideshow_statusbar},
   {"slideshow_scrollbar",        D_BOOL, "FALSE", &conf.slideshow_scrollbar},
   {"slideshow_keep_aspcet",      D_BOOL, "TRUE",  &conf.slideshow_keep_aspect},
   {"slideshow_interval",         D_FLOAT,"5.00",  &conf.slideshow_interval},
   {"slideshow_repeat",           D_BOOL, "FALSE", &conf.slideshow_repeat},
   {"slideshow_set_bg",           D_BOOL, "FALSE", &conf.slideshow_set_bg},
   {"slideshow_bg_red",           D_INT,  "0",     &conf.slideshow_bg_color[0]},
   {"slideshow_bg_green",         D_INT,  "0",     &conf.slideshow_bg_color[1]},
   {"slideshow_bg_blue",          D_INT,  "0",     &conf.slideshow_bg_color[2]},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_search [] = {
   {"search_similar_accuracy",         D_FLOAT, "0.97",  &conf.search_similar_accuracy},
   {"search_similar_open_collectiony", D_BOOL,  "TRUE",  &conf.search_similar_open_collection},
   {"simwin_sel_thumbview",            D_BOOL,  "TRUE",  &conf.simwin_sel_thumbview},
   {"simwin_sel_preview",              D_BOOL,  "TRUE",  &conf.simwin_sel_preview},
   {"simwin_sel_new_win",              D_BOOL,  "FALSE", &conf.simwin_sel_new_win},
   {"simwin_sel_shared_win",           D_BOOL,  "FALSE", &conf.simwin_sel_shared_win},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_dnd [] = {
   {"dnd_enable_to_external",   D_BOOL, "TRUE",   &conf.dnd_enable_to_external},
   {"dnd_enable_from_external", D_BOOL, "TRUE",   &conf.dnd_enable_from_external},
   {"dnd_refresh_list_always",  D_BOOL, "FALSE",  &conf.dnd_refresh_list_always},
   {NULL, D_NULL, NULL, NULL}
};


static ConfParam param_wallpaper [] = {
   {"wallpaper_menu", D_STRING,
    "GNOME1,background-properties-capplet;GNOME2,gnome-background-properties;KDE,kcmshell background;KDE(RedHat8),kcmshell kde-background",
    &conf.wallpaper_menu},
   {NULL, D_NULL, NULL, NULL}
};

static ConfParam param_progs [] = {
   {"progs[0]",    D_STRING, "Gimp,gimp-remote -n,FALSE", &conf.progs[0]},
   {"progs[1]",    D_STRING, "XPaint,xpaint,FALSE",       &conf.progs[1]},
   {"progs[2]",    D_STRING, "ElectricEyes,ee,FALSE",     &conf.progs[2]},
   {"progs[3]",    D_STRING, "animate,animate,FALSE",     &conf.progs[3]},
   {"progs[4]",    D_STRING, "enfle,enfle,FALSE",         &conf.progs[4]},
   {"progs[5]",    D_STRING, NULL,                        &conf.progs[5]},
   {"progs[6]",    D_STRING, NULL,                        &conf.progs[6]},
   {"progs[7]",    D_STRING, NULL,                        &conf.progs[7]},
   {"progs[8]",    D_STRING, NULL,                        &conf.progs[8]},
   {"progs[9]",    D_STRING, NULL,                        &conf.progs[9]},
   {"progs[10]",   D_STRING, NULL,                        &conf.progs[10]},
   {"progs[11]",   D_STRING, NULL,                        &conf.progs[11]},
   {"progs[12]",   D_STRING, NULL,                        &conf.progs[12]},
   {"progs[13]",   D_STRING, NULL,                        &conf.progs[13]},
   {"progs[14]",   D_STRING, NULL,                        &conf.progs[14]},
   {"progs[15]",   D_STRING, NULL,                        &conf.progs[15]},
   {"web_browser", D_STRING, "mozilla %s",                &conf.web_browser},
   {"text_viewer", D_STRING, "emacs",                     &conf.text_viewer},
   {"use_internal_text_viewer",  D_BOOL, "TRUE", &conf.text_viewer_use_internal},
   {"scripts_use_default_search_dir_list", D_BOOL,
    "TRUE", &conf.scripts_use_default_search_dir_list},
   {"scripts_search_dir_list", D_STRING,
    SCRIPTS_DEFAULT_SEARCH_DIR_LIST, &conf.scripts_search_dir_list},
   {"scripts_show_dialog",     D_BOOL,   "FALSE",   &conf.scripts_show_dialog},
   {NULL, D_NULL, NULL, NULL}
};


static ConfParam param_plugin [] = {
   {"plugin_use_default_search_dir_list", D_BOOL,
    "TRUE", &conf.plugin_use_default_search_dir_list},
   {"plugin_search_dir_list", D_STRING,
    PLUGIN_DEFAULT_SEARCH_DIR_LIST, &conf.plugin_search_dir_list},

   {NULL, D_NULL, NULL, NULL}
};



/******************
 *  Key Config
 ******************/
ConfParam keyprefs_common[] = {
   {"common_auto_completion1",  D_STRING, "<control>I", &akey.common_auto_completion1},
   {"common_auto_completion2",  D_STRING, NULL,         &akey.common_auto_completion2},
   {"common_popup_menu",        D_STRING, "<shift>F10", &akey.common_popup_menu},
   {NULL, D_NULL, NULL, NULL}
};

ConfParam keyprefs_imgwin[] = {
   {"imgwin_zoomin",              D_STRING, "S",          &akey.imgwin_zoomin},
   {"imgwin_zoomout",             D_STRING, "A",          &akey.imgwin_zoomout},
   {"imgwin_fit_img",             D_STRING, "W",          &akey.imgwin_fit_img},
   {"imgwin_zoom10",              D_STRING, "1",          &akey.imgwin_zoom10},
   {"imgwin_zoom25",              D_STRING, "2",          &akey.imgwin_zoom25},
   {"imgwin_zoom50",              D_STRING, "3",          &akey.imgwin_zoom50},
   {"imgwin_zoom75",              D_STRING, "4",          &akey.imgwin_zoom75},
   {"imgwin_zoom100",             D_STRING, "5",          &akey.imgwin_zoom100},
   {"imgwin_zoom125",             D_STRING, "6",          &akey.imgwin_zoom125},
   {"imgwin_zoom150",             D_STRING, "7",          &akey.imgwin_zoom150},
   {"imgwin_zoom175",             D_STRING, "8",          &akey.imgwin_zoom175},
   {"imgwin_zoom200",             D_STRING, "9",          &akey.imgwin_zoom200},
   {NULL, D_NULL, NULL, NULL}
};

ConfParam keyprefs_thumbwin[] = {
   {"thumbwin_disp_mode[0]",      D_STRING, "<alt>1",     &akey.thumbwin_disp_mode[0]},
   {"thumbwin_disp_mode[1]",      D_STRING, "<alt>2",     &akey.thumbwin_disp_mode[1]},
   {"thumbwin_disp_mode[2]",      D_STRING, "<alt>3",     &akey.thumbwin_disp_mode[2]},
   {"thumbwin_disp_mode[3]",      D_STRING, "<alt>4",     &akey.thumbwin_disp_mode[3]},
   {"thumbwin_disp_mode[4]",      D_STRING, "<alt>5",     &akey.thumbwin_disp_mode[4]},
   {"thumbwin_disp_mode[5]",      D_STRING, "<alt>6",     &akey.thumbwin_disp_mode[5]},
   {"thumbwin_disp_mode[6]",      D_STRING, "<alt>7",     &akey.thumbwin_disp_mode[6]},
   {"thumbwin_disp_mode[7]",      D_STRING, "<alt>8",     &akey.thumbwin_disp_mode[7]},
   {"thumbwin_disp_mode[8]",      D_STRING, "<alt>9",     &akey.thumbwin_disp_mode[8]},
   {"thumbwin_disp_mode[9]",      D_STRING, "<alt>0",     &akey.thumbwin_disp_mode[9]},
   {"thumbwin_disp_mode[10]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[10]},
   {"thumbwin_disp_mode[11]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[11]},
   {"thumbwin_disp_mode[12]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[12]},
   {"thumbwin_disp_mode[13]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[13]},
   {"thumbwin_disp_mode[14]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[14]},
   {"thumbwin_disp_mode[15]",     D_STRING, NULL,         &akey.thumbwin_disp_mode[15]},

   {NULL, D_NULL, NULL, NULL}
};


static PrefsSection conf_sections [] = {
   {"Common",           param_common},
   {"Startup",          param_startup},
   {"Image View",       param_imageview},
   {"Thumbnail View",   param_thumbview},
   {"Thumbnail Album",  param_thumbalbum},
   {"Directory View",   param_dirview},
   {"Preview",          param_preview},
   {"Cache",            param_cache},
   {"Comment",          param_comment},
   {"Slide Show",       param_slideshow},
   {"Search",           param_search},
   {"Drag and Drop",    param_dnd},
   {"Wallpaper",        param_wallpaper},
   {"External Program", param_progs},
   {"Plugin",           param_plugin},
   {NULL, NULL},
};


static PrefsSection keyconf_sections [] = {
   {"Common",           keyprefs_common},
   {"Image Window",     keyprefs_imgwin},
   {"Thumbnail Window", keyprefs_thumbwin},
   {NULL, NULL},
};


static PrefsFile conf_files [] = {
   {GIMV_RC,         conf_sections},
   {GIMV_KEYCONF_RC, keyconf_sections},
   {NULL, NULL},
};


/*
 *  store_config:
 *     @ Convert config data string to eache data type and store to memory.
 *
 *  data   : Pointer to memory chunk to store data.
 *  val    : Data value string.
 *  type   : Data type to convert.
 */
static void
store_config (gpointer data, gchar *val, DataType type)
{
   switch (type) {
   case D_STRING:
      g_free (*((gchar **)data));
      if (!val)
         *((gchar **) data) = NULL;
      else
         *((gchar **) data) = g_strdup(val);
      break;
   case D_INT:
      if (!val) break;
      *((gint *) data) =  atoi(val);
      break;
   case D_FLOAT:
      if (!val) break;
      *((gfloat *) data) =  atof(val);
      break;
   case D_BOOL:
      if (!val) break;
      if (!g_strcasecmp (val, "TRUE"))
         *((gboolean *) data) = TRUE;
      else
         *((gboolean *) data) = FALSE;
      break;
   case D_ENUM:
      if (!val) break;
      *((gint *) data) =  atoi(val);
      break;
   default:
      break;
   }
}


/*
 *  prefs_load_config_default:
 *     @ Load default config.
 */
static void
prefs_load_config_default (PrefsSection *sections)
{
   gint i, j;

   for (j = 0; sections[j].section_name; j++) {
      ConfParam *param = sections[j].param;
      for (i = 0; param[i].keyname; i++)
         store_config (param[i].data, param[i].default_val, param[i].data_type);
   }
   return;
}


/*
 *  prefs_load_rc:
 *     @ Load config from disk.
 */
static void
prefs_load_rc (gchar *filename, PrefsSection *sections)
{
   gchar rcfile[MAX_PATH_LEN];
   FILE *gimvrc;
   gchar buf[BUF_SIZE];
   gchar **pair;
   gint i, j;

   prefs_load_config_default (sections);

   g_snprintf (rcfile, MAX_PATH_LEN, "%s/%s/%s",
               getenv("HOME"), GIMV_RC_DIR, filename);

   gimvrc = fopen(rcfile, "r");
   if (!gimvrc) {
      gchar *tmpstr;

      g_warning (_("Can't open rc file: %s\n"
                   "Use default setting ..."), rcfile);

      tmpstr = conf.scripts_search_dir_list;
      conf.scripts_search_dir_list
         = g_strconcat (conf.scripts_search_dir_list,
                        ",", getenv("HOME"), "/",  GIMV_RC_DIR, "/scripts",
                        NULL);
      g_free (tmpstr);

      return;
   }

   while (fgets (buf, sizeof(buf), gimvrc)) {
      if (buf[0] == '[' || buf[0] == '\n')
         continue;

      g_strstrip (buf);

      pair = g_strsplit (buf, "=", 2);
      if (pair[0]) g_strstrip(pair[0]);
      if (pair[1]) g_strstrip(pair[1]);

      for (j = 0; sections[j].section_name; j++) {
         ConfParam *param = sections[j].param;
         for (i = 0; param[i].keyname; i++) {
            if (!g_strcasecmp (param[i].keyname, pair[0])) {
               store_config (param[i].data, pair[1], param[i].data_type);
               break;
            }
         }
      }
      g_strfreev (pair);
   }

   fclose (gimvrc);
}


void
prefs_load_config (void)
{
   gint i;

   for (i = 0; conf_files[i].filename; i++)
      prefs_load_rc (conf_files[i].filename, conf_files[i].sections);

   gimv_plugin_prefs_read_files ();
}


/*
 *  prefs_save_rc:
 *     @ Save config to disk.
 */
static void
prefs_save_rc (gchar *filename, PrefsSection *sections)
{
   gchar dir[MAX_PATH_LEN], rcfile[MAX_PATH_LEN];
   FILE *gimvrc;
   gint i, j;
   gchar *bool;
   struct stat st;

   g_snprintf (dir, MAX_PATH_LEN, "%s/%s", getenv("HOME"), GIMV_RC_DIR);

   if (stat(dir, &st)) {
      mkdir(dir, S_IRWXU);
      g_warning (_("Directory \"%s\" not found. Created it ..."), dir);
   } else {
      if (!S_ISDIR(st.st_mode)) {
         g_warning (_("\"%s\" found, but it's not directory. Abort creating ..."), dir);
         return;
      }
   }

   g_snprintf (rcfile, MAX_PATH_LEN, "%s/%s/%s", getenv("HOME"),
               GIMV_RC_DIR, filename);
   gimvrc = fopen (rcfile, "w");
   if (!gimvrc) {
      g_warning (_("Can't open rc file for write."));
      return;
   }

   for (j = 0; sections[j].section_name; j++) {
      ConfParam *param = sections[j].param;
      fprintf (gimvrc, "[%s]\n", sections[j].section_name);
      for (i = 0; param[i].keyname; i++) {
         switch (param[i].data_type) {
         case D_STRING:
            if (!*(gchar **) param[i].data)
               fprintf (gimvrc, "%s=\n", param[i].keyname);
            else
               fprintf (gimvrc, "%s=%s\n", param[i].keyname,
                        *(gchar **) param[i].data);
            break;
         case D_INT:
            fprintf (gimvrc, "%s=%d\n", param[i].keyname,
                     *((gint *) param[i].data));
            break;
         case D_FLOAT:
            fprintf (gimvrc, "%s=%f\n", param[i].keyname,
                     *((gfloat *) param[i].data));
            break;
         case D_BOOL:
            if (*((gboolean *)param[i].data))
               bool = "TRUE";
            else
               bool = "FALSE";
            fprintf (gimvrc, "%s=%s\n", param[i].keyname, bool);
            break;
         case D_ENUM:
            fprintf (gimvrc, "%s=%d\n", param[i].keyname,
                     *((gint *) param[i].data));
            break;
         default:
            break;
         }
      }
      fprintf (gimvrc, "\n");
   }

   fclose (gimvrc);
}


void
prefs_save_config (void)
{
   gint i;

   for (i = 0; conf_files[i].filename; i++)
      prefs_save_rc (conf_files[i].filename, conf_files[i].sections);

   gimv_plugin_prefs_write_files ();
}


gint
prefs_mouse_get_num (gint button_id, gint mod_id, const gchar *string)
{
   gint i, j, num = 0;
   gchar **buttons, **mods;

   g_return_val_if_fail (button_id >= 0 && button_id < 6, 0);
   g_return_val_if_fail (mod_id >= 0 && mod_id < 4, 0);
   g_return_val_if_fail (string, 0);

   buttons = g_strsplit (string, ";", 6);
   if (!buttons) return 0;

   for (i = 0; i < 6; i++) {
      mods = g_strsplit (buttons[i], ",", 4);
      if (!mods) continue;
      for (j = 0; j < 4; j++) {
         if (i == button_id && j == mod_id) {
            g_strstrip (mods[j]);
            num = atoi (mods[j]);
            break;
         }
      }
      g_strfreev (mods);
   }

   g_strfreev (buttons);

   return num;
}


gint
prefs_mouse_get_num_from_event (GdkEventButton *event, const gchar *string)
{
   gint bid = 1, mid = 0;
   g_return_val_if_fail (event, 0);

   /* get button id */
   if(event->type == GDK_2BUTTON_PRESS && event->button == 1)
      bid = 0;
   else
      bid = event->button;
   if (bid < 0 || bid > 5)
      return 0;

   /* get modifier id */
   if (event->state & GDK_SHIFT_MASK)
      mid = 1;
   else if (event->state & GDK_CONTROL_MASK)
      mid = 2;
   else if (event->state & GDK_MOD1_MASK)
      mid = 3;
   else
      mid = 0;

   return prefs_mouse_get_num (bid, mid, string);
}
