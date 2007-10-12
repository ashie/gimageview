/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * Copyright (C) 2001-2002 the xine project
 * Copyright (C) 2002 Takuro Ashie
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
 *
 * the xine engine in a widget - implementation
 */

#include "gimv_xine_priv.h"

#ifdef ENABLE_XINE

#include "gimv_xine.h"


static xine_t *priv_xine = NULL;
static guint  *priv_xine_ref_count = 0;
static char    configfile[256];


GimvXinePrivate *
gimv_xine_private_new (void)
{
   GimvXinePrivate *priv;

   priv = g_new0 (GimvXinePrivate, 1);

   /*
    * create a new xine instance, load config values
    */

   if (priv_xine_ref_count == 0) {
      priv->xine = priv_xine = xine_new ();
      g_snprintf (configfile, 255, "%s/.gimv/xinerc", getenv ("HOME"));
      priv->configfile = g_strdup (configfile);
      xine_config_load (priv->xine, configfile);
      xine_init (priv->xine);
      priv_xine_ref_count++;
   } else {
      priv->xine = priv_xine;
      priv->configfile = g_strdup (configfile);
   }

   priv->stream               = NULL;
   priv->event_queue          = NULL;
   priv->vo_driver            = NULL;
   priv->ao_driver            = NULL;

   priv->oldwidth             = 0;
   priv->oldheight            = 0;

   priv->video_driver_id = 0;
   priv->audio_driver_id = 0;

   return priv;
}


void
gimv_xine_private_destroy (GimvXinePrivate *priv)
{
   if (!priv) return;

   if (priv->video_driver_id)
      g_free (priv->video_driver_id);
   priv->video_driver_id = NULL;

   if (priv->audio_driver_id)
      g_free (priv->audio_driver_id);
   priv->audio_driver_id = NULL;

   /* exit xine */
   if (priv_xine_ref_count > 0) {
      /* save configuration */
      xine_config_save (priv->xine, priv->configfile);
      g_free (priv->configfile);
      priv->configfile = NULL;

      priv_xine_ref_count--;
      if (priv_xine_ref_count == 0) {
         xine_exit (priv_xine);
         priv_xine = NULL;
      }
   }

   priv->xine = NULL;

   g_free (priv);
}


/******************************************************************************
 *
 *   functions for snap shot.
 *
 *   These codes are mostly taken from xine-ui (src/xitk/snapshot.c).
 *
 *   This feature was originally created for and is dedicated to
 *       Vicki Gynn <vicki@anvil.org>
 *
 *   Copyright 2001 (c) Andrew Meredith <andrew@anvil.org>
 *
 ******************************************************************************/
#define PIXSZ 3
#define BIT_DEPTH 8

/*
 * Scale line with no horizontal scaling. For NTSC mpeg2 dvd input in
 * 4:3 output format (720x480 -> 720x540)
 */
static void
scale_line_1_1 (guchar *source, guchar *dest,
                gint width, gint step)
{
   memcpy (dest, source, width);
}


/*
 * Interpolates 64 output pixels from 45 source pixels using shifts.
 * Useful for scaling a PAL mpeg2 dvd input source to 1024x768
 * fullscreen resolution, or to 16:9 format on a monitor using square
 * pixels.
 * (720 x 576 ==> 1024 x 576)
 */
