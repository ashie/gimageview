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

#include <gtk/gtk.h>

#include "dither.h"

static gint *evenerr = NULL,*odderr = NULL;
static guchar *dbuf = NULL;


guchar *ditherinit(gint w)
{
   ditherfinish();
   evenerr = g_malloc0 (3 * (w + 10) * sizeof(gint));
   if (!evenerr) goto ERROR0;

   odderr =  g_malloc0 (3 * (w + 10) * sizeof(gint));
   if (!odderr) goto ERROR1;

   dbuf = g_malloc (w);
   if (!dbuf) goto ERROR2;

   return dbuf;

ERROR2:
   g_free (odderr);
   odderr = NULL;
ERROR1:
   g_free (evenerr);
   evenerr = NULL;
ERROR0:
   return NULL;
}


void ditherfinish(void)
{
   if (evenerr) {
      g_free (evenerr);
      evenerr = NULL;
   }

   if (odderr) {
      g_free (odderr);
      odderr  = NULL;
   }

   if (dbuf) {
      g_free (dbuf);
      dbuf = NULL;
   }
}

void ditherline(guchar *theline, gint linenum, gint width)
{
   gint x,y,lx;
   gint c0, c1, c2, times2;
   gint terr0, terr1, terr2, actual0, actual1, actual2;
   gint start, addon, r, g, b;
   gint *thiserr;
   gint *nexterr;

   y = linenum;
   if ((y & 1 ) == 0) {
      start = 0;
      addon = 1;
      thiserr = evenerr + 3;
      nexterr = odderr + width * 3;
   } else {
      start = width - 1;
      addon = -1;
      thiserr = odderr + 3;
      nexterr = evenerr + width * 3;
   }
   nexterr[0] = nexterr[1] = nexterr[2] = 0;

   x = start;
   for(lx = 0; lx < width; lx++) {
      r = theline[x * 3];
      g = theline[x * 3 + 1];
      b = theline[x * 3 + 2];

      terr0 = r + ((thiserr[0] + 8) >> 4);
      terr1 = g + ((thiserr[1] + 8) >> 4);
      terr2 = b + ((thiserr[2] + 8) >> 4);

      /* is this going to screw up on white? */  
      actual0 = (terr0 >> 5) * 255/7;
      actual1 = (terr1 >> 5) * 255/7;
      actual2 = (terr2 >> 6) * 255/3;
  
      if (actual0 < 0)   actual0 = 0;
      if (actual0 > 255) actual0 = 255;
      if (actual1 < 0)   actual1 = 0;
      if (actual1 > 255) actual1 = 255;
      if( actual2 < 0)   actual2 = 0;
      if (actual2 > 255) actual2 = 255;
  
      c0 = terr0 - actual0;
      c1 = terr1 - actual1;
      c2 = terr2 - actual2;

      times2 = (c0 << 1);
      nexterr[-3]  = c0; c0 += times2;
      nexterr[ 3] += c0; c0 += times2;
      nexterr[ 0] += c0; c0 += times2;
      thiserr[ 3] += c0;

      times2 = (c1 << 1);
      nexterr[-2]  = c1; c1 += times2;
      nexterr[ 4] += c1; c1 += times2;
      nexterr[ 1] += c1; c1 += times2;
      thiserr[ 4] += c1;

      times2 = (c2 << 1);
      nexterr[-1] =  c2; c2 += times2;
      nexterr[ 5] += c2; c2 += times2;
      nexterr[ 2] += c2; c2 += times2;
      thiserr[ 5] += c2;

      dbuf[x] = (actual0 >> 5) * 32 + (actual1 >> 5) * 4 + (actual2 >> 6);

      thiserr += 3;
      nexterr -= 3;
      x += addon;
   }
}
