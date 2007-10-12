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

#ifndef __GIMV_IMAGE_VIEW_H__
#define __GIMV_IMAGE_VIEW_H__

#include "gimageview.h"

#include "gimv_image.h"
#include "gimv_image_info.h"
#include "gimv_image_loader.h"

#define GIMV_TYPE_IMAGE_VIEW            (gimv_image_view_get_type ())
#define GIMV_IMAGE_VIEW(obj)            (GTK_CHECK_CAST (obj, gimv_image_view_get_type (), GimvImageView))
#define GIMV_IMAGE_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_image_view_get_type, GimvImageViewClass))
#define GIMV_IS_IMAGE_VIEW(obj)         (GTK_CHECK_TYPE (obj, gimv_image_view_get_type ()))
#define GIMV_IS_IMAGE_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_IMAGE_VIEW))


#define GIMV_IMAGE_VIEW_DEFAULT_VIEW_MODE N_("Default Image Viewer")

#define GIMV_IMAGE_VIEW_MIN_SCALE 5
#define GIMV_IMAGE_VIEW_MAX_SCALE 100000
#define GIMV_IMAGE_VIEW_SCALE_STEP 5


typedef enum
{
   GIMV_IMAGE_VIEW_ZOOM_100,  /* Real Size */
   GIMV_IMAGE_VIEW_ZOOM_IN,   /* Increase Image Size 5% */
   GIMV_IMAGE_VIEW_ZOOM_OUT,  /* Decrease Image Size 5% */
   GIMV_IMAGE_VIEW_ZOOM_FIT,  /* Fit to Widnow */
   GIMV_IMAGE_VIEW_ZOOM_FIT_ZOOM_OUT_ONLY,
   GIMV_IMAGE_VIEW_ZOOM_FIT_WIDTH,
   GIMV_IMAGE_VIEW_ZOOM_FIT_HEIGHT,
   GIMV_IMAGE_VIEW_ZOOM_10,
   GIMV_IMAGE_VIEW_ZOOM_25,
   GIMV_IMAGE_VIEW_ZOOM_50,
   GIMV_IMAGE_VIEW_ZOOM_75,
   GIMV_IMAGE_VIEW_ZOOM_125,
   GIMV_IMAGE_VIEW_ZOOM_150,
   GIMV_IMAGE_VIEW_ZOOM_175,
   GIMV_IMAGE_VIEW_ZOOM_200,
   GIMV_IMAGE_VIEW_ZOOM_300,
   GIMV_IMAGE_VIEW_ZOOM_FREE
} GimvImageViewZoomType;


typedef enum
{
   GIMV_IMAGE_VIEW_ROTATE_0,
   GIMV_IMAGE_VIEW_ROTATE_90,
   GIMV_IMAGE_VIEW_ROTATE_180,
   GIMV_IMAGE_VIEW_ROTATE_270
} GimvImageViewOrientation;


typedef enum
{
   GimvImageViewPlayableDisable = -1,
   GimvImageViewPlayableStop,
   GimvImageViewPlayablePlay,
   GimvImageViewPlayablePause,
   GimvImageViewPlayableForward,
   GimvImageViewPlayableReverse,
} GimvImageViewPlayableStatus;


typedef enum
{
   GimvImageViewPlayerVisibleHide,
   GimvImageViewPlayerVisibleShow,
   GimvImageViewPlayerVisibleAuto
} GimvImageViewPlayerVisibleType;


typedef struct GimvImageViewClass_Tag        GimvImageViewClass;
typedef struct GimvImageViewPrivate_Tag      GimvImageViewPrivate;
typedef struct GimvImageViewPlugin_Tag       GimvImageViewPlugin;
typedef struct GimvImageViewScalableIF_Tag   GimvImageViewScalableIF;
typedef struct GimvImageViewRotatableIF_Tag  GimvImageViewRotatableIF;
typedef struct GimvImageViewScrollableIF_Tag GimvImageViewScrollableIF;
typedef struct GimvImageViewPlayableIF_Tag   GimvImageViewPlayableIF;


typedef GList *(*GimvImageViewNextFn)     (GimvImageView *iv,
                                           gpointer   list_owner,
                                           GList     *list,
                                           gpointer   data);
