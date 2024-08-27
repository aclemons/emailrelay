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
/// \file gidentity_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gidentity.h"
#include "gscope.h"
#include "gstr.h"
#include "grange.h"
#include "gbuffer.h"
#include "gconvert.h"
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
			Account( SID_NAME_USE , const std::string & , const std::string & , const std::string & ) ;
			SID_NAME_USE type {SidTypeInvalid} ;
			std::string sid ;
			std::string domain ;
			std::string name ; // if requested
			bool valid() const noexcept { return type == SidTypeUser ; }
		} ;
		std::string sid() ;
		std::string rootsid() ;
		std::string sidstr( PSID sid_p ) ;
		std::string computername() ;
		Account lookup( std::string_view , bool = false ) ;
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
	static_assert( noexcept(std::string()) , "" ) ; // only guaranteed for c++17
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

G::Identity G::Identity::invalid( SignalSafe ) noexcept
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

std::pair<G::Identity,std::string> G::Identity::lookup( std::string_view name )
{
	auto account = IdentityImp::lookup( name , true ) ;
	if( !account.valid() )
		throw NoSuchUser( name ) ;

	Identity id ;
	id.m_sid = account.sid ;
	return std::make_pair( id , account.name ) ;
}

std::pair<G::Identity,std::string> G::Identity::lookup( std::string_view name , std::nothrow_t )
{
	auto account = IdentityImp::lookup( name , true ) ;
	if( account.valid() )
	{
		Identity id ;
		id.m_sid = account.sid ;
		return std::make_pair( id , account.name ) ;
	}
	else
	{
		return std::make_pair( Identity() , std::string() ) ;
	}
}

gid_t G::Identity::lookupGroup( const std::string & /*group*/ )
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
	return G::nowide::convertSidToStringSid( sid_p ) ;
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

std::string G::IdentityImp::computername()
{
	return nowide::getComputerNameEx() ;
}

G::IdentityImp::Account G::IdentityImp::lookup( std::string_view name , bool with_canonical_name )
{
	const Account error ;
	if( name.empty() || name.find('\\') != std::string::npos )
		return error ;
	std::string domain = computername() ; // => local accounts
	if( domain.empty() )
		return error ;
	std::string full_name = domain.append(1U,'\\').append(name.data(),name.size()) ;

	DWORD sidsize = 0 ;
	DWORD domainsize = 0 ;
	SID_NAME_USE type = SidTypeInvalid ;
	if( nowide::lookupAccountName( full_name , NULL , &sidsize , false , &domainsize , &type ) )
		return error ;
	G::Buffer<char> sidbuffer( std::max(DWORD(1),sidsize) ) ;
	if( !nowide::lookupAccountName( full_name , sidbuffer.data() , &sidsize , true , &domainsize , &type ) )
		return error ;
	SID * sid_p = G::buffer_cast<SID*>(sidbuffer) ;

	std::string canonical_name ;
	if( with_canonical_name )
	{
		DWORD namebuffersize = 0 ;
		DWORD domainbuffersize = 0 ;
		if( nowide::lookupAccountSid( sid_p , nullptr , false , &namebuffersize , false , &domainbuffersize , &type ) )
			return error ;
		if( !nowide::lookupAccountSid( sid_p , &canonical_name , true , &namebuffersize , true , &domainbuffersize , &type ) )
			return error ;
		if( canonical_name.empty() )
			return error ;
	}

	return { type , sidstr(sid_p) , domain , canonical_name } ;
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

G::IdentityImp::Account::Account( SID_NAME_USE type_ , const std::string & sid_ ,
	const std::string & domain_ , const std::string & name_ ) :
		type(type_) ,
		sid(sid_) ,
		domain(domain_) ,
		name(name_)
{
}

