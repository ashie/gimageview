# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

thumbview_plugin_LTLIBRARIES = \
	libgimv_thumbview_detail.la

thumbview_plugindir = $(DESTDIR)$(plugindir)/$(THUMBNAIL_VIEW_PLUGIN_DIR)

INCLUDES = \
	$(GTK_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

libgimv_thumbview_detail_la_LDFLAGS = \
	-module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_thumbview_detail_la_LIBADD  = $(GTK_LIBS)
libgimv_thumbview_detail_la_SOURCES = \
	detailview_prefs.c detailview_prefs.h \
	detailview.c detailview.h \
	detailview_priv.h
