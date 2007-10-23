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

/*
 * 2003-05-17 Takuro Ashie <ashie@homa.ne.jp>
 *
 *     Fetched from textutils-2.1 and adapt to GImageView.
 *
 *     Copyright (C) 1995-2002 Free Software Foundation, Inc.
 *     Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>.
 */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#include "gimv_dupl_finder.h"
#include "gimv_image_info.h"
#include "gimv_thumb.h"
#include "utils_md5.h"


/* Most systems do not distinguish between external and internal
   text representations.  */
/* FIXME: This begs for an autoconf test.  */
#if O_BINARY
# define OPENOPTS(BINARY) ((BINARY) != 0 ? TEXT1TO1 : TEXTCNVT)
# define TEXT1TO1 "rb"
# define TEXTCNVT "r"
#else
# if defined VMS
#  define OPENOPTS(BINARY) ((BINARY) != 0 ? TEXT1TO1 : TEXTCNVT)
#  define TEXT1TO1 "rb", "ctx=stm"
#  define TEXTCNVT "r", "ctx=stm"
# else
#  if UNIX || __UNIX__ || unix || __unix__ || _POSIX_VERSION
#   define OPENOPTS(BINARY) "r"
#  else
    /* The following line is intended to evoke an error.
       Using #error is not portable enough.  */
    "Cannot determine system type."
#  endif
# endif
#endif

#define DIGEST_BIN_BYTES (128 / 8)

static gboolean
digest_file (const gchar *filename, gint binary, guchar *bin_result)
{
   FILE *fp;
   gint err;

   fp = fopen (filename, OPENOPTS (binary));
   if (fp == NULL)
      return FALSE;

   err = md5_stream (fp, bin_result);
   if (err) {
      fclose (fp);
      return FALSE;
   }

   if (fclose (fp) == EOF)
      return FALSE;

   return TRUE;
}


static gpointer
duplicates_md5_get_data (GimvThumb *thumb)
{
   const gchar *filename;
   gchar *temp_file = NULL;
   guchar bin_buffer[DIGEST_BIN_BYTES + 1];
   gboolean success;

   g_return_val_if_fail (GIMV_IS_THUMB(thumb), NULL);
   g_return_val_if_fail (thumb->info, NULL);

   if (gimv_image_info_need_temp_file (thumb->info)) {
      temp_file = gimv_image_info_get_temp_file (thumb->info);
      filename = temp_file;
   } else {
      filename = gimv_image_info_get_path (thumb->info);
   }

   g_return_val_if_fail (filename && *filename, NULL);

   success = digest_file (filename, TRUE, bin_buffer);
   if (!success)
      return NULL;

   bin_buffer[DIGEST_BIN_BYTES] = '\0';

   return g_strdup (bin_buffer);
}


static gint
duplicates_md5_compare (gpointer data1, gpointer data2, gfloat *similarity)
{
   const gchar *str1 = data1, *str2 = data2;
   gint retval;

   if (!str1)
      return str2 ? 1 : 0;
   else if (!str2)
      return 1;

   retval = strcmp (str1, str2);

   if (!retval)
      *similarity = 1.0;
   else
      *similarity = 0.0;

   return retval;
}


static void
duplicates_md5_data_delete (gpointer data)
{
   g_free (data);
}


GimvDuplCompFuncTable gimv_dupl_md5_funcs =
{
   label:       N_("md5sum"),
   get_data:    duplicates_md5_get_data,
   compare:     duplicates_md5_compare,
   data_delete: duplicates_md5_data_delete,
};
