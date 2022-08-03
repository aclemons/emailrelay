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

GAuth::Secret::Secret()
{
	G_ASSERT( !valid() ) ;
}

GAuth::Secret::Secret( G::string_view id ) :
	m_id(G::sv_to_string(id))
{
	G_ASSERT( !valid() ) ;
}

GAuth::Secret::Secret( G::string_view secret , G::string_view secret_encoding ,
	G::string_view id , bool id_encoding_xtext , G::string_view context ) :
		m_id(G::sv_to_string(id)) ,
		m_context(G::sv_to_string(context))
{
	G_ASSERT( secret_encoding == G::Str::lower(secret_encoding) ) ;
	std::string reason = check( secret , secret_encoding , id , id_encoding_xtext ) ;
	if( !reason.empty() )
		throw context.empty() ? Error(reason) : Error(m_context,reason) ;

	if( secret_encoding == "plain"_sv )
	{
		G_ASSERT( G::Xtext::valid(secret) ) ;
		m_key = G::Xtext::decode( secret ) ;
	}
	else if( secret_encoding == "md5"_sv && isDotted(secret) )
	{
		m_key = undotted( secret ) ;
		m_mask_type = G::sv_to_string( secret_encoding ) ;
	}
	else
	{
		G_ASSERT( G::Base64::valid(secret) ) ; // check()ed
		m_key = G::Base64::decode( secret ) ;
		m_mask_type = G::sv_to_string( secret_encoding ) ;
	}
}

std::string GAuth::Secret::check( G::string_view secret , G::string_view secret_encoding ,
	G::string_view id , bool id_encoding_xtext )
{
	if( secret.empty() )
		return "empty secret" ;
	if( secret_encoding.empty() || secret_encoding != G::Str::lower(secret_encoding) || !G::Str::isPrintableAscii(secret_encoding) )
		return "invalid encoding type" ;
	if( id.empty() )
		return "empty id" ;
	if( id_encoding_xtext && !G::Xtext::valid(id) )
		return "invalid xtext encoding of id" ;
	if( secret_encoding == "plain"_sv && !G::Xtext::valid(secret) )
		return "invalid xtext encoding of secret" ;
	if( secret_encoding == "md5"_sv && !( isDotted(secret) || G::Base64::valid(secret) ) )
		return "invalid encoding of md5 secret" ;
	if( secret_encoding != "md5"_sv && secret_encoding != "plain"_sv && !G::Base64::valid(secret) )
		return "invalid base64 encoding of secret" ;
	return std::string() ;
}

GAuth::Secret GAuth::Secret::none( G::string_view id )
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
	if( !m_context.empty() )
	{
		ss << " from " << m_context ;
	}
	return ss.str() ;
}

bool GAuth::Secret::isDotted( G::string_view s )
{
	return
		s.size() >= 15U &&
		s.find_first_not_of("0123456789."_sv) == std::string::npos &&
		G::StringFieldView(s,'.').count() == 8U ;
}

std::string GAuth::Secret::undotted( G::string_view s )
{
	std::string result ;
	for( G::StringFieldView decimal(s,".",1U) ; decimal ; ++decimal )
	{
		G::Md5::big_t n = 0U ;
		G::string_view d = decimal() ;
		for( const char & c : d )
		{
			n *= 10U ;
			n += (c-'0') ;
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

