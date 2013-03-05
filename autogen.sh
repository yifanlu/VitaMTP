#!/bin/sh
#set -e

srcdir=`dirname $0`

ACLOCAL_FLAGS="-I ${srcdir}/m4 ${ACLOCAL_FLAGS}"

fail() {
    status=$?
    echo "Last command failed with status $status in directory $(pwd)."
    echo "Aborting"
    exit $status
}

# Refresh GNU autotools toolchain: libtool
echo "Removing libtool cruft"
rm -f ltmain.sh config.guess config.sub
echo "Running libtoolize"
libtoolize --copy --force || fail

# Refresh GNU autotools toolchain: aclocal autoheader
echo "Removing aclocal cruft"
rm -f aclocal.m4
echo "Running aclocal $ACLOCAL_FLAGS"
aclocal $ACLOCAL_FLAGS || fail
echo "Removing autoheader cruft"
rm -f config.h.in src/config.h.in
echo "Running autoheader"
autoheader || fail

# Refresh GNU autotools toolchain: automake
echo "Removing automake cruft"
rm -f depcomp install-sh missing mkinstalldirs
rm -f stamp-h*
echo "Running automake"
touch config.rpath
automake --add-missing --gnu || fail

# Refresh GNU autotools toolchain: autoconf
echo "Removing autoconf cruft"
rm -f configure
rm -rf autom4te*.cache/
echo "Running autoconf"
autoconf

echo "Finished!"

