# Configure paths for libwmf
# Takuro Ashie 2003-01-06
# stolen from Elliot Lee 2000-01-10
# stolen from Raph Levien 98-11-18
# stolen from Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_LIBWMF([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libwmf, and define LIBWMF_CFLAGS and LIBWMF_LIBS
dnl
AC_DEFUN([AM_PATH_LIBWMF],
[dnl 
dnl Get the cflags and libraries from the libwmf-config script
dnl
AC_ARG_WITH(libwmf-prefix,[  --with-libwmf-prefix=PFX   Prefix where libwmf is installed (optional)],
            libwmf_prefix="$withval", libwmf_prefix="")
AC_ARG_WITH(libwmf-exec-prefix,[  --with-libwmf-exec-prefix=PFX Exec prefix where libwmf is installed (optional)],
            libwmf_exec_prefix="$withval", libwmf_exec_prefix="")
AC_ARG_ENABLE(libwmftest, [  --disable-libwmftest       Do not try to compile and run a test libwmf program],
		    , enable_libwmftest=yes)

  if test x$libwmf_exec_prefix != x ; then
     libwmf_args="$libwmf_args --exec-prefix=$libwmf_exec_prefix"
     if test x${LIBWMF_CONFIG+set} = xset ; then
        LIBWMF_CONFIG=$libwmf_exec_prefix/libwmf-config
     fi
  fi
  if test x$libwmf_prefix != x ; then
     libwmf_args="$libwmf_args --prefix=$libwmf_prefix"
     if test x${LIBWMF_CONFIG+set} = xset ; then
        LIBWMF_CONFIG=$libwmf_prefix/bin/libwmf-config
     fi
  fi

  AC_PATH_PROG(LIBWMF_CONFIG, libwmf-config, no)
  min_libwmf_version=ifelse([$1], ,0.2.8,$1)
  AC_MSG_CHECKING(for libwmf - version >= $min_libwmf_version)
  no_libwmf=""
  if test "$LIBWMF_CONFIG" = "no" ; then
    no_libwmf=yes
  else
    LIBWMF_CFLAGS=`$LIBWMF_CONFIG $libwmfconf_args --cflags`
    LIBWMF_LIBS=`$LIBWMF_CONFIG $libwmfconf_args --libs`

    libwmf_major_version=`$LIBWMF_CONFIG $libwmf_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    libwmf_minor_version=`$LIBWMF_CONFIG $libwmf_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    libwmf_micro_version=`$LIBWMF_CONFIG $libwmf_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_libwmftest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LIBWMF_CFLAGS"
      LIBS="$LIBS $LIBWMF_LIBS"
dnl
dnl Now check if the installed LIBWMF is sufficiently new. (Also sanity
dnl checks the results of libwmf-config to some extent
dnl
      rm -f conf.libwmftest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwmf/api.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.libwmftest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_libwmf_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libwmf_version");
     exit(1);
  }
  return 0;
}

],, no_libwmf=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_libwmf" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBWMF_CONFIG" = "no" ; then
       echo "*** The libwmf-config script installed by LIBWMF could not be found"
       echo "*** If LIBWMF was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBWMF_CONFIG environment variable to the"
       echo "*** full path to libwmf-config."
     else
       if test -f conf.libwmftest ; then
        :
       else
          echo "*** Could not run LIBWMF test program, checking why..."
          CFLAGS="$CFLAGS $LIBWMF_CFLAGS"
          LIBS="$LIBS $LIBWMF_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <libwmf/api.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBWMF or finding the wrong"
          echo "*** version of LIBWMF. If it is not finding LIBWMF, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBWMF was incorrectly installed"
          echo "*** or that you have moved LIBWMF since it was installed. In the latter case, you"
          echo "*** may want to edit the libwmf-config script: $LIBWMF_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LIBWMF_CFLAGS=""
     LIBWMF_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBWMF_CFLAGS)
  AC_SUBST(LIBWMF_LIBS)
  rm -f conf.libwmftest
])
