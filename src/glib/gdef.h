/*
   Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * gdef.h
 */

/* This header is always the first header included in source
 * files. It takes care of a lot of portability concerns
 * and it works best when autoconf is used to set GCONFIG
 * preprocessor switches. Either G_UNIX or G_WINDOWS are expected
 * to be defined on the compiler command-line.
 */

#ifndef G_DEF__H
#define G_DEF__H

	/* Pull in GCONFIG definitions. Uses an odd file name to avoid
	 * picking up a header from some third-party library. If this
	 * file does not exist then there might be a suitable template
	 * file to copy, or just create an empty file and maybe tweak
	 * the values that are defaulted below.
	 */
	#include <gconfig_defs.h>

	/* Check target operating-system switches
	 */
	#if !defined(G_WINDOWS) && !defined(G_UNIX)
		#if defined(_WIN32)
			#define G_WINDOWS 1
		#else
			#define G_UNIX 1
		#endif
	#endif
	#if defined(G_WINDOWS) && defined(G_UNIX)
		#error invalid compilation switches - define G_WINDOWS or G_UNIX but not both
	#endif

	/* Define supplementary o/s switches
	 */
	#if defined(__MINGW32__) && !defined(G_MINGW)
		/* mingw-w64 */
		#define G_MINGW 1
	#endif
	#if defined(G_MINGW) && !defined(G_WINDOWS)
		#error invalid compilation switches - G_MINGW requires G_WINDOWS
	#endif
	#if defined(__NetBSD__)
		#define G_UNIX_BSD 1
		#define G_UNIX_NETBSD 1
	#endif
	#if defined(__OpenBSD__)
		#define G_UNIX_BSD 1
		#define G_UNIX_OPENBSD 1
	#endif
	#if defined(__FreeBSD__)
		#define G_UNIX_BSD 1
		#define G_UNIX_FREEBSD 1
	#endif
	#if defined(__APPLE__)
		#define G_UNIX_BSD 1
		#define G_UNIX_MAC 1
	#endif
	#if defined(linux) || defined(__linux__)
		#define G_UNIX_LINUX 1
	#endif

	/* Define the compiler c++ dialect
	 */
	#if !defined(G_COMPILER_CXX_11)
		#if defined(_MSC_VER)
			/* MSC_VER 1500=>VS9(2008) 1700=>VS11(2012), 1800=>VS12(2013) 1900=>VS14(2015) */
			#define G_COMPILER_CXX_11 (_MSC_VER >= 1900)
		#else
			#if __cplusplus > 199711L
				#define G_COMPILER_CXX_11 1
			#endif
		#endif
	#endif

	/* Apply GCONFIG defaults in case of no autoconf
	 */
	#if !defined(GCONFIG_HAVE_CXX_NULLPTR)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_NULLPTR 1
		#else
			#define GCONFIG_HAVE_CXX_NULLPTR 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_CONSTEXPR)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_CONSTEXPR 1
		#else
			#define GCONFIG_HAVE_CXX_CONSTEXPR 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_ENUM_CLASS)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_ENUM_CLASS 1
		#else
			#define GCONFIG_HAVE_CXX_ENUM_CLASS 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_NOEXCEPT)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_NOEXCEPT 1
		#else
			#define GCONFIG_HAVE_CXX_NOEXCEPT 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_OVERRIDE)
		#if defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800
			#define GCONFIG_HAVE_CXX_OVERRIDE 1
		#else
			#define GCONFIG_HAVE_CXX_OVERRIDE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_FINAL)
		#if defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800
			#define GCONFIG_HAVE_CXX_FINAL 1
		#else
			#define GCONFIG_HAVE_CXX_FINAL 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_ALIGNMENT)
		/* no std::align in gcc 4.8 with -std=c++11 */
		#if defined(G_COMPILER_CXX_11) && defined(_MSC_VER)
			#define GCONFIG_HAVE_CXX_ALIGNMENT 0
		#else
			#define GCONFIG_HAVE_CXX_ALIGNMENT 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_SHARED_PTR)
		#if defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800
			#define GCONFIG_HAVE_CXX_SHARED_PTR 1
		#else
			#define GCONFIG_HAVE_CXX_SHARED_PTR 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_STD_THREAD)
		#if !defined(G_MINGW) && ( defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800 )
			#define GCONFIG_HAVE_CXX_STD_THREAD 1
		#else
			#define GCONFIG_HAVE_CXX_STD_THREAD 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED)
		#if defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800
			#define GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED 1
		#else
			#define GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_EMPLACE)
		#if defined(G_COMPILER_CXX_11) || _MSC_VER >= 1800
			#define GCONFIG_HAVE_CXX_EMPLACE 1
		#else
			#define GCONFIG_HAVE_CXX_EMPLACE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_MOVE)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_MOVE 1
		#else
			#define GCONFIG_HAVE_CXX_MOVE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_DELETED)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_DELETED 1
		#else
			#define GCONFIG_HAVE_CXX_DELETED 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_DEFAULTED)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_DEFAULTED 1
		#else
			#define GCONFIG_HAVE_CXX_DEFAULTED 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_INITIALIZER_LIST)
		#ifdef G_COMPILER_CXX_11
			#define GCONFIG_HAVE_CXX_INITIALIZER_LIST 1
		#else
			#define GCONFIG_HAVE_CXX_INITIALIZER_LIST 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SYS_UTSNAME_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SYS_UTSNAME_H 1
		#else
			#define GCONFIG_HAVE_SYS_UTSNAME_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SYS_SELECT_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SYS_SELECT_H 1
		#else
			#define GCONFIG_HAVE_SYS_SELECT_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SYS_SOCKET_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SYS_SOCKET_H 1
		#else
			#define GCONFIG_HAVE_SYS_SOCKET_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SYS_MMAN_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SYS_MMAN_H 1
		#else
			#define GCONFIG_HAVE_SYS_MMAN_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_NETINET_IN_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_NETINET_IN_H 1
		#else
			#define GCONFIG_HAVE_NETINET_IN_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_NETDB_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_NETDB_H 1
		#else
			#define GCONFIG_HAVE_NETDB_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_ARPA_INET_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_ARPA_INET_H 1
		#else
			#define GCONFIG_HAVE_ARPA_INET_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_LONG_LONG)
		#define GCONFIG_HAVE_LONG_LONG 1
	#endif
	#if !defined(GCONFIG_HAVE_LONG_LONG_LONG)
		/* "long long" is longer than "long" */
		#define GCONFIG_HAVE_LONG_LONG_LONG 0
	#endif
	#if !defined(GCONFIG_HAVE_STDINT_H)
		#define GCONFIG_HAVE_STDINT_H 1
	#endif
	#if !defined(GCONFIG_HAVE_INTTYPES_H)
		#define GCONFIG_HAVE_INTTYPES_H 1
	#endif
	#if !defined(GCONFIG_HAVE_INT64)
		#define GCONFIG_HAVE_INT64 1
	#endif
	#if !defined(GCONFIG_HAVE_INT32)
		#define GCONFIG_HAVE_INT32 1
	#endif
	#if !defined(GCONFIG_HAVE_INT16)
		#define GCONFIG_HAVE_INT16 1
	#endif
	#if !defined(GCONFIG_HAVE_DIRENT_H)
		#define GCONFIG_HAVE_DIRENT_H 1
	#endif
	#if !defined(GCONFIG_HAVE_READLINK)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_READLINK 1
		#else
			#define GCONFIG_HAVE_READLINK 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_ERRNO_T)
		#ifdef G_WINDOWS
			#define GCONFIG_HAVE_ERRNO_T 1
		#else
			#define GCONFIG_HAVE_ERRNO_T 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_GETENV_S)
		#if ( defined(G_WINDOWS) && !defined(G_MINGW) ) || defined(getenv_s)
			#define GCONFIG_HAVE_GETENV_S 1
		#else
			#define GCONFIG_HAVE_GETENV_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_GETPWNAM)
		#define GCONFIG_HAVE_GETPWNAM 1
	#endif
	#if !defined(GCONFIG_HAVE_GETPWNAM_R)
		#define GCONFIG_HAVE_GETPWNAM_R 1
	#endif
	#if !defined(GCONFIG_HAVE_GMTIME_R)
		#define GCONFIG_HAVE_GMTIME_R 1
	#endif
	#if !defined(GCONFIG_HAVE_GMTIME_S)
		#if defined(G_WINDOWS) || defined(gmtime_s)
			#define GCONFIG_HAVE_GMTIME_S 1
		#else
			#define GCONFIG_HAVE_GMTIME_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_IPV6)
		#define GCONFIG_HAVE_IPV6 1
	#endif
	#if !defined(GCONFIG_HAVE_LOCALTIME_R)
		#define GCONFIG_HAVE_LOCALTIME_R 1
	#endif
	#if !defined(GCONFIG_HAVE_LOCALTIME_S)
		#if defined(G_WINDOWS) || defined(localtime_s)
			#define GCONFIG_HAVE_LOCALTIME_S 1
		#else
			#define GCONFIG_HAVE_LOCALTIME_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MEMORY_H)
		#define GCONFIG_HAVE_MEMORY_H 1
	#endif
	#if !defined(GCONFIG_HAVE_SETPGRP_BSD)
		#if defined(G_UNIX_LINUX) || defined(G_UNIX_MAC)
			#define GCONFIG_HAVE_SETPGRP_BSD 0
		#else
			#define GCONFIG_HAVE_SETPGRP_BSD 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SETGROUPS)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SETGROUPS 1
		#else
			#define GCONFIG_HAVE_SETGROUPS 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_STATBUF_NSEC)
		#define GCONFIG_HAVE_STATBUF_NSEC 0
	#endif
	#if !defined(GCONFIG_HAVE_SIN6_LEN)
		#define GCONFIG_HAVE_SIN6_LEN 0
	#endif
	#if !defined(GCONFIG_HAVE_SOCKLEN_T)
		#define GCONFIG_HAVE_SOCKLEN_T 1
	#endif
	#if !defined(GCONFIG_HAVE_STDLIB_H)
		#define GCONFIG_HAVE_STDLIB_H 1
	#endif
	#if !defined(GCONFIG_HAVE_STRNCPY_S)
		#if defined(G_WINDOWS) || defined(strncpy_s)
			#define GCONFIG_HAVE_STRNCPY_S 1
		#else
			#define GCONFIG_HAVE_STRNCPY_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_STRINGS_H)
		#define GCONFIG_HAVE_STRINGS_H 1
	#endif
	#if !defined(GCONFIG_HAVE_STRING_H)
		#define GCONFIG_HAVE_STRING_H 1
	#endif
	#if !defined(GCONFIG_HAVE_NDIR_H)
		#define GCONFIG_HAVE_NDIR_H 0
	#endif
	#if !defined(GCONFIG_HAVE_SYS_DIR_H)
		#define GCONFIG_HAVE_SYS_DIR_H 0
	#endif
	#if !defined(GCONFIG_HAVE_SYS_NDIR_H)
		#define GCONFIG_HAVE_SYS_NDIR_H 0
	#endif
	#if !defined(GCONFIG_HAVE_SYS_STAT_H)
		#define GCONFIG_HAVE_SYS_STAT_H 1
	#endif
	#if !defined(GCONFIG_HAVE_SYS_WAIT_H)
		#define GCONFIG_HAVE_SYS_WAIT_H 1
	#endif
	#if !defined(GCONFIG_HAVE_SYS_TIME_H)
		#define GCONFIG_HAVE_SYS_TIME_H 1
	#endif
	#if !defined(GCONFIG_HAVE_SYS_TYPES_H)
		#define GCONFIG_HAVE_SYS_TYPES_H 1
	#endif
	#if !defined(GCONFIG_HAVE_UNISTD_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_UNISTD_H 1
		#else
			#define GCONFIG_HAVE_UNISTD_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_ERRNO_H)
		#define GCONFIG_HAVE_ERRNO_H 1
	#endif
	#if !defined(GCONFIG_HAVE_INET_NTOP)
		#define GCONFIG_HAVE_INET_NTOP 1
	#endif
	#if !defined(GCONFIG_HAVE_IF_NAMETOINDEX)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_IF_NAMETOINDEX 1
		#else
			#define GCONFIG_HAVE_IF_NAMETOINDEX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_INET_PTON)
		#define GCONFIG_HAVE_INET_PTON 1
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_ABORT)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_ABORT 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_ABORT 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_EXIT)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_EXIT 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_EXIT 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_SYSTEM)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_SYSTEM 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_SYSTEM 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_MEMSET)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_MEMSET 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_MEMSET 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_MEMCPY)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_MEMCPY 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_MEMCPY 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_STRERROR)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_STRERROR 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_STRERROR 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_FOPEN)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_FOPEN 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_FOPEN 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_FCLOSE)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_FCLOSE 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_FCLOSE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MISSING_STD_FPUTS)
		#if defined(_MSG_VER) && (_MSC_VER <= 1500)
			#define GCONFIG_HAVE_MISSING_STD_FPUTS 1
		#else
			#define GCONFIG_HAVE_MISSING_STD_FPUTS 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_UID_T)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_UID_T 1
		#else
			#define GCONFIG_HAVE_UID_T 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_GID_T)
		#define GCONFIG_HAVE_GID_T GCONFIG_HAVE_UID_T
	#endif
	#if !defined(GCONFIG_HAVE_UINTPTR_T)
		#define GCONFIG_HAVE_UINTPTR_T 0
	#endif
	#if !defined(GCONFIG_HAVE_SSIZE_T)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_SSIZE_T 1
		#else
			#define GCONFIG_HAVE_SSIZE_T 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PID_T)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_PID_T 1
		#else
			#define GCONFIG_HAVE_PID_T 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_GET_WINDOW_LONG_PTR)
		#ifdef G_WINDOWS
			#define GCONFIG_HAVE_GET_WINDOW_LONG_PTR 1
		#else
			#define GCONFIG_HAVE_GET_WINDOW_LONG_PTR 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MREMAP)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_MREMAP 1
		#else
			#define GCONFIG_HAVE_MREMAP 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_TIMERFD)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_TIMERFD 1
		#else
			#define GCONFIG_HAVE_TIMERFD 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PAM)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_PAM 1
		#else
			#define GCONFIG_HAVE_PAM 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PAM_IN_SECURITY)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_PAM_IN_SECURITY 1
		#else
			#define GCONFIG_HAVE_PAM_IN_SECURITY 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PAM_IN_INCLUDE)
		#define GCONFIG_HAVE_PAM_IN_INCLUDE 0
	#endif
	#if !defined(GCONFIG_HAVE_PAM_IN_PAM)
		#define GCONFIG_HAVE_PAM_IN_PAM 0
	#endif
	#if !defined(GCONFIG_HAVE_GET_NATIVE_SYSTEM_INFO)
		#ifdef G_WINDOWS
			#define GCONFIG_HAVE_GET_NATIVE_SYSTEM_INFO 1
		#else
			#define GCONFIG_HAVE_GET_NATIVE_SYSTEM_INFO 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_LIBJPEG)
		#define GCONFIG_HAVE_LIBJPEG 0
	#endif
	#if !defined(GCONFIG_HAVE_LIBPNG)
		#define GCONFIG_HAVE_LIBPNG 0
	#endif
	#if !defined(GCONFIG_HAVE_LIBAV)
		#define GCONFIG_HAVE_LIBAV 0
	#endif
	#if !defined(GCONFIG_HAVE_CURSES)
		#define GCONFIG_HAVE_CURSES 0
	#endif
	#if !defined(GCONFIG_HAVE_LIBEXIV)
		#define GCONFIG_HAVE_LIBEXIV 0
	#endif
	#if !defined(GCONFIG_HAVE_V4L)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_V4L 1
		#else
			#define GCONFIG_HAVE_V4L 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_LIBV4L)
		#define GCONFIG_HAVE_LIBV4L 0
	#endif
	#if !defined(GCONFIG_ENABLE_IPV6)
		#define GCONFIG_ENABLE_IPV6 GCONFIG_HAVE_IPV6
	#endif
	#if !defined(GCONFIG_ENABLE_STD_THREAD)
		#define GCONFIG_ENABLE_STD_THREAD GCONFIG_HAVE_CXX_STD_THREAD
	#endif
	#if !defined(GCONFIG_HAVE_SEM_INIT)
		#if defined(G_UNIX_LINUX) || defined(G_UNIX_FREEBSD)
			#define GCONFIG_HAVE_SEM_INIT 1
		#else
			#define GCONFIG_HAVE_SEM_INIT 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_X11)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_X11 1
		#else
			#define GCONFIG_HAVE_X11 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MBEDTLS)
		#define GCONFIG_HAVE_MBEDTLS 1
	#endif
	#if !defined(GCONFIG_HAVE_MBEDTLS_NET_H)
		#define GCONFIG_HAVE_MBEDTLS_NET_H 0
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_OPENSSL 1
		#else
			#define GCONFIG_HAVE_OPENSSL 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_MIN_MAX)
		#define GCONFIG_HAVE_OPENSSL_MIN_MAX 0
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_SSLv23_METHOD)
		#define GCONFIG_HAVE_OPENSSL_SSLv23_METHOD 0
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_SSLv3_METHOD)
		#define GCONFIG_HAVE_OPENSSL_SSLv3_METHOD 0
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_TLSv1_1_METHOD)
		#define GCONFIG_HAVE_OPENSSL_TLSv1_1_METHOD 1
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_TLSv1_2_METHOD)
		#define GCONFIG_HAVE_OPENSSL_TLSv1_2_METHOD 1
	#endif
	#if !defined(GCONFIG_HAVE_BOOST)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_BOOST 1
		#else
			#define GCONFIG_HAVE_BOOST 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_ALSA)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_ALSA 1
		#else
			#define GCONFIG_HAVE_ALSA 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_FMEMOPEN)
		#if !defined(G_UNIX_OPENBSD)
			#define GCONFIG_HAVE_FMEMOPEN 1
		#else
			#define GCONFIG_HAVE_FMEMOPEN 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_IP_MREQN)
		#if defined(G_UNIX_LINUX) || defined(G_UNIX_FREEBSD)
			#define GCONFIG_HAVE_IP_MREQN 1
		#else
			#define GCONFIG_HAVE_IP_MREQN 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_EXECVPE)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_EXECVPE 1
		#else
			#define GCONFIG_HAVE_EXECVPE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX)
		#if defined(G_WINDOWS) && !defined(G_MINGW)
			#define GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX 1
		#else
			#define GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX)
		#if defined(G_WINDOWS) && !defined(G_MINGW)
			#define GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX 1
		#else
			#define GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_STD_WSTRING)
		#define GCONFIG_HAVE_CXX_STD_WSTRING 1
	#endif
	#if !defined(GCONFIG_HAVE_DLOPEN)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_DLOPEN 1
		#else
			#define GCONFIG_HAVE_DLOPEN 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_FSOPEN)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_FSOPEN 0
		#else
			#define GCONFIG_HAVE_FSOPEN 1
		#endif
	#endif

	/* Include early o/s headers
	 */
	#if defined(G_WINDOWS)
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
		#endif
		#include <winsock2.h>
		#include <windows.h>
		#include <ws2tcpip.h>
	#endif

	/* Include commonly-used c++ headers
	 */
	#ifdef __cplusplus
		#include <cstddef>
		#include <cstdlib>
		#include <ios>
		#include <iosfwd>
		#if GCONFIG_HAVE_CXX_INITIALIZER_LIST
		#include <initializer_list>
		#endif
	#endif

	/* Include main o/s headers
	 */
	#if defined(G_WINDOWS)
		#include <shellapi.h>
		#include <direct.h>
		#ifdef G_MINGW
			#include <sys/stat.h>
			#include <unistd.h>
		#else
			#include <share.h>
		#endif
	#else
		#if GCONFIG_HAVE_SYS_TYPES_H
			#include <sys/types.h>
		#endif
		#if GCONFIG_HAVE_SYS_STAT_H
			#include <sys/stat.h>
		#endif
		#if GCONFIG_HAVE_INTTYPES_H
			#include <inttypes.h>
		#endif
		#if GCONFIG_HAVE_STDINT_H
			#include <stdint.h>
		#endif
		#if GCONFIG_HAVE_UNISTD_H
			#include <unistd.h>
		#endif
		#if GCONFIG_HAVE_ERRNO_H
			#include <errno.h>
		#endif
		#if GCONFIG_HAVE_SYS_WAIT_H
			#include <sys/wait.h>
		#endif
		#if GCONFIG_HAVE_SYS_UTSNAME_H
			#include <sys/utsname.h>
		#endif
		#if GCONFIG_HAVE_SYS_SELECT_H
			#include <sys/select.h>
		#endif
		#if GCONFIG_HAVE_SYS_SOCKET_H
			#include <sys/socket.h>
		#endif
		#ifndef MSG_NOSIGNAL
			#define MSG_NOSIGNAL 0
		#endif
		#if GCONFIG_HAVE_SYS_MMAN_H
			#include <sys/mman.h>
			#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
				#define MAP_ANONYMOUS MAP_ANON
			#endif
			#ifndef MREMAP_MAYMOVE
				#define MREMAP_MAYMOVE 0
			#endif
		#endif
		#if GCONFIG_HAVE_NETINET_IN_H
			#include <netinet/in.h>
		#endif
		#if GCONFIG_HAVE_NETDB_H
			#include <netdb.h>
		#endif
		#if GCONFIG_HAVE_ARPA_INET_H
			#include <arpa/inet.h>
		#endif
		#if GCONFIG_HAVE_IF_NAMETOINDEX
			#include <net/if.h>
		#endif
	#endif
	#ifndef __cplusplus
		#include <wchar.h>
	#endif

	/* Undefine some out-dated macros
	 */
	#ifdef __cplusplus
		#ifdef max
			#undef max
		#endif
		#ifdef min
			#undef min
		#endif
	#endif

	/* Define a few Windows-style types under unix
	 */
	#if ! defined(G_WINDOWS)
		typedef unsigned char BOOL ;
		typedef unsigned int HDC ;
		typedef unsigned int HWND ;
		typedef unsigned int HINSTANCE ;
		typedef unsigned int HANDLE ;
		typedef wchar_t TCHAR ;
		typedef int SOCKET ;
	#endif

	/* Pull some std types into the global namespace
	 */
	#ifdef __cplusplus
		using std::size_t ;
	#endif

	/* Define fixed-size types - the underlying types should come
	 * from C99's stdint.h, but they are all optional
	 */
	#if defined(G_WINDOWS)
		#if GCONFIG_HAVE_INT64
			typedef INT64 g_int64_t ;
			typedef UINT64 g_uint64_t ;
		#endif
		#if GCONFIG_HAVE_INT32
			typedef UINT32 g_uint32_t ;
			typedef UINT16 g_uint16_t ;
		#endif
		#if GCONFIG_HAVE_INT16
			typedef INT32 g_int32_t ;
			typedef INT16 g_int16_t ;
		#endif
	#else
		#if GCONFIG_HAVE_INT64
			typedef int64_t g_int64_t ;
			typedef uint64_t g_uint64_t ;
		#endif
		#if GCONFIG_HAVE_INT32
			typedef uint32_t g_uint32_t ;
			typedef int32_t g_int32_t ;
		#endif
		#if GCONFIG_HAVE_INT16
			typedef uint16_t g_uint16_t ;
			typedef int16_t g_int16_t ;
		#endif
	#endif
	#if GCONFIG_HAVE_UINTPTR_T
		typedef uintptr_t g_uintptr_t ; /* uintptr_t in C99 and C++2011 */
	#else
		typedef size_t g_uintptr_t ; /* using size_t assumes a non-segmented architecture - see also windows LONG_PTR */
	#endif
	#if __cplusplus
		#if GCONFIG_HAVE_INT64
			typedef char assert_sizeof_int64_is_8[sizeof(g_int64_t)==8U?1:-1] ;
			typedef char assert_sizeof_uint64_is_8[sizeof(g_uint64_t)==8U?1:-1] ;
		#endif
		#if GCONFIG_HAVE_INT32
			typedef char assert_sizeof_uint32_is_4[sizeof(g_uint32_t)==4U?1:-1] ;
			typedef char assert_sizeof_int32_is_4[sizeof(g_int32_t)==4U?1:-1] ;
		#endif
		#if GCONFIG_HAVE_INT16
			typedef char assert_sizeof_uint16_is_2[sizeof(g_uint16_t)==2U?1:-1] ;
			typedef char assert_sizeof_int16_is_2[sizeof(g_int16_t)==2U?1:-1] ;
		#endif
		#if GCONFIG_HAVE_UINTPTR_T
			typedef char assert_uintptr_t_is_big_enough[sizeof(g_uintptr_t)>=sizeof(void*)?1:-1] ; /* try 'typedef unsigned long g_uintptr_t' if this fails */
		#endif
	#endif

	/* Define missing standard types
	 */
	#if ! GCONFIG_HAVE_UID_T
		typedef int uid_t ;
	#endif
	#if ! GCONFIG_HAVE_GID_T
		typedef int gid_t ;
	#endif
	#if ! GCONFIG_HAVE_SSIZE_T
		#if defined(SSIZE_T)
			typedef SSIZE_T ssize_t ;
		#else
			/* read(2) return type -- 'int' on some unix systems */
			typedef int ssize_t ;
		#endif
	#endif
	#if ! GCONFIG_HAVE_PID_T
		typedef unsigned int pid_t ;
	#endif
	#if ! GCONFIG_HAVE_SOCKLEN_T
		typedef int socklen_t ;
	#endif
	#if ! GCONFIG_HAVE_ERRNO_T
		/* whatever_s() return type */
		typedef int errno_t ;
	#endif
	#if ! GCONFIG_HAVE_CXX_STD_WSTRING
		#if __cplusplus
			namespace std
			{
				typedef basic_string<wchar_t> wstring ;
			}
		#endif
	#endif

	/* Pull stuff into the std namespace as a fix for old compilers
	 */
	#ifdef __cplusplus
		#include <cstdio>
		#include <cstring>
		#include <ctime>
		#include <cstdlib>
		namespace std
		{
			#if GCONFIG_HAVE_MISSING_STD_ABORT
				using ::abort ; // cstdlib
			#endif
			#if GCONFIG_HAVE_MISSING_STD_EXIT
				using ::exit ; // cstdlib
			#endif
			#if GCONFIG_HAVE_MISSING_STD_SYSTEM
				using ::system ; // cstdlib
			#endif
			#if GCONFIG_HAVE_MISSING_STD_MEMSET
				using ::memset ; // cstring
			#endif
			#if GCONFIG_HAVE_MISSING_STD_MEMCPY
				using ::memcpy ; // cstring
			#endif
			#if GCONFIG_HAVE_MISSING_STD_STRERROR
				using ::strerror ; // cstring
			#endif
			#if GCONFIG_HAVE_MISSING_STD_FOPEN
				using ::fopen ; // cstdio
			#endif
			#if GCONFIG_HAVE_MISSING_STD_FCLOSE
				using ::fclose ; // cstdio
			#endif
			#if GCONFIG_HAVE_MISSING_STD_FPUTS
				using ::fputs ; // cstdio
			#endif
			#if GCONFIG_HAVE_MISSING_STD_STRUCT_TM
				using ::tm ; // ctime
			#endif
			#if GCONFIG_HAVE_MISSING_STD_TIME_T
				using ::time_t ; // ctime
			#endif
		}
	#endif

	/* Use smaller buffers and limits if building with the uClibc run-time library.
	 * See glimits.h. This assumes that the uClibc header "features.h" has been
	 * included as a side-effect of including system headers above.
	 */
	#ifdef __UCLIBC__
		#ifndef G_NOT_SMALL
			#define G_SMALL
		#endif
	#endif

	/* Macros to explicitly ignore unused values
	 */
	#define G_IGNORE_PASTE_IMP(a,b) a##b
	#define G_IGNORE_PASTE(a,b) G_IGNORE_PASTE_IMP(a,b)
	#define G_IGNORE_RETURN(type,expr) do { type G_IGNORE_PASTE(ignore_,__LINE__) = expr ; (void) G_IGNORE_PASTE(ignore_,__LINE__) ; } while(0)
	#define G_IGNORE_PARAMETER(type,name) (void)name
	#define G_IGNORE_VARIABLE(type,name) (void)name

	/* C++ language backwards compatibility
	 */
	#ifdef __cplusplus

		#if ! GCONFIG_HAVE_CXX_NULLPTR
			#ifndef nullptr
				#define nullptr NULL
			#endif
		#endif

		#if GCONFIG_HAVE_CXX_CONSTEXPR
			#define g__constexpr constexpr /* for in-class initialisation of static integer-type constants */
			#define g__constexpr_fn constexpr /* for constexpr on functions */
			#define G_CONSTANT( int_type , name , value ) static constexpr int_type name = value
		#else
			#define g__constexpr const /* for in-class initialisation of static integer-type constants */
			#define g__constexpr_fn /* for constexpr on functions */
			#define G_CONSTANT( int_type , name , value ) enum { name = value }
		#endif

		#if GCONFIG_HAVE_CXX_ENUM_CLASS
			#define g__enum(name) enum class name
			#define g__enum_end(name)
		#else
			#define g__enum(name) struct name { enum X
			#define g__enum_end(name) X x;name(int x_):x(static_cast<X>(x_)){};name():x(static_cast<X>(0)){};operator int()const{return static_cast<int>(x);}};
		#endif

		#if GCONFIG_HAVE_CXX_DELETED
			#define g__eq_delete =delete
		#else
			#define g__eq_delete
		#endif

		#if GCONFIG_HAVE_CXX_DEFAULTED
			#define g__eq_default =default
		#else
			#define g__eq_default
		#endif

		#if GCONFIG_HAVE_CXX_NOEXCEPT
			#define g__noexcept noexcept
		#else
			#define g__noexcept throw()
		#endif

		#if ! GCONFIG_HAVE_CXX_OVERRIDE
			#define override
		#endif

		#if GCONFIG_HAVE_CXX_FINAL
			#define g__final final
		#else
			#define g__final
		#endif

		#if !GCONFIG_HAVE_CXX_SHARED_PTR || defined(G_WITH_SHARED_PTR)
			#include <algorithm>
			#include <utility>
			namespace G {
				template <typename T> struct shared_ptr_deleter
				{
					static void fn( void * p ) { delete static_cast<T*>(p) ; }
				} ;
				struct shared_ptr_control
				{
					explicit shared_ptr_control( void * object , void (*deleter)(void*) ) :
						m_object(object) , m_deleter(deleter) , m_n(1U) , m_w(0U) {}
					explicit shared_ptr_control( const void * object , void (*deleter)(void*) ) :
						m_object(const_cast<void*>(object)) , m_deleter(deleter) , m_n(1U) , m_w(0U) {}
					void inc() { m_n++ ; }
					bool dec() { m_n-- ; return m_n == 0 ; }
					void winc() { m_w++ ; }
					bool wdec() { m_w-- ; return unused() ; }
					bool unused() const { return m_n == 0 && m_w == 0 ; }
					unsigned int use_count() const { return m_object ? m_n : 0U ; }
					void * m_object ;
					void (*m_deleter)(void *) ;
					unsigned int m_n ;
					unsigned int m_w ;
				} ;
				template <typename T> class shared_ptr
				{
					public:
						typedef shared_ptr<T> ptr ;
						explicit shared_ptr( T * p = 0 ) : m_control(p?new shared_ptr_control(p,&shared_ptr_deleter<T>::fn):nullptr) , m_p(p) {}
						shared_ptr( T * p , void (*d)(void*) ) : m_control(p?new shared_ptr_control(p,d):nullptr) , m_p(p) {}
						template <typename Y> shared_ptr( const shared_ptr<Y> & obj , T * p ) : m_control(obj.m_control) , m_p(p) { inc(); }
						~shared_ptr() { dec(); }
						void reset( T * p = 0 ) { ptr(p).swap(*this); }
						void reset( T * p , void (*d)(void*) ) { ptr(p,d).swap(*this); }
						T * get() { return m_p; }
						const T * get() const { return m_p; }
						T * operator->() { return m_p; }
						const T * operator->() const { return m_p; }
						T & operator*() { return *m_p; }
						const T & operator*() const { return *m_p; }
						operator bool () const { return m_p != nullptr; }
						template <typename D> shared_ptr( const shared_ptr<D> & o ) : m_control(o.m_control) , m_p(o.m_p) { inc(); }
						shared_ptr( const shared_ptr<T> & o ) : m_control(o.m_control) , m_p(o.m_p) { inc(); }
						ptr & operator=( const shared_ptr<T> & o ) { ptr(o).swap(*this); return *this; }
						unsigned int use_count() const { return m_control ? m_control->use_count() : 0U ; }
						bool unique() const { return use_count() == 1; }
						static shared_ptr<T> from_weak( shared_ptr_control * control , T * p ) { return (control&&control->m_object)?ptr(control,p):ptr(); }
					private:
						shared_ptr( shared_ptr_control * control , T * p ) : m_control(control) , m_p(p) { inc(); }
						void swap( shared_ptr<T> & o ) g__noexcept { std::swap(m_p,o.m_p); std::swap(m_control,o.m_control); }
						void dec()
						{
							if( m_control && m_control->dec() )
							{
								m_control->m_deleter( m_control->m_object ) ;
								m_control->m_object = nullptr ;
							}
							if( m_control && m_control->unused() )
								delete m_control ;
						}
						void inc() { if(m_control) m_control->inc(); }
					public:
						shared_ptr_control * m_control ;
						T * m_p ;
				} ;
				template <typename T> class weak_ptr
				{
					public:
						weak_ptr() : m_control(nullptr) , m_p(nullptr) {}
						weak_ptr( const shared_ptr<T> & sp ) : m_control(sp.m_control) , m_p(sp.m_p) { m_control->winc(); } // implicit
						weak_ptr( const weak_ptr<T> & other ) : m_control(other.m_control) , m_p(other.m_p) { if(m_control) m_control->winc(); }
						~weak_ptr() { if(m_control && m_control->wdec()) delete m_control; }
						weak_ptr<T> & operator=( const weak_ptr<T> & other ) { weak_ptr<T>(other).swap(*this); return *this; }
						void swap( weak_ptr<T> & other ) g__noexcept { std::swap(m_control,other.m_control); std::swap(m_p,other.m_p); }
						void reset() { if(m_control&&m_control->wdec()) delete m_control; m_control = nullptr; m_p = nullptr; }
						bool expired() const { return m_control == nullptr; }
						unsigned int use_count() const { return expired() ? 0U : m_control->use_count(); }
						shared_ptr<T> lock() const { return shared_ptr<T>::from_weak(m_control,m_p); }
					private:
						shared_ptr_control * m_control ;
						T * m_p ;
				} ;
				template <typename T, typename U> shared_ptr<T> const_pointer_cast( const shared_ptr<U> & ptr_in ) g__noexcept
				{
					T * p = const_cast<T*>(ptr_in.get()) ;
					return shared_ptr<T>( ptr_in , p ) ;
				}
				template <typename T> class default_delete
				{
					public: void operator()( T * p ) { delete p ; }
				} ;
				template <typename T, typename D=default_delete<T> > class unique_ptr
				{
					public:
						unique_ptr() g__noexcept : m_p(0) {}
						explicit unique_ptr( T * p ) g__noexcept : m_p(p) {}
						unique_ptr( T * p , D d ) g__noexcept : m_p(p) , m_d(d) {}
						~unique_ptr() { reset(); }
						void reset( T * p ) g__noexcept { m_d(m_p); m_p = p; }
						void reset() g__noexcept { reset(nullptr); }
						T * get() g__noexcept { return m_p; }
						T & operator*() g__noexcept { return *m_p; }
						const T * get() const g__noexcept { return m_p; }
						const T & operator*() const g__noexcept { return *m_p; }
						T * operator->() g__noexcept { return m_p; }
						const T * operator->() const g__noexcept { return m_p; }
						T * release() g__noexcept { T * p = m_p; m_p = 0; return p; }
						operator bool () const g__noexcept { return m_p != nullptr; }
						void swap( unique_ptr<T,D> & other ) g__noexcept { using std::swap; swap(m_p,other.m_p); swap(m_d,other.m_d); }
						/* define a copy constructor that behaves like auto_ptr -- this reasonable if the code also compiles correctly with c++11 */
						unique_ptr( const unique_ptr<T,D> & rhs ) { m_p = rhs.m_p ; const_cast<unique_ptr<T,D>&>(rhs).m_p = 0 ; }
					private:
						void operator=( const unique_ptr<T,D> & ) g__eq_delete ;
					private:
						T * m_p ;
						D m_d ;
				} ;
				template <typename T, typename D> void swap( unique_ptr<T,D> & a , unique_ptr<T,D> & b ) g__noexcept { a.swap(b) ; }
			}
		#endif

		#if !GCONFIG_HAVE_CXX_SHARED_PTR || defined(G_USE_SHARED_PTR)
			using G::shared_ptr ;
			using G::unique_ptr ;
			using G::weak_ptr ;
			using G::const_pointer_cast ;
		#else
			#include <memory>
			using std::shared_ptr ;
			using std::unique_ptr ;
			using std::weak_ptr ;
			using std::const_pointer_cast ;
		#endif

		#if GCONFIG_ENABLE_STD_THREAD
			#include <thread>
			#include <mutex>
			#include <cstring>
			namespace G
			{
				struct threading /// Helper class for std::thread capabilities.
				{
					G_CONSTANT( int , using_std_thread , 1 ) ;
					typedef std::thread thread_type ;
					typedef std::mutex mutex_type ;
					typedef std::lock_guard<std::mutex> lock_type ;
					static bool works() ; // run-time test -- see gthread.cpp
					static void yield() g__noexcept { std::this_thread::yield() ; }
				} ;
			}
		#else
			namespace G
			{
				class dummy_thread
				{
					public:
						typedef int id ;
						template <typename T_fn> explicit dummy_thread( T_fn fn ) { fn() ; }
						template <typename T_fn,typename T_arg1> dummy_thread( T_fn fn , T_arg1 arg1 ) { fn(arg1) ; }
						template <typename T_fn,typename T_arg1,typename T_arg2> dummy_thread( T_fn fn , T_arg1 arg1 , T_arg2 arg2 ) { fn(arg1,arg2) ; }
						dummy_thread() {}
						bool joinable() const g__noexcept { return false ; }
						void detach() {}
						void join() {}
						id get_id() const { return 0 ; }
				} ;
				class dummy_mutex {} ;
				class dummy_lock { public: explicit dummy_lock( dummy_mutex & ) {} } ;
				struct threading
				{
					G_CONSTANT( int , using_std_thread , 0 ) ;
					typedef G::dummy_thread thread_type ;
					typedef G::dummy_mutex mutex_type ;
					typedef G::dummy_lock lock_type ;
					static bool works() ;
					static void yield() g__noexcept {}
				} ;
			}
		#endif

		#if ! GCONFIG_HAVE_CXX_TYPE_TRAITS_MAKE_UNSIGNED
			namespace std
			{
				template <typename T> struct make_unsigned {} ;
				template <> struct make_unsigned<unsigned char> { typedef unsigned char type ; } ;
				template <> struct make_unsigned<char> { typedef unsigned char type ; } ;
				template <> struct make_unsigned<signed char> { typedef unsigned char type ; } ;
				template <> struct make_unsigned<unsigned short> { typedef unsigned short type ; } ;
				template <> struct make_unsigned<short> { typedef unsigned short type ; } ;
				template <> struct make_unsigned<unsigned int> { typedef unsigned int type ; } ;
				template <> struct make_unsigned<int> { typedef unsigned int type ; } ;
				template <> struct make_unsigned<unsigned long> { typedef unsigned long type ; } ;
				template <> struct make_unsigned<long> { typedef unsigned long type ; } ;
				#if GCONFIG_HAVE_LONG_LONG
				template <> struct make_unsigned<unsigned long long> { typedef unsigned long long type ; } ;
				template <> struct make_unsigned<long long> { typedef unsigned long long type ; } ;
				#endif
			}
		#endif

	#endif

	/* Run-time o/s identification
	*/
	#ifdef __cplusplus
		namespace G
		{
			#ifdef G_WINDOWS
				inline g__constexpr_fn bool is_windows() { return true ; }
			#else
				inline g__constexpr_fn bool is_windows() { return false ; }
			#endif
			#ifdef G_UNIX_LINUX
				inline g__constexpr_fn bool is_linux() { return true ; }
			#else
				inline g__constexpr_fn bool is_linux() { return false ; }
			#endif
			#ifdef G_UNIX_FREEBSD
				inline g__constexpr_fn bool is_free_bsd() { return true ; }
			#else
				inline g__constexpr_fn bool is_free_bsd() { return false ; }
			#endif
			#ifdef G_UNIX_OPENBSD
				inline g__constexpr_fn bool is_open_bsd() { return true ; }
			#else
				inline g__constexpr_fn bool is_open_bsd() { return false ; }
			#endif
			#ifdef G_UNIX_BSD
				inline g__constexpr_fn bool is_bsd() { return true ; }
			#else
				inline g__constexpr_fn bool is_bsd() { return false ; }
			#endif
		}
	#endif

	/* Network code fix-ups
	 */

	typedef g_uint16_t g_port_t ; /* since 'in_port_t' not always available */

	#ifdef G_WINDOWS
		#ifdef G_MINGW
			#ifndef AI_NUMERICSERV
				#define AI_NUMERICSERV 0
			#endif
		#endif
		#ifndef MSG_NOSIGNAL
			#define MSG_NOSIGNAL 0
		#endif
	#endif
	#ifndef AI_ADDRCONFIG
		#define AI_ADDRCONFIG 0
	#endif
	#ifndef INADDR_NONE
		/* (should be in netinet/in.h) */
		#define INADDR_NONE 0xffffffff
	#endif

	/* Inline portability shims
	 */

	#ifdef __cplusplus
		#if GCONFIG_HAVE_IPV6
			inline void gnet_address6_init( sockaddr_in6 & s )
			{
				#if GCONFIG_HAVE_SIN6_LEN
					s.sin6_len = sizeof(s) ;
				#else
					(void) s ;
				#endif
			}
		#endif
	#endif

	/* Inline definitions of missing functions
	 */

	#ifdef __cplusplus
		namespace GNet { int inet_pton_imp( int f , const char * p , void * result ) ; }
		#if ! GCONFIG_HAVE_INET_PTON
			inline int inet_pton( int f , const char * p , void * result )
			{
				return GNet::inet_pton_imp( f , p , result ) ;
			}
		#endif
	#endif

	#ifdef __cplusplus
		namespace GNet { const char * inet_ntop_imp( int f , void * ap , char * buffer , size_t n ) ; }
		#if ! GCONFIG_HAVE_INET_NTOP
			inline const char * inet_ntop( int f , void * ap , char * buffer , size_t n )
			{
				return GNet::inet_ntop_imp( f , ap , buffer , n ) ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_IF_NAMETOINDEX
		#ifdef __cplusplus
			inline unsigned int if_nametoindex( const char * )
			{
				return 0U ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_READLINK && !defined(readlink)
		#ifdef __cplusplus
			inline ssize_t readlink( const char * , char * , size_t )
			{
				return -1 ;
			}
		#endif
	#endif

	#if GCONFIG_HAVE_GETPWNAM && ! GCONFIG_HAVE_GETPWNAM_R
		#ifdef __cplusplus
			#include <pwd.h>
			inline int getpwnam_r( const char * name , struct passwd * pwd ,
				char * buf , size_t buflen , struct passwd ** result )
			{
				struct passwd * p = ::getpwnam( name ) ;
				if( p )
				{
					*pwd = *p ; /* string pointers still point into library storage */
					*result = pwd ;
					return 0 ;
				}
				else
				{
					*result = nullptr ;
					return 0 ;
				}
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_GMTIME_R && !defined(gmtime_r) && defined(G_UNIX)
		#ifdef __cplusplus
			#include <ctime>
			inline std::tm * gmtime_r( const std::time_t * tp , std::tm * tm_p )
			{
				const struct std::tm * p = std::gmtime( tp ) ;
				if( p == 0 ) return 0 ;
				*tm_p = *p ;
				return tm_p ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_LOCALTIME_R && !defined(localtime_r) && defined(G_UNIX)
		#ifdef __cplusplus
			#include <ctime>
			inline struct std::tm * localtime_r( const std::time_t * tp , struct std::tm * tm_p )
			{
				const struct std::tm * p = std::localtime( tp ) ;
				if( p == 0 ) return 0 ;
				*tm_p = *p ;
				return tm_p ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_LOCALTIME_S && !defined(localtime_s)
		#ifdef __cplusplus
			inline errno_t localtime_s( struct std::tm * tm_p , const std::time_t * tp )
			{
				const errno_t e_inval = 22 ;
				if( tm_p == nullptr ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon =
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == nullptr || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::localtime( tp ) ;
				if( p == 0 ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_GMTIME_S && !defined(gmtime_s)
		#ifdef __cplusplus
			inline errno_t gmtime_s( struct std::tm * tm_p , const std::time_t * tp )
			{
				const errno_t e_inval = 22 ;
				if( tm_p == nullptr ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon =
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == nullptr || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::gmtime( tp ) ;
				if( p == 0 ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif
	#endif

	#if GCONFIG_HAVE_SETGROUPS
		#include <grp.h>
	#else
		#ifdef __cplusplus
			inline int setgroups( size_t , const gid_t * )
			{
				return 0 ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_GETENV_S
		#ifdef __cplusplus
			#include <cstdlib>
			inline errno_t getenv_s( size_t * n_out , char * buffer , size_t n_in , const char * name )
			{
				const errno_t e_inval = 22 ;
				const errno_t e_range = 34 ;
				if( n_out == nullptr || name == nullptr ) return e_inval ;
				const char * p = ::getenv( name ) ;
				*n_out = p ? (strlen(p) + 1U) : 0 ;
				if( p && *n_out > n_in ) return e_range ;
				if( p && buffer ) strcpy( buffer , p ) ;
				return 0 ;
			}

		#endif
	#endif

	#if ! GCONFIG_HAVE_STRNCPY_S
		#ifndef _TRUNCATE
			#define _TRUNCATE (~((size_t)(0U)))
		#endif
		#ifdef __cplusplus
			inline errno_t strncpy_s( char * dst , size_t n_dst , const char * src , size_t n_src )
			{
				if( dst == nullptr ) return EINVAL ;
				if( src == nullptr ) { *dst = '\0' ; return EINVAL ; }
				if( n_dst == 0U ) { return EINVAL ; }
				size_t d = strlen(src) ; if( n_src != _TRUNCATE && n_src < d ) d = n_src ;
				if( d >= n_dst && n_src == _TRUNCATE )
				{
					strncpy( dst , src , n_dst ) ;
					dst[n_dst-1U] = '\0' ;
				}
				else if( d >= n_dst )
				{
					*dst = '\0' ;
					return ERANGE ;
				}
				else
				{
					strncpy( dst , src , d ) ;
					dst[d] = '\0' ;
				}
				return 0 ;
			}

		#endif
	#endif

	#if ! GCONFIG_HAVE_GET_WINDOW_LONG_PTR && defined(G_WINDOWS)
		#ifdef __cplusplus
			typedef char assert_thirty_two_bit_windows[sizeof(void*)==4U?1:-1] ; // if this fails then we are on win64 so no need for this block at all
			const int GWLP_HINSTANCE = GWL_HINSTANCE ;
			const int GWLP_WNDPROC = GWL_WNDPROC ;
			const int DWLP_USER = DWL_USER ;
			inline LONG_PTR GetWindowLongPtr( HWND h , int id )
			{
				return static_cast<LONG_PTR>(::GetWindowLong(h,id)) ;
			}
			inline LONG_PTR SetWindowLongPtr( HWND h , int id , LONG_PTR value )
			{
				return static_cast<LONG_PTR>(::SetWindowLong(h,id,static_cast<LONG>(value))) ;
			}
		#endif
	#endif

	#if ! GCONFIG_HAVE_MREMAP && defined(G_UNIX)
		#ifdef __cplusplus
			inline void * mremap( void * , size_t , size_t , int )
			{
				errno = ENOSYS ;
				return (void*)(-1) ;
			}
		#endif
	#endif

	#if GCONFIG_HAVE_SETPGRP_BSD && defined(G_UNIX)
		#ifdef __cplusplus
			inline int setpgrp()
			{
				return ::setpgrp( 0 , 0 ) ;
			}
		#endif
	#endif

#endif
