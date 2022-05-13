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
/// \file gsaslserverbasic.cpp
///

#include "gdef.h"
#include "gsaslserverbasic.h"
#include "gmd5.h"
#include "ghash.h"
#include "gcram.h"
#include "gbase64.h"
#include "gstr.h"
#include "gtest.h"
#include "gdatetime.h"
#include "grandom.h"
#include "glog.h"
#include "gassert.h"
#include <sstream>
#include <algorithm>
#include <functional>
#include <utility>

//| \class GAuth::SaslServerBasicImp
/// A private pimple-pattern implementation class used by GAuth::SaslServerBasic.
///
class GAuth::SaslServerBasicImp
{
public:
	SaslServerBasicImp( const SaslServerSecrets & , bool , const std::string & ,
		std::pair<bool,std::string> , std::pair<bool,std::string> ) ;
	G::StringArray mechanisms( bool ) const ;
	void reset() ;
	bool init( bool , const std::string & mechanism ) ;
	std::string mechanism() const ;
	std::string preferredMechanism( bool ) const ;
	std::string initialChallenge() const ;
	std::string apply( const std::string & response , bool & done ) ;
	bool trusted( const GNet::Address & ) const ;
	bool trustedCore( const std::string & , const GNet::Address & ) const ;
	std::string id() const ;
	bool authenticated() const ;
	static void filter( G::StringArray & ,
		std::pair<bool,std::string> , const std::string & = std::string() ) ;

private:
	bool m_first_apply ;
	const SaslServerSecrets & m_secrets ;
	G::StringArray m_mechanisms_secure ;
	G::StringArray m_mechanisms_insecure ;
	std::string m_mechanism ;
	std::string m_challenge ;
	bool m_authenticated ;
	std::string m_id ;
	std::string m_trustee ;
	static const char * login_challenge_1 ;
	static const char * login_challenge_2 ;
} ;

const char * GAuth::SaslServerBasicImp::login_challenge_1 = "Username:" ;
const char * GAuth::SaslServerBasicImp::login_challenge_2 = "Password:" ;

// ===

GAuth::SaslServerBasicImp::SaslServerBasicImp( const SaslServerSecrets & secrets ,
	bool with_apop , const std::string & config ,
	std::pair<bool,std::string> config_secure ,
	std::pair<bool,std::string> config_insecure ) :
		m_first_apply(true) ,
		m_secrets(secrets) ,
		m_authenticated(false)
{
	// prepare a list of mechanisms, but remove any that are completely unusable
	G::StringArray mechanisms ;
	{
		// if there are any plain secrets then all mechanisms are usable
		if( secrets.contains("PLAIN",std::string()) )
		{
			mechanisms = Cram::hashTypes( "CRAM-" ) ;
			mechanisms.push_back( "PLAIN" ) ;
			mechanisms.push_back( "LOGIN" ) ;
		}
		else
		{
			for( const auto & cram : Cram::hashTypes(std::string(),true) )
			{
				// if there are any CRAM-X secrets then enable the CRAM-X mechansism
				if( secrets.contains(cram,std::string()) )
					mechanisms.push_back( "CRAM-" + cram ) ;
			}
		}
		if( with_apop )
			mechanisms.push_back( "APOP" ) ;
	}

	m_mechanisms_secure = mechanisms ;
	m_mechanisms_insecure = mechanisms ;

	if( m_mechanisms_secure.empty() && secrets.valid() )
	{
		// RFC-4954 4 p6
		m_mechanisms_secure.push_back( "PLAIN" ) ;
	}

	G::StringArray config_list = G::Str::splitIntoTokens( G::Str::upper(config) , ";" ) ;

	// M: for secure/insecure (M)echanisms to allow
	// X: for secure/insecure mechanisms to e(X)clude
	// A: for the insecure (A)llow list
	// D: for the insecure (D)eny list
	std::pair<bool,std::string> m ;
	m.first = G::Str::headMatch( config_list , "M:" ) ;
	m.second = G::Str::headMatchResidue( config_list , "M:" ) ;
	std::string x = G::Str::headMatchResidue( config_list , "X:" ) ;
	std::pair<bool,std::string> a ;
	a.first = G::Str::headMatch( config_list , "A:" ) ;
	a.second = G::Str::headMatchResidue( config_list , "A:" ) ;
	std::string d = G::Str::headMatchResidue( config_list , "D:" ) ;

	filter( m_mechanisms_secure , m , x ) ;
	filter( m_mechanisms_insecure , m , x ) ;
	filter( m_mechanisms_insecure , a , d ) ;
	filter( m_mechanisms_secure , config_secure ) ;
	filter( m_mechanisms_insecure , config_insecure ) ;

	if( m_mechanisms_secure.empty() && m_mechanisms_insecure.empty() )
	{
		G_DEBUG( "GAuth::SaslServerBasicImp::ctor: no authentication mechanisms" ) ; // was throw
	}
}

