dnl Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
dnl GCONFIG_FN_CHECK_CXX
dnl ----------------------
dnl Checks c++ language features.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_CXX],[
	AC_REQUIRE([GCONFIG_FN_CXX_NULLPTR])
	AC_REQUIRE([GCONFIG_FN_CXX_CONSTEXPR])
	AC_REQUIRE([GCONFIG_FN_CXX_ENUM_CLASS])
	AC_REQUIRE([GCONFIG_FN_CXX_NOEXCEPT])
	AC_REQUIRE([GCONFIG_FN_CXX_OVERRIDE])
	AC_REQUIRE([GCONFIG_FN_CXX_FINAL])
	AC_REQUIRE([GCONFIG_FN_CXX_TYPE_TRAITS])
	AC_REQUIRE([GCONFIG_FN_CXX_EMPLACE])
	AC_REQUIRE([GCONFIG_FN_CXX_ALIGNMENT])
	AC_REQUIRE([GCONFIG_FN_CXX_MOVE])
	AC_REQUIRE([GCONFIG_FN_CXX_SHARED_PTR])
	AC_REQUIRE([GCONFIG_FN_CXX_STD_THREAD])
	AC_REQUIRE([GCONFIG_FN_CXX_STD_WSTRING])
	AC_REQUIRE([GCONFIG_FN_CXX_DELETED])
	AC_REQUIRE([GCONFIG_FN_CXX_DEFAULTED])
	AC_REQUIRE([GCONFIG_FN_CXX_INITIALIZER_LIST])
])

dnl GCONFIG_FN_CHECK_FUNCTIONS
dnl --------------------------
dnl Checks for various functions.
dnl
AC_DEFUN([GCONFIG_FN_CHECK_FUNCTIONS],[
	AC_REQUIRE([GCONFIG_FN_GETPWNAM])
	AC_REQUIRE([GCONFIG_FN_GETPWNAM_R])
	AC_REQUIRE([GCONFIG_FN_GMTIME_R])
	AC_REQUIRE([GCONFIG_FN_GMTIME_S])
	AC_REQUIRE([GCONFIG_FN_LOCALTIME_R])
	AC_REQUIRE([GCONFIG_FN_LOCALTIME_S])
	AC_REQUIRE([GCONFIG_FN_STRNCPY_S])
	AC_REQUIRE([GCONFIG_FN_GETENV_S])
	AC_REQUIRE([GCONFIG_FN_FSOPEN])
	AC_REQUIRE([GCONFIG_FN_READLINK])
	AC_REQUIRE([GCONFIG_FN_ICONV])
	AC_REQUIRE([GCONFIG_FN_PROC_PIDPATH])
	AC_REQUIRE([GCONFIG_FN_SETPGRP_BSD])
	AC_REQUIRE([GCONFIG_FN_SETGROUPS])
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
	AC_REQUIRE([GCONFIG_FN_STATBUF_NSEC])
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

dnl GCONFIG_FN_CXX_CONSTEXPR
dnl ------------------------
dnl Tests for c++ support for constexpr in a static initialisation.
dnl
AC_DEFUN([GCONFIG_FN_CXX_CONSTEXPR],
[AC_CACHE_CHECK([for c++ constexpr],[gconfig_cv_cxx_constexpr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[template <typename T> struct Foo {static constexpr int foo = 1;} ;]
		],
		[
		])],
		gconfig_cv_cxx_constexpr=yes ,
		gconfig_cv_cxx_constexpr=no )
])
	if test "$gconfig_cv_cxx_constexpr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_CONSTEXPR,1,[Define true if compiler supports c++ constexpr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_CONSTEXPR,0,[Define true if compiler supports c++ constexpr])
	fi
])

dnl GCONFIG_FN_CXX_DEFAULTED
dnl ----------------------
dnl Tests for c++ =default.
dnl
AC_DEFUN([GCONFIG_FN_CXX_DEFAULTED],
[AC_CACHE_CHECK([for c++ eq default],[gconfig_cv_cxx_defaulted],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) && __cplusplus < 200000L]
			[#error gcc is too noisy when using =delete without std=c++11]
			[#endif]
			[struct X { X() = default ; } ;]
		] ,
		[
		])],
		gconfig_cv_cxx_defaulted=yes ,
		gconfig_cv_cxx_defaulted=no )
])
	if test "$gconfig_cv_cxx_defaulted" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_DEFAULTED,1,[Define true if compiler supports c++ =default])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_DEFAULTED,0,[Define true if compiler supports c++ =default])
	fi
])

