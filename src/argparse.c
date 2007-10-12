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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gimageview.h"

#include "argparse.h"
#include "charset.h"
#include "prefs.h"

ArgsVal args_val;

static struct arg_opt options[] =
{
   /* file load */
   { "directory", 'd', NULL,    N_("Scan directory at start up")                            },
   { "recursive", 'R', NULL,    N_("Scan directory recursively (use with \"-d\")")          },
   { "scan-dot",  'D', NULL,    N_("Read dotfile when scanning directory (use with \"-d\")")},
   { "ignore-ext",'e', NULL,    N_("Ignore file name extension")                            },

   /* image window */
   { "scale",     's', "SCALE", N_("Specify image scale on image window [%]")               },
   { "buffer",    'b', "ON/OFF",N_("Keep original image on memory or not")                  },
   { "menubar",   'M', NULL,    N_("Show menu bar on image view window")                    },
   { "toolbar",   'T', NULL,    N_("Show tool bar on image view window")                    },

   /*  */
   { "imagewin",  'I', NULL,    N_("Open empty image window at start up")                   },
   { "thumbwin",  'w', NULL,    N_("Open thumbnail window at start up")                     },

   /* default window */
   { "imageview", 'i', NULL,    N_("Open all images in imageview window")                   },
   { "thumbview", 't', NULL,    N_("Open all images in thumbnail window")                   },
   { "slideshow", 'S', NULL,    N_("Open images files in slide show")                       },
   { "wait",      'W', "TIME",  N_("Interval of slideshow (use with \"-S\") [sec]")         },

   /* etc */
   { "version",   'v', NULL,    N_("Print version infomation")                              },
   { "help",      'h', NULL,    N_("Show this message")                                     },
   { NULL,         0,  NULL,    NULL,                                                       },
};


/*
 *  parse_opt:
 *
 *  key    : short option character.
 *  arg    : arguments.
 *  Return : state (enum)
 */
static arg_error_t
parse_opt (int key, char *arg)
{
   int value_bool;

   if (arg && (!g_strcasecmp (arg, "ON") || !g_strcasecmp (arg, "enable")))
      value_bool = TRUE;
   else
      value_bool = FALSE;

   switch (key) {
   case 'I':
      args_val.open_imagewin = TRUE;
      break;
   case 'w':
      args_val.open_thumbwin = TRUE;
      break;
   case 'd':
      args_val.read_dir = TRUE;
      break;
   case 'R':
      args_val.read_dir_recursive = TRUE;
      break;
   case 'D':
      args_val.read_dot = TRUE;
      conf.read_dotfile = TRUE;
      break;
   case 'e':
      args_val.ignore_ext = TRUE;
      conf.detect_filetype_by_ext = FALSE;
      break;
   case 's':
      conf.imgview_scale = atoi(arg);
      conf.imgview_default_zoom = 0;
      break;
   case 'b':
      conf.imgview_buffer = value_bool;
      break;
   case 'M':
      conf.imgwin_show_menubar = TRUE;
      break;
   case 'T':
      conf.imgwin_show_toolbar = TRUE;
      break;
   case 'i':
      conf.default_file_open_window = IMAGE_VIEW_WINDOW;
      conf.default_dir_open_window = IMAGE_VIEW_WINDOW;
      break;
   case 't':
      conf.default_file_open_window = THUMBNAIL_WINDOW;
      conf.default_dir_open_window = THUMBNAIL_WINDOW;
      break;
   case 'S':
      args_val.exec_slideshow = TRUE;
      break;
   case 'W':
      args_val.slideshow_interval = atof(arg);
      break;
   case 'v':
      arg_version ();
      break;
   case 'h':
      return ARG_HELP;
      break;
   default:
   {
      gchar *tmpstr;
      tmpstr = charset_internal_to_locale(_("Unknown option: \"-%s\"\n"));
      fprintf (stderr, tmpstr, arg);
      g_free (tmpstr);
      exit (1);
   }
   }
   return 0;
}


/*
 *  arg_version:
 *     @ Print program version info and exit.
 */
