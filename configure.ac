dnl -*- Mode: Autoconf; tab-width: 8; indent-tabs-mode: nil; -*-
dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT(src/gimageview.c)
PACKAGE=gimageview

dnl version number
MAJOR_VERSION=0
MINOR_VERSION=2
MICRO_VERSION=27
EXTRA_VERSION=
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION$EXTRA_VERSION

dnl 
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")

dnl Specify a header configuration file
AM_CONFIG_HEADER(config.h)



dnl ******************************
dnl Checks for programs.
dnl ******************************
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AC_ISC_POSIX
AM_MAINTAINER_MODE


dnl ******************************
dnl libtool
dnl ******************************
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL


dnl ******************************
dnl Checks for host
dnl ******************************
AC_CANONICAL_HOST

dnl Used for enabling the "-no-undefined" flag while generating DLLs
dnl Copied from the official gtk+-2 configure.in
AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test x"$platform_win32" = x"yes")

if test x"$platform_win32" = x"yes"; then
    WINDRES=windres
    AC_SUBST(WINDRES)
fi

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test x"$os_win32" = x"yes")


dnl ******************************
dnl required library version
dnl ******************************
# Gtk+-2.0
GLIB2_REQUIRED=2.10.0
GMODULE2_REQUIRED=2.10.0
GDK_PIXBUF2_REQUIRED=2.10.0
GTK2_REQUIRED=2.10.0
AC_SUBST(GLIB2_REQUIRED)
AC_SUBST(GMODULE2_REQUIRED)
AC_SUBST(GTK2_REQUIRED)
AC_SUBST(GDK_PIXBUF2_REQUIRED)

# Backends
LIBRSVG_REQUIRED=1.0.0
LIBWMF_REQUIRED=0.2.8
XINE_REQUIRED=1.0.0
AC_SUBST(LIBRSVG_REQUIRED)
AC_SUBST(LIBWMF_REQUIRED)
AC_SUBST(XINE_REQUIRED)


dnl ******************************
dnl Check for GTK
dnl ******************************
enable_gtk2=yes
PKG_CHECK_MODULES(GTK,
   [glib-2.0       >= $GLIB2_REQUIRED
    gmodule-2.0    >= $GMODULE2_REQUIRED
    gdk-pixbuf-2.0 >= $GDK_PIXBUF2_REQUIRED
    gtk+-2.0       >= $GTK2_REQUIRED], ,
   [ AC_MSG_ERROR(Test for GTK+-2.0 failed. See the file 'INSTALL' for help) ])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)


dnl ******************************
dnl Check for image libraries
dnl ******************************
GIMV_IMAGELIB_CFLAGS=""
GIMV_IMAGELIB_LIBS=""

AC_SUBST(GIMV_IMAGELIB_CFLAGS)
AC_SUBST(GIMV_IMAGELIB_LIBS)



dnl ********************
dnl Test for GZIP
dnl ********************
ZLIB_CFLAGS=""
ZLIB_LIBS="-lz"
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)


dnl ********************
dnl Test for bzlib
dnl ********************
BZLIB_CFLAGS=""
BZLIB_LIBS=""
AC_CHECK_HEADER(bzlib.h, [
   AC_CHECK_LIB(bz2, BZ2_bzlibVersion, [
      available_bzlib=yes;
      BZLIB_LIBS="-lbz2";
      AC_DEFINE(BZAPI_NEEDS_PREFIX, 1, [Define if your bzlib needs BZ2_ prefix.])
   ], [
      AC_CHECK_LIB(bz2, bzlibVersion, [
         available_bzlib=yes; BZLIB_LIBS="-lbz2";
      ])
   ])
])

if test x"$available_bzlib" = xyes; then
   AC_DEFINE(HAVE_BZLIB, 1, [Define if you have bzlib.])
fi
AM_CONDITIONAL(HAVE_BZLIB, test x"$available_bzlib" = x"yes")
AC_SUBST(BZLIB_CFLAGS)
AC_SUBST(BZLIB_LIBS)


