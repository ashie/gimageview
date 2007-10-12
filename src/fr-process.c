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

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "fr-process.h"
#include "gtk2-compat.h"


#define REFRESH_RATE 100


enum {
   START,
   DONE,
   LAST_SIGNAL
};

static GtkObjectClass *parent_class;
static guint fr_process_signals[LAST_SIGNAL] = { 0 };


static void
fr_process_destroy (GtkObject *object)
{
   FRProcess *fr_proc;

   g_return_if_fail (object != NULL);
   g_return_if_fail (FR_IS_PROCESS (object));
  
   fr_proc = FR_PROCESS (object);

   fr_process_stop (fr_proc);
   fr_process_clear (fr_proc);
   g_ptr_array_free (fr_proc->comm, FALSE);
   g_ptr_array_free (fr_proc->dir, FALSE);

   if (fr_proc->row_output != NULL) {
      g_list_foreach (fr_proc->row_output, (GFunc) g_free, NULL);
      g_list_free (fr_proc->row_output);
      fr_proc->row_output = NULL;
   }

   /* Chain up */
   if (GTK_OBJECT_CLASS (parent_class)->destroy)
      (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
fr_process_class_init (FRProcessClass *class)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass *) class;
   parent_class = gtk_type_class (gtk_object_get_type ());

   fr_process_signals[START] =
      gtk_signal_new ("start",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRProcessClass, start),
                      gtk_marshal_NONE__NONE,
                      GTK_TYPE_NONE, 0);
   fr_process_signals[DONE] =
      gtk_signal_new ("done",
                      GTK_RUN_LAST,
                      GTK_CLASS_TYPE (object_class),
                      GTK_SIGNAL_OFFSET (FRProcessClass, done),
                      gtk_marshal_NONE__INT,
                      GTK_TYPE_NONE, 1,
                      GTK_TYPE_INT);
   gtk_object_class_add_signals (object_class, fr_process_signals, 
                                 LAST_SIGNAL);

   object_class->destroy = fr_process_destroy;
   class->done = NULL;
}


static void
fr_process_init (FRProcess *fr_proc)
{
   fr_proc->comm = g_ptr_array_new ();
   fr_proc->dir = g_ptr_array_new ();
   fr_proc->n_comm = -1;

   fr_proc->command_pid = 0;
   fr_proc->output_fd = 0;

   fr_proc->log_timeout = -1;
   fr_proc->not_processed = 0;
   fr_proc->row_output = NULL;

   fr_proc->proc_line_func = NULL;
   fr_proc->proc_line_data = NULL;

   fr_proc->running = FALSE;

   fr_proc->use_standard_locale = TRUE;

   gtk_object_ref (GTK_OBJECT (fr_proc));
   gtk_object_sink (GTK_OBJECT (fr_proc));
}


GtkType
fr_process_get_type (void)
{
   static GtkType fr_process_type = 0;

   if (! fr_process_type) {
      GtkTypeInfo fr_process_info = {
         "FRProcess",
         sizeof (FRProcess),
         sizeof (FRProcessClass),
         (GtkClassInitFunc) fr_process_class_init,
         (GtkObjectInitFunc) fr_process_init,
         NULL, /* reserved_1 */
         NULL, /* reserved_2 */
         (GtkClassInitFunc) NULL
      };

      fr_process_type = gtk_type_unique (gtk_object_get_type (), 
                                         &fr_process_info);
   }

   return fr_process_type;
}


FRProcess * 
fr_process_new (void)
{
   FRProcess *fr_proc;
   fr_proc = FR_PROCESS (gtk_type_new (fr_process_get_type ()));
   return fr_proc;
}


void
fr_process_begin_command (FRProcess *fr_proc, 
                          char *arg)
{
   g_return_if_fail (fr_proc != NULL);

   fr_proc->n_comm++;
   g_ptr_array_add (fr_proc->comm, g_list_prepend (NULL, g_strdup (arg)));
   g_ptr_array_add (fr_proc->dir, NULL);
}


void
fr_process_set_working_dir (FRProcess *fr_proc, 
                            char *dir)
{
   g_return_if_fail (fr_proc != NULL);
   g_ptr_array_index (fr_proc->dir, fr_proc->n_comm) = g_strdup (dir);
}


void
fr_process_add_arg (FRProcess *fr_proc, 
                    char *arg)
{
   g_return_if_fail (fr_proc != NULL);
   g_ptr_array_index (fr_proc->comm, fr_proc->n_comm)
      = g_list_prepend (g_ptr_array_index (fr_proc->comm, fr_proc->n_comm),
                        g_strdup (arg));
}


