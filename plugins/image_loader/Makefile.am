# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

if PLATFORM_WIN32
no_undefined = -no-undefined
GIMV_LIBS = -L$(top_builddir)/src -Wl,-lgimv
endif

CLEANFILES = *~ .deps/* .*.swp .*.swo

image_loader_plugin_LTLIBRARIES = \
	libgimv_bmp_loader.la \
	libgimv_mag_loader.la \
	libgimv_pcx_loader.la \
	libgimv_png_loader.la \
	libgimv_tga_loader.la \
	libgimv_xbm_loader.la \
	libgimv_xcf_loader.la \
	libgimv_xvpics_loader.la

if ENABLE_GDK_IMLIB
gimv_imlib1_loader_plugin_LTLIBRARIES = libgimv_imlib1_loader.la
endif

if ENABLE_GDK_PIXBUF
gimv_pixbuf_loader_plugin_LTLIBRARIES = libgimv_pixbuf_loader.la
endif

if ENABLE_JPEG
gimv_jpeg_loader_plugin_LTLIBRARIES = libgimv_jpeg_loader.la
endif

if ENABLE_MNG
gimv_mng_loader_plugin_LTLIBRARIES = libgimv_mng_loader.la
endif

if ENABLE_SPI
gimv_spi_loader_plugin_LTLIBRARIES = libgimv_spi_loader.la
endif

if ENABLE_LIBRSVG
gimv_rsvg_loader_plugin_LTLIBRARIES = libgimv_rsvg_loader.la
endif

if ENABLE_WMF
gimv_wmf_loader_plugin_LTLIBRARIES = libgimv_wmf_loader.la
endif

image_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_imlib1_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_jpeg_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_mng_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_pixbuf_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_spi_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_rsvg_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)
gimv_wmf_loader_plugindir = $(DESTDIR)$(plugindir)/$(IMAGE_LOADER_PLUGIN_DIR)

INCLUDES = \
	$(GTK_CFLAGS) \
	$(GIMV_IMAGELIB_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(PNG_CFLAGS) \
	$(JPEG_CFLAGS) \
	$(MNG_CFLAGS) \
	$(XML_CFLAGS) \
	$(LIBRSVG_CFLAGS) \
	$(LIBWMF_CFLAGS) \
	$(DLLLOADER_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

CPPFLAGS = \
	-DPLUGINDIR=\""$(plugindir)"\"

libgimv_bmp_loader_la_LIBADD = $(GTK_LIBS)
libgimv_bmp_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_bmp_loader_la_SOURCES = bmp.c bmp.h

libgimv_imlib1_loader_la_LIBADD = $(GTK_LIBS) $(GDK_IMLIB_LIBS)
libgimv_imlib1_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_imlib1_loader_la_SOURCES = imlib_loader.c imlib_loader.h

libgimv_jpeg_loader_la_LIBADD = $(GTK_LIBS) $(JPEG_LIBS)
libgimv_jpeg_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_jpeg_loader_la_SOURCES = jpeg_loader.c

libgimv_mag_loader_la_LIBADD = $(GTK_LIBS)
libgimv_mag_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_mag_loader_la_SOURCES = mag.c mag.h

libgimv_mng_loader_la_LIBADD = $(GTK_LIBS) $(MNG_LIBS)
libgimv_mng_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_mng_loader_la_SOURCES = mng.c mng.h

libgimv_pcx_loader_la_LIBADD = $(GTK_LIBS)
libgimv_pcx_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_pcx_loader_la_SOURCES = pcx.c pcx.h

libgimv_pixbuf_loader_la_LIBADD = $(GTK_LIBS) $(GDK_PIXBUF_LIBS)
libgimv_pixbuf_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_pixbuf_loader_la_SOURCES = \
	pixbuf_anim.c pixbuf_anim.h \
	pixbuf_loader.c pixbuf_loader.h

libgimv_png_loader_la_LIBADD = $(GTK_LIBS) $(PNG_LIBS) $(ZLIB_LIBS)
libgimv_png_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_png_loader_la_SOURCES = png_loader.c png_loader.h

libgimv_spi_loader_la_LIBADD = $(GTK_LIBS) $(DLLLOADER_LIBS)
libgimv_spi_loader_la_LDFLAGS = -module -avoid-version  $(no_undefined) $(GIMV_LIBS)
libgimv_spi_loader_la_SOURCES = \
	prefs_spi.c prefs_spi.h \
	spi.c spi.h spi-private.h \
	spi_image_loader.c spi_image_loader.h

libgimv_rsvg_loader_la_LIBADD = $(GTK_LIBS) $(XML_LIBS) $(LIBRSVG_LIBS)
libgimv_rsvg_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_rsvg_loader_la_SOURCES = svg.c svg.h

libgimv_tga_loader_la_LIBADD = $(GTK_LIBS)
libgimv_tga_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_tga_loader_la_SOURCES = tga.c tga.h

libgimv_wmf_loader_la_LIBADD = $(GTK_LIBS) $(LIBWMF_LIBS)
libgimv_wmf_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_wmf_loader_la_SOURCES = wmf.c wmf.h

libgimv_xbm_loader_la_LIBADD = $(GTK_LIBS)
libgimv_xbm_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xbm_loader_la_SOURCES = xbm.c xbm.h

libgimv_xcf_loader_la_LIBADD = $(GTK_LIBS)
libgimv_xcf_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xcf_loader_la_SOURCES = xcf.c xcf.h

libgimv_xvpics_loader_la_LIBADD = $(GTK_LIBS)
libgimv_xvpics_loader_la_LDFLAGS = -module -avoid-version $(no_undefined) $(GIMV_LIBS)
libgimv_xvpics_loader_la_SOURCES = xvpics.c xvpics.h
