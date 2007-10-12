#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

m4dir="m4"

#if test -d $srcdir/$m4dir; then
#    rm -f $srcdir/acinclude.m4
#    for m4f in $srcdir/$m4dir/*.m4; do
#       cat $m4f >> acinclude.m4
#    done
#else
#    echo "Directory '$m4dir' is missing."
#    exit 1
#fi

libtoolize --copy --force \
  && aclocal -I m4 \
  && autoheader \
  && automake --add-missing --foreign --copy \
  && autoconf
