//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsasl_native.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gsaslclient.h"
#include "gsaslserver.h"
#include "gstr.h"
#include "gmd5.h"
#include "gdatetime.h"
#include "gmemory.h"
#include "gdebug.h"
#include <sstream>
#include <algorithm> // set_intersection
#include <set>

namespace
{
	const char * login_challenge_1 = "Username:" ;
	const char * login_challenge_2 = "Password:" ;
}

/// \class GSmtp::SaslServerImp
/// A private pimple-pattern implementation class used by GSmtp::SaslServer.
/// 
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
	bool m_advertise_login ;
	bool m_advertise_plain ;
	bool m_advertise_cram_md5 ;
	bool m_advertise_force_one ;

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

/// \class GSmtp::SaslClientImp
/// A private pimple-pattern implementation class used by GSmtp::SaslClient.
/// 
class GSmtp::SaslClientImp 
{
public:
	const SaslClient::Secrets & m_secrets ;
	explicit SaslClientImp( const SaslClient::Secrets & ) ;
} ;

// ===

GSmtp::SaslServerImp::SaslServerImp( const SaslServer::Secrets & secrets , bool , bool force_one ) :
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

std::string GSmtp::SaslServerImp::mechanisms( const std::string & sep ) const
{
	G::Strings m ;
	if( m_advertise_login ) m.push_back( "LOGIN" ) ;
	if( m_advertise_plain ) m.push_back( "PLAIN" ) ;
	if( m_advertise_cram_md5 ) m.push_back( "CRAM-MD5" ) ;
	return G::Str::join( m , sep ) ;
}

bool GSmtp::SaslServerImp::init( const std::string & mechanism )
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
		G_DEBUG( "GSmtp::SaslServer: exception: " << what ) ;
	}
	return ok ;
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
		std::string what = e.what() ;
		G_DEBUG( "GSmtp::SaslClient: " << what ) ;
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
	G_DEBUG( "GSmtp::SaslServerImp::trusted: \"" << address.displayString(false) << "\"" ) ;
	G::Strings wc = address.wildcards() ;
	for( G::Strings::iterator p = wc.begin() ; p != wc.end() ; ++p )
	{
		if( trustedCore(address.displayString(false),*p) )
			return true ;
	}
	return false ;
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
	std::string m = m_imp->m_mechanism ;
	if( m == "LOGIN" )
		return login_challenge_1 ;
	else if( m == "PLAIN" )
		return std::string() ;
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
	else if( m_imp->m_mechanism == "PLAIN" )
	{
		G_DEBUG( "GSmtp::SaslServer::apply: response: \"" << G::Str::printable(response) << "\"" ) ;
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
		G_DEBUG( "GSmtp::SaslServer::apply: response: \"" << response << "\"" ) ;
		m_imp->m_first = false ;
		m_imp->m_id = response ;
		if( !m_imp->m_id.empty() )
			next_challenge = login_challenge_2 ;
	}
	else // LOGIN password
	{
		G_ASSERT( m_imp->m_mechanism == "LOGIN" ) ;
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

std::string GSmtp::SaslClient::response( const std::string & mechanism , const std::string & challenge , 
	bool & done , bool & error , bool & sensitive ) const
{
	done = false ;
	error = false ;
	sensitive = false ;

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
	else if( mechanism == "PLAIN" )
	{
		std::string sep( 1U , '\0' ) ;
		std::string id = m_imp->m_secrets.id(mechanism) ;
		std::string secret = m_imp->m_secrets.secret(mechanism) ;
		rsp = sep + id + sep + secret ;
		error = id.empty() || secret.empty() ;
		done = true ;
		sensitive = true ;
	}
	else if( mechanism == "LOGIN" )
	{
		if( challenge == login_challenge_1 )
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
			sensitive = true ;
		}
		else
		{
			error = true ;
		}
	}
	else
	{
		error = true ;
	}

	if( error )
	{
		G_WARNING( "GSmtp::SaslClient: invalid challenge" ) ;
		done = true ;
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

	const std::string login( "LOGIN" ) ;
	const std::string plain( "PLAIN" ) ;
	const std::string cram( "CRAM-MD5" ) ;

	// create a them set
	std::set<std::string> them ;
	for( G::Strings::const_iterator p = mechanism_list.begin() ; p != mechanism_list.end() ; ++p )
		them.insert( G::Str::upper(*p) ) ;

	// create an us set
	std::set<std::string> us ;
	if( !m_imp->m_secrets.id(login).empty() ) us.insert(login) ;
	if( !m_imp->m_secrets.id(plain).empty() ) us.insert(plain) ;
	if( !m_imp->m_secrets.id(cram).empty() ) us.insert(cram) ;

	// get the intersection
	std::set<std::string> both ;
	std::set_intersection( them.begin() , them.end() , us.begin() , us.end() , std::inserter(both,both.end()) ) ;

	// preferred order: cram, plain, login
	std::string m ;
	if( m.empty() && both.find(cram) != both.end() ) m = cram ;
	if( m.empty() && both.find(plain) != both.end() ) m = plain ;
	if( m.empty() && both.find(login) != both.end() ) m = login ;
	G_DEBUG( "GSmtp::SaslClient::preferred: we prefer \"" << m << "\"" ) ;

	return m ;
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

