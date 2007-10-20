# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

desktopdir = $(DESTDIR)$(datadir)/applications
desktop_in_files = gimageview.desktop.in
#desktop_DATA = gimageview.desktop
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 
@INTLTOOL_DESKTOP_RULE@

icondir = $(datadir)/pixmaps
icon_DATA = gimv.png

prefsdir = $(DESTDIR)$(gimv_datadir)
prefs_DATA = \
	gtkrc

mplayerrcdir = $(DESTDIR)$(gimv_datadir)
mplayerrc_DATA = \
	mplayerrc

EXTRA_DIST = \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(icon_DATA) \
	$(prefs_DATA) \
	$(mplayerrc_DATA)
