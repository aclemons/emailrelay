//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gmd5_aladdin.cpp
//

#include "gdef.h"
#include "gmd5.h"
#include "gstr.h"
#include "gstrings.h"
#include "gassert.h"

#include "md5.h"

namespace
{
	void init( md5_state_t & context )
	{
		::md5_init( &context ) ;
	}
	void update( md5_state_t & context , const std::string & input )
	{
		char * p = const_cast<char*>(input.c_str()) ;
		int size = static_cast<int>( input.size() ) ;
		::md5_append( &context , reinterpret_cast<unsigned char*>(p) , size ) ;
	}
	std::string final( md5_state_t & context )
	{
		const size_t L = 16U ; // output size
		unsigned char buffer[L] ;
		::md5_finish( &context , buffer ) ;
		const char * buffer_p = reinterpret_cast<char *>(buffer) ;
		return std::string( buffer_p , L ) ;
	}
	std::string writeOut( const md5_state_t & context )
	{
		G_ASSERT( sizeof context.abcd[0] <= sizeof(unsigned long) ) ;
		return 
			G::Str::fromULong( context.abcd[0] ) + "." +
			G::Str::fromULong( context.abcd[1] ) + "." +
			G::Str::fromULong( context.abcd[2] ) + "." +
			G::Str::fromULong( context.abcd[3] ) ;
	}
	void readIn( md5_state_t & context , G::Strings & s )
	{
		static md5_state_t zero_context ;
		context = zero_context ;
		context.count[0] = 0x200 ; // magic number -- see cyrus sasl lib/md5.c
		G_ASSERT( context.count[1] == 0 ) ;
		context.abcd[0] = G::Str::toULong(s.front()) ; s.pop_front() ;
		context.abcd[1] = G::Str::toULong(s.front()) ; s.pop_front() ;
		context.abcd[2] = G::Str::toULong(s.front()) ; s.pop_front() ;
		context.abcd[3] = G::Str::toULong(s.front()) ; s.pop_front() ;
		G_ASSERT( context.buf[0] == 0 ) ;
	}
}

std::string G::Md5::xor_( const std::string & s1 , const std::string & s2 )
{
	G_ASSERT( s1.length() == s2.length() ) ;
	std::string::const_iterator p1 = s1.begin() ;
	std::string::const_iterator p2 = s2.begin() ;
	std::string result ;
	for( ; p1 != s1.end() ; ++p1 , ++p2 )
	{
		unsigned char c1 = static_cast<unsigned char>(*p1) ;
		unsigned char c2 = static_cast<unsigned char>(*p2) ;
		unsigned char c = c1 ^ c2 ;
		result.append( 1U , static_cast<char>(c) ) ;
	}
	return result ;
}

std::string G::Md5::key64( std::string k )
{
	const size_t B = 64U ;
	if( k.length() > B )
		k = digest(k) ;
	if( k.length() < B )
		k.append( std::string(B-k.length(),'\0') ) ;
	G_ASSERT( k.length() == B ) ;
	return k ;
}

std::string G::Md5::ipad()
{
	const size_t B = 64U ;
	return std::string( B , '\066' ) ; // 00110110 = 00,110,110
}

std::string G::Md5::opad()
{
	const size_t B = 64U ;
	return std::string( B , '\134' ) ; // 01011100 = 01,011,100
}

std::string G::Md5::mask( const std::string & k )
{
	std::string k64 = key64( k ) ;
	return mask( k64 , ipad() ) + "." + mask( k64 , opad() ) ;
}

std::string G::Md5::mask( const std::string & k64 , const std::string & pad )
{
	md5_state_t context ;
	init( context ) ;
	update( context , xor_(k64,pad) ) ;
	return writeOut( context ) ;
}

std::string G::Md5::hmac( const std::string & masked_key , const std::string & input , Masked )
{
	G::Strings part_list ;
	G::Str::splitIntoTokens( masked_key , part_list , "." ) ;
	if( part_list.size() != 8U )
		throw InvalidMaskedKey( masked_key ) ;

	md5_state_t inner_context ;
	md5_state_t outer_context ;
	readIn( inner_context , part_list ) ;
	readIn( outer_context , part_list ) ;
	update( inner_context , input ) ;
	update( outer_context , final(inner_context) ) ;
	return final( outer_context ) ;
}

std::string G::Md5::hmac( const std::string & k , const std::string & input )
{
	std::string k64 = key64( k ) ;
	return digest( xor_(k64,opad()) , digest(xor_(k64,ipad()),input) ) ;
}

std::string G::Md5::digest( const std::string & input )
{
	return digest( input , NULL ) ;
}

std::string G::Md5::digest( const std::string & input_1 , const std::string & input_2 )
{
	return digest( input_1 , &input_2 ) ;
}

std::string G::Md5::digest( const std::string & input_1 , const std::string * input_2 )
{
	md5_state_t context ;
	init( context ) ;
	update( context , input_1 ) ;
	if( input_2 != NULL )
		update( context , *input_2 ) ;
	return final( context ) ;
}

std::string G::Md5::printable( const std::string & input )
{
	G_ASSERT( input.length() == 16U ) ;

	std::string result ;
	const char * hex = "0123456789abcdef" ;
	const size_t n = input.length() ;
	for( size_t i = 0U ; i < n ; i++ )
	{
		unsigned char c = static_cast<unsigned char>(input.at(i)) ;
		result.append( 1U , hex[(c>>4U)&0x0F] ) ;
		result.append( 1U , hex[(c>>0U)&0x0F] ) ;
	}

	return result ;
}

// Source-file inclusion is not nice, but it means that the "md5.c"
// file can be left in its pristine state but still benefit (?) from
// portability tweaks in "gdef.h" etc.
//
#include "md5.c"

