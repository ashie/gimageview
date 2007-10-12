## Process this file with automake to produce Makefile.in
# Makefile.am for enfle
# $Id$

if PLATFORM_WIN32
else
if ENABLE_SPI
noinst_LTLIBRARIES = libdllloader.la
endif
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/lib

libdllloader_la_SOURCES = \
	compat.h common.h \
	windef.h w32api.h export.h \
	mm.c mm.h \
	pe_image.c pe_image.h pe.h \
	module.c module.h \
	kernel32.c kernel32.h \
	user32.c user32.h \
	advapi32.c advapi32.h \
	msvcrt.c msvcrt.h \
	borlndmm.c borlndmm.h \
	hash.c hash.h \
	converter.c converter.h \
	dlist.c dlist.h \
	libstring.c libstring.h \
	misc.c misc.h
