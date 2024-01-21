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
/// \file gsaslclient.cpp
///

#include "gdef.h"
#include "gsaslclient.h"
#include "gstringview.h"
#include "gstringlist.h"
#include "gssl.h"
#include "gmd5.h"
#include "ghash.h"
#include "gcram.h"
#include "gbase64.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>

//| \class GAuth::SaslClientImp
/// A private pimple-pattern implementation class used by GAuth::SaslClient.
///
class GAuth::SaslClientImp
{
public:
	using Response = SaslClient::Response ;
	SaslClientImp( const SaslClientSecrets & , const std::string & ) ;
	bool validSelector( G::string_view ) const ;
	bool mustAuthenticate( G::string_view ) const ;
	std::string mechanism( const G::StringArray & , G::string_view ) const ;
	Response initialResponse( G::string_view selector , std::size_t ) const ;
	Response response( G::string_view mechanism , G::string_view challenge , G::string_view selector ) const ;
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
	static G::string_view login_challenge_1 ;
	static G::string_view login_challenge_2 ;
} ;

G::string_view GAuth::SaslClientImp::login_challenge_1 { "Username:" , 9U } ;
G::string_view GAuth::SaslClientImp::login_challenge_2 { "Password:" , 9U } ;

// ===

GAuth::SaslClientImp::SaslClientImp( const SaslClientSecrets & secrets ,
	const std::string & sasl_client_config ) :
		m_secrets(secrets) ,
		m_config(sasl_client_config) ,
		PLAIN("PLAIN") ,
		LOGIN("LOGIN")
{
}

