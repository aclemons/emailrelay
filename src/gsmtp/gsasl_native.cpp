//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gsasl_native.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gsasl.h"
#include "gstr.h"
#include "gmd5.h"
#include "gdatetime.h"
#include "gmemory.h"
#include "gdebug.h"
#include <sstream>

namespace
{
	const char * login_challenge_1 = "Username:" ;
	const char * login_challenge_2 = "Password:" ;
}

// Class: GSmtp::SaslServerImp
// Description: A private pimple-pattern implementation class used by GSmtp::SaslServer.
//
class GSmtp::SaslServerImp 
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
	bool m_strict ;
	bool m_advertise_force_one ;
	bool m_advertise_login ;
	bool m_advertise_cram_md5 ;

public:
	SaslServerImp( const SaslServer::Secrets & , bool , bool ) ;
	bool init( const std::string & mechanism ) ;
	bool validate( const std::string & secret , const std::string & response ) const ;
	static std::string digest( const std::string & secret , const std::string & challenge ) ;
	static std::string cramDigest( const std::string & secret , const std::string & challenge ) ;
	bool trusted( GNet::Address ) const ;
	bool trustedCore( const std::string & , const std::string & ) const ;
	static std::string clientResponse( const std::string & secret , 
		const std::string & challenge , bool cram , bool & error ) ;
	std::string mechanisms( const std::string & ) const ;
} ;

// Class: GSmtp::SaslClientImp
// Description: A private pimple-pattern implementation class used by GSmtp::SaslClient.
//
class GSmtp::SaslClientImp 
{
public:
	const SaslClient::Secrets & m_secrets ;
	explicit SaslClientImp( const SaslClient::Secrets & ) ;
} ;

// ===

GSmtp::SaslServerImp::SaslServerImp( const SaslServer::Secrets & secrets , bool strict , bool force_one ) :
	m_first(true) ,
	m_secrets(secrets) ,
	m_authenticated(false) ,
	m_strict(strict) ,
	m_advertise_force_one(force_one)
{
	m_advertise_login = m_secrets.contains("LOGIN") ;
	m_advertise_cram_md5 = m_secrets.contains("CRAM-MD5") ;
	if( m_strict )
	{
		m_advertise_login = false ;
	}
	if( force_one && !m_advertise_login && !m_advertise_cram_md5 )
	{
		if( m_strict )
			m_advertise_cram_md5 = true ;
		else
			m_advertise_login = true ;
	}
}

std::string GSmtp::SaslServerImp::mechanisms( const std::string & sep ) const
{
	std::string s ;
	if( m_advertise_login )
	{
		s.append("LOGIN") ;
	}
	if( m_advertise_cram_md5 ) 
	{
		if( !s.empty() ) s.append( sep ) ;
		s.append("CRAM-MD5") ;
	}
	return s ;
}

bool GSmtp::SaslServerImp::init( const std::string & mechanism )
{
	m_authenticated = false ;
	m_id = std::string() ;
	m_trustee = std::string() ;
	m_first = true ;
	m_challenge = std::string() ;
	m_mechanism = std::string() ;

	if( mechanism == "LOGIN" )
	{
		m_mechanism = mechanism ;
		return true ;
	}
	else if( mechanism == "CRAM-MD5" || mechanism == "APOP" )
	{
		m_mechanism = mechanism ;
		std::ostringstream ss ;
		ss << "<" << ::rand() << "." << G::DateTime::now() << "@" << GNet::Local::fqdn() << ">" ;
		m_challenge = ss.str() ;
		return true ;
	}
	else
	{
		return false ;
	}
}

bool GSmtp::SaslServerImp::validate( const std::string & secret , const std::string & response ) const
{
	try
	{
		G_ASSERT( m_mechanism == "CRAM-MD5" || m_mechanism == "APOP" ) ;
		bool cram = m_mechanism == "CRAM-MD5" ;
		std::string hash = cram ? cramDigest(secret,m_challenge) : digest(secret,m_challenge) ;
		return response == hash ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GSmtp::SaslServer: exception: " << e.what() ) ;
		return false ;
	}
	catch(...)
	{
		G_WARNING( "GSmtp::SaslServer: exception" ) ;
		return false ;
	}
}

std::string GSmtp::SaslServerImp::clientResponse( const std::string & secret , 
	const std::string & challenge , bool cram , bool & error )
{
	try 
	{ 
		G_DEBUG( "GSmtp::SaslServerImp::clientResponse: challenge=\"" << challenge << "\"" ) ;
		return cram ? cramDigest(secret,challenge) : digest(secret,challenge) ;
	} 
	catch( std::exception & e )
	{ 
		G_WARNING( "GSmtp::SaslClient: " << e.what() ) ;
		error = true ; 
	}
	catch(...)
	{
		error = true ; 
	}
	return std::string() ;
}