void GAuth::SaslServerBasicImp::reset()
{
	m_first_apply = true ;
	m_authenticated = false ;
	m_id.clear() ;
	m_trustee.clear() ;
	m_challenge.clear() ;
	m_mechanism.clear() ;
}

void GAuth::SaslServerBasicImp::filter( G::StringArray & mechanisms ,
	std::pair<bool,std::string> allow_pair , const std::string & deny )
{
	G::StringArray allowlist = G::Str::splitIntoTokens( allow_pair.second , "," ) ;
	if( allow_pair.first && allowlist.empty() )
		mechanisms.clear() ;
	else
		mechanisms.erase( G::Str::keepMatch( mechanisms.begin() , mechanisms.end() , allowlist , true ) , mechanisms.end() ) ;

	G::StringArray denylist = G::Str::splitIntoTokens( deny , "," ) ;
	mechanisms.erase( G::Str::removeMatch( mechanisms.begin() , mechanisms.end() , denylist , true ) , mechanisms.end() ) ;
}

G::StringArray GAuth::SaslServerBasicImp::mechanisms( bool secure ) const
{
	return secure ? m_mechanisms_secure : m_mechanisms_insecure ;
}

bool GAuth::SaslServerBasicImp::init( bool secure , const std::string & mechanism_in )
{
	reset() ;

	std::string mechanism = G::Str::upper( mechanism_in ) ;
	const G::StringArray & mechanisms = secure ? m_mechanisms_secure : m_mechanisms_insecure ;

	if( mechanism.empty() || std::find(mechanisms.begin(),mechanisms.end(),mechanism) == mechanisms.end() )
	{
		G_DEBUG( "GAuth::SaslServerBasicImp::init: requested mechanism [" << mechanism << "] is not in our list" ) ;
		return false ;
	}
	else if( mechanism == "APOP" || mechanism.find("CRAM-") == 0U )
	{
		m_mechanism = mechanism ;
		m_challenge = Cram::challenge( G::Random::rand() ) ;
		return true ;
	}
	else
	{
		m_mechanism = mechanism ;
		return true ;
	}
}

std::string GAuth::SaslServerBasicImp::preferredMechanism( bool secure ) const
{
	if( !m_id.empty() )
	{
		auto mechanism_list = mechanisms( secure ) ;
		std::reverse( mechanism_list.begin() , mechanism_list.end() ) ;
		for( const auto & m : mechanism_list )
		{
			if( G::Str::headMatch( m , "CRAM-" ) )
			{
				std::string type = G::Str::lower( m.substr(5U) ) ; // eg. "sha1"
				if( m_secrets.contains( type , m_id ) )
					return m ;
			}
		}
	}
	return std::string() ;
}

std::string GAuth::SaslServerBasicImp::initialChallenge() const
{
	// see RFC-4422 section 5
	if( m_mechanism == "PLAIN" ) // "client-first"
		return std::string() ;
	else if( m_mechanism == "LOGIN" ) // "variable"
		return login_challenge_1 ;
	else // APOP/CRAM-X "server-first"
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
	G::StringArray wildcards = address.wildcards() ;
	return std::any_of( wildcards.cbegin() , wildcards.cend() ,
		[&](const std::string &wca){return trustedCore(wca,address);} ) ;
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

GAuth::SaslServerBasic::SaslServerBasic( const SaslServerSecrets & secrets ,
	bool with_apop , const std::string & config , std::pair<bool,std::string> config_secure ,
	std::pair<bool,std::string> config_insecure ) :
		m_imp(std::make_unique<SaslServerBasicImp>(secrets,with_apop,config,std::move(config_secure),std::move(config_insecure)))
{
}

GAuth::SaslServerBasic::~SaslServerBasic()
= default ;

G::StringArray GAuth::SaslServerBasic::mechanisms( bool secure ) const
{
	return m_imp->mechanisms( secure ) ;
}

std::string GAuth::SaslServerBasic::mechanism() const
{
	return m_imp->mechanism() ;
}

std::string GAuth::SaslServerBasic::preferredMechanism( bool secure ) const
{
	return m_imp->preferredMechanism( secure ) ;
}

bool GAuth::SaslServerBasic::trusted( const GNet::Address & address ) const
{
	return m_imp->trusted( address ) ;
}

void GAuth::SaslServerBasic::reset()
{
	return m_imp->reset() ;
}

bool GAuth::SaslServerBasic::init( bool secure , const std::string & mechanism )
{
	return m_imp->init( secure , mechanism ) ;
}

bool GAuth::SaslServerBasic::mustChallenge() const
{
	const bool plain = G::Str::imatch( m_imp->mechanism() , "PLAIN" ) ;
	const bool login = !plain && G::Str::imatch( m_imp->mechanism() , "LOGIN" ) ;
	return !plain && !login ;
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

