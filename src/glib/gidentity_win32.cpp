//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gidentity_win32.cpp
///

#include "gdef.h"
#include "gidentity.h"
#include "gscope.h"
#include "gstr.h"
#include "grange.h"
#include "gbuffer.h"
#include <sddl.h> // ConvertSidToStringSid()
#include <sstream>
#include <vector>
#include <algorithm>

namespace G
{
	namespace IdentityImp
	{
		struct Account
		{
			Account() = default ;
			Account( SID_NAME_USE , const std::string & , const std::string & ) ;
			SID_NAME_USE type {SidTypeInvalid} ;
			std::string sid ;
			std::string domain ;
			bool valid() const noexcept { return type == SidTypeUser ; }
		} ;
		std::string sid() ;
		std::string rootsid() ;
		std::string sidstr( PSID sid_p ) ;
		std::string computername() ;
		Account lookup( const std::string & name ) ;
	}
}

G::Identity::Identity( uid_t , gid_t , const std::string & sid ) :
	m_uid(-1) ,
	m_gid(-1) ,
	m_sid(sid)
{
}

G::Identity::Identity() noexcept : // invalid()
	m_uid(-1) ,
	m_gid(-1)
{
	static_assert( noexcept(std::string()) , "" ) ; // c++17
}

G::Identity::Identity( SignalSafe ) noexcept : // invalid()
	Identity()
{
}

G::Identity::Identity( const std::string & name , const std::string & ) :
	Identity()
{
	auto account = IdentityImp::lookup( name ) ;
	if( !account.valid() )
		throw NoSuchUser( name ) ;
	m_sid = account.sid ;
}

G::Identity G::Identity::effective() noexcept
{
	return { -1 , -1 , IdentityImp::sid() } ;
}

G::Identity G::Identity::real() noexcept
{
	return effective() ;
}

G::Identity G::Identity::invalid() noexcept
{
	return {} ;
}

G::Identity G::Identity::invalid( SignalSafe safe ) noexcept
{
	return {} ;
}

G::Identity G::Identity::root() noexcept
{
	Identity id( "Administrator" ) ; // hmm
	if( id != invalid() )
		return id ;
	return { -1 , -1 , IdentityImp::rootsid() } ;
}

std::string G::Identity::str() const
{
	return sid() ;
}

uid_t G::Identity::userid() const noexcept
{
	if( m_sid.empty() ) return false ;
	return G::Str::toInt( G::Str::tail( m_sid , m_sid.rfind('-') , "" ) , "-1" ) ; // "RID"
}

gid_t G::Identity::groupid() const noexcept
{
	return -1 ;
}

std::string G::Identity::sid() const
{
	return
		m_sid.empty() ?
			std::string("S-1-0-0") :
			m_sid ;
}

bool G::Identity::isRoot() const noexcept
{
	return G::Str::headMatch(m_sid,"S-1-5-") && G::Str::tailMatch(m_sid,"-500") ;
}

bool G::Identity::operator==( const Identity & other ) const noexcept
{
	return m_sid == other.m_sid ;
}

bool G::Identity::operator!=( const Identity & other ) const noexcept
{
	return m_sid != other.m_sid ;
}

G::Identity G::Identity::lookup( const std::string & name )
{
	Identity result ;
	auto account = IdentityImp::lookup( name ) ;
	if( !account.valid() )
		throw NoSuchUser( name ) ;
	result.m_sid = account.sid ;
	return result ;
}

G::Identity G::Identity::lookup( const std::string & name , std::nothrow_t )
{
	Identity result ;
	auto account = IdentityImp::lookup( name ) ;
	if( account.valid() )
		result.m_sid = account.sid ;
	return result ;
}

gid_t G::Identity::lookupGroup( const std::string & group )
{
	return -1 ;
}

bool G::Identity::match( std::pair<int,int> range ) const
{
	return G::Range::within( range , userid() ) ;
}

// ==

std::string G::IdentityImp::sidstr( PSID sid_p )
{
	char * sidstr_p = nullptr ;
	if( !ConvertSidToStringSid( sid_p , &sidstr_p ) || sidstr_p == nullptr )
		return {} ;
	std::string result( sidstr_p ) ;
	LocalFree( sidstr_p ) ;
	return result ;
}

