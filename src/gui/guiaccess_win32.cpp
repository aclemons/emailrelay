//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file guiaccess_win32.cpp
///

#include "gdef.h"
#include "guiaccess.h"
#include "gbuffer.h"
#include "gexception.h"
#include "gassert.h"
#include <processthreadsapi.h>
#include <sddl.h>
#include <aclapi.h>

namespace Gui
{
	namespace AccessImp
	{
		void add_user_write_permissions_to_directory( const std::string & ) ;
		struct Error ;
		struct Token ;
		struct Sid ;
		struct UserSid ;
		struct DirectoryWriteAccessFor ;
		struct Dacl ;
	}
}


bool Gui::Access::modify( const G::Path & path , bool b )
{
	try
	{
		// this is used to open up permissions on ProgramData/E-MailRelay so
		// that the installing-user can edit emailrelay-start.bat (etc) -- if
		// it fails then it doesn't stop anything else working, but it becomes
		// a pain to modify server startup options
		if( !b )
			AccessImp::add_user_write_permissions_to_directory( path.str() ) ;
		return true ;
	}
	catch( std::exception & )
	{
	}
	return false ;
}

struct Gui::AccessImp::Error : public G::Exception
{
	explicit Error( DWORD e = GetLastError() ) :
		G::Exception("error") ,
		m_e(e)
	{
	}
	DWORD m_e ;
} ;

struct Gui::AccessImp::Token
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
	Token( const Token & ) = delete ;
	Token( Token && ) = delete ;
	Token & operator=( const Token & ) = delete ;
	Token & operator=( Token && ) = delete ;
	HANDLE m_h ;
} ;

struct Gui::AccessImp::Sid
{
	virtual ~Sid() = default ;
	virtual PSID ptr() const = 0 ;
} ;

struct Gui::AccessImp::UserSid : Sid
{
	UserSid( const Token & token )
	{
		DWORD size = 0 ;
		GetTokenInformation( token.handle() , TokenUser , nullptr , 0 , &size ) ;
		if( size == 0 ) throw Error() ;
		m_buffer.resize( static_cast<std::size_t>(size) ) ;
		BOOL rc = GetTokenInformation( token.handle() , TokenUser , &m_buffer[0] , size , &size ) ;
		if( rc == 0 )
			throw Error() ;
	}
	virtual PSID ptr() const override
	{
		const TOKEN_USER * user_info = G::buffer_cast<TOKEN_USER*>( m_buffer ) ;
		return user_info->User.Sid ;
	}
	std::string str() const
	{
		char * p = nullptr ;
		ConvertSidToStringSidA( ptr() , &p ) ;
		std::string s ;
		if( p )
		{
			s = std::string( p ) ;
			LocalFree( p ) ;
		}
		return s ;
	}
	~UserSid() override = default ;
	G::Buffer<char> m_buffer ;
} ;

struct Gui::AccessImp::DirectoryWriteAccessFor : public EXPLICIT_ACCESS_A
{
	explicit DirectoryWriteAccessFor( const Sid & sid )
	{
		EXPLICIT_ACCESS_A zero{} ;
		*static_cast<EXPLICIT_ACCESS_A*>(this) = zero ;
		grfAccessPermissions = GENERIC_ALL ;
		grfAccessMode = GRANT_ACCESS ;
		grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ;
		Trustee.TrusteeForm = TRUSTEE_IS_SID ;
		Trustee.TrusteeType = TRUSTEE_IS_USER ;
		Trustee.ptstrName = reinterpret_cast<LPSTR>(sid.ptr()) ;
	}
} ;

struct Gui::AccessImp::Dacl
{
	explicit Dacl( const std::string & path ) :
		m_sd(nullptr) ,
		m_dacl(nullptr) ,
		m_free_me(false)
	{
		DWORD rc = GetNamedSecurityInfoA( path.c_str() , SE_FILE_OBJECT , DACL_SECURITY_INFORMATION ,
			nullptr , nullptr , &m_dacl , nullptr , &m_sd ) ;
		if( rc != ERROR_SUCCESS || m_dacl == nullptr )
			throw Error() ;
	}
	~Dacl()
	{
		if( m_sd )
			LocalFree( m_sd ) ;
		if( m_free_me )
			LocalFree( m_dacl ) ;
	}
	void add( const EXPLICIT_ACCESS_A & access )
	{
		ACL * new_dacl = nullptr ;
		DWORD rc = SetEntriesInAclA( 1 , const_cast<EXPLICIT_ACCESS_A*>(&access) , m_dacl , &new_dacl ) ;
		if( rc != ERROR_SUCCESS || new_dacl == nullptr )
			throw Error() ;
		m_dacl = new_dacl ;
		m_free_me = true ;
	}
	void applyTo( const std::string & path )
	{
		DWORD rc = SetNamedSecurityInfoA( const_cast<char*>(path.c_str()) , SE_FILE_OBJECT ,
			DACL_SECURITY_INFORMATION , nullptr , nullptr , m_dacl , nullptr ) ;
		if( rc != ERROR_SUCCESS )
			throw Error() ;
	}
	Dacl( const Dacl & ) = delete ;
	Dacl( Dacl && ) = delete ;
	Dacl & operator=( const Dacl & ) = delete ;
	Dacl & operator=( Dacl && ) = delete ;
	PSECURITY_DESCRIPTOR m_sd ;
	ACL * m_dacl ;
	bool m_free_me ;
} ;

void Gui::AccessImp::add_user_write_permissions_to_directory( const std::string & path )
{
	Dacl dacl( path ) ;
	dacl.add( DirectoryWriteAccessFor( UserSid(Token()) ) ) ;
	dacl.applyTo( path ) ;
}

