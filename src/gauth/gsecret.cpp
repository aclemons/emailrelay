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
/// \file gsecret.cpp
///

#include "gdef.h"
#include "gsecret.h"
#include "gstr.h"
#include "gstringfield.h"
#include "gbase64.h"
#include "gxtext.h"
#include "gmd5.h"
#include "gassert.h"
#include <sstream>

GAuth::Secret::Secret( Value id , Value secret , std::string_view hash_function ,
	std::string_view context ) :
		m_hash_function(G::Str::lower(hash_function)) ,
		m_context(G::sv_to_string(context))
{
	std::string reason = check( id , secret , m_hash_function ) ;
	if( !reason.empty() )
		throw context.empty() ? Error(reason) : Error(m_context,reason) ;

	m_id = decode( id ) ;
	m_secret = decode( secret ) ;
}

GAuth::Secret::Secret() // private -- see Secret::none()
{
	G_ASSERT( !valid() ) ;
}

std::string GAuth::Secret::check( Value id , Value secret , std::string_view hash_function )
{
	if( id.first.empty() )
		return "empty id" ;

	if( secret.first.empty() )
		return "empty secret" ;

	if( !validEncodingType(id) )
		return "invalid encoding type for id" ;

	if( !validEncodingType(secret) )
		return "invalid encoding type for secret" ;

	if( !validEncoding(id) )
		return "invalid " + G::sv_to_string(id.second) + " encoding of id" ;

	if( !validEncoding(secret) )
		return "invalid " + G::sv_to_string(id.second) + " encoding of secret" ;

	if( encoding(secret) == Encoding::dotted && !G::Str::imatch(hash_function,"md5"_sv) )
		return "invalid use of dotted format" ;

	return {} ;
}

GAuth::Secret GAuth::Secret::none()
{
	return {} ;
}

bool GAuth::Secret::valid() const
{
	return !m_secret.empty() ;
}

std::string GAuth::Secret::secret() const
{
	if( !valid() ) throw Error() ;
	return m_secret ;
}

bool GAuth::Secret::masked() const
{
	return !m_hash_function.empty() ;
}

std::string GAuth::Secret::id() const
{
	if( !valid() ) throw Error() ;
	return m_id ;
}

std::string GAuth::Secret::maskHashFunction() const
{
	if( !valid() ) throw Error() ;
	return m_hash_function ;
}

std::string GAuth::Secret::info( const std::string & id_in ) const
{
	std::ostringstream ss ;
	ss << (valid()?(masked()?maskHashFunction():std::string("plaintext")):std::string("missing")) << " secret" ;
	std::string id_ = ( id_in.empty() && valid() ) ? m_id : id_in ;
	if( !id_.empty() )
	{
		 ss << " for [" << G::Str::printable(id_) << "]" ;
	}
	if( !m_context.empty() )
	{
		ss << " from " << m_context ;
	}
	return ss.str() ;
}

bool GAuth::Secret::isDotted( std::string_view s )
{
	return
		s.size() >= 15U &&
		s.find_first_not_of("0123456789."_sv) == std::string::npos &&
		G::StringFieldView(s,'.').count() == 8U ;
}

std::string GAuth::Secret::undotted( std::string_view s )
{
	std::string result ;
	for( G::StringFieldView decimal(s,".",1U) ; decimal ; ++decimal )
	{
		G::Md5::big_t n = 0U ;
		std::string_view d = decimal() ;
		for( const char & c : d )
		{
			n *= 10U ;
			n += (G::Md5::big_t(c)-'0') ;
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

bool GAuth::Secret::validEncodingType( Value value )
{
	return
		value.second.empty() ||
		G::Str::imatch( value.second , "xtext"_sv ) ||
		G::Str::imatch( value.second , "base64"_sv ) ||
		G::Str::imatch( value.second , "dotted"_sv ) ;
}

GAuth::Secret::Encoding GAuth::Secret::encoding( Value value )
{
	if( value.second.empty() )
		return Encoding::raw ;
	else if( G::Str::imatch( value.second , "xtext"_sv ) )
		return Encoding::xtext ;
	else if( G::Str::imatch( value.second , "dotted"_sv ) )
		return Encoding::dotted ;
	else
		return Encoding::base64 ;
}

bool GAuth::Secret::validEncoding( Value value )
{
	if( encoding(value) == Encoding::raw )
		return true ;
	else if( encoding(value) == Encoding::xtext )
		return G::Xtext::valid( value.first ) ;
	else if( encoding(value) == Encoding::dotted )
		return isDotted( value.first ) ;
	else
		return G::Base64::valid( value.first ) ;
}

std::string GAuth::Secret::decode( Value value )
{
	if( encoding(value) == Encoding::raw )
		return G::sv_to_string( value.first ) ;
	else if( encoding(value) == Encoding::xtext )
		return G::Xtext::decode( value.first ) ;
	else if( encoding(value) == Encoding::dotted )
		return undotted( value.first ) ;
	else
		return G::Base64::decode( value.first ) ;
}

