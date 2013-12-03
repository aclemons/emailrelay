//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsaslserverbasic_native.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "glocal.h"
#include "gsaslserverbasic.h"
#include "gmd5.h"
#include "gstr.h"
#include "gdatetime.h"
#include "gdebug.h"
#include <sstream>
#include <cstdlib> // std::rand()

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
	bool m_first ;
	const SaslServer::Secrets & m_secrets ;
	std::string m_mechanisms ;
	std::string m_mechanism ;
	std::string m_challenge ;
	bool m_authenticated ;
	std::string m_id ;
	std::string m_trustee ;
	bool m_advertise_login ;
	bool m_advertise_plain ;
	bool m_advertise_cram_md5 ;
	bool m_advertise_force_one ;

public:
	SaslServerBasicImp( const SaslServer::Secrets & , bool , bool ) ;
	bool init( const std::string & mechanism ) ;
	bool validate( const std::string & secret , const std::string & response ) const ;
	bool trusted( GNet::Address ) const ;
	bool trustedCore( const std::string & , const std::string & ) const ;
	std::string mechanisms( const std::string & ) const ;
	static std::string cramDigest( const std::string & secret , const std::string & challenge ) ;
	static std::string digest( const std::string & secret , const std::string & challenge ) ;
} ;

// ===

GAuth::SaslServerBasicImp::SaslServerBasicImp( const SaslServer::Secrets & secrets , bool , bool force_one ) :
	m_first(true) ,
	m_secrets(secrets) ,
	m_authenticated(false) ,
	m_advertise_login(false) ,
	m_advertise_plain(false) ,
	m_advertise_cram_md5(false) ,
	m_advertise_force_one(force_one) 
{
	// only advertise mechanisms that appear in the secrets file
	m_advertise_login = m_secrets.contains("LOGIN") ;
	m_advertise_plain = m_secrets.contains("LOGIN") || m_secrets.contains("PLAIN") ;
	m_advertise_cram_md5 = m_secrets.contains("CRAM-MD5") ;
	if( force_one && !m_advertise_login && !m_advertise_cram_md5 && !m_advertise_plain )
	{
		m_advertise_login = true ;
	}
}

std::string GAuth::SaslServerBasicImp::mechanisms( const std::string & sep ) const
{
	G::Strings m ;
	if( m_advertise_login ) m.push_back( "LOGIN" ) ;
	if( m_advertise_plain ) m.push_back( "PLAIN" ) ;
	if( m_advertise_cram_md5 ) m.push_back( "CRAM-MD5" ) ;
	return G::Str::join( m , sep ) ;
}

bool GAuth::SaslServerBasicImp::init( const std::string & mechanism )
{
	m_authenticated = false ;
	m_id.erase() ;
	m_trustee.erase() ;
	m_first = true ;
	m_challenge.erase() ;
	m_mechanism.erase() ;

	if( mechanism == "LOGIN" || mechanism == "PLAIN" )
	{
		m_mechanism = mechanism ;
		return true ;
	}
	else if( mechanism == "CRAM-MD5" || mechanism == "APOP" )
	{
		m_mechanism = mechanism ;
		std::ostringstream ss ;
		ss << "<" << std::rand() << "." << G::DateTime::now() << "@" << GNet::Local::fqdn() << ">" ;
		m_challenge = ss.str() ;
		return true ;
	}
	else
	{
		return false ;
	}
}

bool GAuth::SaslServerBasicImp::validate( const std::string & secret , const std::string & response ) const
{
	bool ok = false ;
	try
	{
		G_ASSERT( m_mechanism == "CRAM-MD5" || m_mechanism == "APOP" ) ;
		bool cram = m_mechanism == "CRAM-MD5" ;
		std::string hash = cram ? cramDigest(secret,m_challenge) : digest(secret,m_challenge) ;
		ok = response == hash ;
	}
	catch( std::exception & e )
	{
		std::string what = e.what() ;
		G_DEBUG( "GAuth::SaslServer: exception: " << what ) ;
	}
	return ok ;
}

std::string GAuth::SaslServerBasicImp::cramDigest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::hmac(secret,challenge,G::Md5::Masked())) ; 
}

std::string GAuth::SaslServerBasicImp::digest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::digest(challenge,secret)) ;
}

bool GAuth::SaslServerBasicImp::trusted( GNet::Address address ) const
{
	G_DEBUG( "GAuth::SaslServerBasicImp::trusted: \"" << address.displayString(false) << "\"" ) ;
	G::Strings wc = address.wildcards() ;
	for( G::Strings::iterator p = wc.begin() ; p != wc.end() ; ++p )
	{
		if( trustedCore(*p,address.displayString(false)) )
			return true ;
	}
	return false ;
}

bool GAuth::SaslServerBasicImp::trustedCore( const std::string & secrets_key , 
	const std::string & address_to_log ) const
{
	G_DEBUG( "GAuth::SaslServerBasicImp::trustedCore: \"" << secrets_key << "\", \"" << address_to_log << "\"" ) ;
	std::string secret = m_secrets.secret("NONE",secrets_key) ;
	bool trusted = ! secret.empty() ;
	if( trusted ) 
	{
		G_LOG( "GAuth::SaslServer::trusted: trusting \"" << address_to_log << "\" "
			<< "(matched on NONE/server/" << secrets_key << ")" ) ;
		const_cast<SaslServerBasicImp*>(this)->m_trustee = secret ;
	}
	return trusted ;
}

