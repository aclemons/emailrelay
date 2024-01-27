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
dnl GCONFIG_FN_ARFLAGS
dnl ------------------
dnl Does AC_SUBST to set ARFLAGS to "cr", depending on the output from
dnl "ar --version".
dnl
AC_DEFUN([GCONFIG_FN_ARFLAGS],
[
	gconfig_arflags="${ARFLAGS-cru}"
	if test "`uname 2>/dev/null`" = "Linux"
	then
		if "${AR}" --version | grep "GNU ar" > /dev/null
		then
			gconfig_arflags="cr"
		fi
	fi
	AC_SUBST([ARFLAGS],["$gconfig_arflags"])
])

dnl GCONFIG_FN_CHECK_CXX
dnl ----------------------
dnl Checks c++ language features.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_CXX],[
	AC_REQUIRE([GCONFIG_FN_CXX_ALIGNMENT])
	AC_REQUIRE([GCONFIG_FN_CXX_MAKE_UNIQUE])
	AC_REQUIRE([GCONFIG_FN_CXX_STD_THREAD])
	AC_REQUIRE([GCONFIG_FN_CXX_STRING_VIEW])
])

dnl GCONFIG_FN_CHECK_FUNCTIONS
dnl --------------------------
dnl Checks for various functions.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_FUNCTIONS],[
	AC_REQUIRE([GCONFIG_FN_GETPWNAM])
	AC_REQUIRE([GCONFIG_FN_GETPWNAM_R])
	AC_REQUIRE([GCONFIG_FN_GETGRNAM])
	AC_REQUIRE([GCONFIG_FN_GETGRNAM_R])
	AC_REQUIRE([GCONFIG_FN_GETTEXT])
	AC_REQUIRE([GCONFIG_FN_GMTIME_R])
	AC_REQUIRE([GCONFIG_FN_GMTIME_S])
	AC_REQUIRE([GCONFIG_FN_LOCALTIME_R])
	AC_REQUIRE([GCONFIG_FN_LOCALTIME_S])
	AC_REQUIRE([GCONFIG_FN_STRNCPY_S])
	AC_REQUIRE([GCONFIG_FN_GETENV_S])
	AC_REQUIRE([GCONFIG_FN_PUTENV_S])
	AC_REQUIRE([GCONFIG_FN_FSOPEN])
	AC_REQUIRE([GCONFIG_FN_FOPEN_S])
	AC_REQUIRE([GCONFIG_FN_SOPEN])
	AC_REQUIRE([GCONFIG_FN_SOPEN_S])
	AC_REQUIRE([GCONFIG_FN_EXTENDED_OPEN])
	AC_REQUIRE([GCONFIG_FN_READLINK])
	AC_REQUIRE([GCONFIG_FN_PROC_PIDPATH])
	AC_REQUIRE([GCONFIG_FN_SETPGRP_BSD])
	AC_REQUIRE([GCONFIG_FN_SETGROUPS])
	AC_REQUIRE([GCONFIG_FN_EXECVPE])
	AC_REQUIRE([GCONFIG_FN_SIGPROCMASK])
	AC_REQUIRE([GCONFIG_FN_PTHREAD_SIGMASK])
	AC_REQUIRE([GCONFIG_FN_WINDOWS_CREATE_WAITABLE_TIMER_EX])
	AC_REQUIRE([GCONFIG_FN_WINDOWS_CREATE_EVENT_EX])
	AC_REQUIRE([GCONFIG_FN_WINDOWS_INIT_COMMON_CONTROLS_EX])
	AC_REQUIRE([GCONFIG_FN_WINDOWS_STARTUP_INFO_EX])
])

dnl GCONFIG_FN_CHECK_HEADERS
dnl ------------------------
dnl Does AC_CHECK_HEADERS for common headers.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_HEADERS],[
	AC_CHECK_HEADER([sys/types.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_TYPES_H],1,[Define true if sys/types.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_TYPES_H],0,[Define true if sys/types.h is available]))
	AC_CHECK_HEADER([sys/stat.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_STAT_H],1,[Define true if sys/stat.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_STAT_H],0,[Define true if sys/stat.h is available]))
	AC_CHECK_HEADER([sys/wait.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_WAIT_H],1,[Define true if sys/wait.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_WAIT_H],0,[Define true if sys/wait.h is available]))
	AC_CHECK_HEADER([sys/mman.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_MMAN_H],1,[Define true if sys/mman.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_MMAN_H],0,[Define true if sys/mman.h is available]))
	AC_CHECK_HEADER([sys/select.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_SELECT_H],1,[Define true if sys/select.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_SELECT_H],0,[Define true if sys/select.h is available]))
	AC_CHECK_HEADER([sys/socket.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_SOCKET_H],1,[Define true if sys/socket.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_SOCKET_H],0,[Define true if sys/socket.h is available]))
	AC_CHECK_HEADER([sys/utsname.h],
		AC_DEFINE([GCONFIG_HAVE_SYS_UTSNAME_H],1,[Define true if sys/utsname.h is available]),
		AC_DEFINE([GCONFIG_HAVE_SYS_UTSNAME_H],0,[Define true if sys/utsname.h is available]))
	AC_CHECK_HEADER([netdb.h],
		AC_DEFINE([GCONFIG_HAVE_NETDB_H],1,[Define true if netdb.h is available]),
		AC_DEFINE([GCONFIG_HAVE_NETDB_H],0,[Define true if netdb.h is available]))
	AC_CHECK_HEADER([netinet/in.h],
		AC_DEFINE([GCONFIG_HAVE_NETINET_IN_H],1,[Define true if netinet/in.h is available]),
		AC_DEFINE([GCONFIG_HAVE_NETINET_IN_H],0,[Define true if netinet/in.h is available]))
	AC_CHECK_HEADER([net/if.h],
		AC_DEFINE([GCONFIG_HAVE_NET_IF_H],1,[Define true if net/if.h is available]),
		AC_DEFINE([GCONFIG_HAVE_NET_IF_H],0,[Define true if net/if.h is available]))
	AC_CHECK_HEADER([arpa/inet.h],
		AC_DEFINE([GCONFIG_HAVE_ARPA_INET_H],1,[Define true if arpa/inet.h is available]),
		AC_DEFINE([GCONFIG_HAVE_ARPA_INET_H],0,[Define true if arpa/inet.h is available]))
	AC_CHECK_HEADER([stdint.h],
		AC_DEFINE([GCONFIG_HAVE_STDINT_H],1,[Define true if stdint.h is available]),
		AC_DEFINE([GCONFIG_HAVE_STDINT_H],0,[Define true if stdint.h is available]))
	AC_CHECK_HEADER([inttypes.h],
		AC_DEFINE([GCONFIG_HAVE_INTTYPES_H],1,[Define true if inttypes.h is available]),
		AC_DEFINE([GCONFIG_HAVE_INTTYPES_H],0,[Define true if inttypes.h is available]))
	AC_CHECK_HEADER([unistd.h],
		AC_DEFINE([GCONFIG_HAVE_UNISTD_H],1,[Define true if unistd.h is available]),
		AC_DEFINE([GCONFIG_HAVE_UNISTD_H],0,[Define true if unistd.h is available]))
	AC_CHECK_HEADER([errno.h],
		AC_DEFINE([GCONFIG_HAVE_ERRNO_H],1,[Define true if errno.h is available]),
		AC_DEFINE([GCONFIG_HAVE_ERRNO_H],0,[Define true if errno.h is available]))
	AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[#include <iphlpapi.h>]],[[]])],
		AC_DEFINE([GCONFIG_HAVE_WINDOWS_IPHLPAPI_H],1,[Define true if iphlpapi.h is available]),
		AC_DEFINE([GCONFIG_HAVE_WINDOWS_IPHLPAPI_H],0,[Define true if iphlpapi.h is available]))
	AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[#include <versionhelpers.h>]],[[]])],
		AC_DEFINE([GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H],1,[Define true if versionhelpers.h is available]),
		AC_DEFINE([GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H],0,[Define true if versionhelpers.h is available]))
	AC_CHECK_HEADER([pwd.h],
		AC_DEFINE([GCONFIG_HAVE_PWD_H],1,[Define true if pwd.h is available]),
		AC_DEFINE([GCONFIG_HAVE_PWD_H],0,[Define true if pwd.h is available]))
	AC_CHECK_HEADER([grp.h],
		AC_DEFINE([GCONFIG_HAVE_GRP_H],1,[Define true if grp.h is available]),
		AC_DEFINE([GCONFIG_HAVE_GRP_H],0,[Define true if grp.h is available]))
])

dnl GCONFIG_FN_CHECK_NET
dnl ----------------------
dnl Checks for network stuff.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_NET],[
	AC_REQUIRE([GCONFIG_FN_IPV6])
	AC_REQUIRE([GCONFIG_FN_SIN6_LEN])
	AC_REQUIRE([GCONFIG_FN_INET_NTOP])
	AC_REQUIRE([GCONFIG_FN_INET_PTON])
	AC_REQUIRE([GCONFIG_FN_EPOLL])
	AC_REQUIRE([GCONFIG_FN_RTNETLINK])
	AC_REQUIRE([GCONFIG_FN_NETROUTE])
	AC_REQUIRE([GCONFIG_FN_IFNAMETOINDEX])
	AC_REQUIRE([GCONFIG_FN_IFNAMETOLUID])
	AC_REQUIRE([GCONFIG_FN_IFINDEX])
	AC_REQUIRE([GCONFIG_FN_GAISTRERROR])
	AC_REQUIRE([GCONFIG_FN_UDS])
	AC_REQUIRE([GCONFIG_FN_UDS_LEN])
])

dnl GCONFIG_FN_CHECK_TYPES
dnl ----------------------
dnl Does AC_CHECK_TYPE for common types.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_TYPES],[
	AC_REQUIRE([GCONFIG_FN_TYPE_SOCKLEN_T])
	AC_REQUIRE([GCONFIG_FN_TYPE_ERRNO_T])
	AC_REQUIRE([GCONFIG_FN_TYPE_SSIZE_T])
	AC_REQUIRE([GCONFIG_FN_TYPE_UINTPTR_T])
	AC_REQUIRE([GCONFIG_FN_TYPE_INT64])
	AC_REQUIRE([GCONFIG_FN_TYPE_INT32])
	AC_REQUIRE([GCONFIG_FN_TYPE_INT16])
	AC_CHECK_TYPES([pid_t],
		AC_DEFINE([GCONFIG_HAVE_PID_T],1,[Define true if pid_t is a type]),
		AC_DEFINE([GCONFIG_HAVE_PID_T],0,[Define true if pid_t is a type]))
	AC_CHECK_TYPES([uid_t],
		AC_DEFINE([GCONFIG_HAVE_UID_T],1,[Define true if uid_t is a type]),
		AC_DEFINE([GCONFIG_HAVE_UID_T],0,[Define true if uid_t is a type]))
	AC_CHECK_TYPES([gid_t],
		AC_DEFINE([GCONFIG_HAVE_GID_T],1,[Define true if gid_t is a type]),
		AC_DEFINE([GCONFIG_HAVE_GID_T],0,[Define true if gid_t is a type]))
	AC_REQUIRE([GCONFIG_FN_STATBUF_TIMESPEC])
	AC_REQUIRE([GCONFIG_FN_STATBUF_NSEC])
	AC_REQUIRE([GCONFIG_FN_IOVEC_SIMPLE])
])

dnl GCONFIG_FN_CXX_ALIGNMENT
dnl ------------------------
dnl Tests for c++ std::align.
dnl
AC_DEFUN([GCONFIG_FN_CXX_ALIGNMENT],
[AC_CACHE_CHECK([for c++ std::align],[gconfig_cv_cxx_alignment],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <memory>]
			[#include <cstring>]
			[using std::size_t ;]
			[void * x = 0 ;]
			[int i[30] ;]
			[void * p = &i[0] ;]
			[size_t n = sizeof(i) ;]
		],
		[
			[x = std::align(alignof(long),2,p,n) ;]
		])],
		gconfig_cv_cxx_alignment=yes ,
		gconfig_cv_cxx_alignment=no )
])
	if test "$gconfig_cv_cxx_alignment" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_ALIGNMENT,1,[Define true if compiler has std::align()])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_ALIGNMENT,0,[Define true if compiler has std::align()])
	fi
])

dnl GCONFIG_FN_CXX_MAKE_UNIQUE
dnl --------------------------
dnl Tests for c++ std::make_unique.
dnl
AC_DEFUN([GCONFIG_FN_CXX_MAKE_UNIQUE],
[AC_CACHE_CHECK([for c++ std::make_unique],[gconfig_cv_cxx_std_make_unique],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <memory>]
			[#include <utility>]
			[struct Foo {};]
		],
		[
			[auto ptr = std::make_unique<Foo>() ;]
		])],
		gconfig_cv_cxx_std_make_unique=yes ,
		gconfig_cv_cxx_std_make_unique=no )
])
	if test "$gconfig_cv_cxx_std_make_unique" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_MAKE_UNIQUE,1,[Define true if compiler has std::make_unique])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_MAKE_UNIQUE,0,[Define true if compiler has std::make_unique])
	fi
])