dnl GCONFIG_FN_CXX_DELETED
dnl ----------------------
dnl Tests for c++ =delete.
dnl
AC_DEFUN([GCONFIG_FN_CXX_DELETED],
[AC_CACHE_CHECK([for c++ eq delete],[gconfig_cv_cxx_deleted],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) && __cplusplus < 200000L]
			[#error gcc is too noisy when using =delete without std=c++11]
			[#endif]
			[struct X { X(const X&) = delete ; } ;]
		],
		[
		])],
		gconfig_cv_cxx_deleted=yes ,
		gconfig_cv_cxx_deleted=no )
])
	if test "$gconfig_cv_cxx_deleted" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_DELETED,1,[Define true if compiler supports c++ =delete])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_DELETED,0,[Define true if compiler supports c++ =delete])
	fi
])

dnl GCONFIG_FN_CXX_EMPLACE
dnl ----------------------
dnl Tests for c++ std::vector::emplace_back() etc.
dnl
AC_DEFUN([GCONFIG_FN_CXX_EMPLACE],
[AC_CACHE_CHECK([for c++ emplace_back and friends],[gconfig_cv_cxx_emplace],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <vector>]
			[std::vector<int> v ;]
		],
		[
			[v.emplace_back(1) ;]
		])],
		gconfig_cv_cxx_emplace=yes ,
		gconfig_cv_cxx_emplace=no )
])
	if test "$gconfig_cv_cxx_emplace" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_EMPLACE,1,[Define true if compiler has std::vector::emplace_back()])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_EMPLACE,0,[Define true if compiler has std::vector::emplace_back()])
	fi
])

dnl GCONFIG_FN_CXX_ENUM_CLASS
dnl -------------------------
dnl Tests for c++ support for class enums.
dnl
AC_DEFUN([GCONFIG_FN_CXX_ENUM_CLASS],
[AC_CACHE_CHECK([for c++ class enums],[gconfig_cv_cxx_enum_class],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[enum class Type { t1 , t2 } ;]
			[Type t = Type::t1 ;]
		],
		[
		])],
		gconfig_cv_cxx_enum_class=yes ,
		gconfig_cv_cxx_enum_class=no )
])
	if test "$gconfig_cv_cxx_enum_class" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_ENUM_CLASS,1,[Define true if compiler supports c++ class enums])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_ENUM_CLASS,0,[Define true if compiler supports c++ class enums])
	fi
])

dnl GCONFIG_FN_CXX_FINAL
dnl --------------------
dnl Tests for c++ final keyword.
dnl
AC_DEFUN([GCONFIG_FN_CXX_FINAL],
[AC_CACHE_CHECK([for c++ final keyword],[gconfig_cv_cxx_final],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) && __cplusplus < 200000L]
			[#error gcc is too noisy when using override/final without std=c++11]
			[#endif]
			[struct base { virtual void fn() {} } ;]
			[struct derived : public base { virtual void fn() final {} } ;]
			[derived d ;]
		],
		[
		])],
		gconfig_cv_cxx_final=yes ,
		gconfig_cv_cxx_final=no )
])
	if test "$gconfig_cv_cxx_final" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_FINAL,1,[Define true if compiler supports c++ final keyword])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_FINAL,0,[Define true if compiler supports c++ final keyword])
	fi
])

dnl GCONFIG_FN_CXX_INITIALIZER_LIST
dnl -------------------------------
dnl Tests for c++ initializer_list.
dnl
AC_DEFUN([GCONFIG_FN_CXX_INITIALIZER_LIST],
[AC_CACHE_CHECK([for c++ initializer_list],[gconfig_cv_cxx_initializer_list],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <initializer_list>]
			[struct X { X( std::initializer_list<int> ) {} } ;]
		],
		[
		])],
		gconfig_cv_cxx_initializer_list=yes ,
		gconfig_cv_cxx_initializer_list=no )
])
	if test "$gconfig_cv_cxx_initializer_list" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_INITIALIZER_LIST,1,[Define true if compiler supports c++ initializer_list])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_INITIALIZER_LIST,0,[Define true if compiler supports c++ initializer_list])
	fi
])

