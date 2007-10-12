/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001-2002 Takuro Ashie
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */


#include <ctype.h>
#include "utils.h"

#ifndef FALSE
#  define FALSE   (0)
#endif

#ifndef TRUE
#  define TRUE    (!FALSE)
#endif


int
string_is_ascii_printable (const char *string)
{
   int i = 0, printable;

   if (!string) return FALSE;

   while (string[i]) {
      printable = isprint(string[i]);
      if (!printable) return FALSE;
      i++;
   }

   return TRUE;
}


int
string_is_ascii_graphable (const char *string)
{
   int i = 0, printable;

   if (!string) return FALSE;

   while (string[i]) {
      printable = isgraph(string[i]);
      if (!printable) return FALSE;
      i++;
   }

   return TRUE;
}