dnl GCONFIG_FN_CXX_STD_THREAD
dnl -------------------------
dnl Calls GCONFIG_FN_CXX_STD_THREAD_IMP with a suitable warning message.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD],
[
	GCONFIG_FN_CXX_STD_THREAD_IMP([std::thread_asynchronous_script_execution])
])

dnl GCONFIG_FN_CXX_STD_THREAD_IMP
dnl -----------------------------
dnl Tests for a viable c++ std::thread class under the current compile and link options
dnl and adds '-pthread' as necessary. The first parameter is a warning message added to
dnl gconfig_warnings, something like 'std::thread_multithreading'.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD_IMP],
[
	if test "$enable_std_thread" = "no"
	then
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,0,[Define true if compiler has std::thread])
	else
		GCONFIG_FN_CXX_STD_THREAD_PTHREAD

		if test "$gconfig_cxx_std_thread" = "yes" ; then
			AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,1,[Define true if compiler has std::thread])
		else
			AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,0,[Define true if compiler has std::thread])
			gconfig_warnings="$gconfig_warnings $1"
		fi
	fi
])

dnl GCONFIG_FN_CXX_STD_THREAD_PTHREAD
dnl ---------------------------------
dnl Tests for a viable c++ std::thread class under the current compile and link options,
dnl setting 'gconfig_cxx_std_thread' and adding '-pthread' as necessary.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD_PTHREAD],
[
	AC_MSG_CHECKING([for c++ std::thread without -pthread])
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <thread>]
			[#include <cstdio>]
		],
		[
			[std::thread t(std::fopen,"/dev/null","r");]
			[t.join();]
		])],
		gconfig_cxx_std_thread_links=yes ,
		gconfig_cxx_std_thread_links=no )
	AC_MSG_RESULT([$gconfig_cxx_std_thread_links])

	AC_MSG_CHECKING([for c++ std::thread at runtime])
	AC_RUN_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <thread>]
			[#include <cstdio>]
		],
		[
			[std::thread t(std::fopen,"/dev/null","r");]
			[t.join();]
		])],
		gconfig_cxx_std_thread=yes ,
		gconfig_cxx_std_thread=no ,
		gconfig_cxx_std_thread=$gconfig_cxx_std_thread_links )
	AC_MSG_RESULT([$gconfig_cxx_std_thread])

	if test "$gconfig_cxx_std_thread" != "yes"
	then
		gconfig_save_CXXFLAGS="$CXXFLAGS"
		gconfig_save_LDFLAGS="$LDFLAGS"
		CXXFLAGS="$CXXFLAGS -pthread"
		LDFLAGS="$LDFLAGS -pthread"
		AC_MSG_CHECKING([for c++ std::thread with -pthread])
		AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[
				[#include <thread>]
				[#include <cstdio>]
			],
			[
				[std::thread t(std::fopen,"/dev/null","r");]
				[t.join();]
			])],
			gconfig_cxx_std_thread=yes ,
			gconfig_cxx_std_thread=no )
		AC_MSG_RESULT([$gconfig_cxx_std_thread])
		if test "$gconfig_cxx_std_thread" = "no" ; then
			CXXFLAGS="$gconfig_save_CXXFLAGS"
			LDFLAGS="$gconfig_save_LDFLAGS"
		fi
	fi
])

dnl GCONFIG_FN_CXX_STRING_VIEW
dnl --------------------------
dnl Tests for std::string_view.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STRING_VIEW],
[AC_CACHE_CHECK([for c++ std::string_view],[gconfig_cv_cxx_string_view],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <string_view>]
			[std::string_view sv("sv") ;]
		],
		[
		])],
		gconfig_cv_cxx_string_view=yes ,
		gconfig_cv_cxx_string_view=no )
])
	if test "$gconfig_cv_cxx_string_view" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_STRING_VIEW,1,[Define true if compiler supports c++ string_view])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_STRING_VIEW,0,[Define true if compiler supports c++ string_view])
	fi
])

dnl GCONFIG_FN_ENABLE_ADMIN
dnl -----------------------
dnl Optionally disables the admin interface.
dnl Typically used after AC_ARG_ENABLE(admin).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_ADMIN],
[
	AM_CONDITIONAL([GCONFIG_ADMIN],[test "$enable_admin" != "no"])
])

dnl GCONFIG_FN_ENABLE_BSD
dnl ---------------------
dnl Enables bsd tweaks if "--enable-bsd" is used. Typically used after
dnl AC_ARG_ENABLE(bsd).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_BSD],
[
	gconfig_bsd="$enable_bsd"
	AM_CONDITIONAL([GCONFIG_BSD],test "$enable_bsd" = "yes" -o "`uname`" = "NetBSD" -o "`uname`" = "FreeBSD" -o "`uname`" = "OpenBSD" )
])

dnl GCONFIG_FN_ENABLE_DEBUG
dnl -----------------------
dnl Defines G_WITH_DEBUG if "--enable-debug". Defaults to "no" but allows
dnl "--enable-debug=full" as per kdevelop. Typically used after
dnl AC_ARG_ENABLE(debug).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_DEBUG],
[
	if test "$enable_debug" = "no" -o -z "$enable_debug"
	then
		:
	else
		AC_DEFINE(G_WITH_DEBUG,1,[Define to enable debug messages at compile-time])
	fi
])

dnl GCONFIG_FN_ENABLE_DNSBL
dnl -----------------------
dnl Enables DNSBL unless "--disable-dnsbl" is used.
dnl Typically used after AC_ARG_ENABLE(dnsbl).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_DNSBL],
[
	if test "$enable_dnsbl" = "no"
	then
		AM_CONDITIONAL([GCONFIG_DNSBL],[false])
	else
		AM_CONDITIONAL([GCONFIG_DNSBL],[true])
	fi
])

dnl GCONFIG_FN_ENABLE_EPOLL
dnl -----------------------
dnl Enables the epoll event-loop if epoll is available and "--disable-epoll"
dnl is not used. Typically used after GCONFIG_FN_EPOLL and AC_ARG_ENABLE(epoll).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_EPOLL],
[
	if test "$enable_epoll" != "no"
	then
		if test "$gconfig_cv_epoll" = "no"
		then
			if test "$enable_epoll" = "yes"
			then
				AC_MSG_WARN([ignoring --enable-epoll])
			fi
			gconfig_use_epoll="no"
		else
			gconfig_use_epoll="yes"
		fi
	else
		gconfig_use_epoll="no"
	fi

	if test "$gconfig_use_epoll" = "yes" ; then
		AC_DEFINE(GCONFIG_ENABLE_EPOLL,1,[Define true to use epoll])
	else
		AC_DEFINE(GCONFIG_ENABLE_EPOLL,0,[Define true to use epoll])
	fi
	AM_CONDITIONAL([GCONFIG_EPOLL],test "$gconfig_use_epoll" = "yes")
])

dnl GCONFIG_FN_ENABLE_GUI
dnl ---------------------
dnl Allows for "if GCONFIG_GUI" conditionals in makefiles, based on "--enable-gui"
dnl or "gconfig_have_qt" and "gconfig_qt_build" if "auto". Typically used after
dnl GCONFIG_FN_QT, GCONFIG_FN_QT_BUILD and AC_ARG_ENABLE(gui).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_GUI],
[
	if test "$enable_gui" = "no"
	then
		gconfig_gui="no"
	:
	elif test "$enable_gui" = "yes"
	then
		gconfig_gui="yes"
		if test "$gconfig_have_qt" = "no"
		then
			AC_MSG_WARN([gui enabled but no qt tools found])
		fi
		if test "$gconfig_qt_build" = "no"
		then
			AC_MSG_WARN([gui enabled but qt does not compile])
		fi
	:
	else
		if test "$gconfig_have_qt" = "yes" -a "$gconfig_qt_build" = "yes"
		then
			gconfig_gui="yes"
		else
			gconfig_gui="no"
		fi
	fi

	if test "$gconfig_gui" = "no" -a "$enable_gui" != "no"
	then
		gconfig_warnings="$gconfig_warnings qt_graphical_user_interface"
	fi

	AC_SUBST([GCONFIG_QT_LIBS],[$QT_LIBS])
	AC_SUBST([GCONFIG_QT_CFLAGS],[$QT_CFLAGS])
	AC_SUBST([GCONFIG_QT_MOC],[$QT_MOC])

	AM_CONDITIONAL([GCONFIG_GUI],[test "$gconfig_gui" = "yes"])
])

dnl GCONFIG_FN_ENABLE_INSTALL_HOOK
dnl ------------------------------
dnl The "--disable-install-hook" option can be used to disable tricksy install
dnl steps when building a package for distribution.
dnl
dnl Typically used after AC_ARG_ENABLE(install-hook).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_INSTALL_HOOK],
[
	AM_CONDITIONAL([GCONFIG_INSTALL_HOOK],test "$enable_install_hook" != "no")
])

dnl GCONFIG_FN_ENABLE_INTERFACE_NAMES
dnl ---------------------------------
dnl Enables interface-name source files in makefiles unless
dnl "--disable-interface-names" is used. Typically used
dnl after AC_ARG_ENABLE(interface-names).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_INTERFACE_NAMES],
[
	AM_CONDITIONAL([GCONFIG_INTERFACE_NAMES],test "$enable_interface_names" != "no")
])

dnl GCONFIG_FN_ENABLE_MAC
dnl ---------------------
dnl Enables mac tweaks if "--enable-mac" is used. Typically used after
dnl AC_ARG_ENABLE(mac).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_MAC],
[
	AM_CONDITIONAL([GCONFIG_MAC],test "$enable_mac" = "yes" -o "`uname`" = "Darwin")
])

dnl GCONFIG_FN_ENABLE_POP
dnl ---------------------
dnl Disables POP if "--disable-pop" is used.
dnl Typically used after AC_ARG_ENABLE(pop).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_POP],
[
	if test "$enable_pop" = "no"
	then
		gconfig_use_pop="no"
	else
		gconfig_use_pop="yes"
	fi

	AM_CONDITIONAL([GCONFIG_POP],test "$gconfig_use_pop" = "yes")
])

dnl GCONFIG_FN_ENABLE_STD_THREAD
dnl ----------------------------
dnl Defines GCONFIG_ENABLE_STD_THREAD based on the GCONFIG_FN_CXX_STD_THREAD
dnl result, unless "--disable-std-thread" has disabled it. Using
dnl "--disable-std-thread" is useful for old versions of mingw32-w64.
dnl
dnl Typically used after GCONFIG_FN_CXX_STD_THREAD and AC_ARG_ENABLE(std-thread).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_STD_THREAD],
[
	if test "$enable_std_thread" = "no"
	then
		AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,0,[Define true to use std::thread])
	else
		if test "$enable_std_thread" = "yes"
		then
			if test "$gconfig_cxx_std_thread" = "no"
			then
				AC_MSG_WARN([std::thread test compilation failed - see config.log])
				AC_MSG_WARN([try setting CXXFLAGS etc to enable the compiler's c++11 threading support])
				AC_MSG_ERROR([cannot enable std::thread because the feature test failed])
			fi
			AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,1,[Define true to use std::thread])
		else
			if test "$gconfig_cxx_std_thread" = "yes"
			then
				AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,1,[Define true to use std::thread])
			else
				AC_DEFINE(GCONFIG_ENABLE_STD_THREAD,0,[Define true to use std::thread])
			fi
		fi
	fi
])

dnl GCONFIG_FN_ENABLE_SUBMISSION
dnl ----------------------------
dnl Enables submission-tool functionality.
dnl Typically used after AC_ARG_ENABLE(submission).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_SUBMISSION],
[
	if test "$enable_submission" = "yes"
	then
		AC_DEFINE(GCONFIG_ENABLE_SUBMISSION,1,[Define true to enable submission-tool functionality])
	else
		AC_DEFINE(GCONFIG_ENABLE_SUBMISSION,0,[Define true to enable submission-tool functionality])
	fi
])

dnl GCONFIG_FN_ENABLE_TESTING
dnl -------------------------
dnl Disables make-check tests if "--disable-testing" is used.
dnl Eg. "make distcheck DISTCHECK_CONFIGURE_FLAGS=--disable-testing".
dnl Typically used after AC_ARG_ENABLE(testing).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_TESTING],
[
	AM_CONDITIONAL([GCONFIG_TESTING],test "$enable_testing" != "no")
])