dnl ********************
dnl Test for libpng
dnl ********************
PNG_CFLAGS=""
PNG_LIBS=""
AC_CHECK_LIB(png, png_read_info, [
   AC_CHECK_HEADER(png.h,
      [ png_ok=yes ],
      [ png_ok=no ])
], [
   AC_MSG_ERROR(*** (PNG library is not found) ***)
], -lz -lm)

if test x"$png_ok" = xyes; then
   AC_MSG_CHECKING([for png_structp in png.h])
   AC_TRY_COMPILE([ #include <png.h> ],
      [ png_structp pp;
        png_infop info;
        png_colorp cmap;
        png_get_header_ver(NULL);
        png_create_read_struct; ],
      [ png_ok=yes ],
      [ png_ok=no ])
   AC_MSG_RESULT($png_ok)
   if test x"$png_ok" = xyes; then
      PNG_LIBS="-lpng -lz"
   else
      AC_MSG_ERROR(*** PNG library (libpng) is too old ***)
   fi
else
   AC_MSG_ERROR(*** PNG library (libpng) required ***)
fi

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)
AM_CONDITIONAL(HAVE_PNG, test "x$PNG_LIBS" != x)


dnl ********************
dnl Test for libjpeg
dnl ********************
JPEG_CFLAGS=""
JPEG_LIBS=""
available_jpeg=no
AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
   available_jpeg=yes,
   available_jpeg=no)

if test x"$available_jpeg" = xyes; then
   AC_MSG_CHECKING([for jpeglib.h])
   AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
      available_jpeg=yes,
      available_jpeg=no)
   AC_MSG_RESULT($available_jpeg)
   if test x"$available_jpeg" = xyes; then
      JPEG_LIBS='-ljpeg'
      AC_CHECK_LIB(jpeg, jpeg_simple_progression,     
         AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1,
                   [Define if your JPEG library supports progressive saving.]),
         AC_MSG_WARN([JPEG library does not support progressive saving.]))
   fi
fi

if test x"$available_jpeg" = xyes; then
   AC_DEFINE(ENABLE_JPEG, 1,
             [Define if you want to compile JPEG plugin.])
else
   AC_MSG_WARN([*** JPEG loader will not be built ***])
fi

AM_CONDITIONAL(ENABLE_JPEG, test x"$available_jpeg" = x"yes")
AC_SUBST(JPEG_CFLAGS)
AC_SUBST(JPEG_LIBS)

enable_jpeg=$available_jpeg


dnl ********************
dnl Test for libmng
dnl ********************
AC_ARG_WITH(libmng, 
   [  --with-libmng           Use libmng for MNG image [default=yes]],
   [enable_mng="${withval}"], [enable_mng=yes])

