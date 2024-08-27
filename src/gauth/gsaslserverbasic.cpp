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
#include "gstringview.h"
#include "gstringlist.h"
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
	SaslServerBasicImp( const SaslServerSecrets & , bool ,
		const std::string & config , const std::string & challenge_domain ) ;
	G::StringArray mechanisms( bool ) const ;
	void reset() ;
	bool init( bool , const std::string & mechanism ) ;
	std::string mechanism() const ;
	std::string preferredMechanism( bool ) const ;
	std::string initialChallenge() const ;
	std::string apply( const std::string & response , bool & done ) ;
	bool trusted( const G::StringArray & , const std::string & ) const ;
	bool trustedCore( const std::string & , const std::string & ) const ;
	std::string id() const ;
	bool authenticated() const ;

public:
	~SaslServerBasicImp() = default ;
	SaslServerBasicImp( const SaslServerBasicImp & ) = delete ;
	SaslServerBasicImp( SaslServerBasicImp && ) = delete ;
	SaslServerBasicImp & operator=( const SaslServerBasicImp & ) = delete ;
	SaslServerBasicImp & operator=( SaslServerBasicImp && ) = delete ;

private:
	bool m_first_apply {true} ;
	const SaslServerSecrets & m_secrets ;
	G::StringArray m_mechanisms_secure ;
	G::StringArray m_mechanisms_insecure ;
	std::string m_mechanism ;
	std::string m_challenge ;
	std::string m_challenge_domain ;
	bool m_authenticated {false} ;
	std::string m_id ;
	std::string m_trustee ;
	static std::string_view login_challenge_1 ;
	static std::string_view login_challenge_2 ;
} ;

std::string_view GAuth::SaslServerBasicImp::login_challenge_1 {"Username:",9U} ;
std::string_view GAuth::SaslServerBasicImp::login_challenge_2 {"Password:",9U} ;

// ===

