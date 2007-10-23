/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * gtk_prop.h
 *
 * Copyright (C) 1999 Rasca, Berlin
 * EMail: thron@gmx.de
 *
 * Olivier Fourdan (fourdan@xfce.org)
 * Heavily modified as part of the Xfce project (http://www.xfce.org)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#ifndef __GIMV_FILE_PROP_WIN_H__
#define __GIMV_FILE_PROP_WIN_H__

#include <sys/types.h>
#include <gtk/gtk.h>

#include "gimv_image_info.h"

/*
 *  flags
 */
enum {
   GTK_PROP_MULTI           = 1 << 0,
   GTK_PROP_STALE_LINK      = 1 << 1,
   GTK_PROP_EDITABLE        = 1 << 2,
   GTK_PROP_NOT_DETECT_TYPE = 1 << 3
};

#define DLG_OK         (1<<0)
#define DLG_CANCEL     (1<<1)
#define DLG_YES        (1<<2)
#define DLG_NO	       (1<<3)
#define DLG_CONTINUE   (1<<4)
#define DLG_CLOSE      (1<<5)
#define DLG_ALL	       (1<<6)
#define DLG_SKIP       (1<<7)
/* */
#define DLG_OK_CANCEL  (DLG_OK|DLG_CANCEL)
#define DLG_YES_NO     (DLG_YES|DLG_NO)
/* */
#define DLG_ENTRY_VIEW  (1<<8)
#define DLG_ENTRY_EDIT  (1<<9)
#define DLG_COMBO       (1<<10)
/* */
#define DLG_INFO	(1<<11)
#define DLG_WARN        (1<<12)
#define DLG_ERROR       (1<<13)
#define DLG_QUESTION	(1<<14)

#define DLG_RC_CANCEL	0
#define DLG_RC_OK	1
#define DLG_RC_ALL	2
#define DLG_RC_CONTINUE 3
#define DLG_RC_SKIP	4
#define DLG_RC_DESTROY  5

typedef struct
{
   mode_t mode;
   uid_t uid;
   gid_t gid;
   time_t ctime;
   time_t mtime;
   time_t atime;
   off_t size;
} GimvFileProp;

gboolean gimv_file_prop_win_run (GimvImageInfo *info,
                                 gint           flags);

#endif /* __GIMV_FILE_PROP_WIN_H__ */