void
fr_process_end_command (FRProcess *fr_proc)
{
   g_return_if_fail (fr_proc != NULL);
   g_ptr_array_index (fr_proc->comm, fr_proc->n_comm)
      = g_list_reverse (g_ptr_array_index (fr_proc->comm, fr_proc->n_comm));
}


void
fr_process_clear (FRProcess *fr_proc)
{
   gint i;

   g_return_if_fail (fr_proc != NULL);

   for (i = 0; i <= fr_proc->n_comm; i++) {
      /* free command. */
      g_list_foreach (g_ptr_array_index (fr_proc->comm, i), (GFunc) g_free, NULL);
      g_list_free (g_ptr_array_index (fr_proc->comm, i));
      g_ptr_array_index (fr_proc->comm, i) = NULL;

      /* free working directory. */
      if (g_ptr_array_index (fr_proc->dir, i) != NULL)
         g_free (g_ptr_array_index (fr_proc->dir, i));
      g_ptr_array_index (fr_proc->dir, i) = NULL;
   }

   for (i = 0; i <= fr_proc->n_comm; i++) {
      g_ptr_array_remove_index (fr_proc->comm, 0);
      g_ptr_array_remove_index (fr_proc->dir, 0);
   }

   fr_proc->n_comm = -1;
}


void
fr_process_set_proc_line_func (FRProcess *fr_proc, 
                               ProcLineFunc func,
                               gpointer data)
{
   g_return_if_fail (fr_proc != NULL);
   fr_proc->proc_line_func = func;
   fr_proc->proc_line_data = data;
}


static gboolean
process_output (FRProcess *fr_proc)
{
   int n, i;
   char *line, *eol;

   n = read (fr_proc->output_fd, 
             fr_proc->buffer + fr_proc->not_processed, 
             FR_PROCCESS_BUFFER_SIZE - fr_proc->not_processed);

   if (n <= 0)
      return FALSE;

   fr_proc->buffer[fr_proc->not_processed + n] = 0;

   line = fr_proc->buffer;
   while ((eol = strchr (line, '\n')) != NULL) {
      *eol = 0;

      fr_proc->row_output = g_list_prepend (fr_proc->row_output,
                                            g_strdup (line));

      if (fr_proc->proc_stdout && (fr_proc->proc_line_func != NULL))
         (*fr_proc->proc_line_func) (line, 
                                     fr_proc->proc_line_data);
      line = eol + 1;
   }
	
   /* shift unprocessed text to the beginning. */

   fr_proc->not_processed = strlen (line);
   for (i = 0; *line != 0; line++, i++)
      fr_proc->buffer[i] = *line;

   return TRUE;
}


static gint check_child (gpointer data);


static void
start_current_command (FRProcess *fr_proc)
{
   pid_t pid;
   int pipe_fd[2];

   if (pipe (pipe_fd) < 0) {
      fr_proc->error = FR_PROC_ERROR_PIPE;
      gtk_signal_emit (GTK_OBJECT (fr_proc), 
                       fr_process_signals[DONE],
                       fr_proc->error);
      return;
   }

   pid = fork ();

   if (pid < 0) {
      close (pipe_fd[0]);
      close (pipe_fd[1]);

      fr_proc->error = FR_PROC_ERROR_FORK;
      gtk_signal_emit (GTK_OBJECT (fr_proc), 
                       fr_process_signals[DONE],
                       fr_proc->error);

      return;
   }

   if (pid == 0) {
      GList *arg_list, *scan;
      char *command;
      char *dir;
      char ** argv;
      int i = 0;

      arg_list = g_ptr_array_index (fr_proc->comm, fr_proc->current_command);
      dir = g_ptr_array_index (fr_proc->dir, fr_proc->current_command);
      command = (char *) arg_list->data;

      if (dir != NULL) {
#ifdef DEBUG
         g_print ("cd %s\n", dir); 
#endif
         if (chdir (dir) != 0)
            _exit (FR_PROC_ERROR_CANNOT_CHDIR);
      }

#ifdef DEBUG
      g_print ("%s", command); 
#endif

      argv = g_new (char *, g_list_length (arg_list) + 1);
      for (scan = arg_list; scan; scan = scan->next) {
         argv[i++] = scan->data;

#ifdef DEBUG
         g_print (" %s", (char*) scan->data);
#endif
      }
      argv[i] = NULL;

#ifdef DEBUG
      g_print ("\n"); 
#endif

      close (pipe_fd[0]);
      dup2 (pipe_fd[1], STDOUT_FILENO);
      /* dup2 (pipe_fd[1], STDERR_FILENO); FIXME */	

      if (fr_proc->use_standard_locale)
         putenv ("LC_ALL=C");

      execvp (command, argv);

      _exit (255);
   }

   close (pipe_fd[1]);

   fr_proc->command_pid = pid;
   fr_proc->output_fd = pipe_fd[0];
   fcntl (fr_proc->output_fd, F_SETFL, O_NONBLOCK);

   fr_proc->not_processed = 0;
   fr_proc->log_timeout = gtk_timeout_add (REFRESH_RATE, 
                                           check_child,
                                           fr_proc);
}


