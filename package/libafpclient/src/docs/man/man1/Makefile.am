# Makefile.am for man/man1/

SUFFIXES= .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$< >$@

GENERATED_MANS	= afpcmd.1
TEMPLATE_FILES	= afpcmd.1.tmpl
NONGENERATED_MANS	=	
man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS)

