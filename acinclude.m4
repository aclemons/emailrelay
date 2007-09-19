dnl Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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

dnl socketlen_t
dnl
AC_DEFUN([ACLOCAL_TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t],[aclocal_type_socklen_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <sys/types.h>
#include <sys/socket.h>]],
		[[socklen_t len = 42; return len;]])],
		aclocal_type_socklen_t=yes,
		aclocal_type_socklen_t=no )
])
	if test $aclocal_type_socklen_t = yes; then
		AC_DEFINE(HAVE_SOCKLEN_T, 1,[Define to 1 if socklen_t type definition in sys/socket.h])
	else
		AC_DEFINE(HAVE_SOCKLEN_T, 0,[Define to 1 if socklen_t type definition in sys/socket.h])
	fi
])

dnl ipv6
dnl
AC_DEFUN([ACLOCAL_CHECK_IPV6],
[AC_CACHE_CHECK([for ipv6],[aclocal_ipv6],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>]],
		[[sockaddr_in6 * p = 0;]])],
		aclocal_ipv6=yes ,
		aclocal_ipv6=no )
])
	if test $aclocal_ipv6 = yes; then
		AC_DEFINE(HAVE_IPV6,1,[Define to 1 if ipv6 is available])
	else
		AC_DEFINE(HAVE_IPV6,0,[Define to 1 if ipv6 is available])
	fi
])

dnl getipnodebyname for ipv6 rfc2553
dnl
AC_DEFUN([ACLOCAL_CHECK_GETIPNODEBYNAME],
[AC_CACHE_CHECK([for getipnodebyname],[aclocal_getipnodebyname],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>]],
		[[int i=0; getipnodebyname("",AF_INET6,AI_DEFAULT,&i);]])],
		aclocal_getipnodebyname=yes ,
		aclocal_getipnodebyname=no )
])
	if test $aclocal_getipnodebyname = yes; then
		AC_DEFINE(HAVE_GETIPNODEBYNAME,1,[Define to 1 if getipnodebyname() is available])
	else
		AC_DEFINE(HAVE_GETIPNODEBYNAME,0,[Define to 1 if getipnodebyname() is available])
	fi
])

dnl check for sin6_len in sockaddr_in6
dnl
AC_DEFUN([ACLOCAL_CHECK_SIN6_LEN],
[AC_CACHE_CHECK([for sin6_len],[aclocal_sin6_len],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>]],
		[[struct sockaddr_in6 s; s.sin6_len = 1;]])],
		aclocal_sin6_len=yes ,
		aclocal_sin6_len=no )
])
	if test $aclocal_sin6_len = yes; then
		AC_DEFINE(HAVE_SIN6_LEN,1,[Define to 1 if sockaddr_in6 has a sin6_len member])
	else
		AC_DEFINE(HAVE_SIN6_LEN,0,[Define to 1 if sockaddr_in6 has a sin6_len member])
	fi
])

dnl setgroups
dnl
AC_DEFUN([ACLOCAL_CHECK_SETGROUPS],
[AC_CACHE_CHECK([for setgroups],[aclocal_setgroups],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <sys/types.h>
#include <unistd.h>
#include <grp.h>]],
		[[setgroups(0,0) ;]])],
		aclocal_setgroups=yes ,
		aclocal_setgroups=no )
])
	if test $aclocal_setgroups = yes; then
		AC_DEFINE(HAVE_SETGROUPS,1,[Define to 1 if setgroups is available])
	else
		AC_DEFINE(HAVE_SETGROUPS,0,[Define to 1 if setgroups is available])
	fi
])

dnl gmtime_r
dnl
AC_DEFUN([ACLOCAL_CHECK_GMTIME_R],
[AC_CACHE_CHECK([for gmtime_r],[aclocal_gmtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <time.h>]],
		[[gmtime_r((time_t*)0,(struct tm*)0) ;]])],
		aclocal_gmtime_r=yes ,
		aclocal_gmtime_r=no )
])
	if test $aclocal_gmtime_r = yes; then
		AC_DEFINE(HAVE_GMTIME_R,1,[Define to 1 if gmtime_r in time.h])
	else
		AC_DEFINE(HAVE_GMTIME_R,0,[Define to 1 if gmtime_r in time.h])
	fi
])

