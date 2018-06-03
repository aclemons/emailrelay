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
// gsecret.cpp
//

#include "gdef.h"
#include "gsecret.h"
#include "gstr.h"
#include "gbase64.h"
#include "gxtext.h"
#include "gmd5.h"
#include "gassert.h"
#include <sstream>

GAuth::Secret::Secret()
{
	G_ASSERT( !valid() ) ;
}

GAuth::Secret::Secret( const std::string & id ) :
	m_id(id)
{
	G_ASSERT( !valid() ) ;
}

GAuth::Secret::Secret( const std::string & secret , const std::string & type ,
	const std::string & id , const std::string & context ) :
		m_id(id) ,
		m_context(context)
{
	G_ASSERT( type == G::Str::lower(type) ) ;
	std::string reason = check( secret , type , id ) ;
	if( !reason.empty() )
		throw Error( reason ) ;

	if( type == "plain" )
	{
		m_key = G::Xtext::decode( secret ) ;
	}
	else if( type == "md5" && isDotted(secret) )
	{
		m_key = undotted( secret ) ;
		m_mask_type = type ;
	}
	else
	{
		G_ASSERT( G::Base64::valid(secret) ) ;
		m_key = G::Base64::decode( secret ) ;
		m_mask_type = type ;
	}
}

std::string GAuth::Secret::check( const std::string & secret , const std::string & type , const std::string & id )
{
	if( secret.empty() )
		return "empty shared key" ;
	if( type.empty() || type != G::Str::lower(type) || !G::Str::isPrintableAscii(type) )
		return "invalid encoding type" ;
	if( id.empty() || !G::Xtext::valid(id) )
		return "invalid id" ;
	if( type == "plain" && !G::Xtext::valid(secret) )
		return "invalid plain secret" ;
	if( type == "md5" && !( isDotted(secret) || G::Base64::valid(secret) ) )
		return "invalid encoding of md5 secret" ;
	if( type != "md5" && type != "plain" && !G::Base64::valid(secret) )
		return "invalid base64 encoding of secret" ;
	return std::string() ;
}

GAuth::Secret GAuth::Secret::none( const std::string & id )
{
	return Secret( id ) ;
}

GAuth::Secret GAuth::Secret::none()
{
	return Secret() ;
}

bool GAuth::Secret::valid() const
{
	return !m_key.empty() ;
}

std::string GAuth::Secret::key() const
{
	if( !valid() ) throw Error() ;
	return m_key ;
}

bool GAuth::Secret::masked() const
{
	return !m_mask_type.empty() ;
}

std::string GAuth::Secret::id() const
{
	if( !valid() ) throw Error() ;
	return m_id ;
}

std::string GAuth::Secret::maskType() const
{
	if( !valid() ) throw Error() ;
	return m_mask_type ;
}

std::string GAuth::Secret::info( const std::string & id_in ) const
{
	std::ostringstream ss ;
	ss << (valid()?(masked()?maskType():std::string("plaintext")):std::string("missing")) << " secret" ;
	std::string id_ = ( id_in.empty() && valid() ) ? m_id : id_in ;
	if( !id_.empty() )
	{
		 ss << " for [" << G::Str::printable(id_) << "]" ;
	}
	ss << m_context ; // eg. secrets-file line number
	return ss.str() ;
}

bool GAuth::Secret::isDotted( const std::string & s )
{
	return
		s.length() >= 15U &&
		s.find_first_not_of("0123456789.") == std::string::npos &&
		G::Str::splitIntoFields(s,".").size() == 8U ;
}

std::string GAuth::Secret::undotted( const std::string & s )
{
	G::StringArray decimals = G::Str::splitIntoFields( s , "." ) ; decimals.resize( 8U ) ;
	std::string result ;
	for( size_t i = 0U ; i < 8U ; i++ )
	{
		G::Md5::big_t n = 0U ;
		for( std::string::iterator p = decimals[i].begin() ; p != decimals[i].end() ; ++p )
		{
			n *= 10U ;
			n += ((*p)-'0') ;
		}
		for( int j = 0 ; j < 4 ; j++ )
		{
			unsigned char uc = ( n & 0xffU ) ;
			n >>= 8 ;
			result.push_back( static_cast<char>(uc) ) ;
		}
	}
	return result ;
}

/// \file gsecret.cpp
