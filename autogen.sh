#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

libtoolize --copy --force \
  && aclocal -I m4 \
  && autoheader \
  && automake --add-missing --foreign --copy \
  && autoconf