std::string G::IdentityImp::sid()
{
	HANDLE htoken = NULL ;
	if( !OpenProcessToken( GetCurrentProcess() , TOKEN_QUERY , &htoken ) )
		return {} ;
	G::ScopeExit close( [htoken](){CloseHandle(htoken);} ) ;
	DWORD size = 0 ;
	G::Buffer<char> buffer( sizeof(TOKEN_USER) ) ;
	if( !GetTokenInformation( htoken , TokenUser , &buffer[0] , static_cast<DWORD>(buffer.size()) , &size ) && size )
		buffer.resize( static_cast<std::size_t>(size) ) ;
	if( !GetTokenInformation( htoken , TokenUser , &buffer[0] , static_cast<DWORD>(buffer.size()) , &size ) )
		return {} ;
	TOKEN_USER * info_p = G::buffer_cast<TOKEN_USER*>( buffer ) ;
	return sidstr( info_p->User.Sid ) ;
}

#if 0
std::string G::IdentityImp::username()
{
	std::vector<char> buffer ;
	buffer.reserve( 100U ) ;
	buffer.resize( 1U ) ;
	DWORD size = static_cast<DWORD>( buffer.size() ) ;
	if( !GetUserNameA( &buffer[0] , &size ) && GetLastError() == ERROR_INSUFFICIENT_BUFFER && size )
		buffer.resize( static_cast<std::size_t>(size) ) ;
	if( size == 0 || !GetUserNameA( &buffer[0] , &size ) )
		return {} ;
	std::size_t n = std::min( buffer.size() , static_cast<std::size_t>(size) ) ;
	buffer.at( n-1U ) = '\0' ;
	return std::string( &buffer[0] ) ;
}
#endif

std::string G::IdentityImp::computername()
{
	std::vector<char> buffer ;
	buffer.reserve( 100U ) ;
	buffer.resize( 1U ) ;
	DWORD size = static_cast<DWORD>( buffer.size() ) ;
	if( !GetComputerNameExA( ComputerNameNetBIOS , &buffer[0] , &size ) && GetLastError() == ERROR_MORE_DATA && size )
		buffer.resize( static_cast<std::size_t>(size) ) ;
	if( !GetComputerNameExA( ComputerNameNetBIOS , &buffer[0] , &size ) )
		return {} ;
	std::size_t n = std::min( buffer.size()-1U , static_cast<std::size_t>(size) ) ;
	buffer.at( n ) = '\0' ;
	return std::string( &buffer[0] ) ;
}

G::IdentityImp::Account G::IdentityImp::lookup( const std::string & name )
{
	if( name.empty() || name.find('\\') != std::string::npos )
		return {} ;
	std::string domain = computername() ; // => local accounts
	if( domain.empty() )
		return {} ;
	std::string full_name = domain.append(1U,'\\').append(name) ;
	DWORD sidsize = 0 ;
	DWORD domainsize = 0 ;
	SID_NAME_USE type = SidTypeInvalid ;
	if( LookupAccountNameA( NULL , full_name.c_str() , NULL , &sidsize , NULL , &domainsize , &type ) )
		return {} ;
	G::Buffer<char> sidbuffer( std::max(DWORD(1),sidsize) ) ;
	std::vector<char> domainbuffer( std::max(DWORD(1),domainsize) ) ;
	if( !LookupAccountNameA( NULL , full_name.c_str() , &sidbuffer[0] , &sidsize , &domainbuffer[0] , &domainsize , &type ) )
		return {} ;
	SID * sid_p = G::buffer_cast<SID*>(sidbuffer) ;
	return { type , sidstr(sid_p) , &domain[0] } ;
}

std::string G::IdentityImp::rootsid()
{
	DWORD size = 0 ;
	G::Buffer<char> buffer( 1U ) ;
	WELL_KNOWN_SID_TYPE type = WinLocalAccountAndAdministratorSid ;
	if( !CreateWellKnownSid( type , NULL , &buffer[0] , &size ) && size )
		buffer.resize( static_cast<std::size_t>(size) ) ;
	if( !CreateWellKnownSid( type , NULL , &buffer[0] , &size ) )
		return {} ;
	SID * sid_p = G::buffer_cast<SID*>( buffer ) ;
	return sidstr( sid_p ) ;
}

G::IdentityImp::Account::Account( SID_NAME_USE type_ , const std::string & sid_ , const std::string & domain_ ) :
	type(type_) ,
	sid(sid_) ,
	domain(domain_)
{
}

