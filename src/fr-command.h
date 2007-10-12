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

#ifndef FR_COMMAND_H
#define FR_COMMAND_H


#include <gtk/gtk.h>
#include "fr-process.h"


#define FR_COMMAND_TYPE        fr_command_get_type ()
#define FR_COMMAND(o)          GTK_CHECK_CAST (o, FR_COMMAND_TYPE, FRCommand)
#define FR_COMMAND_CLASS(k)    GTK_CHECK_CLASS_CAST (k, FR_COMMAND_TYPE, FRCommandClass)
#define IS_FR_COMMAND(o)       GTK_CHECK_TYPE (o, FR_COMMAND_TYPE)


typedef struct _FRCommand       FRCommand;
typedef struct _FRCommandClass  FRCommandClass;


typedef enum {
   FR_ACTION_LIST,
   FR_ACTION_ADD,
   FR_ACTION_DELETE,
   FR_ACTION_EXTRACT,
} FRAction;


struct _FRCommand
{
   GtkObject  __parent;
   GList *file_list; /* FileData elements */

   /*<protected>*/
   guint propAddCanUpdate : 1;
   guint propExtractCanAvoidOverwrite : 1;
   guint propExtractCanSkipOlder : 1;
   guint propExtractCanJunkPaths : 1;

   guint propHasRatio : 1;

   gpointer    archive;

   /*<private>*/
   FRProcess  *process;
   FRAction    action;
   char       *filename;
};


struct _FRCommandClass
{
   GtkObjectClass __parent_class;

   /*<virtual functions>*/

   void        (*list)         (FRCommand *comm);

   void        (*add)          (FRCommand *comm,
                                GList *file_list,
                                gchar *base_dir,
                                gboolean update); 

   void        (*delete)       (FRCommand *comm,
                                GList *file_list); 

   void        (*extract)      (FRCommand *comm,
                                GList *file_list,
                                char *dest_dir,
                                gboolean overwrite,
                                gboolean skip_older,
                                gboolean junk_paths);

   /*<signals>*/

   void        (*start)        (FRCommand *comm,
                                FRAction action); 

   void        (*done)         (FRCommand *comm,
                                FRAction action,
                                FRProcError error);
};


GtkType        fr_command_get_type           (void);
void           fr_command_construct          (FRCommand *comm,
                                              FRProcess *process,
                                              const char *filename);
void           fr_command_set_filename       (FRCommand *comm,
                                              const char *filename);
void           fr_command_list               (FRCommand *comm);
void           fr_command_add                (FRCommand *comm,
                                              GList *file_list,
                                              gchar *base_dir,
                                              gboolean update); 
void           fr_command_delete             (FRCommand *comm,
                                              GList *file_list); 
void           fr_command_extract            (FRCommand *comm,
                                              GList *file_list,
                                              char *dest_dir,
                                              gboolean overwrite,
                                              gboolean skip_older,
                                              gboolean junk_paths);


#endif /* FR_COMMAND_H */