void
arg_version (void)
{
   gchar *tmpstr;
   printf ("%s %s\n", GIMV_PROG_NAME, VERSION);
   tmpstr = charset_internal_to_locale (
      _("Copyright (C) 2001 Takuro Ashie\n"
        "This is free software; see the source for copying conditions.  There is NO\n"
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"));
   printf (tmpstr);
   g_free (tmpstr);

   exit (0);
}


#define DOC_OFFSET 26

/*
 *  arg_help:
 *     @ Print help message.
 *
 *  argv   : arguments.
 *  stream : file stream for printing help message.
 */
void
arg_help (char *argv[], FILE *stream) {
   int i, j;
   char buf[BUF_SIZE], *tmpstr1, *tmpstr2;

   tmpstr1 = charset_internal_to_locale (
                _("Copyright (C) 2001 Takuro Ashie\n\n"
                  "Usage: %s [OPTION...] [Image Files...]\n\n"));
   fprintf (stream, tmpstr1, argv[0]);
   g_free (tmpstr1);

   for (i = 0; options[i].name; i++) {
      if (options[i].arg)
         g_snprintf (buf, BUF_SIZE, "  -%c, --%s=%s",
                     options[i].key, options[i].name, options[i].arg);
      else
         g_snprintf (buf, BUF_SIZE, "  -%c, --%s",
                     options[i].key, options[i].name);
      for (j = strlen (buf); j < DOC_OFFSET; j++)
         strncat (buf, " ", 1);
      tmpstr1 = charset_internal_to_locale (buf);
      tmpstr2 = charset_internal_to_locale (_(options[i].doc));
      fprintf (stream, "%s %s\n", tmpstr1, tmpstr2);
      g_free (tmpstr1);
      g_free (tmpstr2);
   }

   fprintf (stream, "\n");

   if (stream == stderr)
      exit (1);
   else
      exit (0);
}

#undef DOC_OFFSET


/*
 *  parse_long_opt:
 *     @ Convert long option to short it, and call parse_opt () if success
 *       converting.
 *
 *  argv   : long option string.
 *  Return : state (enum)
 */
static arg_error_t
parse_long_opt (char *argv) {
   int i, arglen, optlen;
   char *argval;
   arg_error_t retval;
   gchar *tmpstr;

   for (i = 0; options[i].name; i++) {
      arglen = strlen(argv);
      optlen = strlen(options[i].name);

      /*
        if (arglen != optlen)
        continue;
      */

      if (!strncmp (argv, options[i].name, optlen)) {
         if (options[i].arg == NULL) {
            retval = parse_opt (options[i].key, NULL);
            return retval;
         }
         if (argv[optlen] == '=') {           /* if arg with value */
            argval = argv + optlen + 1;
            if (!*argval) {
               return ARG_NO_VAL;
            } else {
               retval = parse_opt (options[i].key, argval);
               return retval;
            }
         } else                               /* error if no value */
            return ARG_ERR_PARSE;
      }
   }

   /* error */
   tmpstr = charset_internal_to_locale (_("Unknown option: \"--%s\"\n"));
   fprintf (stderr, tmpstr, argv);
   g_free (tmpstr);

   exit (1);
}


/*
 *  arg_parse:
 *     @ Command line argument parser.
 *
 *  argc      : argument num.
 *  argv      : arguments.
 *  remaining : Start element number of remaining argument
 *              (These arguments are not parsed by this parser).
 *              The first unknown string will be treated as start of remaining.
 */
void
arg_parse (int argc, char *argv[], int *remaining)
{
   int i, j, k, num;
   arg_error_t retval = ARG_ERR_UNKNOWN;
   gchar *tmpstr;

   args_val_init ();

   for (i = 1; i < argc; i++) {
      if (!strncmp (argv[i], "--", 2)) {            /* long option */
         retval = parse_long_opt (argv[i] + 2);
      } else if (!strncmp (argv[i], "-", 1)) {      /* short option */

         num = strlen (argv[i]) - 1;

         for (j = 1; j <= num; j++) {

            retval = ARG_ERR_UNKNOWN;

            for (k = 0; options[k].name; k++) {

               /* arg with value */
               if (options[k].key == (int)argv[i][j] && options[k].arg) {
                  if (j == num && i < argc - 1)
                     retval = parse_opt ((int)argv[i][j], argv[i+1]);
                  else
                     arg_help (argv, stderr);
                  i++;

                  /* arg with no value */
               } else if (options[k].key == (int)argv[i][j]) {
                  retval = parse_opt ((int)argv[i][j], NULL);
               }
            }

            /* unknown option */
            if (retval == ARG_ERR_UNKNOWN) {
               tmpstr = charset_internal_to_locale (_("Unknown option: \"-%c\"\n"));
               fprintf (stderr, tmpstr, argv[i][j]);
               g_free (tmpstr);
               exit (1);
            }
         }
      } else {
         break;
      }

      if (retval == ARG_HELP)
         arg_help (argv, stdout);
      else if (retval != ARG_NOERR && retval != ARG_WITH_VAL)
         arg_help (argv, stderr);
   }
   *remaining = i;
   return;
}


void
args_val_init (void)
{
   args_val.read_dir           = conf.startup_read_dir;
   args_val.read_dir_recursive = conf.scan_dir_recursive;
   args_val.read_dot           = conf.read_dotfile;
   args_val.ignore_ext         = !conf.detect_filetype_by_ext;
   args_val.open_imagewin      = FALSE;
   args_val.open_thumbwin      = conf.startup_open_thumbwin;
   args_val.exec_slideshow     = FALSE;
   args_val.slideshow_interval = conf.slideshow_interval;
}
