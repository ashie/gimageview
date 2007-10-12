/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 *  $Id$
 */

#ifndef FR_COMMAND_ZIP_H
#define FR_COMMAND_ZIP_H


#include <gtk/gtk.h>
#include "fr-command.h"
#include "fr-process.h"


#define FR_COMMAND_ZIP_TYPE        fr_command_zip_get_type ()
#define FR_COMMAND_ZIP(o)          GTK_CHECK_CAST (o, FR_COMMAND_ZIP_TYPE, FRCommandZip)
#define FR_COMMAND_ZIP_CLASS(k)    GTK_CHECK_CLASS_CAST (k, FR_COMMAND_ZIP_TYPE, FRCommandZipClass)
#define IS_FR_COMMAND_ZIP(o)       GTK_CHECK_TYPE (o, FR_COMMAND_ZIP_TYPE)


typedef struct _FRCommandZip       FRCommandZip;
typedef struct _FRCommandZipClass  FRCommandZipClass;


struct _FRCommandZip
{
	FRCommand  __parent;
};


struct _FRCommandZipClass
{
	FRCommandClass __parent_class;
};


GtkType      fr_command_zip_get_type        (void);

FRCommand*   fr_command_zip_new             (FRProcess *process,
                                             const char *filename,
                                             FRArchive   *archive);


#endif /* FR_COMMAND_ZIP_H */
