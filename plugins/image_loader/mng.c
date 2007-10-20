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

#include "mng.h"

#ifdef ENABLE_MNG

#include <libmng.h>

/* avoid libjpeg's bug */
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#include "gimv_plugin.h"

typedef struct MNGAnim_Tag
{
   GimvImage  *image;
   GimvIO     *file;
   gchar      *filename;
   mng_handle  MNG_handle;
   mng_ptr     canvas;
   mng_uint32  width;
   mng_uint32  height;
   gint        bytes_per_pixel;
   mng_uint32  delay;
   GTimer     *timer;
} MNGAnim;


/* GimvAnim methos */
static gint     vfmng_iterate         (GimvAnim *anim);
static gint     vfmng_get_interval    (GimvAnim *anim);
static void     vfmng_delete          (GimvAnim *anim);

/* libmng callback */
static mng_bool mymng_error           (mng_handle  mng,
                                       mng_int32   code,
                                       mng_int8    severity,
                                       mng_chunkid chunktype,
                                       mng_uint32  chunkseq,
                                       mng_int32   extra1,
                                       mng_int32   extra2,
                                       mng_pchar   text);
static mng_ptr  mymng_malloc_callback (mng_size_t  how_many);
static void     mymng_free_callback   (mng_ptr     pointer,
                                       mng_size_t  number);
static mng_bool mymng_open_stream     (mng_handle  mng);
static mng_bool mymng_close_stream    (mng_handle  mng);
static mng_bool mymng_read_stream     (mng_handle  mng,
                                       mng_ptr     buffer,
                                       mng_uint32  size,
                                       mng_uint32 *bytesread);
static mng_uint32 mymng_get_ticks     (mng_handle mng);
static mng_bool mymng_set_timer       (mng_handle  mng,
                                       mng_uint32  msecs);
static mng_bool mymng_process_header  (mng_handle  mng,
                                       mng_uint32  width,
                                       mng_uint32  height);
static mng_ptr  mymng_get_canvas_line (mng_handle  mng,
                                       mng_uint32  line);
static mng_bool mymng_refresh         (mng_handle  mng,
                                       mng_uint32  x,
                                       mng_uint32  y,
                                       mng_uint32  w,
                                       mng_uint32  h);


static GimvImageLoaderPlugin gimv_mng_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "MNG",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        mng_load,
   }
};

static const gchar *jng_extensions[] =
{
   "jng",
};

static const gchar *mng_extensions[] =
{
   "mng",
};