dnl GCONFIG_FN_CXX_MOVE
dnl -------------------
dnl Tests for c++ std::move.
dnl
AC_DEFUN([GCONFIG_FN_CXX_MOVE],
[AC_CACHE_CHECK([for c++ std::move],[gconfig_cv_cxx_move],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <utility>]
			[struct X {} x ;]
			[void fn( X&& ) ;]
		],
		[
			[fn( std::move(x) ) ;]
		])],
		gconfig_cv_cxx_move=yes ,
		gconfig_cv_cxx_move=no )
])
	if test "$gconfig_cv_cxx_move" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_MOVE,1,[Define true if compiler has std::move()])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_MOVE,0,[Define true if compiler has std::move()])
	fi
])

dnl GCONFIG_FN_CXX_NOEXCEPT
dnl -----------------------
dnl Tests for c++ noexcept support.
dnl
AC_DEFUN([GCONFIG_FN_CXX_NOEXCEPT],
[AC_CACHE_CHECK([for c++ noexcept],[gconfig_cv_cxx_noexcept],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[void fn() noexcept ;]
		],
		[
		])],
		gconfig_cv_cxx_noexcept=yes ,
		gconfig_cv_cxx_noexcept=no )
])
	if test "$gconfig_cv_cxx_noexcept" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_NOEXCEPT,1,[Define true if compiler supports c++ noexcept])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_NOEXCEPT,0,[Define true if compiler supports c++ noexcept])
	fi
])

dnl GCONFIG_FN_CXX_NULLPTR
dnl ----------------------
dnl Tests for c++ nullptr keyword.
dnl
AC_DEFUN([GCONFIG_FN_CXX_NULLPTR],
[AC_CACHE_CHECK([for c++ nullptr],[gconfig_cv_cxx_nullptr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[void * p = nullptr ;]
		] ,
		[
		])],
		gconfig_cv_cxx_nullptr=yes ,
		gconfig_cv_cxx_nullptr=no )
])
	if test "$gconfig_cv_cxx_nullptr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_NULLPTR,1,[Define true if compiler supports c++ nullptr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_NULLPTR,0,[Define true if compiler supports c++ nullptr])
	fi
])

dnl GCONFIG_FN_CXX_OVERRIDE
dnl -----------------------
dnl Tests for c++ override support.
dnl
AC_DEFUN([GCONFIG_FN_CXX_OVERRIDE],
[AC_CACHE_CHECK([for c++ override],[gconfig_cv_cxx_override],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#if defined(__GNUC__) && __cplusplus < 200000L]
			[#error gcc is too noisy when using override/final without std=c++11]
			[#endif]
			[struct base { virtual void fn() {} } ;]
			[struct derived : public base { virtual void fn() override {} } ;]
			[derived d ;]
		],
		[
		])],
		gconfig_cv_cxx_override=yes ,
		gconfig_cv_cxx_override=no )
])
	if test "$gconfig_cv_cxx_override" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_OVERRIDE,1,[Define true if compiler supports c++ override])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_OVERRIDE,0,[Define true if compiler supports c++ override])
	fi
])

dnl GCONFIG_FN_CXX_SHARED_PTR
dnl -------------------------
dnl Tests for c++ std::shared_ptr.
dnl
AC_DEFUN([GCONFIG_FN_CXX_SHARED_PTR],
[AC_CACHE_CHECK([for c++ std::shared_ptr and friends],[gconfig_cv_cxx_std_shared_ptr],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <memory>]
			[typedef std::shared_ptr<int> ptr ;]
		],
		[
		])],
		gconfig_cv_cxx_std_shared_ptr=yes ,
		gconfig_cv_cxx_std_shared_ptr=no )
])
	if test "$gconfig_cv_cxx_std_shared_ptr" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_SHARED_PTR,1,[Define true if compiler has std::shared_ptr])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_SHARED_PTR,0,[Define true if compiler has std::shared_ptr])
	fi
])

dnl GCONFIG_FN_CXX_STD_THREAD
dnl -----------------------------
dnl Tests for a viable c++ std::thread class under the current compile and link options.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD_IMP],
[
	AC_MSG_CHECKING([for c++ std::thread])
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

	if test "$gconfig_cxx_std_thread" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,1,[Define true if compiler has std::thread])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_THREAD,0,[Define true if compiler has std::thread])
		gconfig_warnings="$gconfig_warnings $1"
	fi
])

