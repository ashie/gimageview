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

#ifndef FR_COMMAND_RAR_H
#define FR_COMMAND_RAR_H


#include <gtk/gtk.h>
#include "fr-command.h"
#include "fr-process.h"
#include "gimv_image_info.h"


#define FR_COMMAND_RAR_TYPE        fr_command_rar_get_type ()
#define FR_COMMAND_RAR(o)          GTK_CHECK_CAST (o, FR_COMMAND_RAR_TYPE, FRCommandRar)
#define FR_COMMAND_RAR_CLASS(k)    GTK_CHECK_CLASS_CAST (k, FR_COMMAND_RAR_TYPE, FRCommandRarClass)
#define IS_FR_COMMAND_RAR(o)       GTK_CHECK_TYPE (o, FR_COMMAND_RAR_TYPE)


typedef struct _FRCommandRar       FRCommandRar;
typedef struct _FRCommandRarClass  FRCommandRarClass;


struct _FRCommandRar
{
	FRCommand  __parent;

	gboolean       list_started;
	gboolean       odd_line;
	GimvImageInfo *fdata;
};


struct _FRCommandRarClass
{
	FRCommandClass __parent_class;
};


GtkType      fr_command_rar_get_type (void);

FRCommand*   fr_command_rar_new      (FRProcess  *process,
                                      const char *filename,
                                      FRArchive  *archive);


#endif /* FR_COMMAND_RAR_H */
