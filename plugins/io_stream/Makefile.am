# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

CLEANFILES = *~ .*.swp .*.swo

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

io_stream_plugin_LTLIBRARIES = \
	libgimv_gzip_io.la

if HAVE_BZLIB
gimv_bz_io_plugin_LTLIBRARIES = libgimv_bz_io.la
endif

io_stream_plugindir = $(DESTDIR)$(plugindir)/$(IO_STREAM_PLUGIN_DIR)
gimv_bz_io_plugindir = $(DESTDIR)$(plugindir)/$(IO_STREAM_PLUGIN_DIR)

AM_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(ZLIB_CLAGS) \
	$(BZLIB_CFLAGS) \
        -I$(top_srcdir) \
        -I$(top_srcdir)/src

libgimv_bz_io_la_LIBADD = $(GTK_LIBS) $(BZLIB_LIBS)
libgimv_bz_io_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_bz_io_la_SOURCES = gimv_bz_io.c gimv_bz_io.h

libgimv_gzip_io_la_LIBADD = $(GTK_LIBS) $(ZLIB_LIBS)
libgimv_gzip_io_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_gzip_io_la_SOURCES = gimv_gzip_io.c gimv_gzip_io.h
