//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstringlist.h"
#include "gssl.h"
#include "gbase64.h"
#include "glocal.h"
#include "gexception.h"
#include "gtest.h"
#include "glog.h"
#include <algorithm>

namespace GAuth
{
	namespace CramImp
	{
		GSsl::Library & lib()
		{
			GSsl::Library * p = GSsl::Library::instance() ;
			if( p == nullptr ) throw Cram::NoTls() ;
			return *p ;
		}
		struct DigesterAdaptor /// Used by GAuth::Cram to use GSsl::Digester.
		{
			explicit DigesterAdaptor( G::string_view name ) :
				m_name(G::sv_to_string(name))
			{
				GSsl::Digester d( CramImp::lib().digester(m_name) ) ;
				m_blocksize = d.blocksize() ;
			}
			std::string operator()( G::string_view data_1 , G::string_view data_2 ) const
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
			explicit PostDigesterAdaptor( G::string_view name ) :
				m_name(G::sv_to_string(name))
			{
				GSsl::Digester d( CramImp::lib().digester(m_name,std::string(),true) ) ;
				if( d.statesize() == 0U )
					throw Cram::NoState( m_name ) ;
				m_valuesize = d.valuesize() ;
				m_blocksize = d.blocksize() ;
			}
			std::string operator()( const std::string & state_pair , const std::string & data ) const
			{
				if( state_pair.size() != (2U*m_valuesize) ) throw Cram::InvalidState( m_name ) ;
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

std::string GAuth::Cram::response( G::string_view hash_type , bool as_hmac ,
	const Secret & secret , G::string_view challenge , G::string_view id_prefix )
{
	try
	{
		G_DEBUG( "GAuth::Cram::response: [" << hash_type << "]"
			<< "[" << as_hmac << "]"
			<< "[" << G::Str::printable(secret.secret()) << "]"
			<< "[" << secret.maskHashFunction() << "][" << challenge << "]"
			<< "[" << G::Str::printable(id_prefix) << "]"
			<< "[" << responseImp(hash_type,as_hmac,secret,challenge) << "]" ) ;

		return G::sv_to_string(id_prefix).append(1U,' ').append(responseImp(hash_type,as_hmac,secret,challenge)) ;
	}
	catch( std::exception & e )
	{
		G_WARNING( "GAuth::Cram::response: challenge-response failure: " << e.what() ) ;
		return std::string() ;
	}
}

bool GAuth::Cram::validate( G::string_view hash_type , bool as_hmac ,
	const Secret & secret , G::string_view challenge ,
	G::string_view response_in )
{
	try
	{
		G_DEBUG( "GAuth::Cram::validate: [" << hash_type << "]"
			<< "[" << as_hmac << "]"
			<< "[" << G::Str::printable(secret.secret()) << "]"
			<< "[" << secret.maskHashFunction() << "]"
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

std::string GAuth::Cram::id( G::string_view response )
{
	// the response is "<id> <hexchars>" but also allow for ids with spaces
	std::size_t pos = response.rfind( ' ' ) ;
	return G::Str::head( response , pos ) ;
}

std::string GAuth::Cram::responseImp( G::string_view mechanism_hash_type , bool as_hmac ,
	const Secret & secret , G::string_view challenge )
{
	G_DEBUG( "GAuth::Cram::responseImp: mechanism-hash=[" << mechanism_hash_type << "] "
		<< "secret-hash=[" << secret.maskHashFunction() << "] "
		<< "as-hmac=" << as_hmac ) ;

	if( !as_hmac )
	{
		if( secret.masked() )
			throw BadType( secret.maskHashFunction() ) ;

		if( G::Str::imatch( mechanism_hash_type , "MD5"_sv ) )
		{
			return G::Hash::printable( G::Md5::digest(challenge,secret.secret()) ) ;
		}
		else
		{
			CramImp::DigesterAdaptor digest( mechanism_hash_type ) ;
			return G::Hash::printable( digest(challenge,secret.secret()) ) ;
		}
	}
	else if( secret.masked() )
	{
		if( ! G::Str::imatch(secret.maskHashFunction(),mechanism_hash_type) )
			throw Mismatch( secret.maskHashFunction() , G::sv_to_string(mechanism_hash_type) ) ;

		if( G::Str::imatch( mechanism_hash_type , "MD5"_sv ) )
		{
			return G::Hash::printable( G::Hash::hmac(G::Md5::postdigest,secret.secret(),G::sv_to_string(challenge),G::Hash::Masked()) ) ;
		}
		else
		{
			CramImp::PostDigesterAdaptor postdigest( mechanism_hash_type ) ;
			return G::Hash::printable( G::Hash::hmac(postdigest,secret.secret(),G::sv_to_string(challenge),G::Hash::Masked()) ) ;
		}
	}
	else
	{
		if( G::Str::imatch( mechanism_hash_type , "MD5"_sv ) )
		{
			return G::Hash::printable( G::Hash::hmac(G::Md5::digest2,G::Md5::blocksize(),secret.secret(),G::sv_to_string(challenge)) ) ;
		}
		else
		{
			CramImp::DigesterAdaptor digest( mechanism_hash_type ) ;
			return G::Hash::printable( G::Hash::hmac(digest,digest.blocksize(),secret.secret(),G::sv_to_string(challenge)) ) ;
		}
	}
}

G::StringArray GAuth::Cram::hashTypes( G::string_view prefix , bool require_state )
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
	if( !G::StringList::match( result , "MD5" ) )
		result.push_back( "MD5" ) ;

	if( !prefix.empty() )
	{
		for( auto & hashtype : result )
			hashtype.insert( 0U , prefix.data() , prefix.size() ) ;
	}
	return result ;
}

std::string GAuth::Cram::challenge( unsigned int random , const std::string & challenge_domain_in )
{
	std::string challenge_domain = challenge_domain_in.empty() ? GNet::Local::canonicalName() : challenge_domain_in ;
	return std::string(1U,'<')
		.append(std::to_string(random)).append(1U,'.')
		.append(std::to_string(G::SystemTime::now().s())).append(1U,'@')
		.append(challenge_domain).append(1U,'>') ;
}