dnl GCONFIG_FN_ENABLE_UDS
dnl ---------------------
dnl Enables unix domain sockets if detected unless "--disable-uds" is
dnl used. Requires GCONFIG_FN_UDS to set gconfig_cv_uds.
dnl Typically used after AC_ARG_ENABLE(uds).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_UDS],
[
	AC_REQUIRE([GCONFIG_FN_UDS])
	if test "$enable_uds" = "no"
	then
		AM_CONDITIONAL([GCONFIG_UDS],[false])
	else
		if test "$enable_uds" = "yes" -a "$gconfig_cv_uds" = "no"
		then
			AC_MSG_WARN([forcing use of unix domain sockets even though not detected])
		fi
		AM_CONDITIONAL([GCONFIG_UDS],[true])
	fi
])

dnl GCONFIG_FN_ENABLE_VERBOSE
dnl -------------------------
dnl Defines "GCONFIG_NO_LOG" if "--disable-verbose". Typically used after
dnl AC_ARG_ENABLE(verbose).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_VERBOSE],
[
	if test "$enable_verbose" != "no"
	then
		:
	else
		AC_DEFINE(GCONFIG_NO_LOG,1,[Define to disable the G_LOG macro])
	fi
])

dnl GCONFIG_FN_ENABLE_WINDOWS
dnl -------------------------
dnl Enables windows tweaks if "--enable-windows" is used. This is normally only
dnl required for doing a cross-compilation from linux. Typically used after
dnl AC_ARG_ENABLE(windows).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_WINDOWS],
[
	if test "$enable_windows" = "yes" -o "`uname -o 2>/dev/null`" = "Msys"
	then
		AC_DEFINE(GCONFIG_WINDOWS,1,[Define true for a windows build])
		AC_DEFINE(GCONFIG_MINGW,1,[Define true for a windows build using the mingw tool chain])
	else
		AC_DEFINE(GCONFIG_WINDOWS,0,[Define true for a windows build])
		AC_DEFINE(GCONFIG_MINGW,0,[Define true for a windows build using the mingw tool chain])
	fi
	AM_CONDITIONAL([GCONFIG_WINDOWS],test "$enable_windows" = "yes" -o "`uname -o 2>/dev/null`" = "Msys")
])

dnl GCONFIG_FN_EPOLL
dnl ----------------
dnl Tests for linux epoll().
dnl
AC_DEFUN([GCONFIG_FN_EPOLL],
[AC_CACHE_CHECK([for epoll],[gconfig_cv_epoll],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/epoll.h>]
			[int fd = 0 ;]
		] ,
		[
			[fd = epoll_create1( EPOLL_CLOEXEC ) ;]
		])] ,
		[gconfig_cv_epoll=yes],
		[gconfig_cv_epoll=no])
])
	if test "$gconfig_cv_epoll" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_EPOLL,1,[Define true to enable use of epoll])
	else
		AC_DEFINE(GCONFIG_HAVE_EPOLL,0,[Define true to enable use of epoll])
	fi
])

dnl GCONFIG_FN_EXECVPE
dnl ------------------
dnl Tests for execvpe().
dnl
AC_DEFUN([GCONFIG_FN_EXECVPE],
[AC_CACHE_CHECK([for execvpe],[gconfig_cv_execvpe],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <unistd.h>]
			[char * array[] = { 0 , 0 } ;]
			[int rc = 0 ;]
		],
		[
			[rc = execvpe( "path" , array , array ) ;]
		])],
		gconfig_cv_execvpe=yes ,
		gconfig_cv_execvpe=no )
])
	if test "$gconfig_cv_execvpe" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_EXECVPE,1,[Define true if have execvpe() in unistd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_EXECVPE,0,[Define true if have execvpe() in unistd.h])
	fi
])

dnl GCONFIG_FN_EXTENDED_OPEN
dnl ------------------------
dnl Defines GCONFIG_HAVE_EXTENDED_OPEN if fstream open
dnl can take a third, share-mode parameter.
dnl
AC_DEFUN([GCONFIG_FN_EXTENDED_OPEN],
[AC_CACHE_CHECK([for extended fstream open],[gconfig_cv_extended_open],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <windows.h>]
			[#include <share.h>]
			[#include <fstream>]
		],
		[
			[std::ofstream stream ;]
			[stream.open(".",std::ios_base::out,_SH_DENYNO) ;]
		])],
		gconfig_cv_extended_open=yes ,
		gconfig_cv_extended_open=no )
])
	if test "$gconfig_cv_extended_open" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_EXTENDED_OPEN,1,[Define true if extended fstream::open() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_EXTENDED_OPEN,0,[Define true if extended fstream::open() is available])
	fi
])

dnl GCONFIG_FN_FOPEN_S
dnl ------------------
dnl Defines GCONFIG_HAVE_FSOPEN if fopen_s() is available.
dnl
AC_DEFUN([GCONFIG_FN_FOPEN_S],
[AC_CACHE_CHECK([for fopen_s()],[gconfig_cv_fopen_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdio.h>]
			[#include <share.h>]
			[FILE * fp = 0 ;]
			[errno_t e = 0 ;]
		],
		[
			[e = fopen_s(&fp,"foo","w") ;]
		])],
		gconfig_cv_fopen_s=yes ,
		gconfig_cv_fopen_s=no )
])
	if test "$gconfig_cv_fopen_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_FOPEN_S,1,[Define true if fopen_s() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_FOPEN_S,0,[Define true if fopen_s() is available])
	fi
])

dnl GCONFIG_FN_FSOPEN
dnl -----------------
dnl Defines GCONFIG_HAVE_FSOPEN if _fsopen() is available.
dnl
AC_DEFUN([GCONFIG_FN_FSOPEN],
[AC_CACHE_CHECK([for _fsopen()],[gconfig_cv_fsopen],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdio.h>]
			[#include <share.h>]
			[FILE * fp = 0 ;]
		],
		[
			[fp = _fsopen("foo","w",_SH_DENYNO) ;]
		])],
		gconfig_cv_fsopen=yes ,
		gconfig_cv_fsopen=no )
])
	if test "$gconfig_cv_fsopen" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_FSOPEN,1,[Define true if _fsopen() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_FSOPEN,0,[Define true if _fsopen() is available])
	fi
])

dnl GCONFIG_FN_GAISTRERROR
dnl ----------------------
dnl Tests for gai_strerror() (see getaddinfo(3)).
dnl
AC_DEFUN([GCONFIG_FN_GAISTRERROR],
[AC_CACHE_CHECK([for gai_strerror()],[gconfig_cv_gaistrerror],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <iphlpapi.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netdb.h>]
			[#endif]
			[const char * p = 0;]
		],
		[
			[p = gai_strerror( 123 ) ;]
		])],
		gconfig_cv_gaistrerror=yes ,
		gconfig_cv_gaistrerror=no )
])
	if test "$gconfig_cv_gaistrerror" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GAISTRERROR,1,[Define true if gai_strerror() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_GAISTRERROR,0,[Define true if gai_strerror() is available])
	fi
])

dnl GCONFIG_FN_GETENV_S
dnl -------------------
dnl Tests for getenv_s().
dnl
AC_DEFUN([GCONFIG_FN_GETENV_S],
[AC_CACHE_CHECK([for getenv_s],[gconfig_cv_getenv_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[#include <stdlib.h>]
			[size_t n = 10 ;]
			[char buf[10] ;]
		],
		[
			[getenv_s( &n , buf , 10U , "foo" ) ;]
		])],
		gconfig_cv_getenv_s=yes ,
		gconfig_cv_getenv_s=no )
])
	if test "$gconfig_cv_getenv_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETENV_S,1,[Define true if getenv_s in stdlib.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETENV_S,0,[Define true if getenv_s in stdlib.h])
	fi
])

dnl GCONFIG_FN_GETGRNAM
dnl -------------------
dnl Tests for getgrnam().
dnl
AC_DEFUN([GCONFIG_FN_GETGRNAM],
[AC_CACHE_CHECK([for getgrnam],[gconfig_cv_getgrnam],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <grp.h>]
			[struct group * p = 0 ;]
		],
		[
			[p = getgrnam( "x" ) ;]
		])],
		gconfig_cv_getgrnam=yes ,
		gconfig_cv_getgrnam=no )
])
	if test "$gconfig_cv_getgrnam" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETGRNAM,1,[Define true if getgrnam in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETGRNAM,0,[Define true if getgrnam in pwd.h])
	fi
])

dnl GCONFIG_FN_GETGRNAM_R
dnl ---------------------
dnl Tests for getgrnam_r().
dnl
AC_DEFUN([GCONFIG_FN_GETGRNAM_R],
[AC_CACHE_CHECK([for getgrnam_r],[gconfig_cv_getgrnam_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <grp.h>]
			[char buf[100] ;]
			[struct group p ;]
			[struct group * p_out = 0 ;]
		],
		[
			[getgrnam_r( "x" , &p , buf , 100U , &p_out ) ;]
		])],
		gconfig_cv_getgrnam_r=yes ,
		gconfig_cv_getgrnam_r=no )
])
	if test "$gconfig_cv_getgrnam_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETGRNAM_R,1,[Define true if getgrnam_r in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETGRNAM_R,0,[Define true if getgrnam_r in pwd.h])
	fi
])

dnl GCONFIG_FN_GETPWNAM
dnl -------------------
dnl Tests for getpwnam().
dnl
AC_DEFUN([GCONFIG_FN_GETPWNAM],
[AC_CACHE_CHECK([for getpwnam],[gconfig_cv_getpwnam],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <pwd.h>]
			[struct passwd * p = 0 ;]
		],
		[
			[p = getpwnam( "x" ) ;]
		])],
		gconfig_cv_getpwnam=yes ,
		gconfig_cv_getpwnam=no )
])
	if test "$gconfig_cv_getpwnam" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM,1,[Define true if getpwnam in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM,0,[Define true if getpwnam in pwd.h])
	fi
])

dnl GCONFIG_FN_GETPWNAM_R
dnl ---------------------
dnl Tests for getpwnam_r().
dnl
AC_DEFUN([GCONFIG_FN_GETPWNAM_R],
[AC_CACHE_CHECK([for getpwnam_r],[gconfig_cv_getpwnam_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <pwd.h>]
			[char buf[100] ;]
			[struct passwd p ;]
			[struct passwd * p_out = 0 ;]
		],
		[
			[getpwnam_r( "x" , &p , buf , 100U , &p_out ) ;]
		])],
		gconfig_cv_getpwnam_r=yes ,
		gconfig_cv_getpwnam_r=no )
])
	if test "$gconfig_cv_getpwnam_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM_R,1,[Define true if getpwnam_r in pwd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GETPWNAM_R,0,[Define true if getpwnam_r in pwd.h])
	fi
])

dnl GCONFIG_FN_GETTEXT
dnl --------------------
dnl Tests for gettext and sets gconfig_cv_gettext. Used before
dnl AC_ARG_WITH and GCONFIG_FN_WITH_GETTEXT.
dnl
dnl Typically needs CFLAGS, LIBS and LDFLAGS etc. to be set
dnl correctly.
dnl
dnl See also GCONFIG_FN_GETTEXT_NEW and AM_GNU_GETTEXT.
dnl
AC_DEFUN([GCONFIG_FN_GETTEXT],
[AC_CACHE_CHECK([for gettext],[gconfig_cv_gettext],
[
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <libintl.h>]
			[const char * p = 0;]
			[const char * dir = 0;]
			[const char * md = 0;]
		],
		[
			[p = gettext( "foo" );]
			[dir = bindtextdomain( "foo" , "bar" );]
			[md = textdomain( "foo" );]
		])],
		gconfig_cv_gettext=yes ,
		gconfig_cv_gettext=no )
])
])

dnl GCONFIG_FN_GMTIME_R
dnl -------------------
dnl Tests for gmtime_r().
dnl
AC_DEFUN([GCONFIG_FN_GMTIME_R],
[AC_CACHE_CHECK([for gmtime_r],[gconfig_cv_gmtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <time.h>]
			[time_t t = 0 ;]
			[struct tm b ;]
			[struct tm * bp = 0 ;]
		],
		[
			[bp = gmtime_r(&t,&b) ;]
		])],
		gconfig_cv_gmtime_r=yes ,
		gconfig_cv_gmtime_r=no )
])
	if test "$gconfig_cv_gmtime_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GMTIME_R,1,[Define true if gmtime_r in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GMTIME_R,0,[Define true if gmtime_r in time.h])
	fi
])

dnl GCONFIG_FN_GMTIME_S
dnl -------------------
dnl Tests for gmtime_s().
dnl
AC_DEFUN([GCONFIG_FN_GMTIME_S],
[AC_CACHE_CHECK([for gmtime_s],[gconfig_cv_gmtime_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[#include <time.h>]
			[time_t t = 0 ;]
			[struct tm b ;]
		],
		[
			[gmtime_s( &b , &t ) ;]
		])],
		gconfig_cv_gmtime_s=yes ,
		gconfig_cv_gmtime_s=no )
])
	if test "$gconfig_cv_gmtime_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GMTIME_S,1,[Define true if gmtime_s in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_GMTIME_S,0,[Define true if gmtime_s in time.h])
	fi
])

