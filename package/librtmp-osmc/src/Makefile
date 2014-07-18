VERSION=v2.4

prefix=/usr/local

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

SYS=posix
#SYS=mingw

CRYPTO=OPENSSL
#CRYPTO=POLARSSL
#CRYPTO=GNUTLS
LIBZ=-lz
LIB_GNUTLS=-lgnutls -lhogweed -lnettle -lgmp $(LIBZ)
LIB_OPENSSL=-lssl -lcrypto $(LIBZ)
LIB_POLARSSL=-lpolarssl $(LIBZ)
CRYPTO_LIB=$(LIB_$(CRYPTO))
DEF_=-DNO_CRYPTO
CRYPTO_DEF=$(DEF_$(CRYPTO))

DEF=-DRTMPDUMP_VERSION=\"$(VERSION)\" $(CRYPTO_DEF) $(XDEF)
OPT=-O2
CFLAGS=-Wall $(XCFLAGS) $(INC) $(DEF) $(OPT)
LDFLAGS=-Wall $(XLDFLAGS)

bindir=$(prefix)/bin
sbindir=$(prefix)/sbin
mandir=$(prefix)/man

BINDIR=$(DESTDIR)$(bindir)
SBINDIR=$(DESTDIR)$(sbindir)
MANDIR=$(DESTDIR)$(mandir)

LIBS_posix=
LIBS_darwin=
LIBS_mingw=-lws2_32 -lwinmm -lgdi32
LIB_RTMP=-Llibrtmp -lrtmp
LIBS=$(LIB_RTMP) $(CRYPTO_LIB) $(LIBS_$(SYS)) $(XLIBS)

THREADLIB_posix=-lpthread
THREADLIB_darwin=-lpthread
THREADLIB_mingw=
THREADLIB=$(THREADLIB_$(SYS))
SLIBS=$(THREADLIB) $(LIBS)

LIBRTMP=librtmp/librtmp.a
INCRTMP=librtmp/rtmp_sys.h librtmp/rtmp.h librtmp/log.h librtmp/amf.h

EXT_posix=
EXT_darwin=
EXT_mingw=.exe
EXT=$(EXT_$(SYS))

PROGS=rtmpdump rtmpgw rtmpsrv rtmpsuck

all:	$(LIBRTMP) $(PROGS)

$(PROGS): $(LIBRTMP)

install:	$(PROGS)
	-mkdir -p $(BINDIR) $(SBINDIR) $(MANDIR)/man1 $(MANDIR)/man8
	cp rtmpdump$(EXT) $(BINDIR)
	cp rtmpgw$(EXT) rtmpsrv$(EXT) rtmpsuck$(EXT) $(SBINDIR)
	cp rtmpdump.1 $(MANDIR)/man1
	cp rtmpgw.8 $(MANDIR)/man8
	@cd librtmp; $(MAKE) install

clean:
	rm -f *.o rtmpdump$(EXT) rtmpgw$(EXT) rtmpsrv$(EXT) rtmpsuck$(EXT)
	@cd librtmp; $(MAKE) clean

FORCE:

$(LIBRTMP): FORCE
	@cd librtmp; $(MAKE) all

rtmpdump: rtmpdump.o
	$(CC) $(LDFLAGS) -o $@$(EXT) $@.o $(LIBS)

rtmpsrv: rtmpsrv.o thread.o
	$(CC) $(LDFLAGS) -o $@$(EXT) $@.o thread.o $(SLIBS)

rtmpsuck: rtmpsuck.o thread.o
	$(CC) $(LDFLAGS) -o $@$(EXT) $@.o thread.o $(SLIBS)

rtmpgw: rtmpgw.o thread.o
	$(CC) $(LDFLAGS) -o $@$(EXT) $@.o thread.o $(SLIBS)

rtmpgw.o: rtmpgw.c $(INCRTMP) Makefile
rtmpdump.o: rtmpdump.c $(INCRTMP) Makefile
rtmpsrv.o: rtmpsrv.c $(INCRTMP) Makefile
rtmpsuck.o: rtmpsuck.c $(INCRTMP) Makefile
thread.o: thread.c thread.h
