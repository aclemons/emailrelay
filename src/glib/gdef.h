/*
   Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
 * files. It takes care of some portability issues, and
 * is a good candidate for precompilation. It requires
 * either G_UNIX or G_WIN32 to be defined on the compiler
 * command line, although G_UNIX may also be inferred from
 * autoconf's HAVE_CONFIG_H.
 */

#ifndef G_DEF_H
#define G_DEF_H

	/* Autoconf responses
	 */
	#if defined(HAVE_CONFIG_H)

		#if ! defined(G_UNIX)
			#define G_UNIX
		#endif

		#include <config.h>

		#if HAVE_BUGGY_CTIME
			#include <time.h>
			#ifdef __cplusplus
				#include <ctime>
			#endif
		#endif

		#if ! HAVE_GETPWNAM_R
			#ifdef __cplusplus
				#include <sys/types.h>
				#include <pwd.h>
			#endif
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
	#if !defined(G_WIN32) && !defined(G_UNIX)
		#error invalid compilation switches - define G_WIN32 or G_UNIX
	#endif
	#if defined(G_WIN32) && defined(G_UNIX)
		#error invalid compilation switches - define G_WIN32 or G_UNIX
	#endif
	#if defined(__MINGW32__) && !defined(G_MINGW)
		#define G_MINGW 1
	#endif

	/* Define supplementary o/s compilation switches
	 */
	#if defined(G_WIN32) && ! defined(G_WINDOWS)
		#define G_WINDOWS
	#endif

	/* Define the compiler
	 */
	#if defined(_MSC_VER)
		#define G_COMPILER_IS_MICROSOFT 1
		#if (_MSC_VER <= 1200)
			#define G_COMPILER_IS_OLD 1
		#endif
	#endif
	#if defined(__GNUC__)
		#define G_COMPILER_IS_GNU 1
	#endif

	/* Define extra microsoft header tweaks
	 */
	#ifdef G_WIN32_IE
		#ifndef _WIN32_IE
			#define _WIN32_IE 0x600
		#endif
	#endif

	/* Include main o/s headers
	 */
	#if defined(G_WINDOWS)
		#if defined(G_MINGW)
			#define __USE_W32_SOCKETS
		#endif
		#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
		#endif
		#include <windows.h>
		#include <winsock2.h>
		#include <shellapi.h>
		#include <direct.h>
		#if defined(G_MINGW)
			#include <sys/stat.h>
			#include <unistd.h>
			#include <stdlib.h>
		#endif
	#else
		#include <unistd.h>
		#include <sys/stat.h>
		#include <sys/types.h>
		#if ! defined(G_MINGW)
			#include <sys/wait.h>
		#endif
		#if ! defined(HAVE_CONFIG_H)
			#include <grp.h>
		#endif
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
		#include <ctime>
	#else
		#include <stddef.h>
	#endif

	/* Define Windows-style types under unix
	 */
	#if ! defined(G_WINDOWS)
		typedef unsigned char BOOL ;
		typedef unsigned int HWND ;
		typedef unsigned int HINSTANCE ;
		typedef unsigned int HANDLE ;
		typedef wchar_t TCHAR ;
	#endif

	/* Define fixed-size types
	 */
	#if defined(G_WINDOWS)
		#if defined(G_COMPILER_IS_OLD)
			typedef unsigned int g_uint32_t ;
			typedef unsigned short g_uint16_t ;
			typedef int g_int32_t ;
			typedef short g_int16_t ;
		#else
			typedef UINT32 g_uint32_t ;
			typedef UINT16 g_uint16_t ;
			typedef INT32 g_int32_t ;
			typedef INT16 g_int16_t ;
		#endif
	#else
		#include <stdint.h>
		typedef uint32_t g_uint32_t ;
		typedef uint16_t g_uint16_t ;
		typedef int32_t g_int32_t ;
		typedef int16_t g_int16_t ;
	#endif
	#if __cplusplus
		typedef char assert_sizeof_uint16_is_2[sizeof(g_uint16_t)==2U?1:-1] ;
		typedef char assert_sizeof_uint32_is_4[sizeof(g_uint32_t)==4U?1:-1] ;
	#endif

	/* Define missing standard types
	 */
	#if defined(G_WINDOWS)
		typedef int uid_t ;
		typedef int gid_t ;
		#if defined(G_MINGW) 
			typedef int errno_t ;
		#endif
		#if defined(G_COMPILER_IS_MICROSOFT)
			#if defined(G_COMPILER_IS_OLD)
				typedef int errno_t ;
				typedef long LONG_PTR ;
				typedef char assert_sizeof_long_ptr_is_ok[sizeof(LONG_PTR)==sizeof(void*)?1:-1] ;
			#endif
			typedef SSIZE_T ssize_t ;
			typedef unsigned int pid_t ;
		#endif
	#endif

	/* Pull some std types into the global namespace
	 */
	#ifdef __cplusplus
		#if !defined(G_COMPILER_IS_OLD)
			using std::size_t ;
		#endif
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

	/* Pull stuff into the std namespace
	 */
	#ifdef __cplusplus
		#if defined(G_COMPILER_IS_MICROSOFT)
		namespace std
		{
			using ::abort ;
			#if defined(G_COMPILER_IS_OLD)
				using ::size_t ;
				using ::tm ;
				using ::time_t ;
				using ::time ;
				using ::localtime ;
				using ::gmtime ;
				using ::strftime ;
				using ::mktime ;
				using ::strchr ;
				using ::strrchr ;
				using ::strlen ;
				using ::strcpy ;
				using ::strncpy ;
				using ::strstr ;
				using ::strspn ;
				using ::system ;
				using ::remove ;
				using ::rename ;
				using ::rand ;
				using ::srand ;
				using ::getenv ;
				using ::printf ;
				using ::putchar ;
				using ::fputs ;
				using ::malloc ;
				using ::exit ;
			#endif
		}
		#endif
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

	/* Macros to explicitly ignore unused values.
	 */
	#define G_IGNORE_RETURN(type) (void)
	#define G_IGNORE_PARAMETER(type,name) (void)name
	#define G_IGNORE_VARIABLE(name) name=name

	/* Inline definitions of missing functions
	 */

	#if defined(HAVE_CONFIG_H)
		#if ! HAVE_GETPWNAM_R
			#define G_DEF_GETPWNAM_R_INLINE 1
		#endif
		#if ! HAVE_GMTIME_R
			#define G_DEF_GMTIME_R_INLINE 1
		#endif
		#if ! HAVE_LOCALTIME_R
			#define G_DEF_LOCALTIME_R_INLINE 1
		#endif
		#if HAVE_SETGROUPS
			#include <grp.h>
		#else
			#define G_DEF_SETGROUPS_INLINE 1
		#endif
	#else
		#if defined(G_MINGW)
			#define G_DEF_GMTIME_R_INLINE 1
			#define G_DEF_LOCALTIME_R_INLINE 1
			#define G_DEF_LOCALTIME_S_INLINE 1
			#define G_DEF_GMTIME_S_INLINE 1
			#define G_DEF_GETENV_S_INLINE 1
		#endif
		#if defined(G_COMPILER_IS_OLD)
			#define G_DEF_LOCALTIME_S_INLINE 1
			#define G_DEF_GMTIME_S_INLINE 1
			#define G_DEF_GETENV_S_INLINE 1
			#define G_DEF_GET_WINDOW_LONG_PTR_INLINE 1
		#endif
	#endif

	#if defined(G_DEF_GETPWNAM_R_INLINE)
		#ifdef __cplusplus
			inline int getpwnam_r( const char * name , struct passwd * pwd ,
				char * buf , size_t buflen , struct passwd ** result )
			{
				struct passwd * p = ::getpwnam( name ) ;
				if( p ) 
				{
					*pwd = *p ; /* let the string pointers dangle into the library storage */
					*result = pwd ;
					return 0 ;
				}
				else
				{
					*result = NULL ;
					return 0 ; /* or errno */
				}
			}
			#endif
	#endif

	#if defined(G_DEF_GMTIME_R_INLINE)
		#ifdef __cplusplus
			inline struct std::tm * gmtime_r( const std::time_t * tp , struct std::tm * tm_p ) 
			{
				const struct std::tm * p = std::gmtime( tp ) ;
				if( p == 0 ) return 0 ;
				*tm_p = *p ;
				return tm_p ;
			}
		#endif
	#endif

	#if defined(G_DEF_LOCALTIME_R_INLINE)
		#ifdef __cplusplus
			inline struct std::tm * localtime_r( const std::time_t * tp , struct std::tm * tm_p ) 
			{
				const struct std::tm * p = std::localtime( tp ) ;
				if( p == 0 ) return 0 ;
				*tm_p = *p ;
				return tm_p ;
			}
		#endif
	#endif

	#if defined(G_DEF_LOCALTIME_S_INLINE)
		#ifdef __cplusplus
			inline int localtime_s( struct std::tm * tm_p , const std::time_t * tp ) 
			{
				const errno_t e_inval = 22 ;
				if( tm_p == NULL ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon = 
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == NULL || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::localtime( tp ) ;
				if( p == 0 ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif
	#endif

	#if defined(G_DEF_GMTIME_S_INLINE)
		#ifdef __cplusplus
			inline int gmtime_s( struct std::tm * tm_p , const std::time_t * tp ) 
			{
				const errno_t e_inval = 22 ;
				if( tm_p == NULL ) return e_inval ;
				tm_p->tm_sec = tm_p->tm_min = tm_p->tm_hour = tm_p->tm_mday = tm_p->tm_mon = 
					tm_p->tm_year = tm_p->tm_wday = tm_p->tm_yday = tm_p->tm_isdst = -1 ;
				if( tp == NULL || *tp < 0 ) return e_inval ;
				const struct std::tm * p = std::gmtime( tp ) ;
				if( p == 0 ) return e_inval ;
				*tm_p = *p ;
				return 0 ;
			}
		#endif
	#endif

	#if defined(G_DEF_SETGROUPS_INLINE)
		#ifdef __cplusplus
			inline int setgroups( size_t , const gid_t * )
			{
				return 0 ;
			}
		#endif
	#endif

	#if defined(G_DEF_GETENV_S_INLINE)
		#ifdef __cplusplus
			inline errno_t getenv_s( size_t * n_out , char * buffer , size_t n_in , const char * name )
			{
				const errno_t e_inval = 22 ;
				const errno_t e_range = 34 ;
				if( n_out == NULL || name == NULL ) return e_inval ;
				const char * p = ::getenv( name ) ;
				*n_out = p ? (strlen(p) + 1U) : 0 ;
				if( p && *n_out > n_in ) return e_range ;
				if( p && buffer ) strcpy( buffer , p ) ;
				return 0 ;
			}

		#endif
	#endif

	#if defined(G_DEF_GET_WINDOW_LONG_PTR_INLINE)
		#ifdef __cplusplus
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

#endif