std::string GSmtp::SaslServerImp::cramDigest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::hmac(secret,challenge,G::Md5::Masked())) ; 
}

std::string GSmtp::SaslServerImp::digest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::digest(challenge,secret)) ;
}

bool GSmtp::SaslServerImp::trusted( GNet::Address address ) const
{
	std::string ip = address.displayString(false) ;
	G_DEBUG( "GSmtp::SaslServerImp::trusted: \"" << ip << "\"" ) ;
	G::StringArray part ;
	G::Str::splitIntoFields( ip , part , "." ) ;
	if( part.size() == 4U )
	{
		return 
			trustedCore(ip,ip) || 
			trustedCore(ip,part[0]+"."+part[1]+"."+part[2]+".*") || 
			trustedCore(ip,part[0]+"."+part[1]+".*.*") ||
			trustedCore(ip,part[0]+".*.*.*") ||
			trustedCore(ip,"*.*.*.*") ;
	}
	else
	{
		return trustedCore( ip , ip ) ;
	}
}

bool GSmtp::SaslServerImp::trustedCore( const std::string & full , const std::string & key ) const
{
	G_DEBUG( "GSmtp::SaslServerImp::trustedCore: \"" << full << "\", \"" << key << "\"" ) ;
	std::string secret = m_secrets.secret("NONE",key) ;
	bool trusted = ! secret.empty() ;
	if( trusted ) 
	{
		G_LOG( "GSmtp::SaslServer::trusted: trusting \"" << full << "\" "
			<< "(matched on NONE/server/" << key << "/" << secret << ")" ) ;
		const_cast<SaslServerImp*>(this)->m_trustee = secret ;
	}
	return trusted ;
}

// ===

std::string GSmtp::SaslServer::mechanisms( char c ) const
{
	return m_imp->mechanisms( std::string(1U,c) ) ;
}

std::string GSmtp::SaslServer::mechanism() const
{
	return m_imp->m_mechanism ;
}

bool GSmtp::SaslServer::trusted( GNet::Address a ) const
{
	G_DEBUG( "GSmtp::SaslServer::trusted: checking \"" << a.displayString(false) << "\"" ) ;
	return m_imp->trusted(a) ;
}

GSmtp::SaslServer::SaslServer( const SaslServer::Secrets & secrets , bool strict , bool force_one ) :
	m_imp(new SaslServerImp(secrets,strict,force_one))
{
}

bool GSmtp::SaslServer::active() const
{
	return m_imp->m_secrets.valid() ;
}

GSmtp::SaslServer::~SaslServer()
{
	delete m_imp ;
}

bool GSmtp::SaslServer::mustChallenge() const
{
	return m_imp->m_mechanism == "CRAM-MD5" || m_imp->m_mechanism == "APOP" ;
}

bool GSmtp::SaslServer::init( const std::string & mechanism )
{
	bool rc = m_imp->init( mechanism ) ;
	G_DEBUG( "GSmtp::SaslServer::init: \"" << mechanism << "\" -> \"" << m_imp->m_mechanism << "\"" ) ;
	return rc ;
}

std::string GSmtp::SaslServer::initialChallenge() const
{
	if( m_imp->m_mechanism == "LOGIN" )
		return login_challenge_1 ;
	else
		return m_imp->m_challenge ;
}

