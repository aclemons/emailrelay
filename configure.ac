dnl Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
dnl configure.ac
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_INIT([E-MailRelay],[2.5.1rc3],[],[emailrelay])
AC_CONFIG_SRCDIR([src/glib/gdef.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([no-define])
AM_SILENT_RULES([no])
AM_EXTRA_RECURSIVE_TARGETS([extra])
AC_CONFIG_HEADERS([src/gconfig_defs.h])
AM_MAINTAINER_MODE
AC_DISABLE_OPTION_CHECKING

dnl check for programs
dnl
AC_PROG_CC([cc gcc clang])
AC_PROG_CXX([c++ g++ clang++])
AX_CXX_COMPILE_STDCXX([11],[noext],[mandatory])
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_SED
AC_PROG_GREP
AC_CHECK_PROGS([MAKE],[make gmake])
AC_CHECK_PROGS([AR],[ar gar])
AC_CHECK_PROGS([GZIP],[gzip])
AC_CHECK_PROG([GCONFIG_HAVE_DOXYGEN],[doxygen],[yes])
AC_CHECK_PROG([GCONFIG_HAVE_MAN2HTML],[man2html],[yes])
GCONFIG_FN_PROG_WINDRES
GCONFIG_FN_PROG_WINDMC
GCONFIG_FN_ARFLAGS

dnl check for system libraries
dnl
AC_LANG([C])
GCONFIG_FN_SEARCHLIBS_NAMESERVICE
GCONFIG_FN_SEARCHLIBS_SOCKET

dnl check for header files, functions, types etc.
dnl
AC_LANG([C++])
GCONFIG_FN_CHECK_HEADERS
GCONFIG_FN_CHECK_TYPES
GCONFIG_FN_CHECK_CXX
GCONFIG_FN_CHECK_NET
GCONFIG_FN_CHECK_FUNCTIONS

dnl initialise aclocal/pkg.m4
dnl
# if the configure script fails in this area then you are probably missing pkg.m4
PKG_PROG_PKG_CONFIG([0.9.0])

dnl check for packages
dnl
GCONFIG_FN_QT
GCONFIG_FN_QT_BUILD
GCONFIG_FN_TLS_OPENSSL
GCONFIG_FN_TLS_MBEDTLS

dnl "--enable-whatever" for internal features
dnl
AC_ARG_ENABLE([bsd],AS_HELP_STRING([--enable-bsd],[enable building for bsd (default auto)]))
GCONFIG_FN_ENABLE_BSD
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[enable extra debug messages at compile-time (default no)]))
GCONFIG_FN_ENABLE_DEBUG
AC_ARG_ENABLE([epoll],AS_HELP_STRING([--enable-epoll],[enable epoll (default auto)]))
GCONFIG_FN_ENABLE_EPOLL
AC_ARG_ENABLE([gui],AS_HELP_STRING([--enable-gui],[enable configuration gui (requires Qt) (default auto)]))
GCONFIG_FN_ENABLE_GUI
AC_ARG_ENABLE([install-hook],AS_HELP_STRING([--enable-install-hook],[enable fixing up the start/stop configuration file at install time (default yes)]))
GCONFIG_FN_ENABLE_INSTALL_HOOK
AC_ARG_ENABLE([interface-names],AS_HELP_STRING([--enable-interface-names],[allow network interface names for defining listening addresses (default yes)]))
GCONFIG_FN_ENABLE_INTERFACE_NAMES
AC_ARG_ENABLE([pop],AS_HELP_STRING([--enable-pop],[enable pop (default yes)]))
GCONFIG_FN_ENABLE_POP
AC_ARG_ENABLE([mac],AS_HELP_STRING([--enable-mac],[enable building for mac os x (default auto)]))
GCONFIG_FN_ENABLE_MAC
AC_ARG_ENABLE([std-thread],AS_HELP_STRING([--enable-std-thread],[use std::thread or not (default auto)]))
GCONFIG_FN_ENABLE_STD_THREAD
AC_ARG_ENABLE([testing],AS_HELP_STRING([--enable-testing],[enable make check tests (default yes)]))
GCONFIG_FN_ENABLE_TESTING
AC_ARG_ENABLE([verbose],AS_HELP_STRING([--enable-verbose],[enable verbose logging (default yes)]))
GCONFIG_FN_ENABLE_VERBOSE
AC_ARG_ENABLE([windows],AS_HELP_STRING([--enable-windows],[enable building for windows (default auto)]))
GCONFIG_FN_ENABLE_WINDOWS
AC_ARG_ENABLE([uds],AS_HELP_STRING([--enable-uds],[enable unix domain sockets (default auto)]))
GCONFIG_FN_ENABLE_UDS
AC_ARG_ENABLE([dnsbl],AS_HELP_STRING([--enable-dnsbl],[enable DNSBL (default yes)]))
GCONFIG_FN_ENABLE_DNSBL
AC_ARG_ENABLE([submission],AS_HELP_STRING([--enable-submission],[main binary is a submission tool when named emailrelay-submit (default no)]))
GCONFIG_FN_ENABLE_SUBMISSION
AC_ARG_ENABLE([admin],AS_HELP_STRING([--enable-admin],[enable admin interface (default yes)]))
GCONFIG_FN_ENABLE_ADMIN

dnl "--with-whatever" for external dependencies
dnl
AC_ARG_WITH([doxygen],AS_HELP_STRING([--with-doxygen],[generate source code documentation with doxygen (default auto)]))
GCONFIG_FN_WITH_DOXYGEN
AC_ARG_WITH([gettext],AS_HELP_STRING([--with-gettext],[use gettext (default auto)]))
GCONFIG_FN_WITH_GETTEXT
AC_ARG_WITH([man2html],AS_HELP_STRING([--with-man2html],[convert man pages to html using man2html (default auto)]))
GCONFIG_FN_WITH_MAN2HTML
AC_ARG_WITH([openssl],AS_HELP_STRING([--with-openssl],[use openssl for tls layer (default auto)]))
AC_ARG_WITH([mbedtls],AS_HELP_STRING([--with-mbedtls],[use mbedtls for tls layer (default auto)]))
GCONFIG_FN_TLS
AC_ARG_WITH([pam],AS_HELP_STRING([--with-pam],[use linux pam for authentication (default auto)]))
GCONFIG_FN_WITH_PAM

dnl directory tweaking -- define e_ prefixed directory variables
dnl
dnl not AC_PREFIX_DEFAULT([/usr])
GCONFIG_FN_SET_DIRECTORIES_E

dnl generate files
dnl
AC_CONFIG_FILES([Makefile libexec/Makefile src/Makefile src/glib/Makefile src/gssl/Makefile src/gnet/Makefile src/gauth/Makefile src/gstore/Makefile src/gsmtp/Makefile src/gpop/Makefile src/gfilters/Makefile src/gverifiers/Makefile src/main/Makefile src/main/icon/Makefile src/win32/Makefile src/gui/Makefile bin/Makefile doc/Makefile etc/Makefile test/Makefile test/certificates/Makefile m4/Makefile bsd/Makefile debian/Makefile po/Makefile unity/Makefile])
AC_OUTPUT

dnl final warnings
dnl
GCONFIG_FN_WARNINGS