AC_DEFUN([GCONFIG_FN_CXX_STD_THREAD], [GCONFIG_FN_CXX_STD_THREAD_IMP([std::thread_asynchronous_script_execution])])
dnl GCONFIG_FN_CXX_STD_WSTRING
dnl --------------------------
dnl Tests for std::wstring typedef.
dnl
AC_DEFUN([GCONFIG_FN_CXX_STD_WSTRING],
[AC_CACHE_CHECK([for c++ std::wstring],[gconfig_cv_cxx_std_wstring],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <string>]
			[std::wstring ws;]
		],
		[
		])],
		gconfig_cv_cxx_std_wstring=yes ,
		gconfig_cv_cxx_std_wstring=no )
])
	if test "$gconfig_cv_cxx_std_wstring" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_WSTRING,1,[Define true if compiler has std::wstring])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_STD_WSTRING,0,[Define true if compiler has std::wstring])
	fi
])

dnl GCONFIG_FN_CXX_TYPE_TRAITS
dnl --------------------------
dnl Tests for c++ <type_traits> std::make_unsigned.
dnl
AC_DEFUN([GCONFIG_FN_CXX_TYPE_TRAITS],
[AC_CACHE_CHECK([for c++ type_traits],[gconfig_cv_cxx_type_traits_make_unsigned],
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[
			[#include <type_traits>]
			[std::make_unsigned<int>::type i = 0U ;]
		],
		[
		])],
		gconfig_cv_cxx_type_traits_make_unsigned=yes ,
		gconfig_cv_cxx_type_traits_make_unsigned=no )
])
	if test "$gconfig_cv_cxx_type_traits_make_unsigned" = "yes" ; then
		AC_DEFINE(GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED,1,[Define true if compiler has <type_traits> make_unsigned])
	else
		AC_DEFINE(GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED,0,[Define true if compiler has <type_traits> make_unsigned])
	fi
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
dnl Defines _DEBUG if "--enable-debug". Defaults to "no" but allows
dnl "--enable-debug=full" as per kdevelop. Typically used after
dnl AC_ARG_ENABLE(debug).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_DEBUG],
[
	if test "$enable_debug" = "no" -o -z "$enable_debug"
	then
		:
	else
		AC_DEFINE(_DEBUG,1,[Define to enable debug messages at compile-time])
	fi
])

