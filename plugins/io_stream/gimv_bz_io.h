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

#ifndef __GIMV_BZ_IO__
#define __GIMV_BZ_IO__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_BZLIB

#include <gimv_io.h>

/* API functions */
#ifdef BZAPI_NEEDS_PREFIX
#  define BZLIBVERSION BZ2_bzlibVersion
#  define BZOPEN  BZ2_bzopen
#  define BZREAD  BZ2_bzread
#  define BZERROR BZ2_bzerror
#  define BZCLOSE BZ2_bzclose
#else
#  define BZLIBVERSION bzlibVersion
#  define BZOPEN  bzopen
#  define BZREAD  bzread
#  define BZERROR bzerror
#  define BZCLOSE bzclose
#endif

GimvIO *gimv_bz_io_new (const gchar *url, const gchar *mode);

#endif /* HAVE_BZLIB */

#endif /* __GIMV_BZ_IO__ */
