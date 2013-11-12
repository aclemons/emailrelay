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
// gsaslclient_native.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gsaslclient.h"
#include "gmd5.h"
#include "gstr.h"
#include "gdebug.h"
#include <algorithm> // set_intersection
#include <set>

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
	const SaslClient::Secrets & m_secrets ;
	explicit SaslClientImp( const SaslClient::Secrets & ) ;
	static std::string clientResponse( const std::string & secret , 
		const std::string & challenge , bool cram , bool & error ) ;
	static std::string digest( const std::string & secret , const std::string & challenge ) ;
	static std::string cramDigest( const std::string & secret , const std::string & challenge ) ;
} ;

// ===

GAuth::SaslClientImp::SaslClientImp( const SaslClient::Secrets & secrets ) :
	m_secrets(secrets)
{
}

std::string GAuth::SaslClientImp::clientResponse( const std::string & secret , 
	const std::string & challenge , bool cram , bool & error )
{
	try 
	{ 
		G_DEBUG( "GAuth::SaslClientImp::clientResponse: challenge=\"" << challenge << "\"" ) ;
		return cram ? cramDigest(secret,challenge) : digest(secret,challenge) ;
	} 
	catch( std::exception & e )
	{ 
		std::string what = e.what() ;
		G_DEBUG( "GAuth::SaslClient: " << what ) ;
		error = true ; 
	}
	return std::string() ;
}

std::string GAuth::SaslClientImp::cramDigest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::hmac(secret,challenge,G::Md5::Masked())) ; 
}

std::string GAuth::SaslClientImp::digest( const std::string & secret , const std::string & challenge )
{
	return G::Md5::printable(G::Md5::digest(challenge,secret)) ;
}

// ===

GAuth::SaslClient::SaslClient( const SaslClient::Secrets & secrets , const std::string & server_name ) :
	m_imp(new SaslClientImp(secrets) )
{
	G_IGNORE_PARAMETER(std::string,server_name) ;
	G_DEBUG( "GAuth::SaslClient::ctor: server-name=\"" << server_name << "\", active=" << active() ) ;
}

GAuth::SaslClient::~SaslClient()
{
	delete m_imp ;
}

bool GAuth::SaslClient::active() const
{
	return m_imp->m_secrets.valid() ;
}

std::string GAuth::SaslClient::response( const std::string & mechanism , const std::string & challenge , 
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
			rsp = id + " " + SaslClientImp::clientResponse( secret , challenge , cram , error ) ;

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
		G_WARNING( "GAuth::SaslClient: invalid challenge" ) ;
		done = true ;
	}

	return rsp ;
}

std::string GAuth::SaslClient::preferred( const G::Strings & mechanism_list ) const
{
	G_DEBUG( "GAuth::SaslClient::preferred: server's mechanisms: [" << G::Str::join(mechanism_list,",") << "]" ) ;

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
	G_DEBUG( "GAuth::SaslClient::preferred: we prefer \"" << m << "\"" ) ;

	return m ;
}

// ==

GAuth::SaslClient::Secrets::~Secrets()
{
}

