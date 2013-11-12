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
// md5.cpp
//
// An implementation of the RFC-1321 message digest algorithm.
//
// This code was developed from main body of RFC 1321 without reference to the
// RSA reference implementation in the appendix.
//
// A minor portability advantage over the RSA implementation is that there is no 
// need to define a datatype that is exactly 32 bits: the requirement is that 
// 'big_t' is at least 32 bits, but it can be more.
// 
// Stylistic advantages are that it is written in C++ with an enclosing namespace,
// it does not use preprocessor macros, and there is an element of layering with 
// digest_stream built on top of the low-level digest class.

#include "md5.h"

md5::digest::digest()
{
	init() ;
}

md5::digest::digest( state_type d_in ) :
	a(d_in.a) ,
	b(d_in.b) ,
	c(d_in.c) ,
	d(d_in.d)
{
}

md5::digest::state_type md5::digest::state() const
{
	big_t mask = 0 ;
	small_t thirty_two = 32U ;
	small_t sizeof_thirty_two_bits = 4U ; // 4x8=32
	if( sizeof(mask) > sizeof_thirty_two_bits ) 
	{
		mask = ~0U ;
		mask <<= thirty_two ; // ignore warnings here
	}
	state_type result = { a & ~mask , b & ~mask , c & ~mask , d & ~mask } ;
	return result ;
}

md5::digest::digest( const md5::string_type & s )
{
	init() ;
	small_t n = block::blocks( s.length() ) ;
	for( small_t i = 0U ; i < n ; ++i )
	{
		block blk( s , i , block::end(s.length()) ) ;
		add( blk ) ;
	}
}

void md5::digest::add( const block & m )
{
	digest old( *this ) ;
	round1( m ) ;
	round2( m ) ;
	round3( m ) ;
	round4( m ) ;
	add( old ) ;
}

void md5::digest::init()
{
	a = 0x67452301UL ;
	b = 0xefcdab89UL ;
	c = 0x98badcfeUL ;
	d = 0x10325476UL ;
}

void md5::digest::add( const digest & other )
{
	a += other.a ;
	b += other.b ;
	c += other.c ;
	d += other.d ;
}

void md5::digest::round1( const block & m )
{
	digest & r = *this ;
	r(m,F,ABCD, 0, 7, 1); r(m,F,DABC, 1,12, 2); r(m,F,CDAB, 2,17, 3); r(m,F,BCDA, 3,22, 4);
	r(m,F,ABCD, 4, 7, 5); r(m,F,DABC, 5,12, 6); r(m,F,CDAB, 6,17, 7); r(m,F,BCDA, 7,22, 8);
	r(m,F,ABCD, 8, 7, 9); r(m,F,DABC, 9,12,10); r(m,F,CDAB,10,17,11); r(m,F,BCDA,11,22,12);
	r(m,F,ABCD,12, 7,13); r(m,F,DABC,13,12,14); r(m,F,CDAB,14,17,15); r(m,F,BCDA,15,22,16);
}

void md5::digest::round2( const block & m )
{
	digest & r = *this ;
	r(m,G,ABCD, 1, 5,17); r(m,G,DABC, 6, 9,18); r(m,G,CDAB,11,14,19); r(m,G,BCDA, 0,20,20);
	r(m,G,ABCD, 5, 5,21); r(m,G,DABC,10, 9,22); r(m,G,CDAB,15,14,23); r(m,G,BCDA, 4,20,24);
	r(m,G,ABCD, 9, 5,25); r(m,G,DABC,14, 9,26); r(m,G,CDAB, 3,14,27); r(m,G,BCDA, 8,20,28);
	r(m,G,ABCD,13, 5,29); r(m,G,DABC, 2, 9,30); r(m,G,CDAB, 7,14,31); r(m,G,BCDA,12,20,32);
}

void md5::digest::round3( const block & m )
{
	digest & r = *this ;
	r(m,H,ABCD, 5, 4,33); r(m,H,DABC, 8,11,34); r(m,H,CDAB,11,16,35); r(m,H,BCDA,14,23,36);
	r(m,H,ABCD, 1, 4,37); r(m,H,DABC, 4,11,38); r(m,H,CDAB, 7,16,39); r(m,H,BCDA,10,23,40);
	r(m,H,ABCD,13, 4,41); r(m,H,DABC, 0,11,42); r(m,H,CDAB, 3,16,43); r(m,H,BCDA, 6,23,44);
	r(m,H,ABCD, 9, 4,45); r(m,H,DABC,12,11,46); r(m,H,CDAB,15,16,47); r(m,H,BCDA, 2,23,48);
}