MNG_CFLAGS=""
MNG_LIBS=""
if test x"$enable_mng" = x"yes"; then
   if test x"$available_jpeg" = x"yes"; then
      AC_CHECK_HEADER(libmng.h, [
         AC_CHECK_LIB(mng, mng_initialize, [
            AC_MSG_CHECKING([for libmng functions])
            AC_TRY_COMPILE([ #include <libmng.h> ],
               [ mng_handle handle;
                 mng_version_text();
                 handle = mng_initialize(NULL, NULL, NULL, MNG_NULL);
                 mng_setcb_errorproc(handle, NULL);
                 mng_setcb_openstream(handle, NULL);
                 mng_setcb_closestream(handle, NULL);
                 mng_setcb_readdata(handle, NULL);
                 mng_setcb_gettickcount(handle, NULL);
                 mng_setcb_settimer(handle, NULL);
                 mng_setcb_processheader(handle, NULL);
                 mng_setcb_getcanvasline(handle, NULL);
                 mng_setcb_refresh(handle, NULL);
                 mng_get_userdata(handle);
                 mng_set_canvasstyle(handle, MNG_CANVAS_RGB8);
                 mng_set_bgcolor(handle, 0, 0, 0);
                 mng_readdisplay(handle);
                 mng_readdisplay_resume(handle); ],
               [ MNG_LIBS="$JPEG_LIBS -lz -lmng";
                 AC_DEFINE(ENABLE_MNG, 1, [Define if you want to compile mng plugin.])],
               [ enable_mng=no;
                 AC_MSG_WARN(*** MNG library (libmng) is found but it is too old ***) ])
            AC_MSG_RESULT($enable_mng)
         ], [
            enable_mng=no
         ], $MNG_LIBS )
      ], [
         enable_mng=no:
      ])
   else
      AC_MSG_WARN([*** libmng requires libjpeg. MNG plugin will not be compiled.])
      enable_mng=no
   fi
fi

AM_CONDITIONAL(ENABLE_MNG, test x"$enable_mng" = xyes)
AC_SUBST(MNG_CFLAGS)
AC_SUBST(MNG_LIBS)


dnl ********************
dnl Test for librsvg
dnl ********************
AC_ARG_WITH(librsvg, 
            [  --with-librsvg          Use librsvg for SVG image [default=yes]],
            enable_librsvg="${withval}",
            enable_librsvg=yes)

if test x"$enable_librsvg" = xyes; then
   PKG_CHECK_MODULES(LIBRSVG, librsvg-2.0, , [
      enable_librsvg=no;
      AC_MSG_WARN(Did not find librsvg-2.0 installed)
   ])
fi

AM_CONDITIONAL(ENABLE_LIBRSVG, test x"$enable_librsvg" = xyes)

if test x"$enable_librsvg" = xyes; then
   AC_DEFINE(HAVE_LIBRSVG, 1, [Define if you have librsvg.])
   AC_DEFINE(ENABLE_SVG, 1, [Define if you want to compile svg plugin.])
fi

AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)
AC_SUBST(LIBRSVG_CFLAGS) 
AC_SUBST(LIBRSVG_LIBS)


dnl ****************************
dnl Check for libwmf
dnl ****************************
AC_ARG_WITH(libwmf,
   [  --with-libwmf              Use libwmf library for Windows Meta File [default=yes]],
   [enable_libwmf="${withval}"],
   [enable_libwmf=yes])

if test "x$enable_libwmf" = xyes; then
   have_libwmf=no
   AM_PATH_LIBWMF($LIBWMF_REQUIRED,
      [have_libwmf=yes], [have_libwmf=no])

   if test "x$have_libwmf" = xyes; then
      AC_DEFINE(ENABLE_WMF, 1, [Define if you want to compile wmf plugin.])
   else
      AC_MSG_WARN(Did not find libwmf installed)
      enable_libwmf=no;
   fi
fi

AM_CONDITIONAL(ENABLE_WMF, test x"$enable_libwmf" = xyes)


dnl ****************************
dnl check for Xinerama
dnl ****************************
AC_PATH_XTRA
XINERAMA_LIBS=""
AC_CHECK_LIB(Xinerama, XineramaQueryScreens,
               [XINERAMA_LIBS="-lXinerama"
                  AC_CHECK_HEADERS(X11/extensions/Xinerama.h)],
                    [],[$X_LIBS -lX11 -lXext])
AC_SUBST(XINERAMA_LIBS)
AC_SUBST(X_LIBS)


movie_support=""

dnl ****************************
dnl Check for Xine library
dnl ****************************
AC_ARG_WITH(xine,
   [  --with-xine             Use Xine library for movie (experimental) [default=no]],
   [enable_xine="${withval}"],
   [enable_xine=no])

if test x"$enable_xine" = xyes; then
   have_xine=no
   AM_PATH_XINE($XINE_REQUIRED,
      [have_xine=yes], [have_xine=no])

   if test x"$have_xine" = xyes; then
      AC_DEFINE(ENABLE_XINE, 1, [Define if you want to compile xine plugin.])
      movie_support="Xine ${movie_support}"
   else
      AC_MSG_WARN(Did not find xine-lib installed)
      enable_xine=no;
   fi
fi

