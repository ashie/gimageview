/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2003 Takuro Ashie
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

#include "gimv_dupl_finder.h"
#include "gimv_image_info.h"
#include "gimv_thumb.h"


static gint
duplicates_file_size_compare (gpointer data1, gpointer data2, gfloat *similarity)
{
   GimvThumb *thumb1 = data1, *thumb2 = data2;
   gint retval, d;

   if (!thumb1)
      return thumb2 ? 1 : 0;
   else if (!thumb2)
      return 1;

   d = thumb1->info->st.st_size - thumb2->info->st.st_size;

   if (d < 0)
      retval = -1;
   else if (d > 0)
      retval = 1;
   else
      retval = 0;

   if (!d)
      *similarity = 1.0;
   else
      *similarity = 0.0;

   return retval;
}


GimvDuplCompFuncTable gimv_dupl_file_size_funcs =
{
   label:       N_("File Size"),
   get_data:    NULL,
   compare:     duplicates_file_size_compare,
   data_delete: NULL,
};
