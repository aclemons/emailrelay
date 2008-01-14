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
		AC_DEFINE(HAVE_SOCKLEN_T,1,[Define to 1 if socklen_t type definition in sys/socket.h])
	else
		AC_DEFINE(HAVE_SOCKLEN_T,0,[Define to 1 if socklen_t type definition in sys/socket.h])
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
dnl defaults to no but allows --enable-debug=full as per kdevelop
dnl
AC_DEFUN([ENABLE_DEBUG],
[
	if test "$enable_debug" = "no" -o -z "$enable_debug"
	then
		:
	else
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
		:
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
	AC_SUBST(MOC)
	AM_CONDITIONAL(GUI,test x$enable_gui != xno -a x$qt4moc = xyes )
	if test x$enable_exec = xno -a x$enable_gui != xno -a x$qt4moc = xyes
	then
		AC_MSG_ERROR([using --disable-exec requires --disable-gui])
	fi
])

dnl enable-verbose
dnl
dnl disable-verbose disables the verbose-logging macro
dnl
AC_DEFUN([ENABLE_VERBOSE],
[
	if test x$enable_verbose != xno
	then
		:
	else
		AC_DEFINE(G_NO_LOG,1,[Define to disable the G_LOG macro])
	fi
])

dnl enable-pop
dnl
dnl disable-pop builds the pop library from do-nothing stubs
dnl
AC_DEFUN([ENABLE_POP],
[
	if test x$enable_pop != xno
	then
		:
	else
		AC_DEFINE(USE_NO_POP,1,[Define to eliminate unused pop code as a size optimisation])
	fi
	AM_CONDITIONAL(POP,test x$enable_pop != xno)
])

dnl enable-exec
dnl
dnl disable-exec removes source files are concerned with exec-ing external programs
dnl
AC_DEFUN([ENABLE_EXEC],
[
	if test x$enable_exec != xno
	then
		:
	else
		AC_DEFINE(USE_NO_EXEC,1,[Define to eliminate unused exec-ing code as a size optimisation])
	fi
	AM_CONDITIONAL(EXEC,test x$enable_exec != xno)
])

dnl enable-admin
dnl
dnl disable-admin removes source files that implement the admin interface
dnl
AC_DEFUN([ENABLE_ADMIN],
[
	if test x$enable_admin != xno
	then
		:
	else
		AC_DEFINE(USE_NO_ADMIN,1,[Define to eliminate unused admin interface code as a size optimisation])
	fi
	AM_CONDITIONAL(ADMIN,test x$enable_admin != xno)
])

dnl enable-auth
dnl
dnl disable-admin removes source files that implement authentication
dnl
AC_DEFUN([ENABLE_AUTH],
[
	if test x$enable_auth != xno
	then
		:
	else
		AC_DEFINE(USE_NO_AUTH,1,[Define to eliminate unused authentication code as a size optimisation])
	fi
	AM_CONDITIONAL(AUTH,test x$enable_auth != xno)
	if test x$enable_auth = xno -a x$enable_pop != xno
	then
		AC_MSG_ERROR([using --disable-auth requires --disable-pop])
	fi
])

dnl enable-dns
dnl
dnl disable-dns disables dns lookup so host and service names must be given as ip addresses and port numbers
dnl
AC_DEFUN([ENABLE_DNS],
[
	AM_CONDITIONAL(DNS,test x$enable_dns != xno)
])

dnl enable-identity
dnl
dnl disable-identity disables userid switching thereby removing the dependence on getpwnam and /etc/passwd
dnl
AC_DEFUN([ENABLE_IDENTITY],
[
	AM_CONDITIONAL(IDENTITY,test x$enable_identity != xno)
])

dnl enable-small-fragments
dnl
dnl enable-fragments compiles certain source files in lots of little pieces so
dnl the linker can throw away fragments that are not needed in the final executable
dnl
dnl requires perl on the path and probably messes up a lot of autoconf/automake 
dnl features, so only use if really necessary
dnl
AC_DEFUN([ENABLE_SMALL_FRAGMENTS],
[
	AM_CONDITIONAL(SMALL_FRAGMENTS,test x$enable_small_fragments = xyes)
	if test x$enable_small_fragments = xyes
	then
		AC_MSG_NOTICE([creating source file fragments])
		FRAGMENTS_LIST="`perl $srcdir/bin/fragment.pl_ -r $srcdir/src $srcdir/src/fragments`"
		for fragment in $FRAGMENTS_LIST "" ; do if test "$fragment" != "" ; then
			AC_MSG_NOTICE([creating source file fragment for $fragment])
		fi ; done
	fi
	AC_SUBST(FRAGMENTS_LIST)
])