dnl localtime_r
dnl
AC_DEFUN([ACLOCAL_CHECK_LOCALTIME_R],
[AC_CACHE_CHECK([for localtime_r],[aclocal_localtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <time.h>]],
		[[localtime_r((time_t*)0,(struct tm*)0) ;]])],
		aclocal_localtime_r=yes ,
		aclocal_localtime_r=no )
])
	if test $aclocal_localtime_r = yes; then
		AC_DEFINE(HAVE_LOCALTIME_R,1,[Define to 1 if localtime_r in time.h])
	else
		AC_DEFINE(HAVE_LOCALTIME_R,0,[Define to 1 if localtime_r in time.h])
	fi
])

dnl buggy ctime
dnl sunpro5 ctime + unistd.h doesnt compile -- fix with time.h first
dnl
AC_DEFUN([ACLOCAL_CHECK_BUGGY_CTIME],
[AC_CACHE_CHECK([for buggy ctime],[aclocal_buggy_ctime],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <ctime>
#include <unistd.h>]],
		[[ ]])] ,
		aclocal_buggy_ctime=no ,
		aclocal_buggy_ctime=yes )
])
	if test $aclocal_buggy_ctime = yes; then
		AC_DEFINE(HAVE_BUGGY_CTIME,1,[Define to 1 if <ctime> requires <time.h>])
	else
		AC_DEFINE(HAVE_BUGGY_CTIME,0,[Define to 1 if <ctime> requires <time.h>])
	fi
])

dnl compiler name and version 
dnl used for -Ilib/<version> -- only needed for pre 3.0 
dnl gcc -- maps gcc2.96 onto gcc2.95
dnl
AC_DEFUN([ACLOCAL_COMPILER_VERSION],
[
changequote(<<,>>)
	COMPILER_VERSION=`$CXX --version 2>/dev/null | sed q | sed 's/[^0-9 .]*//g;s/\./ /g;s/^ *//;s/ /./;s/ .*//;s/^/gcc/' | sed 's/gcc2.96/gcc2.95/'`
	if test -z "${COMPILER_VERSION}"
	then
		COMPILER_VERSION=`$CXX -V 2>&1 | sed q | grep WorkShop | sed 's/[^0-9]*//;s/[ \.].*//;s/^/sunpro/'`
	fi
changequote([,])
	AC_SUBST(COMPILER_VERSION)
])

dnl enable-debug
dnl
AC_DEFUN([ENABLE_DEBUG],
[
if test "$enable_debug" = "yes"
then
	AC_DEFINE(_DEBUG,1,[Define to enable extra debug messages at compile-time])
fi
])

dnl enable-gui
dnl
AC_DEFUN([ENABLE_GUI],
[
qt4="no"
qt4moc="no"
if test "$enable_gui" = "no"
then
	AC_DEFINE(HAVE_GUI,0,[Define to 1 to enable gui code])
else
    PKG_CHECK_MODULES(QT,QtGui >= 4.0.1,[qt4=yes],[AC_MSG_RESULT([no])])
	if test "$qt4" = "yes"
	then
		MOC="${e_qtmoc}"
		AC_PATH_PROG(MOC,moc)
		AC_MSG_CHECKING([moc is for qt 4])
		if test x$GREP = x ; then GREP=grep ; fi
		if test -x "$MOC" -a "`$MOC -v 2>&1 | $GREP 'Qt 4'`" != "" ; then
			AC_MSG_RESULT([yes])
			qt4moc="yes"
		else
			AC_MSG_RESULT([no])
		fi
	else
		QT_LIBS=""
		AC_SUBST(QT_LIBS)
	fi
fi
if test "$qt4moc" = "yes"
then
	AC_DEFINE(HAVE_GUI,1,[Define to 1 to enable gui code])
else
	AC_DEFINE(HAVE_GUI,0,[Define to 1 to enable gui code])
fi
AC_SUBST(MOC)
AM_CONDITIONAL(GUI,test x$enable_gui != xno -a x$qt4moc = xyes )
])

dnl enable-ipv6
dnl
AC_DEFUN([ENABLE_IPV6],
[
if test "$enable_ipv6" = "yes"
then
	if test "$aclocal_ipv6" != "yes"
	then
		AC_MSG_WARN([ignoring --enable-ipv6])
		IP="ipv4"
	else
		IP="ipv6"
	fi
else
	IP="ipv4"
fi
AC_SUBST(IP)
])

