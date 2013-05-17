#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.52])
AC_INIT([vitamtp], [2.0.0], [support@yifanlu.com])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/vitamtp.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_PROG_INSTALL
AM_ICONV
PKG_PROG_PKG_CONFIG

# Optionally enable and check for doxygen
AC_ARG_ENABLE([doxygen],
    AS_HELP_STRING([--enable-doxygen], [Build API documentation using Doxygen [default=no]]),
    [ac_enable_doxygen=$enableval], [ac_enable_doxygen=auto])
if test "x$ac_enable_doxygen" == "xyes"; then
    AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
    if test "x$HAVE_DOXYGEN" = "xfalse" -a "x$ac_enable_doxygen" = "xyes"; then
        AC_MSG_ERROR([*** API documentation explicitly requested but Doxygen not found])
    fi
    AC_MSG_NOTICE([API documentation will be generated using Doxygen])
else
    HAVE_DOXYGEN=false
    AC_MSG_NOTICE([API documentation will not be generated])
fi
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)

# Allow user to disable building OpenCMA
AC_ARG_ENABLE([opencma],
    AS_HELP_STRING([--enable-opencma[=type]], [Build type OpenCMA program (no,dynamic,static) [default=dynamic]]),
    [ac_enable_opencma=$enableval], [ac_enable_opencma=dynamic])
AM_CONDITIONAL([BUILD_OPENCMA], [test "x$ac_enable_opencma" != "xno"])
AM_CONDITIONAL([STATIC_OPENCMA], [test "x$ac_enable_opencma" != "xdynamic"])

# Device support
DEVICE_CFLAGS=
AC_ARG_ENABLE([usb-support],
    AS_HELP_STRING([--disable-usb-support], [Disable VitaMTP USB device support [default=no]]),
    [ac_enable_usb=$enableval], [ac_enable_usb=yes])
AC_ARG_ENABLE([wireless-support],
    AS_HELP_STRING([--disable-wireless-support], [Disable VitaMTP wireless device support [default=no]]),
    [ac_enable_wireless=$enableval], [ac_enable_wireless=yes])
AM_CONDITIONAL([ENABLE_USB], [test x"$ac_enable_usb" = "xyes"])
if test x"$ac_enable_usb" = "xyes" ; then
    PKG_CHECK_MODULES(LIBUSB, libusb-1.0)
    DEVICE_CFLAGS="$DEVICE_CFLAGS -DPTP_USB_SUPPORT"
    #AC_CHECK_LIB([usb-1.0], [libusb_init], [], [AC_MSG_ERROR([libusb-1.0 not found])])
    #AC_CHECK_HEADERS([libusb-1.0/libusb.h], [], [AC_MSG_ERROR([Cannot find libusb-1.0 header])])
fi
AM_CONDITIONAL([ENABLE_WIRELESS], [test x"$ac_enable_wireless" = "xyes"])
if test x"$ac_enable_wireless" = "xyes" ; then
    AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/socket.h], [], [AC_MSG_ERROR([Cannot find required networking header.])])
    AC_CHECK_FUNCS([select socket socketpair], [], [AC_MSG_ERROR([Cannot find required networking function.])])
    DEVICE_CFLAGS="$DEVICE_CFLAGS -DPTP_IP_SUPPORT"
fi
AC_SUBST(DEVICE_CFLAGS)

# Checks for XML.
PKG_CHECK_MODULES(XML, libxml-2.0)
#AC_CHECK_LIB([xml2], [xmlReadMemory], [], [AC_MSG_ERROR([libxml2 not found])])
#AC_CHECK_HEADERS([libxml/parser.h libxml/xmlmemory.h libxml/xmlwriter.h], [], [AC_MSG_ERROR([Cannot find libxml2 header])])

# Check for pthreads
ACX_PTHREAD
PTHREAD_CFLAGS="$PTHREAD_CFLAGS -D_REENTRANT -D_THREAD_SAFE -D_POSIX_PTHREAD_SEMANTICS"
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

# Checks for additional headers
AC_CHECK_HEADERS([errno.h fcntl.h iconv.h limits.h memory.h stdarg.h stddef.h stdlib.h string.h sys/statvfs.h time.h unistd.h], [], [AC_MSG_ERROR([Cannot find required header.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_STAT
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memset mkdir strchr strcspn strdup strrchr strstr strtol], [], [AC_MSG_ERROR([Cannot find required function.])])

# Output
# Required for gphoto-ptp2
AX_NEED_STDINT_H([src/_stdint.h])
AC_NEED_BYTEORDER_H(src/gphoto2-endian.h)

AC_CONFIG_FILES([doc/Doxyfile Makefile doc/Makefile src/Makefile libvitamtp.pc])

AC_OUTPUT
