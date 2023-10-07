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
/// \file gmd5.cpp
///

#include "gdef.h"
#include "gmd5.h"
#include "ghashstate.h"
#include "gexception.h"
#include "gassert.h"
#include <string> // std::string
#include <cstdlib> // std::size_t
#include <array>

namespace G
{
	namespace Md5Imp
	{
		using digest_state = Md5::digest_state ;
		using small_t = Md5::small_t ;
		using big_t = Md5::big_t ;
		class digest ;
		class format ;
		class block ;
	}
}

//| \class G::Md5Imp::digest
/// A class that calculates an md5 digest from one or more 64-byte blocks of
/// data using the algorithm described by RFC-1321.
///
/// A digest can be calculated in one go from an arbitrarily-sized block of
/// data, or incrementally from a series of 64-byte blocks. The 64-byte
/// blocks must be passed as Md5Imp::block objects.
///
class G::Md5Imp::digest : private G::Md5::digest_state
{
public:
	digest() ;
		// Default constructor. The message to be digested
		// should be add()ed in 64-byte blocks.

	explicit digest( const std::string & s ) ;
		// Constuctor. Calculates a digest for the given
		// message string. Do not use add() with this
		// constructor.

	explicit digest( G::string_view ) ;
		// Constuctor. Calculates a digest for the given
		// message data. Do not use add() with this
		// constructor.

	explicit digest( digest_state ) ;
		// Constructor taking the result of an earlier call
		// to state(). This allows calculation of a digest
		// from a stream of 64-byte blocks to be suspended
		// mid-stream and then resumed using a new digest
		// object.

	digest_state state() const ;
		// Returns the internal state. Typically passed to
		// the Md5Imp::format class.

	void add( const block & ) ;
		// Adds a 64-byte block of the message.

private:
	using aux_fn_t = big_t (*)(big_t, big_t, big_t) ;
	enum class Permutation { ABCD , DABC , CDAB , BCDA } ;
	using P = Permutation ;

private:
	explicit digest( const block & ) ;
	void add( const digest & ) ;
	void init() ;
	void calculate( const block & ) ;
	static big_t T( small_t i ) ;
	static big_t rot32( small_t places , big_t n ) ;
	void operator()( const block & , aux_fn_t , Permutation , small_t , small_t , small_t ) ;
	static big_t op( const block & , aux_fn_t , big_t , big_t , big_t , big_t , small_t , small_t , small_t ) ;
	void round1( const block & ) ;
	void round2( const block & ) ;
	void round3( const block & ) ;
	void round4( const block & ) ;
	static big_t F( big_t x , big_t y , big_t z ) ;
	static big_t G( big_t x , big_t y , big_t z ) ;
	static big_t H( big_t x , big_t y , big_t z ) ;
	static big_t I( big_t x , big_t y , big_t z ) ;
} ;

//| \class G::Md5Imp::format
/// A thin veneer over G::HashState.
///
class G::Md5Imp::format
{
public:
	static std::string encode( const digest_state & ) ;
		// Returns the digest state as a string typically
		// containing non-printing characters.

	static std::string encode( const digest_state & , big_t n ) ;
		// Returns the digest state and a stream-size
		// in the encode() format.

	static digest_state decode( const std::string & , small_t & ) ;
		// Converts a encode() string back into a digest
		// state and a stream-size.

public:
	format() = delete ;
} ;

//| \class G::Md5Imp::block
/// A helper class used by the Md5Imp::digest implementation to represent a
/// 64-character data block.
///
class G::Md5Imp::block
{
public:
	block( const char * data_p , std::size_t data_n , small_t block_offset , big_t end_value ) ;
		// Constructor. The data pointer is kept.
		//
		// The 'block-offset' indicates, in units of 64-character
		// blocks, how far into 'p' the current block's data is.
		//
		// The data pointer must have at least 64 bytes beyond the
		// 'block-offset' point, except for the last block in
		// a message sequence. Note that this is the number
		// of blocks, not the number of bytes.
		//
		// The 'end-value' is derived from the length of the
		// full message (not just the current block). It is only
		// used for the last block. See end().

