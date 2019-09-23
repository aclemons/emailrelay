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
// gsaslserverbasic.cpp
//

#include "gdef.h"
#include "gsaslserverbasic.h"
#include "gmd5.h"
#include "ghash.h"
#include "gcram.h"
#include "gbase64.h"
#include "gstr.h"
#include "gtest.h"
#include "gdatetime.h"
#include "glog.h"
#include "gassert.h"
#include <sstream>
#include <algorithm>
#include <functional>

namespace
{
	const char * login_challenge_1 = "Username:" ;
	const char * login_challenge_2 = "Password:" ;
}

/// \class GAuth::SaslServerBasicImp
/// A private pimple-pattern implementation class used by GAuth::SaslServerBasic.
///
class GAuth::SaslServerBasicImp
{
public:
	SaslServerBasicImp( const SaslServerSecrets & , const std::string & , bool ) ;
	std::string mechanisms( const std::string & ) const ;
	bool init( const std::string & mechanism ) ;
	std::string mechanism() const ;
	std::string initialChallenge() const ;
	std::string apply( const std::string & response , bool & done ) ;
	bool trusted( const GNet::Address & ) const ;
	bool trustedCore( const std::string & , const GNet::Address & ) const ;
	bool active() const ;
	std::string id() const ;
	bool authenticated() const ;

private:
	bool m_allow_apop ;
	bool m_first_apply ;
	const SaslServerSecrets & m_secrets ;
	G::StringArray m_mechanisms ; // that have secrets
	std::string m_mechanism ;
	std::string m_challenge ;
	bool m_authenticated ;
	std::string m_id ;
	std::string m_trustee ;
} ;

// ===

GAuth::SaslServerBasicImp::SaslServerBasicImp( const SaslServerSecrets & secrets , const std::string & sasl_server_config , bool allow_apop ) :
	m_allow_apop(allow_apop) ,
	m_first_apply(true) ,
	m_secrets(secrets) ,
	m_authenticated(false)
{
	m_mechanisms = Cram::hashTypes( "CRAM-" ) ;
	m_mechanisms.push_back( "PLAIN" ) ;
	m_mechanisms.push_back( "LOGIN" ) ;
	if( G::Test::enabled("sasl-server-oauth") )
		m_mechanisms.push_back( "XOAUTH2" ) ; // to allow testing of the client-side

	// use the configuration string as a mechanism whitelist and/or blacklist
	if( !sasl_server_config.empty() )
	{
		G::StringArray list = G::Str::splitIntoTokens( G::Str::upper(sasl_server_config) , ";" ) ;
		G::StringArray whitelist = G::Str::splitIntoTokens( G::Str::headMatchResidue( list , "M:" ) , "," ) ;
		G::StringArray blacklist = G::Str::splitIntoTokens( G::Str::headMatchResidue( list , "X:" ) , "," ) ;
		m_mechanisms.erase( G::Str::keepMatch( m_mechanisms.begin() , m_mechanisms.end() , whitelist , true ) , m_mechanisms.end() ) ;
		m_mechanisms.erase( G::Str::removeMatch( m_mechanisms.begin() , m_mechanisms.end() , blacklist , true ) , m_mechanisms.end() ) ;
		if( m_mechanisms.empty() )
			throw SaslServerBasic::NoMechanisms() ;
	}
}

std::string GAuth::SaslServerBasicImp::mechanisms( const std::string & sep ) const
{
	return G::Str::join( sep , m_mechanisms ) ;
}

bool GAuth::SaslServerBasicImp::init( const std::string & mechanism_in )
{
	m_first_apply = true ;
	m_authenticated = false ;
	m_id.erase() ;
	m_trustee.erase() ;
	m_challenge.erase() ;
	m_mechanism.erase() ;

	std::string mechanism = G::Str::upper( mechanism_in ) ;
	if( m_allow_apop && mechanism == "APOP" )
	{
		m_mechanism = mechanism ;
		m_challenge = Cram::challenge() ;
		return true ;
	}
	else if( std::find(m_mechanisms.begin(),m_mechanisms.end(),mechanism) == m_mechanisms.end() )
	{
		G_DEBUG( "GAuth::SaslServerBasicImp::init: requested mechanism [" << mechanism << "] is not in our list" ) ;
		return false ;
	}
	else if( mechanism.find("CRAM-") == 0U )
	{
		m_mechanism = mechanism ;
		m_challenge = Cram::challenge() ;
		return true ;
	}
	else
	{
		m_mechanism = mechanism ;
		return true ;
	}
}

