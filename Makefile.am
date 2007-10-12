# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

SUBDIRS = doc etc m4 po lib src plugins

CLEANFILES = *~ .deps/* .*.swp .*.swo

endocdir = $(DESTDIR)$(gimv_docdir)/text/C
endoc_DATA = \
	AUTHORS \
	COPYING \
	ChangeLog \
	README \
	INSTALL \
	NEWS \
	TODO \
	HACKING

EXTRA_DIST = \
	intltool-merge.in \
	intltool-update.in \
	intltool-extract.in \
	autogen.sh \
	gimageview.spec \
	gimageview.spec.in \
	$(endoc_DATA)
