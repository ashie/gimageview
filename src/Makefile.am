# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

SUBDIRS = pixmaps prefs_ui

CLEANFILES = *~ .*.swp .*.swo gimv.exp gimv.def gimv.res gimv.rc
bin_PROGRAMS = gimv

gimv_SOURCES = \
	gimv_dlist.c gimv_dlist.h \
	gimv_elist.c gimv_elist.h \
	gimv_cell_pixmap.c gimv_cell_pixmap.h \
	gimv_marshal.c gimv_marshal.h \
	gimv_scrolled.c gimv_scrolled.h \
	gimv_zalbum.c gimv_zalbum.h \
	gimv_zlist.c gimv_zlist.h \
\
	argparse.c argparse.h \
	charset.c charset.h \
	fileload.c fileload.h \
	fileutil.c fileutil.h \
	fr-archive.c fr-archive.h \
	fr-command.c fr-command.h \
	fr-process.c fr-process.h \
	gfileutil.c gfileutil.h \
	gimageview.c gimageview.h \
	gimv_anim.c gimv_anim.h \
	gimv_comment.c gimv_comment.h \
	gimv_comment_view.c gimv_comment_view.h \
	gimv_dir_view.c gimv_dir_view.h \
	gimv_dupl_finder.c gimv_dupl_finder.h \
	gimv_dupl_win.c gimv_dupl_win.h \
	gimv_exif_view.c gimv_exif_view.h \
	gimv_icon_stock.c gimv_icon_stock.h \
	gimv_image.c gimv_image.h \
	gimv_image_info.c gimv_image_info.h \
	gimv_image_loader.c gimv_image_loader.h \
	gimv_image_saver.c gimv_image_saver.h \
	gimv_image_view.c gimv_image_view.h \
	gimv_image_view_draw.c \
	gimv_image_win.c gimv_image_win.h \
	gimv_io.c gimv_io.h \
	gimv_io_file.c gimv_io_file.h \
	gimv_io_mem.c gimv_io_mem.h \
	gimv_mime_types.c gimv_mime_types.h \
	gimv_nav_win.c gimv_nav_win.h \
	gimv_plugin.c gimv_plugin.h \
	gimv_prefs_ui_utils.c gimv_prefs_ui_utils.h \
	gimv_prefs_win.c gimv_prefs_win.h \
	gimv_slideshow.c gimv_slideshow.h \
	gimv_text_win.c gimv_text_win.h \
	gimv_thumb.c gimv_thumb.h \
	gimv_thumb_cache.c gimv_thumb_cache.h \
	gimv_thumb_view.c gimv_thumb_view.h \
	gimv_thumb_view_album.c \
	gimv_thumb_win.c gimv_thumb_win.h \
	gtk_prop.c gtk_prop.h \
	gtkutils.c gtkutils.h \
	help.c help.h \
	md5.c md5.h \
	menu.c menu.h \
	prefs.c prefs.h \
	utils.c utils.h \
	utils_auto_comp.c utils_auto_comp.h \
	utils_dnd.c utils_dnd.h \
\
	compare_filesize.c \
	compare_md5.c \
	compare_similar.c \
	japanese.c japanese.h

EXTRA_gimv_SOURCES = \
	gimv.ico

CPPFLAGS= \
	-DDATADIR=\""$(gimv_datadir)"\" \
	-DDOCDIR=\""$(gimv_docdir)"\" \
	-DICONDIR=\""$(gimv_icondir)"\" \
	-DPLUGINDIR=\""$(plugindir)"\" \
	-DARCHIVER_PLUGIN_DIR=\""$(ARCHIVER_PLUGIN_DIR)"\" \
	-DIMAGE_LOADER_PLUGIN_DIR=\""$(IMAGE_LOADER_PLUGIN_DIR)"\" \
	-DIMAGE_SAVER_PLUGIN_DIR=\""$(IMAGE_SAVER_PLUGIN_DIR)"\" \
	-DIMAGE_VIEW_PLUGIN_DIR=\""$(IMAGE_VIEW_PLUGIN_DIR)"\" \
	-DIO_STREAM_PLUGIN_DIR=\""$(IO_STREAM_PLUGIN_DIR)"\" \
	-DTHUMBNAIL_PLUGIN_DIR=\""$(THUMBNAIL_PLUGIN_DIR)"\" \
	-DIMAGE_VIEW_PLUGIN_DIR=\""$(IMAGE_VIEW_PLUGIN_DIR)"\" \
	-DTHUMBNAIL_VIEW_PLUGIN_DIR=\""$(THUMBNAIL_VIEW_PLUGIN_DIR)"\"

INCLUDES = \
	$(GTK_CFLAGS) \
	$(EXIF_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libs


if PLATFORM_WIN32
gimv_exp = gimv.exp

gimv.exp: gimv.def
	$(DLLTOOL) --dllname gimv$(EXEEXT) --input-def gimv.def \
	--output-exp $@ --output-lib libgimv.a 

gimv.def: $(gimv_OBJECTS)
	echo EXPORTS > gimv.def
	nm $(gimv_OBJECTS) | grep ' T _' | sed 's/.* T _//' >> gimv.def
	nm $(gimv_OBJECTS) | grep ' D _' | sed 's/.* D _//' | sed "s/\$$/ DATA/" >> gimv.def

gimv_res = gimv.res

gimv.rc:
	echo "1 ICON gimv.ico" > gimv.rc

gimv.res: gimv.rc
	$(WINDRES) $< -O coff -o $@

endif

gimv_LDFLAGS = \
	-export-dynamic

gimv_LDADD = \
	$(gimv_res) \
	$(gimv_exp) \
	$(GTK_LIBS) \
	$(EXIF_LIBS) \
	$(XINERAMA_LIBS) \
	$(X_LIBS) \
	$(top_builddir)/src/prefs_ui/libgimv_prefs_ui.la