std::string GAuth::SaslServerBasicImp::initialChallenge() const
{
	// see RFC-4422 section 5
	if( m_mechanism == "PLAIN" ) // "client-first"
		return std::string() ;
	else if( m_mechanism == "XOAUTH2" ) // for testing -- "client-first"
		return std::string() ;
	else if( m_mechanism == "LOGIN" ) // "variable"
		return login_challenge_1 ;
	else // CRAM-X "server-first"
		return m_challenge ;
}

std::string GAuth::SaslServerBasicImp::apply( const std::string & response , bool & done )
{
	G_DEBUG( "GAuth::SaslServerBasic::apply: response: \"" << G::Str::printable(response) << "\"" ) ;

	bool first_apply = m_first_apply ;
	m_first_apply = false ;

	done = false ;
	std::string id ;
	Secret secret = Secret::none() ;
	std::string next_challenge ;

	if( m_mechanism.find("CRAM-") == 0U || m_mechanism == "APOP" )
	{
		id = Cram::id( response ) ;
		secret = Secret::none() ;
		if( !id.empty() )
		{
			if( m_mechanism == "APOP" )
			{
				secret = m_secrets.serverSecret( "plain" , id ) ; // (APOP is MD5 but not HMAC)
			}
			else
			{
				std::string hash_type = m_mechanism.substr(5U) ;
				secret = m_secrets.serverSecret( hash_type , id ) ;
				if( !secret.valid() )
					secret = m_secrets.serverSecret( "plain" , id ) ;
			}
		}
		if( !secret.valid() )
		{
			m_authenticated = false ;
		}
		else
		{
			m_id = id ;
			m_authenticated =
				m_mechanism == "APOP" ?
					Cram::validate( "MD5" , false , secret , m_challenge , response ) :
					Cram::validate( m_mechanism.substr(5U) , true , secret , m_challenge , response ) ;
		}
		done = true ;
	}
	else if( m_mechanism == "PLAIN" )
	{
		// PLAIN has a single response containing three nul-separated fields
		std::string sep( 1U , '\0' ) ;
		std::string id_pwd_in = G::Str::tail( response , sep ) ;
		id = G::Str::head( id_pwd_in , sep ) ;
		std::string pwd_in = G::Str::tail( id_pwd_in , sep ) ;
		secret = m_secrets.serverSecret( "plain" , id ) ;

		m_authenticated = secret.valid() && !id.empty() && !pwd_in.empty() && pwd_in == secret.key() ;
		m_id = id ;
		done = true ;
	}
	else if( m_mechanism == "XOAUTH2" && first_apply ) // for testing
	{
		if( G::Test::enabled("sasl-server-oauth-pass") )
		{
			m_id = "test" ;
			m_authenticated = true ;
			done = true ;
		}
		else
		{
			next_challenge = "not authenticated, send empty response" ;
			done = false ;
		}
	}
	else if( m_mechanism == "XOAUTH2" ) // for testing
	{
		m_authenticated = false ;
		done = true ;
	}
	else if( first_apply ) // LOGIN username
	{
		// LOGIN uses two prompts; the first response is the username and the second is the password
		G_ASSERT( m_mechanism == "LOGIN" ) ;
		id = m_id = response ;
		if( !m_id.empty() )
			next_challenge = login_challenge_2 ;
	}
	else // LOGIN password
	{
		G_ASSERT( m_mechanism == "LOGIN" ) ;
		id = m_id ;
		secret = m_secrets.serverSecret( "plain" , m_id ) ;
		m_authenticated = secret.valid() && !response.empty() && response == secret.key() ;
		done = true ;
	}

	if( done )
	{
		std::ostringstream ss ;
		ss
			<< (m_authenticated?"successful":"failed") << " authentication of remote client using mechanism ["
			<< G::Str::lower(m_mechanism) << "] and " << secret.info(id) ;
		if( m_authenticated )
			G_LOG( "GAuth::SaslServerBasicImp::apply: " << ss.str() ) ;
		else
			G_WARNING( "GAuth::SaslServerBasicImp::apply: " << ss.str() ) ;
	}

	return next_challenge ;
}

