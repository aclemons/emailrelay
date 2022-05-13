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
/// \file gsaslclient.cpp
///

#include "gdef.h"
#include "gsaslclient.h"
#include "gssl.h"
#include "gmd5.h"
#include "ghash.h"
#include "gcram.h"
#include "gbase64.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <sstream>

//| \class GAuth::SaslClientImp
/// A private pimple-pattern implementation class used by GAuth::SaslClient.
///
class GAuth::SaslClientImp
{
public:
	using Response = SaslClient::Response ;
	SaslClientImp( const SaslClientSecrets & , const std::string & ) ;
	bool active() const ;
	std::string mechanism( const G::StringArray & ) const ;
	std::string initialResponse( std::size_t limit ) const ;
	Response response( const std::string & mechanism , const std::string & challenge ) const ;
	bool next() ;
	std::string mechanism() const ;
	std::string id() const ;
	std::string info() const ;
	static bool match( const G::StringArray & mechanisms , const std::string & ) ;

private:
	const SaslClientSecrets & m_secrets ;
	std::string m_config ;
	mutable G::StringArray m_mechanisms ;
	mutable std::string m_info ;
	mutable std::string m_id ;
	std::string PLAIN ;
	std::string LOGIN ;
	static const char * const login_challenge_1 ;
	static const char * const login_challenge_2 ;
} ;

const char * const GAuth::SaslClientImp::login_challenge_1 = "Username:" ;
const char * const GAuth::SaslClientImp::login_challenge_2 = "Password:" ;

// ===

GAuth::SaslClientImp::SaslClientImp( const SaslClientSecrets & secrets , const std::string & sasl_client_config ) :
	m_secrets(secrets) ,
	m_config(sasl_client_config) ,
	PLAIN("PLAIN") ,
	LOGIN("LOGIN")
{
}

std::string GAuth::SaslClientImp::mechanism( const G::StringArray & server_mechanisms ) const
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
	G::StringArray our_list ;
	if( m_secrets.clientSecret("plain").valid() )
	{
		our_list = Cram::hashTypes( "CRAM-" , false ) ;
	}
	else
	{
		our_list = Cram::hashTypes( "CRAM-" , true ) ;
		for( auto p = our_list.begin() ; p != our_list.end() ; )
		{
			if( m_secrets.clientSecret((*p).substr(5U)).valid() )
				++p ;
			else
				p = our_list.erase( p ) ;
		}
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

	// use the configuration string as a mechanism whitelist and/or blocklist
	if( !m_config.empty() )
	{
		bool simple = G::Str::imatch( our_list , m_config ) ; // eg. allow "plain" as well as "m:plain"
		G::StringArray list = G::Str::splitIntoTokens( G::Str::upper(m_config) , ";" ) ;
		G::StringArray whitelist = G::Str::splitIntoTokens( simple ? G::Str::upper(m_config) : G::Str::headMatchResidue( list , "M:" ) , "," ) ;
		G::StringArray blocklist = G::Str::splitIntoTokens( G::Str::headMatchResidue( list , "X:" ) , "," ) ;
		our_list.erase( G::Str::keepMatch( our_list.begin() , our_list.end() , whitelist , true ) , our_list.end() ) ;
		our_list.erase( G::Str::removeMatch( our_list.begin() , our_list.end() , blocklist , true ) , our_list.end() ) ;
	}

	// build the list of mechanisms that we can use with the server
	m_mechanisms.clear() ;
	for( auto & our_mechanism : our_list )
	{
		if( match(server_mechanisms,our_mechanism) )
		{
			m_mechanisms.push_back( our_mechanism ) ;
		}
	}

	G_DEBUG( "GAuth::SaslClientImp::mechanism: server mechanisms: [" << G::Str::join(",",server_mechanisms) << "]" ) ;
	G_DEBUG( "GAuth::SaslClientImp::mechanism: our mechanisms: [" << G::Str::join(",",our_list) << "]" ) ;
	G_DEBUG( "GAuth::SaslClientImp::mechanism: usable mechanisms: [" << G::Str::join(",",m_mechanisms) << "]" ) ;

	return m_mechanisms.empty() ? std::string() : m_mechanisms.at(0U) ;
}