dnl GCONFIG_FN_ICONV_LIBC
dnl ---------------------
dnl Tests for iconv in libc.
dnl
dnl Note that using AC_SEARCH_LIBS is no good because the test does not
dnl include the header, and including the header can modify the library
dnl requirements.
dnl
dnl See also "/usr/share/aclocal/iconv.m4" for a more complete solution.
dnl
AC_DEFUN([GCONFIG_FN_ICONV_LIBC],
[AC_CACHE_CHECK([for iconv in libc],[gconfig_cv_iconv_libc],
[
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <iconv.h>]
		],
		[
			[iconv_t i = iconv_open( "C" , "utf8" ) ;]
			[iconv( i , NULL , NULL , NULL , NULL ) ;]
			[iconv_close(i) ;]
		])],
		gconfig_cv_iconv_libc=yes ,
		gconfig_cv_iconv_libc=no )
])
])

dnl GCONFIG_FN_ICONV_LIBICONV
dnl -------------------------
dnl Tests for iconv in libiconv. Adds to LIBS if required.
dnl
AC_DEFUN([GCONFIG_FN_ICONV_LIBICONV],
[AC_CACHE_CHECK([for iconv in libiconv],[gconfig_cv_iconv_libiconv],
[
	gconfig_save_LIBS="$LIBS"
	LIBS="$LIBS -liconv"
	AC_LINK_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <iconv.h>]
		],
		[
			[iconv_t i = iconv_open( "C" , "utf8" ) ;]
			[iconv( i , NULL , NULL , NULL , NULL ) ;]
			[iconv_close(i) ;]
		])],
		gconfig_cv_iconv_libiconv=yes ,
		gconfig_cv_iconv_libiconv=no )
	])
	if test "$gconfig_cv_iconv_libiconv" = "no" ; then
		LIBS="$gconfig_save_LIBS"
	fi
])
])

dnl GCONFIG_FN_ICONV
dnl ----------------
dnl Tests for iconv.
dnl
AC_DEFUN([GCONFIG_FN_ICONV],
[
	AC_REQUIRE([GCONFIG_FN_ICONV_LIBC])
	AC_REQUIRE([GCONFIG_FN_ICONV_LIBICONV])
	if test "$gconfig_cv_iconv_libc" = "yes" -o "$gconfig_cv_iconv_libiconv" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_ICONV,1,[Define true to enable use of iconv])
	else
		AC_DEFINE(GCONFIG_HAVE_ICONV,0,[Define true to enable use of iconv])
	fi
	AM_CONDITIONAL([GCONFIG_ICONV],[test "$gconfig_cv_iconv_libc" = "yes" -o "$gconfig_cv_iconv_libiconv" = "yes"])
])

dnl GCONFIG_FN_IFINDEX
dnl ------------------
dnl Tests for struct ifreq ifr_ifindex and SIOCGIFINDEX.
dnl
AC_DEFUN([GCONFIG_FN_IFINDEX],
[AC_CACHE_CHECK([for ifreq ifr_index],[gconfig_cv_ifindex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <iphlpapi.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <arpa/inet.h>]
				[#include <net/if.h>]
				[#include <sys/ioctl.h>]
			[#endif]
			[struct ifreq req ;]
			[int i = 0 ;]
		],
		[
			[(void) ioctl( i , SIOCGIFINDEX , &req , sizeof(req) );]
			[i = req.ifr_ifindex ;]
		])],
		gconfig_cv_ifindex=yes ,
		gconfig_cv_ifindex=no )
])
	if test "$gconfig_cv_ifindex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IFINDEX,1,[Define true if struct ifreq has ifr_ifindex])
	else
		AC_DEFINE(GCONFIG_HAVE_IFINDEX,0,[Define true if struct ifreq has ifr_ifindex])
	fi
])

dnl GCONFIG_FN_IFNAMETOINDEX
dnl -------------------------
dnl Tests for if_nametoindex().
dnl
AC_DEFUN([GCONFIG_FN_IFNAMETOINDEX],
[AC_CACHE_CHECK([for if_nametoindex()],[gconfig_cv_ifnametoindex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <iphlpapi.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <arpa/inet.h>]
				[#include <net/if.h>]
			[#endif]
			[unsigned int i = 0 ;]
		],
		[
			[i = if_nametoindex("net0") ;]
		])],
		gconfig_cv_ifnametoindex=yes ,
		gconfig_cv_ifnametoindex=no )
])
	if test "$gconfig_cv_ifnametoindex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IFNAMETOINDEX,1,[Define true if if_nametoindex() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IFNAMETOINDEX,0,[Define true if if_nametoindex() is available])
	fi
])

dnl GCONFIG_FN_IFNAMETOLUID
dnl -----------------------
dnl Tests for ConvertInterfaceNameToLuid().
dnl
AC_DEFUN([GCONFIG_FN_IFNAMETOLUID],
[AC_CACHE_CHECK([for ConvertInterfaceNameToLuid()],[gconfig_cv_ifnametoluid],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <iphlpapi.h>]
			[#endif]
			[NET_LUID luid ;]
		],
		[
			[ConvertInterfaceNameToLuidA( "eth0" , &luid ) ;]
		])],
		gconfig_cv_ifnametoluid=yes ,
		gconfig_cv_ifnametoluid=no )
])
	if test "$gconfig_cv_ifnametoluid" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IFNAMETOLUID,1,[Define true if ConvertInterfaceNameToLuid() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IFNAMETOLUID,0,[Define true if ConvertInterfaceNameToLuid() is available])
	fi
])

dnl GCONFIG_FN_INET_NTOP
dnl --------------------
dnl Tests for inet_ntop().
dnl
AC_DEFUN([GCONFIG_FN_INET_NTOP],
[AC_CACHE_CHECK([for inet_ntop()],[gconfig_cv_inet_ntop],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
				[#include <arpa/inet.h>]
			[#endif]
			[char a[100] ;]
			[char buffer[100] ;]
		],
		[
			[inet_ntop(AF_INET,a,buffer,10) ;]
		])],
		gconfig_cv_inet_ntop=yes ,
		gconfig_cv_inet_ntop=no )
])
	if test "$gconfig_cv_inet_ntop" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INET_NTOP,1,[Define true if inet_ntop() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_INET_NTOP,0,[Define true if inet_ntop() is available])
	fi
])

dnl GCONFIG_FN_INET_PTON
dnl --------------------
dnl Tests for inet_pton().
dnl
AC_DEFUN([GCONFIG_FN_INET_PTON],
[AC_CACHE_CHECK([for inet_pton()],[gconfig_cv_inet_pton],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
				[#include <arpa/inet.h>]
			[#endif]
			[char buffer[100] ;]
		],
		[
			[inet_pton(AF_INET,"0",buffer) ;]
		])],
		gconfig_cv_inet_pton=yes ,
		gconfig_cv_inet_pton=no )
])
	if test "$gconfig_cv_inet_pton" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INET_PTON,1,[Define true if inet_pton() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_INET_PTON,0,[Define true if inet_pton() is available])
	fi
])

dnl GCONFIG_FN_IOVEC_SIMPLE
dnl -----------------------
dnl Tests whether struct iovec is available and matches
dnl the layout of a trivial {char*,size_t} structure.
dnl
AC_DEFUN([GCONFIG_FN_IOVEC_SIMPLE],
[AC_CACHE_CHECK([for iovec layout],[gconfig_cv_iovec_is_simple],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <cstddef>]
			[#include <sys/types.h>]
			[#include <sys/uio.h>]
			[struct simple { const char * p ; std::size_t n ; } ;]
		],
		[
            static_assert( sizeof(simple) == sizeof(::iovec) , "" ) ;
            static_assert( alignof(simple) == alignof(::iovec) , "" ) ;
            static_assert( sizeof(simple::p) == sizeof(::iovec::iov_base) , "" ) ;
            static_assert( sizeof(simple::n) == sizeof(::iovec::iov_len) , "" ) ;
            static_assert( offsetof(simple,p) == offsetof(::iovec,iov_base) , "" ) ;
            static_assert( offsetof(simple,n) == offsetof(::iovec,iov_len) , "" ) ;
		])],
		gconfig_cv_iovec_is_simple=yes ,
		gconfig_cv_iovec_is_simple=no )
])
	if test "$gconfig_cv_iovec_is_simple" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IOVEC_SIMPLE,1,[Define true if struct iovec has a simple layout])
	else
		AC_DEFINE(GCONFIG_HAVE_IOVEC_SIMPLE,0,[Define true if struct iovec has a simple layout])
	fi
])

dnl GCONFIG_FN_IPV6
dnl ---------------
dnl Tests for a minimum set of IPv6 features available.
dnl
AC_DEFUN([GCONFIG_FN_IPV6],
[AC_CACHE_CHECK([for ipv6],[gconfig_cv_ipv6],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
				[#include <arpa/inet.h>]
				[#include <netdb.h>]
			[#endif]
			[struct sockaddr_in6 * p = 0;]
			[int f = AF_INET6 ;]
			[struct addrinfo ai ;]
			[struct addrinfo *aip = 0 ;]
		],
		[
			[getaddrinfo("local","http",&ai,&aip) ;]
		])],
		gconfig_cv_ipv6=yes ,
		gconfig_cv_ipv6=no )
])
	if test "$gconfig_cv_ipv6" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_IPV6,1,[Define true if ipv6 is available])
	else
		AC_DEFINE(GCONFIG_HAVE_IPV6,0,[Define true if ipv6 is available])
	fi
])

dnl GCONFIG_FN_LOCALTIME_R
dnl ----------------------
dnl Tests for localtime_r().
dnl
AC_DEFUN([GCONFIG_FN_LOCALTIME_R],
[AC_CACHE_CHECK([for localtime_r],[gconfig_cv_localtime_r],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <time.h>]
			[time_t t = 0 ;]
			[struct tm b ;]
			[struct tm * bp ;]
		],
		[
			[bp = localtime_r( &t , &b ) ;]
		])],
		gconfig_cv_localtime_r=yes ,
		gconfig_cv_localtime_r=no )
])
	if test "$gconfig_cv_localtime_r" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_R,1,[Define true if localtime_r in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_R,0,[Define true if localtime_r in time.h])
	fi
])

dnl GCONFIG_FN_LOCALTIME_S
dnl ----------------------
dnl Tests for localtime_s().
dnl
AC_DEFUN([GCONFIG_FN_LOCALTIME_S],
[AC_CACHE_CHECK([for localtime_s],[gconfig_cv_localtime_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[#include <time.h>]
			[time_t t ;]
			[struct tm b ;]
		],
		[
			[localtime_s( &b , &t ) ;]
		])],
		gconfig_cv_localtime_s=yes ,
		gconfig_cv_localtime_s=no )
])
	if test "$gconfig_cv_localtime_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_S,1,[Define true if localtime_s in time.h])
	else
		AC_DEFINE(GCONFIG_HAVE_LOCALTIME_S,0,[Define true if localtime_s in time.h])
	fi
])

dnl GCONFIG_FN_NETROUTE
dnl --------------------
dnl Tests for BSD routing sockets.
dnl
AC_DEFUN([GCONFIG_FN_NETROUTE],
[AC_CACHE_CHECK([for routing sockets],[gconfig_cv_netroute],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <sys/time.h>]
			[#include <sys/socket.h>]
			[#include <net/if.h>]
			[#include <net/route.h>]
			[struct rt_msghdr header1 ;]
			[struct ifa_msghdr header2 ;]
		] ,
		[
			[(void) socket( PF_ROUTE , SOCK_RAW , AF_INET ) ;]
			[header1.rtm_msglen = header2.ifam_msglen = 0 ;]
			[header1.rtm_type = RTM_ADD ;]
			[header2.ifam_type = RTM_NEWADDR ;]
		])] ,
		[gconfig_cv_netroute=yes],
		[gconfig_cv_netroute=no])
])
	if test "$gconfig_cv_netroute" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_NETROUTE,1,[Define true to enable use of bsd routing sockets])
	else
		AC_DEFINE(GCONFIG_HAVE_NETROUTE,0,[Define true to enable use of bsd routing sockets])
		if test "`uname`" = "NetBSD" -o "`uname`" = "FreeBSD" -o "`uname`" = "OpenBSD" ; then
			gconfig_warnings="$gconfig_warnings netroute_network_interface_event_notification"
		fi
	fi
])

