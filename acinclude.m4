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

dnl socketlen_t
dnl derived from lars brinkhoff...
dnl
AC_DEFUN(ACLOCAL_TYPE_SOCKLEN_T,
[AC_CACHE_CHECK([for socklen_t], aclocal_cv_type_socklen_t,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
#include <sys/socket.h>],
  [socklen_t len = 42; return len;],
  aclocal_cv_type_socklen_t=yes,
  aclocal_cv_type_socklen_t=no)
])
  if test $aclocal_cv_type_socklen_t = yes; then
    AC_DEFINE(HAVE_SOCKLEN_T, 1,[Define if socklen_t type definition in sys/socket.h])
  else
    AC_DEFINE(HAVE_SOCKLEN_T, 0,[Define if socklen_t type definition in sys/socket.h])
  fi
])

dnl gmtime_r
dnl
AC_DEFUN([ACLOCAL_CHECK_GMTIME_R],
[AC_CACHE_CHECK([for gmtime_r], aclocal_cv_gmtime_r,
[
	AC_TRY_COMPILE(
		[#include <time.h>],
		[gmtime_r((time_t*)0,(struct tm*)0) ;],
		aclocal_cv_gmtime_r=yes ,
		aclocal_cv_gmtime_r=no )
])
	if test $aclocal_cv_gmtime_r = yes; then
		AC_DEFINE(HAVE_GMTIME_R,1,[Define if gmtime_r in time.h])
	else
		AC_DEFINE(HAVE_GMTIME_R,0,[Define if gmtime_r in time.h])
	fi
])

dnl localtime_r
dnl
AC_DEFUN([ACLOCAL_CHECK_LOCALTIME_R],
[AC_CACHE_CHECK([for localtime_r], aclocal_cv_localtime_r,
[
	AC_TRY_COMPILE(
		[#include <time.h>],
		[localtime_r((time_t*)0,(struct tm*)0) ;],
		aclocal_cv_localtime_r=yes ,
		aclocal_cv_localtime_r=no )
])
	if test $aclocal_cv_localtime_r = yes; then
		AC_DEFINE(HAVE_LOCALTIME_R,1,[Define if localtime_r in time.h])
	else
		AC_DEFINE(HAVE_LOCALTIME_R,0,[Define if localtime_r in time.h])
	fi
])

dnl buggy ctime
dnl sunpro5 ctime + unistd.h doesnt compile -- fix with time.h first
dnl
AC_DEFUN([ACLOCAL_CHECK_BUGGY_CTIME],
[AC_CACHE_CHECK([for buggy ctime], aclocal_cv_buggy_ctime,
[
	AC_TRY_COMPILE(
		[#include <ctime>
#include <unistd.h>],
		[] ,
		aclocal_cv_buggy_ctime=no ,
		aclocal_cv_buggy_ctime=yes )
])
	if test $aclocal_cv_buggy_ctime = yes; then
		AC_DEFINE(HAVE_BUGGY_CTIME,1,[Define if <ctime> requires <time.h>])
	else
		AC_DEFINE(HAVE_BUGGY_CTIME,0,[Define if <ctime> requires <time.h>])
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
	mandir='/usr/man'
	datadir='/usr/share'
	#
	# not used by emailrelay
	#bindir=
	#sysconfdir=
	#sharedstatedir=
	#libdir=
	#includedir=
	#oldincludedir=
	#infodir=
	#
	# emailrelay-specific
	e_sbindir="$sbindir"
	e_libexecdir="$libexecdir/$PACKAGE"
	e_docdir="$datadir/doc/$PACKAGE"
	e_initdir="/etc/init.d"
	e_spooldir="$localstatedir/spool/$PACKAGE"
	e_man1dir="$datadir/man/man1"
	e_examplesdir="$datadir/doc/$PACKAGE/examples"
])

