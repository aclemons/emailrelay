/*
   Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
 * preprocessor switches.
 */

#ifndef G_DEF_H
#define G_DEF_H

	/* Pull in GCONFIG definitions. Uses an odd file name to avoid
	 * picking up a header from some third-party library. If this
	 * file does not exist then there might be a suitable template
	 * file to copy, or just create an empty file and maybe tweak
	 * the values that are defaulted below.
	 */
	#ifndef GCONFIG_NO_GCONFIG_DEFS
		#include <gconfig_defs.h>
	#endif

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

	/* Apply GCONFIG defaults in case of no autoconf
	 */
	#if !defined(GCONFIG_HAVE_CXX_ALIGNMENT)
		#define GCONFIG_HAVE_CXX_ALIGNMENT 1
	#endif
	#if !defined(GCONFIG_HAVE_CXX_STD_MAKE_UNIQUE)
		#if defined(_MSC_VER)
			#define GCONFIG_HAVE_CXX_STD_MAKE_UNIQUE 1
		#else
			#if __cplusplus >= 201400L
				#define GCONFIG_HAVE_CXX_STD_MAKE_UNIQUE 1
			#else
				#define GCONFIG_HAVE_CXX_STD_MAKE_UNIQUE 0
			#endif
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_CXX_STRING_VIEW)
		#define GCONFIG_HAVE_CXX_STRING_VIEW 0
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
	#if !defined(GCONFIG_HAVE_PUTENV_S)
		#if ( defined(G_WINDOWS) && !defined(G_MINGW) ) || defined(putenv_s)
			#define GCONFIG_HAVE_PUTENV_S 1
		#else
			#define GCONFIG_HAVE_PUTENV_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PUTENV)
		#define GCONFIG_HAVE_PUTENV 1
	#endif
	#if !defined(GCONFIG_HAVE_GETPWNAM)
		#define GCONFIG_HAVE_GETPWNAM 1
	#endif
	#if !defined(GCONFIG_HAVE_GETPWNAM_R)
		#define GCONFIG_HAVE_GETPWNAM_R 1
	#endif
		#if !defined(GCONFIG_HAVE_GETGRNAM)
		#define GCONFIG_HAVE_GETGRNAM 1
	#endif
	#if !defined(GCONFIG_HAVE_GETGRNAM_R)
		#define GCONFIG_HAVE_GETGRNAM_R 1
	#endif
	#if !defined(GCONFIG_HAVE_GMTIME_R)
		#if defined(G_WINDOWS) && !defined(gmtime_r)
			#define GCONFIG_HAVE_GMTIME_R 0
		#else
			#define GCONFIG_HAVE_GMTIME_R 1
		#endif
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
	#if !defined(GCONFIG_HAVE_EXECVPE)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_EXECVPE 1
		#else
			#define GCONFIG_HAVE_EXECVPE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_RTNETLINK)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_RTNETLINK 1
		#else
			#define GCONFIG_HAVE_RTNETLINK 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_NETROUTE)
		#ifdef G_UNIX_BSD
			#define GCONFIG_HAVE_NETROUTE 1
		#else
			#define GCONFIG_HAVE_NETROUTE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_LOCALTIME_R)
		#if defined(G_WINDOWS) && !defined(localtime_r)
			#define GCONFIG_HAVE_LOCALTIME_R 0
		#else
			#define GCONFIG_HAVE_LOCALTIME_R 1
		#endif
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
	#if !defined(GCONFIG_HAVE_STATBUF_TIMESPEC)
		#if defined(G_UNIX_MAC)
			#define GCONFIG_HAVE_STATBUF_TIMESPEC 1
		#else
			#define GCONFIG_HAVE_STATBUF_TIMESPEC 0
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
		#if ( defined(G_WINDOWS) && !defined(G_MINGW) ) || defined(strncpy_s)
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
		#ifdef G_UNIX
			#define GCONFIG_HAVE_SYS_WAIT_H 1
		#else
			#define GCONFIG_HAVE_SYS_WAIT_H 0
		#endif
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
	#if !defined(GCONFIG_HAVE_NET_IF_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_NET_IF_H 1
		#else
			#define GCONFIG_HAVE_NET_IF_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_WINDOWS_IPHLPAPI_H)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_WINDOWS_IPHLPAPI_H 0
		#else
			#define GCONFIG_HAVE_WINDOWS_IPHLPAPI_H 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_GAISTRERROR)
		#define GCONFIG_HAVE_GAISTRERROR 1
	#endif
	#if !defined(GCONFIG_HAVE_INET_NTOP)
		#define GCONFIG_HAVE_INET_NTOP 1
	#endif
	#if !defined(GCONFIG_HAVE_IFNAMETOINDEX)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_IFNAMETOINDEX 1
		#else
			#define GCONFIG_HAVE_IFNAMETOINDEX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_IFNAMETOLUID)
		#if defined(G_WINDOWS) && !defined(G_MINGW)
			#define GCONFIG_HAVE_IFNAMETOLUID 1
		#else
			#define GCONFIG_HAVE_IFNAMETOLUID 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_IFINDEX)
		#ifdef G_UNIX_LINUX
			#define GCONFIG_HAVE_IFINDEX 1
		#else
			#define GCONFIG_HAVE_IFINDEX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_INET_PTON)
		#define GCONFIG_HAVE_INET_PTON 1
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
		#if !defined(GCONFIG_PAM_CONST)
		#if defined(G_UNIX_LINUX) || defined(G_UNIX_BSD)
			#define GCONFIG_PAM_CONST 1
		#else
			#define GCONFIG_PAM_CONST 0
		#endif
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
	#if !defined(GCONFIG_HAVE_GETTEXT)
		/* see AM_GNU_GETTEXT */
		#if defined(ENABLE_NLS) && ENABLE_NLS == 1
			#define GCONFIG_HAVE_GETTEXT 1
		#else
			#define GCONFIG_HAVE_GETTEXT 0
		#endif
	#endif
	#if !defined(GCONFIG_ENABLE_IPV6)
		#define GCONFIG_ENABLE_IPV6 GCONFIG_HAVE_IPV6
	#endif
	#if !defined(GCONFIG_ENABLE_STD_THREAD)
		#define GCONFIG_ENABLE_STD_THREAD 1
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
	#if !defined(GCONFIG_HAVE_WAYLAND)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_WAYLAND 0
		#else
			#define GCONFIG_HAVE_WAYLAND 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_MBEDTLS)
		#define GCONFIG_HAVE_MBEDTLS 1
	#endif
	#if !defined(GCONFIG_HAVE_MBEDTLS_NET_H)
		#define GCONFIG_HAVE_MBEDTLS_NET_H 0
	#endif
	#if !defined(GCONFIG_MBEDTLS_DISABLE_PSA_HEADER)
		#ifdef G_UNIX
			#define GCONFIG_MBEDTLS_DISABLE_PSA_HEADER 0
		#else
			#define GCONFIG_MBEDTLS_DISABLE_PSA_HEADER 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_OPENSSL 1
		#else
			#define GCONFIG_HAVE_OPENSSL 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_OPENSSL_MIN_MAX)
		#define GCONFIG_HAVE_OPENSSL_MIN_MAX 1
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
	#if !defined(GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX)
		#if defined(G_WINDOWS) && !defined(G_MINGW)
			#define GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX 1
		#else
			#define GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H)
		#if defined(G_WINDOWS)
			#define GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H 1
		#else
			#define GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX)
		#if defined(G_WINDOWS) && !defined(G_MINGW)
			#define GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX 1
		#else
			#define GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX 0
		#endif
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
	#if !defined(GCONFIG_HAVE_FOPEN_S)
		#if defined(G_WINDOWS)
			#define GCONFIG_HAVE_FOPEN_S 1
		#else
			#define GCONFIG_HAVE_FOPEN_S 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SOPEN)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_SOPEN 0
		#else
			#define GCONFIG_HAVE_SOPEN 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SOPEN_S)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_SOPEN_S 0
		#else
			#define GCONFIG_HAVE_SOPEN_S 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_EXTENDED_OPEN)
		#if defined(G_UNIX) || defined(G_MINGW)
			#define GCONFIG_HAVE_EXTENDED_OPEN 0
		#else
			#define GCONFIG_HAVE_EXTENDED_OPEN 1
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_SIGPROCMASK)
		#if defined(G_UNIX)
			#define GCONFIG_HAVE_SIGPROCMASK 1
		#else
			#define GCONFIG_HAVE_SIGPROCMASK 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_PTHREAD_SIGMASK)
		#if defined(G_UNIX)
			#define GCONFIG_HAVE_PTHREAD_SIGMASK 1
		#else
			#define GCONFIG_HAVE_PTHREAD_SIGMASK 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_DIRENT_D_TYPE)
		#if defined(_DIRENT_HAVE_D_TYPE)
			#define GCONFIG_HAVE_DIRENT_D_TYPE 1
		#else
			#define GCONFIG_HAVE_DIRENT_D_TYPE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_IOVEC_SIMPLE)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_IOVEC_SIMPLE 1
		#else
			#define GCONFIG_HAVE_IOVEC_SIMPLE 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_UDS)
		#ifdef G_UNIX
			#define GCONFIG_HAVE_UDS 1
		#else
			#define GCONFIG_HAVE_UDS 0
		#endif
	#endif
	#if !defined(GCONFIG_HAVE_UDS_LEN)
		#ifdef G_UNIX_BSD
			#define GCONFIG_HAVE_UDS_LEN 1
		#else
			#define GCONFIG_HAVE_UDS_LEN 0
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
		#if GCONFIG_HAVE_WINDOWS_IPHLPAPI_H
			#include <iphlpapi.h>
		#endif
		#if GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H
			#include <versionhelpers.h>
		#endif
	#endif

	/* Include commonly-used c++ headers
	 */
	#ifdef __cplusplus
		#include <cstddef>
		#include <cstdlib>
		#include <ios>
		#include <iosfwd>
	#else
		#include <stddef.h>
		#include <stdlib.h>
	#endif

	/* Include main o/s headers
	 */
	#if defined(G_WINDOWS)
		#include <shellapi.h>
		#include <direct.h>
		#include <share.h>
	#endif
	#if GCONFIG_HAVE_SYS_TYPES_H
		#include <sys/types.h>
	#endif
	#if GCONFIG_HAVE_SYS_STAT_H
		#include <sys/stat.h>
	#endif
	#if GCONFIG_HAVE_INTTYPES_H
		#ifdef __cplusplus
			#include <cinttypes>
		#else
			#include <inttypes.h>
		#endif
	#endif
	#if GCONFIG_HAVE_STDINT_H
		#ifdef __cplusplus
			#include <cstdint>
		#else
			#include <stdint.h>
		#endif
	#endif
	#if GCONFIG_HAVE_UNISTD_H
		#include <unistd.h>
	#endif
	#if GCONFIG_HAVE_ERRNO_H
		#ifdef __cplusplus
			#include <cerrno>
		#else
			#include <errno.h>
		#endif
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
	#if GCONFIG_HAVE_NET_IF_H
		#include <net/if.h>
	#endif
	#if GCONFIG_HAVE_PWD_H
		#include <pwd.h>
	#endif
	#if GCONFIG_HAVE_GRP_H
		#include <grp.h>
	#endif
	#ifndef __cplusplus
		#include <wchar.h>
	#endif

 	#ifdef __cplusplus

		/* Undefine some unwanted macros
	 	*/
		#ifdef max
			#undef max
		#endif
		#ifdef min
			#undef min
		#endif
		#ifdef alignas
			#undef alignas
		#endif
		#ifdef stdin
			#undef stdin
		#endif
		#ifdef stdout
			#undef stdout
		#endif
		#ifdef stderr
			#undef stderr
		#endif

		/* Define a few Windows-style types under unix
	 	*/
		#if ! defined(G_WINDOWS)
			using BOOL = unsigned char ;
			using HDC = unsigned int ;
			using HWND = unsigned int ;
			using HINSTANCE = unsigned int ;
			using HANDLE = unsigned int ;
			using TCHAR = wchar_t ;
			using SOCKET = int ;
		#endif

		/* Define a null value for opaque pointer types that are
	 	 * never dereferenced
	 	 */
		#define HNULL 0

		/* Define fixed-size types - the underlying types should come
	 	 * from C99's stdint.h, but they are all optional
	 	 */
		#if defined(G_WINDOWS)
			#if GCONFIG_HAVE_INT64
				using g_int64_t = INT64 ;
				using g_uint64_t = UINT64 ;
			#endif
			#if GCONFIG_HAVE_INT32
				using g_int32_t = INT32 ;
				using g_uint32_t = UINT32 ;
			#endif
			#if GCONFIG_HAVE_INT16
				using g_int16_t = INT16 ;
				using g_uint16_t = UINT16 ;
			#endif
		#else
			#if GCONFIG_HAVE_INT64
				using g_int64_t = int64_t ;
				using g_uint64_t = uint64_t ;
			#endif
			#if GCONFIG_HAVE_INT32
				using g_int32_t = int32_t ;
				using g_uint32_t = uint32_t ;
			#endif
			#if GCONFIG_HAVE_INT16
				using g_int16_t = int16_t ;
				using g_uint16_t = uint16_t ;
			#endif
		#endif
		#if GCONFIG_HAVE_UINTPTR_T
			using g_uintptr_t = uintptr_t ; // uintptr_t in C99 and C++2011
		#else
			using g_uintptr_t = std::size_t ; // assumes a non-segmented architecture - see also windows LONG_PTR
		#endif
		#if GCONFIG_HAVE_INT64
			static_assert( sizeof(g_int64_t) == 8U , "uint64 wrong size" ) ;
			static_assert( sizeof(g_uint64_t) == 8U , "int64 wrong size" ) ;
		#endif
		#if GCONFIG_HAVE_INT32
			static_assert( sizeof(g_uint32_t) == 4U , "uint32 wrong size" ) ;
			static_assert( sizeof(g_int32_t) == 4U , "int32 wrong size" ) ;
		#endif
		#if GCONFIG_HAVE_INT16
			static_assert( sizeof(g_uint16_t) == 2U , "uint16 wrong size" ) ;
			static_assert( sizeof(g_int16_t) == 2U , "int16 wrong size" ) ;
		#endif
		static_assert( sizeof(g_uintptr_t) >= sizeof(void*) , "" ) ; // try 'using g_uintptr_t = unsigned long'

		/* Define missing standard types
	 	*/
		#if ! GCONFIG_HAVE_UID_T
			using uid_t = int ;
		#endif
		#if ! GCONFIG_HAVE_GID_T
			using gid_t = int ;
		#endif
		#if ! GCONFIG_HAVE_SSIZE_T
			#if defined(SSIZE_T)
				using ssize_t = SSIZE_T ;
			#else
				/* read(2) return type -- 'int' on some unix systems */
				using ssize_t = int ;
			#endif
		#endif
		#if ! GCONFIG_HAVE_PID_T
			using pid_t = unsigned int ;
		#endif
		#if ! GCONFIG_HAVE_SOCKLEN_T
			using socklen_t = int ;
		#endif
		#if ! GCONFIG_HAVE_ERRNO_T
			using errno_t = int ; // whatever_s() return type
		#endif

		/* Attributes
	 	*/
		#if __cplusplus >= 201700L
			#define GDEF_NORETURN_LHS [[noreturn]]
			#define GDEF_UNUSED [[maybe_unused]]
			#define GDEF_FALLTHROUGH [[fallthrough]];
		#else
			#if defined(__GNUC__) || defined(__clang__)
				#define GDEF_NORETURN_RHS __attribute__((noreturn))
				#define GDEF_UNUSED __attribute__((unused))
				#if defined(__GNUC__) && __GNUC__ >= 7
					#define GDEF_FALLTHROUGH __attribute__((fallthrough));
				#endif
				#if defined(__clang__) && __clang_major__ >= 10
					#define GDEF_FALLTHROUGH __attribute__((fallthrough));
				#endif
			#endif
			#if defined(_MSC_VER)
				#define GDEF_NORETURN_LHS __declspec(noreturn)
			#endif
		#endif
		#ifndef GDEF_NORETURN_LHS
			#define GDEF_NORETURN_LHS
		#endif
		#ifndef GDEF_NORETURN_RHS
			#define GDEF_NORETURN_RHS
		#endif
		#ifndef GDEF_UNUSED
			#define GDEF_UNUSED
		#endif
		#ifndef GDEF_FALLTHROUGH
			#define GDEF_FALLTHROUGH
		#endif
		#include <tuple>
		namespace G { template <typename... T> inline void gdef_ignore( T&& ... ) {} }
		#define GDEF_IGNORE_PARAMS(...) G::gdef_ignore(__VA_ARGS__)
		#define GDEF_IGNORE_RETURN std::ignore =
		#define GDEF_IGNORE_PARAM(name) std::ignore = name
		#define GDEF_IGNORE_VARIABLE(name) std::ignore = name

		/* C++ language backwards compatibility
	 	*/
		#if !GCONFIG_HAVE_CXX_STD_MAKE_UNIQUE
			#include <memory>
			#include <utility>
			namespace std // NOLINT
			{
				template <typename T, typename... Args>
				std::unique_ptr<T> make_unique( Args&&... args )
				{
					return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) ) ; // NOLINT
				}
			}
		#endif

		/* Threading helper
	 	*/
		#if GCONFIG_ENABLE_STD_THREAD
			#include <thread>
			#include <mutex>
			#include <cstring>
			namespace G
			{
				struct threading /// Helper class for std::thread capabilities.
				{
					static constexpr bool using_std_thread = true ;
					using thread_type = std::thread ;
					using mutex_type = std::mutex ;
					using lock_type = std::lock_guard<std::mutex> ;
					static bool works() ; // run-time test -- see gthread.cpp
					static void yield() noexcept { std::this_thread::yield() ; }
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
						bool joinable() const noexcept { return false ; }
						void detach() {}
						void join() {}
						id get_id() const { return 0 ; }
				} ;
				class dummy_mutex {} ;
				class dummy_lock { public: explicit dummy_lock( dummy_mutex & ) {} } ;
				struct threading
				{
					static constexpr bool using_std_thread = false ;
					using thread_type = G::dummy_thread ;
					using mutex_type = G::dummy_mutex ;
					using lock_type = G::dummy_lock ;
					static bool works() ;
					static void yield() noexcept {}
				} ;
			}
		#endif

		/* Run-time o/s identification
		*/
		namespace G
		{
			#ifdef G_WINDOWS
				inline constexpr bool is_windows() { return true ; }
			#else
				inline constexpr bool is_windows() { return false ; }
			#endif
			#ifdef G_MINGW
				inline constexpr bool is_wine() { return true ; }
			#else
				inline constexpr bool is_wine() { return false ; }
			#endif
			#ifdef G_UNIX_LINUX
				inline constexpr bool is_linux() { return true ; }
			#else
				inline constexpr bool is_linux() { return false ; }
			#endif
			#ifdef G_UNIX_FREEBSD
				inline constexpr bool is_free_bsd() { return true ; }
			#else
				inline constexpr bool is_free_bsd() { return false ; }
			#endif
			#ifdef G_UNIX_OPENBSD
				inline constexpr bool is_open_bsd() { return true ; }
			#else
				inline constexpr bool is_open_bsd() { return false ; }
			#endif
			#ifdef G_UNIX_BSD
				inline constexpr bool is_bsd() { return true ; }
			#else
				inline constexpr bool is_bsd() { return false ; }
			#endif
		}

		/* Network code fix-ups
	 	*/

		using g_port_t = g_uint16_t ; /* since 'in_port_t' not always available */

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

		#if GCONFIG_HAVE_IPV6
			inline void gdef_address6_init( sockaddr_in6 & s )
			{
				#if GCONFIG_HAVE_SIN6_LEN
					s.sin6_len = sizeof(s) ;
				#else
					(void) s ;
				#endif
			}
		#endif

		/* Inline definitions of missing functions
	 	*/

		#if ! GCONFIG_HAVE_GAISTRERROR
			inline const char * gai_strerror( int ) // wrt getaddrinfo(3)
			{
				return nullptr ;
			}
		#endif

		namespace GNet { int inet_pton_imp( int f , const char * p , void * result ) ; }
		#if ! GCONFIG_HAVE_INET_PTON
			inline int inet_pton( int f , const char * p , void * result )
			{
				return GNet::inet_pton_imp( f , p , result ) ;
			}
		#endif

		namespace GNet { const char * inet_ntop_imp( int f , void * ap , char * buffer , std::size_t n ) ; }
		#if ! GCONFIG_HAVE_INET_NTOP
			inline const char * inet_ntop( int f , void * ap , char * buffer , std::size_t n )
			{
				return GNet::inet_ntop_imp( f , ap , buffer , n ) ;
			}
		#endif

		#if GCONFIG_HAVE_PTHREAD_SIGMASK && GCONFIG_ENABLE_STD_THREAD
			#include <csignal>
			inline int gdef_pthread_sigmask( int how , const sigset_t * set_p , sigset_t * oldset_p ) noexcept
			{
				return pthread_sigmask( how , set_p , oldset_p ) ;
			}
		#else
			#if GCONFIG_HAVE_SIGPROCMASK
				#include <csignal>
				inline int gdef_pthread_sigmask( int how , const sigset_t * set_p , sigset_t * oldset_p ) noexcept
				{
					return sigprocmask( how , set_p , oldset_p ) ;
				}
			#else
				template <typename... T> int gdef_pthread_sigmask(T...) noexcept { return 0 ; }
			#endif
		#endif

		#if GCONFIG_HAVE_IFNAMETOLUID
			#include <iphlpapi.h>
			inline unsigned long gdef_if_nametoindex( const char * p )
			{
				NET_LUID luid ;
				if( ConvertInterfaceNameToLuidA( p , &luid ) )
				{
					_set_errno( EINVAL ) ;
					return 0U ;
				}
				NET_IFINDEX result = 0 ;
				if( ConvertInterfaceLuidToIndex( &luid , &result ) != NO_ERROR )
				{
					_set_errno( EINVAL ) ;
					return 0U ;
				}
				_set_errno( 0 ) ;
				return result ;
			}
		#else
			#if GCONFIG_HAVE_IFNAMETOINDEX
				inline unsigned long gdef_if_nametoindex( const char * p )
				{
					#ifdef G_WINDOWS
						_set_errno( 0 ) ;
					#else
						errno = 0 ;
					#endif
					return if_nametoindex( p ) ; // int->long
				}
			#else
				inline unsigned long gdef_if_nametoindex( const char * )
				{
					#ifdef G_WINDOWS
						_set_errno( EINVAL ) ;
					#else
						errno = EINVAL ;
					#endif
					return 0UL ;
				}
			#endif
		#endif
		#if ! GCONFIG_HAVE_READLINK && !defined(readlink)
			inline ssize_t readlink( const char * , char * , std::size_t )
			{
				return -1 ;
			}
		#endif
		#if ! GCONFIG_HAVE_EXECVPE && !defined(execvpe) && defined(G_UNIX)
			inline int execvpe( const char * , char * [] , char * [] )
			{
				errno = EINVAL ;
				return -1 ;
			}
		#endif

		#if GCONFIG_HAVE_GETPWNAM && ! GCONFIG_HAVE_GETPWNAM_R
			#include <pwd.h>
			inline int getpwnam_r( const char * name , struct passwd * pwd ,
				char * buf , std::size_t buflen , struct passwd ** result )
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

		#if GCONFIG_HAVE_GETGRNAM && ! GCONFIG_HAVE_GETGRNAM_R
			#include <pwd.h>
			inline int getgrnam_r( const char * name , struct group * grp ,
				char * buf , std::size_t buflen , struct passwd ** result )
			{
				struct group * p = ::getgrnam( name ) ;
				if( p )
				{
					*grp = *p ; /* string pointers still point into library storage */
					*result = grp ;
					return 0 ;
				}
				else
				{
					*result = nullptr ;
					return 0 ;
				}
			}
		#endif

		#if ! GCONFIG_HAVE_GMTIME_R && !defined(gmtime_r)
			#include <ctime>
			inline std::tm * gmtime_r( const std::time_t * tp , std::tm * tm_p )
			{
				#if GCONFIG_HAVE_GMTIME_S || defined(gmtime_s)
					errno_t e = gmtime_s( tm_p , tp ) ;
					if( e ) return nullptr ;
					return tm_p ;
				#else
					const struct std::tm * p = std::gmtime( tp ) ;
					if( p == 0 ) return nullptr ;
					*tm_p = *p ;
					return tm_p ;
				#endif
			}
		#endif

		#if ! GCONFIG_HAVE_LOCALTIME_R && !defined(localtime_r)
			#include <ctime>
			inline struct std::tm * localtime_r( const std::time_t * tp , struct std::tm * tm_p )
			{
				#if GCONFIG_HAVE_LOCALTIME_S || defined(localtime_s)
					errno_t e = localtime_s( tm_p , tp ) ;
					if( e ) return nullptr ;
					return tm_p ;
				#else
					const struct std::tm * p = std::localtime( tp ) ;
					if( p == 0 ) return nullptr ;
					*tm_p = *p ;
					return tm_p ;
				#endif
			}
		#endif

		#if ! GCONFIG_HAVE_LOCALTIME_S && !defined(localtime_s)
			#include <ctime>
			inline errno_t localtime_s( struct std::tm * tm_p , const std::time_t * tp )
			{
				const errno_t e_inval = 22 ;
				if( tm_p == nullptr ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon =
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == nullptr || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::localtime( tp ) ;
				if( p == nullptr ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif

		#if ! GCONFIG_HAVE_GMTIME_S && !defined(gmtime_s)
			#include <ctime>
			inline errno_t gmtime_s( struct std::tm * tm_p , const std::time_t * tp )
			{
				const errno_t e_inval = 22 ;
				if( tm_p == nullptr ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon =
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == nullptr || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::gmtime( tp ) ;
				if( p == nullptr ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif

		#if GCONFIG_HAVE_SETGROUPS
			#include <grp.h>
		#else
			inline int setgroups( std::size_t , const gid_t * )
			{
				return 0 ;
			}
		#endif

		#if ! GCONFIG_HAVE_GET_WINDOW_LONG_PTR && defined(G_WINDOWS)
			static_assert( sizeof(void*) == 4U , "" ) ; // if this fails then we are on win64 so no need for this block at all
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

		#if ! GCONFIG_HAVE_WINDOWS_VERSIONHELPERS_H && defined(G_WINDOWS)
			inline bool IsWindowsVistaOrGreater()
			{
				WORD major = HIBYTE( _WIN32_WINNT_VISTA ) ;
				WORD minor = LOBYTE( _WIN32_WINNT_VISTA ) ;
				OSVERSIONINFOEXW info {} ;
				info.dwOSVersionInfoSize = sizeof(info) ;
				info.dwMajorVersion = major ;
				info.dwMinorVersion = minor ;
				return !! VerifyVersionInfoW( &info , VER_MAJORVERSION | VER_MINORVERSION ,
					VerSetConditionMask( VerSetConditionMask(0,VER_MAJORVERSION,VER_GREATER_EQUAL) ,
							VER_MINORVERSION , VER_GREATER_EQUAL ) ) ;
			}
		#endif

		#if ! GCONFIG_HAVE_MREMAP && defined(G_UNIX)
			inline void * mremap( void * , std::size_t , std::size_t , int )
			{
				errno = ENOSYS ;
				return (void*)(-1) ;
			}
		#endif

		#if GCONFIG_HAVE_SETPGRP_BSD && defined(G_UNIX)
			inline int setpgrp()
			{
				return ::setpgrp( 0 , 0 ) ;
			}
		#endif

		#if ! GCONFIG_HAVE_CXX_ALIGNMENT
			namespace std // NOLINT
			{
				// missing in gcc 4.8.4 -- original copyright 2001-2016 FSF Inc, GPLv3
				inline void * align( size_t align , size_t size , void * & ptr_inout , size_t & space ) noexcept
				{
					const auto ptr = reinterpret_cast<uintptr_t>( ptr_inout ) ;
					const auto aligned = ( ptr - 1U + align ) & -align ;
					const auto diff = aligned - ptr ;
					if( (size + diff) > space )
					{
						return nullptr ;
					}
					else
					{
						space -= diff ;
						ptr_inout = reinterpret_cast<void*>( aligned ) ;
						return ptr_inout ;
					}
				}
			}
		#endif

	#endif

#endif