dnl GCONFIG_FN_PAM_CONST
dnl --------------------
dnl Tests for constness of the PAM API. Use after GCONFIG_FN_PAM.
dnl
AC_DEFUN([GCONFIG_FN_PAM_CONST],
[AC_CACHE_CHECK([for pam constness],[gconfig_cv_pam_const],
[
	if test "$gconfig_cv_pam_in_security" = "yes"
	then
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
			[
				[#include <security/pam_appl.h>]
				[const void * vp = 0; pam_handle_t * pam = 0; int rc = 0;]
			],
			[
				[rc = pam_get_item( pam , PAM_USER , &vp )]
			])],
			gconfig_cv_pam_const=yes ,
			gconfig_cv_pam_const=no )
	else
		if test "$gconfig_cv_pam_in_pam" = "yes"
		then
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
				[
					[#include <pam/pam_appl.h>]
					[const void * vp = 0; pam_handle_t * pam = 0; int rc = 0;]
				],
				[
					[rc = pam_get_item( pam , PAM_USER , &vp )]
				])],
				gconfig_cv_pam_const=yes ,
				gconfig_cv_pam_const=no )
		else
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
				[
					[#include <pam_appl.h>]
					[const void * vp = 0; pam_handle_t * pam = 0; int rc = 0;]
				],
				[
					[rc = pam_get_item( pam , PAM_USER , &vp )]
				])],
				gconfig_cv_pam_const=yes ,
				gconfig_cv_pam_const=no )
		fi
	fi
])
	if test "$gconfig_cv_pam_const" = "yes" ; then
		AC_DEFINE(GCONFIG_PAM_CONST,1,[Define true if the PAM API uses const])
	else
		AC_DEFINE(GCONFIG_PAM_CONST,0,[Define true if the PAM API uses const])
	fi
])

dnl GCONFIG_FN_PAM_IN_WHATEVER
dnl --------------------------
dnl Tests for pam header file location. Sets local gconfig_cv_pam_in_whatever
dnl depeding on whether the header is included from the whatever directory.
dnl
AC_DEFUN([GCONFIG_FN_PAM_IN_SECURITY],
[AC_CACHE_CHECK([for security/pam_appl.h],[gconfig_cv_pam_in_security],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <security/pam_appl.h>]
			[pam_conv c ; pam_handle_t *pam = 0 ; int rc = 0 ;]
		] ,
		[
			[rc = pam_start("login","user",&c,&pam) ;]
		])] ,
		[gconfig_cv_pam_in_security=yes],
		[gconfig_cv_pam_in_security=no])
])
])
AC_DEFUN([GCONFIG_FN_PAM_IN_PAM],
[AC_CACHE_CHECK([for pam/pam_appl.h],[gconfig_cv_pam_in_pam],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <pam/pam_appl.h>]
			[pam_conv c ; pam_handle_t *pam = 0 ; int rc = 0 ;]
		] ,
		[
			[rc = pam_start("login","user",&c,&pam) ;]
		])] ,
		[gconfig_cv_pam_in_pam=yes],
		[gconfig_cv_pam_in_pam=no])
])
])
AC_DEFUN([GCONFIG_FN_PAM_IN_INCLUDE],
[AC_CACHE_CHECK([for include/pam_appl.h],[gconfig_cv_pam_in_include],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <pam_appl.h>]
			[pam_conv c ; pam_handle_t *pam = 0 ; int rc = 0 ;]
		] ,
		[
			[rc = pam_start("login","user",&c,&pam) ;]
		])] ,
		[gconfig_cv_pam_in_include=yes],
		[gconfig_cv_pam_in_include=no])
])
])

dnl GCONFIG_FN_PAM
dnl --------------
dnl Tests for pam headers.
dnl
AC_DEFUN([GCONFIG_FN_PAM],
[
	AC_REQUIRE([GCONFIG_FN_PAM_IN_SECURITY])
	AC_REQUIRE([GCONFIG_FN_PAM_IN_PAM])
	AC_REQUIRE([GCONFIG_FN_PAM_IN_INCLUDE])

	if test "$gconfig_cv_pam_in_security" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_SECURITY,1,[Define true to include pam_appl.h from security])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_SECURITY,0,[Define true to include pam_appl.h from security])
	fi

	if test "$gconfig_cv_pam_in_pam" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_PAM,1,[Define true to include pam_appl.h from pam])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_PAM,0,[Define true to include pam_appl.h from pam])
	fi

	if test "$gconfig_cv_pam_in_include" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_INCLUDE,1,[Define true to include pam_appl.h as-is])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM_IN_INCLUDE,0,[Define true to include pam_appl.h as-is])
	fi
])

dnl GCONFIG_FN_PROC_PIDPATH
dnl -----------------------
dnl Tests for proc_pidpath() (osx).
dnl
AC_DEFUN([GCONFIG_FN_PROC_PIDPATH],
[AC_CACHE_CHECK([for proc_pidpath],[gconfig_cv_proc_pidpath],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdlib.h>]
			[#include <libproc.h>]
			[char buf[100] ;]
			[int rc = 0 ;]
		],
		[
			[rc = proc_pidpath( (pid_t)1 , buf , sizeof(buf) ) ;]
		])],
		gconfig_cv_proc_pidpath=yes ,
		gconfig_cv_proc_pidpath=no )
])
	if test "$gconfig_cv_proc_pidpath" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PROC_PIDPATH,1,[Define true if have OSX proc_pidpath()])
	else
		AC_DEFINE(GCONFIG_HAVE_PROC_PIDPATH,0,[Define true if have OSX proc_pidpath()])
	fi
])

dnl GCONFIG_FN_PROG_WINDMC
dnl ----------------------
dnl Sets GCONFIG_WINDMC in makefiles as the windows message compiler,
dnl based on $CXX but overridable from the configure command-line or
dnl environment.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDMC],[
	if test "$GCONFIG_WINDMC" = ""
	then
		GCONFIG_WINDMC="`echo \"$CXX\" | sed -E 's/-[gc]\+\+.*/-windmc/'`"
		if test "$GCONFIG_WINDMC" = ""
		then
			GCONFIG_WINDMC="./fakemc.exe"
		fi
	fi
	AC_MSG_CHECKING([message compiler])
	AC_MSG_RESULT([$GCONFIG_WINDMC])
	AC_SUBST([GCONFIG_WINDMC])
])

dnl GCONFIG_FN_PROG_WINDRES
dnl -----------------------
dnl Sets GCONFIG_WINDRES in makefiles as the windows resource compiler,
dnl based on $CXX but overridable from the configure command-line or
dnl environment.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDRES],[
	if test "$GCONFIG_WINDRES" = ""
	then
		GCONFIG_WINDRES="`echo \"$CXX\" | sed -E 's/-[gc]\+\+.*/-windres/'`"
		if test "$GCONFIG_WINDRES" = ""
		then
			GCONFIG_WINDRES="windres"
		fi
	fi
	AC_MSG_CHECKING([for resource compiler])
	AC_MSG_RESULT([$GCONFIG_WINDRES])
	AC_SUBST([GCONFIG_WINDRES])
])

dnl GCONFIG_FN_PTHREAD_SIGMASK
dnl --------------------------
dnl Tests for pthread_sigmask().
dnl
AC_DEFUN([GCONFIG_FN_PTHREAD_SIGMASK],
[AC_CACHE_CHECK([for pthread_sigmask],[gconfig_cv_pthread_sigmask],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <csignal>]
			[#include <cstddef>]
			[sigset_t set ;]
			[int rc ;]
		],
		[
			[sigemptyset( &set ) ;]
			[sigaddset( &set , SIGTERM ) ;]
			[rc = pthread_sigmask( SIG_BLOCK|SIG_UNBLOCK|SIG_SETMASK , &set , NULL ) ;]
		])],
		gconfig_cv_pthread_sigmask=yes ,
		gconfig_cv_pthread_sigmask=no )
])
	if test "$gconfig_cv_pthread_sigmask" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PTHREAD_SIGMASK,1,[Define true if pthread_sigmask() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_PTHREAD_SIGMASK,0,[Define true if pthread_sigmask() is available])
	fi
])

dnl GCONFIG_FN_PUTENV_S
dnl -------------------
dnl Tests for _putenv_s().
dnl
AC_DEFUN([GCONFIG_FN_PUTENV_S],
[AC_CACHE_CHECK([for putenv_s],[gconfig_cv_putenv_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[#include <stdlib.h>]
		],
		[
			[_putenv_s( "name" , "value" ) ;]
		])],
		gconfig_cv_putenv_s=yes ,
		gconfig_cv_putenv_s=no )
])
	if test "$gconfig_cv_putenv_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_PUTENV_S,1,[Define true if putenv_s in stdlib.h])
	else
		AC_DEFINE(GCONFIG_HAVE_PUTENV_S,0,[Define true if putenv_s in stdlib.h])
	fi
])

dnl GCONFIG_FN_QT_BUILD
dnl -------------------
dnl Tests for successful Qt5 compilation if GCONFIG_FN_QT
dnl has set gconfig_have_qt. Does nothing if --disable-gui.
dnl Sets gconfig_qt_build.
dnl
AC_DEFUN([GCONFIG_FN_QT_BUILD],
[AC_CACHE_CHECK([for QT compilation],[gconfig_cv_qt_build],
[
	if test "$gconfig_have_qt" = "yes" -a "$enable_gui" != "no"
	then
		gconfig_save_LIBS="$LIBS"
		gconfig_save_CXXFLAGS="$CXXFLAGS"
		LIBS="$LIBS $QT_LIBS"
		CXXFLAGS="$CXXFLAGS $QT_CFLAGS"
		AC_LINK_IFELSE([AC_LANG_PROGRAM(
			[
				[#include <QtCore/QtCore>]
				[#if QT_VERSION < 0x050000]
				[#error Qt is too old]
				[#endif]
				[#include <QtGui/QtGui>]
				[#include <QtWidgets/QtWidgets>]
				[#include <QtCore/QtPlugin>]
			],
			[
				[throw QSize(1,1).width() ;]
			])],
			gconfig_cv_qt_build=yes ,
			gconfig_cv_qt_build=no )
		LIBS="$gconfig_save_LIBS"
		CXXFLAGS="$gconfig_save_CXXFLAGS"
	else
		gconfig_cv_qt_build=no
	fi
])
	gconfig_qt_build="$gconfig_cv_qt_build"
])

dnl GCONFIG_FN_QT
dnl -------------
dnl Tests for Qt5. Sets gconfig_have_qt, QT_MOC, QT_LRELEASE, QT_LIBS and
dnl QT_CFLAGS. A fallback copy of "pkg.m4" should be included in the
dnl distribution.
dnl
AC_DEFUN([GCONFIG_FN_QT],
[
	# skip the madness if the user has specified everything we need
	if test "$QT_MOC" != "" -a "$QT_CFLAGS" != "" -a "$QT_LIBS" != ""
	then
		if echo "$QT_MOC" | grep -q /
		then
			QT_LRELEASE="`dirname \"$QT_MOC\"`/lrelease"
		else
			QT_LRELEASE="lrelease"
		fi
		AC_MSG_CHECKING([for QT])
		AC_MSG_RESULT([overridden])
		gconfig_have_qt=yes
	else

		# try pkg-config -- this says 'checking for QT'
		PKG_CHECK_MODULES([QT],[Qt5Widgets > 5],
			[
				gconfig_pkgconfig_qt=yes
			],
			[
				gconfig_pkgconfig_qt=no
				AC_MSG_NOTICE([no QT 5 pkg-config])
			]
		)

		# allow the moc command to be defined with QT_MOC on the configure
		# command-line, typically also with CXXFLAGS and LIBS pointing to Qt
		# headers and libraries
		AC_ARG_VAR([QT_MOC],[moc command for QT])

		if echo "$QT_MOC" | grep -q /
		then
			QT_LRELEASE="`dirname \"$QT_MOC\"`/lrelease"
		else
			QT_LRELEASE="lrelease"
		fi

		# or build the moc command using pkg-config results
		if test "$QT_MOC" = ""
		then
			if test "$gconfig_pkgconfig_qt" = "yes"
			then
				QT_MOC="`$PKG_CONFIG --variable=host_bins Qt5Core`/moc"
				QT_LRELEASE="`$PKG_CONFIG --variable=host_bins Qt5Core`/lrelease"
				QT_CHOOSER="`$PKG_CONFIG --variable=exec_prefix Qt5Core`/bin/qtchooser"
				if test -x "$QT_MOC" ; then : ; else QT_MOC="" ; fi
				if test -x "$QT_LRELEASE" ; then : ; else QT_LRELEASE="" ; fi
				if test -x "$QT_CHOOSER" ; then : ; else QT_CHOOSER="" ; fi
				if test "$QT_MOC" = "" -a "$QT_CHOOSER" != ""
				then
					QT_MOC="$QT_CHOOSER -run-tool=moc -qt=qt5"
				fi
				if test "$QT_LRELEASE" = "" -a "$QT_CHOOSER" != ""
				then
					QT_LRELEASE="$QT_CHOOSER -run-tool=lrelease -qt=qt5"
				fi
			fi
		fi

		# or find moc on the path
		if test "$QT_MOC" = ""
		then
			AC_PATH_PROG([QT_MOC],[moc])
		fi

		if test "$QT_LRELEASE" = ""
		then
			AC_PATH_PROG([QT_LRELEASE],[lrelease])
			if test "$QT_LRELEASE" = ""
			then
				QT_LRELEASE=false
			fi
		fi

		if test "$QT_MOC" != ""
		then
			AC_MSG_NOTICE([QT moc command: $QT_MOC])
		fi

		# set gconfig_have_qt, QT_CFLAGS and QT_LIBS iff we have a moc command
		if test "$QT_MOC" != ""
		then
			gconfig_have_qt="yes"
			if test "$gconfig_pkgconfig_qt" = "yes"
			then
				QT_CFLAGS="-fPIC `$PKG_CONFIG --cflags Qt5Widgets`"
				QT_LIBS="`$PKG_CONFIG --libs Qt5Widgets`"
			else
				QT_CFLAGS="-fPIC"
				QT_LIBS=""
			fi
		else
			gconfig_have_qt="no"
		fi

		# mac modifications
		if test "$QT_MOC" != "" -a "`uname`" = "Darwin"
		then
			QT_DIR="`dirname $QT_MOC`/.."
			QT_CFLAGS="-F $QT_DIR/lib"
			QT_LIBS="-F $QT_DIR/lib -framework QtWidgets -framework QtGui -framework QtCore"
		fi
	fi
])

dnl GCONFIG_FN_READLINK
dnl -------------------
dnl Tests for readlink().
dnl
AC_DEFUN([GCONFIG_FN_READLINK],
[AC_CACHE_CHECK([for readlink],[gconfig_cv_readlink],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <unistd.h>]
			[long n = 0 ;]
			[char buf[10] ;]
		],
		[
			[n = readlink( "foo" , buf , sizeof(buf) ) ;]
		])],
		gconfig_cv_readlink=yes ,
		gconfig_cv_readlink=no )
])
	if test "$gconfig_cv_readlink" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_READLINK,1,[Define true if have readlink() in unistd.h])
	else
		AC_DEFINE(GCONFIG_HAVE_READLINK,0,[Define true if have readlink() in unistd.h])
	fi
])

