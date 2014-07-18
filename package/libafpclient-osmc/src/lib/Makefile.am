

libafpclient_la_CFLAGS = -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/include @CFLAGS@

lib_LTLIBRARIES = libafpclient.la

libafpclient_adir = $(includedir)/afpfs-ng

libafpclient_a_HEADERS = $(top_srcdir)/include/afp.h \
                         $(top_srcdir)/include/codepage.h \
                         $(top_srcdir)/include/libafpclient.h \
                         $(top_srcdir)/include/midlevel.h \
                         $(top_srcdir)/include/utils.h \
                         $(top_srcdir)/include/afp_protocol.h \
                         $(top_srcdir)/include/dsi.h \
                         $(top_srcdir)/include/map_def.h \
                         $(top_srcdir)/include/uams_def.h

libafpclient_la_SOURCES = afp.c codepage.c did.c dsi.c map_def.c uams.c uams_def.c unicode.c users.c utils.c resource.c log.c client.c server.c connect.c loop.c midlevel.c proto_attr.c proto_desktop.c proto_directory.c proto_files.c proto_fork.c proto_login.c proto_map.c proto_replyblock.c proto_server.c proto_volume.c proto_session.c afp_url.c status.c forklist.c debug.c lowlevel.c

# libafpclient_la_LDFLAGS = -module -avoid-version