/* uuuuum */
static void scale_line_45_64 (guchar *source, guchar *dest,
                              gint width, gint step)
{
   gint p1, p2;

   while ((width -= 64) >= 0) {
      p1 = source[0];
      p2 = source[1];
      dest[0] = p1;
      dest[1] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[2];
      dest[2] = (5 * p2 + 3 * p1) >> 3;
      p2 = source[3];
      dest[3] = (7 * p1 + 1 * p2) >> 3;
      dest[4] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[4];
      dest[5] = (1 * p2 + 1 * p1) >> 1;
      p2 = source[5];
      dest[6] = (3 * p1 + 1 * p2) >> 2;
      dest[7] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[6];
      dest[8] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[7];
      dest[9] = (5 * p1 + 3 * p2) >> 3;
      p1 = source[8];
      dest[10] = p2;
      dest[11] = (1 * p2 + 3 * p1) >> 2;
      p2 = source[9];
      dest[12] = (5 * p1 + 3 * p2) >> 3;
      p1 = source[10];
      dest[13] = (7 * p2 + 1 * p1) >> 3;
      dest[14] = (1 * p2 + 7 * p1) >> 3;
      p2 = source[11];
      dest[15] = (1 * p1 + 1 * p2) >> 1;
      p1 = source[12];
      dest[16] = (3 * p2 + 1 * p1) >> 2;
      dest[17] = p1;
      p2 = source[13];
      dest[18] = (3 * p1 + 5 * p2) >> 3;
      p1 = source[14];
      dest[19] = (5 * p2 + 3 * p1) >> 3;
      p2 = source[15];
      dest[20] = p1;
      dest[21] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[16];
      dest[22] = (1 * p2 + 1 * p1) >> 1;
      p2 = source[17];
      dest[23] = (7 * p1 + 1 * p2) >> 3;
      dest[24] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[18];
      dest[25] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[19];
      dest[26] = (3 * p1 + 1 * p2) >> 2;
      dest[27] = p2;
      p1 = source[20];
      dest[28] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[21];
      dest[29] = (5 * p1 + 3 * p2) >> 3;
      p1 = source[22];
      dest[30] = (7 * p2 + 1 * p1) >> 3;
      dest[31] = (1 * p2 + 3 * p1) >> 2;
      p2 = source[23];
      dest[32] = (1 * p1 + 1 * p2) >> 1;
      p1 = source[24];
      dest[33] = (3 * p2 + 1 * p1) >> 2;
      dest[34] = (1 * p2 + 7 * p1) >> 3;
      p2 = source[25];
      dest[35] = (3 * p1 + 5 * p2) >> 3;
      p1 = source[26];
      dest[36] = (3 * p2 + 1 * p1) >> 2;
      p2 = source[27];
      dest[37] = p1;
      dest[38] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[28];
      dest[39] = (5 * p2 + 3 * p1) >> 3;
      p2 = source[29];
      dest[40] = (7 * p1 + 1 * p2) >> 3;
      dest[41] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[30];
      dest[42] = (1 * p2 + 1 * p1) >> 1;
      p2 = source[31];
      dest[43] = (3 * p1 + 1 * p2) >> 2;
      dest[44] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[32];
      dest[45] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[33];
      dest[46] = (5 * p1 + 3 * p2) >> 3;
      p1 = source[34];
      dest[47] = p2;
      dest[48] = (1 * p2 + 3 * p1) >> 2;
      p2 = source[35];
      dest[49] = (1 * p1 + 1 * p2) >> 1;
      p1 = source[36];
      dest[50] = (7 * p2 + 1 * p1) >> 3;
      dest[51] = (1 * p2 + 7 * p1) >> 3;
      p2 = source[37];
      dest[52] = (1 * p1 + 1 * p2) >> 1;
      p1 = source[38];
      dest[53] = (3 * p2 + 1 * p1) >> 2;
      dest[54] = p1;
      p2 = source[39];
      dest[55] = (3 * p1 + 5 * p2) >> 3;
      p1 = source[40];
      dest[56] = (5 * p2 + 3 * p1) >> 3;
      p2 = source[41];
      dest[57] = (7 * p1 + 1 * p2) >> 3;
      dest[58] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[42];
      dest[59] = (1 * p2 + 1 * p1) >> 1;
      p2 = source[43];
      dest[60] = (7 * p1 + 1 * p2) >> 3;
      dest[61] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[44];
      dest[62] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[45];
      dest[63] = (3 * p1 + 1 * p2) >> 2;
      source += 45;
      dest += 64;
   }

   if ((width += 64) <= 0) goto done;
   *dest++ = source[0];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[0] + 3 * source[1]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[1] + 3 * source[2]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[2] + 1 * source[3]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1  *source[2] + 3 * source[3]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[3] + 1 * source[4]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[4] + 1 * source[5]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[4] + 7 * source[5]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[5] + 5 * source[6]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[6] + 3 * source[7]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = source[7];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[7] + 3 * source[8]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[8] + 3 * source[9]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[9] + 1 * source[10]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[9] + 7 * source[10]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[10] + 1 * source[11]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[11] + 1 * source[12]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = source[12];
   if (--width <= 0) goto done;
   *dest++ = (3 * source[12] + 5 * source[13]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[13] + 3 * source[14]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = source[14];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[14] + 3 * source[15]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[15] + 1 * source[16]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[16] + 1 * source[17]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[16] + 7 * source[17]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[17] + 5 * source[18]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[18] + 1 * source[19]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = source[19];
   if (--width <= 0) goto done;
   *dest++ = (3 * source[19] + 5 * source[20]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[20] + 3 * source[21]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[21] + 1 * source[22]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[21] + 3 * source[22]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[22] + 1 * source[23]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[23] + 1 * source[24]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[23] + 7 * source[24]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[24] + 5 * source[25]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[25] + 1 * source[26]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = source[26];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[26] + 3 * source[27]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[27] + 3 * source[28]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[28] + 1 * source[29]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[28] + 7 * source[29]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[29] + 1 * source[30]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[30] + 1 * source[31]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[30] + 7 * source[31]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[31] + 5 * source[32]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[32] + 3 * source[33]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = source[33];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[33] + 3 * source[34]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[34] + 1 * source[35]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[35] + 1 * source[36]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[35] + 7 * source[36]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[36] + 1 * source[37]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[37] + 1 * source[38]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = source[38];
   if (--width <= 0) goto done;
   *dest++ = (3 * source[38] + 5 * source[39]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[39] + 3 * source[40]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[40] + 1 * source[41]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[40] + 3 * source[41]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[41] + 1 * source[42]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[42] + 1 * source[43]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[42] + 7 * source[43]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[43] + 5 * source[44]) >> 3;
 done: ;
}


/*
 * Interpolates 16 output pixels from 15 source pixels using shifts.
 * Useful for scaling a PAL mpeg2 dvd input source to 4:3 format on
 * a monitor using square pixels.
 * (720 x 576 ==> 768 x 576)
 */
/* uum */
static void
scale_line_15_16 (guchar *source, guchar *dest,
                  gint width, gint step)
{
   gint p1, p2;

   while ((width -= 16) >= 0) {
      p1 = source[0];
      dest[0] = p1;
      p2 = source[1];
      dest[1] = (1 * p1 + 7 * p2) >> 3;
      p1 = source[2];
      dest[2] = (1 * p2 + 7 * p1) >> 3;
      p2 = source[3];
      dest[3] = (1 * p1 + 3 * p2) >> 2;
      p1 = source[4];
      dest[4] = (1 * p2 + 3 * p1) >> 2;
      p2 = source[5];
      dest[5] = (3 * p1 + 5 * p2) >> 3;
      p1 = source[6];
      dest[6] = (3 * p2 + 5 * p1) >> 3;
      p2 = source[7];
      dest[7] = (1 * p1 + 1 * p1) >> 1;
      p1 = source[8];
      dest[8] = (1 * p2 + 1 * p1) >> 1;
      p2 = source[9];
      dest[9] = (5 * p1 + 3 * p2) >> 3;
      p1 = source[10];
      dest[10] = (5 * p2 + 3 * p1) >> 3;
      p2 = source[11];
      dest[11] = (3 * p1 + 1 * p2) >> 2;
      p1 = source[12];
      dest[12] = (3 * p2 + 1 * p1) >> 2;
      p2 = source[13];
      dest[13] = (7 * p1 + 1 * p2) >> 3;
      p1 = source[14];
      dest[14] = (7 * p2 + 1 * p1) >> 3;
      dest[15] = p1;
      source += 15;
      dest += 16;
   }

   if ((width += 16) <= 0) goto done;
   *dest++ = source[0];
   if (--width <= 0) goto done;
   *dest++ = (1 * source[0] + 7 * source[1]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[1] + 7 * source[2]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[2] + 3 * source[3]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[3] + 3 * source[4]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[4] + 5 * source[5]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[5] + 5 * source[6]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[6] + 1 * source[7]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (1 * source[7] + 1 * source[8]) >> 1;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[8] + 3 * source[9]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (5 * source[9] + 3 * source[10]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[10] + 1 * source[11]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (3 * source[11] + 1 * source[12]) >> 2;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[12] + 1 * source[13]) >> 3;
   if (--width <= 0) goto done;
   *dest++ = (7 * source[13] + 1 * source[14]) >> 3;
 done: ;
}


static int
scale_image (GimvXinePrivImage *image)
{
   gint i;
   gint step = 1; /* unused variable for the scale functions */

   /* pointers for post-scaled line buffer */
   guchar *n_y;
   guchar *n_u;
   guchar *n_v;

   /* pointers into pre-scaled line buffers */
   guchar *oy = image->y;
   guchar *ou = image->u;
   guchar *ov = image->v;
   guchar *oy_p = image->y;
   guchar *ou_p = image->u;
   guchar *ov_p = image->v;

   /* pointers into post-scaled line buffers */
   guchar *ny_p;
   guchar *nu_p;
   guchar *nv_p;

   /* old line widths */
   gint oy_width = image->width;
   gint ou_width = image->u_width;
   gint ov_width = image->v_width;

   /* new line widths NB scale factor is factored by 32768 for rounding */
   gint ny_width = (oy_width * image->scale_factor) / 32768;
   gint nu_width = (ou_width * image->scale_factor) / 32768;
   gint nv_width = (ov_width * image->scale_factor) / 32768;

   /* allocate new buffer space space for post-scaled line buffers */
   n_y = g_malloc (ny_width * image->height);
   if (!n_y) return 0;
   n_u = g_malloc (nu_width * image->u_height);
   if (!n_u) return 0;
   n_v = g_malloc (nv_width * image->v_height);
   if (!n_v) return 0;

   /* set post-scaled line buffer progress pointers */
   ny_p = n_y;
   nu_p = n_u;
   nv_p = n_v;

   /* Do the scaling */

   for (i = 0; i < image->height; ++i) {
      image->scale_line (oy_p, ny_p, ny_width, step);
      oy_p += oy_width;
      ny_p += ny_width;
   }

   for (i = 0; i < image->u_height; ++i) {
      image->scale_line (ou_p, nu_p, nu_width, step);
      ou_p += ou_width;
      nu_p += nu_width;
   }

   for (i = 0; i < image->v_height; ++i) {
      image->scale_line (ov_p, nv_p, nv_width, step);
      ov_p += ov_width;
      nv_p += nv_width;
   }

   /* switch to post-scaled data and widths */
   image->y = n_y;
   image->u = n_u;
   image->v = n_v;
   image->width = ny_width;
   image->u_width = nu_width;
   image->v_width = nv_width;

   if (image->yuy2) {
      g_free (oy);
      g_free (ou);
      g_free (ov);
   }

   return 1;
}


/*
 *  This function was pinched from filter_yuy2tov12.c, part of
 *  transcode, a linux video stream processing tool
 *
 *  Copyright (C) Thomas ŽÖstreich - June 2001
 *
 *  Thanks Thomas
 *      
 */
static void
yuy2toyv12 (GimvXinePrivImage *image)
{
   gint i,j,w2;

   /* I420 */
   guchar *y = image->y;
   guchar *u = image->u;
   guchar *v = image->v;

   guchar *input = image->yuy2;
    
   gint width  = image->width;
   gint height = image->height;

   w2 = width / 2;

   for (i = 0; i < height; i += 2) {
      for (j = 0; j < w2; j++) {

         /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
         *(y++) = *(input++);
         *(u++) = *(input++);
         *(y++) = *(input++);
         *(v++) = *(input++);
      }
      
      /* down sampling */
      
      for (j = 0; j < w2; j++) {
         /* skip every second line for U and V */
         *(y++) = *(input++);
         input++;
         *(y++) = *(input++);
         input++;
      }
   }
}


/*
 *  This function is a fudge .. a hack.
 *
 *  It is in place purely to get snapshots going for YUY2 data
 *  longer term there needs to be a bit of a reshuffle to account
 *  for the two fundamentally different YUV formats. Things would
 *  have been different had I known how YUY2 was done before designing
 *  the flow. Teach me to make assumptions I guess.
 *
 *  So .. this function converts the YUY2 image to YV12. The downside
 *  being that as YV12 has half as many chroma rows as YUY2, there is
 *  a loss of image quality.
 */
/* uuuuuuuuuuuuuuum. */
static gint
yuy2_fudge (GimvXinePrivImage *image)
{
   image->y = g_malloc0 (image->height * image->width);
   if (!image->y) goto ERROR0;

   image->u = g_malloc0 (image->u_height * image->u_width);
   if (!image->u) goto ERROR1;

   image->v = g_malloc0 (image->v_height * image->v_width);
   if (!image->v) goto ERROR2;

   yuy2toyv12 (image);

   return 1;

 ERROR2:
   g_free (image->u);
   image->u = NULL;
 ERROR1:
   g_free (image->y);
   image->y = NULL;
 ERROR0:
   return 0;
}


#define clip_8_bit(val)                                                        \
{                                                                              \
   if (val < 0)                                                                \
      val = 0;                                                                 \
   else                                                                        \
      if (val > 255) val = 255;                                                \
}


/*
 *   Create rgb data from yv12
 */
static guchar *
yv12_2_rgb (GimvXinePrivImage *image)
{
   gint i, j;

   gint y, u, v;
   gint r, g, b;

   gint sub_i_u;
   gint sub_i_v;

   gint sub_j_u;
   gint sub_j_v;

   guchar *rgb;

   rgb = g_malloc0 (image->width * image->height * PIXSZ * sizeof (guchar));
   if (!rgb) return NULL;

   for (i = 0; i < image->height; ++i) {
      /* calculate u & v rows */
      sub_i_u = ((i * image->u_height) / image->height);
      sub_i_v = ((i * image->v_height) / image->height);

      for (j = 0; j < image->width; ++j) {
         /* calculate u & v columns */
         sub_j_u = ((j * image->u_width) / image->width);
         sub_j_v = ((j * image->v_width) / image->width);

         /***************************************************
          *
          *  Colour conversion from 
          *    http://www.inforamp.net/~poynton/notes/colour_and_gamma/ColorFAQ.html#RTFToC30
          *
          *  Thanks to Billy Biggs <vektor@dumbterm.net>
          *  for the pointer and the following conversion.
          *
          *   R' = [ 1.1644         0    1.5960 ]   ([ Y' ]   [  16 ])
          *   G' = [ 1.1644   -0.3918   -0.8130 ] * ([ Cb ] - [ 128 ])
          *   B' = [ 1.1644    2.0172         0 ]   ([ Cr ]   [ 128 ])
          *
          *  Where in Xine the above values are represented as
          *
          *   Y' == image->y
          *   Cb == image->u
          *   Cr == image->v
          *
          ***************************************************/

         y = image->y[(i * image->width) + j] - 16;
         u = image->u[(sub_i_u * image->u_width) + sub_j_u] - 128;
         v = image->v[(sub_i_v * image->v_width) + sub_j_v] - 128;

         r = (1.1644 * y)                + (1.5960 * v);
         g = (1.1644 * y) - (0.3918 * u) - (0.8130 * v);
         b = (1.1644 * y) + (2.0172 * u);

         clip_8_bit (r);
         clip_8_bit (g);
         clip_8_bit (b);

         rgb[(i * image->width + j) * PIXSZ + 0] = r;
         rgb[(i * image->width + j) * PIXSZ + 1] = g;
         rgb[(i * image->width + j) * PIXSZ + 2] = b;
      }
   }

   return rgb;
}


GimvXinePrivImage *
gimv_xine_priv_image_new (gint imgsize)
{
   GimvXinePrivImage *image;

   image = g_new0 (GimvXinePrivImage, 1);

   if (imgsize > 0) {
      image->img = g_malloc0 (imgsize);

      if (!image->img) {
         g_free (image);
         return NULL;
      }
   } else {
      image->img = NULL;
   }

   image->y    = NULL;
   image->u    = NULL;
   image->v    = NULL;
   image->yuy2 = NULL;
  
   return image;
}


void
gimv_xine_priv_image_delete (GimvXinePrivImage *image)
{
   g_free (image->img);
   g_free (image);
}


guchar *
gimv_xine_priv_yuv2rgb (GimvXinePrivImage *image)
{
   guchar *rgb;

   g_return_val_if_fail (image, NULL);

   switch (image->ratio_code) {
   case XINE_VO_ASPECT_SQUARE:
      image->scale_line = scale_line_1_1;
      image->scale_factor = (32768 * 1) / 1;
      break;

   case XINE_VO_ASPECT_4_3:
      image->scale_line = scale_line_15_16;
      image->scale_factor = (32768 * 16) / 15;
      break;

   case XINE_VO_ASPECT_ANAMORPHIC:
      image->scale_line = scale_line_45_64;
      image->scale_factor = (32768 * 64) / 45;
      break;

   case XINE_VO_ASPECT_DVB:
      image->scale_line = scale_line_45_64;
      image->scale_factor = (32768 * 64) / 45;
      break;

   case XINE_VO_ASPECT_DONT_TOUCH:
   default:
      /* the mpeg standard has a few that we don't know about */
      g_print ( "unknown aspect ratio. will assume 1:1\n" ); 
      image->scale_line = scale_line_1_1;
      image->scale_factor = (32768 * 1) / 1;
      break;
   }

   switch ( image->format ) {
   case XINE_IMGFMT_YV12:
      printf( "XINE_IMGFMT_YV12\n" ); 
      image->y        = image->img;
      image->u        = image->img + (image->width*image->height);
      image->v        = image->img
                           + (image->width * image->height)
                           + (image->width * image->height) / 4;
      image->u_width  = ((image->width  + 1) / 2);
      image->v_width  = ((image->width  + 1) / 2);
      image->u_height = ((image->height + 1) / 2);
      image->v_height = ((image->height + 1) / 2);
      break;

   case XINE_IMGFMT_YUY2: 
      printf( "XINE_IMGFMT_YUY2\n" );
      image->yuy2     = image->img;
      image->u_width  = ((image->width  + 1) / 2);
      image->v_width  = ((image->width  + 1) / 2);
      image->u_height = ((image->height + 1) / 2);
      image->v_height = ((image->height + 1) / 2);
      break;

   default:                
      printf( "Unknown\nError: Format Code %d Unknown\n", image->format ); 
      printf( "  ** Please report this error to andrew@anvil.org **\n" );
      //prvt_image_free( &image );
      return NULL;
   }

   /*
    *  If YUY2 convert to YV12
    */
   if (image->format == XINE_IMGFMT_YUY2) {
      if (!yuy2_fudge (image)) {
         return NULL;
      }
   }

   scale_image (image);

   rgb = yv12_2_rgb (image);

   /* FIXME */
   g_free (image->y);
   g_free (image->u);
   g_free (image->v);
   image->y = NULL;
   image->u = NULL;
   image->v = NULL;

   return rgb;
}


#endif /* ENABLE_XINE */
