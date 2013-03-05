lib_LTLIBRARIES=libvitamtp.la
include_HEADERS=vitamtp.h
libvitamtp_la_SOURCES=vitamtp.c vitacma.c xmlparsing.c
libvitamtp_la_LIBADD=$(MTP_LIBS) $(XML2_LIBS)
libvitamtp_la_CFLAGS=$(MTP_CFLAGS) $(XML2_CFLAGS) -std=c99
bin_PROGRAMS=opencma
opencma_SOURCES=opencma.c database.c utilities.c
opencma_LDFLAGS=-static
opencma_LDADD=libvitamtp.la
