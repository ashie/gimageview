# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

CLEANFILES = *~ .*.swp .*.swo

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

if ENABLE_MPLAYER
gimv_mplayer_plugin_LTLIBRARIES = libgimv_mplayer.la
endif

if ENABLE_XINE
gimv_xine_plugin_LTLIBRARIES = libgimv_xine.la
endif

gimv_mplayer_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_VIEW_PLUGIN_DIR)
gimv_xine_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_VIEW_PLUGIN_DIR)

CPPFLAGS= \
	-DDATADIR=\""$(gimv_datadir)"\"

INCLUDES = \
	$(GTK_CFLAGS) \
	$(XINE_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

libgimv_mplayer_la_LIBADD = $(GTK_LIBS)
libgimv_mplayer_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_mplayer_la_SOURCES = \
	gimv_mplayer.c gimv_mplayer.h \
	image_view_mplayer.c image_view_mplayer.h \
	mplayer_image_loader.c mplayer_image_loader.h \
	prefs_mplayer.c prefs_mplayer.h

libgimv_xine_la_LIBADD = $(GTK_LIBS) $(XINE_LIBS)
libgimv_xine_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xine_la_SOURCES = \
	gimv_xine.c gimv_xine.h \
	gimv_xine_priv.c gimv_xine_priv.h \
	gimv_xine_post.c gimv_xine_post.h \
	image_view_xine.c image_view_xine.h \
	xine_image_loader.c xine_image_loader.h \
	prefs_xine.c prefs_xine.h
