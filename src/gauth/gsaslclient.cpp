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
	explicit SaslClientImp( const SaslClientSecrets & ) ;
	bool active() const ;
	std::string preferred( const G::StringArray & ) const ;
	std::string response( const std::string & mechanism , const std::string & challenge ,
		bool & done , bool & sensitive ) const ;
	bool next() ;
	std::string preferred() const ;
	std::string id() const ;
	std::string info() const ;

private:
	static void log( const std::string & , const G::StringArray & , const Secret & ) ;
	static void log( const G::StringArray & , const G::StringArray & , const Secret & ) ;
	static void log( const std::string & mechanisms , const std::string & info , bool supported ) ;
	static bool match( const G::StringArray & , const std::string & ) ;

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

	// build our list of possible mechanisms
	G::StringArray our_mechanisms ;
	Secret plain_secret = m_secrets.clientSecret( "plain" ) ;
	if( plain_secret.valid() )
	{
		// if we have a plaintext password then we can use any
		// cram mechanism for which we have a hash function
		our_mechanisms = Cram::hashTypes( "CRAM-" , false ) ;
		our_mechanisms.push_back( PLAIN ) ;
		our_mechanisms.push_back( LOGIN ) ;
		log( our_mechanisms , server_mechanisms , plain_secret ) ;
	}
	else
	{
		// if we only have masked passwords then we can only
		// use matching mechanisms, and those hash functions
		// must support initialisation with intermediate state
		our_mechanisms = Cram::hashTypes( "CRAM-" , true ) ;
		for( G::StringArray::iterator p = our_mechanisms.begin() ; p != our_mechanisms.end() ; )
		{
			std::string encoding_type = (*p).substr(5U) ;
			Secret secret = m_secrets.clientSecret( encoding_type ) ;
			if( !secret.valid() )
			{
				p = our_mechanisms.erase( p ) ;
			}
			else
			{
				log( *p , server_mechanisms , secret ) ;
				++p ;
			}
		}
	}
	G_DEBUG( "GAuth::SaslClientImp::preferred: server mechanisms: [" << G::Str::join(",",server_mechanisms) << "]" ) ;
	G_DEBUG( "GAuth::SaslClientImp::preferred: our mechanisms: [" << G::Str::join(",",our_mechanisms) << "]" ) ;

	// build the list of mechanisms that we can use with the server
	m_mechanisms.clear() ;
	for( G::StringArray::iterator p = our_mechanisms.begin() ; p != our_mechanisms.end() ; ++p )
	{
		if( match(server_mechanisms,*p) )
		{
			m_mechanisms.push_back( *p ) ;
		}
	}

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

std::string GAuth::SaslClientImp::response( const std::string & mechanism ,
	const std::string & challenge , bool & done , bool & sensitive ) const
{
	done = false ;
	sensitive = true ;

	Secret secret = Secret::none() ;
	std::string rsp ;
	if( mechanism.find("CRAM-") == 0U )
	{
		std::string hash_type = mechanism.substr( 5U ) ;
		secret = m_secrets.clientSecret( hash_type ) ;
		if( !secret.valid() )
			secret = m_secrets.clientSecret( "plain" ) ;
		rsp = Cram::response( hash_type , true , secret , challenge , secret.id() ) ;
		done = true ;
	}
	else if( mechanism == "APOP" )
	{
		secret = m_secrets.clientSecret( "MD5" ) ;
		rsp = Cram::response( "MD5" , false , secret , challenge , secret.id() ) ;
		done = true ;
	}
	else if( mechanism == PLAIN )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		const std::string sep( 1U , '\0' ) ;
		rsp = sep + secret.id() + sep + secret.key() ;
		done = true ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_1 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp = secret.id() ;
		done = false ;
		sensitive = false ;
	}
	else if( mechanism == LOGIN && challenge == login_challenge_2 )
	{
		secret = m_secrets.clientSecret( "plain" ) ;
		rsp = secret.key() ;
		done = true ;
	}

	if( done )
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

void GAuth::SaslClientImp::log( const std::string & our_mechanism , const G::StringArray & server_mechanisms ,
	const Secret & secret )
{
	if( G::LogOutput::instance() != nullptr && G::LogOutput::instance()->at(G::Log::s_LogVerbose) )
	{
		log( G::Str::lower(our_mechanism) , secret.info() , match(server_mechanisms,our_mechanism) ) ;
	}
}

void GAuth::SaslClientImp::log( const G::StringArray & our_mechanisms , const G::StringArray & server_mechanisms ,
	const Secret & secret )
{
	G::StringArray good_list , bad_list ;
	for( G::StringArray::const_iterator p = our_mechanisms.begin() ; p != our_mechanisms.end() ; ++p )
	{
		match(server_mechanisms,*p) ? good_list.push_back(G::Str::lower(*p)) : bad_list.push_back(G::Str::lower(*p)) ;
	}
	if( !good_list.empty() )
	{
		log( G::Str::join(",",good_list) , secret.info() , true ) ;
	}
	if( !bad_list.empty() )
	{
		log( G::Str::join(",",bad_list) , secret.info() , false ) ;
	}
}

void GAuth::SaslClientImp::log( const std::string & mechanisms , const std::string & info , bool supported )
{
	G_LOG( "GAuth::SaslClientImp::log: authentication with remote server using "
		<< "[" << mechanisms << "] and " << info
		<< (supported?"":": not supported by server") ) ;
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

std::string GAuth::SaslClient::response( const std::string & mechanism , const std::string & challenge ,
	bool & done , bool & sensitive ) const
{
	return m_imp->response( mechanism , challenge , done , sensitive ) ;
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