	static big_t end( small_t data_length ) ;
		// Takes the total number of bytes in the input message and
		// returns a value which can be passed to the constructor's
		// third parameter. This is used for the last block in
		// the sequence of blocks that make up a complete message.

	static small_t blocks( small_t data_length ) ;
		// Takes the total number of bytes in the input message and
		// returns the number of 64-byte blocks, allowing for
		// padding. In practice 0..55 maps to 1, 56..119 maps to
		// 2, etc.

	big_t X( small_t ) const ;
		// Returns a value from within the block. See RFC-1321.

public:
	~block() = default ;
	block( const block & ) = delete ;
	block( block && ) = delete ;
	block & operator=( const block & ) = delete ;
	block & operator=( block && ) = delete ;

private:
	small_t x( small_t ) const ;
	static small_t rounded( small_t n ) ;

private:
	const char * m_p ;
	std::size_t m_n ;
	small_t m_block ;
	big_t m_end_value ;
} ;

// ==

G::Md5Imp::digest::digest() :
	digest_state{}
{
	init() ;
}

G::Md5Imp::digest::digest( const std::string & s ) :
	digest_state{}
{
	init() ;
	small_t nblocks = block::blocks( s.size() ) ;
	for( small_t i = 0U ; i < nblocks ; ++i )
	{
		block blk( s.data() , s.size() , i , block::end(s.size()) ) ;
		add( blk ) ;
	}
}

G::Md5Imp::digest::digest( string_view sv ) :
	digest_state{}
{
	init() ;
	small_t nblocks = block::blocks( sv.size() ) ;
	for( small_t i = 0U ; i < nblocks ; ++i )
	{
		block blk( sv.data() , sv.size() , i , block::end(sv.size()) ) ;
		add( blk ) ;
	}
}

G::Md5Imp::digest::digest( digest_state d_in ) :
	digest_state{}
{
	a = d_in.a ;
	b = d_in.b ;
	c = d_in.c ;
	d = d_in.d ;
}

void G::Md5Imp::digest::init()
{
	a = 0x67452301UL ;
	b = 0xefcdab89UL ;
	c = 0x98badcfeUL ;
	d = 0x10325476UL ;
}

G::Md5Imp::digest::digest_state G::Md5Imp::digest::state() const
{
	big_t mask = 0 ;
	small_t thirty_two = 32U ;
	small_t sizeof_thirty_two_bits = 4U ; // 4x8=32
	if( sizeof(mask) > sizeof_thirty_two_bits )
	{
		mask = ~0U ;
		mask <<= thirty_two ; // ignore warnings here
	}
	digest_state result = { a & ~mask , b & ~mask , c & ~mask , d & ~mask } ;
	return result ;
}

void G::Md5Imp::digest::add( const block & blk )
{
	digest old( *this ) ;
	round1( blk ) ;
	round2( blk ) ;
	round3( blk ) ;
	round4( blk ) ;
	add( old ) ;
}

void G::Md5Imp::digest::add( const digest & other )
{
	a += other.a ;
	b += other.b ;
	c += other.c ;
	d += other.d ;
}

void G::Md5Imp::digest::round1( const block & m )
{
	digest & r = *this ;
	r(m,F,P::ABCD, 0, 7, 1); r(m,F,P::DABC, 1,12, 2); r(m,F,P::CDAB, 2,17, 3); r(m,F,P::BCDA, 3,22, 4);
	r(m,F,P::ABCD, 4, 7, 5); r(m,F,P::DABC, 5,12, 6); r(m,F,P::CDAB, 6,17, 7); r(m,F,P::BCDA, 7,22, 8);
	r(m,F,P::ABCD, 8, 7, 9); r(m,F,P::DABC, 9,12,10); r(m,F,P::CDAB,10,17,11); r(m,F,P::BCDA,11,22,12);
	r(m,F,P::ABCD,12, 7,13); r(m,F,P::DABC,13,12,14); r(m,F,P::CDAB,14,17,15); r(m,F,P::BCDA,15,22,16);
}