AM_CONDITIONAL(ENABLE_XINE, test x"$enable_xine" = xyes)


dnl ******************************
dnl for gettext
dnl ******************************
GETTEXT_PACKAGE=gimv
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package])

ALL_LINGUAS="fr ja"
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(GIMV_LOCALEDIR)

dnl ***************************************************************************
dnl Check for bind_textdomain_codeset, including -lintl if GLib brings it in.
dnl ***************************************************************************
gimv_save_LIBS=$LIBS
LIBS="$LIBS $GTK_LIBS"
AC_CHECK_FUNCS(bind_textdomain_codeset)
LIBS=$gimv_save_LIBS


dnl *******************
dnl for intltool
dnl *******************
AC_PROG_INTLTOOL


dnl ******************************
dnl enable/disable EXIF support
dnl ******************************
AC_ARG_ENABLE(exif,
   [  --enable-exif           enable EXIF support [default=yes]],
   [enable_exif="${enableval}"], [enable_exif=yes])
AM_CONDITIONAL(ENABLE_EXIF, test x"$enable_exif" = xyes)
if test x"$enable_exif" = xyes; then
   AC_DEFINE(ENABLE_EXIF, 1, [Define if you want to enable EXIF feature.])
   EXIF_CFLAGS="-I\$(top_srcdir)/lib"
   EXIF_LIBS="\$(top_builddir)/lib/libexif/libexif.la"
   AC_SUBST(EXIF_CFLAGS)
   AC_SUBST(EXIF_LIBS)
fi


dnl **************************************
dnl enable/disable susie plugin support
dnl **************************************
AC_ARG_ENABLE(spi,
   [  --enable-spi            susie plugin support (broken) [default=no]],
   [enable_spi="${enableval}"], [enable_spi=no])
AM_CONDITIONAL(ENABLE_SPI, test x"$enable_spi" = xyes)

if test x"$enable_spi" = xyes; then
  AC_DEFINE(ENABLE_SPI, 1, [Define if you want to enable Susie plugin support feature.])
  if test x"$platform_win32" = xno; then
    DLLLOADER_CFLAGS="-I\$(top_srcdir)/lib -I\$(top_srcdir)/lib/dllloader"
    DLLLOADER_LIBS="\$(top_builddir)/lib/dllloader/libdllloader.la"
    AC_SUBST(DLLLOADER_CFLAGS)
    AC_SUBST(DLLLOADER_LIBS)
  fi
fi


dnl ******************************
dnl disable/enable splash logo
dnl ******************************
AC_ARG_ENABLE(splash,
   [  --enable-splash         enable splash logo],
   [enable_splash="${enableval}"], [enable_splash=yes])
if test x"$enable_splash" = xyes; then
   AC_DEFINE(ENABLE_SPLASH, 1, [Define if you want to enable splash feature.])
fi


dnl *********************************
dnl disable/enable MPlayer Support
dnl *********************************
AC_ARG_ENABLE(mplayer,
   [  --enable-mplayer        enable MPlyer support (experimental) [default=no]],
   [enable_mplayer="${enableval}"], [enable_mplayer=no])
if test x"$enable_mplayer" = xyes; then
   AC_DEFINE(ENABLE_MPLAYER, 1, [Define if you want to compile MPlayer plugin.])
   movie_support="MPlayer ${movie_support}"
fi

AM_CONDITIONAL(ENABLE_MPLAYER, test x"$enable_mplayer" = x"yes")


dnl *********************************
dnl Check Movie Support
dnl *********************************
if test x"$movie_support" = x; then
   movie_support=no
else
   AC_DEFINE(ENABLE_MOVIE, 1, [Define if you use movie support])
fi


dnl ******************************
dnl Checks for header files.
dnl ******************************
AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([							\
	alloca.h argz.h							\
	errno.h								\
	fcntl.h								\
	langinfo.h libintl.h limits.h locale.h				\
	malloc.h memory.h						\
	netinet/in.h nl_types.h						\
	stddef.h stdlib.h string.h strings.h sys/param.h sys/time.h	\
	unistd.h utime.h						\
])


