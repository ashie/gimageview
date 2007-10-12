/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 *  Profile
 *  copyright (c) 2002-2003 Kazuki IWAMOTO http://www.maid.org/ iwm@maid.org

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  2004-02-24 Takuro Ashie <ashie@homa.ne.jp>
 *      Adapt to GImageView.
 *
 *  2003-10-26 Takuro Ashie <ashie@homa.ne.jp>
 *      Copy section and key string before delete it.
 *
 *  2003-09-03 Takuro Ashie <ashie@homa.ne.jp>
 *      Added Kz and kz_ prefix.
 *      GObjectize.
 *      Remove also space befor a section when delete it.
 *      Added const keyword for value field of KzPrrofileList.
 *
 *  2003-08-28 Takuro Ashie <ashie@homa.ne.jp>
 *      Translated comments into English.
 *
 *  2003-08-27 Takuro Ashie <ashie@homa.ne.jp>
 *      Aggregated related codes into this file.
 *      Modified coding style.
 *      Simplize profile_open().
 */

#include "gimv_prefs.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>


enum {
   SECTION_ADDED_SIGNAL,
   SECTION_DELETED_SIGNAL,
   KEY_ADDED_SIGNAL,
   KEY_DELETED_SIGNAL,
   CHANGED_SIGNAL,
   LAST_SIGNAL
};


#define g_strlen(s) ((s)!=NULL?strlen(s):0)
#define g_strcmp(s1,s2) ((s1)!=NULL && (s2)!=NULL?strcmp(s1,s2):0)


static void gimv_prefs_class_init (GimvPrefsClass *klass);
static void gimv_prefs_init       (GimvPrefs      *profile);
static void gimv_prefs_destroy    (GtkObject      *object);


static GtkObjectClass *parent_class = NULL;

static gint gimv_prefs_signals[LAST_SIGNAL] = {0};


GtkType
gimv_prefs_get_type (void)
{
   static GtkType gimv_prefs_type = 0;

   if (!gimv_prefs_type) {
      static const GtkTypeInfo gimv_prefs_info = {
         "GimvPrefs",
         sizeof (GimvPrefs),
         sizeof (GimvPrefsClass),
         (GtkClassInitFunc) gimv_prefs_class_init,
         (GtkObjectInitFunc) gimv_prefs_init,
         NULL,
         NULL,
         (GtkClassInitFunc) NULL,
      };

      gimv_prefs_type = gtk_type_unique (gtk_object_get_type (),
					 &gimv_prefs_info);
   }

   return gimv_prefs_type;
}


static void
gimv_prefs_class_init (GimvPrefsClass *klass)
{
   GtkObjectClass *object_class;

   parent_class = gtk_type_class (gtk_object_get_type ());
   object_class = (GtkObjectClass *) klass;

   object_class->destroy = gimv_prefs_destroy;

   klass->section_added   = NULL;
   klass->section_deleted = NULL;
   klass->key_added       = NULL;
   klass->key_deleted     = NULL;
   klass->changed         = NULL;

	gimv_prefs_signals[SECTION_ADDED_SIGNAL]
		= gtk_signal_new ("section-added",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvPrefsClass, section_added),
                        gtk_marshal_NONE__STRING,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_STRING);

	gimv_prefs_signals[SECTION_DELETED_SIGNAL]
		= gtk_signal_new ("section-deleted",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvPrefsClass, section_deleted),
                        gtk_marshal_NONE__STRING,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_STRING);

	gimv_prefs_signals[KEY_ADDED_SIGNAL]
		= gtk_signal_new ("key-added",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvPrefsClass, key_added),
                        gtk_marshal_NONE__POINTER_POINTER,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_STRING, GTK_TYPE_STRING);

	gimv_prefs_signals[KEY_DELETED_SIGNAL]
		= gtk_signal_new ("key-deleted",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvPrefsClass, key_deleted),
                        gtk_marshal_NONE__POINTER_POINTER,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_STRING, GTK_TYPE_STRING);

	gimv_prefs_signals[CHANGED_SIGNAL]
		= gtk_signal_new ("changed",
                        GTK_RUN_FIRST,
                        GTK_CLASS_TYPE (object_class),
                        GTK_SIGNAL_OFFSET (GimvPrefsClass, changed),
                        gtk_marshal_NONE__POINTER_POINTER_POINTER,
                        GTK_TYPE_NONE, 3,
                        GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING);
}


