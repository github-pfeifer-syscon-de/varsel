dnl Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR([m4])

AC_INIT([varsel],[0.1],[varsel@pfeifer-syscon.de])

dnl point to unique-file-in-source-dir, for safety check
AC_CONFIG_SRCDIR([src/VarselApp.cpp])

dnl use -no-portablitity to stop complaining about $(shell... in res/Makefile.am
dnl i dont get it, as the shell is everywhere, turtle power!
AM_INIT_AUTOMAKE([1.11 foreign])

AC_CONFIG_HEADERS([config.h])

AM_SILENT_RULES([yes])

AX_CHECK_ENABLE_DEBUG([info], [DEBUG], [RELEASE], [IS-RELEASE])
AC_PROG_CC
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([20], [noext], [optional])

LT_INIT

AC_CANONICAL_HOST
AC_MSG_NOTICE([host_os $host_os])

dnl don't repeat dependencies
dnl PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])

PKG_CHECK_MODULES([GENERICIMG],[genericimg >= 0.4.3])

PKG_CHECK_MODULES([VTE],[vte-2.91 >= 0.78.0])

PKG_CHECK_MODULES([SRCVIEW],[gtksourceview-4 >= 4.8])

PKG_CHECK_MODULES([ARCHIVE],[libarchive >= 3.7.0])

PKG_CHECK_MODULES([LIBGIT2],[libgit2 >= 1.8.0])

AM_GNU_GETTEXT([external])

if test "$host_os" = "mingw32" ; then
    AC_SUBST(EXTRA_LDFLAGS, "-mwindows")
fi

AC_CONFIG_FILES([
po/Makefile.in
Makefile
res/Makefile
resEdit/Makefile
resList/Makefile
srcLib/Makefile
srcEdit/Makefile
srcList/Makefile
src/Makefile
test/Makefile
])
AC_OUTPUT