dnl GCONFIG_FN_RTNETLINK
dnl --------------------
dnl Tests for linux rtnetlink.
dnl
AC_DEFUN([GCONFIG_FN_RTNETLINK],
[AC_CACHE_CHECK([for rtnetlink],[gconfig_cv_rtnetlink],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <asm/types.h>]
			[#include <sys/socket.h>]
			[#include <linux/netlink.h>]
			[#include <linux/rtnetlink.h>]
			[int protocol = 0 ;]
			[struct sockaddr_nl sa ;]
		] ,
		[
			[protocol = NETLINK_ROUTE ;]
			[sa.nl_family = AF_NETLINK ;]
			[sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR ;]
		])] ,
		[gconfig_cv_rtnetlink=yes],
		[gconfig_cv_rtnetlink=no])
])
	if test "$gconfig_cv_rtnetlink" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_RTNETLINK,1,[Define true to enable use of linux rtnetlink])
	else
		AC_DEFINE(GCONFIG_HAVE_RTNETLINK,0,[Define true to enable use of linux rtnetlink])
		if test "`uname`" = "Linux" ; then
			gconfig_warnings="$gconfig_warnings rtnetlink_network_interface_event_notification"
		fi
	fi
])

dnl GCONFIG_FN_SEARCHLIBS_NAMESERVICE
dnl ---------------------------------
dnl Does AC_SEARCH_LIBS for the name service library.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_NAMESERVICE],[
	AC_SEARCH_LIBS([gethostbyname],[nsl],[gconfig_have_libnsl=yes],[gconfig_have_libnsl=no])
])

dnl GCONFIG_FN_SEARCHLIBS_SOCKET
dnl ----------------------------
dnl Does AC_SEARCH_LIBS for the sockets library.
dnl
AC_DEFUN([GCONFIG_FN_SEARCHLIBS_SOCKET],[
	AC_SEARCH_LIBS([connect],[socket],[gconfig_have_libsocket=yes],[gconfig_have_libsocket=no])
])

dnl GCONFIG_FN_SET_DIRECTORIES_E
dnl ----------------------------
dnl Sets makefile variables for install directory paths, usually incorporating
dnl the package name. These should be used in conjunction with DESTDIR when
dnl writing install rules in makefiles. Standard extensions of these variables,
dnl such as e_sysconf_DATA, are also magically meaningful.
dnl
AC_DEFUN([GCONFIG_FN_SET_DIRECTORIES_E],
[
	if test "$e_libdir" = ""
	then
		e_libdir="$libexecdir/$PACKAGE"
	fi
	if test "$e_examplesdir" = ""
	then
		e_examplesdir="$e_libdir/examples"
	fi
	if test "$e_sysconfdir" = ""
	then
		e_sysconfdir="$sysconfdir"
	fi
	if test "$e_docdir" = ""
	then
		e_docdir="$docdir"
		if test "$e_docdir" = ""
		then
			e_docdir="$datadir/$PACKAGE/doc"
		fi
	fi
	if test "$e_spooldir" = ""
	then
		e_spooldir="$localstatedir/spool/$PACKAGE"
	fi
	if test "$e_pamdir" = ""
	then
		e_pamdir="$sysconfdir/pam.d"
	fi
	if test "$e_initdir" = ""
	then
		e_initdir="$e_libdir/init"
	fi
	if test "$e_bsdinitdir" = ""
	then
		if test "$gconfig_bsd" = "yes"
		then
			e_bsdinitdir="$sysconfdir/rc.d"
		else
			e_bsdinitdir="$e_libdir/init/bsd"
		fi
	fi
	if test "$e_icondir" = ""
	then
		e_icondir="$datadir/$PACKAGE"
	fi
	if test "$e_trdir" = ""
	then
		e_trdir="$datadir/$PACKAGE"
	fi
	if test "$e_rundir" = ""
	then
		# (linux fhs says "/run", not "/var/run")
		e_rundir="/run/$PACKAGE"
	fi
	if test "$e_systemddir" = ""
	then
		# keep as an example file by default - installed fully by rpm and deb
		e_systemddir="$e_examplesdir"
		#e_systemddir="$libdir/systemd/system"
	fi

	AC_SUBST([e_docdir])
	AC_SUBST([e_initdir])
	AC_SUBST([e_bsdinitdir])
	AC_SUBST([e_icondir])
	AC_SUBST([e_trdir])
	AC_SUBST([e_spooldir])
	AC_SUBST([e_examplesdir])
	AC_SUBST([e_libdir])
	AC_SUBST([e_pamdir])
	AC_SUBST([e_sysconfdir])
	AC_SUBST([e_rundir])
	AC_SUBST([e_systemddir])
])

dnl GCONFIG_FN_SETGROUPS
dnl --------------------
dnl Tests for setgroups().
dnl
AC_DEFUN([GCONFIG_FN_SETGROUPS],
[AC_CACHE_CHECK([for setgroups],[gconfig_cv_setgroups],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <unistd.h>]
			[#include <grp.h>]
		],
		[
			[setgroups(0,0) ;]
		])],
		gconfig_cv_setgroups=yes ,
		gconfig_cv_setgroups=no )
])
	if test "$gconfig_cv_setgroups" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SETGROUPS,1,[Define true if setgroups is available])
	else
		AC_DEFINE(GCONFIG_HAVE_SETGROUPS,0,[Define true if setgroups is available])
	fi
])

dnl GCONFIG_FN_SETPGRP_BSD
dnl ----------------------
dnl Tests for the bsd two-parameter form of setpgrp().
dnl
AC_DEFUN([GCONFIG_FN_SETPGRP_BSD],
[AC_CACHE_CHECK([for bsd setpgrp],[gconfig_cv_setpgrp_bsd],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <unistd.h>]
		],
		[
			[setpgrp(0,0) ;]
		])],
		gconfig_cv_setpgrp_bsd=yes ,
		gconfig_cv_setpgrp_bsd=no )
])
	if test "$gconfig_cv_setpgrp_bsd" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SETPGRP_BSD,1,[Define true if setpgrp has two parameters])
	else
		AC_DEFINE(GCONFIG_HAVE_SETPGRP_BSD,0,[Define true if setpgrp has two parameters])
	fi
])

dnl GCONFIG_FN_SIGPROCMASK
dnl ----------------------
dnl Tests for sigprocmask().
dnl
AC_DEFUN([GCONFIG_FN_SIGPROCMASK],
[AC_CACHE_CHECK([for sigprocmask],[gconfig_cv_sigprocmask],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <csignal>]
			[#include <cstddef>]
			[sigset_t set ;]
			[int rc ;]
		],
		[
			[sigemptyset( &set ) ;]
			[sigaddset( &set , SIGTERM ) ;]
			[rc = sigprocmask( SIG_BLOCK|SIG_UNBLOCK|SIG_SETMASK , &set , NULL ) ;]
		])],
		gconfig_cv_sigprocmask=yes ,
		gconfig_cv_sigprocmask=no )
])
	if test "$gconfig_cv_sigprocmask" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SIGPROCMASK,1,[Define true if sigprocmask() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_SIGPROCMASK,0,[Define true if sigprocmask() is available])
	fi
])

dnl GCONFIG_FN_SIN6_LEN
dnl -------------------
dnl Tests whether sin6_len is in sockaddr_in6.
dnl
AC_DEFUN([GCONFIG_FN_SIN6_LEN],
[AC_CACHE_CHECK([for sin6_len],[gconfig_cv_sin6_len],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
				[#include <netinet/in.h>]
			[#endif]
			[struct sockaddr_in6 s ;]
		],
		[
			[ s.sin6_len = 1;]
		])],
		gconfig_cv_sin6_len=yes ,
		gconfig_cv_sin6_len=no )
])
	if test "$gconfig_cv_sin6_len" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SIN6_LEN,1,[Define true if sockaddr_in6 has a sin6_len member])
	else
		AC_DEFINE(GCONFIG_HAVE_SIN6_LEN,0,[Define true if sockaddr_in6 has a sin6_len member])
	fi
])

dnl GCONFIG_FN_SOPEN
dnl ----------------
dnl Defines GCONFIG_HAVE_SOPEN if _sopen() is available.
dnl
AC_DEFUN([GCONFIG_FN_SOPEN],
[AC_CACHE_CHECK([for _sopen()],[gconfig_cv_sopen],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <io.h>]
			[#include <share.h>]
			[#include <fcntl.h>]
			[int fd = 0 ;]
		],
		[
			[fd = _sopen("foo",_O_WRONLY,_SH_DENYNO) ;]
		])],
		gconfig_cv_sopen=yes ,
		gconfig_cv_sopen=no )
])
	if test "$gconfig_cv_sopen" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SOPEN,1,[Define true if _sopen() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_SOPEN,0,[Define true if _sopen() is available])
	fi
])

dnl GCONFIG_FN_SOPEN_S
dnl ------------------
dnl Defines GCONFIG_HAVE_SOPEN if _sopen_s() is available.
dnl
AC_DEFUN([GCONFIG_FN_SOPEN_S],
[AC_CACHE_CHECK([for _sopen_s()],[gconfig_cv_sopen_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <io.h>]
			[#include <share.h>]
			[#include <fcntl.h>]
			[#include <sys/stat.h>]
			[int fd = 0 ;]
		],
		[
			[errno_t e = _sopen_s(&fd,"foo",_O_WRONLY,_SH_DENYNO,_S_IWRITE) ;]
			[if( e ) return 1 ;]
		])],
		gconfig_cv_sopen_s=yes ,
		gconfig_cv_sopen_s=no )
])
	if test "$gconfig_cv_sopen_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SOPEN_S,1,[Define true if _sopen_s() is available])
	else
		AC_DEFINE(GCONFIG_HAVE_SOPEN_S,0,[Define true if _sopen_s() is available])
	fi
])

dnl GCONFIG_FN_STATBUF_NSEC
dnl -----------------------
dnl Tests whether stat provides nanosecond file times.
dnl
AC_DEFUN([GCONFIG_FN_STATBUF_NSEC],
[AC_CACHE_CHECK([for statbuf nanoseconds],[gconfig_cv_statbuf_nsec],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <sys/types.h>]
				[#include <sys/stat.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/stat.h>]
				[#include <unistd.h>]
			[#endif]
			[struct stat statbuf ;]
		],
		[
			[statbuf.st_atim.tv_nsec = 0 ;]
		])],
		gconfig_cv_statbuf_nsec=yes ,
		gconfig_cv_statbuf_nsec=no )
])
	if test "$gconfig_cv_statbuf_nsec" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_STATBUF_NSEC,1,[Define true if statbuf has a st_atim.tv_nsec member])
	else
		AC_DEFINE(GCONFIG_HAVE_STATBUF_NSEC,0,[Define true if statbuf has a st_atim.tv_nsec member])
	fi
])