typedef GList *(*GimvImageViewPrevFn)     (GimvImageView *iv,
                                           gpointer   list_owner,
                                           GList     *list,
                                           gpointer   data);
typedef GList *(*GimvImageViewNthFn)      (GimvImageView *iv,
                                           gpointer   list_owner,
                                           GList     *list,
                                           guint      nth,
                                           gpointer   data);
typedef void (*GimvImageViewRemoveListFn) (GimvImageView *iv,
                                           gpointer list_owner,
                                           gpointer data);

struct GimvImageView_Tag
{
   GtkVBox          parent;

   GtkWidget       *draw_area;
   GimvImageViewPlugin *draw_area_funcs;

   /* other widgets */
   GtkWidget       *table;
   GtkWidget       *hscrollbar;
   GtkWidget       *vscrollbar;
   GtkWidget       *nav_button;
   GtkWidget       *imageview_popup;
   GtkWidget       *zoom_menu;
   GtkWidget       *rotate_menu;
   GtkWidget       *movie_menu;
   GtkWidget       *view_modes_menu;
   GdkCursor       *cursor;
   GtkAdjustment   *hadj, *vadj;

   GtkWidget       *progressbar;

   GtkWidget       *player_container;
   GtkWidget       *player_toolbar;

   struct    /* buttons in player */
   {
      GtkWidget *rw;
      GtkWidget *play;
      GtkWidget *stop;
      GtkWidget *fw;
      GtkWidget *eject;
      GtkWidget *seekbar;
      GtkWidget *play_icon;
   } player;

   /* information about image */
   GimvImageInfo   *info;

   /* image */
   GimvImageLoader *loader;
   GimvImage       *image;

   /* imageview status */
   GdkColor        *bg_color;

   GimvImageViewPrivate *priv;
};


struct GimvImageViewClass_Tag
{
   GtkVBoxClass parent_class;

   /* signals */
   void     (*image_changed)     (GimvImageView *iv);
   void     (*load_start)        (GimvImageView *iv,
                                  GimvImageInfo *info);
   void     (*load_end)          (GimvImageView *iv,
                                  GimvImageInfo *info,
                                  gboolean       cancel);
   void     (*set_list)          (GimvImageView *iv);
   void     (*unset_list)        (GimvImageView *iv);
   void     (*rendered)          (GimvImageView *iv);
   void     (*toggle_aspect)     (GimvImageView *iv,
                                  gboolean       keep_aspect);
   void     (*toggle_buffer)     (GimvImageView *iv,
                                  gboolean       buffered);
   void     (*thumbnail_created) (GimvImageView *iv,
                                  GimvImageInfo *info);

   gboolean (*image_pressed)     (GimvImageView  *iv,
                                  GdkEventButton *button);
   gboolean (*image_released)    (GimvImageView  *iv,
                                  GdkEventButton *button);
   gboolean (*image_clicked)     (GimvImageView  *iv,
                                  GdkEventButton *button);
};


GList     *gimv_image_view_get_list               (void);
GtkType    gimv_image_view_get_type               (void);
GtkWidget *gimv_image_view_new                    (GimvImageInfo  *info);

void       gimv_image_view_change_image           (GimvImageView  *iv,
                                                   GimvImageInfo  *info);
void       gimv_image_view_change_image_info      (GimvImageView  *iv,
                                                   GimvImageInfo  *info);
void       gimv_image_view_draw_image             (GimvImageView  *iv);
void       gimv_image_view_show_image             (GimvImageView  *iv);

void       gimv_image_view_create_thumbnail       (GimvImageView  *iv);

/* creating/show/hide child widgets */
void       gimv_image_view_change_view_mode       (GimvImageView  *iv,
                                                   const gchar    *label);
GtkWidget *gimv_image_view_create_zoom_menu       (GtkWidget      *window,
                                                   GimvImageView  *iv,
                                                   const gchar    *path);
GtkWidget *gimv_image_view_create_rotate_menu     (GtkWidget      *window,
                                                   GimvImageView  *iv,
                                                   const gchar    *path);
GtkWidget *gimv_image_view_create_popup_menu      (GtkWidget      *window,
                                                   GimvImageView  *iv,
                                                   const gchar    *path);