void G::Md5Imp::digest::round2( const block & m )
{
	digest & r = *this ;
	r(m,G,P::ABCD, 1, 5,17); r(m,G,P::DABC, 6, 9,18); r(m,G,P::CDAB,11,14,19); r(m,G,P::BCDA, 0,20,20);
	r(m,G,P::ABCD, 5, 5,21); r(m,G,P::DABC,10, 9,22); r(m,G,P::CDAB,15,14,23); r(m,G,P::BCDA, 4,20,24);
	r(m,G,P::ABCD, 9, 5,25); r(m,G,P::DABC,14, 9,26); r(m,G,P::CDAB, 3,14,27); r(m,G,P::BCDA, 8,20,28);
	r(m,G,P::ABCD,13, 5,29); r(m,G,P::DABC, 2, 9,30); r(m,G,P::CDAB, 7,14,31); r(m,G,P::BCDA,12,20,32);
}

void G::Md5Imp::digest::round3( const block & m )
{
	digest & r = *this ;
	r(m,H,P::ABCD, 5, 4,33); r(m,H,P::DABC, 8,11,34); r(m,H,P::CDAB,11,16,35); r(m,H,P::BCDA,14,23,36);
	r(m,H,P::ABCD, 1, 4,37); r(m,H,P::DABC, 4,11,38); r(m,H,P::CDAB, 7,16,39); r(m,H,P::BCDA,10,23,40);
	r(m,H,P::ABCD,13, 4,41); r(m,H,P::DABC, 0,11,42); r(m,H,P::CDAB, 3,16,43); r(m,H,P::BCDA, 6,23,44);
	r(m,H,P::ABCD, 9, 4,45); r(m,H,P::DABC,12,11,46); r(m,H,P::CDAB,15,16,47); r(m,H,P::BCDA, 2,23,48);
}

void G::Md5Imp::digest::round4( const block & m )
{
	digest & r = *this ;
	r(m,I,P::ABCD, 0, 6,49); r(m,I,P::DABC, 7,10,50); r(m,I,P::CDAB,14,15,51); r(m,I,P::BCDA, 5,21,52);
	r(m,I,P::ABCD,12, 6,53); r(m,I,P::DABC, 3,10,54); r(m,I,P::CDAB,10,15,55); r(m,I,P::BCDA, 1,21,56);
	r(m,I,P::ABCD, 8, 6,57); r(m,I,P::DABC,15,10,58); r(m,I,P::CDAB, 6,15,59); r(m,I,P::BCDA,13,21,60);
	r(m,I,P::ABCD, 4, 6,61); r(m,I,P::DABC,11,10,62); r(m,I,P::CDAB, 2,15,63); r(m,I,P::BCDA, 9,21,64);
}