bool GAuth::SaslClientImp::next()
{
	if( !m_mechanisms.empty() )
		m_mechanisms.erase( m_mechanisms.begin() ) ;
	return !m_mechanisms.empty() ;
}

std::string GAuth::SaslClientImp::mechanism() const
{
	return m_mechanisms.empty() ? std::string() : m_mechanisms.at(0U) ;
}

std::string GAuth::SaslClientImp::initialResponse( std::size_t limit ) const
{
	// (the implementation of response() is stateless because it can derive
	// state from the challege, so we doesn't need to worry here about
	// side-effects between initialResponse() and response())

	const std::string m = mechanism() ;
	if( m.empty() || m.find("CRAM-") == 0U )
		return std::string() ;

	Response rsp = response( m , m=="LOGIN"?std::string(login_challenge_1):std::string() ) ;
	if( rsp.error || rsp.data.size() > limit )
		return std::string() ;
	else
		return rsp.data ;
}

GAuth::SaslClient::Response GAuth::SaslClientImp::response( const std::string & mechanism ,
	const std::string & challenge ) const
{
	Response rsp ;
	rsp.sensitive = true ;
	rsp.error = true ;
	rsp.final = false ;

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
		rsp.error = !secret.valid() ;
		rsp.final = true ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_1 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp.data = secret.id() ;
		rsp.error = !secret.valid() ;
		rsp.final = false ;
		rsp.sensitive = false ; // userid
	}
	else if( mechanism == LOGIN && challenge == login_challenge_2 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp.data = secret.key() ;
		rsp.error = !secret.valid() ;
		rsp.final = true ;
	}
	else if( mechanism == "XOAUTH2" && challenge.empty() )
	{
		secret = m_secrets.clientSecret( "oauth" ) ;
		rsp.data = secret.key() ;
		rsp.error = !secret.valid() ;
		rsp.final = true ; // not always -- may get an informational challenge
	}
	else if( mechanism == "XOAUTH2" )
	{
		secret = m_secrets.clientSecret( "oauth" ) ;
		rsp.data.clear() ; // information-only challenge gets an empty response
		rsp.error = false ;
		rsp.final = true ;
		rsp.sensitive = false ; // information-only
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

GAuth::SaslClient::SaslClient( const SaslClientSecrets & secrets , const std::string & config ) :
	m_imp(std::make_unique<SaslClientImp>(secrets,config))
{
}

GAuth::SaslClient::~SaslClient()
= default;

bool GAuth::SaslClient::active() const
{
	return m_imp->active() ;
}

GAuth::SaslClient::Response GAuth::SaslClient::response( const std::string & mechanism , const std::string & challenge ) const
{
	return m_imp->response( mechanism , challenge ) ;
}

std::string GAuth::SaslClient::initialResponse( std::size_t limit ) const
{
	return m_imp->initialResponse( limit ) ;
}

std::string GAuth::SaslClient::mechanism( const G::StringArray & server_mechanisms ) const
{
	return m_imp->mechanism( server_mechanisms ) ;
}

bool GAuth::SaslClient::next()
{
	return m_imp->next() ;
}

std::string GAuth::SaslClient::next( const std::string & s )
{
	if( s.empty() ) return s ;
	return m_imp->next() ? mechanism() : std::string() ;
}

std::string GAuth::SaslClient::mechanism() const
{
	return m_imp->mechanism() ;
}

std::string GAuth::SaslClient::id() const
{
	return m_imp->id() ;
}

std::string GAuth::SaslClient::info() const
{
	return m_imp->info() ;
}

