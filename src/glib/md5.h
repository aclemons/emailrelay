//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// md5.h
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

#ifndef MD5_GHW_H
#define MD5_GHW_H

#include <string>

namespace md5
{
	typedef unsigned long big_t ; // at least 32 bits, may be more
	typedef unsigned int small_t ; // at least size_t
	typedef char assert_big_t_is_big_enough[sizeof(big_t)>=4U?1:-1] ;
	class digest ;
	class digest_stream ;
	class format ;
	class message ;
}

// Class: md5::digest
// Description: An md5 digest class. See RFC 1321.
//
class md5::digest 
{
public:
	struct state_type { big_t a ; big_t b ; big_t c ; big_t d ; } ;

	explicit digest( const std::string & s ) ;
		// Constuctor. Calculates a digest for the 
		// given message string.

	explicit digest( state_type ) ;
		// Constructor taking the result of an 
		// earlier call to state().

	state_type state() const ;
		// Returns the internal state. Typically
		// passed to the md5::format class.

	digest() ; 
		// Default constructor. The message to
		// be digested should be add()ed
		// in 64-byte blocks.

	void add( const message & block ) ;
		// Adds a 64-byte block of the message.

private:
	typedef big_t (*aux_fn_t)( big_t , big_t , big_t ) ;
	enum Permutation { ABCD , DABC , CDAB , BCDA } ;
	big_t a ;
	big_t b ;
	big_t c ;
	big_t d ;

private:
	explicit digest( const message & m ) ;
	digest( const digest & ) ;
	void add( const digest & ) ;
	void init() ;
	void calculate( const message & ) ;
	static big_t T( small_t i ) ;
	static big_t rot32( small_t places , big_t n ) ;
	void operator()( const message & , aux_fn_t , Permutation , small_t , small_t , small_t ) ;
	static big_t op( const message & , aux_fn_t , big_t , big_t , big_t , big_t , small_t , small_t , small_t ) ;
	void round1( const message & ) ;
	void round2( const message & ) ;
	void round3( const message & ) ;
	void round4( const message & ) ;
	static big_t F( big_t x , big_t y , big_t z ) ;
	static big_t G( big_t x , big_t y , big_t z ) ;
	static big_t H( big_t x , big_t y , big_t z ) ;
	static big_t I( big_t x , big_t y , big_t z ) ;
} ;

// Class: md5::format
// Description: A static string-formatting class for the output 
// of md5::digest.
//
class md5::format 
{
public:
	static std::string rfc( const digest & ) ;
		// Returns the digest string in the RFC format.

	static std::string rfc( const digest::state_type & ) ;
		// Returns the digest string in the RFC format.

	static std::string raw( const digest::state_type & ) ;
		// Returns the raw digest data as a std::string.
		// The returned std::string buffer will typically 
		// contain non-printing characters, including NULs.

private:
	static std::string raw( big_t n ) ;
	static std::string str( big_t n ) ;
	static std::string str2( small_t n ) ;
	static std::string str1( small_t n ) ;
	format() ; // not implemented
} ;

// Class: md5::message
// Description: A helper class for md5::digest representing a
// 64-character data block.
//
class md5::message 
{
public:
	message( const std::string & s , small_t block_offset , big_t end_value ) ;
		// Constructor. Unusually, the string reference is 
		// kept, so beware of temporaries.
		//
		// The 'block-offset' indicates, in units of 64-character
		// blocks, how far down 's' the current block's data is.
		//
		// The 'end-value' is derived from the length of the 
		// full string (not just the current block). It is only
		// used for the last block. See end().

	static big_t end( small_t data_length ) ;
		// Takes the total number of bytes in the input data and 
		// returns a value which can be passed to the constructor's
		// third parameter.

	static small_t blocks( small_t data_length ) ;
		// Takes the total number of bytes in the input data and 
		// returns the number of 64-byte blocks, allowing for
		// padding. In practice 0..55 maps to 1, 56..119 maps to
		// 2, etc.

private:
	friend class digest ;
	message( const message & ) ; // not implemented
	void operator=( const message & ) ; // not implemented
	big_t X( small_t ) const ;
	small_t x( small_t ) const ;
	static small_t rounded( small_t n ) ;

private:
	const std::string & m_s ;
	small_t m_block ;
	big_t m_end_value ;
} ;

// Class: md5::digest_stream
// Description: An md5 digest class with buffering. The buffering allows
// incremental calculation of an md5 digest, without requiring either
// the complete input string or precise 64-byte blocks.
//
class md5::digest_stream 
{
public:
	struct state_type { digest::state_type d ; small_t n ; std::string s ; } ;

	digest_stream() ;
		// Default constructor.

	digest_stream( digest::state_type d , small_t n ) ;
		// Constructor taking state(). The "state_type::s" string
		// is implicitly empty, so 'n' must be a multiple
		// of sixty-four.

	void add( const std::string & ) ;
		// Adds more message data.

	void close() ;
		// Called after the last add().

	small_t size() const ;
		// Returns how many data bytes have been
		// accumulated so far.

	state_type state() const ;
		// Returns the current state. Only useful after
		// close().

private:
	digest m_digest ;
	std::string m_buffer ;
	small_t m_length ;
} ;

#endif

