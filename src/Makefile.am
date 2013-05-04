SUBDIRS=

# udev rules to prevent need of root
if USE_LINUX
SUBDIRS+=linux
endif

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
CURRENT=1
AGE=1
REVISION=2
SOVERSION=$(CURRENT):$(REVISION):$(AGE)
LT_CURRENT_MINUS_AGE=`expr $(CURRENT) - $(AGE)`

# shared library for installing
lib_LTLIBRARIES=libvitamtp.la
libvitamtp_la_SOURCES=datautils.c device.c ptp.c vitamtp.c
libvitamtp_la_CFLAGS=-std=gnu99 -fgnu89-inline
libvitamtp_la_LDFLAGS=-no-undefined -export-symbols-regex "VitaMTP_[0-9A-Za-z_]+" -version-info $(SOVERSION)

if BUILD_OPENCMA
# static library for linking
noinst_LIBRARIES=libvitamtp.a
libvitamtp_a_SOURCES=$(libvitamtp_la_SOURCES)
libvitamtp_a_CFLAGS=$(libvitamtp_la_CFLAGS)

# opencma program
bin_PROGRAMS=opencma
opencma_SOURCES=opencma.c database.c utilities.c
opencma_CFLAGS=$(PTHREAD_CFLAGS) -std=gnu99 -fgnu89-inline
opencma_LDFLAGS=$(PTHREAD_LIBS)
if STATIC_OPENCMA
opencma_LDADD=libvitamtp.a
else
opencma_LDADD=libvitamtp.la
endif
endif

DISTCLEANFILES = _stdint.h gphoto2-endian.h