GtkWidget *gimv_image_view_create_movie_menu      (GtkWidget      *window,
                                                   GimvImageView  *iv,
                                                   const gchar    *path);
GtkWidget *gimv_image_view_create_view_modes_menu (GtkWidget      *window,
                                                   GimvImageView  *iv,
                                                   const gchar    *path);
void       gimv_image_view_set_progressbar        (GimvImageView  *iv,
                                                   GtkWidget      *progressbar);
void       gimv_image_view_set_player_visible     (GimvImageView  *iv,
                                                   GimvImageViewPlayerVisibleType type);
GimvImageViewPlayerVisibleType
           gimv_image_view_get_player_visible     (GimvImageView  *iv);
void       gimv_image_view_show_scrollbar         (GimvImageView  *iv);
void       gimv_image_view_hide_scrollbar         (GimvImageView  *iv);
void       gimv_image_view_popup_menu             (GimvImageView  *iv,
                                                   GdkEventButton *event);
void       gimv_image_view_set_bg_color           (GimvImageView  *iv,
                                                   gint            red,
                                                   gint            green,
                                                   gint            blue);
void       gimv_image_view_open_navwin            (GimvImageView  *iv,
                                                   gint            x_root,
                                                   gint            y_root);
void       gimv_image_view_set_fullscreen         (GimvImageView  *iv,
                                                   GtkWindow      *fullscreen);
void       gimv_image_view_unset_fullscreen       (GimvImageView  *iv);

/* loading */
void       gimv_image_view_free_image_buf         (GimvImageView  *iv);
void       gimv_image_view_load_image_buf         (GimvImageView  *iv);
gboolean   gimv_image_view_is_loading             (GimvImageView  *iv);
void       gimv_image_view_cancel_loading         (GimvImageView  *iv);

/* scalable interface */
void       gimv_image_view_zoom_image             (GimvImageView  *iv,
                                                   GimvImageViewZoomType zoom,
                                                   gfloat x_scale,
                                                   gfloat y_scale);
gboolean   gimv_image_view_get_image_size         (GimvImageView  *iv,
                                                   gint           *width,
                                                   gint           *height);

/* rotatable interface */
void       gimv_image_view_rotate_image           (GimvImageView  *iv,
                                                   GimvImageViewOrientation angle);
void       gimv_image_view_rotate_ccw             (GimvImageView  *iv);
void       gimv_image_view_rotate_cw              (GimvImageView  *iv);
GimvImageViewOrientation
           gimv_image_view_get_orientation        (GimvImageView  *iv);

/* scrollable interface */
/*
 * +----------->
 * | (0, 0)                               (width, 0)
 * |  +------------------------------------+
 * |  |                                    |
 * |  |         Image                      |
 * v  |                                    |
 *    |   (x, y)               (x + fwidth, y)
 *    |     +-------------------+          |
 *    |     |                   |          |
 *    |     |                   |          |
 *    |     |    View Port      |          |
 *    |     |                   |          |
 *    |     |                   |          |
 *    |     +-------------------+          |
 *    |   (x, y + fheight)     (x + fwidth, y + fheight)
 *    |                                    |
 *    +------------------------------------+
 *   (0, height)                          (width, height)
 */
void       gimv_image_view_get_image_frame_size   (GimvImageView  *iv,
                                                   gint           *fwidth,
                                                   gint           *fheight);
gboolean   gimv_image_view_get_view_position      (GimvImageView  *iv,
                                                   gint           *x,
                                                   gint           *y);
void       gimv_image_view_moveto                 (GimvImageView  *iv,
                                                   gint            x,
                                                   gint            y);
#define    gimv_image_view_set_view_position(iv, x, y) \
           gimv_image_view_moveto (iv, x, y)
void       gimv_image_view_reset_scrollbar        (GimvImageView  *iv);

/* playable interface */
gboolean   gimv_image_view_is_playable            (GimvImageView  *iv);
void       gimv_image_view_playable_play          (GimvImageView  *iv);
void       gimv_image_view_playable_stop          (GimvImageView  *iv);
void       gimv_image_view_playable_pause         (GimvImageView  *iv);
void       gimv_image_view_playable_forward       (GimvImageView  *iv);
void       gimv_image_view_playable_reverse       (GimvImageView  *iv);
void       gimv_image_view_playable_seek          (GimvImageView  *iv,
                                                   guint           pos);
