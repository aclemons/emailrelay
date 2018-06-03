//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsaslclient.cpp
//

#include "gdef.h"
#include "gsaslclient.h"
#include "gssl.h"
#include "gmd5.h"
#include "ghash.h"
#include "gcram.h"
#include "gbase64.h"
#include "gstr.h"
#include "glogoutput.h"
#include "gdebug.h"
#include <algorithm>
#include <sstream>

namespace
{
	const char * login_challenge_1 = "Username:" ;
	const char * login_challenge_2 = "Password:" ;
}

/// \class GAuth::SaslClientImp
/// A private pimple-pattern implementation class used by GAuth::SaslClient.
///
class GAuth::SaslClientImp
{
public:
	typedef SaslClient::Response Response ;
	explicit SaslClientImp( const SaslClientSecrets & ) ;
	bool active() const ;
	std::string preferred( const G::StringArray & ) const ;
	Response response( const std::string & mechanism , const std::string & challenge ) const ;
	bool next() ;
	std::string preferred() const ;
	std::string id() const ;
	std::string info() const ;
	static bool match( const G::StringArray & mechanisms , const std::string & ) ;

private:
	const SaslClientSecrets & m_secrets ;
	mutable G::StringArray m_mechanisms ;
	mutable std::string m_info ;
	mutable std::string m_id ;
	std::string PLAIN ;
	std::string LOGIN ;
} ;

// ===

GAuth::SaslClientImp::SaslClientImp( const SaslClientSecrets & secrets ) :
	m_secrets(secrets) ,
	PLAIN("PLAIN") ,
	LOGIN("LOGIN")
{
}

std::string GAuth::SaslClientImp::preferred( const G::StringArray & server_mechanisms ) const
{
	// short-circuit if no secrets file
	if( !active() )
		return std::string() ;

	// if we have a plaintext password then we can use any cram
	// mechanism for which we have a hash function -- otherwise
	// we can use cram mechanisms where we have a hashed password
	// of the correct type and the hash function is capable of
	// initialisation with an intermediate state
	//
	G::StringArray our_list = Cram::hashTypes( "CRAM-" , !m_secrets.clientSecret("plain").valid() ) ;
	for( G::StringArray::iterator p = our_list.begin() ; p != our_list.end() ; )
	{
		if( m_secrets.clientSecret((*p).substr(5U)).valid() )
			++p ;
		else
			p = our_list.erase( p ) ;
	}
	if( m_secrets.clientSecret("oauth").valid() )
	{
		our_list.push_back( "XOAUTH2" ) ;
	}
	if( m_secrets.clientSecret("plain").valid() )
	{
		our_list.push_back( PLAIN ) ;
		our_list.push_back( LOGIN ) ;
	}

	// build the list of mechanisms that we can use with the server
	m_mechanisms.clear() ;
	for( G::StringArray::iterator p = our_list.begin() ; p != our_list.end() ; ++p )
	{
		if( match(server_mechanisms,*p) )
		{
			m_mechanisms.push_back( *p ) ;
		}
	}

	G_DEBUG( "GAuth::SaslClientImp::preferred: server mechanisms: [" << G::Str::join(",",server_mechanisms) << "]" ) ;
	G_DEBUG( "GAuth::SaslClientImp::preferred: our mechanisms: [" << G::Str::join(",",our_list) << "]" ) ;
	G_DEBUG( "GAuth::SaslClientImp::preferred: usable mechanisms: [" << G::Str::join(",",m_mechanisms) << "]" ) ;

	return m_mechanisms.empty() ? std::string() : m_mechanisms.at(0U) ;
}

bool GAuth::SaslClientImp::next()
{
	if( !m_mechanisms.empty() )
		m_mechanisms.erase( m_mechanisms.begin() ) ;
	return !m_mechanisms.empty() ;
}

std::string GAuth::SaslClientImp::preferred() const
{
	return m_mechanisms.empty() ? std::string() : m_mechanisms.at(0U) ;
}

GAuth::SaslClient::Response GAuth::SaslClientImp::response( const std::string & mechanism ,
	const std::string & challenge ) const
{
	Response rsp ;
	rsp.error = true ;
	rsp.final = false ;
	rsp.sensitive = true ;

	Secret secret = Secret::none() ;
	if( mechanism.find("CRAM-") == 0U )
	{
		std::string hash_type = mechanism.substr( 5U ) ;
		secret = m_secrets.clientSecret( hash_type ) ;
		if( !secret.valid() )
			secret = m_secrets.clientSecret( "plain" ) ;
		rsp.data = Cram::response( hash_type , true , secret , challenge , secret.id() ) ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == "APOP" )
	{
		secret = m_secrets.clientSecret( "MD5" ) ;
		rsp.data = Cram::response( "MD5" , false , secret , challenge , secret.id() ) ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == PLAIN )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		const std::string sep( 1U , '\0' ) ;
		rsp.data = sep + secret.id() + sep + secret.key() ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_1 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp.data = secret.id() ;
		rsp.error = rsp.data.empty() ;
		rsp.final = false ;
		rsp.sensitive = false ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_2 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp.data = secret.key() ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == "XOAUTH2" && challenge.empty() )
	{
		secret = m_secrets.clientSecret( "oauth" ) ;
		rsp.data = secret.key() ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ; // not well-defined, may get an informational challenge
		rsp.sensitive = true ;
	}
	else if( mechanism == "XOAUTH2" )
	{
		secret = m_secrets.clientSecret( "oauth" ) ;
		rsp.data.clear() ; // information-only challenge gets an empty response
		rsp.error = false ;
		rsp.final = true ;
		rsp.sensitive = false ;
	}

	if( rsp.final )
	{
		std::ostringstream ss ;
		ss << "using mechanism [" << G::Str::lower(mechanism) << "] and " << secret.info() ;
		m_info = ss.str() ;
		m_id = secret.id() ;
	}

	return rsp ;
}

std::string GAuth::SaslClientImp::id() const
{
	return m_id ;
}

std::string GAuth::SaslClientImp::info() const
{
	return m_info ;
}

bool GAuth::SaslClientImp::active() const
{
	return m_secrets.valid() ;
}

bool GAuth::SaslClientImp::match( const G::StringArray & mechanisms , const std::string & mechanism )
{
	return std::find(mechanisms.begin(),mechanisms.end(),mechanism) != mechanisms.end() ;
}

// ===

GAuth::SaslClient::SaslClient( const SaslClientSecrets & secrets ) :
	m_imp(new SaslClientImp(secrets) )
{
}

GAuth::SaslClient::~SaslClient()
{
	delete m_imp ;
}

bool GAuth::SaslClient::active() const
{
	return m_imp->active() ;
}

GAuth::SaslClient::Response GAuth::SaslClient::response( const std::string & mechanism , const std::string & challenge ) const
{
	return m_imp->response( mechanism , challenge ) ;
}

std::string GAuth::SaslClient::preferred( const G::StringArray & server_mechanisms ) const
{
	return m_imp->preferred( server_mechanisms ) ;
}

bool GAuth::SaslClient::next()
{
	return m_imp->next() ;
}

std::string GAuth::SaslClient::preferred() const
{
	return m_imp->preferred() ;
}

std::string GAuth::SaslClient::id() const
{
	return m_imp->id() ;
}

std::string GAuth::SaslClient::info() const
{
	return m_imp->info() ;
}

