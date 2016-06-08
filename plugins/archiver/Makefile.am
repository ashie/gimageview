# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

archiver_plugin_LTLIBRARIES = \
	libgimv_lha_extarc.la \
	libgimv_rar_extarc.la \
	libgimv_tar_extarc.la \
	libgimv_zip_extarc.la

archiver_plugindir = $(DESTDIR)$(plugindir)/$(ARCHIVER_PLUGIN_DIR)

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src $(GTK_CFLAGS)

libgimv_lha_extarc_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_lha_extarc_la_SOURCES = lha-ext.c lha-ext.h
libgimv_lha_extarc_la_LIBADD = $(GTK_LIBS)

libgimv_rar_extarc_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_rar_extarc_la_SOURCES = rar-ext.c rar-ext.h
libgimv_rar_extarc_la_LIBADD = $(GTK_LIBS)

libgimv_tar_extarc_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_tar_extarc_la_SOURCES = tar-ext.c tar-ext.h
libgimv_tar_extarc_la_LIBADD = $(GTK_LIBS)

libgimv_zip_extarc_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_zip_extarc_la_SOURCES = zip-ext.c zip-ext.h
libgimv_zip_extarc_la_LIBADD = $(GTK_LIBS)
