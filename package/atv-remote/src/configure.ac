#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(atvclient,0.1,christoph.cantillon@roots.be)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_LANG_C

# Checks for libraries.
PKG_CHECK_MODULES(LIBUSB, libusb)
AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_CHECK_FUNCS([memset strtol])

AC_OUTPUT(Makefile src/Makefile)