bool GAuth::SaslServerBasicImp::trusted( const GNet::Address & address ) const
{
	G_DEBUG( "GAuth::SaslServerBasicImp::trusted: \"" << address.hostPartString() << "\"" ) ;
	G::StringArray wc = address.wildcards() ;
	for( G::StringArray::iterator p = wc.begin() ; p != wc.end() ; ++p )
	{
		if( trustedCore(*p,address) )
			return true ;
	}
	return false ;
}

bool GAuth::SaslServerBasicImp::trustedCore( const std::string & address_wildcard , const GNet::Address & address ) const
{
	G_DEBUG( "GAuth::SaslServerBasicImp::trustedCore: \"" << address_wildcard << "\", \"" << address.hostPartString() << "\"" ) ;
	std::pair<std::string,std::string> pair = m_secrets.serverTrust( address_wildcard ) ;
	std::string & trustee = pair.first ;
	if( !trustee.empty() )
	{
		G_LOG( "GAuth::SaslServer::trusted: trusting [" << address.hostPartString() << "]: "
			<< "matched [" << address_wildcard << "] from " << pair.second ) ;
		const_cast<SaslServerBasicImp*>(this)->m_trustee = trustee ;
		return true ;
	}
	else
	{
		return false ;
	}
}

bool GAuth::SaslServerBasicImp::active() const
{
	return m_secrets.valid() ;
}

std::string GAuth::SaslServerBasicImp::mechanism() const
{
	return m_mechanism ;
}

std::string GAuth::SaslServerBasicImp::id() const
{
	return m_authenticated ? m_id : m_trustee ;
}

bool GAuth::SaslServerBasicImp::authenticated() const
{
	return m_authenticated ;
}

// ===

GAuth::SaslServerBasic::SaslServerBasic( const SaslServerSecrets & secrets , const std::string & config , bool allow_apop ) :
	m_imp(new SaslServerBasicImp(secrets,config,allow_apop))
{
}

GAuth::SaslServerBasic::~SaslServerBasic()
{
	delete m_imp ;
}

std::string GAuth::SaslServerBasic::mechanisms( char c ) const
{
	return m_imp->mechanisms( std::string(1U,c) ) ;
}

std::string GAuth::SaslServerBasic::mechanism() const
{
	return m_imp->mechanism() ;
}

bool GAuth::SaslServerBasic::trusted( const GNet::Address & address ) const
{
	return m_imp->trusted( address ) ;
}

bool GAuth::SaslServerBasic::active() const
{
	return m_imp->active() ;
}

bool GAuth::SaslServerBasic::init( const std::string & mechanism )
{
	return m_imp->init( mechanism ) ;
}

bool GAuth::SaslServerBasic::mustChallenge() const
{
	std::string m = G::Str::upper( m_imp->mechanism() ) ; // upper() just in case
	return m != "PLAIN" && m != "LOGIN" && m != "XOAUTH2" ;
}

std::string GAuth::SaslServerBasic::initialChallenge() const
{
	return m_imp->initialChallenge() ;
}

std::string GAuth::SaslServerBasic::apply( const std::string & response , bool & done )
{
	return m_imp->apply( response , done ) ;
}

bool GAuth::SaslServerBasic::authenticated() const
{
	return m_imp->authenticated() ;
}

std::string GAuth::SaslServerBasic::id() const
{
	return m_imp->id() ;
}

bool GAuth::SaslServerBasic::requiresEncryption() const
{
	return false ;
}

