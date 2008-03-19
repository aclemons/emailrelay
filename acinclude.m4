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

dnl aclocal-type-socketlen-t
dnl
dnl Defines HAVE_SOCKLEN_T.
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

dnl aclocal-check-ipv6
dnl
dnl Defines HAVE_IPV6.
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

dnl aclocal-check-getipnodebyname 
dnl
dnl Defines HAVE_GETIPNODEBYNAME if the ipv6 function 
dnl getipnodebyname() as per rfc2553 is available.
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

dnl aclocal-check-sin6-len
dnl
dnl Defines HAVE_SIN6_LEN if sin6_len is in sockaddr_in6.
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

dnl aclocal-check-setgroups
dnl
dnl Defines HAVE_SETGROUPS.
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
dnl Defines HAVE_GMTIME_R.
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

dnl aclocal-check-localtime_r
dnl
dnl Defines HAVE_LOCALTIME_R.
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

dnl aclocal-check-buggy-ctime
dnl
dnl Defines HAVE_BUGGY_CTIME if ctime + unistd.h doesnt compile.
dnl Needed for old versions of sunpro. Remove soon.
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

dnl aclocal-compiler-version
dnl
dnl Sets COMPILER_VERSION in makefiles.
dnl
dnl Used for -Ilib/<version>. Doesnt work very well but only 
dnl needed for pre 3.0 gcc. Maps gcc2.96 onto gcc2.95.
dnl
AC_DEFUN([ACLOCAL_COMPILER_VERSION],
[
changequote(<<,>>)
	COMPILER_VERSION_GCC=`$CXX --version 2>/dev/null | sed q | grep GCC | sed 's/[a-zA-Z][a-zA-Z]*[0-9]*//g' | sed 's/[^0-9 .]*//g;s/\./ /g;s/^ *//;s/ /./;s/ .*//;s/^/gcc/' | sed 's/gcc2.96/gcc2.95/'`
	COMPILER_VERSION_SUNPRO=`$CXX -V 2>&1 | sed q | grep WorkShop | sed 's/[^0-9]*//;s/[ \.].*//;s/^/sunpro/'`
	COMPILER_VERSION_ICC=`$CXX --version 2>&1 | sed q | grep ICC | sed 's/[^0-9]*//;s/[ \.].*//;s/^/icc/'`
changequote([,])
	COMPILER_VERSION="${COMPILER_VERSION_GCC}${COMPILER_VERSION_SUNPRO}${COMPILER_VERSION_ICC}"
	AC_SUBST(COMPILER_VERSION)
])

dnl aclocal-check-qt4
dnl
dnl Sets the $MOC variable and MOC in makefiles to the moc
dnl path if qt4 is found. Also sets the $aclocal_moc
dnl variable to the same value if the moc is from qt4.
dnl
dnl In the implementation remember that AC_PATH_PROG does
dnl nothing if the variable is already defined, and that
dnl it does an internal AC_SUBST.
dnl
dnl The PKG_CHECK_MODULES macro is used to modify CFLAGS etc.
dnl
AC_DEFUN([ACLOCAL_CHECK_QT4],
[
	PKG_CHECK_MODULES(QT,QtGui >= 4.0.1,[qt4=yes],[AC_MSG_RESULT([no])])

	MOC="${e_qtmoc}"
	AC_PATH_PROG(MOC,moc)

	if test "$MOC" != ""
	then
		AC_MSG_CHECKING([moc is for qt 4])
		if test x$GREP = x ; then GREP=grep ; fi
		if test -x "$MOC" -a "`$MOC -v 2>&1 | $GREP 'Qt 4'`" != "" ; then
			AC_MSG_RESULT([yes])
			aclocal_moc="$MOC"
		else
			AC_MSG_RESULT([no])
			aclocal_moc=""
		fi
	fi
])

dnl enable-gui
dnl
dnl Sets QT_LIBS, MOC and "if GUI" in makefiles if a GUI build is required.
dnl
dnl Requires ACLOCAL_CHECK_QT4 to have been run first.
dnl
AC_DEFUN([ENABLE_GUI],
[
	if test "$enable_gui" = "no"
	then
		MOC=""
	else
		if test "$enable_gui" = "yes" -a "$aclocal_moc" = ""
		then
			AC_MSG_WARN([ignoring --enable-gui, set e_qtmoc to a Qt4 moc program to override])
		fi
		MOC="$aclocal_moc"
	fi

	if test "`uname`" = "Darwin" -a "$QT_LIBS" = ""
	then
		QT_LIBS="-framework QtGui -framework QtCore"
	fi

	AC_SUBST(QT_LIBS)
	AC_SUBST(MOC)
	AM_CONDITIONAL(GUI,test x$MOC != x )

	if test x$enable_exec = xno -a x$MOC != x
	then
		AC_MSG_ERROR([using --disable-exec requires --disable-gui])
	fi
])

dnl enable-debug
dnl
dnl Defines _DEBUG if requested. Defaults to "no" but 
dnl allows "--enable-debug=full" as per kdevelop.
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

dnl enable-verbose
dnl
dnl The "--disable-verbose" switch disables the verbose-logging macro.
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
dnl The "--disable-pop" switch builds the pop library from 
dnl do-nothing stubs.
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
dnl The "--disable-exec" switch removes source files are concerned 
dnl with exec-ing external programs.
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
dnl The "--disable-admin" switch removes source files that implement 
dnl the admin interface.
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
dnl The "--disable-admin" switch removes source files that implement authentication.
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
dnl The "--disable-dns" switch disables dns lookup so host and service 
dnl names must be given as ip addresses and port numbers. This can be
dnl make static linking easier, especially in embedded systems.
dnl
AC_DEFUN([ENABLE_DNS],
[
	AM_CONDITIONAL(DNS,test x$enable_dns != xno)
])

