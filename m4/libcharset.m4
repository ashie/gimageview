# Macro to add for using libcharset.
# Takuro Ashie <ashie@homa.ne.jp>, 2002.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License or the GNU Library General Public License but which still want
# to provide support for the GNU gettext functionality.
# Please note that the actual code of the GNU gettext library is covered
# by the GNU Library General Public License, and the rest of the GNU
# gettext package package is covered by the GNU General Public License.
# They are *not* in the public domain.

AC_DEFUN([AM_LIBCHARSET],
[
  AC_ARG_WITH([libcharset-prefix],
[  --with-libcharset-prefix=DIR  search for libcharset in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])

  AC_CACHE_CHECK(for libcharset, am_cv_libcharset, [
    am_cv_libcharset=no
    am_save_LIBS="$LIBS"
    LIBS="$LIBS -lcharset"
    AC_TRY_LINK([#include <libcharset.h>],
      [const char *charset = locale_charset ();],
      am_cv_libcharset=yes)
    LIBS="$am_save_LIBS"
  ])

  LIBCHARSET=
  if test "$am_cv_libcharset" = yes; then
    AC_DEFINE(HAVE_LIBCHARSET, 1, [Define if you have the locale_charset() function.])
    LIBCHARSET="-lcharset"
  fi
  AC_SUBST(LIBCHARSET)
])
