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

#ifndef __ARGPARSE_H__
#define __ARGPARSE_H__

#include <stdio.h>
#include <gtk/gtk.h>

typedef enum
{
   ARG_NOERR,
   ARG_WITH_VAL,
   ARG_NO_VAL,
   ARG_ERR_PARSE,
   ARG_ERR_UNKNOWN,
   ARG_HELP
} arg_error_t;

struct arg_opt
{
   const char *name;
   const int   key;
   const char *arg;
   const char *doc;
};

typedef struct ArgsVal_Tag
{
   gboolean read_dir;
   gboolean read_dir_recursive;
   gboolean read_dot;
   gboolean ignore_ext;
   gboolean open_imagewin;
   gboolean open_thumbwin;
   gboolean exec_slideshow;
   gfloat   slideshow_interval;     /* [sec] */
} ArgsVal;

void arg_version   (void);
void arg_help      (char *argv[], FILE *stream);
void arg_parse     (int argc, char *argv[], int *remaining);
void args_val_init (void);

#endif /* __ARGPARSE_H__ */
