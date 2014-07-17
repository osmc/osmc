noinst_LTLIBRARIES = librquota.la

rquota_SOURCES_GENERATED = libnfs-raw-rquota.c
rquota_HEADERS_GENERATED = libnfs-raw-rquota.h
rquota_GENERATED = $(rquota_SOURCES_GENERATED) $(rquota_HEADERS_GENERATED)

CLEANFILES = $(rquota_GENERATED) rquota-stamp

librquota_la_CPPFLAGS = -I$(abs_top_srcdir)/include
librquota_la_SOURCES = \
	$(rquota_SOURCES_GENERATED) \
	rquota.c

$(rquota_GENERATED) : rquota-stamp
rquota-stamp : rquota.x
	rm -f $(rquota_GENERATED)
	rpcgen -h @RPCGENFLAGS@ $< > libnfs-raw-rquota.h
	rpcgen -c @RPCGENFLAGS@ $< | sed -e "s/#include \".*rquota.h\"/#include \"libnfs-raw-rquota.h\"/" > libnfs-raw-rquota.c
	touch rquota-stamp
	