static void
gimv_prefs_init (GimvPrefs *profile)
{
	profile->edit    = FALSE;
	profile->file    = NULL;
	profile->subfile = NULL;
	profile->list    = NULL;
	profile->sublist = NULL;
}


static void
gimv_prefs_destroy (GtkObject *object)
{
	GimvPrefs *profile = GIMV_PREFS(object);
	GimvPrefsList *p, *q;

	if (profile->file)
	{
		g_free(profile->file);
		profile->file = NULL;
	}

	if (profile->subfile)
	{
		g_free(profile->subfile);
		profile->subfile = NULL;
	}

	for (p = profile->list; p; p = q)
	{
		q = p->next;
		g_free(p->data);
		g_free(p->section);
		g_free(p->key);
		g_free(p);
	}
	profile->list = NULL;

	for (p = profile->sublist; p; p = q)
	{
		q = p->next;
		g_free(p->data);
		g_free(p->section);
		g_free(p->key);
		g_free(p);
	}
	profile->sublist = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		GTK_OBJECT_CLASS (parent_class)->destroy(object);
}


GimvPrefs *
gimv_prefs_new (void)
{
	GimvPrefs *profile = GIMV_PREFS(gtk_object_new(GIMV_TYPE_PREFS, NULL));
	return profile;
}