dnl **************************************************************
dnl Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_TYPES([struct user_desc],,, [#include <asm/ldt.h>])
if test x$CC = xgcc; then
   CFLAGS="$CFLAGS -Wall"
fi	
if test x"$platform_win32" = xyes; then
   LDFLAGS="$LDFLAGS -mwindows"
fi

dnl **********************************
dnl Checks for library functions.
dnl **********************************
AC_FUNC_ALLOCA
dnl AC_FUNC_CHOWN
dnl AC_FUNC_FORK
dnl AC_FUNC_LSTAT
dnl AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
dnl AC_FUNC_MKTIME
dnl AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([						\
	__argz_count __argz_next __argz_stringify		\
	dcgettext dup2						\
	endgrent endpwent					\
	floor							\
	getcwd getpagesize getwd				\
	memmove mempcpy memset mkdir munmap			\
	nl_langinfo						\
	pow putenv setenv					\
	setlocale sqrt stpcpy strcasecmp strchr strcspn		\
	strdup strrchr strstr strtoul				\
	uname utime						\
])


dnl ******************************
dnl set data dir definition
dnl ******************************
gimv_datadir="\${datadir}/\${PACKAGE}"
docdir="\${datadir}/doc"
gimv_docdir="\${docdir}/\${PACKAGE}"
gimv_icondir="\${datadir}/\${PACKAGE}/pixmaps"
AC_SUBST(docdir)
AC_SUBST(gimv_datadir)
AC_SUBST(gimv_docdir)
AC_SUBST(gimv_icondir)

dnl Plugin directory
plugindir="\${libdir}/\${PACKAGE}"
ARCHIVER_PLUGIN_DIR="archiver"
IMAGE_LOADER_PLUGIN_DIR="image_loader"
IMAGE_SAVER_PLUGIN_DIR="image_saver"
IO_STREAM_PLUGIN_DIR="io_stream"
IMAGE_VIEW_PLUGIN_DIR="image_view"
THUMBNAIL_VIEW_PLUGIN_DIR="thumbnail_view"
THUMBNAIL_PLUGIN_DIR="thumbnail"

AC_SUBST(plugindir)
AC_SUBST(ARCHIVER_PLUGIN_DIR)
AC_SUBST(IMAGE_LOADER_PLUGIN_DIR)
AC_SUBST(IMAGE_SAVER_PLUGIN_DIR)
AC_SUBST(IO_STREAM_PLUGIN_DIR)
AC_SUBST(IMAGE_VIEW_PLUGIN_DIR)
AC_SUBST(THUMBNAIL_VIEW_PLUGIN_DIR)
AC_SUBST(THUMBNAIL_PLUGIN_DIR)


AC_OUTPUT([
Makefile
doc/Makefile
doc/ja/Makefile
etc/Makefile
m4/Makefile
po/Makefile.in
lib/Makefile
lib/dllloader/Makefile
lib/libexif/Makefile
plugins/Makefile
plugins/archiver/Makefile
plugins/image_loader/Makefile
plugins/image_saver/Makefile
plugins/image_view/Makefile
plugins/io_stream/Makefile
plugins/thumbnail/Makefile
plugins/thumbnail_view/Makefile
src/Makefile
src/cursors/Makefile
src/pixmaps/Makefile
src/prefs_ui/Makefile
gimageview.spec
])


dnl
dnl Show some info:
dnl

AC_MSG_RESULT([
Configure Result :

   C Compiler    : ${CC}
   CFLAGS        : ${CFLAGS}
   Install path  : ${prefix}
   JPEG (libjpg) : ${enable_jpeg}
   MNG  (libmng) : ${enable_mng}
   SVG  (librsvg): ${enable_librsvg}
   WMF  (libwmf) : ${enable_libwmf}
   Movie Support : ${movie_support}
   EXIF          : ${enable_exif}
   Susie plugin  : ${enable_spi}
   Show splash   : ${enable_splash}
])
