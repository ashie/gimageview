/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * Profile
 * copyright (c) 2002-2003 Kazuki IWAMOTO http://www.maid.org/ iwm@maid.org
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  2004-02-24 Takuro Ashie <ashie@homa.ne.jp>
 *      Adapt to GImageView.
 *
 *  2003-09-03 Takuro Ashie <ashie@homa.ne.jp>
 *      Added Kz and kz_ prefix.
 *      GObjectize.
 *      Added const keyword for value field of KzPrrofileList.
 *
 *  2003-08-28 Takuro Ashie <ashie@homa.ne.jp>
 *      Translated comments into English.
 *
 *  2003-08-27 Takuro Ashie <ashie@homa.ne.jp>
 *      Modified coding style.
 *      Changed interface of profile_open().
 */

#ifndef __GIMV_PREFS_H__
#define __GIMV_PREFS_H__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>

#define GIMV_TYPE_PREFS		        (gimv_prefs_get_type ())
#define GIMV_PREFS(obj)            (GTK_CHECK_CAST (obj, gimv_prefs_get_type (), GimvPrefs))
#define GIMV_PREFS_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, gimv_prefs_get_type, GimvPrefsClass))
#define GIMV_IS_PREFS(obj)         (GTK_CHECK_TYPE (obj, gimv_prefs_get_type ()))
#define GIMV_IS_PREFS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMV_TYPE_PREFS))


typedef struct GimvPrefs_Tag      GimvPrefs;
typedef struct GimvPrefsClass_Tag GimvPrefsClass;
typedef struct GimvPrefsList_Tag  GimvPrefsList;


#define GIMV_PREFS_DATA_TYPE_UNKNOW 0
#define GIMV_PREFS_DATA_TYPE_SPACE 1
#define GIMV_PREFS_DATA_TYPE_COMMENT 2
#define GIMV_PREFS_DATA_TYPE_SECTION 3
#define GIMV_PREFS_DATA_TYPE_KEY 4

#define GIMV_PREFS_VALUE_TYPE_BOOL 0
#define GIMV_PREFS_VALUE_TYPE_INT 1
#define GIMV_PREFS_VALUE_TYPE_STRING 2
#define GIMV_PREFS_VALUE_TYPE_ARRAY 3


struct GimvPrefs_Tag
{
   GtkObject parent;

   gboolean edit;
   gchar *file, *subfile;
   GimvPrefsList *list, *sublist;
};


struct GimvPrefsList_Tag
{
   gchar *data, *section, *key;
   const gchar *value;
   guint type;
   struct GimvPrefsList_Tag *prev, *next;
};


struct GimvPrefsClass_Tag
{
   GtkObjectClass parent_class;

	/* -- signals -- */
   void (*section_added)   (GimvPrefs *profile,
                            const gchar *section);
   void (*section_deleted) (GimvPrefs *profile,
                            const gchar *section);
   void (*key_added)       (GimvPrefs *profile,
                            const gchar *section,
                            const gchar *key);
   void (*key_deleted)     (GimvPrefs *profile,
                            const gchar *section,
                            const gchar *key);
   void (*changed)         (GimvPrefs *profile,
                            const gchar *section,
                            const gchar *key,
                            const gchar *old_value);
};


GtkType    gimv_prefs_get_type (void);
GimvPrefs *gimv_prefs_new      (void);


/*
 * Open the initialize file.
 * @file,file name
 * @RET,the GimvPrefs struct
 */
GimvPrefs *gimv_prefs_open (const gchar *file,
                            const gchar *subfile);

/*
 * Close the initialize file.
 * @profile,the GimvPrefs struct
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_close (GimvPrefs *profile);


/*
 * Save the initialize file.
 * @profile,the GimvPrefs struct
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_save (GimvPrefs *profile);


/*
 * Get a string from the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @key,name of the key
 * @RET,string,NULL:error
 */
gchar *gimv_prefs_get_string (GimvPrefs *profile,
                              const gchar *section,
                              const gchar *key);


/*
 * Get size of a value.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @key,name of the key
 * @type,value type
 * @RET,bytes,0:error
 */
gint gimv_prefs_get_size (GimvPrefs *profile,
                          const gchar *section,
                          const gchar *key,
                          const guint type);


/*
 * Get a value from the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @key,name of the key
 * @value,buffer to store value.
 * @size,size of buffer to store value.
 * @type,value type
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_get_value (GimvPrefs *profile,
                               const gchar *section,
                               const gchar *key,
                               gpointer value,
                               const gint size,
                               const guint type);


/*
 * Set a value into the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @key,name of the key
 * @value,buffer which store the value.
 * @size,size of buffer which store the value.
 * @type,value type
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_set_value (GimvPrefs *profile,
                               const gchar *section,
                               const gchar *key,
                               gconstpointer value,
                               const gint size,
                               const guint type);


/*
 * Delete a section from the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_delete_section (GimvPrefs *profile,
                                    const gchar *section);


/*
 * Delete a key from the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @key,key
 * @RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_delete_key (GimvPrefs *profile,
                                const gchar *section,
                                const gchar *key);


/*
 * Enumerate sections in the initialize file.
 * @profile,the GimvPrefs struct
 * @RET,list of sections,NULL:error
 */
GList *gimv_prefs_enum_section (GimvPrefs *profile);


/*
 * Enumelate keys in the initialize file.
 * @profile,the GimvPrefs struct
 * @section,name of the section
 * @RET,list of keys,NULL:error
*/
GList *gimv_prefs_enum_key (GimvPrefs *profile,
                            const gchar *section);

#endif /* __GIMV_PREFS_H__ */