GAuth::SaslServerBasicImp::SaslServerBasicImp( const SaslServerSecrets & secrets ,
	bool with_apop , const std::string & config , const std::string & challenge_domain ) :
		m_secrets(secrets) ,
		m_challenge_domain(challenge_domain)
{
	// prepare a list of mechanisms, but remove any that are completely unusable
	G::StringArray mechanisms ;
	{
		// if there are any plain secrets then all mechanisms are usable
		if( secrets.contains( "PLAIN" , {} ) )
		{
			mechanisms = Cram::hashTypes( "CRAM-" ) ;
			mechanisms.emplace_back( "PLAIN" ) ;
			mechanisms.emplace_back( "LOGIN" ) ;
		}
		else
		{
			// if there are any CRAM-X secrets then enable the CRAM-X mechansism
			for( const auto & cram : Cram::hashTypes({},true) )
			{
				if( secrets.contains( cram , {} ) )
					mechanisms.push_back( std::string("CRAM-").append(cram) ) ;
			}
		}
		if( with_apop )
			mechanisms.emplace_back( "APOP" ) ;
	}

	m_mechanisms_secure = mechanisms ;
	m_mechanisms_insecure = mechanisms ;

	// RFC-4954 4 p6 -- PLAIN is always an option when secure
	if( m_mechanisms_secure.empty() && secrets.valid() )
	{
		m_mechanisms_secure.emplace_back( "PLAIN" ) ;
	}

	// apply the allow/deny configuration
	//
	// backwards compatibility if no A or D:
	// M: for secure/insecure (M)echanisms to allow
	// X: for secure/insecure mechanisms to e(X)clude
	//
	// new behaviour:
	// M: for insecure (M)echanisms to allow
	// X: for insecure mechanisms to e(X)clude
	// A: for the secure (A)llow list
	// D: for the secure (D)eny list
	//
	// eg: "m:;a:plain,login"
	//
	G::StringArray config_list = G::Str::splitIntoTokens( G::Str::upper(config) , ";" ) ;
	bool have_m = G::StringList::headMatch( config_list , "M:" ) ;
	bool have_a = G::StringList::headMatch( config_list , "A:" ) ;
	bool have_d = G::StringList::headMatch( config_list , "D:" ) ;
	std::string m = G::StringList::headMatchResidue( config_list , "M:" ) ;
	std::string x = G::StringList::headMatchResidue( config_list , "X:" ) ;
	std::string a = G::StringList::headMatchResidue( config_list , "A:" ) ;
	std::string d = G::StringList::headMatchResidue( config_list , "D:" ) ;
	auto m_ = have_m ? std::optional<std::string>(m) : std::optional<std::string>() ;
	auto a_ = have_a ? std::optional<std::string>(a) : std::optional<std::string>() ;
	if( have_a || have_d )
	{
		G::StringList::Filter(m_mechanisms_insecure).allow(m_).deny(x) ;
		G::StringList::Filter(m_mechanisms_secure).allow(a_).deny(d) ;
	}
	else
	{
		G::StringList::Filter(m_mechanisms_insecure).allow(m_).deny(x) ;
		G::StringList::Filter(m_mechanisms_secure).allow(m_).deny(x) ;
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
		m_challenge = Cram::challenge( G::Random::rand() , m_challenge_domain ) ;
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
	return {} ;
}

std::string GAuth::SaslServerBasicImp::initialChallenge() const
{
	// see RFC-4422 section 5
	if( m_mechanism == "PLAIN" ) // "client-first"
		return {} ;
	else if( m_mechanism == "LOGIN" ) // "variable"
		return G::sv_to_string(login_challenge_1) ;
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
				secret = m_secrets.serverSecret( "plain"_sv , id ) ; // (APOP is MD5 but not HMAC)
			}
			else
			{
				std::string hash_type = m_mechanism.substr(5U) ;
				secret = m_secrets.serverSecret( hash_type , id ) ;
				if( !secret.valid() )
					secret = m_secrets.serverSecret( "plain"_sv , id ) ;
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
		std::string_view sep( "\0" , 1U ) ;
		std::string_view id_pwd_in = G::Str::tailView( response , sep ) ;
		id = G::Str::head( id_pwd_in , sep ) ;
		std::string_view pwd_in = G::Str::tailView( id_pwd_in , sep ) ;
		secret = m_secrets.serverSecret( "plain"_sv , id ) ;

		m_authenticated = secret.valid() && !id.empty() && !pwd_in.empty() && pwd_in == secret.secret() ;
		m_id = id ;
		done = true ;
	}
	else if( first_apply ) // LOGIN username
	{
		// LOGIN uses two prompts; the first response is the username and the second is the password
		G_ASSERT( m_mechanism == "LOGIN" ) ;
		id = m_id = response ;
		if( !m_id.empty() )
			next_challenge = G::sv_to_string(login_challenge_2) ;
	}
	else // LOGIN password
	{
		G_ASSERT( m_mechanism == "LOGIN" ) ;
		id = m_id ;
		secret = m_secrets.serverSecret( "plain"_sv , m_id ) ;
		m_authenticated = secret.valid() && !response.empty() && response == secret.secret() ;
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

bool GAuth::SaslServerBasicImp::trusted( const G::StringArray & address_wildcards , const std::string & address_display ) const
{
	return std::any_of( address_wildcards.cbegin() , address_wildcards.cend() ,
		[&](const std::string &wca){return trustedCore(wca,address_display);} ) ;
}

bool GAuth::SaslServerBasicImp::trustedCore( const std::string & address_wildcard , const std::string & address_display ) const
{
	G_DEBUG( "GAuth::SaslServerBasicImp::trustedCore: \"" << address_wildcard << "\", \"" << address_display << "\"" ) ;
	std::pair<std::string,std::string> pair = m_secrets.serverTrust( address_wildcard ) ;
	std::string & trustee = pair.first ;
	if( !trustee.empty() )
	{
		G_LOG( "GAuth::SaslServer::trusted: trusting [" << address_display << "]: "
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

GAuth::SaslServerBasic::SaslServerBasic( const SaslServerSecrets & secrets , bool with_apop ,
	const std::string & config , const std::string & challenge_domain ) :
		m_imp(std::make_unique<SaslServerBasicImp>(secrets,with_apop,config,challenge_domain))
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

bool GAuth::SaslServerBasic::trusted( const G::StringArray & address_wildcards ,
	const std::string & address_display ) const
{
	return m_imp->trusted( address_wildcards , address_display ) ;
}

void GAuth::SaslServerBasic::reset()
{
	m_imp->reset() ;
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

