# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

CLEANFILES = *~ .*.swp .*.swo

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -lgimv
endif

image_saver_plugin_LTLIBRARIES = \
	libgimv_png_saver.la \
	libgimv_pnm_saver.la \
	libgimv_xvpics_saver.la

image_saver_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_SAVER_PLUGIN_DIR)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src $(GTK_CFLAGS)

libgimv_png_saver_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_png_saver_la_SOURCES = png_saver.c
libgimv_png_saver_la_LIBADD = $(GTK_LIBS) $(PNG_LIBS)

libgimv_pnm_saver_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_pnm_saver_la_SOURCES = pnm_saver.c
libgimv_pnm_saver_la_LIBADD = $(GTK_LIBS)

libgimv_xvpics_saver_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xvpics_saver_la_SOURCES = dither.c dither.h xvpics_saver.c
libgimv_xvpics_saver_la_LIBADD = $(GTK_LIBS)