dnl GCONFIG_FN_STATBUF_TIMESPEC
dnl ---------------------------
dnl Tests whether stat uses _timespec substructures (eg. MacOS).
dnl
AC_DEFUN([GCONFIG_FN_STATBUF_TIMESPEC],
[AC_CACHE_CHECK([for statbuf timespec],[gconfig_cv_statbuf_timespec],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
				[#include <sys/types.h>]
				[#include <sys/stat.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/stat.h>]
				[#include <unistd.h>]
			[#endif]
			[struct stat statbuf ;]
		],
		[
			[statbuf.st_mtimespec.tv_sec = 0 ;]
			[statbuf.st_mtimespec.tv_nsec = 0 ;]
		])],
		gconfig_cv_statbuf_timespec=yes ,
		gconfig_cv_statbuf_timespec=no )
])
	if test "$gconfig_cv_statbuf_timespec" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_STATBUF_TIMESPEC,1,[Define true if statbuf has a st_atimespec member])
	else
		AC_DEFINE(GCONFIG_HAVE_STATBUF_TIMESPEC,0,[Define true if statbuf has a st_atimespec member])
	fi
])

dnl GCONFIG_FN_STRNCPY_S
dnl --------------------
dnl Tests for strncpy_s().
dnl
AC_DEFUN([GCONFIG_FN_STRNCPY_S],
[AC_CACHE_CHECK([for strncpy_s],[gconfig_cv_strncpy_s],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[#include <string.h>]
			[char buffer[100] ;]
		],
		[
			[strncpy_s( buffer , sizeof(buffer) , "foo" , 3U ) ;]
		])],
		gconfig_cv_strncpy_s=yes ,
		gconfig_cv_strncpy_s=no )
])
	if test "$gconfig_cv_strncpy_s" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_STRNCPY_S,1,[Define true if strncpy_s in string.h])
	else
		AC_DEFINE(GCONFIG_HAVE_STRNCPY_S,0,[Define true if strncpy_s in string.h])
	fi
])

dnl GCONFIG_FN_TLS
dnl --------------
dnl Chooses TLS libraries. Typically used after GCONFIG_FN_TLS_OPENSSL,
dnl GCONFIG_FN_TLS_MBEDTLS, AC_ARG_WITH(openssl) and AC_ARG_WITH(mbedtls).
dnl
AC_DEFUN([GCONFIG_FN_TLS],
[
	if test "$with_openssl" = "yes" -a "$gconfig_cv_ssl_openssl" = "no"
	then
		AC_MSG_ERROR([cannot use --with-openssl: openssl is not available: check config.log and try setting CPPFLAGS])
	fi
	if test "$with_openssl" != "no" -a "$gconfig_cv_ssl_openssl" = "yes"
	then
		gconfig_ssl_use_openssl=yes
	else
		gconfig_ssl_use_openssl=no
	fi

	if test "$with_mbedtls" = "yes" -a "$gconfig_cv_ssl_mbedtls" = "no"
	then
		AC_MSG_ERROR([cannot use --with-mbedtls: mbedtls is not available: check config.log and try setting CPPFLAGS])
	fi
	if test "$with_mbedtls" != "no" -a "$gconfig_cv_ssl_mbedtls" = "yes"
	then
		gconfig_ssl_use_mbedtls=yes
	else
		gconfig_ssl_use_mbedtls=no
	fi

	if test "$gconfig_ssl_use_openssl" = "yes" -a "$gconfig_ssl_use_mbedtls" = "yes"
	then
		gconfig_ssl_notice="openssl and mbedtls"
		gconfig_ssl_use_none=no
		gconfig_ssl_use_both=yes
		gconfig_ssl_use_openssl_only=no
		gconfig_ssl_use_mbedtls_only=no
		GCONFIG_TLS_LIBS="$gconfig_ssl_mbedtls_libs $gconfig_ssl_openssl_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "yes" -a "$gconfig_ssl_use_mbedtls" = "no"
	then
		gconfig_ssl_notice="openssl"
		gconfig_ssl_use_none=no
		gconfig_ssl_use_both=no
		gconfig_ssl_use_openssl_only=yes
		gconfig_ssl_use_mbedtls_only=no
		GCONFIG_TLS_LIBS="$gconfig_ssl_openssl_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "no" -a "$gconfig_ssl_use_mbedtls" = "yes"
	then
		gconfig_ssl_notice="mbedtls"
		gconfig_ssl_use_none=no
		gconfig_ssl_use_both=no
		gconfig_ssl_use_openssl_only=no
		gconfig_ssl_use_mbedtls_only=yes
		GCONFIG_TLS_LIBS="$gconfig_ssl_mbedtls_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "no" -a "$gconfig_ssl_use_mbedtls" = "no"
	then
		gconfig_ssl_notice="none"
		gconfig_ssl_use_none=yes
		gconfig_ssl_use_both=no
		gconfig_ssl_use_openssl_only=no
		gconfig_ssl_use_mbedtls_only=no
		GCONFIG_TLS_LIBS=""
	fi

	if test "$gconfig_ssl_use_none" = "yes"
	then
		gconfig_warnings="$gconfig_warnings openssl/mbedtls_transport_layer_security"
	fi

	AC_SUBST([GCONFIG_TLS_LIBS])
	AM_CONDITIONAL([GCONFIG_TLS_USE_BOTH],test "$gconfig_ssl_use_both" = "yes")
	AM_CONDITIONAL([GCONFIG_TLS_USE_OPENSSL],test "$gconfig_ssl_use_openssl_only" = "yes")
	AM_CONDITIONAL([GCONFIG_TLS_USE_MBEDTLS],test "$gconfig_ssl_use_mbedtls_only" = "yes")
	AM_CONDITIONAL([GCONFIG_TLS_USE_NONE],test "$gconfig_ssl_use_none" = "yes")
	AC_MSG_NOTICE([using tls library: $gconfig_ssl_notice])
])

dnl GCONFIG_FN_TLS_MBEDTLS
dnl ----------------------
dnl Tests for mbedTLS.
dnl
AC_DEFUN([GCONFIG_FN_TLS_MBEDTLS],
[AC_CACHE_CHECK([for mbedtls],[gconfig_cv_ssl_mbedtls],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <mbedtls/ssl.h>]
			[mbedtls_ssl_context * p = 0 ;]
		],
		[
		])],
		gconfig_cv_ssl_mbedtls=yes,
		gconfig_cv_ssl_mbedtls=no )
])

	if test "$gconfig_cv_ssl_mbedtls" = "yes"
	then
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <mbedtls/net.h>]
			[int x = MBEDTLS_ERR_NET_RECV_FAILED;]
		],
		[
		])],
		gconfig_ssl_mbedtls_net_h=yes,
		gconfig_ssl_mbedtls_net_h=no )

		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <mbedtls/net_sockets.h>]
			[int x = MBEDTLS_ERR_NET_RECV_FAILED;]
		],
		[
		])],
		gconfig_ssl_mbedtls_net_sockets_h=yes,
		gconfig_ssl_mbedtls_net_sockets_h=no )
	fi

	if test "$gconfig_cv_ssl_mbedtls" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_MBEDTLS,1,[Define true to enable mbedtls])
		gconfig_ssl_mbedtls_libs="-lmbedtls -lmbedx509 -lmbedcrypto"
	else
		AC_DEFINE(GCONFIG_HAVE_MBEDTLS,0,[Define true to enable mbedtls])
		gconfig_ssl_mbedtls_libs=""
	fi

	if test "$gconfig_ssl_mbedtls_net_h" = "yes" -a "$gconfig_ssl_mbedtls_net_sockets_h" = "no" ; then
		AC_DEFINE(GCONFIG_HAVE_MBEDTLS_NET_H,1,[Define true to use deprecated mbedtls/net.h])
	else
		AC_DEFINE(GCONFIG_HAVE_MBEDTLS_NET_H,0,[Define true to use deprecated mbedtls/net.h])
	fi
])

dnl GCONFIG_FN_TLS_OPENSSL
dnl ----------------------
dnl Tests for OpenSSL.
dnl
AC_DEFUN([GCONFIG_FN_TLS_OPENSSL],
[AC_CACHE_CHECK([for openssl],[gconfig_cv_ssl_openssl],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <openssl/ssl.h>]
			[SSL_CTX * p = 0 ;]
		],
		[
		])],
		gconfig_cv_ssl_openssl=yes,
		gconfig_cv_ssl_openssl=no )
])
	if test "$gconfig_cv_ssl_openssl" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL,1,[Define true to enable openssl])
		gconfig_ssl_openssl_libs="-lssl -lcrypto"
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL,0,[Define true to enable openssl])
		gconfig_ssl_openssl_libs=""
	fi

	if test "$gconfig_cv_ssl_openssl" = "yes"
	then
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[TLS_method();]])],
		gconfig_ssl_openssl_tls_method=yes,
		gconfig_ssl_openssl_tls_method=no )

		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <openssl/ssl.h>]],
		[[SSL_set_min_proto_version((SSL*)0,0);]])],
		gconfig_ssl_openssl_min_max=yes,
		gconfig_ssl_openssl_min_max=no )
	else
		gconfig_ssl_openssl_tls_method=no
		gconfig_ssl_openssl_min_max=no
	fi

	if test "$gconfig_ssl_openssl_tls_method" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLS_METHOD,1,[Define true if openssl has TLS_method])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_TLS_METHOD,0,[Define true if openssl has TLS_method])
	fi
	if test "$gconfig_ssl_openssl_min_max" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_MIN_MAX,1,[Define true if openssl has SSL_set_min_proto_version])
	else
		AC_DEFINE(GCONFIG_HAVE_OPENSSL_MIN_MAX,0,[Define true if openssl has SSL_set_min_proto_version])
	fi
])

dnl GCONFIG_FN_TYPE_ERRNO_T
dnl -----------------------
dnl Tests for errno_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_ERRNO_T],
[AC_CACHE_CHECK([for errno_t],[gconfig_cv_type_errno_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <error.h>]
			[errno_t e = 42 ;]
		],
		[
		])],
		gconfig_cv_type_errno_t=yes,
		gconfig_cv_type_errno_t=no )
])
	if test "$gconfig_cv_type_errno_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_ERRNO_T,1,[Define true if errno_t type definition in error.h])
	else
		AC_DEFINE(GCONFIG_HAVE_ERRNO_T,0,[Define true if errno_t type definition in error.h])
	fi
])

dnl GCONFIG_FN_TYPE_INT16
dnl -------------------------
dnl Tests for 16-bit integer types.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_INT16],
[AC_CACHE_CHECK([for 'int16'],[gconfig_cv_int16],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifndef _WIN32]
			[#include <stdint.h>]
			[int16_t n = 0 ;]
			[uint16_t m = 0U ;]
			[#endif]
		] ,
		[
		])],
		gconfig_cv_int16=yes ,
		gconfig_cv_int16=no )
])
	if test "$gconfig_cv_int16" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INT16,1,[Define true if compiler has int16_t in stdint.h])
	else
		AC_DEFINE(GCONFIG_HAVE_INT16,0,[Define true if compiler has int16_t in stdint.h])
	fi
])

dnl GCONFIG_FN_TYPE_INT32
dnl -------------------------
dnl Tests for 32-bit integer types.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_INT32],
[AC_CACHE_CHECK([for 'int32'],[gconfig_cv_int32],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifndef _WIN32]
			[#include <stdint.h>]
			[int32_t n = 0 ;]
			[uint32_t m = 0U ;]
			[#endif]
		] ,
		[
		])],
		gconfig_cv_int32=yes ,
		gconfig_cv_int32=no )
])
	if test "$gconfig_cv_int32" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INT32,1,[Define true if compiler has int32_t in stdint.h])
	else
		AC_DEFINE(GCONFIG_HAVE_INT32,0,[Define true if compiler has int32_t in stdint.h])
	fi
])

dnl GCONFIG_FN_TYPE_INT64
dnl -------------------------
dnl Tests for 64-bit integer types.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_INT64],
[AC_CACHE_CHECK([for 'int64'],[gconfig_cv_int64],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
			[#include <windows.h>]
			[INT64 n = 0 ;]
			[UINT64 m = 0U ;]
			[#else]
			[#include <stdint.h>]
			[int64_t n = 0 ;]
			[uint64_t m = 0U ;]
			[#endif]
		] ,
		[
		])],
		gconfig_cv_int64=yes ,
		gconfig_cv_int64=no )
])
	if test "$gconfig_cv_int64" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_INT64,1,[Define true if compiler has int64_t in stdint.h])
	else
		AC_DEFINE(GCONFIG_HAVE_INT64,0,[Define true if compiler has int64_t in stdint.h])
	fi
])

