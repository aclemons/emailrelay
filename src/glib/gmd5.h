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
///
/// \file gmd5.h
///

#ifndef G_MD5_H
#define G_MD5_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class Md5 ;
}

/// \class G::Md5
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
	G_EXCEPTION( Error , "internal md5 error" ) ;
	typedef size_t big_t ; // To hold at least 32 bits.
	typedef size_t small_t ; // To hold at least a size_t and no bigger than a big_t.
	struct digest_state /// Holds the four parts of the md5 state.
		{ big_t a ; big_t b ; big_t c ; big_t d ; } ;
	struct digest_stream_state /// Holds the md5 state plus unprocessed residual data.
		{ digest_state d ; small_t n ; std::string s ; } ;

	Md5() ;
		///< Default constructor.

	explicit Md5( const std::string & state ) ;
		///< Constructor using an intermediate state() string.

	std::string state() const ;
		///< Returns the current intermediate state as a 20
		///< character string, although this requires the size of
		///< the added data is a multiple of the blocksize().
		///< Note that the trailing 4 characters represent
		///< the total size of the added data.
		/// \see G::HashState

	void add( const std::string & data ) ;
		///< Adds more data.

	std::string value() ;
		///< Returns the hash value as a 16-character string. No
		///< more add()s are allowed. The resulting string is not
		///< generally printable and it may have embedded nulls.
		/// \see G::HashState, G::Hash::printable().

	static size_t blocksize() ;
		///< Returns the block size in bytes (64).

	static size_t valuesize() ;
		///< Returns the value() size in bytes (16).

	static size_t statesize() ;
		///< Returns the size of the state() string (20).

	static std::string digest( const std::string & input ) ;
		///< A convenience function that returns a digest from
		///< one input.

	static std::string digest( const std::string & input_1 , const std::string & input_2 ) ;
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
		///< a string of 32 non-printing characters.

private:
	Md5( const Md5 & ) ;
	void operator=( const Md5 & ) ;

private:
	digest_state m_d ;
	size_t m_n ;
	std::string m_s ;
} ;

#endif
