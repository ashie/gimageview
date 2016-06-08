# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc etc m4 po lib src plugins

CLEANFILES = *~ .*.swp .*.swo

endocdir = $(DESTDIR)$(gimv_docdir)/text/C
endoc_DATA = \
	AUTHORS \
	COPYING \
	ChangeLog \
	README \
	INSTALL \
	NEWS \
	TODO

EXTRA_DIST = \
	intltool-merge.in \
	intltool-update.in \
	intltool-extract.in \
	autogen.sh \
	gimageview.spec \
	gimageview.spec.in \
	$(endoc_DATA)
