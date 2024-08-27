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
/// \file gmd5.h
///

#ifndef G_MD5_H
#define G_MD5_H

#include "gdef.h"
#include "gexception.h"
#include "gstringview.h"
#include <string>

namespace G
{
	class Md5 ;
}

//| \class G::Md5
/// MD5 message digest class.
///
/// Eg:
/// \code
/// Md5 h1 ;
/// std::string x = ... ;
/// std::string y = ... ;
/// assert( (x.size() % 64U) == 0 ) ;
/// h1.add( x ) ;
/// Md5 h2( h1.state() ) ;
/// h2.add( y ) ;
/// assert( h2.value() == Md5::digest(x+y) ) ;
/// \endcode
///
class G::Md5
{
public:
	G_EXCEPTION( Error , tx("internal md5 error") )
	G_EXCEPTION_CLASS( InvalidState , tx("invalid md5 hash state") )
	using big_t = std::size_t ; // To hold at least 32 bits.
	using small_t = std::size_t ; // To hold at least a std::size_t and no bigger than a big_t.
	struct digest_state /// Holds the four parts of the md5 state.
		{ big_t a ; big_t b ; big_t c ; big_t d ; } ;
	struct digest_stream_state /// Holds the md5 state plus unprocessed residual data.
		{ digest_state d ; small_t n ; std::string s ; } ;
	static_assert( sizeof(big_t) >= 4 , "" ) ;
	static_assert( sizeof(small_t) >= sizeof(std::size_t) && sizeof(small_t) <= sizeof(big_t) , "" ) ;

	Md5() ;
		///< Default constructor.

	explicit Md5( const std::string & state ) ;
		///< Constructor using an intermediate state() string.
		///< Precondition: state.size() == 20

	std::string state() const ;
		///< Returns the current intermediate state as a 20
		///< character string, although this requires the size of
		///< the added data is a multiple of the blocksize().
		///< Note that the trailing 4 characters represent
		///< the total size of the added data.
		/// \see G::HashState

	void add( const std::string & data ) ;
		///< Adds more data.

	void add( const char * data , std::size_t size ) ;
		///< Adds more data.

	std::string value() ;
		///< Returns the hash value as a 16-character string. No
		///< more add()s are allowed. The resulting string is not
		///< generally printable and it may have embedded nulls.
		/// \see G::HashState, G::Hash::printable().

	static std::size_t blocksize() ;
		///< Returns the block size in bytes (64).

	static std::size_t valuesize() ;
		///< Returns the value() size in bytes (16).

	static std::size_t statesize() ;
		///< Returns the size of the state() string (20).

	static std::string digest( const std::string & input ) ;
		///< A convenience function that returns a digest from
		///< one input.

	static std::string digest( std::string_view input ) ;
		///< A convenience function that returns a digest from
		///< one input.

	static std::string digest( const std::string & input_1 , const std::string & input_2 ) ;
		///< A convenience function that returns a digest from
		///< two inputs.

	static std::string digest( std::string_view input_1 , std::string_view input_2 ) ;
		///< A convenience function that returns a digest from
		///< two inputs.

	static std::string digest2( const std::string & input_1 , const std::string & input_2 ) ;
		///< A non-overloaded name for the digest() overload
		///< taking two parameters.

	static std::string predigest( const std::string & padded_key ) ;
		///< A convenience function that add()s the given string
		///< of length blocksize() (typically a padded key) and
		///< returns the resulting state() truncated to valuesize()
		///< characters.

	static std::string postdigest( const std::string & state_pair , const std::string & message ) ;
		///< A convenience function that returns the value()
		///< from an outer digest that is initialised with the
		///< second half of the state pair, and with the value()
		///< of an inner digest add()ed; the inner digest being
		///< initialised with the first half of the state pair,
		///< and with the given message add()ed. The result is
		///< a string of 32 non-printing characters. Throws
		///< InvalidState if the state-pair string is not valid.

public:
	~Md5() = default ;
	Md5( const Md5 & ) = delete ;
	Md5( Md5 && ) = delete ;
	Md5 & operator=( const Md5 & ) = delete ;
	Md5 & operator=( Md5 && ) = delete ;

private:
	void consume() ;

private:
	std::size_t m_n{0U} ;
	digest_state m_d ;
	std::string m_s ;
} ;

#endif
