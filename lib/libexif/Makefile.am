if ENABLE_EXIF
noinst_LTLIBRARIES = libexif.la
endif

INCLUDES = \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib

CPPFLAGS= \
	-DGETTEXT_PACKAGE=PACKAGE \
	-DLIBEXIF_LOCALEDIR=\""$(localedir)"\"

libexif_la_SOURCES = \
	exif-byte-order.c exif-byte-order.h \
	exif-content.c exif-content.h \
	exif-data.c exif-data.h \
	exif-entry.c exif-entry.h \
	exif-format.c exif-format.h \
	exif-ifd.c exif-ifd.h \
	exif-loader.c exif-loader.h \
	exif-result.h \
	exif-tag.c exif-tag.h \
	exif-utils.c exif-utils.h \
	i18n.h \
        jpeg-data.c jpeg-data.h	\
        jpeg-marker.c jpeg-marker.h
