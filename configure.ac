#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(vitamtp, 1.0, support@yifanlu.com)
AC_OUTPUT(Makefile src/Makefile)
AC_CONFIG_SRCDIR([src/vitamtp.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES(MTP, libmtp, found_libmtp=yes, found_libmtp=no)
AC_SUBST(MTP_CFLAGS)
AC_SUBST(MTP_LIBS)
if test x"$found_libmtp" = "xno" ; then
   AC_MSG_ERROR([Cannot find libmtp, please install it and make sure pkg-config can see it.])
fi
PKG_CHECK_MODULES(XML2, libxml-2.0, found_libxml2=yes, found_libxml2=no)
AC_SUBST(XML2_CFLAGS)
AC_SUBST(XML2_LIBS)
if test x"$found_libxml2" = "xno" ; then
   AC_MSG_ERROR([Cannot find libxml-2.0, please install it and make sure pkg-config can see it.])
fi

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/statvfs.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([getcwd memset mkdir strchr strcspn strdup strrchr strstr])

AC_OUTPUT
