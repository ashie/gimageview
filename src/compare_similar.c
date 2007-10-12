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

/*
 * These codes are mostly taken from GQview.
 * GQview code Copyright (C) 2001 John Ellis
 */

#include <stdlib.h>

#include "gimv_image.h"
#include "gimv_thumb.h"
#include "gimv_dupl_finder.h"
#include "prefs.h"

/*
 * These functions are intended to find images with similar color content. For
 * example when an image was saved at different compression levels or dimensions
 * (scaled down/up) the contents are similar, but these files do not match by file
 * size, dimensions, or checksum.
 *
 * These functions create a 32 x 32 array for each color channel (red, green, blue).
 * The array represents the average color of each corresponding part of the
 * image. (imagine the image cut into 1024 rectangles, or a 32 x 32 grid.
 * Each grid is then processed for the average color value, this is what
 * is stored in the array)
 *
 * To compare two images, generate a ImageSimilarityData for each image, then
 * pass them to the compare function. The return value is the percent match
 * of the two images. (for this, simple comparisons are used, basically the return
 * is an average of the corresponding array differences)
 *
 * for image_sim_compare(), the return is 0.0 to 1.0:
 *  1.0 for exact matches (an image is compared to itself)
 *  0.0 for exact opposite images (compare an all black to an all white image)
 * generally only a match of > 0.85 are significant at all, and >.95 is useful to
 * find images that have been re-saved to other formats, dimensions, or compression.
 */

typedef struct ImageSimilarityData_Tag ImageSimilarityData;

struct ImageSimilarityData_Tag
{
   guint8 avg_r[1024];
   guint8 avg_g[1024];
   guint8 avg_b[1024];

   gint filled;
};


ImageSimilarityData *image_sim_new            (void);
void                 image_sim_delete         (ImageSimilarityData *sd);

void                 image_sim_fill_data      (ImageSimilarityData *sd,
                                               GimvImage           *image);
ImageSimilarityData *image_sim_new_from_image (GimvImage           *image);
ImageSimilarityData *image_sim_new_from_thumb (GimvThumb           *thumb);

gfloat               image_sim_compare        (ImageSimilarityData *a,
                                               ImageSimilarityData *b);
gfloat               image_sim_compare_fast   (ImageSimilarityData *a,
                                               ImageSimilarityData *b,
                                               gfloat               min);


ImageSimilarityData *
image_sim_new (void)
{
   ImageSimilarityData *sd = g_new0 (ImageSimilarityData, 1);

   return sd;
}

void
image_sim_delete (ImageSimilarityData *sd)
{
   g_free (sd);
}

void
image_sim_fill_data (ImageSimilarityData *sd, GimvImage *image)
{
   gint w, h;
   gint rs;
   guchar *pix;
   gint has_alpha;
   gint p_step;

   guchar *p;
   gint i;
   gint j;
   gint x_inc, y_inc;
   gint xs, ys;

   gint x_small = FALSE;	/* if less than 32 w or h, set TRUE */
   gint y_small = FALSE;

   if (!sd || !image) return;

   w = gimv_image_width (image);
   h = gimv_image_height (image);
   rs = gimv_image_rowstride (image);
   pix = gimv_image_get_pixels (image);
   has_alpha = gimv_image_has_alpha (image);

   p_step = has_alpha ? 4 : 3;
   x_inc = w / 32;
   y_inc = h / 32;

   if (x_inc < 1) {
      x_inc = 1;
      x_small = TRUE;
   }
   if (y_inc < 1) {
      y_inc = 1;
      y_small = TRUE;
   }

   j = 0;

   for (ys = 0; ys < 32; ys++) {
      if (y_small) j = (float)h / 32 * ys;

      i = 0;

      for (xs = 0; xs < 32; xs++) {
         gint x, y;
         gint r, g, b;

         if (x_small)
            i = (float) w / 32 * xs;

         r = g = b = 0;

         for (y = j; y < j + y_inc; y++) {
            p = pix + (y * rs) + (i * p_step);
            for (x = i; x < i + x_inc; x++) {
               r += *p; p++;
               g += *p; p++;
               b += *p; p++;
               if (has_alpha) p++;
            }
         }
         r /= x_inc * y_inc;
         g /= x_inc * y_inc;
         b /= x_inc * y_inc;

         sd->avg_r[ys * 32 + xs] = r;
         sd->avg_g[ys * 32 + xs] = g;
         sd->avg_b[ys * 32 + xs] = b;

         i += x_inc;
      }

      j += y_inc;
   }

   sd->filled = TRUE;
}

