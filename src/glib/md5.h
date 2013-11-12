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
// md5.h
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
//

#ifndef MD5_GHW_H
#define MD5_GHW_H

#include <string> // std::string
#include <cstdlib> // std::size_t

namespace md5
{
	typedef std::string string_type ; ///< A string type.
	typedef std::string::size_type size_type ; ///< A std::size_t type.
	typedef size_type big_t ; ///< To hold at least 32 bits, maybe more. Try unsigned long on small systems.
	typedef size_type small_t ; ///< To hold at least a size_t. Must fit in a big_t.
	typedef char assert_big_t_is_big_enough[sizeof(big_t)>=4U?1:-1] ; ///< A static assertion check.
	typedef char assert_small_t_is_big_enough[sizeof(small_t)>=sizeof(size_type)?1:-1]; ///< A static assertion check.
	class digest ;
	class digest_stream ;
	class format ;
	class block ;
}

/// \class md5::digest
/// A class that calculates an md5 digest from one or more 64-byte blocks of
/// data using the algorithm described by RFC 1321.
///
/// Digests are made up of four integers which can be formatted into more
/// usable forms using the md5::format class. 
///
/// A digest can be calculated in one go from an arbitrarily-sized block of
/// data, or incrementally from a series of 64-byte blocks. The 64-byte
/// blocks must be passed as md5::block objects. 
///
/// In practice the requirement for 64-byte blocks of input data may be 
/// inconvenient, so the md5::digest_stream class is provided to allow 
/// calculation of digests from a stream of arbitrarily-sized data blocks.
///
/// \code
///	std::string hash( const std::string & in )
///	{
///		md5::digest d( in ) ;
///		return md5::format::rfc( d ) ;
///	}
/// \endcode
///
class md5::digest 
{
public:
	struct state_type /// Holds the md5 algorithm state. Used by md5::digest.
		{ big_t a ; big_t b ; big_t c ; big_t d ; } ;

	digest() ; 
		///< Default constructor. The message to
		///< be digested should be add()ed
		///< in 64-byte blocks.

	explicit digest( const string_type & s ) ;
		///< Constuctor. Calculates a digest for the 
		///< given message string. Do not use add()
		///< with this constructor.

	explicit digest( state_type ) ;
		///< Constructor taking the result of an 
		///< earlier call to state(). This allows
		///< calculation of a digest from a stream
		///< of 64-byte blocks to be suspended 
		///< mid-stream and then resumed using a 
		///< new digest object.

	state_type state() const ;
		///< Returns the internal state. Typically
		///< passed to the md5::format class.

	void add( const block & ) ;
		///< Adds a 64-byte block of the message.

private:
	typedef big_t (*aux_fn_t)( big_t , big_t , big_t ) ;
	enum Permutation { ABCD , DABC , CDAB , BCDA } ;
	big_t a ;
	big_t b ;
	big_t c ;
	big_t d ;

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

/// \class md5::format
/// A static string-formatting class for the output of md5::digest.
/// Various static methods are provided to convert the 
/// md5::digest::state_type structure into more useful formats, 
/// including the printable format defined by RFC 1321.
///
class md5::format 
{
public:
	static string_type rfc( const digest & ) ;
		///< Returns the digest string in the RFC format.

	static string_type rfc( const digest::state_type & ) ;
		///< Returns the digest string in the RFC format.

	static string_type raw( const digest::state_type & ) ;
		///< Returns the raw digest data as a std::string.
		///< The returned std::string buffer will typically 
		///< contain non-printing characters, including NULs.

private:
	static string_type raw( big_t n ) ;
	static string_type str( big_t n ) ;
	static string_type str2( small_t n ) ;
	static string_type str1( small_t n ) ;
	format() ; // not implemented
} ;

/// \class md5::block
/// A helper class used by the md5::digest implementation to represent a
/// 64-character data block.
///
class md5::block 
{
public:
	block( const string_type & s , small_t block_offset , big_t end_value ) ;
		///< Constructor. Unusually, the string reference is 
		///< kept, so beware of binding temporaries.
		///<
		///< The 'block-offset' indicates, in units of 64-character
		///< blocks, how far down 's' the current block's data is.
		///<
		///< The string must hold at least 64 bytes beyond the
		///< 'block-offset' point, except for the last block in
		///< a message sequence. Note that this is the number
		///< of blocks, not the number of bytes.
		///<
		///< The 'end-value' is derived from the length of the 
		///< full message (not just the current block). It is only
		///< used for the last block. See end().

	static big_t end( small_t data_length ) ;
		///< Takes the total number of bytes in the input message and
		///< returns a value which can be passed to the constructor's
		///< third parameter. This is used for the last block in
		///< the sequence of blocks that make up a complete message.

	static small_t blocks( small_t data_length ) ;
		///< Takes the total number of bytes in the input message and 
		///< returns the number of 64-byte blocks, allowing for
		///< padding. In practice 0..55 maps to 1, 56..119 maps to
		///< 2, etc.

	big_t X( small_t ) const ;
		///< Returns a value from within the block. See RFC 1321.

private:
	block( const block & ) ; // not implemented
	void operator=( const block & ) ; // not implemented
	small_t x( small_t ) const ;
	static small_t rounded( small_t n ) ;

private:
	const string_type & m_s ;
	small_t m_block ;
	big_t m_end_value ;
} ;

/// \class md5::digest_stream
/// A class that calculates an md5 digest from a data stream
/// using the algorithm described by RFC 1321.
///
/// The implementation is layered on top of the block-oriented
/// md5::digest by adding an element of buffering. The buffering 
/// allows incremental calculation of an md5 digest without 
/// requiring either the complete input string or precise 
/// 64-byte blocks.
///
/// \code
///	std::string hash( std::istream & in )
///	{
///		md5::digest_stream d ;
///		while( in.good() )
///		{
///			std::string line ;
///			std::getline( in , line ) ;
///			d.add( line ) ;
///		}
///		d.close() ;
///		return md5::format::rfc( d ) ;
///	}
/// \endcode
///
class md5::digest_stream 
{
public:
	struct state_type ///< Holds the state of an md5 digest stream. Used by md5::digest_stream.
		{ digest::state_type d ; small_t n ; string_type s ; } ;

	digest_stream() ;
		///< Default constructor.

	digest_stream( digest::state_type d , small_t n ) ;
		///< Constructor taking state() allowing digest 
		///< calculation to be suspended and resumed. The 
		///< 'n' parameter must be a multiple of sixty-four 
		///< (since "state_type::s" string is implicitly 
		///< empty).

	void add( const string_type & ) ;
		///< Adds more message data.

	void close() ;
		///< Called after the last add().

	small_t size() const ;
		///< Returns how many data bytes have been
		///< accumulated so far.

	state_type state() const ;
		///< Returns the current state. Only useful after
		///< close().

private:
	digest m_digest ;
	string_type m_buffer ;
	small_t m_length ;
} ;

#endif

