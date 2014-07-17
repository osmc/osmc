noinst_LTLIBRARIES = libnfs.la

nfs_SOURCES_GENERATED = libnfs-raw-nfs.c
nfs_HEADERS_GENERATED = libnfs-raw-nfs.h
nfs_GENERATED = $(nfs_SOURCES_GENERATED) $(nfs_HEADERS_GENERATED)

CLEANFILES = $(nfs_GENERATED) nfs-stamp

libnfs_la_CPPFLAGS = -I$(abs_top_srcdir)/include
libnfs_la_SOURCES = \
	$(nfs_SOURCES_GENERATED) \
	nfs.c nfsacl.c

$(nfs_GENERATED) : nfs-stamp
nfs-stamp : nfs.x
	rm -f $(nfs_GENERATED)
	rpcgen -h @RPCGENFLAGS@ $< > libnfs-raw-nfs.h
	rpcgen -c @RPCGENFLAGS@ $< | sed -e "s/#include \".*nfs.h\"/#include \"libnfs-raw-nfs.h\"/" > libnfs-raw-nfs.c
	touch nfs-stamp
	
