dnl Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
dnl 
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either
dnl version 2 of the License, or (at your option) any later
dnl version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl 
dnl ===
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT(src/gsmtp/gsmtp.h)
AM_INIT_AUTOMAKE(emailrelay,1.1.1)
AM_CONFIG_HEADER(config.h)

dnl ===
dnl check for programs...
dnl
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_AWK
AC_CHECK_PROGS(MAKE,make gmake)
AC_CHECK_PROGS(AR,ar gar)
AC_CHECK_PROGS(GZIP,gzip)
AC_CHECK_PROG(HAVE_DOXYGEN,doxygen,yes)
AC_CHECK_PROG(HAVE_MAN2HTML,man2html,yes)
ACLOCAL_COMPILER_VERSION

dnl ===
dnl check for libraries...
dnl
AC_LANG_C
AC_SEARCH_LIBS(gethostbyname,nsl)
AC_SEARCH_LIBS(connect,socket)

dnl ===
dnl check for header files, functions and typedefs...
dnl
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_FUNCS(glob)
AC_LANG_CPLUSPLUS
ACLOCAL_TYPE_SOCKLEN_T
ACLOCAL_CHECK_BUGGY_CTIME
ACLOCAL_CHECK_GMTIME_R
ACLOCAL_CHECK_LOCALTIME_R

dnl ===
dnl "--enable-debug"
dnl
AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[enable extra debug messages at compile-time]))
ENABLE_DEBUG

dnl ===
dnl "--with-workshop"
dnl
AC_ARG_WITH(workshop,AC_HELP_STRING([--with-workshop],[use Sun WorkShop 'CC -xar' as the 'ar' tool]))
WITH_WORKSHOP

dnl ===
dnl directory tweaking and "--enable-fhs" ...
dnl
dnl not AC_PREFIX_DEFAULT([/usr])
AC_ARG_ENABLE(fhs,AC_HELP_STRING([--enable-fhs],[force FHS-compliant directories, ignoring --prefix etc]))
AC_SUBST(e_sbindir)
if test "$e_sbindir" = "" ; then e_sbindir="$sbindir" ; fi
AC_SUBST(e_libexecdir)
if test "$e_libexecdir" = "" ; then e_libexecdir="$libexecdir" ; fi
AC_SUBST(e_docdir)
if test "$e_docdir" = "" ; then e_docdir="$datadir/$PACKAGE/doc" ; fi
AC_SUBST(e_initdir)
if test "$e_initdir" = "" ; then e_initdir="$libexecdir" ; fi
AC_SUBST(e_spooldir)
if test "$e_spooldir" = "" ; then e_spooldir="$localstatedir/spool/emailrelay" ; fi
AC_SUBST(e_man1dir)
if test "$e_man1dir" = "" ; then e_man1dir="$mandir/man1" ; fi
AC_SUBST(e_examplesdir)
if test "$e_examplesdir" = "" ; then e_examplesdir="$libexecdir" ; fi
ENABLE_FHS

dnl ===
dnl attempt to fix recursive makes where make is gmake -- may break something else
dnl
SET_MAKE=""

dnl ===
dnl generate files...
dnl
AC_OUTPUT(Makefile src/Makefile src/glib/Makefile src/gnet/Makefile src/gsmtp/Makefile src/main/Makefile src/win32/Makefile lib/Makefile lib/gcc2.95/Makefile lib/msvc6.0/Makefile lib/sunpro5/Makefile bin/Makefile doc/Makefile)

