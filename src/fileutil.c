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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <glib.h>

#include "fileutil.h"

#ifndef FALSE
#  define FALSE   (0)
#endif

#ifndef TRUE
#  define TRUE    (!FALSE)
#endif

#ifndef BUF_SIZE
#  define BUF_SIZE 4096
#endif

#ifndef MAX_PATH_LEN
#  define MAX_PATH_LEN 1024
#endif


/*
 *  file_exists:
 *     @ check whether path is exist.
 *
 *  path   : path to check.
 *  Return : return TRUE if exist, and return FALSE if not exist.
 */
int
file_exists (const char *path)
{
   struct stat st;

   if ((!path) || (!*path))
      return FALSE;
   if (stat(path, &st) < 0)
      return FALSE;
   return TRUE;
}


/*
 *  isfile:
 *     @ check whether path is file.
 *
 *  path   : path to check.
 *  Return : return TRUE if path is file, and return FALSE if not.
 */
int
isfile (const char *path)
{
   struct stat st;

   if ((!path) || (!*path))
      return FALSE;
   if (stat(path, &st) < 0)
      return FALSE;
   if (S_ISREG(st.st_mode))
      return TRUE;
   return FALSE;
}


/*
 *  isdir:
 *     @ check whether path is directory.
 *
 *  path   : path to check.
 *  Return : return TRUE if path is directory, and return FALSE if not.
 */
int
isdir (const char *path)
{
   struct stat st;

   if ((!path) || (!*path))
      return FALSE;
   if (stat(path, &st) < 0)
      return FALSE;
   if (S_ISDIR(st.st_mode))
      return TRUE;
   return FALSE;
}


/*
 *  islink:
 *     @ check whether path is link.
 *
 *  path   : path to check.
 *  Return : return 1 if path is link, and return 0 if not.
 */
int
islink(const char *path)
{
   struct stat st;

   if (lstat(path, &st) < 0)
      return FALSE;
   if (S_ISLNK(st.st_mode))
      return TRUE;
   return FALSE;
}


/*
 *  iswritable:
 *     @ check whether path is writable.
 *
 *  path   : path to check.
 *  Return : return TRUE if path is writable, and return FALSE if not.
 */
int
iswritable (const char *path)
{
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct stat st;

   if (stat(path, &st))
      return FALSE;

   if (!S_ISDIR (st.st_mode))
      return FALSE;

   if(uid == st.st_uid && (st.st_mode & S_IWUSR))
      return TRUE;
   else if (gid == st.st_gid && (st.st_mode & S_IWGRP))
      return TRUE;
   else if (st.st_mode & S_IWOTH)
      return TRUE;

   return FALSE;
}


int
isexecutable (const char *path)
{
   struct stat st;

   if (stat(path, &st))
      return FALSE;

   if (st.st_mode & S_IXUSR
       || st.st_mode & S_IXGRP
       || st.st_mode & S_IXOTH)
   {
      return TRUE;
   }

   return FALSE;
}


/*
 *  Return: negative value if it is not directory.
 *          0 if has no sub directory.
            positive value if has sub directory.
 */
int
has_subdirs (const char *path)
{
   struct stat st;

   if ((!path) || (!*path))
      return -1;
   if (stat(path, &st) < 0)
      return -1;
   if (!S_ISDIR(st.st_mode))
      return -1;

   return st.st_nlink - 2;
}


/*
 *  fileutil_makedir:
 *     @ make one directory.
 *
 *  dir    : directory to make.
 *  Return : return TURE if success, and return FALSE if fail.
 */
int
makedir (const char *dir)
{
   if ((!dir) || (!*dir))
      return FALSE;
   if (!mkdir(dir, S_IRWXU))
      return TRUE;
   return FALSE;
}


/*
 *  mkdirs:
 *     @ make directory recursively. similar to "mkdir -p" shell command.
 *     @ The path string until last "/" character will be intented as directory.
 *
 *  path   : src path.
 *  Return : return TRUE if success, and return FALSE if fail.
 */
