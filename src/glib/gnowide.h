//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnowide.h
///
/// Contains inline functions that convert to and from UTF-8
/// strings in order to call wide-character "W()" or "_w()"
/// Windows functions internally.
///
/// This means that in the rest of the code filesystem paths,
/// registry paths, environment variables, command-lines etc.
/// can be always UTF-8, independent of the o/s, current locale
/// or codepage.
///
/// For temporary backwards compatibility, if G_ANSI is
/// undefined then the "A()" API functions are used with no
/// UTF-8 conversions.
///
/// \see http://utf8everywhere.org
///

#ifndef G_NOWIDE_H
#define G_NOWIDE_H

#ifdef G_WINDOWS

#include "gdef.h"
#include "gpath.h"
#include "gstringview.h"
#include "gcodepage.h"
#include "gconvert.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio> // std::remove()
#include <sddl.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <prsht.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <string.h> // ::strdup()

namespace G
{
	namespace nowide
	{
		#ifdef G_ANSI
			// (G_ANSI is deprecated)
			static constexpr bool w = false ;
			using FIND_DATA_type = WIN32_FIND_DATAA ;
			using WNDCLASS_type = WNDCLASSA ;
			using PROPSHEETPAGE_type = PROPSHEETPAGEA ;
			using PROPSHEETHEADER_type = PROPSHEETHEADERA ;
			using NOTIFYICONDATA_type = NOTIFYICONDATAA ;
			#if GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX
				using STARTUPINFO_BASE_type = STARTUPINFOA ;
				using STARTUPINFO_REAL_type = STARTUPINFOEXA ;
				constexpr DWORD STARTUPINFO_flags = EXTENDED_STARTUPINFO_PRESENT ;
			#else
				using STARTUPINFO_BASE_type = STARTUPINFOA ;
				using STARTUPINFO_REAL_type = STARTUPINFOA ;
				constexpr DWORD STARTUPINFO_flags = 0 ;
			#endif
			using IShellLink_type = IShellLinkA ;
			using ADDRINFO_type = ADDRINFOW ;
		#else
			static constexpr bool w = true ;
			using FIND_DATA_type = WIN32_FIND_DATAW ;
			using WNDCLASS_type = WNDCLASSW ;
			using PROPSHEETPAGE_type = PROPSHEETPAGEW ;
			using PROPSHEETHEADER_type = PROPSHEETHEADERW ;
			using NOTIFYICONDATA_type = NOTIFYICONDATAW ;
			#if GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX
				using STARTUPINFO_BASE_type = STARTUPINFOW ;
				using STARTUPINFO_REAL_type = STARTUPINFOEXW ;
				constexpr DWORD STARTUPINFO_flags = EXTENDED_STARTUPINFO_PRESENT ;
			#else
				using STARTUPINFO_BASE_type = STARTUPINFOW ;
				using STARTUPINFO_REAL_type = STARTUPINFOW ;
				constexpr DWORD STARTUPINFO_flags = 0 ;
			#endif
			using IShellLink_type = IShellLinkW ;
			using ADDRINFO_type = ADDRINFOA ;
		#endif