dnl with-openssl
dnl
AC_DEFUN([WITH_OPENSSL],
if test "$with_openssl" != "no"
then
[AC_CACHE_CHECK([for openssl],[aclocal_openssl],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[SSL_CTX * p = 0 ; return 1;]])],
		aclocal_openssl=yes,
		aclocal_openssl=no )
])
    if test "$aclocal_openssl" = "yes"
	then
		AC_DEFINE(HAVE_OPENSSL,1,[Define to 1 to enable tls/ssh code using openssl])
		SSL_LIBS="-lssl -lcrypto"
		SSL="openssl"
	else
		if test "$with_openssl" = "yes"
		then
			AC_MSG_WARN([ignoring --with-openssl, check config.log and try setting CFLAGS])
		fi
		AC_DEFINE(HAVE_OPENSSL,0,[Define to 1 to enable tls/ssh code using openssl])
		SSL_LIBS=""
		SSL="none"
	fi
else
	AC_DEFINE(HAVE_OPENSSL,0,[Define to 1 to enable tls/ssh code using openssl])
	SSL_LIBS=""
	SSL="none"
fi
AC_SUBST(SSL_LIBS)
AC_SUBST(SSL)
])

dnl enable-static-linking
dnl
dnl TODO remove -ldl
dnl
AC_DEFUN([ENABLE_STATIC_LINKING],
[
if test "$enable_static_linking" = "yes"
then
	STATIC_START="-Xlinker -Bstatic"
	STATIC_END="-Xlinker -Bdynamic -ldl"
else
	STATIC_START=""
	STATIC_END=""
fi
AC_SUBST(STATIC_START)
AC_SUBST(STATIC_END)
])

dnl with-workshop
dnl
AC_DEFUN([WITH_WORKSHOP],
[
if test "$with_workshop" = "yes"
then
	chmod +x lib/sunpro5/xar
	AR="`pwd`/lib/sunpro5/xar --cxx \"$CXX\""
	AC_SUBST(AR)
fi
])

dnl with-doxygen
dnl
AC_DEFUN([WITH_DOXYGEN],
[
if test "$with_doxygen" != ""
then
	if test "$with_doxygen" = "yes" -a "$HAVE_DOXYGEN" != "yes"
	then
		AC_MSG_WARN([ignoring --with-doxygen])
	else
		HAVE_DOXYGEN="$with_doxygen"
		AC_SUBST(HAVE_DOXYGEN)
	fi
fi
])

dnl with-man2html
dnl
AC_DEFUN([WITH_MAN2HTML],
[
if test "$with_man2html" != ""
then
	if test "$with_man2html" = "yes" -a "$HAVE_MAN2HTML" != "yes"
	then
		AC_MSG_WARN([ignoring --with-man2html])
	else
		HAVE_MAN2HTML="$with_man2html"
		AC_SUBST(HAVE_MAN2HTML)
	fi
fi
])

dnl enable-fhs
dnl
AC_DEFUN([ENABLE_FHS],
[
if test "$enable_fhs" = "yes"
then
	FHS_COMPLIANCE
fi
])

dnl fhs
dnl
AC_DEFUN([FHS_COMPLIANCE],
[
	# tweaks for fhs compliance...
	#
	prefix='/usr'
	exec_prefix='/usr'
	#
	sbindir='/usr/sbin'
	libexecdir='/usr/lib'
	localstatedir='/var'
	mandir='/usr/share/man'
	datadir='/usr/share'
	sysconfdir='/etc'
	#
	# not used by emailrelay
	#bindir=
	#sharedstatedir=
	#libdir=
	#includedir=
	#oldincludedir=
	#infodir=
	#
	# emailrelay-specific
	e_libexecdir="$libexecdir/$PACKAGE"
	e_docdir="$datadir/doc/$PACKAGE"
	e_initdir="/etc/init.d"
	e_spooldir="$localstatedir/spool/$PACKAGE"
	e_examplesdir="$libexecdir/$PACKAGE/examples"
	e_sysconfdir="$sysconfdir"
])