int
mkdirs (const char *path)
{
   char ss[MAX_PATH_LEN];
   int  i, ii;

   i = 0;
   ii = 0;
   while (path[i] && i < MAX_PATH_LEN) {
      ss[ii++] = path[i];
      ss[ii] = '\0';
      if (i + 1 < MAX_PATH_LEN && path[i + 1] == '/') {
         if (!file_exists(ss)) {
            if (!makedir(ss))
               return FALSE;
         } else if (!isdir(ss))
            return FALSE;
      }
      i++;
   }
   return TRUE;
}


int
ensure_dir_exists (const char *a_path)
{
   if (!a_path) return FALSE;

   if (!isdir (a_path)) {
      char *path = g_strdup (a_path);
      char *p = path;

      while (*p != '\0') {
         p++;
         if ((*p == '/') || (*p == '\0')) {
            int end = TRUE;

            if (*p != '\0') {
               *p = '\0';
               end = FALSE;
            }

            if (!isdir (path)) {
               if (mkdir (path, 0755) < 0) {
                  g_free (path);
                  return FALSE;
               }
            }
            if (!end) *p = '/';
         }
      }
      g_free (path);
   }

   return TRUE;
}


int
get_file_mtime (const char *path)
{
   struct stat st;

   if (! path || ! *path) return 0; 

   if (stat(path, &st))
      return 0;

   return st.st_mtime;
}


/*
 *  fileutile_size2str:
 *     @ add comma.
 *  size   :
 *  space  :
 *  Return :
 */
char *
fileutil_size2str (size_t size, int space)
{
   unsigned int i = 0,  j = 0, n_digit = 0;
   char tmp[14];
   char comma[14];
   char buf[14];

   i = size;

   /* detect digit num */
   while (i > 0) {
      i = i/10;
      n_digit++;
   }

   sprintf (tmp, "%d", size);

   if (strlen (tmp) < 4)
      return g_strdup (tmp);

   /* until first comma */
   if (n_digit % 3 != 0) {
      for (i = 0; i < n_digit % 3; i++)
         comma[j++] = tmp[i];
      if (i != strlen (tmp))
         comma[j++] = ',';
   }

   /* until end of string */
   while (tmp[i] != '\0'){
      comma[j++] = tmp[i++];
      comma[j++] = tmp[i++];
      comma[j++] = tmp[i++];
      if(tmp[i] != '\0')
         comma[j++] = ',';
   }

   /* end of string */
   comma[j] = '\0';

   if (space) {
      g_snprintf (buf, 14, "%13s", comma);
      return g_strdup (buf);
   } else {
      return g_strdup(comma);
   }
}


/*
 *  fileutile_time2str:
 *  time   :
 *  Return :
 */
char *
fileutil_time2str (time_t time)
{
   struct tm *jst = localtime (&time);
   char *week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
   char timestamp[256];

   g_snprintf (timestamp, 256, "%4d/%02d/%02d %s %02d:%02d",
               jst->tm_year+1900, jst->tm_mon+1, jst->tm_mday,
               week[jst->tm_wday], jst->tm_hour, jst->tm_min);

   return g_strdup (timestamp);
}


/*
 *  fileutile_uid2str:
 *  uid    :
 *  Return :
 */
char *
fileutil_uid2str (uid_t uid)
{
   struct passwd *pw = getpwuid (uid);
   char buf[16];

   if (pw) {
      return g_strdup (pw->pw_name);
   } else {
      g_snprintf (buf, 16, "%d", uid);
      return g_strdup (buf);
   }
}


/*
 *  fileutile_gid2str:
 *  gid    :
 *  Return :
 */
char *
fileutil_gid2str (gid_t gid)
{
   struct group *gr = getgrgid (gid);
   char buf[16];

   if (gr) {
      return g_strdup (gr->gr_name);
   } else {
      g_snprintf (buf, 16, "%d", gid);
      return g_strdup (buf);
   }
}


