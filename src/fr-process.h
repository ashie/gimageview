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

#ifndef FR_PROCESS_H
#define FR_PROCESS_H

#include <glib.h>
#include <gtk/gtkobject.h>
#include <sys/types.h>

#define FR_TYPE_PROCESS            (fr_process_get_type ())
#define FR_PROCESS(obj)            (GTK_CHECK_CAST ((obj), FR_TYPE_PROCESS, FRProcess))
#define FR_PROCESS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), FR_TYPE_PROCESS, FRProcessClass))
#define FR_IS_PROCESS(obj)         (GTK_CHECK_TYPE ((obj), FR_TYPE_PROCESS))
#define FR_IS_PROCESS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), FR_TYPE_PROCESS))

#define FR_PROCCESS_BUFFER_SIZE 4096

typedef enum {
   FR_PROC_ERROR_NONE,
   FR_PROC_ERROR_GENERIC,
   FR_PROC_ERROR_COMMAND_NOT_FOUND,
   FR_PROC_ERROR_EXITED_ABNORMALLY,
   FR_PROC_ERROR_PIPE,
   FR_PROC_ERROR_FORK,
   FR_PROC_ERROR_CANNOT_CHDIR,
   FR_PROC_ERROR_STOPPED
} FRProcError;

typedef void (*ProcLineFunc) (char *line, gpointer data);

typedef struct _FRProcess       FRProcess;
typedef struct _FRProcessClass  FRProcessClass;

struct _FRProcess {
   GtkObject __parent;

   GPtrArray  *comm;
   GPtrArray  *dir;
   gint        n_comm;

   pid_t   command_pid;
   int     output_fd;

   int     log_timeout;
   char    buffer[FR_PROCCESS_BUFFER_SIZE + 1];
   int     not_processed;
   GList * row_output;

   ProcLineFunc proc_line_func;
   gpointer     proc_line_data;

   FRProcError  error;

   gboolean running;
   gboolean proc_stdout;
   gint current_command;	

   gboolean use_standard_locale;
};

struct _FRProcessClass {
   GtkObjectClass __parent_class;

   /* -- Signals -- */

   void (* start) (FRProcess *fr_proc);

   void (* done)  (FRProcess *fr_proc,
                   FRProcError error);
};

GtkType     fr_process_get_type             (void);
FRProcess * fr_process_new                  (void);
void        fr_process_clear                (FRProcess *fr_proc);
void        fr_process_begin_command        (FRProcess *fr_proc,
                                             char *arg);
void        fr_process_add_arg              (FRProcess *fr_proc,
                                             char *arg);
void        fr_process_end_command          (FRProcess *fr_proc);
void        fr_process_set_working_dir      (FRProcess *fr_proc,
                                             char *arg);
void        fr_process_set_proc_line_func   (FRProcess *fr_proc, 
                                             ProcLineFunc func,
                                             gpointer func_data);
void        fr_process_use_standard_locale  (FRProcess *fr_proc,
                                             gboolean use_stand_locale);

void        fr_process_start                (FRProcess *fr_proc, 
                                             gboolean proc_stdout);
void        fr_process_stop                 (FRProcess *fr_proc);


#endif /* FR_PROCESS_H */