ImageSimilarityData *
image_sim_new_from_image (GimvImage *image)
{
   ImageSimilarityData *sd;

   sd = image_sim_new ();
   image_sim_fill_data (sd, image);

   return sd;
}


ImageSimilarityData *
image_sim_new_from_thumb (GimvThumb *thumb)
{
   GimvImage *image;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   ImageSimilarityData *data;

   g_return_val_if_fail (GIMV_IS_THUMB(thumb), NULL);

   if (!gimv_thumb_has_thumbnail (thumb)) return NULL;

   gimv_thumb_get_thumb (thumb, &pixmap, &mask)     ;
   if (!pixmap) return NULL;;
   image = gimv_image_create_from_drawable (pixmap, 0, 0,
                                            thumb->thumb_width,
                                            thumb->thumb_height);

   if (!image) return NULL;
   data = image_sim_new_from_image (image);

   gimv_image_unref (image);

   return data;
}


gfloat
image_sim_compare (ImageSimilarityData *a, ImageSimilarityData *b)
{
   gfloat sim;
   gint i;

   if (!a || !b || !a->filled || !b->filled) return 0.0;

   sim = 0.0;

   for (i = 0; i < 1024; i++) {
      sim += (float) abs (a->avg_r[i] - b->avg_r[i]) / 255.0;
      sim += (float) abs (a->avg_g[i] - b->avg_g[i]) / 255.0;
      sim += (float) abs (a->avg_b[i] - b->avg_b[i]) / 255.0;
   }

   sim /= (1024.0 * 3.0);

   return 1.0 - sim;
}

/* this uses a cutoff point so that it can abort early when it gets to
 * a point that can simply no longer make the cut-off point.
 */
gfloat
image_sim_compare_fast (ImageSimilarityData *a, ImageSimilarityData *b, gfloat min)
{
   gfloat sim;
   gint i;
   gint j;

   if (!a || !b || !a->filled || !b->filled) return 0.0;

   min = 1.0 - min;
   sim = 0.0;

   for (j = 0; j < 1024; j+= 32) {
      for (i = j; i < j + 32; i++) {
         sim += (float) abs (a->avg_r[i] - b->avg_r[i]) / 255.0;
         sim += (float) abs (a->avg_g[i] - b->avg_g[i]) / 255.0;
         sim += (float) abs (a->avg_b[i] - b->avg_b[i]) / 255.0;
      }
      /* check for abort, if so return 0.0 */
      if (sim / (1024.0 * 3.0) > min) return 0.0;
   }

   sim /= (1024.0 * 3.0);

   return 1.0 - sim;
}


static gpointer
duplicates_similar_get_data (GimvThumb *thumb)
{
   return image_sim_new_from_thumb (thumb);
}


static gint
duplicates_similar_compare (gpointer data1, gpointer data2, gfloat *similarity)
{
   gfloat sim = image_sim_compare (data1, data2);

   if (similarity)
      *similarity = sim;

   if (sim > conf.search_similar_accuracy) {
      return 0;
   }

   return -1;
}


static void
duplicates_similar_data_delete (gpointer data)
{
   image_sim_delete (data);
}


GimvDuplCompFuncTable gimv_dupl_similar_funcs =
{
   label:       N_("Similarity"),
   get_data:    duplicates_similar_get_data,
   compare:     duplicates_similar_compare,
   data_delete: duplicates_similar_data_delete,
};
