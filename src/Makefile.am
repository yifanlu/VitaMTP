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
CURRENT=0
AGE=0
REVISION=1
SOVERSION=$(CURRENT):$(REVISION):$(AGE)
LT_CURRENT_MINUS_AGE=`expr $(CURRENT) - $(AGE)`

# static library for linking
noinst_LIBRARIES=libvitamtp.a
libvitamtp_a_SOURCES=device.c ptp.c vitamtp.c vitacma.c xmlparsing.c
libvitamtp_a_CFLAGS=$(MTP_CFLAGS) -std=c99 -Os

# shared library for installing
lib_LTLIBRARIES=libvitamtp.la
libvitamtp_la_SOURCES=device.c ptp.c vitamtp.c vitacma.c xmlparsing.c
libvitamtp_la_LIBADD=$(LIBS) $(MTP_LIBS)
libvitamtp_la_CFLAGS=$(MTP_CFLAGS) -std=c99 -Os
libvitamtp_la_LDFLAGS=@OSFLAGS@ @LDFLAGS@ -no-undefined -export-symbols-regex "VitaMTP_[0-9A-Za-z_]+" -version-info $(SOVERSION)

# opencma program
bin_PROGRAMS=opencma
opencma_SOURCES=opencma.c database.c utilities.c
opencma_CFLAGS=$(MTP_CFLAGS) -std=c99 -Os
opencma_LDADD=libvitamtp.a $(MTP_STATIC_LIBS) $(LIBS)
opencma_LDFLAGS=@OSFLAGS@ -static-libtool-libs

DISTCLEANFILES = _stdint.h gphoto2-endian.h