void md5::digest::round4( const block & m )
{
	digest & r = *this ;
	r(m,I,ABCD, 0, 6,49); r(m,I,DABC, 7,10,50); r(m,I,CDAB,14,15,51); r(m,I,BCDA, 5,21,52);
	r(m,I,ABCD,12, 6,53); r(m,I,DABC, 3,10,54); r(m,I,CDAB,10,15,55); r(m,I,BCDA, 1,21,56);
	r(m,I,ABCD, 8, 6,57); r(m,I,DABC,15,10,58); r(m,I,CDAB, 6,15,59); r(m,I,BCDA,13,21,60);
	r(m,I,ABCD, 4, 6,61); r(m,I,DABC,11,10,62); r(m,I,CDAB, 2,15,63); r(m,I,BCDA, 9,21,64);
}

void md5::digest::operator()( const block & m , aux_fn_t aux , Permutation p , small_t k , small_t s , small_t i )
{
	if( p == ABCD ) a = op( m , aux , a , b , c , d , k , s , i ) ;
	if( p == DABC ) d = op( m , aux , d , a , b , c , k , s , i ) ;
	if( p == CDAB ) c = op( m , aux , c , d , a , b , k , s , i ) ;
	if( p == BCDA ) b = op( m , aux , b , c , d , a , k , s , i ) ;
}

md5::big_t md5::digest::op( const block & m , aux_fn_t aux , big_t a , big_t b , big_t c , big_t d , 
	small_t k , small_t s , small_t i )
{
	return b + rot32( s , ( a + (*aux)( b , c , d ) + m.X(k) + T(i) ) ) ;
}

md5::big_t md5::digest::rot32( small_t places , big_t n )
{
	// circular rotate of 32 LSBs, with corruption of higher bits
	big_t overflow_mask = ( 1UL << places ) - 1UL ; // in case big_t is more than 32 bits
	big_t overflow = ( n >> ( 32U - places ) ) ;
	return ( n << places ) | ( overflow & overflow_mask ) ;
}

md5::big_t md5::digest::F( big_t x , big_t y , big_t z )
{
	return ( x & y ) | ( ~x & z ) ;
}

md5::big_t md5::digest::G( big_t x , big_t y , big_t z )
{
	return ( x & z ) | ( y & ~z ) ;
}

md5::big_t md5::digest::H( big_t x , big_t y , big_t z )
{
	return x ^ y ^ z ;
}

md5::big_t md5::digest::I( big_t x , big_t y , big_t z )
{
	return y ^ ( x | ~z ) ;
}

md5::big_t md5::digest::T( small_t i )
{
	// T = static_cast<big_t>( 4294967296.0 * std::fabs(std::sin(static_cast<double>(i))) ) for 1 <= i <= 64
	//
	static big_t t_map[] = 
	{
		0xd76aa478UL ,
		0xe8c7b756UL ,
		0x242070dbUL ,
		0xc1bdceeeUL ,
		0xf57c0fafUL ,
		0x4787c62aUL ,
		0xa8304613UL ,
		0xfd469501UL ,
		0x698098d8UL ,
		0x8b44f7afUL ,
		0xffff5bb1UL ,
		0x895cd7beUL ,
		0x6b901122UL ,
		0xfd987193UL ,
		0xa679438eUL ,
		0x49b40821UL ,
		0xf61e2562UL ,
		0xc040b340UL ,
		0x265e5a51UL ,
		0xe9b6c7aaUL ,
		0xd62f105dUL ,
		0x02441453UL ,
		0xd8a1e681UL ,
		0xe7d3fbc8UL ,
		0x21e1cde6UL ,
		0xc33707d6UL ,
		0xf4d50d87UL ,
		0x455a14edUL ,
		0xa9e3e905UL ,
		0xfcefa3f8UL ,
		0x676f02d9UL ,
		0x8d2a4c8aUL ,
		0xfffa3942UL ,
		0x8771f681UL ,
		0x6d9d6122UL ,
		0xfde5380cUL ,
		0xa4beea44UL ,
		0x4bdecfa9UL ,
		0xf6bb4b60UL ,
		0xbebfbc70UL ,
		0x289b7ec6UL ,
		0xeaa127faUL ,
		0xd4ef3085UL ,
		0x04881d05UL ,
		0xd9d4d039UL ,
		0xe6db99e5UL ,
		0x1fa27cf8UL ,
		0xc4ac5665UL ,
		0xf4292244UL ,
		0x432aff97UL ,
		0xab9423a7UL ,
		0xfc93a039UL ,
		0x655b59c3UL ,
		0x8f0ccc92UL ,
		0xffeff47dUL ,
		0x85845dd1UL ,
		0x6fa87e4fUL ,
		0xfe2ce6e0UL ,
		0xa3014314UL ,
		0x4e0811a1UL ,
		0xf7537e82UL ,
		0xbd3af235UL ,
		0x2ad7d2bbUL ,
		0xeb86d391UL } ;
	return t_map[i-1UL] ;
}

// ===

