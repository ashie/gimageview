# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

CLEANFILES = *~ .deps/* .*.swp .*.swo

noinst_LTLIBRARIES = libgimv_prefs_ui.la

INCLUDES = \
	$(GTK_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

libgimv_prefs_ui_la_LIBADD = \
	$(GTK_LIBS)

libgimv_prefs_ui_la_SOURCES = \
	prefs_ui_common.c prefs_ui_common.h \
	prefs_ui_etc.c prefs_ui_etc.h \
	prefs_ui_imagewin.c prefs_ui_imagewin.h \
	prefs_ui_movie.c prefs_ui_movie.h \
	prefs_ui_progs.c prefs_ui_progs.h \
	prefs_ui_thumbalbum.c prefs_ui_thumbalbum.h \
	prefs_ui_thumbwin.c prefs_ui_thumbwin.h \
	prefs_ui_plugin.c prefs_ui_plugin.h