dnl enable-small-config
dnl
dnl enable-small-config replaces the complex command-line parsing code
dnl with something simpler and less functional
dnl
AC_DEFUN([ENABLE_SMALL_CONFIG],
[
	if test x$enable_small_config = xyes
	then
		AC_DEFINE(USE_SMALL_CONFIG,1,[Define to eliminate unused config code as a size optimisation])
	else
		:
	fi
	AM_CONDITIONAL(SMALL_CONFIG,test x$enable_small_config = xyes)
])

dnl enable-small-exceptions
dnl
dnl enable-small-exceptions defines exception classes as functions
dnl
AC_DEFUN([ENABLE_SMALL_EXCEPTIONS],
[
	if test x$enable_small_exceptions = xyes
	then
		AC_DEFINE(USE_SMALL_EXCEPTIONS,1,[Define to have exception types as functions as a size optimisation])
	else
		:
	fi
])

dnl enable-ipv6
dnl
dnl requires ACLOCAL_CHECK_IPV6 to have been run
dnl
AC_DEFUN([ENABLE_IPV6],
[
	if test "$enable_ipv6" = "yes"
	then
		if test "$aclocal_ipv6" != "yes"
		then
			AC_MSG_WARN([ignoring --enable-ipv6])
			aclocal_use_ipv6="no"
		else
			AC_DEFINE(USE_IPV6,1,[Define to use IPv6])
			aclocal_use_ipv6="yes"
		fi
	else
		aclocal_use_ipv6="no"
	fi
	AM_CONDITIONAL(IPV6,test x$aclocal_use_ipv6 = xyes)
])
	
dnl enable-proxy
dnl
dnl disable-proxy disables smtp proxying
dnl
AC_DEFUN([ENABLE_PROXY],
[
	if test x$enable_proxy != xno
	then
		:
	else
		AC_DEFINE(USE_NO_PROXY,1,[Define to eliminate proxying code as a size optimisation])
	fi
	AM_CONDITIONAL(PROXY,test x$enable_proxy != xno)
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
		SSL_LIBS="-lssl -lcrypto"
		aclocal_ssl="openssl"
	else
		if test "$with_openssl" = "yes"
		then
			AC_MSG_WARN([ignoring --with-openssl, check config.log and try setting CFLAGS])
		fi
		SSL_LIBS=""
		aclocal_ssl="none"
	fi
else
	SSL_LIBS=""
	aclocal_ssl="none"
fi
AC_SUBST(SSL_LIBS)
AM_CONDITIONAL(OPENSSL,test x$aclocal_ssl = xopenssl)
])

dnl with-glob
dnl
AC_DEFUN([WITH_GLOB],
if test "$with_glob" != "no"
then
[AC_CACHE_CHECK([for glob],[aclocal_have_glob],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <glob.h>]],
		[[glob_t * p = 0 ; globfree(p) ; return 1;]])],
		aclocal_have_glob=yes,
		aclocal_have_glob=no )
])
    if test "$aclocal_have_glob" = "yes"
	then
		aclocal_use_glob="yes"
	else
		if test "$with_glob" = "yes"
		then
			AC_MSG_WARN([ignoring --with-glob])
		fi
		aclocal_use_glob="no"
	fi
else
    if test "$aclocal_have_glob" = "yes"
	then
		AC_MSG_WARN([not using available glob()])
	fi
	aclocal_use_glob="no"
fi
AM_CONDITIONAL(GLOB,test x$aclocal_use_glob = xyes)
])

dnl enable-static-linking
dnl
dnl only applicable to gcc
dnl
AC_DEFUN([ENABLE_STATIC_LINKING],
[
	if test "$enable_static_linking" = "yes"
	then
		STATIC_START="-Xlinker -Bstatic"
		STATIC_END=""
	else
		STATIC_START=""
		STATIC_END=""
	fi
	AC_SUBST(STATIC_START)
	AC_SUBST(STATIC_END)
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

dnl fhs-compiliance
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