std::string GAuth::SaslClientImp::mechanism( const G::StringArray & server_mechanisms , G::string_view selector ) const
{
	// if we have a plaintext password then we can use any cram
	// mechanism for which we have a hash function -- otherwise
	// we can use cram mechanisms where we have a hashed password
	// of the correct type and the hash function is capable of
	// initialisation with an intermediate state
	//
	G::StringArray our_list ;
	if( m_secrets.clientSecret("plain",selector).valid() )
	{
		our_list = Cram::hashTypes( "CRAM-" , false ) ;
	}
	else
	{
		our_list = Cram::hashTypes( "CRAM-" , true ) ;
		for( auto p = our_list.begin() ; p != our_list.end() ; )
		{
			std::string type = (*p).substr( 5U ) ;
			if( m_secrets.clientSecret(type,selector).valid() )
				++p ;
			else
				p = our_list.erase( p ) ;
		}
	}
	if( m_secrets.clientSecret("oauth",selector).valid() )
	{
		our_list.emplace_back( "XOAUTH2" ) ;
	}
	if( m_secrets.clientSecret("plain",selector).valid() )
	{
		our_list.push_back( PLAIN ) ;
		our_list.push_back( LOGIN ) ;
	}

	// use the configuration string as a mechanism whitelist and/or blocklist
	if( !m_config.empty() )
	{
		bool simple = G::StringList::imatch( our_list , m_config ) ; // eg. allow "plain" as well as "m:plain"
		G::StringArray list = G::Str::splitIntoTokens( G::Str::upper(m_config) , ";" ) ;
		G::StringArray whitelist = G::Str::splitIntoTokens( simple ? G::Str::upper(m_config) : G::StringList::headMatchResidue( list , "M:" ) , "," ) ;
		G::StringArray blocklist = G::Str::splitIntoTokens( G::StringList::headMatchResidue( list , "X:" ) , "," ) ;
		G::StringList::keepMatch( our_list , whitelist , G::StringList::Ignore::Case ) ;
		G::StringList::removeMatch( our_list , blocklist , G::StringList::Ignore::Case ) ;
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

GAuth::SaslClient::Response GAuth::SaslClientImp::initialResponse( G::string_view selector , std::size_t limit ) const
{
	// (the implementation of response() is stateless because it can derive
	// state from the challege, so we doesn't need to worry here about
	// side-effects between initialResponse() and response())

	if( m_mechanisms.empty() || m_mechanisms[0U].find("CRAM-") == 0U )
		return {} ;

	const std::string & m = m_mechanisms[0] ;
	Response rsp = response( m , m == "LOGIN" ? login_challenge_1 : G::string_view() , selector ) ;
	if( rsp.error || rsp.data.size() > limit )
		return {} ;
	else
		return rsp ;
}

GAuth::SaslClient::Response GAuth::SaslClientImp::response( G::string_view mechanism ,
	G::string_view challenge , G::string_view selector ) const
{
	Response rsp ;
	rsp.sensitive = true ;
	rsp.error = true ;
	rsp.final = false ;

	Secret secret = Secret::none() ;
	if( mechanism.find("CRAM-") == 0U )
	{
		G::string_view hash_type = mechanism.substr( 5U ) ;
		secret = m_secrets.clientSecret( hash_type , selector ) ;
		if( !secret.valid() )
			secret = m_secrets.clientSecret( "plain" , selector ) ;
		rsp.data = Cram::response( hash_type , true , secret , challenge , secret.id() ) ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == "APOP"_sv )
	{
		secret = m_secrets.clientSecret( "MD5"_sv , selector ) ;
		rsp.data = Cram::response( "MD5"_sv , false , secret , challenge , secret.id() ) ;
		rsp.error = rsp.data.empty() ;
		rsp.final = true ;
	}
	else if( mechanism == PLAIN )
	{
		secret = m_secrets.clientSecret( "plain"_sv , selector ) ;
		rsp.data = std::string(1U,'\0').append(secret.id()).append(1U,'\0').append(secret.secret()) ;
		rsp.error = !secret.valid() ;
		rsp.final = true ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_1 )
	{
		secret = m_secrets.clientSecret( "plain"_sv , selector ) ;
		rsp.data = secret.id() ;
		rsp.error = !secret.valid() ;
		rsp.final = false ;
		rsp.sensitive = false ; // userid
	}
	else if( mechanism == LOGIN && challenge == login_challenge_2 )
	{
		secret = m_secrets.clientSecret( "plain"_sv , selector ) ;
		rsp.data = secret.secret() ;
		rsp.error = !secret.valid() ;
		rsp.final = true ;
	}
	else if( mechanism == "XOAUTH2"_sv && challenge.empty() )
	{
		secret = m_secrets.clientSecret( "oauth"_sv , selector ) ;
		rsp.data = secret.secret() ;
		rsp.error = !secret.valid() ;
		rsp.final = true ; // not always -- may get an informational challenge
	}
	else if( mechanism == "XOAUTH2"_sv )
	{
		secret = m_secrets.clientSecret( "oauth"_sv , selector ) ;
		rsp.data.clear() ; // information-only challenge gets an empty response
		rsp.error = false ;
		rsp.final = true ;
		rsp.sensitive = false ; // information-only
	}

	if( rsp.final )
	{
		m_info
			.assign("using mechanism [")
			.append(G::Str::lower(G::sv_to_string(mechanism)))
			.append("] and ",6U)
			.append(secret.info()) ;
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

bool GAuth::SaslClientImp::validSelector( G::string_view selector ) const
{
	return m_secrets.validSelector( selector ) ;
}

bool GAuth::SaslClientImp::mustAuthenticate( G::string_view selector ) const
{
	return m_secrets.mustAuthenticate( selector ) ;
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

bool GAuth::SaslClient::validSelector( G::string_view selector ) const
{
	return m_imp->validSelector( selector ) ;
}

bool GAuth::SaslClient::mustAuthenticate( G::string_view selector ) const
{
	return m_imp->mustAuthenticate( selector ) ;
}

GAuth::SaslClient::Response GAuth::SaslClient::response( G::string_view mechanism ,
	G::string_view challenge , G::string_view selector ) const
{
	return m_imp->response( mechanism , challenge , selector ) ;
}

GAuth::SaslClient::Response GAuth::SaslClient::initialResponse( G::string_view selector , std::size_t limit ) const
{
	return m_imp->initialResponse( selector , limit ) ;
}

std::string GAuth::SaslClient::mechanism( const G::StringArray & server_mechanisms , G::string_view selector ) const
{
	return m_imp->mechanism( server_mechanisms , selector ) ;
}

bool GAuth::SaslClient::next()
{
	return m_imp->next() ;
}

#ifndef G_LIB_SMALL
std::string GAuth::SaslClient::next( const std::string & s )
{
	if( s.empty() ) return s ;
	return m_imp->next() ? mechanism() : std::string() ;
}
#endif

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

