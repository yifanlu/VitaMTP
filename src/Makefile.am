include_HEADERS=vitamtp.h

# static library for linking
noinst_LIBRARIES=libvitamtp.a
libvitamtp_a_SOURCES=vitamtp.c vitacma.c xmlparsing.c
libvitamtp_a_CFLAGS=$(MTP_CFLAGS) -std=c99

# shared library for installing
lib_LTLIBRARIES=libvitamtp.la
libvitamtp_la_SOURCES=vitamtp.c vitacma.c xmlparsing.c
libvitamtp_la_LIBADD=$(LIBS) $(MTP_LIBS)
libvitamtp_la_CFLAGS=$(MTP_CFLAGS) -std=c99

# opencma program
bin_PROGRAMS=opencma
opencma_SOURCES=opencma.c database.c utilities.c
opencma_CFLAGS=$(MTP_CFLAGS) -std=c99 
opencma_LDADD=libvitamtp.a $(MTP_STATIC_LIBS) $(LIBS)
opencma_LDFLAGS=-static-libtool-libs
