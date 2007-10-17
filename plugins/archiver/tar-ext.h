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

#ifndef FR_COMMAND_TAR_H
#define FR_COMMAND_TAR_H

#include <glib-object.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_TAR            (fr_command_tar_get_type ())
#define FR_COMMAND_TAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_TAR, FRCommandTar))
#define FR_COMMAND_TAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_TAR, FRCommandTarClass))
#define FR_IS_COMMAND_TAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_TAR))
#define FR_IS_COMMAND_TAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_TAR))
#define FR_COMMAND_TAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_TAR, FRCommandTarClass))

typedef struct _FRCommandTar       FRCommandTar;
typedef struct _FRCommandTarClass  FRCommandTarClass;

typedef enum {
   FR_COMPRESS_PROGRAM_NONE,
   FR_COMPRESS_PROGRAM_GZIP,
   FR_COMPRESS_PROGRAM_BZIP,
   FR_COMPRESS_PROGRAM_BZIP2,
   FR_COMPRESS_PROGRAM_COMPRESS,
   FR_COMPRESS_PROGRAM_LZO,
} FRCompressProgram;

struct _FRCommandTar
{
   FRCommand  __parent;
   FRCompressProgram compress_prog;
};

struct _FRCommandTarClass
{
   FRCommandClass __parent_class;
};

GType        fr_command_tar_get_type        (void);

FRCommand*   fr_command_tar_new             (FRProcess *process,
                                             const char *filename,
                                             FRArchive  *archive,
                                             FRCompressProgram prog);

#endif /* FR_COMMAND_TAR_H */