void       gimv_image_view_playable_eject         (GimvImageView  *iv);
GimvImageViewPlayableStatus
           gimv_image_view_playable_get_status    (GimvImageView  *iv);
guint      gimv_image_view_playable_get_length    (GimvImageView  *iv);
guint      gimv_image_view_playable_get_position  (GimvImageView  *iv);

/* list interface */
void       gimv_image_view_set_list               (GimvImageView  *iv,
                                                   GList          *list,
                                                   GList          *current,
                                                   gpointer        list_owner,
                                                   GimvImageViewNextFn  next_fn,
                                                   GimvImageViewPrevFn  prev_fn,
                                                   GimvImageViewNthFn   nth_fn,
                                                   GimvImageViewRemoveListFn remove_list_fn,
                                                   gpointer        user_data);
void       gimv_image_view_remove_list            (GimvImageView  *iv,
                                                   gpointer        list_owner);
void       gimv_image_view_set_list_self          (GimvImageView  *iv,
                                                   GList          *list,
                                                   GList          *current);
gboolean   gimv_image_view_has_list               (GimvImageView  *iv);
gint       gimv_image_view_image_list_length      (GimvImageView  *iv);
gint       gimv_image_view_image_list_position    (GimvImageView  *iv);
GList     *gimv_image_view_image_list_current     (GimvImageView  *iv);
void       gimv_image_view_next                   (GimvImageView  *iv);
void       gimv_image_view_prev                   (GimvImageView  *iv);
void       gimv_image_view_nth                    (GimvImageView  *iv,
                                                   guint           nth);


/****************************************************************************
 *
 *  GimvImageView Embeder Plugin interface
 *
 ****************************************************************************/
#define GIMV_IMAGE_VIEW_IF_VERSION 5

struct GimvImageViewPlugin_Tag
{
   const guint32 if_version; /* plugin interface version */

   const gchar * const label;

   gint priority_hint;

   gboolean   (*is_supported_fn)     (GimvImageView *iv,
                                      GimvImageInfo *info);;
   GtkWidget *(*create_fn)           (GimvImageView *iv);
   void       (*create_thumbnail_fn) (GimvImageView *iv,
                                      const gchar   *type);
   void       (*fullscreen_fn)       (GimvImageView *iv);

   GimvImageViewScalableIF     *scalable;
   GimvImageViewRotatableIF    *rotatable;
   GimvImageViewScrollableIF   *scrollable;
   GimvImageViewPlayableIF     *playable;
};


struct GimvImageViewPlayableIF_Tag {
   gboolean (*is_playable_fn)  (GimvImageView *iv,
                                GimvImageInfo *info);
   gboolean (*is_seekable_fn)  (GimvImageView *iv);
   void     (*play_fn)         (GimvImageView *iv);
   void     (*stop_fn)         (GimvImageView *iv);
   void     (*pause_fn)        (GimvImageView *iv);
   void     (*forward_fn)      (GimvImageView *iv);
   void     (*reverse_fn)      (GimvImageView *iv);
   void     (*seek_fn)         (GimvImageView *iv,
                                gfloat         pos); /* [%] */
   void     (*eject_fn)        (GimvImageView *iv);
   GimvImageViewPlayableStatus
            (*get_status_fn)   (GimvImageView *iv);
   guint    (*get_length_fn)   (GimvImageView *iv);
   guint    (*get_position_fn) (GimvImageView *iv);
};


GList *gimv_image_view_plugin_get_list (void);


/* for internal use */
void gimv_image_view_playable_set_status   (GimvImageView *iv,
                                            GimvImageViewPlayableStatus status);
void gimv_image_view_playable_set_position (GimvImageView *iv,
                                            gfloat     pos); /* [%] */

/* for plugin loader */
gboolean gimv_image_view_plugin_regist (const gchar *plugin_name,
                                        const gchar *module_name,
                                        gpointer     impl,
                                        gint         size);

#endif /* __GIMV_IMAGE_VIEW_H__ */
