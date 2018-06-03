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
// gcram.cpp
//

#include "gdef.h"
#include "gcram.h"
#include "ghash.h"
#include "gmd5.h"
#include "gstr.h"
#include "gssl.h"
#include "gbase64.h"
#include "glocal.h"
#include "gtest.h"
#include "gdebug.h"
#include <cstdlib> // std::rand()
#include <algorithm>

namespace
{
	GSsl::Library & lib()
	{
		GSsl::Library * p = GSsl::Library::instance() ;
		if( p == nullptr ) throw std::runtime_error( "no tsl library" ) ;
		return *p ;
	}
	struct DigesterAdaptor
	{
		explicit DigesterAdaptor( const std::string & name ) :
			m_name(name) ,
			m_digester(lib().digester(name)) ,
			m_used(false)
		{
		}
		std::string operator()( const std::string & data_1 , const std::string & data_2 )
		{
			// adaptor must be stateless
			if( m_used )
				m_digester = GSsl::Digester( lib().digester(m_name) ) ;
			m_used = true ;

			m_digester.add( data_1 ) ;
			m_digester.add( data_2 ) ;
			return m_digester.value() ;
		}
		size_t blocksize() const
		{
			return m_digester.blocksize() ;
		}
		std::string m_name ;
		GSsl::Digester m_digester ;
		bool m_used ;
	} ;
	struct PostDigesterAdaptor
	{
		explicit PostDigesterAdaptor( const std::string & name ) :
			m_name(name)
		{
			GSsl::Digester x = lib().digester( m_name ) ;
			if( x.statesize() == 0U )
				throw GAuth::Cram::NoState( m_name ) ;
		}
		std::string operator()( const std::string & state_pair , const std::string & data )
		{
			GSsl::Digester xi = lib().digester( m_name , state_pair.substr(0U,state_pair.size()/2U) ) ;
			xi.add( data ) ;
			GSsl::Digester xo = lib().digester( m_name , state_pair.substr(state_pair.size()/2U) ) ;
			xo.add( xi.value() ) ;
			return xo.value() ;
		}
		std::string m_name ;
	} ;
}

std::string GAuth::Cram::response( const std::string & hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge ,
	const std::string & id_prefix )
{
	try
	{
		G_DEBUG( "GAuth::Cram::response: [" << hash_type << "][" << as_hmac << "]"
			"[" << G::Str::printable(secret.key()) << "][" << secret.maskType() << "][" << challenge << "]"
			"[" << G::Str::printable(id_prefix) << "]"
			"[" << responseImp(hash_type,as_hmac,secret,challenge) << "]" ) ;

		return id_prefix + " " + responseImp(hash_type,as_hmac,secret,challenge) ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "GAuth::Cram::response: exception: " << e.what() ) ; G_IGNORE_VARIABLE( e ) ;
		return std::string() ;
	}
}

bool GAuth::Cram::validate( const std::string & hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge ,
	const std::string & response_in )
{
	try
	{
		G_DEBUG( "GAuth::Cram::validate: [" << hash_type << "][" << as_hmac << "]"
				"[" << G::Str::printable(secret.key()) << "][" << secret.maskType() << "][" << challenge << "][" << response_in << "]"
				"[" << responseImp(hash_type,as_hmac,secret,challenge) << "]" ) ;

		std::string expectation = G::Str::tail( response_in , " " ) ;
		return !expectation.empty() && responseImp(hash_type,as_hmac,secret,challenge) == expectation ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "GAuth::Cram::validate: exception: " << e.what() ) ; G_IGNORE_VARIABLE( e ) ;
		return false ;
	}
}

std::string GAuth::Cram::id( const std::string & response )
{
	return G::Str::head( response , " " ) ;
}

std::string GAuth::Cram::responseImp( const std::string & mechanism_hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge )
{
	G_DEBUG( "GAuth::Cram::responseImp: mechanism-hash=[" << mechanism_hash_type << "] secret-hash=[" << secret.maskType() << "] as-hmac=" << as_hmac ) ;
	if( !as_hmac )
	{
		if( secret.masked() )
			throw BadType( secret.maskType() ) ;

		if( mechanism_hash_type == "MD5" )
		{
			return G::Hash::printable( G::Md5::digest(challenge,secret.key()) ) ;
		}
		else
		{
			DigesterAdaptor digest( mechanism_hash_type ) ;
			return G::Hash::printable( digest(challenge,secret.key()) ) ;
		}
	}
	else if( secret.masked() )
	{
		if( ! G::Str::imatch(secret.maskType(),mechanism_hash_type) )
			throw Mismatch( secret.maskType() , mechanism_hash_type ) ;

		if( mechanism_hash_type == "MD5" )
		{
			return G::Hash::printable( G::Hash::hmac(G::Md5::postdigest,secret.key(),challenge,G::Hash::Masked()) ) ;
		}
		else
		{
			PostDigesterAdaptor postdigest( mechanism_hash_type ) ;
			return G::Hash::printable( G::Hash::hmac(postdigest,secret.key(),challenge,G::Hash::Masked()) ) ;
		}
	}
	else
	{
		if( mechanism_hash_type == "MD5" )
		{
			return G::Hash::printable( G::Hash::hmac(G::Md5::digest2,G::Md5::blocksize(),secret.key(),challenge) ) ;
		}
		else
		{
			DigesterAdaptor digest( mechanism_hash_type ) ;
			return G::Hash::printable( G::Hash::hmac(digest,digest.blocksize(),secret.key(),challenge) ) ;
		}
	}
}

G::StringArray GAuth::Cram::hashTypes( const std::string & prefix , bool require_state )
{
	// we can do CRAM-X for all hash functions (X) provided by the TLS library
	// but if we only have masked passwords (ie. require_state) then we only
	// want hash functions that are capable of initialision with intermediate state
	//
	G::StringArray result = GSsl::Library::digesters( require_state ) ; // strongest first
	if( G::Test::enabled("cram-fake-hash") )
		result.push_back( "FAKE" ) ;

	G_DEBUG( "GAuth::Cram::hashTypes: tls library [" << GSsl::Library::ids() << "]" ) ;
	G_DEBUG( "GAuth::Cram::hashTypes: tls library hash types: [" << G::Str::join(",",result) << "] (" << (require_state?1:0) << ")" ) ;

	// always include MD5 since we use G::Md5 code
	if( std::find(result.begin(),result.end(),"MD5") == result.end() )
		result.push_back( "MD5" ) ;

	if( !prefix.empty() )
	{
		for( G::StringArray::iterator p = result.begin() ; p != result.end() ; ++p )
			*p = prefix + *p ;
	}
	return result ;
}

std::string GAuth::Cram::challenge()
{
	std::ostringstream ss ;
	ss << "<" << std::rand() << "." << G::DateTime::now() << "@" << GNet::Local::canonicalName() << ">" ;
	return ss.str() ;
}

/// \file gcram.cpp