dnl enable-identity
dnl
dnl The "--disable-identity" switch disables userid switching thereby 
dnl removing the dependence on getpwnam and /etc/passwd. This can
dnl make static linking easier, especially in embedded systems.
dnl
AC_DEFUN([ENABLE_IDENTITY],
[
	AM_CONDITIONAL(IDENTITY,test x$enable_identity != xno)
])

dnl enable-small-fragments
dnl
dnl The "--enable-small-fragments" sewitch compiles certain source files in 
dnl lots of little pieces so the linker can throw away fragments that 
dnl are not needed in the final executable.
dnl
dnl This requires perl on the path and probably messes up a lot of 
dnl autoconf/automake features, so only use if really necessary.
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
dnl The "--enable-small-config" switch replaces the complex command-line 
dnl parsing code with something simpler and less functional.
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
dnl The "--enable-small-exceptions" defines exception classes as functions
dnl as a size optimisation. This should probably become the default when
dnl it has had more testing.
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
dnl The "--enable-ipv6" switch enables ipv6 as long as ipv6 is available.
dnl
dnl Note that this requires ACLOCAL_CHECK_IPV6 to have been run.
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
dnl The "--disable-proxy" switch disables smtp proxying as a size optimisation.
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

dnl enable-mac
dnl
dnl The "--enable-mac" switch tweaks things for a mac build.
dnl
AC_DEFUN([ENABLE_MAC],
[
	if test x$enable_mac = xyes -o "`uname`" = "Darwin"
	then
		AC_DEFINE(G_MAC,1,[Define for a mac build])
	fi
	AM_CONDITIONAL(MAC,test x$enable_mac = xyes -o "`uname`" = "Darwin")
])

dnl with-openssl
dnl
dnl Sets SSL_LIBS and "if OPENSSL" in makefiles.
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
dnl Sets "if GLOB" in makefiles. Defaults to auto.
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
dnl The "--enable-static-linking" makes a half-hearted
dnl attempt at static linking. Only applicable to gcc.
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

dnl with-doxygen
dnl
dnl Sets HAVE_DOXYGEN in makefiles if doxygen is to be used.
dnl
dnl Usually used after doing a doxygen program check to set 
dnl the default value for $HAVE_DOXYGEN.
dnl
AC_DEFUN([WITH_DOXYGEN],
[
	if test "$with_doxygen" != ""
	then
		if test "$with_doxygen" = "yes" -a "$HAVE_DOXYGEN" != "yes"
		then
			AC_MSG_WARN([forcing use of doxygen even though not found])
		fi
		HAVE_DOXYGEN="$with_doxygen"
	fi
	AC_SUBST(HAVE_DOXYGEN)
])

dnl with-man2html
dnl
dnl Sets HAVE_MAN2HTML in makefiles if man2html is to be used.
dnl
dnl Usually used after doing a man2html program check to set 
dnl the default value for $HAVE_MAN2HTML.
dnl
AC_DEFUN([WITH_MAN2HTML],
[
	if test "$with_man2html" != ""
	then
		if test "$with_man2html" = "yes" -a "$HAVE_MAN2HTML" != "yes"
		then
			AC_MSG_WARN([forcing use of man2html even though not found])
		fi
		HAVE_MAN2HTML="$with_man2html"
	fi
	AC_SUBST(HAVE_MAN2HTML)
])

dnl set-directories
dnl
dnl Sets directory paths taking account of "--enable-fhs".
dnl
AC_DEFUN([SET_DIRECTORIES],
[
	# the following are used in the makefiles:
	# * sbindir
	# * e_libexecdir
	# * e_examplesdir
	# * e_sysconfdir
	# * mandir
	# * e_docdir
	# * e_spooldir
	# * e_initdir

	if test "$enable_fhs" = "yes"
	then
		prefix='/usr'
		exec_prefix='/usr'
		sbindir="/usr/sbin"
		libexecdir="/usr/lib"
		localstatedir="/var"
		mandir="/usr/share/man"
		datadir="/usr/share"
		sysconfdir="/etc"
	fi
	if test "$e_libexecdir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then 
			e_libexecdir="$libexecdir/$PACKAGE" 
		else 
			e_libexecdir="$libexecdir/$PACKAGE" 
		fi
	fi
	if test "$e_examplesdir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then
			e_examplesdir="$libexecdir/$PACKAGE/examples"
		else 
			e_examplesdir="$libexecdir/$PACKAGE/examples" 
		fi
	fi
	if test "$e_sysconfdir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then 
			e_sysconfdir="$sysconfdir"
		else 
			e_sysconfdir="$sysconfdir" 
		fi
	fi
	if test "$e_docdir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then 
			e_docdir="$datadir/doc/$PACKAGE" 
		else 
			e_docdir="$docdir"
			if test "$e_docdir" = ""
			then
				e_docdir="$datadir/$PACKAGE/doc" 
			fi
		fi
	fi
	if test "$e_spooldir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then 
			e_spooldir="$localstatedir/spool/$PACKAGE"
		else 
			e_spooldir="$localstatedir/spool/$PACKAGE" 
		fi
	fi
	if test "$e_initdir" = "" 
	then 
		if test "$enable_fhs" = "yes" 
		then 
			e_initdir="/etc/init.d" 
		else 
			e_initdir="$libexecdir/$PACKAGE/init" 
		fi
	fi
])

