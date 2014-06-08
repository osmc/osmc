bin_PROGRAMS = afpfsd mount_afp

mount_afp_SOURCES = client.c
mount_afp_CFLAGS = -I$(top_srcdir)/include -D_FILE_OFFSET_BITS=64 @CFLAGS@
mount_afp_LDADD = $(top_builddir)/lib/libafpclient.la

afpfsd_SOURCES = commands.c daemon.c fuse_int.c fuse_error.c
afpfsd_LDADD = $(top_builddir)/lib/libafpclient.la -lfuse
afpfsd_LDFLAGS = -export-dynamic -lfuse
afpfsd_CFLAGS = -I$(top_srcdir)/include -D_FILE_OFFSET_BITS=64 @CFLAGS@

install-data-hook:
	mkdir -p $(DESTDIR)/$(mandir)/man1
	cp mount_afp.1 $(DESTDIR)/$(mandir)/man1
	cp afpfsd.1 $(DESTDIR)/$(mandir)/man1
	cp afp_client.1 $(DESTDIR)/$(mandir)/man1

install-exec-hook:
	rm -f $(DESTDIR)$(bindir)/afp_client
	$(LN_S) mount_afp $(DESTDIR)$(bindir)/afp_client
	cp afpfs $(DESTDIR)$(bindir)/afpfs
