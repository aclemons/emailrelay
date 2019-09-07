//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// access_win32.cpp
//

#include "gdef.h"
#include "access.h"
#include "gexception.h"
#include <processthreadsapi.h>
#include <sddl.h>
#include <aclapi.h>

static void add_user_write_permissions_to_directory( const std::string & ) ;

bool Access::modify( const G::Path & path , bool b )
{
	try
	{
		// this is used to open up permissions on ProgramData/E-MailRelay so
		// that the installing-user can edit emailrelay-start.bat (etc) -- if
		// it doesnt work then it doesn't stop anything working, it's just then
		// a pain to modify startup options
		if( !b )
			add_user_write_permissions_to_directory( path.str() ) ;
		return true ;
	}
	catch( std::exception & )
	{
	}
	return false ;
}

namespace
{
	struct Error : public G::Exception
	{
		explicit Error( DWORD e = GetLastError() ) :
			G::Exception("error") ,
			m_e(e)
		{
		}
		DWORD m_e ;
	} ;
	struct Token
	{
		explicit Token( HANDLE hprocess = GetCurrentProcess() ) :
			m_h(0)
		{
			BOOL rc = OpenProcessToken( hprocess , TOKEN_READ , &m_h ) ;
			if( rc == 0 )
				throw Error() ;
		}
		~Token()
		{
			CloseHandle( m_h ) ;
		}
		HANDLE handle() const
		{
			return m_h ;
		}
		HANDLE m_h ;
	} ;
	struct Sid
	{
		virtual ~Sid() {}
		virtual PSID ptr() const = 0 ;
	} ;
	struct UserSid : Sid
	{
		UserSid( const Token & token )
		{
			DWORD size = 0 ;
			GetTokenInformation( token.handle() , TokenUser , NULL , 0 , &size ) ;
			if( size == 0 ) throw Error() ;
			m_buffer.resize( static_cast<size_t>(size) ) ;
			BOOL rc = GetTokenInformation( token.handle() , TokenUser , &m_buffer[0] , size , &size ) ;
			if( rc == 0 )
				throw Error() ;
		}
		virtual PSID ptr() const override
		{
			const TOKEN_USER * user_info = reinterpret_cast<const TOKEN_USER*>(&m_buffer[0]) ;
			return user_info->User.Sid ;
		}
		std::string str() const
		{
			char * p = nullptr ;
			ConvertSidToStringSid( ptr() , &p ) ;
			std::string s ;
			if( p )
			{
				s = std::string( p ) ;
				LocalFree( p ) ;
			}
			return s ;
		}
		virtual ~UserSid()
		{
		}
		std::vector<char> m_buffer ;
	} ;
	struct DirectoryWriteAccessFor : public EXPLICIT_ACCESS
	{
		explicit DirectoryWriteAccessFor( const Sid & sid )
		{
			static EXPLICIT_ACCESS zero ;
			*static_cast<EXPLICIT_ACCESS*>(this) = zero ;
			grfAccessPermissions = GENERIC_ALL ;
			grfAccessMode = GRANT_ACCESS ;
			grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ;
			Trustee.TrusteeForm = TRUSTEE_IS_SID ;
			Trustee.TrusteeType = TRUSTEE_IS_USER ;
			Trustee.ptstrName = reinterpret_cast<LPSTR>(sid.ptr()) ;
		}
	} ;
	struct Dacl
	{
		explicit Dacl( const std::string & path ) :
			m_sd(nullptr) ,
			m_dacl(nullptr) ,
			m_free_me(false)
		{
			DWORD rc = GetNamedSecurityInfo( path.c_str() , SE_FILE_OBJECT , DACL_SECURITY_INFORMATION ,
				nullptr , nullptr , &m_dacl , nullptr , &m_sd ) ;
			if( rc != ERROR_SUCCESS || m_dacl == NULL )
				throw Error() ;
		}
		~Dacl()
		{
			if( m_sd )
				LocalFree( m_sd ) ;
			if( m_free_me )
				LocalFree( m_dacl ) ;
		}
		void add( const EXPLICIT_ACCESS & access )
		{
			ACL * new_dacl = nullptr ;
			DWORD rc = SetEntriesInAcl( 1 , const_cast<EXPLICIT_ACCESS*>(&access) , m_dacl , &new_dacl ) ;
			if( rc != ERROR_SUCCESS || new_dacl == nullptr )
				throw Error() ;
			m_dacl = new_dacl ;
			m_free_me = true ;
		}
		void applyTo( const std::string & path )
		{
			DWORD rc = SetNamedSecurityInfo( const_cast<char*>(path.c_str()) , SE_FILE_OBJECT , DACL_SECURITY_INFORMATION ,
				nullptr , nullptr , m_dacl , nullptr ) ;
			if( rc != ERROR_SUCCESS )
				throw Error() ;
		}
		PSECURITY_DESCRIPTOR m_sd ;
		ACL * m_dacl ;
		bool m_free_me ;
	} ;
}

static void add_user_write_permissions_to_directory( const std::string & path )
{
	Dacl dacl( path ) ;
	dacl.add( DirectoryWriteAccessFor( UserSid(Token()) ) ) ;
	dacl.applyTo( path ) ;
}

/// \file access_win32.cpp