/*
 *  fileutile_mode2str:
 *  mode   :
 *  Return :
 */
char *
fileutil_mode2str (mode_t mode)
{
   char permission[11] = {"----------"};

   switch (mode & S_IFMT){
   case S_IFREG:
      permission[0] = '-';
      break;
   case S_IFLNK:
      permission[0] = 'l';
      break;
   case S_IFDIR:
      permission[0] = 'd';
      break;
   default:
      permission[0] = '?';
      break;
   }

   if (mode & S_IRUSR)
      permission[1] = 'r';
   if (mode & S_IWUSR)
      permission[2] = 'w';
   if (mode & S_IXUSR)
      permission[3] = 'x';

   if (mode & S_IRGRP)
      permission[4] = 'r';
   if (mode & S_IWGRP)
      permission[5] = 'w';
   if (mode & S_IXGRP)
      permission[6] = 'x';

   if (mode & S_IROTH)
      permission[7] = 'r';
   if (mode & S_IWOTH)
      permission[8] = 'w';
   if (mode & S_IXOTH)
      permission[9] = 'x';

   if (mode & S_ISUID)
      permission[3] = 'S';
   if (mode & S_ISGID)
      permission[6] = 'S';
   if (mode & S_ISVTX)
      permission[9] = 'T';

   permission[11] = 0;

   return g_strdup (permission);
}


/*
 *  fileutil_home2tilde:
 *     @ If path string include HOME DIR, convert it to "~/".
 *     @ The returned string should be freed when no longer needed.
 *
 *  path   : src path.
 *  Return : Short path.
 *           @ If HOME DIR is not included in path string , return src path.
 */
char *
fileutil_home2tilde (const char *path)
{
   char *home = NULL;
   char  buf[MAX_PATH_LEN];
   char *retval;
   size_t len;

   home = getenv("HOME");

   len = strlen (home);
   if (strlen (path) > len
       && !strncmp (path, home, len)
       && (path[len] == '/' || path[len] == '\0'))
   {
      g_snprintf (buf, BUF_SIZE, "~/%s", path + len + 1);
      retval = g_strdup (buf);
   } else {
      retval = g_strdup (path);
   }

   return retval;
}


/*
 *  fileutil_dir_basename:
 *     @ Return directory name (strip parent direcory name).
 *     @ The returned string should be freed when no longer needed.
 *
 *  path   : src path.
 *  Return : Base directory name.
 */
char *
fileutil_dir_basename (const char *path)
{
   char *basename = NULL;
   char *tmpstr = NULL, *endchr = NULL;
   char *retval;

   if (!path)
      return NULL;

   tmpstr = g_strdup (path);
   endchr = strrchr(tmpstr, '/');

   if (endchr && endchr + 1) {
      basename = endchr + 1;
   } else if (endchr) {
      *endchr = '\0';
      endchr = strrchr(tmpstr, '/');
      if (endchr && endchr + 1)
         basename = endchr + 1;
   } else {
      return NULL;
   }

   retval = g_strdup (basename);
   g_free (tmpstr);

   return retval;
}


char *
fileutil_get_extention (const char *filename)
{
   char *ext;

   if (!filename)
      return NULL;

   ext = strrchr(filename, '.');
   if (ext)
      ext = ext + 1;
   else
      return NULL;

   if (ext == "\0")
      return NULL;
   else
      return ext;
}


int
fileutil_extension_is (const char *filename, const char *ext)
{
   int len1, len2;

   if (!filename) return FALSE;
   if (!*filename) return FALSE;
   if (!ext) return FALSE;
   if (!*ext) return FALSE;

   len1 = strlen (filename);
   len2 = strlen (ext);

   if (len1 < len2) return FALSE;

   return !g_strcasecmp (filename + len1 - len2, ext);
}