md5::string_type md5::format::raw( const digest::state_type & d )
{
	string_type result ;
	result.reserve( 16U ) ;
	result.append( raw(d.a) ) ;
	result.append( raw(d.b) ) ;
	result.append( raw(d.c) ) ;
	result.append( raw(d.d) ) ;
	return result ;
}

md5::string_type md5::format::raw( big_t n )
{
	string_type result ;
	result.reserve( 4U ) ;
	result.append( 1U , static_cast<char>(n&0xffUL) ) ; n >>= 8U ;
	result.append( 1U , static_cast<char>(n&0xffUL) ) ; n >>= 8U ;
	result.append( 1U , static_cast<char>(n&0xffUL) ) ; n >>= 8U ;
	result.append( 1U , static_cast<char>(n&0xffUL) ) ;
	return result ;
}

md5::string_type md5::format::rfc( const digest & d )
{
	return rfc( d.state() ) ;
}

md5::string_type md5::format::rfc( const digest::state_type & d )
{
	string_type result ;
	result.reserve( 32U ) ;
	result.append( str(d.a) ) ;
	result.append( str(d.b) ) ;
	result.append( str(d.c) ) ;
	result.append( str(d.d) ) ;
	return result ;
}

md5::string_type md5::format::str( big_t n )
{
	string_type result ;
	result.reserve( 8U ) ;
	result.append( str2(n&0xffUL) ) ; n >>= 8U ;
	result.append( str2(n&0xffUL) ) ; n >>= 8U ;
	result.append( str2(n&0xffUL) ) ; n >>= 8U ;
	result.append( str2(n&0xffUL) ) ;
	return result ;
}

md5::string_type md5::format::str2( small_t n )
{
	return str1((n>>4U)&0xfU) + str1(n&0xfU) ;
}

md5::string_type md5::format::str1( small_t n )
{
	n = n <= 15U ? n : 0U ;
	const char * map = "0123456789abcdef" ;
	return string_type( 1U , map[n] ) ;
}

// ===

md5::block::block( const md5::string_type & s , small_t block , big_t end_value ) :
	m_s(s) ,
	m_block(block) ,
	m_end_value(end_value)
{
}

md5::big_t md5::block::end( small_t length )
{
	big_t result = length ;
	result *= 8UL ;
	return result ;
}

md5::small_t md5::block::rounded( small_t raw_byte_count )
{
	small_t n = raw_byte_count + 64U ;
	return n - ( ( raw_byte_count + 8U ) % 64U ) ;
}

md5::small_t md5::block::blocks( small_t raw_byte_count )
{
	small_t byte_count = rounded(raw_byte_count) + 8U ;
	return byte_count / 64UL ;
}

md5::big_t md5::block::X( small_t dword_index ) const
{
	small_t byte_index = ( m_block * 64U ) + ( dword_index * 4U ) ;
	big_t result = x( byte_index + 3U ) ;
	result <<= 8U ; result += x( byte_index + 2U ) ;
	result <<= 8U ; result += x( byte_index + 1U ) ;
	result <<= 8U ; result += x( byte_index + 0U ) ;
	return result ;
}

md5::small_t md5::block::x( small_t i ) const
{
	small_t length = m_s.length() ;
	if( i < length )
	{
		return static_cast<unsigned char>(m_s[i]) ;
	}
	else if( i < rounded(length) )
	{
		return i == length ? 128U : 0U ;
	}
	else
	{
		small_t byte_shift = i - rounded(length) ;
		if( byte_shift >= sizeof(big_t) ) 
		{
			return 0U ;
		}
		else
		{
			small_t bit_shift = byte_shift * 8U ;

			big_t end_value = m_end_value >> bit_shift ;
			return static_cast<small_t>( end_value & 0xffUL ) ;
		}
	}
}

// ===

md5::digest_stream::digest_stream() :
	m_length(0U)
{
}

md5::digest_stream::digest_stream( digest::state_type dd , small_t length ) :
	m_digest(dd) ,
	m_length(length)
{
}

void md5::digest_stream::add( const md5::string_type & s )
{
	m_buffer.append( s ) ;
	m_length += s.length() ;

	while( m_buffer.length() >= 64U )
	{
		block m( m_buffer , 0U , 0UL ) ;
		m_digest.add( m ) ;
		m_buffer.erase( 0U , 64U ) ;
	}
}

void md5::digest_stream::close()
{
	block b( m_buffer , 0U , block::end(m_length) ) ;
	m_digest.add( b ) ;
	m_buffer.erase() ;
}

md5::digest_stream::state_type md5::digest_stream::state() const
{
	state_type result ;
	result.d = m_digest.state() ;
	result.n = m_length ;
	result.s = m_buffer ;
	return result ;
}

md5::small_t md5::digest_stream::size() const
{
	return m_length ;
}

/// \file md5.cpp