// ===

std::string GAuth::SaslServerBasic::mechanisms( char c ) const
{
	return m_imp->mechanisms( std::string(1U,c) ) ;
}

std::string GAuth::SaslServerBasic::mechanism() const
{
	return m_imp->m_mechanism ;
}

bool GAuth::SaslServerBasic::trusted( GNet::Address a ) const
{
	G_DEBUG( "GAuth::SaslServer::trusted: checking \"" << a.displayString(false) << "\"" ) ;
	return m_imp->trusted(a) ;
}

GAuth::SaslServerBasic::SaslServerBasic( const Secrets & secrets , bool strict , bool force_one ) :
	m_imp(new SaslServerBasicImp(secrets,strict,force_one))
{
}

bool GAuth::SaslServerBasic::active() const
{
	return m_imp->m_secrets.valid() ;
}

GAuth::SaslServerBasic::~SaslServerBasic()
{
	delete m_imp ;
}

bool GAuth::SaslServerBasic::mustChallenge() const
{
	return m_imp->m_mechanism == "CRAM-MD5" || m_imp->m_mechanism == "APOP" ;
}

bool GAuth::SaslServerBasic::init( const std::string & mechanism )
{
	bool rc = m_imp->init( mechanism ) ;
	G_DEBUG( "GAuth::SaslServerBasic::init: \"" << mechanism << "\" -> \"" << m_imp->m_mechanism << "\"" ) ;
	return rc ;
}

std::string GAuth::SaslServerBasic::initialChallenge() const
{
	std::string m = m_imp->m_mechanism ;
	if( m == "LOGIN" )
		return login_challenge_1 ;
	else if( m == "PLAIN" )
		return std::string() ;
	else
		return m_imp->m_challenge ;
}

std::string GAuth::SaslServerBasic::apply( const std::string & response , bool & done )
{
	done = false ;
	std::string next_challenge ;
	if( m_imp->m_mechanism == "CRAM-MD5" || m_imp->m_mechanism == "APOP" )
	{
		G_DEBUG( "GAuth::SaslServerBasic::apply: response: \"" << response << "\"" ) ;
		G::Strings part_list ;
		G::Str::splitIntoTokens( response , part_list , " " ) ;
		if( part_list.size() == 2U )
		{
			m_imp->m_id = part_list.front() ;
			std::string response_tail = part_list.back() ;

			G_DEBUG( "GAuth::SaslServerBasic::apply: id \"" << m_imp->m_id << "\"" ) ;
			std::string secret = m_imp->m_secrets.secret(m_imp->m_mechanism,m_imp->m_id) ;
			if( secret.empty() )
			{
				G_WARNING( "GAuth::SaslServerBasic::apply: no " << m_imp->m_mechanism 
					<< " authentication secret available for \"" << m_imp->m_id << "\"" ) ;
				m_imp->m_authenticated = false ;
			}
			else
			{
				m_imp->m_authenticated = m_imp->validate( secret , response_tail ) ;
			}
		}
		else
		{
			G_WARNING( "GAuth::SaslServerBasic::apply: invalid authentication response" ) ;
		}
		done = true ;
	}
	else if( m_imp->m_mechanism == "PLAIN" )
	{
		G_DEBUG( "GAuth::SaslServerBasic::apply: response: \"" << G::Str::printable(response) << "\"" ) ;
		std::string sep( 1U , '\0' ) ;
		std::string s = G::Str::tail( response , response.find(sep) , std::string() ) ;
		std::string id = G::Str::head( s , s.find(sep) , std::string() ) ;
		std::string pwd = G::Str::tail( s , s.find(sep) , std::string() ) ;
		std::string secret = m_imp->m_secrets.secret("PLAIN",id).empty() ?
			m_imp->m_secrets.secret("LOGIN",id) : m_imp->m_secrets.secret("PLAIN",id) ;

		m_imp->m_id = id ;
		m_imp->m_authenticated = !id.empty() && !pwd.empty() && pwd == secret ;
		done = true ;
	}
	else if( m_imp->m_first ) // LOGIN username
	{
		G_ASSERT( m_imp->m_mechanism == "LOGIN" ) ;
		G_DEBUG( "GAuth::SaslServerBasic::apply: response: \"" << G::Str::printable(response) << "\"" ) ;
		m_imp->m_first = false ;
		m_imp->m_id = response ;
		if( !m_imp->m_id.empty() )
			next_challenge = login_challenge_2 ;
	}
	else // LOGIN password
	{
		G_ASSERT( m_imp->m_mechanism == "LOGIN" ) ;
		G_DEBUG( "GAuth::SaslServerBasic::apply: response: \"[password not logged]\"" ) ;
		std::string secret = m_imp->m_secrets.secret(m_imp->m_mechanism,m_imp->m_id) ;
		m_imp->m_first = true ;
		m_imp->m_authenticated = !response.empty() && response == secret ;
		done = true ;
	}

	if( ! done )
	{
		G_DEBUG( "GAuth::SaslServerBasic::apply: challenge \"" << next_challenge << "\"" ) ;
	}

	return next_challenge ;
}

bool GAuth::SaslServerBasic::authenticated() const
{
	return m_imp->m_authenticated ;
}

std::string GAuth::SaslServerBasic::id() const
{
	return m_imp->m_authenticated ? m_imp->m_id : m_imp->m_trustee ;
}

bool GAuth::SaslServerBasic::requiresEncryption() const
{
	return false ;
}

