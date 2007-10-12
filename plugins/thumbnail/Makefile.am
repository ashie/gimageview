# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

thumbnail_plugin_LTLIBRARIES = \
	libgimv_thumb.la \
	libgimv_nautilus_thumb.la \
	libgimv_nautilus-2.0_thumb.la \
	libgimv_konqueror_thumb.la \
	libgimv_konq_old_thumb.la \
	libgimv_gqview_thumb.la \
	libgimv_ee_thumb.la \
	libgimv_xvpics_thumb.la

thumbnail_plugindir = $(DESTDIR)$(plugindir)/$(THUMBNAIL_PLUGIN_DIR)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src $(GTK_CFLAGS)

libgimv_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_thumb_la_SOURCES = gimv_thumb.c
libgimv_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_nautilus_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_nautilus_thumb_la_SOURCES = nautilus.c
libgimv_nautilus_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_nautilus_2_0_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_nautilus_2_0_thumb_la_SOURCES = nautilus-2.0.c
libgimv_nautilus_2_0_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_konqueror_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_konqueror_thumb_la_SOURCES = konqueror.c
libgimv_konqueror_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_konq_old_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_konq_old_thumb_la_SOURCES = konqueror-old.c
libgimv_konq_old_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_gqview_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_gqview_thumb_la_SOURCES = gqview.c
libgimv_gqview_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_ee_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_ee_thumb_la_SOURCES = ee.c
libgimv_ee_thumb_la_LIBADD  = $(GTK_LIBS)

libgimv_xvpics_thumb_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xvpics_thumb_la_SOURCES = xvpics.c
libgimv_xvpics_thumb_la_LIBADD  = $(GTK_LIBS)
