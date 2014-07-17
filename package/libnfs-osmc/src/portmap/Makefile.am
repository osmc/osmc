noinst_LTLIBRARIES = libportmap.la

portmap_SOURCES_GENERATED = libnfs-raw-portmap.c
portmap_HEADERS_GENERATED = libnfs-raw-portmap.h
portmap_GENERATED = $(portmap_SOURCES_GENERATED) $(portmap_HEADERS_GENERATED)

CLEANFILES = $(portmap_GENERATED) portmap-stamp

libportmap_la_CPPFLAGS = -I$(abs_top_srcdir)/include
libportmap_la_SOURCES = \
	$(portmap_SOURCES_GENERATED) \
	portmap.c

$(portmap_GENERATED) : portmap-stamp
portmap-stamp : portmap.x
	rm -f $(portmap_GENERATED)
	rpcgen -h @RPCGENFLAGS@ $< > libnfs-raw-portmap.h
	rpcgen -c @RPCGENFLAGS@ $< | sed -e "s/#include \".*portmap.h\"/#include \"libnfs-raw-portmap.h\"/" > libnfs-raw-portmap.c
	touch portmap-stamp
	