/******************************************************************************
*                                                                             *
* Converting between numerical value and string functions                     *
*                                                                             *
******************************************************************************/
const static gchar hex[16]={'0', '1', '2', '3', '4', '5', '6', '7',
			    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/*
 * string -> value
 * value,value
 * str,string
 * radix,radix
 * flag,TRUE:signed,FALSE:unsigned
 * RET,TRUE:normal exit,FALSE:error
 */
static gboolean
misc_str_to_val(gint *value,const gchar *str,const gint radix,
		const gboolean flag)
{
	gchar c;
	gint i,j,t;

	*value = 0;
	for (i = 0; str[i] != '\0' && str[i] == ' '; i++);
	if (str[i] == '\0')
		return FALSE;
	if (flag && str[i] == '-')
	{
		i++;
		while (str[i] != '\0')
		{
			t = *value;
			*value *= radix;
			c = toupper(str[i]);
			for (j = 0; j < radix; j++)
				if (hex[j] == c)
					break;
			*value += j;
			if (j == radix || *value < t)
			{
				*value = t;
				return FALSE;
			}
			i++;
		}
		if (*value < 0)
		{
			*value = 0;
			return FALSE;
		}
		*value =- *value;
	}
	else
	{
		while (str[i] != '\0')
		{
			t = *value;
			*value *= radix;
			c = toupper(str[i]);
			for (j = 0; j < radix; j++)
				if (hex[j] == c)
					break;
			*value += j;
			if (j == radix || *value < t)
			{
				*value = t;
				return FALSE;
			}
			i++;
		}
	}
	return TRUE;
}


/*
 * string -> array of value
 * size,bytes
 * text,string
 * radix,radix
 * flag,TRUE:signed,FALSE:unsigned
 * RET,array of value
 */
static guint8 *
misc_str_to_array(gint *size,const gchar *text,
		  const gint radix,const gboolean flag)
{
	gchar *p;
	guint8 *array = NULL;
	gint i, j, value;

	*size = 0;
	if (!text)
		return NULL;
	p = g_malloc((g_strlen(text) + 2) * sizeof(gchar));
	strcpy(p, text);
	for (i = 0; p[i] != '\0'; i++)
	{
		for (j = 0; j < radix; j++)
			if (p[i] == hex[j])
				break;
		if (j >= radix)
			p[i] = '\0';
	}
	p[i+1] = '\0';
	i = 0;
	while (p[i] != '\0')
	{
		if (!misc_str_to_val(&value, p+i, radix, flag))
		{
			g_free(p);
			g_free(array);
			*size = 0;
			return NULL;
		}
		array = g_realloc(array, *size + 1);
		array[(*size)++] = value;
		i += g_strlen(p + i) + 1;
	}
	g_free(p);
	return array;
}

/******************************************************************************
*                                                                             *
* Initialize file related functions                                           *
*                                                                             *
******************************************************************************/
/*
 * listize the file.
 * file,file name
 * RET,list
 */
static GimvPrefsList *
gimv_prefs_list (const gchar *file)
{
	gchar buf[256], *data = NULL, *tmp, *section = NULL;
	gint n;
	FILE *fio;
	GimvPrefsList *p = NULL, *q, *r;

	fio = fopen(file, "rt");
	if (!fio)
		return NULL;
	while (fgets (buf, 256, fio))
	{
		if (!data)
		{
			data = g_strdup(buf);
		}
		else
		{
			tmp = data;
			data = g_strconcat(tmp, buf, NULL);
			g_free(tmp);
		}
		n = data ? g_strlen(data) : 0;
		if (n > 0 && data[n - 1] == '\n')
		{
			data[n - 1] = '\0';
			q = g_malloc0(sizeof(GimvPrefsList));
			q->data = data;
			q->prev = p;
			if (p)
				p->next = q;
			p = q;
			data = NULL;
		}
	}
	if (fclose(fio) != 0 || !p)
	{
		while (p)
		{
			q = p->prev;
			g_free(p->data);
			g_free(p);
			p = q;
		}
		return NULL;
	}
	while (p->prev)
		p = p->prev;
	r = p;
	/* arrange the list */
	while (p)
	{
		data = p->data ? g_strstrip(g_strdup(p->data)) : NULL;
		n = data ? g_strlen(data) : 0;
		if (n <= 0)
		{
			p->type = GIMV_PREFS_DATA_TYPE_SPACE;
		}
		else if (data[0] == '#' || data[0] == ';')
		{
			p->type = GIMV_PREFS_DATA_TYPE_COMMENT;
		}
		else if (data[0] == '[' && data[n - 1] == ']')
		{
			p->type = GIMV_PREFS_DATA_TYPE_SECTION;
			g_free (section);
			section = g_strdup(data + 1);
			section[n - 2] = '\0';
			for (q = p->prev; q && q->type == GIMV_PREFS_DATA_TYPE_SPACE;
			     q = q->prev)
			{
				g_free(q->section);
				q->section = NULL;
			}
		}
		else if (strchr(data, '='))
		{
			p->type = GIMV_PREFS_DATA_TYPE_KEY;
			p->key = g_strdup(data);
			*strchr(p->key, '=') = '\0';
			p->value = strchr(p->data, '=') + 1;
		}
		else
		{
			p->type = GIMV_PREFS_DATA_TYPE_UNKNOW;
		}
		p->section = g_strdup(section);
		g_free(data);
		p = p->next;
	}
	g_free (section);
	return r;
}


/*
 * Open the initialize file.
 * file,file name
 * RET,the GimvPrefs struct
 */
GimvPrefs *
gimv_prefs_open (const gchar *file, const gchar *subfile)
{
	GimvPrefs *profile;

	profile = gimv_prefs_new();

	profile->subfile = subfile ? g_strdup(subfile) : NULL;
	profile->sublist = profile->subfile
		? gimv_prefs_list(profile->subfile) : NULL;

	profile->file = file ? g_strdup(file) : NULL;
	profile->list = profile->file
		? gimv_prefs_list(profile->file) : NULL;

	return profile;
}


/*
 * Close the initialize file.
 * profile,the GimvPrefs struct
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean
gimv_prefs_close (GimvPrefs *profile)
{
	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	gimv_prefs_save (profile);
	gtk_object_destroy(GTK_OBJECT(profile));

	return TRUE;
}


/*
 * Save the initialize file.
 * profile,the GimvPrefs struct
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean
gimv_prefs_save (GimvPrefs *profile)
{
	FILE *fio;
	GimvPrefsList *p;
	
	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	if (!profile->edit) return TRUE;
	if (!profile->file) return FALSE;

	fio = fopen(profile->file, "wt");
	if (!fio) return FALSE;

	/* when the profile was modified */
	for (p = profile->list; p; p = p->next)
	{
		if (p->data)
			fputs(p->data, fio);
		fputc('\n', fio);
	}
	fclose(fio);

	profile->edit = FALSE;

	return TRUE;
}


/*
 * Get a string from the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * key,name of the key
 * RET,string,NULL:error
 */
gchar *
gimv_prefs_get_string (GimvPrefs *profile,
		       const gchar *section,const gchar *key)
{
	GimvPrefsList *p;
	
	g_return_val_if_fail(GIMV_IS_PREFS(profile), NULL);

	if (!profile || !section || !key)
		return FALSE;
	for (p = profile->list; p; p = p->next)
	{
		if (p->type == GIMV_PREFS_DATA_TYPE_KEY 
		    && g_strcmp(p->section, section) == 0
		    && g_strcmp(p->key, key) == 0)
		{
			return g_strdup(p->value);
		}
	}
	for (p = profile->sublist; p; p = p->next)
	{
		if (p->type == GIMV_PREFS_DATA_TYPE_KEY
		    && g_strcmp(p->section, section) == 0
		    && g_strcmp(p->key, key) == 0)
		{
			return g_strdup(p->value);
		}
	}
	return NULL;
}


