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

#ifndef __PREFS_WIN_H__
#define __PREFS_WIN_H__

#include "gimageview.h"

typedef enum
{
   GIMV_PREFS_WIN_ACTION_OK,
   GIMV_PREFS_WIN_ACTION_APPLY,
   GIMV_PREFS_WIN_ACTION_CANCEL
   /* GIMV_PREFS_WIN_ACTION_SET_DEFAULT, */
} GimvPrefsWinAction;

/* FIXME */
typedef GtkWidget *(*GimvPrefsWinCreatePageFn)  (void);
/* return true if want to stop applying */
typedef gboolean   (*GimvPrefsWinApplyFn)       (GimvPrefsWinAction action);

typedef struct GimvPrefsWinPage_Tag {
   const gchar * const     path;
   gint                    priority_hint;
   const gchar * const     icon;
   const gchar * const     icon_open;

   GimvPrefsWinCreatePageFn create_page_fn;
   GimvPrefsWinApplyFn      apply_fn;

} GimvPrefsWinPage;

GtkWidget *gimv_prefs_win_open           (const gchar *path,
                                          GtkWindow *parent);
void       gimv_prefs_win_open_idle      (const gchar *path,
                                          GtkWindow *window);
void       gimv_prefs_win_set_page       (const gchar *path);
gboolean   gimv_prefs_win_is_opened      (void);
GtkWidget *gimv_prefs_win_get            (void);
gboolean   gimv_prefs_win_add_page_entry (GimvPrefsWinPage *page);


#if 0

typedef struct PrefsWin_Tag      PrefsWin;
typedef struct PrefsWinClass_Tag PrefsWinClass;

typedef enum {
   PrefsWinModeNotebook,
   PrefsWinModeTree,
   PrefsWinModeStackButtons,
} PrefsWinMode;

struct PrefsWin_Tag
{
   GtkDialog parent;

   GtkWidget *nav_tree;
   GtkWieget *notebook;
};

struct PrefsWinClass_Tag
{
   GtkDialogClass parent_class;

   void (*ok)     (PrefsWin *window);
   void (*apply)  (PrefsWin *window);
   void (*cancel) (PrefsWin *window);
};


void       prefs_win_new                (PrefsWinMode       mode);
void       prefs_win_new_new_with_pages (PrefsWinMode       mode,
                                         PrefsWinPageEntry *page[],
                                         gint               n_pages);
void       prefs_win_show_modal         (PrefsWin          *window);
void       prefs_win_create_pages       (PrefsWin          *widnow,
                                         PrefsWinPageEntry *page[],
                                         gint               n_pages);
GtkWidget *prefs_win_append_page        (PrefsWin          *window,
                                         PrefsWinPageEntry *page);
void       prefs_win_remove_page        (const gchar *path);

#endif

#endif /* __PREFS_WIN_H__ */
