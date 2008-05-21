dnl Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
dnl 
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or 
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl ===
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT([E-MailRelay],[1.8.1],,[emailrelay])
AC_CONFIG_SRCDIR(src/gsmtp/gsmtp.h)
AM_INIT_AUTOMAKE([no-define])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl ===
dnl check for programs...
dnl
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_GREP
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
AC_SEARCH_LIBS(compress2,z)

dnl ===
dnl check for header files, functions and typedefs...
dnl
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(zlib.h)
AC_CHECK_FUNCS(glob)
AC_LANG_CPLUSPLUS
ACLOCAL_TYPE_SOCKLEN_T
ACLOCAL_CHECK_IPV6
ACLOCAL_CHECK_GETIPNODEBYNAME
ACLOCAL_CHECK_SIN6_LEN
ACLOCAL_CHECK_BUGGY_CTIME
ACLOCAL_CHECK_GMTIME_R
ACLOCAL_CHECK_LOCALTIME_R
ACLOCAL_CHECK_SETGROUPS
ACLOCAL_CHECK_QT4

dnl ===
dnl initialise aclocal/pkg.m4...
dnl
PKG_PROG_PKG_CONFIG(0.9.0)

dnl ===
dnl "--enable-debug"
dnl
AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[enable extra debug messages at compile-time (default no)]))
ENABLE_DEBUG

dnl ===
dnl "--enable-ipv6"
dnl
AC_ARG_ENABLE(ipv6,AC_HELP_STRING([--enable-ipv6],[enable ipv6 (default no)]))
ENABLE_IPV6

dnl ===
dnl "--enable-gui"
dnl
AC_ARG_ENABLE(gui,AC_HELP_STRING([--enable-gui],[enable configuration gui (requires Qt4) (default auto)]))
ENABLE_GUI

dnl ===
dnl "--enable-verbose"
dnl
AC_ARG_ENABLE(verbose,AC_HELP_STRING([--enable-verbose],[enable verbose logging (default yes)]))
ENABLE_VERBOSE

dnl ===
dnl "--enable-pop"
dnl
AC_ARG_ENABLE(pop,AC_HELP_STRING([--enable-pop],[enable pop3 protocol (default yes)]))
ENABLE_POP

dnl ===
dnl "--enable-exec"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-exec],[enable running of external programs (disable-exec requires disable-gui) (default yes)]))
ENABLE_EXEC

dnl ===
dnl "--enable-admin"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-admin],[enable admin interface (default yes)]))
ENABLE_ADMIN

dnl ===
dnl "--enable-auth"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-auth],[enable authentication (disable-auth requires disable-pop) (default yes)]))
ENABLE_AUTH

dnl ===
dnl "--enable-dns"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-dns],[enable dns lookup (default yes)]))
ENABLE_DNS

dnl ===
dnl "--enable-identity"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-identity],[enable process userid switching (default yes)]))
ENABLE_IDENTITY

dnl ===
dnl "--enable-small-config"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-small-config],[enable simplified confuration code for smaller binaries (rtfm) (default no)]))
ENABLE_SMALL_CONFIG

dnl ===
dnl "--enable-small-fragments"
dnl
AC_ARG_ENABLE(exec,AC_HELP_STRING([--enable-small-fragments],[enable fragmented object files for smaller binaries (default no)]))
ENABLE_SMALL_FRAGMENTS

dnl ===
dnl "--enable-small-exceptions"
dnl
AC_ARG_ENABLE(small_exceptions,AC_HELP_STRING([--enable-small-exceptions],[optimise exception classes for smaller binaries (default no)]))
ENABLE_SMALL_EXCEPTIONS

dnl ===
dnl "--enable-proxy"
dnl
AC_ARG_ENABLE(proxy,AC_HELP_STRING([--enable-proxy],[enable proxying (default yes)]))
ENABLE_PROXY

dnl ===
dnl "--enable-mac"
dnl
AC_ARG_ENABLE(mac,AC_HELP_STRING([--enable-mac],[enable building for a mac os x target (default auto)]))
ENABLE_MAC

dnl ===
dnl "--with-doxygen"
dnl
AC_ARG_WITH(doxygen,AC_HELP_STRING([--with-doxygen],[generate source code documentation with doxygen (default auto)]))
WITH_DOXYGEN

dnl ===
dnl "--with-man2html"
dnl
AC_ARG_WITH(man2html,AC_HELP_STRING([--with-man2html],[convert man pages to html using man2html (default auto)]))
WITH_MAN2HTML

dnl ===
dnl "--with-openssl"
dnl
AC_ARG_WITH(openssl,AC_HELP_STRING([--with-openssl],[use openssl for smtp client tls extension (default auto)]))
WITH_OPENSSL

dnl ===
dnl "--with-glob"
dnl
AC_ARG_WITH(glob,AC_HELP_STRING([--with-glob],[use glob() for reading directories (default auto)]))
WITH_GLOB

dnl ===
dnl "--enable-static-linking"
dnl
AC_ARG_ENABLE(static-linking,AC_HELP_STRING([--enable-static-linking],[prefer static linking for some libraries (default no)]))
ENABLE_STATIC_LINKING

dnl ===
dnl "--enable-install-hook"
dnl
AC_ARG_ENABLE(install-hook,AC_HELP_STRING([--enable-install-hook],[enable fixing up the start/stop configuration file at install time (default yes)]))
ENABLE_INSTALL_HOOK

dnl ===
dnl directory tweaking and "--enable-fhs" ...
dnl
dnl define e_ prefixed directory variables
dnl
dnl not AC_PREFIX_DEFAULT([/usr])
AC_ARG_ENABLE(fhs,AC_HELP_STRING([--enable-fhs],[force linux FHS-compliant directories, ignoring --prefix etc (default no)]))
SET_DIRECTORIES
AC_SUBST(e_docdir)
AC_SUBST(e_initdir)
AC_SUBST(e_spooldir)
AC_SUBST(e_examplesdir)
AC_SUBST(e_libexecdir)
AC_SUBST(e_sysconfdir)
AC_SUBST(e_qtmoc)
if test "$e_qtmoc" = "" ; then e_qtmoc="moc" ; fi

dnl ===
dnl generate files...
dnl
AC_OUTPUT(Makefile src/Makefile src/glib/Makefile src/gssl/Makefile src/gnet/Makefile src/gsmtp/Makefile src/gpop/Makefile src/main/Makefile src/win32/Makefile src/gui/Makefile lib/Makefile lib/gcc2.95/Makefile lib/msvc6.0/Makefile bin/Makefile doc/Makefile etc/Makefile test/Makefile extra/Makefile extra/mips/Makefile debian/Makefile src/fragments/Makefile)

