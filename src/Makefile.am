include_HEADERS=vitamtp.h

# ---------------------------------------------------------------------------
# Advanced information about versioning:
#   * "Writing shared libraries" by Mike Hearn
#      http://plan99.net/~mike/writing-shared-libraries.html
#   * libtool.info chapter "Versioning"
#   * libtool.info chapter "Updating library version information"
# ---------------------------------------------------------------------------
# Versioning:
#  - CURRENT (Major):  Increment if the interface has changes. AGE is always
#                      *changed* at the same time.
#  - AGE (Micro):      Increment if any interfaces have been added; set to 0
#		         if any interfaces have been removed. Removal has
#                      precedence over adding, so set to 0 if both happened.
#                      It denotes upward compatibility.
#  - REVISION (Minor): Increment any time the source changes; set to
#			 0 if you incremented CURRENT.
#
#  To summarize. Any interface *change* increment CURRENT. If that interface
#  change does not break upward compatibility (ie it is an addition),
#  increment AGE, Otherwise AGE is reset to 0. If CURRENT has changed,
#  REVISION is set to 0, otherwise REVISION is incremented.
# ---------------------------------------------------------------------------
CURRENT=2
AGE=1
REVISION=0
SOVERSION=$(CURRENT):$(REVISION):$(AGE)
LT_CURRENT_MINUS_AGE=`expr $(CURRENT) - $(AGE)`

if COMPILE_MINGW32
W32_LIBS=-lws2_32
W32_LDFLAGS=-export-dynamic
if MS_LIB_EXE
noinst_DATA=libvitamtp.lib
libvitamtp.def:
	echo "LIBRARY \"@PACKAGE@\"" > libvitamtp.def
	echo "DESCRIPTION \"Vita USB communications library\"" >> libvitamtp.def
	echo "VERSION @VERSION@" >> libvitamtp.def
	echo >> libvitamtp.def
libvitamtp.lib: libvitamtp.la libvitamtp.def
	lib -name:libvitamtp-$(LT_CURRENT_MINUS_AGE).dll -def:libvitamtp.def -out:$@
install-data-local: libvitamtp.lib libvitamtp.def
	$(INSTALL) libvitamtp.def $(DESTDIR)$(libdir)
	$(INSTALL) libvitamtp.lib $(DESTDIR)$(libdir)
endif
endif

# shared library for installing
lib_LTLIBRARIES=libvitamtp.la
libvitamtp_la_SOURCES=vitamtp.h datautils.c device.c ptp.c usb.c vitamtp.c wireless.c
libvitamtp_la_CFLAGS=$(XML_CFLAGS) $(LIBUSB_CFLAGS) $(DEVICE_CFLAGS) -std=gnu99 -fgnu89-inline
libvitamtp_la_LDFLAGS=$(XML_LIBS) $(LIBUSB_LIBS) -no-undefined -export-symbols-regex "VitaMTP_[0-9A-Za-z_]+" -version-info $(SOVERSION) $(W32_LDFLAGS)
libvitamtp_la_LIBADD=$(LTLIBICONV)

if BUILD_OPENCMA
# static library for linking
noinst_LIBRARIES=libvitamtp.a
libvitamtp_a_SOURCES=$(libvitamtp_la_SOURCES)
libvitamtp_a_CFLAGS=$(libvitamtp_la_CFLAGS)

# opencma program
bin_PROGRAMS=opencma
opencma_SOURCES=opencma.h opencma.c database.c utilities.c
opencma_CFLAGS=$(XML_CFLAGS) $(LIBUSB_CFLAGS) $(PTHREAD_CFLAGS) $(DEVICE_CFLAGS) -std=gnu99 -fgnu89-inline
opencma_LDFLAGS=$(XML_LIBS) $(LIBUSB_LIBS) $(LIBICONV) $(PTHREAD_LIBS)
if STATIC_OPENCMA
opencma_LDADD=libvitamtp.a
else
opencma_LDADD=libvitamtp.la
endif
endif

EXTRA_DIST=ptp-pack.c device-flags.h ptp.h
DISTCLEANFILES=_stdint.h gphoto2-endian.h