/*
 * Get size of a value.
 * profile,the GimvPrefs struct
 * section,name of the section
 * key,name of the key
 * type,value type
 * RET,bytes,0:error
 */
gint
gimv_prefs_get_size (GimvPrefs *profile,
		     const gchar *section,const gchar *key,const guint type)
{
	guint8 *array;
	gint n;
	GimvPrefsList *p;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), 0);

	if (!section || !key)
		return 0;
	for (p = profile->list; p; p = p->next)
	{
		if (p->type == GIMV_PREFS_DATA_TYPE_KEY
		    && g_strcmp(p->section, section) == 0
		    && g_strcmp(p->key, key) == 0)
		{
			break;
		}
	}
	if (!p)
	{
		for (p = profile->sublist; p; p = p->next)
			if (p->type == GIMV_PREFS_DATA_TYPE_KEY
			    && g_strcmp(p->section, section) == 0
			    && g_strcmp(p->key, key) == 0)
			{
				break;
			}
	}
	if (!p)
		return 0;
	switch (type)
	{
	case GIMV_PREFS_VALUE_TYPE_BOOL:
		return g_strcmp(p->value, "true") == 0
			|| g_strcmp(p->value, "false") == 0 ? sizeof(gboolean) : 0;
	case GIMV_PREFS_VALUE_TYPE_INT:
		return sizeof(gint);
	case GIMV_PREFS_VALUE_TYPE_STRING:
		return g_strlen(p->value) + 1;
	case GIMV_PREFS_VALUE_TYPE_ARRAY:
		if (!(array=misc_str_to_array(&n, p->value, 10, FALSE)))
			return 0;
		g_free(array);
		return n;
	}
	return 0;
}


