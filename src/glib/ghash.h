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
/// \file ghash.h
///

#ifndef G_HASH_H
#define G_HASH_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class Hash ;
}

//| \class G::Hash
/// A class for creating HMACs using an arbitrary cryptographic hash function
/// as per RFC-2104.
///
class G::Hash
{
public:
	struct Masked  /// An overload discriminator for G::Hash::hmac()
		{} ;

	template <typename Fn2> static std::string hmac( Fn2 digest , std::size_t blocksize , const std::string & key ,
		const std::string & input ) ;
			///< Computes a Hashed Message Authentication Code using the given hash
			///< function. This is typically for challenge-response authentication
			///< where the plaintext input is an arbitrary challenge string from the
			///< server that the client needs to hmac() using their shared private key.
			///<
			///< See also RFC-2104 [HMAC-MD5].
			///<
			///< For hash function H with block size B (64) using shared key SK:
			///<
			///< \code
			///< K = large(SK) ? H(SK) : SK
			///< ipad = 0x36 repeated B times
			///< opad = 0x5C repeated B times
			///< HMAC = H( K XOR opad , H( K XOR ipad , plaintext ) )
			///< \endcode
			///<
			///< The H() function processes a stream of blocks; the first parameter above
			///< represents the first block, and the second parameter is the rest of the
			///< stream (zero-padded up to a block boundary).
			///<
			///< The shared key can be up to B bytes, or if more than B bytes then K is
			///< the L-byte result of hashing the shared-key. K is zero-padded up to
			///< B bytes for XOR-ing.

	template <typename Fn2> static std::string hmac( Fn2 postdigest , const std::string & masked_key ,
		const std::string & input , Masked ) ;
			///< An hmac() overload using a masked key. The postdigest function should
			///< behave like G::Md5::postdigest() and it must throw an exception if the
			///< masked key is invalid.

	template <typename Fn1, typename Fn2> static std::string mask( Fn1 predigest , Fn2 digest ,
		std::size_t blocksize , const std::string & shared_key ) ;
			///< Computes a masked key from the given shared key, returning a non-printable
			///< string. This can be passed to the 'masked' overload of hmac() once the
			///< message is known.
			///<
			///< The predigest and digest functions must behave like G::Md5::predigest()
			///< and G::Md5::digest2().
			///<
			///< A masked key (MK) is the result of doing the initial, plaintext-independent
			///< parts of HMAC computation, taking the intermediate state of both the
			///< inner and outer hash functions.
			///<
			///< \code
			///< K = large(SK) ? H(SK) : SK
			///< HKipad = H( K XOR ipad , )
			///< HKopad = H( K XOR opad , )
			///< MK := ( HKipad , HKopad )
			///< \endcode

	static std::string printable( const std::string & input ) ;
		///< Converts a binary string into a printable form, using a
		///< lowercase hexadecimal encoding.

public:
	Hash() = delete ;

private:
	template <typename Fn2> static std::string keyx( Fn2 , std::size_t blocksize , std::string k ) ;
	static std::string xor_( const std::string & s1 , const std::string & s2 ) ;
	static std::string ipad( std::size_t blocksize ) ;
	static std::string opad( std::size_t blocksize ) ;
} ;

template <typename Fn2>
std::string G::Hash::keyx( Fn2 fn , std::size_t blocksize , std::string k )
{
	if( k.length() > blocksize )
		k = fn( k , std::string() ) ;
	if( k.length() < blocksize )
		k.append( blocksize-k.length() , '\0' ) ;
	return k ;
}

template <typename Fn2>
std::string G::Hash::hmac( Fn2 fn , std::size_t blocksize , const std::string & k , const std::string & input )
{
	const std::string kx = keyx( fn , blocksize , k ) ;
	return fn( xor_(kx,opad(blocksize)) , fn(xor_(kx,ipad(blocksize)),input) ) ;
}

template <typename Fn, typename Fn2>
std::string G::Hash::mask( Fn predigest_fn , Fn2 digest_fn , std::size_t blocksize , const std::string & k )
{
	std::string kx = keyx( digest_fn , blocksize , k ) ;
	std::string ki_state = predigest_fn( xor_(kx,ipad(blocksize)) ) ;
	std::string ko_state = predigest_fn( xor_(kx,opad(blocksize)) ) ;
	return ki_state + ko_state ;
}

template <typename Fn2>
std::string G::Hash::hmac( Fn2 postdigest_fn , const std::string & masked_key , const std::string & input , Masked )
{
	return postdigest_fn( masked_key , input ) ;
}

#endif