static gint
check_child (gpointer data)
{
   FRProcess *fr_proc = data;
   pid_t pid;
   int status;
	
   process_output (fr_proc);

   pid = waitpid (fr_proc->command_pid, &status, WNOHANG);
   if (pid != fr_proc->command_pid) 
      return TRUE;

   if (WIFEXITED (status)) {
      if (WEXITSTATUS (status) == 0)
         fr_proc->error = FR_PROC_ERROR_NONE;
      else if (WEXITSTATUS (status) == 255) 
         fr_proc->error = FR_PROC_ERROR_COMMAND_NOT_FOUND;
      else if (WEXITSTATUS (status) == FR_PROC_ERROR_CANNOT_CHDIR)
         fr_proc->error = FR_PROC_ERROR_CANNOT_CHDIR;
      else
         fr_proc->error = FR_PROC_ERROR_GENERIC;
   } else
      fr_proc->error = FR_PROC_ERROR_EXITED_ABNORMALLY;

   /* Remove check. */

   gtk_timeout_remove (fr_proc->log_timeout);
   fr_proc->log_timeout = -1;
   fr_proc->command_pid = 0;

   /* Read all pending output. */

   if (fr_proc->error == FR_PROC_ERROR_NONE)
      while (process_output (fr_proc)) ;

   close (fr_proc->output_fd);
   fr_proc->output_fd = 0;

   /* Execute next command. */

   if ((fr_proc->error == FR_PROC_ERROR_NONE)
       && (fr_proc->current_command < fr_proc->n_comm)) {
      fr_proc->current_command++;
      start_current_command (fr_proc);
      return FALSE;
   }
	
   fr_proc->current_command = -1;

   if (fr_proc->row_output != NULL)
      fr_proc->row_output = g_list_reverse (fr_proc->row_output);

   fr_proc->running = FALSE;

   gtk_signal_emit (GTK_OBJECT (fr_proc), 
                    fr_process_signals[DONE],
                    fr_proc->error);

   return FALSE;
}


void
fr_process_use_standard_locale (FRProcess *fr_proc,
                                gboolean use_stand_locale)
{
   g_return_if_fail (fr_proc != NULL);
   fr_proc->use_standard_locale = use_stand_locale;
}


void
fr_process_start (FRProcess *fr_proc,
                  gboolean proc_stdout)
{
   g_return_if_fail (fr_proc != NULL);
   g_return_if_fail (fr_proc->n_comm != -1);

   if (fr_proc->running)
      return;

   fr_proc->proc_stdout = proc_stdout;

   if (fr_proc->row_output != NULL) {
      g_list_foreach (fr_proc->row_output, (GFunc) g_free, NULL);
      g_list_free (fr_proc->row_output);
      fr_proc->row_output = NULL;
   }

   gtk_signal_emit (GTK_OBJECT (fr_proc), 
                    fr_process_signals[START]);

   fr_proc->current_command = 0;
   start_current_command (fr_proc);

   fr_proc->running = TRUE;
}


void
fr_process_stop (FRProcess *fr_proc)
{
   g_return_if_fail (fr_proc != NULL);

   if (! fr_proc->running)
      return;

   if (fr_proc->log_timeout != -1) {
      gtk_timeout_remove (fr_proc->log_timeout);
      fr_proc->log_timeout = -1;
   }

   kill (fr_proc->command_pid, SIGKILL);
   waitpid (fr_proc->command_pid, NULL, WUNTRACED);
   fr_proc->command_pid = 0;
   close (fr_proc->output_fd);
   fr_proc->output_fd = 0;

   fr_proc->running = FALSE;

   fr_proc->error = FR_PROC_ERROR_STOPPED;
   gtk_signal_emit (GTK_OBJECT (fr_proc), 
                    fr_process_signals[DONE],
                    fr_proc->error);
}
