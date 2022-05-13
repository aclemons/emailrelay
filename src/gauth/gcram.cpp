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
/// \file gcram.cpp
///

#include "gdef.h"
#include "gcram.h"
#include "ghash.h"
#include "ghashstate.h"
#include "gmd5.h"
#include "gstr.h"
#include "gssl.h"
#include "gbase64.h"
#include "glocal.h"
#include "gexception.h"
#include "gtest.h"
#include "glog.h"
#include <algorithm>

namespace GAuth
{
	namespace CramImp /// An implementation namespace for GAuth::Cram.
	{
		G_EXCEPTION( NoTls , tx("no tls library") ) ;
		GSsl::Library & lib()
		{
			GSsl::Library * p = GSsl::Library::instance() ;
			if( p == nullptr ) throw NoTls() ;
			return *p ;
		}
		struct DigesterAdaptor /// Used by GAuth::Cram to use GSsl::Digester.
		{
			explicit DigesterAdaptor( const std::string & name ) :
				m_name(name)
			{
				GSsl::Digester d( CramImp::lib().digester(m_name) ) ;
				m_blocksize = d.blocksize() ;
			}
			std::string operator()( const std::string & data_1 , const std::string & data_2 ) const
			{
				GSsl::Digester d( CramImp::lib().digester(m_name) ) ;
				d.add( data_1 ) ;
				d.add( data_2 ) ;
				return d.value() ;
			}
			std::size_t blocksize() const
			{
				return m_blocksize ;
			}
			std::string m_name ;
			std::size_t m_blocksize ;
		} ;
		struct PostDigesterAdaptor /// Used by GAuth::Cram to use GSsl::Digester.
		{
			explicit PostDigesterAdaptor( const std::string & name ) :
				m_name(name)
			{
				GSsl::Digester d( CramImp::lib().digester(m_name,std::string(),true) ) ;
				if( d.statesize() == 0U )
					throw GAuth::Cram::NoState( m_name ) ;
				m_valuesize = d.valuesize() ;
				m_blocksize = d.blocksize() ;
			}
			std::string operator()( const std::string & state_pair , const std::string & data ) const
			{
				if( state_pair.size() != (2U*m_valuesize) ) throw GAuth::Cram::InvalidState( m_name ) ;
				std::string state_i = state_pair.substr( 0U , state_pair.size()/2U ) + G::HashStateImp::extension(m_blocksize) ;
				std::string state_o = state_pair.substr( state_pair.size()/2U ) + G::HashStateImp::extension(m_blocksize) ;
				GSsl::Digester xi( CramImp::lib().digester( m_name , state_i ) ) ;
				xi.add( data ) ;
				GSsl::Digester xo( CramImp::lib().digester( m_name , state_o ) ) ;
				xo.add( xi.value() ) ;
				return xo.value() ;
			}
			std::string m_name ;
			std::size_t m_valuesize ;
			std::size_t m_blocksize ;
		} ;
	}
}

std::string GAuth::Cram::response( const std::string & hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge ,
	const std::string & id_prefix )
{
	try
	{
		G_DEBUG( "GAuth::Cram::response: [" << hash_type << "]"
			<< "[" << as_hmac << "]"
			<< "[" << G::Str::printable(secret.key()) << "]"
			<< "[" << secret.maskType() << "][" << challenge << "]"
			<< "[" << G::Str::printable(id_prefix) << "]"
			<< "[" << responseImp(hash_type,as_hmac,secret,challenge) << "]" ) ;

		return id_prefix + " " + responseImp(hash_type,as_hmac,secret,challenge) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GAuth::Cram::response: challenge-response failure: " << e.what() ) ;
		return std::string() ;
	}
}

bool GAuth::Cram::validate( const std::string & hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge ,
	const std::string & response_in )
{
	try
	{
		G_DEBUG( "GAuth::Cram::validate: [" << hash_type << "]"
			<< "[" << as_hmac << "]"
			<< "[" << G::Str::printable(secret.key()) << "]"
			<< "[" << secret.maskType() << "]"
			<< "[" << challenge << "]"
			<< "[" << response_in << "]"
			<< "[" << responseImp(hash_type,as_hmac,secret,challenge) << "]" ) ;

		std::string expectation = G::Str::tail( response_in , response_in.rfind(' ') ) ;
		return !expectation.empty() && responseImp(hash_type,as_hmac,secret,challenge) == expectation ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GAuth::Cram::validate: challenge-response failure: " << e.what() ) ;
		return false ;
	}
}

std::string GAuth::Cram::id( const std::string & response )
{
	// the response is "<id> <hexchars>" but also allow for ids with spaces
	return G::Str::head( response , response.rfind(' ') ) ;
}

std::string GAuth::Cram::responseImp( const std::string & mechanism_hash_type , bool as_hmac ,
	const Secret & secret , const std::string & challenge )
{
	G_DEBUG( "GAuth::Cram::responseImp: mechanism-hash=[" << mechanism_hash_type << "] "
		<< "secret-hash=[" << secret.maskType() << "] "
		<< "as-hmac=" << as_hmac ) ;

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
			CramImp::DigesterAdaptor digest( mechanism_hash_type ) ;
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
			CramImp::PostDigesterAdaptor postdigest( mechanism_hash_type ) ;
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
			CramImp::DigesterAdaptor digest( mechanism_hash_type ) ;
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
	G_DEBUG( "GAuth::Cram::hashTypes: tls library hash types: [" << G::Str::join(",",result) << "] "
		<< "(" << (require_state?1:0) << ")" ) ;

	// always include MD5 since we use G::Md5 code
	if( !G::Str::match( result , "MD5" ) )
		result.push_back( "MD5" ) ;

	if( !prefix.empty() )
	{
		for( auto & hashtype : result )
			hashtype.insert( 0U , prefix ) ;
	}
	return result ;
}

std::string GAuth::Cram::challenge( unsigned int random )
{
	std::ostringstream ss ;
	ss << "<" << random << "."
		<< G::SystemTime::now().s() << "@"
		<< GNet::Local::canonicalName() << ">" ;
	return ss.str() ;
}