static GimvMimeTypeEntry mng_mime_types[] =
{
   {
      mime_type:      "image/x-jng",
      description:    "JNG Image",
      extensions:     jng_extensions,
      extensions_len: sizeof (jng_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "video/x-mng",
      description:    "MNG Image",
      extensions:     mng_extensions,
      extensions_len: sizeof (mng_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_mng_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(mng_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("MNG Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


static GimvAnimFuncTable mng_vf_table = {
   get_length   : NULL,
   get_idx      : NULL,
   get_interval : vfmng_get_interval,
   iterate      : vfmng_iterate,
   seek         : NULL,
   delete       : vfmng_delete,
};


static gboolean
mng_check_type (const gchar *filename)
{
   GimvIO *gio;
   guchar buf[256];
   GimvIOStatus status;
   guint bytes;

   gio = gimv_io_new (filename, "rb");
   if (!gio) return FALSE;

   status = gimv_io_read (gio, buf, 8, &bytes);
   if (bytes != 8)
      goto ERROR;

   if (!(buf[0] == 0x8a && buf[1] == 'M' && buf[2] == 'N' && buf[3] == 'G') &&
       !(buf[0] == 0x8b && buf[1] == 'J' && buf[2] == 'N' && buf[3] == 'G'))
   {
      goto ERROR;
   }

   if (buf[4] != 0x0d ||
       buf[5] != 0x0a ||
       buf[6] != 0x1a ||
       buf[7] != 0x0a)
   {
      /* g_warning ("not mng format"); */
      goto ERROR;
   }

   gimv_io_close (gio);
   return TRUE;

ERROR:
   gimv_io_close (gio);
   return FALSE;
}


static MNGAnim *
mng_anim_new (const gchar *filename, GimvImage *image)
{
   MNGAnim *anim = g_new0 (MNGAnim, 1);
   mng_retcode retval;

   g_return_val_if_fail (filename && *filename, NULL);
   g_return_val_if_fail (image, NULL);

   anim->image = image;
   anim->file = NULL;
   anim->filename = g_strdup (filename);
   anim->MNG_handle =  mng_initialize (image,
                                       mymng_malloc_callback,
                                       mymng_free_callback,
                                       MNG_NULL);
   anim->canvas = NULL;
   anim->delay  = 0;
   anim->width  = 0;
   anim->height = 0;
   anim->bytes_per_pixel = 3;

   retval = mng_setcb_errorproc (anim->MNG_handle, mymng_error);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_openstream (anim->MNG_handle, mymng_open_stream);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_closestream (anim->MNG_handle, mymng_close_stream);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_readdata (anim->MNG_handle, mymng_read_stream);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_gettickcount (anim->MNG_handle, mymng_get_ticks);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_settimer (anim->MNG_handle, mymng_set_timer);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_processheader (anim->MNG_handle, mymng_process_header);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_getcanvasline (anim->MNG_handle, mymng_get_canvas_line);
   if (retval != MNG_NOERROR) goto ERROR;

   retval = mng_setcb_refresh (anim->MNG_handle, mymng_refresh);
   if (retval != MNG_NOERROR) goto ERROR;

   return anim;

ERROR:
   g_free (anim);
   return NULL;
}


static void
mng_anim_delete (MNGAnim *anim)
{
   g_return_if_fail (anim);

   if (anim->file)
      gimv_io_close (anim->file);
   anim->file = NULL;

   if (anim->filename)
      g_free (anim->filename);
   anim->filename = NULL;

   if (anim->canvas)
      g_free (anim->canvas);
   anim->canvas = NULL;

   mng_cleanup (&anim->MNG_handle);

   if (anim->timer)
      g_timer_destroy (anim->timer);
   anim->timer = NULL;

   g_free (anim);
}


/* FIXME */
GimvImage *
mng_load (GimvImageLoader *loader, gpointer data)
{
   const gchar *filename;
   GimvAnim *anim;
   GimvImage *image;
   MNGAnim *mng_anim;

   g_return_val_if_fail (loader, NULL);

   filename = gimv_image_loader_get_path (loader);
   if (!filename || !*filename) return NULL;

   if (!mng_check_type (filename)) return NULL;

   anim = gimv_anim_new ();
   image = (GimvImage *) anim;

   mng_anim = mng_anim_new (filename, image);
   if (!mng_anim) {
      g_object_unref (G_OBJECT (image));
      return NULL;
   }

   image = (GimvImage *) anim;
   anim->anim = mng_anim;
   anim->table = &mng_vf_table;

   mng_readdisplay (mng_anim->MNG_handle);

   if (!anim->anim || !image->image) {
      g_object_unref (G_OBJECT (image));
      return NULL;
   }

   anim->current_frame_idx++;

   return image;
}



/****************************************************************************
 *
 *  GimvAnim methods
 *
 ****************************************************************************/
static gint
vfmng_iterate (GimvAnim *anim)
{
   MNGAnim *mng_anim;
   gint ret;

   g_return_val_if_fail (anim, -1);

   mng_anim = anim->anim;

   mng_anim->delay = 0;
   ret = mng_display_resume (mng_anim->MNG_handle);

   if (!ret) {
      return anim->current_frame_idx;
   }

   return ++anim->current_frame_idx;
}


static gint
vfmng_get_interval (GimvAnim *anim)
{
   MNGAnim *mng_anim;

   g_return_val_if_fail (anim, -1);

   mng_anim = anim->anim;

   return mng_anim->delay;
}


static void
vfmng_delete (GimvAnim *anim)
{
   MNGAnim *mng_anim;

   g_return_if_fail (anim);

   mng_anim = anim->anim;

   mng_anim_delete (mng_anim);
   anim->anim = NULL;
}



/****************************************************************************
 *
 *  libmng callbacks
 *
 ****************************************************************************/
static mng_bool
mymng_error (mng_handle mng,
             mng_int32 code,
             mng_int8 severity,
             mng_chunkid chunktype,
             mng_uint32 chunkseq,
             mng_int32 extra1,
             mng_int32 extra2,
             mng_pchar text)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;
   char		chunk[5];

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   anim = mng_get_userdata (mng);

   /* pull out the chuck type as a string */
   chunk[0] = (char)((chunktype >> 24) & 0xFF);
   chunk[1] = (char)((chunktype >> 16) & 0xFF);
   chunk[2] = (char)((chunktype >>  8) & 0xFF);
   chunk[3] = (char)((chunktype      ) & 0xFF);
   chunk[4] = '\0';

   /* output the error */
   fprintf (stderr, "error playing '%s' chunk %s (%d):\n",
            mng_anim->filename, chunk, chunkseq);
   fprintf (stderr, "%s\n", text);

   return 0;
}


static mng_ptr
mymng_malloc_callback (mng_size_t how_many)
{
   return (mng_ptr) g_new0 (guchar, how_many);
}


static void
mymng_free_callback (mng_ptr pointer, mng_size_t number)
{
   g_free (pointer);
}


static mng_bool
mymng_open_stream(mng_handle mng)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   /* open the file */
   mng_anim->file = gimv_io_new (mng_anim->filename, "rb");
   if (!mng_anim->file) {
      g_print ("unable to open '%s'\n", mng_anim->filename);
      return MNG_FALSE;
   }

   return MNG_TRUE;
}


static mng_bool
mymng_close_stream (mng_handle mng)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   /* close the file */
   if (mng_anim->file)
      gimv_io_close (mng_anim->file);
   mng_anim->file = NULL;

   return MNG_TRUE;
}


static mng_bool
mymng_read_stream(mng_handle  mng,
                  mng_ptr     buffer,
                  mng_uint32  size,
                  mng_uint32 *bytesread)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   /* read the requested amount of data from the file */
   gimv_io_read (mng_anim->file, buffer, size, bytesread);

   return MNG_TRUE;
}


static mng_uint32
mymng_get_ticks (mng_handle mng)
{
   gdouble seconds;
   gulong microseconds;
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   if (mng_anim->timer)
      seconds = g_timer_elapsed (mng_anim->timer,
                                 &microseconds);
   else
      return 0;

   return ((mng_uint32) (seconds * 1000.0 + ((gdouble) microseconds) / 1000.0));
}

static mng_bool
mymng_set_timer (mng_handle mng, mng_uint32 msecs)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   mng_anim->delay = msecs;

   return MNG_TRUE;
}


static mng_bool
mymng_process_header (mng_handle mng,
                      mng_uint32 width,
                      mng_uint32 height)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   if (mng_anim->canvas)
      g_free (mng_anim->canvas);

   mng_anim->canvas
      = g_new0 (guchar, width * height * mng_anim->bytes_per_pixel);

   mng_anim->width = width;
   mng_anim->height = height;
   mng_set_canvasstyle (mng, MNG_CANVAS_RGB8);   /* FIXME */
   mng_set_bgcolor (mng, 65535, 65535, 65535);

   if (mng_anim->timer)
      g_timer_destroy (mng_anim->timer);
   mng_anim->timer = g_timer_new ();
   g_timer_start (mng_anim->timer);

   return MNG_TRUE;
}


