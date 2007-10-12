/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  Copyright (C) 2001 The Free Software Foundation
 *
 *  This program is released under the terms of the GNU General Public 
 *  License version 2, you can find a copy of the lincense in the file
 *  COPYING. *
 *
 *  $Id$
 */

#include "pixbuf_utils.h"

#include <gdk-pixbuf/gdk-pixbuf-loader.h>


/*
 * Returns a copy of pixbuf src rotated 90 degrees clockwise or 90 
 * counterclockwise.
 */
GdkPixbuf *
pixbuf_copy_rotate_90 (GdkPixbuf *src, 
                       gboolean counter_clockwise)
{
   GdkPixbuf *dest;
   gint has_alpha;
   gint src_w, src_h, src_rs;
   gint dest_w, dest_h, dest_rs;
   guchar *src_pix,  *src_p;
   guchar *dest_pix, *dest_p;
   gint i, j;
   gint a;

   if (!src) return NULL;

   src_w = gdk_pixbuf_get_width (src);
   src_h = gdk_pixbuf_get_height (src);
   has_alpha = gdk_pixbuf_get_has_alpha (src);
   src_rs = gdk_pixbuf_get_rowstride (src);
   src_pix = gdk_pixbuf_get_pixels (src);

   dest_w = src_h;
   dest_h = src_w;
   dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, dest_w, dest_h);
   dest_rs = gdk_pixbuf_get_rowstride (dest);
   dest_pix = gdk_pixbuf_get_pixels (dest);

   a = (has_alpha ? 4 : 3);

   for (i = 0; i < src_h; i++) {
      src_p = src_pix + (i * src_rs);
      for (j = 0; j < src_w; j++) {
         if (counter_clockwise)
            dest_p = dest_pix + ((dest_h - j - 1) * dest_rs) + (i * a);
         else
            dest_p = dest_pix + (j * dest_rs) + ((dest_w - i - 1) * a);

         *(dest_p++) = *(src_p++);	/* r */
         *(dest_p++) = *(src_p++);	/* g */
         *(dest_p++) = *(src_p++);	/* b */
         if (has_alpha) *(dest_p) = *(src_p++);	/* a */
      }
   }

   return dest;
}


/*
 * Returns a copy of pixbuf mirrored and or flipped.
 * TO do a 180 degree rotations set both mirror and flipped TRUE
 * if mirror and flip are FALSE, result is a simple copy.
 */
GdkPixbuf *
pixbuf_copy_mirror (GdkPixbuf *src, 
                    gboolean mirror, 
                    gboolean flip)
{
   GdkPixbuf *dest;
   gint has_alpha;
   gint w, h;
   gint    src_rs,   dest_rs;
   guchar *src_pix, *dest_pix;
   guchar *src_p,   *dest_p;
   gint i, j;
   gint a;

   if (!src) return NULL;

   w = gdk_pixbuf_get_width (src);
   h = gdk_pixbuf_get_height (src);
   has_alpha = gdk_pixbuf_get_has_alpha (src);
   src_rs = gdk_pixbuf_get_rowstride (src);
   src_pix = gdk_pixbuf_get_pixels (src);

   dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
   dest_rs = gdk_pixbuf_get_rowstride (dest);
   dest_pix = gdk_pixbuf_get_pixels (dest);

   a = has_alpha ? 4 : 3;

   for (i = 0; i < h; i++)	{
      src_p = src_pix + (i * src_rs);
      if (flip)
         dest_p = dest_pix + ((h - i - 1) * dest_rs);
      else
         dest_p = dest_pix + (i * dest_rs);

      if (mirror) {
         dest_p += (w - 1) * a;
         for (j = 0; j < w; j++) {
            *(dest_p++) = *(src_p++);	/* r */
            *(dest_p++) = *(src_p++);	/* g */
            *(dest_p++) = *(src_p++);	/* b */
            if (has_alpha) *(dest_p) = *(src_p++);	/* a */
            dest_p -= (a + 3);
         }
      } else {
         for (j = 0; j < w; j++) {
            *(dest_p++) = *(src_p++);	/* r */
            *(dest_p++) = *(src_p++);	/* g */
            *(dest_p++) = *(src_p++);	/* b */
            if (has_alpha) *(dest_p++) = *(src_p++);	/* a */
         }
      }
   }

   return dest;
}


void
pixmap_from_xpm (const char **data, 
                 GdkPixmap **pixmap, 
                 GdkBitmap **mask)
{
   GdkPixbuf *pixbuf;

   pixbuf = gdk_pixbuf_new_from_xpm_data (data);
   gdk_pixbuf_render_pixmap_and_mask (pixbuf, pixmap, mask, 127);
   gdk_pixbuf_unref (pixbuf);
}