/*
 * Get a value from the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * key,name of the key
 * value,buffer to store value.
 * size,size of buffer to store value.
 * type,value type
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean
gimv_prefs_get_value (GimvPrefs *profile,
                      const gchar *section,const gchar *key,
                      gpointer value,const gint size,const guint type)
{
	guint8 *array;
	gint n;
	GimvPrefsList *p;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	if (!section || !key || !value)
		return FALSE;
	for (p = profile->list; p; p = p->next)
	{
		if (p->type == GIMV_PREFS_DATA_TYPE_KEY
		    && g_strcmp(p->section, section) == 0
		    && g_strcmp(p->key, key) == 0)
		{
			break;
		}
	}
	if (!p)
	{
		for (p = profile->sublist; p; p = p->next)
			if (p->type == GIMV_PREFS_DATA_TYPE_KEY
			    && g_strcmp(p->section, section) == 0
			    && g_strcmp(p->key, key) == 0)
			{
				break;
			}
	}
	if (!p)
		return FALSE;
	switch (type)
	{
	case GIMV_PREFS_VALUE_TYPE_BOOL:
		if (size < (gint) sizeof(gboolean))
			return FALSE;
		if (strcasecmp (p->value, "true") == 0)
			*((gboolean *)value) = TRUE;
		else if (strcasecmp (p->value, "false") == 0)
			*((gboolean *)value) = FALSE;
		else
			return FALSE;
		break;
	case GIMV_PREFS_VALUE_TYPE_INT:
		if (size < (gint) sizeof(gint))
			return FALSE;
		misc_str_to_val((gint *)value, p->value, 10, TRUE);
		break;
	case GIMV_PREFS_VALUE_TYPE_STRING:
		if (size < (gint) g_strlen(p->value) + 1)
			return FALSE;
		strcpy((gchar *)value,p->value);
		break;
	case GIMV_PREFS_VALUE_TYPE_ARRAY:
		if (!(array = misc_str_to_array(&n, p->value, 10, FALSE)))
			return FALSE;
		if (size <= n)
			g_memmove(value, array, size);
		g_free(array);
		if (n < size)
			return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}


/*
 * Set a value into the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * key,name of the key
 * value,buffer which store the value.
 * size,size of buffer which store the value.
 * type,value type
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean
gimv_prefs_set_value (GimvPrefs *profile,
		      const gchar *section,const gchar *key,
		      gconstpointer value,const gint size,const guint type)
{
	gchar *old_value = NULL;
	gint i;
	GimvPrefsList *p,*q = NULL;
	GQuark quark;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	if (!section || !key || !value)
		return FALSE;

	quark = g_quark_from_string(section);

	for (p = profile->list; p; q = p, p = p->next)
	{
		if (p->type == GIMV_PREFS_DATA_TYPE_KEY
		    && g_strcmp(p->section, section) == 0
		    && g_strcmp(p->key, key) == 0)
		{
			break;
		}
	}

	if (p)
	{
		if (p->data)
			old_value = g_strdup(p->data);
	}
	else
	{
		for (p = q; p; p = p->prev)
		{
			if (p->section && g_strcmp(p->section, section) == 0)
				break;
		}
		if (!p)
		{
			if (q)
			{
				/* insert space between previous data and current data */
				p = g_malloc(sizeof(GimvPrefsList));
				p->type=GIMV_PREFS_DATA_TYPE_SPACE;
				p->value = p->data = p->section = p->key = NULL;
				p->prev = q;
				p->next = q->next;
				q->next = p;
				q = p;
			}
			/* create a section if neither section and key aren't exist */
			p = g_malloc(sizeof(GimvPrefsList));
			p->type = GIMV_PREFS_DATA_TYPE_SECTION;
			p->data = g_strdup_printf("[%s]", section);
			p->section = g_strdup(section);
			p->value = p->key = NULL;
			p->prev = q;
			if (q)
			{
				p->next = q->next;
				q->next = p;
			}
			else
			{
				p->next = NULL;
				profile->list = p;
			}
			gtk_signal_emit(GTK_OBJECT(profile),
					gimv_prefs_signals[SECTION_ADDED_SIGNAL],
					quark, p->section);
		}
		q = p;
		while (q->type == GIMV_PREFS_DATA_TYPE_SPACE && q->section
		       && g_strcmp(p->section, section) == 0 && q->prev)
			q = q->prev;
		/* create a key into last of the section */
		p = g_malloc(sizeof(GimvPrefsList));
		p->type = GIMV_PREFS_DATA_TYPE_KEY;
		p->data = g_strdup_printf("%s=", key);
		p->section = g_strdup(section);
		p->key = g_strdup(key);
		p->value = strchr(p->data,'=')+1;
		p->prev = q;
		p->next = q->next;
		q->next = p;
		if (p->next)
			p->next->prev = p;
		gtk_signal_emit(GTK_OBJECT(profile),
				gimv_prefs_signals[KEY_ADDED_SIGNAL],
				quark, p->section, p->key);
	}

	switch (type)
	{
	case GIMV_PREFS_VALUE_TYPE_BOOL:
		g_free(p->data);
		p->data = g_strdup_printf("%s=%s",
					  p->key, *((gboolean *)value) ? "true" : "false");
		break;
	case GIMV_PREFS_VALUE_TYPE_INT:
		g_free(p->data);
		p->data = g_strdup_printf("%s=%d", p->key, *((gint *)value));
		break;
	case GIMV_PREFS_VALUE_TYPE_STRING:
		g_free(p->data);
		p->data = g_strdup_printf("%s=%s", p->key, (gchar *)value);
		break;
	case GIMV_PREFS_VALUE_TYPE_ARRAY:
		g_free(p->data);
		p->data = g_strdup_printf("%s=%u", p->key, ((guint8 *)value)[0]);
		for (i = 1; i < size; i++)
		{
			gchar *data;
			data = g_strdup_printf("%s %u", p->data, ((guint8 *)value)[i]);
			g_free(p->data);
			p->data = data;
		}
		break;
	default:
		g_free(old_value);
		return FALSE;
	}

	p->value = strchr(p->data,'=') + 1;
	profile->edit = TRUE;

	gtk_signal_emit(GTK_OBJECT(profile), gimv_prefs_signals[CHANGED_SIGNAL],
			quark, p->section, p->key, old_value);

	g_free(old_value);

	return TRUE;
}


static void
gimv_prefs_list_free (GimvPrefs *profile, GimvPrefsList *p)
{
	if (!p) return;

	if (p == profile->list)
		profile->list = p->next;
	if (p->prev)
		p->prev->next = p->next;
	if (p->next)
		p->next->prev = p->prev;

	g_free(p->data);
	g_free(p->section);
	g_free(p->key);
	g_free(p);
}