dnl GCONFIG_FN_TYPE_SOCKLEN_T
dnl -------------------------
dnl Tests for socklen_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t],[gconfig_cv_type_socklen_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#else]
				[#include <sys/types.h>]
				[#include <sys/socket.h>]
			[#endif]
			[socklen_t len = 42 ;]
		],
		[
			[len++ ;]
		])],
		gconfig_cv_type_socklen_t=yes,
		gconfig_cv_type_socklen_t=no )
])
	if test "$gconfig_cv_type_socklen_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SOCKLEN_T,1,[Define true if socklen_t type definition in sys/socket.h])
	else
		AC_DEFINE(GCONFIG_HAVE_SOCKLEN_T,0,[Define true if socklen_t type definition in sys/socket.h])
	fi
])

dnl GCONFIG_FN_TYPE_SSIZE_T
dnl -----------------------
dnl Tests for ssize_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_SSIZE_T],
[AC_CACHE_CHECK([for ssize_t],[gconfig_cv_type_ssize_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <stdio.h>]
			[ssize_t e = 42 ;]
		],
		[
		])],
		gconfig_cv_type_ssize_t=yes,
		gconfig_cv_type_ssize_t=no )
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <unistd.h>]
			[ssize_t e = 42 ;]
		],
		[
		])],
		gconfig_cv_type_ssize_t=yes )
])
	if test "$gconfig_cv_type_ssize_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_SSIZE_T,1,[Define true if ssize_t type is defined])
	else
		AC_DEFINE(GCONFIG_HAVE_SSIZE_T,0,[Define true if ssize_t type is defined])
	fi
])

dnl GCONFIG_FN_TYPE_UINTPTR_T
dnl -------------------------
dnl Tests for uintptr_t.
dnl
AC_DEFUN([GCONFIG_FN_TYPE_UINTPTR_T],
[AC_CACHE_CHECK([for uintptr_t],[gconfig_cv_type_uintptr_t],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <stdint.h>]
			[uintptr_t x = 0 ;]
		],
		[
		])],
		gconfig_cv_type_uintptr_t=yes,
		gconfig_cv_type_uintptr_t=no )
])
	if test "$gconfig_cv_type_uintptr_t" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_UINTPTR_T,1,[Define true if uintptr_t type is defined stdint.h])
	else
		AC_DEFINE(GCONFIG_HAVE_UINTPTR_T,0,[Define true if uintptr_t type is defined stdint.h])
	fi
])

dnl GCONFIG_FN_UDS_LEN
dnl ------------------
dnl Tests for BSD's 'sun_len' in 'sockaddr_un'.
dnl
AC_DEFUN([GCONFIG_FN_UDS_LEN],
[AC_CACHE_CHECK([for unix domain sockets],[gconfig_cv_uds_len],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <sys/socket.h>]
			[#include <sys/un.h>]
			[struct sockaddr_un a ;]
		] ,
		[
			[a.sun_len = 2U ;]
			[a.sun_family = AF_UNIX | PF_UNIX ;]
			[a.sun_path[0] = '\0' ;]
		])] ,
		[gconfig_cv_uds_len=yes],
		[gconfig_cv_uds_len=no])
])
	if test "$gconfig_cv_uds_len" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_UDS_LEN,1,[Define true if sockaddr_un has a sun_len field])
	else
		AC_DEFINE(GCONFIG_HAVE_UDS_LEN,0,[Define true if sockaddr_un has a sun_len field])
	fi
])

dnl GCONFIG_FN_UDS
dnl --------------
dnl Tests for unix domain socket support.
dnl
AC_DEFUN([GCONFIG_FN_UDS],
[AC_CACHE_CHECK([for unix domain sockets],[gconfig_cv_uds],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <sys/types.h>]
			[#include <sys/socket.h>]
			[#include <sys/un.h>]
			[struct sockaddr_un a ;]
		] ,
		[
			[a.sun_family = AF_UNIX | PF_UNIX ;]
			[a.sun_path[0] = '\0' ;]
		])] ,
		[gconfig_cv_uds=yes],
		[gconfig_cv_uds=no])
])
	if test "$gconfig_cv_uds" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_UDS,1,[Define true to use unix domain sockets])
	else
		AC_DEFINE(GCONFIG_HAVE_UDS,0,[Define true to use unix domain sockets])
	fi
])

dnl GCONFIG_FN_WARNINGS
dnl -------------------
dnl Displays a summary warning.
dnl
AC_DEFUN([GCONFIG_FN_WARNINGS],
[
	for gconfig_w in $gconfig_warnings ""
	do
		if test "$gconfig_w" != ""
		then
			echo "$gconfig_w" | sed 's/_/ /g' | while read gconfig_what gconfig_stuff
			do
				AC_MSG_WARN([missing $gconfig_what - no support for $gconfig_stuff])
			done
		fi
	done
])

dnl GCONFIG_FN_WINDOWS_CREATE_EVENT_EX
dnl -------------------------------------------
dnl Tests for CreateEventEx().
dnl
AC_DEFUN([GCONFIG_FN_WINDOWS_CREATE_EVENT_EX],
[AC_CACHE_CHECK([for windows CreateEventEx],[gconfig_cv_win_ce_ex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <synchapi.h>]
			[#endif]
			[HANDLE h = 0 ;]
		],
		[
			[h = CreateEventEx( NULL , NULL , 0 , 0 ) ;]
		])],
		gconfig_cv_win_ce_ex=yes,
		gconfig_cv_win_ce_ex=no )
])
	if test "$gconfig_cv_win_ce_ex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX,1,[Define true if windows CreateEventEx is available])
	else
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX,0,[Define true if windows CreateEventEx is available])
	fi
])

dnl GCONFIG_FN_WINDOWS_CREATE_WAITABLE_TIMER_EX
dnl -------------------------------------------
dnl Tests for CreateWaitableTimerEx().
dnl
AC_DEFUN([GCONFIG_FN_WINDOWS_CREATE_WAITABLE_TIMER_EX],
[AC_CACHE_CHECK([for windows CreateWaitableTimerEx],[gconfig_cv_win_cwt_ex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <ws2tcpip.h>]
			[#endif]
			[HANDLE h = 0 ;]
		],
		[
			[h = CreateWaitableTimerEx( NULL , NULL , 0 , 0 ) ;]
		])],
		gconfig_cv_win_cwt_ex=yes,
		gconfig_cv_win_cwt_ex=no )
])
	if test "$gconfig_cv_win_cwt_ex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX,1,[Define true if windows CreateWaitableTimerEx is available])
	else
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX,0,[Define true if windows CreateWaitableTimerEx is available])
	fi
])

dnl GCONFIG_FN_WINDOWS_INIT_COMMON_CONTROLS_EX
dnl ------------------------------------------
dnl Tests for InitCommonControlsEx().
dnl
AC_DEFUN([GCONFIG_FN_WINDOWS_INIT_COMMON_CONTROLS_EX],
[AC_CACHE_CHECK([for windows InitCommonControlsEx],[gconfig_cv_win_icc_ex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <winsock2.h>]
				[#include <windows.h>]
				[#include <commctrl.h>]
			[#endif]
			[INITCOMMONCONTROLSEX x ;]
			[BOOL rc = 0;]
		],
		[
			[rc = InitCommonControlsEx( &x ) ;]
		])],
		gconfig_cv_win_icc_ex=yes,
		gconfig_cv_win_icc_ex=no )
])
	if test "$gconfig_cv_win_icc_ex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX,1,[Define true if windows InitCommonControlsEx is available])
	else
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX,0,[Define true if windows InitCommonControlsEx is available])
	fi
])

dnl GCONFIG_FN_WINDOWS_STARTUP_INFO_EX
dnl ----------------------------------
dnl Tests for the STARTUPINFOEX structure.
dnl
AC_DEFUN([GCONFIG_FN_WINDOWS_STARTUP_INFO_EX],
[AC_CACHE_CHECK([for windows STARTUPINFOEX],[gconfig_cv_win_si_ex],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#ifdef _WIN32]
				[#include <windows.h>]
				[#include <processthreadsapi.h>]
			[#endif]
			[STARTUPINFOEXA x ;]
			[DWORD d ;]
		],
		[
			[x.lpAttributeList = NULL ;]
			[d = EXTENDED_STARTUPINFO_PRESENT ;]
		])],
		gconfig_cv_win_si_ex=yes,
		gconfig_cv_win_si_ex=no )
])
	if test "$gconfig_cv_win_si_ex" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX,1,[Define true if windows STARTUPINFOEX is available])
	else
		AC_DEFINE(GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX,0,[Define true if windows STARTUPINFOEX is available])
	fi
])

dnl GCONFIG_FN_WITH_DOXYGEN
dnl -----------------------
dnl Tests for doxygen. Typically used after AC_CHECK_PROG(doxygen) and
dnl AC_ARG_WITH(doxygen).
dnl
AC_DEFUN([GCONFIG_FN_WITH_DOXYGEN],
[
	if test "$with_doxygen" != ""
	then
		if test "$with_doxygen" = "yes" -a "$GCONFIG_HAVE_DOXYGEN" != "yes"
		then
			AC_MSG_WARN([forcing use of doxygen even though not found])
		fi
		GCONFIG_HAVE_DOXYGEN="$with_doxygen"
	fi
	AC_SUBST([GCONFIG_HAVE_DOXYGEN])
])

dnl GCONFIG_FN_WITH_GETTEXT
dnl -----------------------
dnl Enables GNU gettext if "--with-gettext" is used.
dnl Typically used after AC_ARG_WITH(gettext).
dnl
dnl Currently does nothing to check for, or add in, the
dnl relevant library code. See also AM_GNU_GETTEXT.
dnl
AC_DEFUN([GCONFIG_FN_WITH_GETTEXT],
[
	if test "$with_gettext" = "yes"
	then
		if test "$gconfig_cv_gettext" = "no"
		then
			AC_MSG_WARN([forcing use of gettext even though not detected])
			gconfig_cv_gettext="yes"
		fi
	else
		gconfig_cv_gettext="no"
	fi

	if test "$gconfig_cv_gettext" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_GETTEXT,1,[Define true to use gettext])
	else
		AC_DEFINE(GCONFIG_HAVE_GETTEXT,0,[Define true to use gettext])
	fi
	AM_CONDITIONAL([GCONFIG_GETTEXT],[test "$gconfig_cv_gettext" = "yes"])
])

dnl GCONFIG_FN_WITH_MAN2HTML
dnl ------------------------
dnl Tests for man2html. Typically used after AC_CHECK_PROG(man2html) and
dnl AC_ARG_WITH(man2html).
dnl
AC_DEFUN([GCONFIG_FN_WITH_MAN2HTML],
[
	if test "$with_man2html" != ""
	then
		if test "$with_man2html" = "yes" -a "$GCONFIG_HAVE_MAN2HTML" != "yes"
		then
			AC_MSG_WARN([forcing use of man2html even though not found])
		fi
		GCONFIG_HAVE_MAN2HTML="$with_man2html"
	fi
	AC_SUBST([GCONFIG_HAVE_MAN2HTML])
])

dnl GCONFIG_FN_WITH_PAM
dnl -------------------
dnl Tests for pam. Typically used after AC_ARG_WITH(pam).
dnl
AC_DEFUN([GCONFIG_FN_WITH_PAM],
[
	AC_REQUIRE([GCONFIG_FN_PAM])
	AC_REQUIRE([GCONFIG_FN_PAM_CONST])

	if test "$with_pam" = "no"
	then
		gconfig_use_pam="no"
	else

		AC_SEARCH_LIBS([pam_end],[pam],[gconfig_have_libpam=yes],[gconfig_have_libpam=no])

		gconfig_pam_compiles="no"
		if test "$gconfig_cv_pam_in_security" = "yes" -o "$gconfig_cv_pam_in_pam" = "yes" -o "$gconfig_cv_pam_in_include" = "yes"
		then
			gconfig_pam_compiles="yes"
		fi

		if test "$with_pam" = "yes" -a "$gconfig_pam_compiles" = "no"
		then
			AC_MSG_WARN([forcing use of pam even though it does not seem to compile])
		fi

		gconfig_use_pam="$with_pam"
		if test "$with_pam" = "" -a "$gconfig_pam_compiles" = "yes"
		then
			gconfig_use_pam="yes"
		fi
	fi

	if test "$gconfig_pam_compiles" != "yes" -a "$with_pam" != "no"
	then
		gconfig_warnings="$gconfig_warnings pam_pam_authentication"
	fi

	if test "$gconfig_use_pam" = "yes"
	then
		AC_DEFINE(GCONFIG_HAVE_PAM,1,[Define true to use pam])
	else
		AC_DEFINE(GCONFIG_HAVE_PAM,0,[Define true to use pam])
	fi
	AM_CONDITIONAL([GCONFIG_PAM],[test "$gconfig_use_pam" = "yes"])
])

