noinst_LTLIBRARIES = libmount.la

mount_SOURCES_GENERATED = libnfs-raw-mount.c
mount_HEADERS_GENERATED = libnfs-raw-mount.h
mount_GENERATED = $(mount_SOURCES_GENERATED) $(mount_HEADERS_GENERATED)

CLEANFILES = $(mount_GENERATED) mount-stamp

libmount_la_CPPFLAGS = -I$(abs_top_srcdir)/include
libmount_la_SOURCES = \
	$(mount_SOURCES_GENERATED) \
	mount.c

$(mount_GENERATED) : mount-stamp
mount-stamp : mount.x
	rm -f $(mount_GENERATED)
	rpcgen -h @RPCGENFLAGS@ $< > libnfs-raw-mount.h
	rpcgen -c @RPCGENFLAGS@ $< | sed -e "s/#include \".*mount.h\"/#include \"libnfs-raw-mount.h\"/" > libnfs-raw-mount.c
	touch mount-stamp
	