dnl GCONFIG_FN_ENABLE_GUI
dnl ---------------------
dnl Allows for "if GUI" conditionals in makefiles, based on "--enable-gui" or QT_MOC.
dnl Typically used after GCONFIG_FN_QT and AC_ARG_ENABLE(gui).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_GUI],
[
	if test "$enable_gui" = "no"
	then
		gconfig_qt=""
		QT_MOC=""
		QT_LIBS=""
		QT_CFLAGS=""
	fi

	if test "$enable_gui" = "yes"
	then
		if test "$QT_MOC" = "" -o "$QT_LIBS" = "" -o "$QT_CFLAGS" = ""
		then
			AC_MSG_WARN([ignoring --enable-gui: set QT_MOC, QT_LIBS and QT_CFLAGS to override])
			gconfig_qt=""
			QT_MOC=""
			QT_LIBS=""
			QT_CFLAGS=""
		fi
	fi

	if test "$enable_gui" != "no" -a "$QT_MOC" = ""
	then
		gconfig_warnings="$gconfig_warnings qt_graphical_user_interface"
	fi

	if test "$QT_MOC" != ""
	then
		AC_MSG_NOTICE([QT version: $gconfig_qt])
		AC_MSG_NOTICE([QT moc command: $QT_MOC])
	fi

	AC_SUBST([GCONFIG_QT_LIBS],[$QT_LIBS])
	AC_SUBST([GCONFIG_QT_CFLAGS],[$QT_CFLAGS])
	AC_SUBST([GCONFIG_QT_MOC],[$QT_MOC])

	AM_CONDITIONAL([GCONFIG_GUI],[test "$QT_MOC" != ""])
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

dnl GCONFIG_FN_ENABLE_IPV6
dnl ----------------------
dnl Enables ipv6 if "--enable-ipv6" is used and ipv6 is available.
dnl Typically used after GCONFIG_FN_IPV6 and AC_ARG_ENABLE(ipv6).
dnl
AC_DEFUN([GCONFIG_FN_ENABLE_IPV6],
[
	if test "$enable_ipv6" = "no"
	then
		gconfig_use_ipv6="no"
	else
		if test "$gconfig_cv_ipv6" = "no"
		then
			if test "$enable_ipv6" = "yes"
			then
				AC_MSG_WARN([ignoring --enable-ipv6])
			fi
			gconfig_use_ipv6="no"
		else
			gconfig_use_ipv6="yes"
		fi
	fi

	if test "$enable_ipv6" != "no" -a "$gconfig_use_ipv6" = "no"
	then
		gconfig_warnings="$gconfig_warnings ipv6_ipv6_networking"
	fi

	if test "$gconfig_use_ipv6" = "yes" ; then
		AC_DEFINE(GCONFIG_ENABLE_IPV6,1,[Define true to use IPv6])
	else
		AC_DEFINE(GCONFIG_ENABLE_IPV6,0,[Define true to use IPv6])
	fi
	AM_CONDITIONAL([GCONFIG_IPV6],test "$gconfig_use_ipv6" = "yes")
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

dnl GCONFIG_FN_ENABLE_STD_THREAD
dnl ----------------------------
dnl Defines GCONFIG_ENABLE_STD_THREAD based on the GCONFIG_FN_CXX_STD_THREAD
dnl result, unless "--disable-std-thread" has disabled it. Using
dnl "--disable-std-thread" is useful for current versions of mingw32-w64.
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

dnl GCONFIG_FN_FSOPEN
dnl -----------------
dnl Defines GCONFIG_HAVE_FSOPEN if _fsopen() and the extra
dnl sharing-mode parameter for std::stream::open() etc are
dnl available.
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
			[fp = _sfopen("foo","w",_SH_DENYNO) ;]
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
dnl Sets GCONFIG_WINDMC=... in makefiles as the windows message compiler,
dnl overridable from the configure command-line.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDMC],[
	if test "$GCONFIG_WINDMC" = ""
	then
		GCONFIG_WINDMC="`echo \"$CC\" | grep mingw32 | sed 's/gcc$/windmc/'`"
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
dnl Sets GCONFIG_WINDRES=windres in makefiles as the windows resource compiler,
dnl overridable from the configure command-line.
dnl
AC_DEFUN([GCONFIG_FN_PROG_WINDRES],[
	if test "$GCONFIG_WINDRES" = ""
	then
		GCONFIG_WINDRES="`echo \"$CC\" | grep mingw32 | sed 's/gcc$/windres/'`"
		if test "$GCONFIG_WINDRES" = ""
		then
			GCONFIG_WINDRES="windres"
		fi
	fi
	AC_MSG_CHECKING([for resource compiler])
	AC_MSG_RESULT([$GCONFIG_WINDRES])
	AC_SUBST([GCONFIG_WINDRES])
])

dnl GCONFIG_FN_QT
dnl -------------
dnl Tests for Qt. Sets QT_MOC, QT_LIBS and QT_CFLAGS according to what pkg-config says.
dnl A fallback copy of "pkg.m4" should be included in the distribution.
dnl
AC_DEFUN([GCONFIG_FN_QT],
[
	PKG_CHECK_MODULES([QT],[Qt5Widgets > 5],
		[gconfig_qt=qt5],
		[
			AC_MSG_NOTICE([no QT 5: trying QT 4])
			PKG_CHECK_MODULES([QT],[QtGui > 4],[gconfig_qt=qt4],[gconfig_qt=""])
		]
	)

	AC_ARG_VAR([QT_MOC],[moc command for QT])

	# find moc from pkg-config
	if test "$QT_MOC" = "" -a "$PKG_CONFIG" != ""
	then
		if test "$gconfig_qt" = "qt5"
		then
			QT_MOC="`$PKG_CONFIG --variable=exec_prefix Qt5Gui`/bin/moc"
			QT_CHOOSER="`$PKG_CONFIG --variable=exec_prefix Qt5Gui`/bin/qtchooser"
			if test -x "$QT_MOC" ; then : ; else QT_MOC="" ; fi
			if test -x "$QT_CHOOSER" ; then : ; else QT_CHOOSER="" ; fi
			if test "$QT_MOC" != "" -a "$QT_CHOOSER" != ""
			then
				QT_MOC="$QT_CHOOSER -run-tool=moc -qt=qt5"
			fi
			if echo "$QT_CFLAGS" | grep -q fPI ; then : ; else
				QT_CFLAGS="$QT_CFLAGS -fPIC"
			fi
		:
		elif test "$gconfig_qt" = "qt4"
		then
			QT_MOC="`$PKG_CONFIG --variable=exec_prefix QtGui`/bin/moc"
			QT_CHOOSER="`$PKG_CONFIG --variable=exec_prefix QtGui`/bin/qtchooser"
			if test -x "$QT_MOC" ; then : ; else QT_MOC="" ; fi
			if test -x "$QT_CHOOSER" ; then : ; else QT_CHOOSER="" ; fi
			if test "$QT_MOC" != "" -a "$QT_CHOOSER" != ""
			then
				QT_MOC="$QT_CHOOSER -run-tool=moc -qt=qt4"
			fi
		fi
	fi

	# if no pkg-config find moc on the path
	if test "$QT_MOC" = ""
	then
		AC_PATH_PROG([QT_MOC],[moc])
	fi

	# special help for mac (frameworks, no pkg-config)
	if test "`uname`" = "Darwin"
	then
		if test "$QT_MOC" != "" -a "$QT_CFLAGS" = "" -a "$QT_LIBS" = ""
		then
			QT_DIR="`dirname $QT_MOC`/.."
			if $QT_MOC -v | grep -q "Qt 4"
			then
				gconfig_qt="qt4"
				QT_CFLAGS="-F $QT_DIR/lib"
				QT_LIBS="-F $QT_DIR/lib -framework QtGui -framework QtCore"
			fi
			if $QT_MOC -v | grep -q "moc 5"
			then
				gconfig_qt="qt5"
				QT_CFLAGS="-F $QT_DIR/lib"
				QT_LIBS="-F $QT_DIR/lib -framework QtWidgets -framework QtGui -framework QtCore"
			fi
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
	if test "$e_libexecdir" = ""
	then
		e_libexecdir="$libexecdir/$PACKAGE"
	fi
	if test "$e_examplesdir" = ""
	then
		e_examplesdir="$libexecdir/$PACKAGE/examples"
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
		e_initdir="$libexecdir/$PACKAGE/init"
	fi
	if test "$e_bsdinitdir" = ""
	then
		if test "$gconfig_bsd" = "yes"
		then
			e_bsdinitdir="$sysconfdir/rc.d"
		else
			e_bsdinitdir="$libexecdir/$PACKAGE/init/bsd"
		fi
	fi
	if test "$e_icondir" = ""
	then
		e_icondir="$datadir/$PACKAGE"
	fi
	if test "$e_rundir" = ""
	then
		# (linux fhs's "/run" not widely used)
		e_rundir="/var/run/$PACKAGE"
	fi

	AC_SUBST([e_docdir])
	AC_SUBST([e_initdir])
	AC_SUBST([e_bsdinitdir])
	AC_SUBST([e_icondir])
	AC_SUBST([e_spooldir])
	AC_SUBST([e_examplesdir])
	AC_SUBST([e_libexecdir])
	AC_SUBST([e_pamdir])
	AC_SUBST([e_sysconfdir])
	AC_SUBST([e_rundir])
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
		GCONFIG_TLS_LIBS="$gconfig_ssl_mbedtls_libs $gconfig_ssl_openssl_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "yes" -a "$gconfig_ssl_use_mbedtls" = "no"
	then
		gconfig_ssl_notice="openssl"
		gconfig_ssl_use_none=no
		gconfig_ssl_use_both=no
		GCONFIG_TLS_LIBS="$gconfig_ssl_openssl_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "no" -a "$gconfig_ssl_use_mbedtls" = "yes"
	then
		gconfig_ssl_notice="mbedtls"
		gconfig_ssl_use_none=no
		gconfig_ssl_use_both=no
		GCONFIG_TLS_LIBS="$gconfig_ssl_mbedtls_libs"
	fi
	if test "$gconfig_ssl_use_openssl" = "no" -a "$gconfig_ssl_use_mbedtls" = "no"
	then
		gconfig_ssl_notice="none"
		gconfig_ssl_use_none=yes
		gconfig_ssl_use_both=no
		GCONFIG_TLS_LIBS=""
	fi

	if test "$gconfig_ssl_use_none" = "yes" -a "$with_openssl" != "no"
	then
		gconfig_warnings="$gconfig_warnings openssl/mbedtls_transport_layer_security"
	fi

	AC_SUBST([GCONFIG_TLS_LIBS])
	AM_CONDITIONAL([GCONFIG_TLS_USE_BOTH],test "$gconfig_ssl_use_both" = "yes")
	AM_CONDITIONAL([GCONFIG_TLS_USE_OPENSSL],test "$gconfig_ssl_use_openssl" = "yes")
	AM_CONDITIONAL([GCONFIG_TLS_USE_MBEDTLS],test "$gconfig_ssl_use_mbedtls" = "yes")
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