std::string GSmtp::SaslServer::apply( const std::string & response , bool & done )
{
	done = false ;
	std::string next_challenge ;
	if( m_imp->m_mechanism == "CRAM-MD5" || m_imp->m_mechanism == "APOP" )
	{
		G_DEBUG( "GSmtp::SaslServer::apply: response: \"" << response << "\"" ) ;
		G::Strings part_list ;
		G::Str::splitIntoTokens( response , part_list , " " ) ;
		if( part_list.size() == 2U )
		{
			m_imp->m_id = part_list.front() ;
			std::string response_tail = part_list.back() ;

			G_DEBUG( "GSmtp::SaslServer::apply: id \"" << m_imp->m_id << "\"" ) ;
			std::string secret = m_imp->m_secrets.secret(m_imp->m_mechanism,m_imp->m_id) ;
			if( secret.empty() )
			{
				G_WARNING( "GSmtp::SaslServer::apply: no " << m_imp->m_mechanism 
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
			G_WARNING( "GSmtp::SaslServer::apply: invalid authentication response" ) ;
		}
		done = true ;
	}
	else if( m_imp->m_first ) // LOGIN username
	{
		G_DEBUG( "GSmtp::SaslServer::apply: response: \"" << response << "\"" ) ;
		m_imp->m_first = false ;
		m_imp->m_id = response ;
		if( !m_imp->m_id.empty() )
			next_challenge = login_challenge_2 ;
	}
	else // LOGIN password
	{
		G_DEBUG( "GSmtp::SaslServer::apply: response: \"[password not logged]\"" ) ;
		std::string secret = m_imp->m_secrets.secret(m_imp->m_mechanism,m_imp->m_id) ;
		m_imp->m_first = true ;
		m_imp->m_authenticated = !response.empty() && response == secret ;
		done = true ;
	}

	if( ! done )
		G_DEBUG( "GSmtp::SaslServer::apply: challenge \"" << next_challenge << "\"" ) ;

	return next_challenge ;
}

bool GSmtp::SaslServer::authenticated() const
{
	return m_imp->m_authenticated ;
}

std::string GSmtp::SaslServer::id() const
{
	return m_imp->m_authenticated ? m_imp->m_id : m_imp->m_trustee ;
}

// ===

GSmtp::SaslClientImp::SaslClientImp( const SaslClient::Secrets & secrets ) :
	m_secrets(secrets)
{
}

// ===

GSmtp::SaslClient::SaslClient( const SaslClient::Secrets & secrets , const std::string & server_name ) :
	m_imp(new SaslClientImp(secrets) )
{
	G_DEBUG( "GSmtp::SaslClient::ctor: server-name=\"" << server_name << "\", active=" << active() ) ;
	G_IGNORE server_name.length() ; // pacify compiler
}

GSmtp::SaslClient::~SaslClient()
{
	delete m_imp ;
}

bool GSmtp::SaslClient::active() const
{
	return m_imp->m_secrets.valid() ;
}

std::string GSmtp::SaslClient::response( const std::string & mechanism , 
	const std::string & challenge , bool & done , bool & error ) const
{
	done = false ;
	error = false ;

	std::string rsp ;
	if( mechanism == "CRAM-MD5" || mechanism == "APOP" )
	{
		const bool cram = mechanism == "CRAM-MD5" ;
		std::string id = m_imp->m_secrets.id(mechanism) ;
		std::string secret = m_imp->m_secrets.secret(mechanism) ;
		error = id.empty() || secret.empty() ;
		if( !error )
			rsp = id + " " + SaslServerImp::clientResponse( secret , challenge , cram , error ) ;

		done = true ;
	}
	else if( challenge == login_challenge_1 )
	{
		rsp = m_imp->m_secrets.id(mechanism) ;
		error = rsp.empty() ;
		done = false ;
	}
	else if( challenge == login_challenge_2 )
	{
		rsp = m_imp->m_secrets.secret(mechanism) ;
		error = rsp.empty() ;
		done = true ;
	}
	else
	{
		G_WARNING( "GSmtp::SaslClient: invalid challenge" ) ;
		done = true ;
		error = true ;
	}
	return rsp ;
}

std::string GSmtp::SaslClient::preferred( const G::Strings & mechanism_list ) const
{
	G_DEBUG( "GSmtp::SaslClient::preferred: server's mechanisms: [" << G::Str::join(mechanism_list,",") << "]" ) ;

	// short-circuit if no secrets
	//
	if( !active() )
		return std::string() ;

	// look for cram-md5 and login in the list
	//
	const std::string login( "LOGIN" ) ;
	const std::string cram( "CRAM-MD5" ) ;
	bool has_login = false ;
	bool has_cram = false ;
	for( G::Strings::const_iterator p = mechanism_list.begin() ; p != mechanism_list.end() ; ++p )
	{
		std::string mechanism = *p ;
		G::Str::toUpper( mechanism ) ;
		if( mechanism == login )
			has_login = true ;
		else if( mechanism == cram )
			has_cram = true ;
	}

	// prefer cram-md5 over login...
	//
	std::string result = has_cram ? cram : ( has_login ? login : std::string() ) ;
	G_DEBUG( "GSmtp::SaslClient::preferred: we prefer \"" << result << "\"" ) ;

	// ... but only if a secret is defined for it
	//
	if( !result.empty() && m_imp->m_secrets.id(result).empty() )
	{
		G_DEBUG( "GSmtp::SaslClient::preferred: .. but no secret" ) ;
		result = std::string() ;
		if( has_cram && has_login )
		{
			result = login ;
			if( m_imp->m_secrets.id(login).empty() )
				result = std::string() ;
		}
		G_DEBUG( "GSmtp::SaslClient::preferred: we now prefer \"" << result << "\"" ) ;

		// one-shot warning
		static bool first = true ;
		if( first ) G_WARNING( "GSmtp::SaslClient: missing \"login\" or \"cram-md5\" entry in secrets file" ) ;
		first = false ;
	}
	return result ;
}

// ==

GSmtp::Valid::~Valid() 
{
}

// ==

GSmtp::SaslServer::Secrets::~Secrets()
{
}

// ==

GSmtp::SaslClient::Secrets::~Secrets()
{
}

