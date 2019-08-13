dnl Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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

AC_INIT([E-MailRelay],[2.0.1],[],[emailrelay])
AC_CONFIG_SRCDIR([src/gsmtp/gsmtp.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([no-define])
AC_CONFIG_HEADERS([gconfig_defs.h])
AM_MAINTAINER_MODE
AC_DISABLE_OPTION_CHECKING

dnl ===
dnl check for programs...
dnl
AC_PROG_CC([cc gcc clang])
AC_PROG_CXX([c++ g++ clang++])
AX_CXX_COMPILE_STDCXX_11([],[optional])
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_SED
AC_PROG_GREP
AC_CHECK_PROGS(MAKE,make gmake)
AC_CHECK_PROGS(AR,ar gar)
AC_CHECK_PROGS(GZIP,gzip)
AC_CHECK_PROG(GCONFIG_HAVE_DOXYGEN,doxygen,yes)
AC_CHECK_PROG(GCONFIG_HAVE_MAN2HTML,man2html,yes)
GCONFIG_FN_PROG_WINDRES
GCONFIG_FN_PROG_WINDMC
GCONFIG_FN_CONFIGURATION

dnl ===
dnl check for libraries...
dnl
AC_LANG([C])
GCONFIG_FN_SEARCHLIBS_POSIX

dnl ===
dnl check for header files, functions and typedefs...
dnl
AC_LANG([C++])
GCONFIG_FN_TYPE_SOCKLEN_T
GCONFIG_FN_TYPE_ERRNO_T
GCONFIG_FN_TYPE_SSIZE_T
GCONFIG_FN_CXX_NULLPTR
GCONFIG_FN_CXX_CONSTEXPR
GCONFIG_FN_CXX_NOEXCEPT
GCONFIG_FN_CXX_OVERRIDE
GCONFIG_FN_CXX_FINAL
GCONFIG_FN_CXX_TYPE_TRAITS
GCONFIG_FN_CXX_EMPLACE
GCONFIG_FN_CXX_ALIGNMENT
GCONFIG_FN_CXX_SHARED_PTR
GCONFIG_FN_CXX_STD_THREAD
GCONFIG_FN_CXX_STD_WSTRING
GCONFIG_FN_IPV6
GCONFIG_FN_SIN6_LEN
GCONFIG_FN_STATBUF_NSEC
GCONFIG_FN_INET_NTOP
GCONFIG_FN_INET_PTON
GCONFIG_FN_GETPWNAM
GCONFIG_FN_GETPWNAM_R
GCONFIG_FN_GMTIME_R
GCONFIG_FN_GMTIME_S
GCONFIG_FN_LOCALTIME_R
GCONFIG_FN_LOCALTIME_S
GCONFIG_FN_STRNCPY_S
GCONFIG_FN_SETPGRP_BSD
GCONFIG_FN_SETGROUPS
GCONFIG_FN_GETENV_S
GCONFIG_FN_READLINK
GCONFIG_FN_ICONV
GCONFIG_FN_PROC_PIDPATH
GCONFIG_FN_QT
GCONFIG_FN_TLS_OPENSSL
GCONFIG_FN_TLS_MBEDTLS

dnl ===
dnl initialise aclocal/pkg.m4...
dnl
# if the configure script fails in this area then you are probably missing pkg.m4
PKG_PROG_PKG_CONFIG(0.9.0)

dnl ===
dnl "--enable-std-thread"
dnl
AC_ARG_ENABLE(std-thread,AS_HELP_STRING([--enable-std-thread],[use std::thread or not (default auto)]))
GCONFIG_FN_ENABLE_STD_THREAD

dnl ===
dnl "--enable-debug"
dnl
AC_ARG_ENABLE(debug,AS_HELP_STRING([--enable-debug],[enable extra debug messages at compile-time (default no)]))
GCONFIG_FN_ENABLE_DEBUG

dnl ===
dnl "--enable-ipv6"
dnl
AC_ARG_ENABLE(ipv6,AS_HELP_STRING([--enable-ipv6],[enable ipv6 (default auto)]))
GCONFIG_FN_ENABLE_IPV6

dnl ===
dnl "--enable-gui"
dnl
AC_ARG_ENABLE(gui,AS_HELP_STRING([--enable-gui],[enable configuration gui (requires Qt) (default auto)]))
GCONFIG_FN_ENABLE_GUI

dnl ===
dnl "--enable-verbose"
dnl
AC_ARG_ENABLE(verbose,AS_HELP_STRING([--enable-verbose],[enable verbose logging (default yes)]))
GCONFIG_FN_ENABLE_VERBOSE

dnl ===
dnl "--enable-bsd"
dnl
AC_ARG_ENABLE(bsd,AS_HELP_STRING([--enable-bsd],[enable building for bsd (default auto)]))
GCONFIG_FN_ENABLE_BSD

dnl ===
dnl "--enable-mac"
dnl
AC_ARG_ENABLE(mac,AS_HELP_STRING([--enable-mac],[enable building for mac os x (default auto)]))
GCONFIG_FN_ENABLE_MAC

dnl ===
dnl "--enable-windows"
dnl
AC_ARG_ENABLE(windows,AS_HELP_STRING([--enable-windows],[enable building for windows (default auto)]))
GCONFIG_FN_ENABLE_WINDOWS

dnl ===
dnl "--enable-testing"
dnl
AC_ARG_ENABLE(testing,AS_HELP_STRING([--enable-testing],[enable make check tests (default yes)]))
GCONFIG_FN_ENABLE_TESTING

dnl ===
dnl "--with-doxygen"
dnl
AC_ARG_WITH(doxygen,AS_HELP_STRING([--with-doxygen],[generate source code documentation with doxygen (default auto)]))
GCONFIG_FN_WITH_DOXYGEN

dnl ===
dnl "--with-man2html"
dnl
AC_ARG_WITH(man2html,AS_HELP_STRING([--with-man2html],[convert man pages to html using man2html (default auto)]))
GCONFIG_FN_WITH_MAN2HTML

dnl ===
dnl "--with-openssl"/"--with-mbedtls"
dnl
AC_ARG_WITH(openssl,AS_HELP_STRING([--with-openssl],[use openssl for tls layer (default auto)]))
AC_ARG_WITH(mbedtls,AS_HELP_STRING([--with-mbedtls],[use mbedtls for tls layer (default auto)]))
GCONFIG_FN_TLS

dnl ===
dnl "--with-pam"
dnl
AC_ARG_WITH(pam,AS_HELP_STRING([--with-pam],[use linux pam for authentication (default auto)]))
GCONFIG_FN_WITH_PAM

dnl ===
dnl "--enable-static-linking"
dnl
AC_ARG_ENABLE(static-linking,AS_HELP_STRING([--enable-static-linking],[prefer static linking for some libraries (default no)]))
GCONFIG_FN_ENABLE_STATIC_LINKING

dnl ===
dnl "--enable-install-hook"
dnl
AC_ARG_ENABLE(install-hook,AS_HELP_STRING([--enable-install-hook],[enable fixing up the start/stop configuration file at install time (default yes)]))
GCONFIG_FN_ENABLE_INSTALL_HOOK

dnl ===
dnl directory tweaking ...
dnl
dnl define e_ prefixed directory variables
dnl
dnl not AC_PREFIX_DEFAULT([/usr])
GCONFIG_FN_SET_DIRECTORIES

dnl ===
dnl generate files...
dnl
AC_CONFIG_FILES([Makefile src/Makefile src/glib/Makefile src/gssl/Makefile src/gnet/Makefile src/gauth/Makefile src/gsmtp/Makefile src/gpop/Makefile src/main/Makefile src/main/icon/Makefile src/win32/Makefile src/gui/Makefile bin/Makefile doc/Makefile etc/Makefile test/Makefile test/certificates/Makefile m4/Makefile bsd/Makefile debian/Makefile])
AC_OUTPUT