void G::Md5Imp::digest::operator()( const block & m , aux_fn_t aux , Permutation p ,
	small_t k , small_t s , small_t i )
{
	if( p == P::ABCD ) a = op( m , aux , a , b , c , d , k , s , i ) ;
	if( p == P::DABC ) d = op( m , aux , d , a , b , c , k , s , i ) ;
	if( p == P::CDAB ) c = op( m , aux , c , d , a , b , k , s , i ) ;
	if( p == P::BCDA ) b = op( m , aux , b , c , d , a , k , s , i ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::op( const block & m , aux_fn_t aux , big_t a , big_t b , big_t c , big_t d ,
	small_t k , small_t s , small_t i )
{
	return b + rot32( s , ( a + (*aux)( b , c , d ) + m.X(k) + T(i) ) ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::rot32( small_t places , big_t n )
{
	// circular rotate of 32 LSBs, with corruption of higher bits
	big_t overflow_mask = ( big_t(1U) << places ) - big_t(1U) ; // in case big_t is more than 32 bits
	big_t overflow = ( n >> ( small_t(32U) - places ) ) ;
	return ( n << places ) | ( overflow & overflow_mask ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::F( big_t x , big_t y , big_t z )
{
	return ( x & y ) | ( ~x & z ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::G( big_t x , big_t y , big_t z )
{
	return ( x & z ) | ( y & ~z ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::H( big_t x , big_t y , big_t z )
{
	return x ^ y ^ z ;
}

G::Md5Imp::big_t G::Md5Imp::digest::I( big_t x , big_t y , big_t z )
{
	return y ^ ( x | ~z ) ;
}

G::Md5Imp::big_t G::Md5Imp::digest::T( small_t i )
{
	// T = static_cast<big_t>( 4294967296.0 * std::fabs(std::sin(static_cast<double>(i))) ) for 1 <= i <= 64
	//
	static constexpr std::array<big_t,64U> t_map {{
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
		0xeb86d391UL }} ;
	G_ASSERT( i > 0 && i <= t_map.size() ) ;
	return t_map.at(i-1U) ;
}

// ===

std::string G::Md5Imp::format::encode( const digest_state & state )
{
	const std::array<big_t,4U> state_array {{ state.a , state.b , state.c , state.d }} ;
	return HashState<16,big_t,small_t>::encode( &state_array[0] ) ;
}

std::string G::Md5Imp::format::encode( const digest_state & state , big_t n )
{
	const std::array<big_t,4U> state_array {{ state.a , state.b , state.c , state.d }} ;
	return HashState<16,big_t,small_t>::encode( &state_array[0] , n ) ;
}

G::Md5Imp::digest_state G::Md5Imp::format::decode( const std::string & str , small_t & n )
{
	std::array<big_t,4U> state_array {{ 0 , 0 , 0 , 0 }} ;
	G::HashState<16,big_t,small_t>::decode( str , &state_array[0] , n ) ;
	digest_state result = { 0 , 0 , 0 , 0 } ;
	result.a = state_array[0] ;
	result.b = state_array[1] ;
	result.c = state_array[2] ;
	result.d = state_array[3] ;
	return result ;
}

// ===

G::Md5Imp::block::block( const char * data_p , std::size_t data_n , small_t block_in , big_t end_value ) :
	m_p(data_p) ,
	m_n(data_n) ,
	m_block(block_in) ,
	m_end_value(end_value)
{
}

G::Md5Imp::big_t G::Md5Imp::block::end( small_t length )
{
	big_t result = length ;
	result *= 8UL ;
	return result ;
}

G::Md5Imp::small_t G::Md5Imp::block::rounded( small_t raw_byte_count )
{
	small_t n = raw_byte_count + 64U ;
	return n - ( ( raw_byte_count + 8U ) % 64U ) ;
}

G::Md5Imp::small_t G::Md5Imp::block::blocks( small_t raw_byte_count )
{
	small_t byte_count = rounded(raw_byte_count) + 8U ;
	return byte_count / 64UL ;
}

G::Md5Imp::big_t G::Md5Imp::block::X( small_t dword_index ) const
{
	small_t byte_index = ( m_block * 64U ) + ( dword_index * 4U ) ;
	big_t result = x( byte_index + 3U ) ;
	result <<= 8U ; result += x( byte_index + 2U ) ;
	result <<= 8U ; result += x( byte_index + 1U ) ;
	result <<= 8U ; result += x( byte_index + 0U ) ;
	return result ;
}

G::Md5Imp::small_t G::Md5Imp::block::x( small_t i ) const
{
	small_t length = m_n ;
	if( i < length )
	{
		return static_cast<unsigned char>(m_p[i]) ;
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

// ==

G::Md5::Md5() :
	m_d(Md5Imp::digest().state())
{
}

G::Md5::Md5( const std::string & str_state ) :
	m_d(Md5Imp::format::decode(str_state,m_n))
{
	G_ASSERT( str_state.size() == (valuesize()+4U) ) ;
}

std::string G::Md5::state() const
{
	G_ASSERT( Md5Imp::format::encode(m_d,m_n).size() == (valuesize()+4U) ) ;
	return Md5Imp::format::encode( m_d , m_n ) ;
}

void G::Md5::add( const char * data_p , std::size_t data_n )
{
	m_s.append( data_p , data_n ) ;
	m_n += data_n ;
	consume() ;
}

void G::Md5::add( const std::string & data )
{
	m_s.append( data ) ;
	m_n += data.size() ;
	consume() ;
}

void G::Md5::consume()
{
	// consume complete blocks and keep the residue in m_s
	Md5Imp::digest dd( m_d ) ;
	while( m_s.length() >= 64U )
	{
		Md5Imp::block blk( m_s.data() , m_s.size() , 0U , 0UL ) ;
		dd.add( blk ) ;
		m_s.erase( 0U , 64U ) ;
	}
	m_d = dd.state() ;
}

std::string G::Md5::value()
{
	Md5Imp::digest dd( m_d ) ;
	Md5Imp::block blk( m_s.data() , m_s.size() , 0U , Md5Imp::block::end(m_n) ) ;
	dd.add( blk ) ;
	m_s.erase() ;
	m_d = dd.state() ;
	return Md5Imp::format::encode( m_d ) ;
}

#ifndef G_LIB_SMALL
std::string G::Md5::digest( const std::string & input )
{
	Md5Imp::digest dd( input ) ;
	return Md5Imp::format::encode( dd.state() ) ;
}
#endif

#ifndef G_LIB_SMALL
std::string G::Md5::digest( string_view input )
{
	Md5Imp::digest dd( input ) ;
	return Md5Imp::format::encode( dd.state() ) ;
}
#endif

std::string G::Md5::digest( const std::string & input_1 , const std::string & input_2 )
{
	G::Md5 x ;
	x.add( input_1 ) ;
	x.add( input_2 ) ;
	return x.value() ;
}

std::string G::Md5::digest( G::string_view input_1 , G::string_view input_2 )
{
	G::Md5 x ;
	x.add( input_1.data() , input_1.size() ) ;
	x.add( input_2.data() , input_2.size() ) ;
	return x.value() ;
}

std::string G::Md5::digest2( const std::string & input_1 , const std::string & input_2 )
{
	return digest( input_1 , input_2 ) ;
}

std::string G::Md5::predigest( const std::string & input )
{
	Md5 x ;
	x.add( input ) ;
	G_ASSERT( input.size() == blocksize() ) ;
	return x.state().substr(0U,valuesize()) ; // strip off the size; added back in postdigest()
}

std::string G::Md5::postdigest( const std::string & state_pair , const std::string & message )
{
	if( state_pair.size() != 32U ) // valuesize()*2
		throw InvalidState() ;

	const char * _64 = "\x40\0\0\0" ; // state size suffix
	std::string state_i = state_pair.substr( 0U , state_pair.size()/2 ).append(_64,4U) ;
	std::string state_o = state_pair.substr( state_pair.size()/2 ).append(_64,4U) ;

	Md5 xi( state_i ) ;
	xi.add( message ) ;

	Md5 xo( state_o ) ;
	xo.add( xi.value() ) ;

	return xo.value() ;
}

std::size_t G::Md5::blocksize()
{
	return 64U ;
}

std::size_t G::Md5::valuesize()
{
	return 16U ;
}

#ifndef G_LIB_SMALL
std::size_t G::Md5::statesize()
{
	return 20U ;
}
#endif

