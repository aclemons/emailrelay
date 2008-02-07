//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or 
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
///
/// \file gdef.h
///
/*
 * gdef.h
 */

/* This header is always the first header included in source
 * files. It takes care of some portability issues, and
 * is a good candidate for precompilation. It requires
 * either G_UNIX or G_WIN32 to be defined on the compiler
 * command line, although G_UNIX may also be inferred from
 * autoconf's HAVE_CONFIG_H.
 */

#ifndef G_DEF_H
#define G_DEF_H

	/* Autoconf, part 1
	 */
	#if defined( HAVE_CONFIG_H )

		#if ! defined( G_UNIX )
			#define G_UNIX
		#endif

		#include <config.h>

		#if HAVE_BUGGY_CTIME
			#include <time.h>
			#include <ctime>
		#endif

		#if ! HAVE_GMTIME_R || ! HAVE_LOCALTIME_R
			#ifdef __cplusplus
				#include <ctime>
			#endif
		#endif

		#if ! HAVE_SOCKLEN_T
			typedef int socklen_t ;
		#endif

		/* just in case, undefine if defined as zero in config.h */
		#if defined(USE_NO_ADMIN) && 0 == USE_NO_ADMIN
			#undef USE_NO_ADMIN
		#endif
		#if defined(USE_NO_AUTH) && 0 == USE_NO_AUTH
			#undef USE_NO_AUTH
		#endif
		#if defined(USE_NO_EXEC) && 0 == USE_NO_EXEC
			#undef USE_NO_EXEC
		#endif
		#if defined(USE_NO_POP) && 0 == USE_NO_POP
			#undef USE_NO_POP
		#endif
		#if defined(USE_SMALL_CONFIG) && 0 == USE_SMALL_CONFIG
			#undef USE_SMALL_CONFIG
		#endif
		#if defined(USE_SMALL_EXCEPTIONS) && 0 == USE_SMALL_EXCEPTIONS
			#undef USE_SMALL_EXCEPTIONS
		#endif
		#if defined(USE_IPV6) && 0 == USE_IPV6
			#undef USE_IPV6
		#endif

	#else
		#define HAVE_ZLIB_H 1
	#endif

	/* Check operating-system switches
	 */
	#if !defined( G_WIN32 ) && !defined( G_UNIX )
		#error invalid compilation switches - define G_WIN32 or G_UNIX
	#endif
	#if defined( G_WIN32 ) && defined( G_UNIX )
		#error invalid compilation switches - define G_WIN32 or G_UNIX
	#endif

	/* Define supplementary o/s compilation switches
	 */
	#if defined( G_WIN32 ) && ! defined( G_WINDOWS )
		#define G_WINDOWS
	#endif

	/* Define the compiler
	 */
	#if defined( _MSC_VER )
		#define G_COMPILER_IS_MICROSOFT 1
	#endif
	#if defined( __GNUC__ )
		#define G_COMPILER_IS_GNU 1
	#endif

	/* Define extra microsoft header tweaks
	 */
	#ifdef G_WIN32_IE
		#ifndef _WIN32_IE
			#define _WIN32_IE 0x600
		#endif
	#endif
	#ifdef G_WIN32_DCOM
		#ifndef _WIN32_DCOM
			#define _WIN32_DCOM
		#endif
	#endif

	/* Include main o/s headers
	 */
	#if defined( G_WINDOWS ) && defined( G_MINGW )
		#define __USE_W32_SOCKETS
		#include <windows.h>
		#include <shellapi.h>
		#include <sys/stat.h>
		#include <unistd.h>
	#elif defined( G_WINDOWS )
		#include <windows.h>
		#include <shellapi.h>
		#include <direct.h>
	#else
		#include <unistd.h>
		#include <sys/stat.h>
		#if ! defined( HAVE_CONFIG_H )
			#include <grp.h>
		#endif
	#endif

	/* Define Windows-style types (only used for unimplemented declarations under unix)
	 */
	#if ! defined( G_WINDOWS )
		typedef unsigned char BOOL ;
		typedef unsigned int HWND ;
		typedef unsigned int HINSTANCE ;
		typedef unsigned int HANDLE ;
	#endif

	/* Include commonly-used system headers (good for pre-compilation)
	 */
	#ifdef __cplusplus
		#include <cstddef>
		#include <exception>
		#include <fstream>
		#include <iostream>
		#include <memory>
		#include <sstream>
		#include <string>
	#endif

	/* Define fixed-size types
	 */
	typedef unsigned long g_uint32_t ;
	typedef unsigned short g_uint16_t ;
	typedef long g_int32_t ;
	typedef short g_int16_t ;

	/* Define short-name types
	 */
	typedef unsigned char uchar_t ;

	/* Define missing standard types
	 */
	#if defined( G_WINDOWS )
		typedef int uid_t ;
		typedef int gid_t ;
		#if ! defined( G_MINGW )
			typedef int ssize_t ;
			typedef unsigned int pid_t ;
		#endif
	#endif

	/* Pull some std types into the global namespace
	 */
	#ifdef __cplusplus
		using std::size_t ;
	#endif

	/* Modify compiler error handling
	 */
	#if defined(G_COMPILER_IS_MICROSOFT)
		#pragma warning( disable : 4100 ) /* unused formal parameter */
		#pragma warning( disable : 4355 ) /* 'this' in initialiser list */
		#pragma warning( disable : 4511 ) /* cannot create default copy ctor */
		#pragma warning( disable : 4512 ) /* cannot create default op=() */
		#pragma warning( disable : 4786 ) /* truncation in debug info */
		#pragma warning( disable : 4275 ) /* dll-interface stuff in <complex> */
	#endif

	/* A macro to explicitly ignore a function's return value.
	 * Some compilers complain when return values are ignored, and
	 * others complain when using a c-style cast...
	 */
	#if 1
		#define G_IGNORE
	#else
		#define G_IGNORE (void)
	#endif

	/* Use smaller buffers and limits if building with the uClibc run-time library.
	 * See glimits.h. This assumes that features.h has been included as a side-effect 
	 * of including system headers above.
	 */
	#ifdef __UCLIBC__
		#ifndef G_NOT_SMALL
			#define G_SMALL
		#endif
	#endif

	/* Autoconf, part 2
	 */
	#if defined( HAVE_CONFIG_H )
		#if ! HAVE_GMTIME_R
			#ifdef __cplusplus
				inline std::tm * gmtime_r( const std::time_t * tp , std::tm * tm_p ) 
				{
					* tm_p = * std::gmtime( tp ) ;
					return tm_p ;
				}
			#endif
		#endif
		#if ! HAVE_LOCALTIME_R
			#ifdef __cplusplus
				inline std::tm * localtime_r( const std::time_t * tp , std::tm * tm_p ) 
				{
					* tm_p = * std::localtime( tp ) ;
					return tm_p ;
				}
			#endif
		#endif
		#if HAVE_SETGROUPS
			#include <grp.h>
		#else
			#ifdef __cplusplus
				inline int setgroups( size_t , const gid_t * )
				{
					return 0 ;
				}
			#endif
		#endif
	#endif

#endif