static mng_ptr
mymng_get_canvas_line (mng_handle mng, mng_uint32 line)
{
   GimvAnim *anim;
   MNGAnim *mng_anim;
   mng_ptr row;

   anim = mng_get_userdata (mng);
   mng_anim = anim->anim;

   if (!mng_anim->canvas)
      return NULL;

   row = ((char *)mng_anim->canvas)
            + mng_anim->bytes_per_pixel * mng_anim->width * line;

   return row;	
}


mng_bool
mymng_refresh (mng_handle mng,
               mng_uint32 x, mng_uint32 y,
               mng_uint32 w, mng_uint32 h)
{
   GimvAnim *anim;
   GimvImage *image;
   MNGAnim *mng_anim;
   gint bytes;
   GimvImageAngle angle;

   anim = mng_get_userdata (mng);
   image = (GimvImage *) anim;
   mng_anim = anim->anim;

   if (!mng_anim->canvas)
      return MNG_FALSE;

   bytes = sizeof (guchar)
      * mng_anim->width * mng_anim->height
      * mng_anim->bytes_per_pixel;
   gimv_anim_update_frame (anim, g_memdup (mng_anim->canvas, bytes),
                           mng_anim->width, mng_anim->height, FALSE);

   /* restore angle */
   angle = image->angle;
   image->angle = 0;
   gimv_image_rotate (image, angle);

   return MNG_TRUE;
}


#endif /* ENABLE_MNG */