/*
 * Delete a section from the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean
gimv_prefs_delete_section (GimvPrefs *profile, const gchar *section)
{
	gboolean result = FALSE;
	GimvPrefsList *p,*q;
	gchar *section_tmp;
	GQuark quark;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	if (!section)
		return FALSE;

	/*
	 * section and p->section may be same memory chunk.
	 */
	section_tmp = g_strdup(section);

	for (p = profile->list; p; p = q)
	{
		q = p->next;
		if (p->section && g_strcmp(p->section, section_tmp) == 0)
		{
			if (p->prev && p->prev->type == GIMV_PREFS_DATA_TYPE_SPACE)
				gimv_prefs_list_free(profile, p->prev);
			gimv_prefs_list_free(profile, p);
			profile->edit = TRUE;
			result = TRUE;
		}
	}

	quark = g_quark_from_string(section_tmp);
	gtk_signal_emit(GTK_OBJECT(profile),
			gimv_prefs_signals[SECTION_DELETED_SIGNAL],
			quark, section_tmp);

	g_free(section_tmp);

	return result;
}


/*
 * Delete a key from the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * key,key
 * RET,TRUE:normal exit,FALSE:error
 */
gboolean gimv_prefs_delete_key (GimvPrefs *profile,
				const gchar *section, const gchar *key)
{
	gboolean result = FALSE;
	GimvPrefsList *p,*q;
	gchar *section_tmp, *key_tmp;
	GQuark quark;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	if (!section || !key)
		return FALSE;

	/*
	 * section and p->section (also key and p->key) may be same memory
	 * chunk.
	 */
	section_tmp = g_strdup(section);
	key_tmp = g_strdup(key);

	for (p = profile->list; p; p = q)
	{
		q = p->next;
		if (p->section && p->key
		    && g_strcmp(p->section, section_tmp) == 0
		    && g_strcmp(p->key, key_tmp) == 0)
		{
			gimv_prefs_list_free(profile, p);
			profile->edit = TRUE;
			result = TRUE;
		}
	}

	quark = g_quark_from_string(section_tmp);
	gtk_signal_emit(GTK_OBJECT(profile),
			gimv_prefs_signals[KEY_DELETED_SIGNAL],
			quark, section_tmp, key_tmp);

	g_free(section_tmp);
	g_free(key_tmp);

	return result;
}


/*
 * Enumerate sections in the initialize file.
 * profile,the GimvPrefs struct
 * RET,list of sections,NULL:error
 */
GList *
gimv_prefs_enum_section (GimvPrefs *profile)
{
	GList *glist = NULL;
	GimvPrefsList *p;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	for (p = profile->list; p; p = p->next)
	{
		if (p->section
		    && (!glist || !g_list_find_custom(glist, p->section,
						      (GCompareFunc)strcmp)))
		{
			glist = g_list_insert_sorted(glist, p->section,
						     (GCompareFunc)strcmp);
		}
	}
	for (p = profile->sublist; p; p = p->next)
	{
		if (p->section
		    && (!glist || !g_list_find_custom(glist, p->section,
						      (GCompareFunc)strcmp)))
		{
			glist = g_list_insert_sorted(glist, p->section,
						     (GCompareFunc)strcmp);
		}
	}
	return glist;
}


/*
 * Enumelate keys in the initialize file.
 * profile,the GimvPrefs struct
 * section,name of the section
 * RET,list of keys,NULL:error
*/
GList *
gimv_prefs_enum_key (GimvPrefs *profile, const gchar *section)
{
	GList *glist = NULL;
	GimvPrefsList *p;

	g_return_val_if_fail(GIMV_IS_PREFS(profile), FALSE);

	for (p = profile->list; p; p = p->next)
	{
		if (p->section && p->key
		    && g_strcmp(p->section, section) == 0
		    && (!glist || !g_list_find_custom(glist, p->key,
						     (GCompareFunc)strcmp)))
		{
			glist = g_list_insert_sorted(glist, p->key,
						     (GCompareFunc)strcmp);
		}
	}
	for (p = profile->sublist; p; p = p->next)
	{
		if (p->section && p->key
		    && g_strcmp (p->section, section) == 0
		    && (!glist || !g_list_find_custom(glist, p->key,
						      (GCompareFunc)strcmp)))
		{
			glist = g_list_insert_sorted(glist, p->key,
						     (GCompareFunc)strcmp);
		}
	}
	return glist;
}