		inline std::string getCommandLine()
		{
			if( w )
				return Convert::narrow( GetCommandLineW() ) ;
			else
				return std::string( GetCommandLineA() ) ;
		}
		inline DWORD getFileAttributes( const Path & path )
		{
			if( w )
				return GetFileAttributesW( Convert::widen(path.str()).c_str() ) ;
			else
				return GetFileAttributesA( path.cstr() ) ;
		}
		inline HANDLE findFirstFile( const Path & path , WIN32_FIND_DATAW * find_data_p )
		{
			return FindFirstFileW( Convert::widen(path.str()).c_str() , find_data_p ) ;
		}
		inline HANDLE findFirstFile( const Path & path , WIN32_FIND_DATAA * find_data_p )
		{
			return FindFirstFileA( path.cstr() , find_data_p ) ;
		}
		inline BOOL findNextFile( HANDLE h , WIN32_FIND_DATAW * find_data_p )
		{
			return FindNextFileW( h , find_data_p ) ;
		}
		inline BOOL findNextFile( HANDLE h , WIN32_FIND_DATAA * find_data_p )
		{
			return FindNextFileA( h , find_data_p ) ;
		}
		inline std::string cFileName( const WIN32_FIND_DATAW & find_data )
		{
			return Convert::narrow( find_data.cFileName ) ;
		}
		inline std::string cFileName( const WIN32_FIND_DATAA & find_data )
		{
			return std::string( find_data.cFileName ) ;
		}
		template <typename T>
		void open( T & io , const Path & path , std::ios_base::openmode mode )
		{
			#if GCONFIG_HAVE_EXTENDED_OPEN
				// msvc and _wfsopen() :-)
				if( w )
					io.open( Convert::widen(path.str()).c_str() , mode , _SH_DENYNO ) ;
				else
					io.open( path.str() , mode , _SH_DENYNO ) ;
			#else
				// mingw :-(
				if( w )
					io.open( Convert::widen(path.str()).c_str() , mode ) ;
				else
					io.open( path.str() , mode ) ;
			#endif
		}
		inline int open( const Path & path , int flags , int pmode , bool inherit = false )
		{
			if( !inherit )
				flags |= _O_NOINHERIT ;
			_set_errno( 0 ) ; // mingw bug
			int fd = -1 ;
			if( w )
			{
				#if GCONFIG_HAVE_WSOPEN_S
					return 0 == _wsopen_s( &fd , Convert::widen(path.str()).c_str() , flags , _SH_DENYNO , pmode ) ? fd : -1 ;
				#else
					return _wopen( Convert::widen(path.str()).c_str() , flags , pmode ) ;
				#endif
			}
			else
			{
				return 0 == _sopen_s( &fd , path.cstr() , flags , _SH_DENYNO , pmode ) ? fd : -1 ;
			}
		}
		inline std::FILE * fopen( const Path & path , const char * mode )
		{
			if( w )
				return _wfsopen( Convert::widen(path.str()).c_str() , Convert::widen(mode).c_str() , _SH_DENYNO ) ;
			else
				return _fsopen( path.cstr() , mode , _SH_DENYNO ) ;
		}
		inline bool rename( const Path & from , const Path & to )
		{
			if( w )
				return 0 == _wrename( Convert::widen(from.str()).c_str() , Convert::widen(to.str()).c_str() ) ;
			else
				return 0 == std::rename( from.cstr() , to.cstr() ) ;
		}
		inline bool remove( const Path & path )
		{
			if( w )
				return 0 == _wremove( Convert::widen(path.str()).c_str() ) ;
			else
				return 0 == std::remove( path.cstr() ) ;
		}
		inline bool rmdir( const Path & path )
		{
			if( w )
				return 0 == _wrmdir( Convert::widen(path.str()).c_str() ) ;
			else
				return 0 == _rmdir( path.cstr() ) ;
		}
		inline int mkdir( const Path & dir )
		{
			if( w )
				return _wmkdir( Convert::widen(dir.str()).c_str() ) ;
			else
				return _mkdir( dir.cstr() ) ;
		}
		typedef struct _stat64 statbuf_type ;
		inline int stat( const Path & path , statbuf_type * statbuf_p )
		{
			if( w )
				return _wstat64( Convert::widen(path.str()).c_str() , statbuf_p ) ;
			else
				return _stat64( path.cstr() , statbuf_p ) ;
		}
		inline std::string getComputerNameEx( COMPUTER_NAME_FORMAT name_type = ComputerNameNetBIOS )
		{
			if( w )
			{
				DWORD size = 0 ;
				std::vector<wchar_t> buffer ;
				if( !GetComputerNameExW( name_type , NULL , &size ) && GetLastError() == ERROR_MORE_DATA && size )
					buffer.resize( static_cast<std::size_t>(size) ) ;
				else
					return {} ;
				if( !GetComputerNameExW( name_type , buffer.data() , &size ) ||
					(static_cast<std::size_t>(size)+1U) != buffer.size() )
						return {} ;
				return Convert::narrow( buffer.data() , buffer.size()-1U ) ;
			}
			else
			{
				DWORD size = 0 ;
				std::vector<char> buffer ;
				if( !GetComputerNameExA( name_type , NULL , &size ) && GetLastError() == ERROR_MORE_DATA && size )
					buffer.resize( static_cast<std::size_t>(size) ) ;
				else
					return {} ;
				if( !GetComputerNameExA( name_type , buffer.data() , &size ) ||
					(static_cast<std::size_t>(size)+1U) != buffer.size() )
						return {} ;
				return std::string( buffer.data() , buffer.size()-1U ) ;
			}
		}
		inline BOOL lookupAccountName( const std::string & full_name , char * sid_buffer , DWORD * sidsize_p ,
			bool with_domain , DWORD * domainsize_p , SID_NAME_USE * type_p )
		{
			if( w )
			{
				std::vector<wchar_t> domainbuffer( std::max(DWORD(1),*domainsize_p) ) ;
				return LookupAccountNameW( NULL , Convert::widen(full_name).c_str() , sid_buffer , sidsize_p ,
					with_domain ? domainbuffer.data() : nullptr , domainsize_p , type_p ) ;
			}
			else
			{
				std::vector<char> domainbuffer( std::max(DWORD(1),*domainsize_p) ) ;
				return LookupAccountNameA( NULL , full_name.c_str() , sid_buffer , sidsize_p ,
					with_domain ? domainbuffer.data() : nullptr , domainsize_p , type_p ) ;
			}
		}
		inline BOOL lookupAccountSid( PSID sid , std::string * name_out_p ,
			bool with_name , DWORD * namesize_p ,
			bool with_domain , DWORD * domainsize_p , SID_NAME_USE * type_p )
		{
			if( w )
			{
				std::vector<wchar_t> namebuffer( std::max(DWORD(1),*namesize_p) ) ;
				std::vector<wchar_t> domainbuffer( std::max(DWORD(1),*domainsize_p) ) ;
				BOOL rc = LookupAccountSidW( NULL , sid ,
					with_name ? namebuffer.data() : nullptr , namesize_p ,
					with_domain ? domainbuffer.data() : nullptr , domainsize_p ,
					type_p ) ;
				if( with_name && name_out_p )
					*name_out_p = Convert::narrow( namebuffer.data() , namebuffer.size() ) ;
				return rc ;
			}
			else
			{
				std::vector<char> namebuffer( std::max(DWORD(1),*namesize_p) ) ;
				std::vector<char> domainbuffer( std::max(DWORD(1),*domainsize_p) ) ;
				BOOL rc = LookupAccountSidA( NULL , sid ,
					with_name ? namebuffer.data() : nullptr , namesize_p ,
					with_domain ? domainbuffer.data() : nullptr , domainsize_p ,
					type_p ) ;
				if( with_name && name_out_p )
					*name_out_p = std::string( namebuffer.data() , namebuffer.size() ) ;
				return rc ;
			}
		}
		inline std::string convertSidToStringSid( PSID sid_p )
		{
			if( w )
			{
				wchar_t * str_p = nullptr ;
				if( !ConvertSidToStringSidW( sid_p , &str_p ) || str_p == nullptr )
					return {} ;
				std::wstring s( str_p ) ;
				LocalFree( str_p ) ;
				return Convert::narrow( s ) ;
			}
			else
			{
				char * str_p = nullptr ;
				if( !ConvertSidToStringSidA( sid_p , &str_p ) || str_p == nullptr )
					return {} ;
				std::string s( str_p ) ;
				LocalFree( str_p ) ;
				return s ;
			}
		}
		inline unsigned int getTextMetricsHeight( HDC hdc ) noexcept
		{
			if( w )
			{
        		TEXTMETRICW tm ;
        		GetTextMetricsW( hdc , &tm ) ;
        		return static_cast<unsigned int>( tm.tmHeight + tm.tmExternalLeading ) ;
			}
			else
			{
        		TEXTMETRICA tm ;
        		GetTextMetricsA( hdc , &tm ) ;
        		return static_cast<unsigned int>( tm.tmHeight + tm.tmExternalLeading ) ;
			}
		}
		inline HPROPSHEETPAGE createPropertySheetPage( PROPSHEETPAGEW * page , const std::string & title , int dialog_id )
		{
			std::wstring wtitle = G::Convert::widen( title ) ;
			page->pszTitle = wtitle.c_str() ;
			page->pszTemplate = dialog_id ? MAKEINTRESOURCEW(dialog_id) : 0 ;
			return CreatePropertySheetPageW( page ) ;
		}
		inline HPROPSHEETPAGE createPropertySheetPage( PROPSHEETPAGEA * page , const std::string & title , int dialog_id ) noexcept
		{
			page->pszTitle = title.c_str() ;
			page->pszTemplate = dialog_id ? MAKEINTRESOURCEA(dialog_id) : 0 ;
			return CreatePropertySheetPageA( page ) ;
		}
		inline INT_PTR propertySheet( PROPSHEETHEADERW * header , const std::string & title , int icon_id )
		{
			std::wstring wtitle = G::Convert::widen( title ) ;
			header->pszIcon = icon_id ? MAKEINTRESOURCEW(icon_id) : 0 ;
			header->pszCaption = wtitle.c_str() ;
			return PropertySheetW( header ) ;
		}
		inline INT_PTR propertySheet( PROPSHEETHEADERA * header , const std::string & title , int icon_id ) noexcept
		{
			header->pszIcon = icon_id ? MAKEINTRESOURCEA(icon_id) : 0 ;
			header->pszCaption = title.c_str() ;
			return PropertySheetA( header ) ;
		}
		inline void reportEvent( HANDLE h , DWORD id , WORD type , const char * message )
		{
			if( w )
			{
				std::wstring wmessage = Convert::widen( message ) ;
				const wchar_t * p [] = { wmessage.c_str() , nullptr } ;
				ReportEventW( h , type , 0 , id , nullptr , 1 , 0 , p , nullptr ) ;
			}
			else
			{
				const char * p [] = { message , nullptr } ;
				ReportEventA( h , type , 0 , id , nullptr , 1 , 0 , p , nullptr ) ;
			}
		}
		inline HANDLE registerEventSource( const std::string & name )
		{
			if( w )
				return RegisterEventSourceW( nullptr , Convert::widen(name).c_str() ) ;
			else
				return RegisterEventSourceA( nullptr , name.c_str() ) ;
		}
		inline LSTATUS regCreateKey( const Path & reg_path , HKEY * key_out_p ,
			HKEY hkey_in = HKEY_LOCAL_MACHINE , bool * is_new_p = nullptr )
		{
			LSTATUS result = 0 ;
			DWORD disposition = 0 ;
			if( w )
				result = RegCreateKeyExW( hkey_in , Convert::widen(reg_path.str()).c_str() ,
					0 , NULL , 0 , KEY_ALL_ACCESS , NULL , key_out_p , &disposition ) ;
			else
				result = RegCreateKeyExA( hkey_in , reg_path.cstr() ,
					0 , NULL , 0 , KEY_ALL_ACCESS , NULL , key_out_p , &disposition ) ;
			if( is_new_p )
				*is_new_p = disposition == REG_CREATED_NEW_KEY ;
			return result ;
		}
		inline LSTATUS regOpenKey( HKEY key_in , const Path & sub , HKEY * key_out_p , bool read_only = false )
		{
			REGSAM access = read_only ? (STANDARD_RIGHTS_READ|KEY_QUERY_VALUE) : KEY_ALL_ACCESS ;
			if( w )
				return RegOpenKeyExW( key_in , Convert::widen(sub.str()).c_str() ,
					0 , access , key_out_p ) ;
			else
				return RegOpenKeyExA( key_in , sub.cstr() ,
					0 , access , key_out_p ) ;
		}
		inline LSTATUS regDeleteKey( HKEY key , const Path & sub )
		{
			if( w )
				return RegDeleteKeyW( key , Convert::widen(sub.str()).c_str() ) ;
			else
				return RegDeleteKeyA( key , sub.cstr() ) ;
		}
		inline LSTATUS regQueryValueType( HKEY key , const Path & sub , DWORD * type_out_p , DWORD * size_out_p )
		{
			if( w )
				return RegQueryValueExW( key , Convert::widen(sub.str()).c_str() ,
					0 , type_out_p , nullptr , size_out_p ) ;
			else
				return RegQueryValueExA( key , sub.cstr() ,
					0 , type_out_p , nullptr , size_out_p ) ;
		}
		inline LSTATUS regGetValueString( HKEY key , const Path & sub , std::string * value_out_p )
		{
			DWORD type = 0 ;
			LSTATUS status = 0 ;
			DWORD size = 0 ;
			if( w )
				status = RegQueryValueExW( key , Convert::widen(sub.str()).c_str() , 0 , &type , nullptr , &size ) ;
			else
				status = RegQueryValueExA( key , sub.cstr() , 0 , &type , nullptr , &size ) ;
			if( type != REG_SZ )
				return ERROR_INVALID_DATA ;
			if( status != ERROR_SUCCESS )
				return status ;
			if( w )
			{
				std::vector<wchar_t> buffer( static_cast<std::size_t>(size)+1U , '\0' ) ;
				status = RegQueryValueExW( key , Convert::widen(sub.str()).c_str() , 0 , &type ,
					reinterpret_cast<BYTE*>(buffer.data()) , &size ) ;
				if( status != ERROR_SUCCESS )
					return status ;
				buffer[std::min(buffer.size()-1U,static_cast<std::size_t>(size))] = L'\0' ;
				*value_out_p = Convert::narrow( buffer.data() ) ;
			}
			else
			{
				std::vector<char> buffer( static_cast<std::size_t>(size)+1U , '\0' ) ;
				status = RegQueryValueExA( key , sub.cstr() , 0 , &type ,
					reinterpret_cast<BYTE*>(buffer.data()) , &size ) ;
				if( status != ERROR_SUCCESS )
					return status ;
				buffer[std::min(buffer.size()-1U,static_cast<std::size_t>(size))] = '\0' ;
				*value_out_p = std::string( buffer.data() ) ;
			}
			return status ;
		}
		inline LSTATUS regGetValueNumber( HKEY key , const Path & sub , DWORD * value_out_p )
		{
			DWORD type = 0 ;
			LSTATUS status = 0 ;
			DWORD size = 4 ;
			if( w )
				status = RegQueryValueExW( key , Convert::widen(sub.str()).c_str() , 0 ,
					&type , reinterpret_cast<BYTE*>(value_out_p) , &size ) ;
			else
				status = RegQueryValueExA( key , sub.cstr() , 0 ,
					&type , reinterpret_cast<BYTE*>(value_out_p) , &size ) ;
			if( type != REG_DWORD || size < 4U )
				return ERROR_INVALID_DATA ;
			if( status != ERROR_SUCCESS )
				return status ;
			return status ;
		}
		inline LSTATUS regSetValue( HKEY key , const Path & sub , const std::string & s )
		{
			if( w )
			{
				std::wstring ws = Convert::widen( s ) ;
				const BYTE * p = reinterpret_cast<const BYTE*>(ws.c_str()) ;
				DWORD n = static_cast<DWORD>( (s.size()*2U)+1U ) ;
				return RegSetValueExW( key , Convert::widen(sub.str()).c_str() , 0 , REG_SZ , p , n ) ;
			}
			else
			{
				DWORD n = static_cast<DWORD>( s.size()+1U ) ;
				const BYTE * p = reinterpret_cast<const BYTE*>(s.c_str()) ;
				return RegSetValueExA( key , sub.cstr() , 0 , REG_SZ , p , n ) ;
			}
		}
		inline LSTATUS regSetValue( HKEY key , const Path & sub , DWORD n )
		{
			if( w )
				return RegSetValueExW( key , Convert::widen(sub.str()).c_str() , 0 , REG_DWORD ,
					reinterpret_cast<const BYTE*>(&n) , sizeof(DWORD) ) ;
			else
				return RegSetValueExA( key , sub.cstr() , 0 , REG_DWORD ,
					reinterpret_cast<const BYTE*>(&n) , sizeof(DWORD) ) ;
		}
		inline BOOL createProcess( std::string_view exe , std::string_view command_line ,
			const char * /*env_char_block_p*/ , const wchar_t * env_wchar_block_p ,
			const Path * cd_path_p , DWORD startup_info_flags ,
			STARTUPINFOW * startup_info , PROCESS_INFORMATION * info_ptr , bool inherit = true )
		{
			return CreateProcessW( exe.empty() ? nullptr : Convert::widen(exe).c_str() ,
				const_cast<wchar_t*>(Convert::widen(command_line).c_str()) ,
				nullptr , nullptr ,
				inherit , startup_info_flags|CREATE_UNICODE_ENVIRONMENT ,
				const_cast<wchar_t*>(env_wchar_block_p) ,
				cd_path_p ? Convert::widen(cd_path_p->str()).c_str() : nullptr ,
				startup_info , info_ptr ) ;
		}
		inline BOOL createProcess( const std::string & exe , const std::string & command_line ,
			const char * env_char_block_p , const wchar_t * /*env_wchar_block_p*/ ,
			const Path * cd_path_p , DWORD startup_info_flags ,
			STARTUPINFOA * startup_info , PROCESS_INFORMATION * info_ptr , bool inherit = true )
		{
			return CreateProcessA( exe.empty() ? nullptr : exe.c_str() ,
				const_cast<char*>(command_line.c_str()) ,
				nullptr , nullptr ,
				inherit , startup_info_flags ,
				const_cast<char*>(env_char_block_p) ,
				cd_path_p ? cd_path_p->cstr() : nullptr ,
				startup_info , info_ptr ) ;
		}
		inline std::string windowsPath()
		{
			if( w )
			{
				std::vector<wchar_t> buffer( MAX_PATH+1 ) ;
				buffer[0] = 0 ;
				unsigned int n = GetWindowsDirectoryW( buffer.data() , MAX_PATH ) ;
				if( n == 0 || n > MAX_PATH )
					return {} ;
				return Convert::narrow( buffer.data() , static_cast<std::size_t>(n) ) ;
			}
			else
			{
				std::vector<char> buffer( MAX_PATH+1 ) ;
				buffer[0] = 0 ;
				unsigned int n = GetWindowsDirectoryA( buffer.data() , MAX_PATH ) ;
				if( n == 0 || n > MAX_PATH )
					return {} ;
				return std::string( buffer.data() , static_cast<std::size_t>(n) ) ;
			}
		}
		inline std::string strerror( int errno_ )
		{
			if( w )
			{
				#if GCONFIG_HAVE_WCSERROR_S
					std::vector<wchar_t> buffer( 80U , '\0' ) ;
					if( _wcserror_s( buffer.data() , buffer.size()-1U , errno_ ) || buffer.at(0U) == L'\0' )
						return std::string("unknown error (").append(std::to_string(errno_)).append(1U,')') ;
					return Convert::narrow( buffer.data() ) ;
				#else
					std::vector<char> buffer( 80U , '\0' ) ;
					if( strerror_s( buffer.data() , buffer.size()-1U , errno_ ) || buffer.at(0U) == '\0' )
						return std::string("unknown error (").append(std::to_string(errno_)).append(1U,')') ;
					return CodePage::fromCodePageAnsi( std::string(buffer.data()) ) ;
				#endif
			}
			else
			{
				std::vector<char> buffer( 80U , '\0' ) ;
				if( strerror_s( buffer.data() , buffer.size()-1U , errno_ ) || buffer.at(0U) == '\0' )
					return std::string("unknown error (").append(std::to_string(errno_)).append(1U,')') ;
				return {buffer.data()} ;
			}
		}
		inline std::string formatMessage( DWORD e )
		{
			if( w )
			{
				wchar_t * ptr = nullptr ;
				FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS ,
					NULL , e , MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT) ,
					reinterpret_cast<wchar_t*>(&ptr) , 1 , NULL ) ;
				std::string result = ptr ? Convert::narrow(ptr) : std::string() ;
				if( ptr ) LocalFree( ptr ) ;
				return result ;
			}
			else
			{
				char * ptr = nullptr ;
				FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS ,
					NULL , e , MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT) ,
					reinterpret_cast<char*>(&ptr) , 1 , NULL ) ;
				std::string result = ptr ? std::string(ptr) : std::string() ;
				if( ptr ) LocalFree( ptr ) ;
				return result ;
			}
		}
		inline G::Path exe()
		{
			if( w )
			{
				std::vector<wchar_t> buffer ;
				std::size_t sizes[] = { 80U , 1024U , 32768U , 0U } ; // documented limit of 32k
				for( std::size_t * size_p = sizes ; *size_p ; ++size_p )
				{
					buffer.resize( *size_p+1U , '\0' ) ;
					DWORD buffer_size = static_cast<DWORD>( buffer.size() ) ;
					DWORD rc = GetModuleFileNameW( HNULL , buffer.data() , buffer_size ) ;
					if( rc == 0 ) break ;
					if( rc < buffer_size )
						return {Convert::narrow( std::wstring(buffer.data(),static_cast<std::size_t>(rc)) )} ;
				}
				return {} ;
			}
			else
			{
				std::vector<char> buffer ;
				std::size_t sizes[] = { 80U , 1024U , 32768U , 0U } ; // documented limit of 32k
				for( std::size_t * size_p = sizes ; *size_p ; ++size_p )
				{
					buffer.resize( *size_p+1U , '\0' ) ;
					DWORD buffer_size = static_cast<DWORD>( buffer.size() ) ;
					DWORD rc = GetModuleFileNameA( HNULL , buffer.data() , buffer_size ) ;
					if( rc == 0 ) break ;
					if( rc < buffer_size )
						return {std::string_view( buffer.data() , static_cast<std::size_t>(rc) )} ;
				}
				return {} ;
			}
		}
		inline G::Path cwd()
		{
			if( w )
			{
				wchar_t * p = _wgetcwd( nullptr , 2048 ) ; // "a buffer of at least .. is .. allocated" "more only if necessary"
				if( p == nullptr )
					return {} ;
				Path result( Convert::narrow(std::wstring(p)) ) ;
				std::free( p ) ;
				return result ;
			}
			else
			{
				char * p = _getcwd( nullptr , 2048 ) ; // "a buffer of at least .. is .. allocated" "more only if necessary"
				if( p == nullptr )
					return {} ;
				Path result( (std::string_view(p)) ) ;
				std::free( p ) ;
				return result ;
			}
		}
		inline std::string getenv( const std::string & name , const std::string & default_ )
		{
			if( w )
			{
				#if GCONFIG_HAVE_WGETENV_S
					std::size_t n = 0U ;
					_wgetenv_s( &n , nullptr , 0U , Convert::widen(name).c_str() ) ;
					if( n == 0U ) return default_ ; // ignore rc
					std::vector<wchar_t> buffer( n ) ;
					auto rc = _wgetenv_s( &n , buffer.data() , buffer.size() , Convert::widen(name).c_str() ) ;
					if( rc != 0 || n != buffer.size() ) return default_ ;
					return Convert::narrow( buffer.data() , buffer.size()-1U ) ;
				#else
					const wchar_t * p = _wgetenv( Convert::widen(name).c_str() ) ;
					if( p == nullptr ) return default_ ;
					return Convert::narrow( p , wcslen(p) ) ;
				#endif
			}
			else
			{
				#if GCONFIG_HAVE_GETENV_S
					std::size_t n = 0U ;
					getenv_s( &n , nullptr , 0U , name.c_str() ) ;
					if( n == 0U ) return default_ ; // ignore rc
					std::vector<char> buffer( n ) ;
					auto rc = getenv_s( &n , buffer.data() , buffer.size() , name.c_str() ) ;
					if( rc != 0 || n != buffer.size() ) return default_ ;
					return { buffer.data() , buffer.size()-1U } ;
				#else
					const char * p = ::getenv( name.c_str() ) ;
					if( p == nullptr ) return default_ ;
					return {p} ;
				#endif
			}
		}
		inline errno_t putenv( const std::string & key , const std::string & value )
		{
			if( w )
			{
				#if GCONFIG_HAVE_WPUTENV_S
					return _wputenv_s( Convert::widen(key).c_str() , Convert::widen(value).c_str() ) ;
				#else
					std::string s = CodePage::toCodePageAnsi( std::string(key).append(1U,'=').append(value) ) ;
					GDEF_WARNING_DISABLE_START( 4996 )
					_putenv( strdup(s.c_str()) ) ; // deliberate leak
					GDEF_WARNING_DISABLE_END
					return 0 ;
				#endif
			}
			else
			{
				return _putenv_s( key.c_str() , value.c_str() ) ;
			}
		}
		inline bool messageBox( HWND hparent , const std::string & message , const std::string & title , DWORD type )
		{
			if( w )
			{
				auto rc = MessageBoxW( hparent , Convert::widen(message).c_str() , Convert::widen(title).c_str() , type ) ;
				return rc == IDOK || rc == IDYES ;
			}
			else
			{
				auto rc = MessageBoxA( hparent , message.c_str() , title.c_str() , type ) ;
				return rc == IDOK || rc == IDYES ;
			}
		}
		inline bool setWindowText( HWND hwnd , const std::string & text )
		{
			if( w )
				return SetWindowTextW( hwnd , Convert::widen(text).c_str() ) ;
			else
				return SetWindowTextA( hwnd , text.c_str() ) ;
		}
		inline std::string getWindowText( HWND hwnd )
		{
			if( w )
			{
				int length = GetWindowTextLengthW( hwnd ) ;
				if( length <= 0 ) return {} ;
				std::vector<wchar_t> buffer( static_cast<std::size_t>(length)+2U ) ;
				GetWindowTextW( hwnd , buffer.data() , length+1 ) ;
				buffer[buffer.size()-1U] = L'\0' ;
				return Convert::narrow( buffer.data() ) ;
			}
			else
			{
				int length = GetWindowTextLengthA( hwnd ) ;
				if( length <= 0 ) return {} ;
				std::vector<char> buffer( static_cast<std::size_t>(length)+2U ) ;
				GetWindowTextA( hwnd , buffer.data() , length+1 ) ;
				buffer[buffer.size()-1U] = '\0' ;
				return {buffer.data()} ;
			}
		}
		inline int getWindowTextLength( HWND hwnd ) noexcept
		{
			if( w )
				return GetWindowTextLengthW( hwnd ) ;
			else
				return GetWindowTextLengthA( hwnd ) ;
		}
		inline HICON loadIcon( HINSTANCE hinstance , unsigned int icon_id ) noexcept
		{
			if( w )
				return LoadIconW( hinstance , MAKEINTRESOURCEW(icon_id) ) ;
			else
				return LoadIconA( hinstance , MAKEINTRESOURCEA(icon_id) ) ;
		}
		inline HICON loadIconApplication() noexcept
		{
			// IDI_APPLICATION uses MAKEINTRESOURCE, which depends on
			// _UNICODE, so use the default definitions for both
			// IDI_APPLICATION and LoadIcon() -- it doesn't matter
			// whether that's A() or W() in practice, as long as they match
			return LoadIcon( HNULL , IDI_APPLICATION ) ; // not A() or W()
		}
		inline HCURSOR loadCursor( HINSTANCE hinstance , int resource_id ) noexcept
		{
			if( w )
				return LoadCursorW( hinstance , MAKEINTRESOURCEW(resource_id) ) ;
			else
				return LoadCursorA( hinstance , MAKEINTRESOURCEA(resource_id) ) ;
		}
		inline HCURSOR loadCursorArrow() noexcept
		{
			return LoadCursor( HNULL , IDC_ARROW ) ; // not A() or W(), as above
		}
		inline HCURSOR loadCursorWait() noexcept
		{
			return LoadCursor( HNULL , IDC_WAIT ) ; // not A() or W(), as above
		}
		inline int shellNotifyIcon( HINSTANCE hinstance , DWORD , NOTIFYICONDATAW * data ,
			unsigned int icon_id , std::string_view tip )
		{
			// (modifies the caller's structure)
			data->hIcon = LoadIconW( hinstance , MAKEINTRESOURCEW(icon_id) ) ;
			if( data->hIcon == HNULL ) return 2 ;

			std::wstring wtip = Convert::widen( tip ) ;
			constexpr std::size_t n = sizeof(data->szTip) / sizeof(data->szTip[0]) ;
			static_assert( n > 0U , "" ) ;
			for( std::size_t i = 0U ; i < n ; i++ )
				data->szTip[i] = ( i < wtip.size() ? wtip[i] : L'\0' ) ;
			data->szTip[n-1U] = L'\0' ;

			return Shell_NotifyIconW( NIM_ADD , data ) ? 0 : 1 ;
		}
		inline int shellNotifyIcon( HINSTANCE hinstance , DWORD , NOTIFYICONDATAA * data ,
			unsigned int icon_id , std::string_view tip )
		{
			// (modifies the caller's structure)
			data->hIcon = LoadIconA( hinstance , MAKEINTRESOURCEA(icon_id) ) ;
			if( data->hIcon == HNULL ) return 2 ;

			constexpr std::size_t n = sizeof(data->szTip) / sizeof(data->szTip[0]) ;
			static_assert( n > 0U , "" ) ;
			for( std::size_t i = 0U ; i < n ; i++ )
				data->szTip[i] = ( i < tip.size() ? tip[i] : '\0' ) ;
			data->szTip[n-1U] = '\0' ;

			return Shell_NotifyIconA( NIM_ADD , data ) ? 0 : 1 ;
		}
		inline bool shellNotifyIcon( DWORD message , NOTIFYICONDATAW * data , std::nothrow_t ) noexcept
		{
			return Shell_NotifyIconW( message , data ) ;
		}
		inline bool shellNotifyIcon( DWORD message , NOTIFYICONDATAA * data , std::nothrow_t ) noexcept
		{
			return Shell_NotifyIconA( message , data ) ;
		}
		inline unsigned int dragQueryFile( HDROP hdrop ) noexcept
		{
			if( w )
				return DragQueryFileW( hdrop , 0xFFFFFFFF , nullptr , 0 ) ;
			else
				return DragQueryFileA( hdrop , 0xFFFFFFFF , nullptr , 0 ) ;
		}
		inline std::string dragQueryFile( HDROP hdrop , unsigned int i )
		{
			if( w )
			{
				unsigned int n = DragQueryFileW( hdrop , i , nullptr , 0U ) ;
				std::vector<wchar_t> buffer( std::size_t(n)+1U , L'\0' ) ;
				n = DragQueryFileW( hdrop , i , buffer.data() , n+1U ) ;
				return Convert::narrow( buffer.data() , std::min(std::size_t(n),buffer.size()) ) ;
			}
			else
			{
				unsigned int n = DragQueryFileA( hdrop , i , nullptr , 0U ) ;
				std::vector<char> buffer( std::size_t(n)+1U , '\0' ) ;
				n = DragQueryFileA( hdrop , i , buffer.data() , n+1U ) ;
				return { buffer.data() , std::min(std::size_t(n),buffer.size()) } ;
			}
		}
		inline INT_PTR dialogBoxParam( HINSTANCE hinstance , int resource_id , HWND parent , DLGPROC fn , LPARAM lparam )
		{
			if( w )
				return DialogBoxParamW( hinstance , MAKEINTRESOURCEW(resource_id) , parent , fn , lparam ) ;
			else
				return DialogBoxParamA( hinstance , MAKEINTRESOURCEA(resource_id) , parent , fn , lparam ) ;
		}
		inline INT_PTR dialogBoxParam( HINSTANCE hinstance , const std::string & resource , HWND parent , DLGPROC fn , LPARAM lparam )
		{
			if( w )
				return DialogBoxParamW( hinstance , Convert::widen(resource).c_str() , parent , fn , lparam ) ;
			else
				return DialogBoxParamA( hinstance , resource.c_str() , parent , fn , lparam ) ;
		}
		inline HWND createDialogParam( HINSTANCE hinstance , int resource_id , HWND parent , DLGPROC fn , LPARAM lparam )
		{
			if( w )
				return CreateDialogParamW( hinstance , MAKEINTRESOURCEW(resource_id) , parent , fn , lparam ) ;
			else
				return CreateDialogParamA( hinstance , MAKEINTRESOURCEA(resource_id) , parent , fn , lparam ) ;
		}
		inline HWND createDialogParam( HINSTANCE hinstance , const std::string & resource , HWND parent , DLGPROC fn , LPARAM lparam )
		{
			if( w )
				return CreateDialogParamW( hinstance , Convert::widen(resource).c_str() , parent , fn , lparam ) ;
			else
				return CreateDialogParamA( hinstance , resource.c_str() , parent , fn , lparam ) ;
		}
		inline void getClassInfo( HINSTANCE hinstance , std::string_view name , WNDCLASSW * info )
		{
			GetClassInfoW( hinstance , Convert::widen(name).c_str() , info ) ;
		}
		inline void getClassInfo( HINSTANCE hinstance , const std::string & name , WNDCLASSA * info )
		{
			GetClassInfoA( hinstance , name.c_str() , info ) ;
		}
		inline ATOM registerClass( WNDCLASSW info , std::string_view name , unsigned int menu_resource_id = 0U )
		{
			std::wstring wname = Convert::widen( name ) ;
			info.lpszClassName = wname.c_str() ;
			if( menu_resource_id )
				info.lpszMenuName = MAKEINTRESOURCEW(menu_resource_id) ;
			return RegisterClassW( &info ) ;
		}
		inline ATOM registerClass( WNDCLASSA info , const std::string & name , unsigned int menu_resource_id = 0U )
		{
			info.lpszClassName = name.c_str() ;
			if( menu_resource_id )
				info.lpszMenuName = MAKEINTRESOURCEA(menu_resource_id) ;
			return RegisterClassA( &info ) ;
		}
		inline std::string getClassName( HWND hwnd )
		{
			if( w )
			{
				std::vector<wchar_t> buffer( 257U ) ; // atom size limit
				buffer[0] = L'\0' ;
				GetClassNameW( hwnd , buffer.data() , static_cast<int>(buffer.size()-1U) ) ;
				buffer[buffer.size()-1U] = L'\0' ;
				return Convert::narrow( buffer.data() ) ;
			}
			else
			{
				std::vector<char> buffer( 257U ) ; // atom size limit
				buffer[0] = '\0' ;
				GetClassNameA( hwnd , buffer.data() , static_cast<int>(buffer.size()-1U) ) ;
				buffer[buffer.size()-1U] = '\0' ;
				return {buffer.data()} ;
			}
		}
		inline HWND createWindowEx( DWORD extended_style , const std::string & class_name ,
			const std::string & title , DWORD style , int x , int y , int dx , int dy ,
			HWND parent , HMENU menu , HINSTANCE hinstance , void * vp )
		{
			if( w )
				return CreateWindowExW( extended_style ,
					Convert::widen(class_name).c_str() , Convert::widen(title).c_str() ,
					style , x , y , dx , dy , parent , menu , hinstance , vp ) ;
			else
				return CreateWindowExA( extended_style ,
					class_name.c_str() , title.c_str() ,
					style , x , y , dx , dy , parent , menu , hinstance , vp ) ;
		}
		inline void check_hwnd( HWND hwnd )
		{
			if( w != !!IsWindowUnicode(hwnd) )
				throw std::runtime_error( "unicode window mismatch" ) ;
		}
		inline LRESULT callWindowProc( LONG_PTR fn , HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
		{
			check_hwnd( hwnd ) ;
			if( w )
				return CallWindowProcW( reinterpret_cast<WNDPROC>(fn) , hwnd , message , wparam , lparam ) ;
			else
				return CallWindowProcA( reinterpret_cast<WNDPROC>(fn) , hwnd , message , wparam , lparam ) ;

		}
		inline LRESULT defWindowProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
		{
			check_hwnd( hwnd ) ;
			if( w )
				return DefWindowProcW( hwnd , message , wparam , lparam ) ;
			else
				return DefWindowProcA( hwnd , message , wparam , lparam ) ;
		}
		inline LRESULT defDlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
		{
			check_hwnd( hwnd ) ;
			if( w )
				return DefDlgProcW( hwnd , message , wparam , lparam ) ;
			else
				return DefDlgProcA( hwnd , message , wparam , lparam ) ;
		}
		inline bool isDialogMessage( HWND hdlg , MSG * msg ) noexcept
		{
			if( w )
				return IsDialogMessageW( hdlg , msg ) ;
			else
				return IsDialogMessageA( hdlg , msg ) ;
		}
		inline bool winHelp( HWND hwnd , const G::Path & path , unsigned int id )
		{
			if( w )
				return WinHelpW( hwnd , G::Convert::widen(path.str()).c_str() , id , 0 ) ;
			else
				return WinHelpA( hwnd , path.cstr() , id , 0 ) ;
		}
		inline HMENU loadMenu( HINSTANCE hinstance , int id )
		{
			if( w )
				return LoadMenuW( hinstance , MAKEINTRESOURCEW(id) ) ;
			else
				return LoadMenuA( hinstance , MAKEINTRESOURCEA(id) ) ;
		}
		inline std::string getMenuString( HMENU hmenu , UINT id , UINT flags )
		{
			if( w )
			{
				int n = GetMenuStringW( hmenu , id , nullptr , 0 , flags ) ;
				if( n <= 0 )
					return {} ;
				std::vector<wchar_t> buffer( static_cast<std::size_t>(n)+1U ) ;
				n = GetMenuStringW( hmenu , id , buffer.data() , static_cast<int>(buffer.size()) , flags ) ;
				if( n <= 0 || static_cast<std::size_t>(n) != (buffer.size()-1U) )
					return {} ;
				return G::Convert::narrow( buffer.data() , buffer.size()-1U ) ;
			}
			else
			{
				int n = GetMenuStringA( hmenu , id , nullptr , 0 , flags ) ;
				if( n <= 0 )
					return {} ;
				std::vector<char> buffer( static_cast<std::size_t>(n)+1U ) ;
				n = GetMenuStringA( hmenu , id , buffer.data() , static_cast<int>(buffer.size()) , flags ) ;
				if( n <= 0 || static_cast<std::size_t>(n) != (buffer.size()-1U) )
					return {} ;
				return std::string( buffer.data() , buffer.size()-1U ) ;
			}
		}
		inline void insertMenuItem( HMENU hmenu , UINT id , const std::string & name )
		{
			if( w )
			{
				std::wstring wname = Convert::widen( name ) ;
				MENUITEMINFOW item {} ;
				item.cbSize = sizeof( item ) ;
				item.fMask = MIIM_STRING | MIIM_ID ; // setting dwTypeData & wID
				item.fType = MFT_STRING ;
				item.wID = id ;
				item.dwTypeData = const_cast<wchar_t*>( wname.c_str() ) ;
				item.cch = static_cast<UINT>( wname.size() ) ;
				InsertMenuItemW( hmenu , 0 , TRUE , &item ) ;
			}
			else
			{
				MENUITEMINFOA item {} ;
				item.cbSize = sizeof( item ) ;
				item.fMask = MIIM_STRING | MIIM_ID ; // setting dwTypeData & wID
				item.fType = MFT_STRING ;
				item.wID = id ;
				item.dwTypeData = const_cast<char*>( name.c_str() ) ;
				item.cch = static_cast<UINT>( name.size() ) ;
				InsertMenuItemA( hmenu , 0 , TRUE , &item ) ;
			}
		}
		inline SC_HANDLE openSCManagerW( DWORD access ) noexcept
		{
			return OpenSCManagerW( nullptr , nullptr , access ) ; // no machine or database name
		}
		inline SC_HANDLE openSCManager( DWORD access ) noexcept
		{
			if( w ) // fwiw
				return OpenSCManagerW( nullptr , nullptr , access ) ; // no machine or database name
			else
				return OpenSCManagerA( nullptr , nullptr , access ) ; // no machine or database name
		}
		inline BOOL startServiceW( SC_HANDLE hservice ) noexcept
		{
			return StartServiceW( hservice , 0 , nullptr ) ; // (no args)
		}
		inline BOOL startService( SC_HANDLE hservice ) noexcept
		{
			if( w )
				return StartServiceW( hservice , 0 , nullptr ) ;
			else
				return StartServiceA( hservice , 0 , nullptr ) ;
		}
    	using ServiceMainWFn = void (WINAPI *)( DWORD , wchar_t ** ) ;
    	using ServiceMainAFn = void (WINAPI *)( DWORD , char ** ) ;
		inline BOOL startServiceCtrlDispatcherW( ServiceMainWFn w_fn ) noexcept
		{
			SERVICE_TABLE_ENTRYW table [2] = { { const_cast<wchar_t*>(L"") , w_fn } , { nullptr , nullptr } } ;
			return StartServiceCtrlDispatcherW( table ) ;
		}
		inline BOOL startServiceCtrlDispatcher( ServiceMainWFn w_fn , ServiceMainAFn a_fn ) noexcept
		{
			if( w )
			{
				SERVICE_TABLE_ENTRYW table [2] = { { const_cast<wchar_t*>(L"") , w_fn } , { nullptr , nullptr } } ;
				return StartServiceCtrlDispatcherW( table ) ;
			}
			else
			{
				SERVICE_TABLE_ENTRYA table [2] = { { const_cast<char*>("") , a_fn } , { nullptr , nullptr } } ;
				return StartServiceCtrlDispatcherA( table ) ;
			}
		}
		inline SC_HANDLE openServiceW( SC_HANDLE hmanager , const std::string & name , DWORD flags )
		{
			return OpenServiceW( hmanager , Convert::widen(name).c_str() , flags ) ;
		}
		inline SC_HANDLE openService( SC_HANDLE hmanager , const std::string & name , DWORD flags )
		{
			if( w )
				return OpenServiceW( hmanager , Convert::widen(name).c_str() , flags ) ;
			else
				return OpenServiceA( hmanager , name.c_str() , flags ) ;
		}
		inline SC_HANDLE createServiceW( SC_HANDLE hmanager , const std::string & name ,
			const std::string & display_name , DWORD start_type , const std::string & commandline )
		{
			return CreateServiceW( hmanager ,
				Convert::widen(name).c_str() , Convert::widen(display_name).c_str() ,
				SERVICE_ALL_ACCESS , SERVICE_WIN32_OWN_PROCESS , start_type , SERVICE_ERROR_NORMAL ,
				Convert::widen(commandline).c_str() ,
				nullptr , nullptr , nullptr , nullptr , nullptr ) ;
		}
		inline SC_HANDLE createService( SC_HANDLE hmanager , const std::string & name ,
			const std::string & display_name , DWORD start_type , const std::string & commandline )
		{
			if( w )
				return CreateServiceW( hmanager ,
					Convert::widen(name).c_str() , Convert::widen(display_name).c_str() ,
					SERVICE_ALL_ACCESS , SERVICE_WIN32_OWN_PROCESS , start_type , SERVICE_ERROR_NORMAL ,
					Convert::widen(commandline).c_str() ,
					nullptr , nullptr , nullptr , nullptr , nullptr ) ;
			else
				return CreateServiceA( hmanager ,
					name.c_str() , display_name.c_str() ,
					SERVICE_ALL_ACCESS , SERVICE_WIN32_OWN_PROCESS , start_type , SERVICE_ERROR_NORMAL ,
					commandline.c_str() ,
					nullptr , nullptr , nullptr , nullptr , nullptr ) ;
		}
		inline SERVICE_STATUS_HANDLE registerServiceCtrlHandlerW( const std::string & service_name , void (WINAPI *handler_fn)(DWORD) )
		{
			return RegisterServiceCtrlHandlerW( Convert::widen(service_name).c_str() , handler_fn ) ;
		}
		inline SERVICE_STATUS_HANDLE registerServiceCtrlHandler( const std::string & service_name , void (WINAPI *handler_fn)(DWORD) )
		{
			if( w )
				return RegisterServiceCtrlHandlerW( Convert::widen(service_name).c_str() , handler_fn ) ;
			else
				return RegisterServiceCtrlHandlerA( service_name.c_str() , handler_fn ) ;
		}
		inline bool changeServiceConfigW( SC_HANDLE hmanager , const std::string & description )
		{
			std::wstring wdescription = Convert::widen( description ) ;
    		SERVICE_DESCRIPTIONW service_description {} ;
    		service_description.lpDescription = const_cast<wchar_t*>( wdescription.c_str() ) ;
    		return ChangeServiceConfig2W( hmanager , SERVICE_CONFIG_DESCRIPTION , &service_description ) ;
		}
		inline bool changeServiceConfig( SC_HANDLE hmanager , const std::string & description )
		{
			if( w )
			{
				std::wstring wdescription = Convert::widen( description ) ;
    			SERVICE_DESCRIPTIONW service_description {} ;
    			service_description.lpDescription = const_cast<wchar_t*>( wdescription.c_str() ) ;
    			return ChangeServiceConfig2W( hmanager , SERVICE_CONFIG_DESCRIPTION , &service_description ) ;
			}
			else
			{
    			SERVICE_DESCRIPTIONA service_description {} ;
    			service_description.lpDescription = const_cast<char*>( description.c_str() ) ;
    			return ChangeServiceConfig2A( hmanager , SERVICE_CONFIG_DESCRIPTION , &service_description ) ;
			}
		}
		inline LONG setWindowLong( HWND hwnd , int index , LONG value ) noexcept
		{
			if( w )
				return SetWindowLongW( hwnd , index , value ) ;
			else
				return SetWindowLongA( hwnd , index , value ) ;
		}
		inline LONG_PTR setWindowLongPtr( HWND hwnd , int index , LONG_PTR value ) noexcept
		{
			if( w )
				return SetWindowLongPtrW( hwnd , index , value ) ;
			else
				return SetWindowLongPtrA( hwnd , index , value ) ;
		}
		inline LONG getWindowLong( HWND hwnd , int index ) noexcept
		{
			if( w )
				return GetWindowLongW( hwnd , index ) ;
			else
				return GetWindowLongA( hwnd , index ) ;
		}
		inline LONG_PTR getWindowLongPtr( HWND hwnd , int index ) noexcept
		{
			if( w )
				return GetWindowLongPtrW( hwnd , index ) ;
			else
				return GetWindowLongPtrA( hwnd , index ) ;
		}
		inline LRESULT sendMessage( HWND hwnd , UINT msg , WPARAM wparam , LPARAM lparam ) noexcept
		{
			if( w )
				return SendMessageW( hwnd , msg , wparam , lparam ) ;
			else
				return SendMessageA( hwnd , msg , wparam , lparam ) ;
		}
		inline LRESULT sendMessageString( HWND hwnd , UINT msg , WPARAM wparam , const std::string & s )
		{
			check_hwnd( hwnd ) ;
			if( IsWindowUnicode(hwnd) )
				return SendMessageW( hwnd , msg , wparam , reinterpret_cast<LPARAM>(Convert::widen(s).c_str()) ) ;
			else
				return SendMessageA( hwnd , msg , wparam , reinterpret_cast<LPARAM>(s.c_str()) ) ;
		}
		inline std::string sendMessageGetString( HWND hwnd , UINT msg , WPARAM wparam )
		{
			check_hwnd( hwnd ) ;
			if( IsWindowUnicode(hwnd) )
			{
				std::vector<wchar_t> buffer( 1024 , L'\0' ) ;
				SendMessageW( hwnd , msg , wparam , reinterpret_cast<LPARAM>(buffer.data()) ) ;
				return Convert::narrow( buffer.data() ) ;
			}
			else
			{
				std::vector<char> buffer( 1024 , '\0' ) ;
				SendMessageA( hwnd , msg , wparam , reinterpret_cast<LPARAM>(buffer.data()) ) ;
				return {buffer.data()} ;
			}
		}
		inline LRESULT sendMessageInsertColumn( HWND hwnd , int sub_item , const std::string & text , int width )
		{
			check_hwnd( hwnd ) ;
			if( IsWindowUnicode(hwnd) )
			{
				std::wstring wtext = Convert::widen( text ) ;
				LVCOLUMNW column ;
				column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
				column.iSubItem = sub_item ;
				column.pszText = const_cast<wchar_t*>( wtext.c_str() ) ;
				column.cx = width ;
				column.fmt = LVCFMT_LEFT ;
				return SendMessageW( hwnd , LVM_INSERTCOLUMNW , static_cast<WPARAM>(sub_item) , reinterpret_cast<LPARAM>(&column) ) ;
			}
			else
			{
				LVCOLUMNA column ;
				column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
				column.iSubItem = sub_item ;
				column.pszText = const_cast<char*>( text.c_str() ) ;
				column.cx = width ;
				column.fmt = LVCFMT_LEFT ;
				return SendMessageA( hwnd , LVM_INSERTCOLUMNA , static_cast<WPARAM>(sub_item) , reinterpret_cast<LPARAM>(&column) ) ;
			}
		}
		inline void sendMessageInsertItem( HWND hwnd , int item , int sub_item , const std::string & text )
		{
			check_hwnd( hwnd ) ;
			if( IsWindowUnicode(hwnd) )
			{
				std::wstring wtext = Convert::widen( text ) ;
				LVITEMW lvitem {} ;
				lvitem.mask = LVIF_TEXT ;
				lvitem.iItem = item ;
				lvitem.iSubItem = sub_item ;
				lvitem.pszText = const_cast<wchar_t*>( wtext.c_str() ) ;
				sendMessage( hwnd , sub_item == 0 ? LVM_INSERTITEMW : LVM_SETITEMW , 0 , reinterpret_cast<LPARAM>(&lvitem) ) ;
			}
			else
			{
				LVITEMA lvitem {} ;
				lvitem.mask = LVIF_TEXT ;
				lvitem.iItem = item ;
				lvitem.iSubItem = sub_item ;
				lvitem.pszText = const_cast<char*>( text.c_str() ) ;
				sendMessage( hwnd , sub_item == 0 ? LVM_INSERTITEMA : LVM_SETITEMA , 0 , reinterpret_cast<LPARAM>(&lvitem) ) ;
			}
		}
		inline BOOL postMessage( HWND hwnd , UINT msg , WPARAM wparam , LPARAM lparam )
		{
			if( w )
				return PostMessageW( hwnd , msg , wparam , lparam ) ;
			else
				return PostMessageA( hwnd , msg , wparam , lparam ) ;
		}
		inline BOOL getMessage( MSG * msg_p , HWND hwnd , UINT filter_min , UINT filter_max )
		{
			if( w )
				return GetMessageW( msg_p , hwnd , filter_min , filter_max ) ;
			else
				return GetMessageA( msg_p , hwnd , filter_min , filter_max ) ;
		}
		inline BOOL peekMessage( MSG * msg_p , HWND hwnd , UINT filter_min , UINT filter_max , UINT remove_type )
		{
			if( w )
				return PeekMessageW( msg_p , hwnd , filter_min , filter_max , remove_type ) ;
			else
				return PeekMessageA( msg_p , hwnd , filter_min , filter_max , remove_type ) ;
		}
		inline LRESULT dispatchMessage( MSG * msg_p )
		{
			if( w )
				return DispatchMessageW( msg_p ) ;
			else
				return DispatchMessageA( msg_p ) ;
		}
		inline IID iidShellLink() noexcept
		{
			if( w )
				return IID_IShellLinkW ;
			else
				return IID_IShellLinkA ;
		}
		inline HRESULT shellLinkSetPath( IShellLinkW * link_p , const Path & path )
		{
			return link_p->SetPath( Convert::widen(path.str()).c_str() ) ;
		}
		inline HRESULT shellLinkSetPath( IShellLinkA * link_p , const Path & path ) noexcept
		{
			return link_p->SetPath( path.cstr() ) ;
		}
		inline HRESULT shellLinkSetWorkingDirectory( IShellLinkW * link_p , const Path & dir )
		{
			return link_p->SetWorkingDirectory( Convert::widen(dir.str()).c_str() ) ;
		}
		inline HRESULT shellLinkSetWorkingDirectory( IShellLinkA * link_p , const Path & dir ) noexcept
		{
			return link_p->SetWorkingDirectory( dir.cstr() ) ;
		}
		inline HRESULT shellLinkSetDescription( IShellLinkW * link_p , std::string_view s )
		{
			return link_p->SetDescription( Convert::widen(s).c_str() ) ;
		}
		inline HRESULT shellLinkSetDescription( IShellLinkA * link_p , const std::string & s ) noexcept
		{
			return link_p->SetDescription( s.c_str() ) ;
		}
		inline HRESULT shellLinkSetArguments( IShellLinkW * link_p , std::string_view s )
		{
			return link_p->SetArguments( Convert::widen(s).c_str() ) ;
		}
		inline HRESULT shellLinkSetArguments( IShellLinkA * link_p , const std::string & s ) noexcept
		{
			return link_p->SetArguments( s.c_str() ) ;
		}
		inline HRESULT shellLinkSetIconLocation( IShellLinkW * link_p , const Path & icon , unsigned int i )
		{
			return link_p->SetIconLocation( Convert::widen(icon.str()).c_str() , i ) ;
		}
		inline HRESULT shellLinkSetIconLocation( IShellLinkA * link_p , const Path & icon , unsigned int i ) noexcept
		{
			return link_p->SetIconLocation( icon.cstr() , i ) ;
		}
		inline HRESULT shellLinkSetShowCmd( IShellLinkW * link_p , int show ) noexcept
		{
			return link_p->SetShowCmd( show ) ;
		}
		inline HRESULT shellLinkSetShowCmd( IShellLinkA * link_p , int show ) noexcept
		{
			return link_p->SetShowCmd( show ) ;
		}
		inline HRESULT persistFileSave( IPersistFile * persist_file_p , const G::Path & link_path , BOOL remember )
		{
			return persist_file_p->Save( Convert::widen(link_path.str()).c_str() , remember ) ;
		}
		inline Path shGetFolderPath( HWND hwnd , int csidl , HANDLE user_token , DWORD flags )
		{
			if( w )
			{
				std::vector<wchar_t> buffer( MAX_PATH+1U , L'\0' ) ;
				if( S_OK != SHGetFolderPathW( hwnd , csidl , user_token , flags , buffer.data() ) )
					return {} ;
				buffer[buffer.size()-1U] = L'\0' ;
				return Path( Convert::narrow(buffer.data()) ) ;
			}
			else
			{
				std::vector<char> buffer( MAX_PATH+1U , '\0' ) ;
				if( S_OK != SHGetFolderPathA( hwnd , csidl , user_token , flags , buffer.data() ) )
					return {} ;
				buffer[buffer.size()-1U] = '\0' ;
				return Path( buffer.data() ) ;
			}
		}
		inline std::string loadString( HINSTANCE hinstance , UINT id )
		{
			if( w )
			{
				std::vector<wchar_t> buffer( 1024U , L'\0' ) ;
				int n = LoadStringW( hinstance , id , buffer.data() , static_cast<int>(buffer.size()-1U) ) ;
				if( n <= 0 ) return {} ;
				return Convert::narrow( buffer.data() , static_cast<std::size_t>(n) ) ;
			}
			else
			{
				std::vector<char> buffer( 1024U , L'\0' ) ;
				int n = LoadStringA( hinstance , id , buffer.data() , static_cast<int>(buffer.size()-1U) ) ;
				if( n <= 0 ) return {} ;
				return {buffer.data(),static_cast<std::size_t>(n)} ;
			}
		}
		inline HANDLE createWaitableTimer( LPSECURITY_ATTRIBUTES attributes , BOOL manual_reset , const std::string & name )
		{
			if( w )
			{
				std::wstring wname = G::Convert::widen( name ) ;
				return CreateWaitableTimerW( attributes , manual_reset , wname.c_str() ) ;
			}
			else
			{
				return CreateWaitableTimerA( attributes , manual_reset , name.c_str() ) ;
			}
		}
		inline INT getAddrInfo( std::string_view host , std::string_view service , const ADDRINFOW * hints , ADDRINFOW ** results )
		{
			return GetAddrInfoW( Convert::widen(host).c_str() , Convert::widen(service).c_str() , hints , results ) ;
		}
		inline INT getAddrInfo( const std::string & host , const std::string & service , const ADDRINFOA * hints , ADDRINFOA ** results )
		{
			return GetAddrInfoA( host.c_str() , service.c_str() , hints , results ) ;
		}
		inline std::string canonicalName( const ADDRINFOW & ai )
		{
			return ai.ai_canonname ? Convert::narrow(ai.ai_canonname) : std::string() ;
		}
		inline std::string canonicalName( const ADDRINFOA & ai )
		{
			return ai.ai_canonname ? std::string(ai.ai_canonname) : std::string() ;
		}
		inline void freeAddrInfo( ADDRINFOW * results )
		{
			if( results )
				FreeAddrInfoW( results ) ;
		}
		inline void freeAddrInfo( ADDRINFOA * results )
		{
			if( results )
				FreeAddrInfoA( results ) ;
		}
	}
}

#endif

#endif
