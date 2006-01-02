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
// gmd5.h
//

#ifndef G_MD5_H
#define G_MD5_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class Md5 ;
}

// Class: G::Md5
// Description: MD5 class.
//
class G::Md5 
{
public:
	G_EXCEPTION( InvalidMaskedKey , "invalid md5 key" ) ;
	struct Masked  // An overload discriminator for G::Md5::hmac()
		{} ;

	static std::string digest( const std::string & input ) ;
		// Creates an MD5 digest. The resulting
		// string is not generally printable and
		// may have embedded NULs.

	static std::string digest( const std::string & input_1 , const std::string & input_2 ) ;
		// An overload which processes two input strings.

	static std::string printable( const std::string & input ) ;
		// Converts a binary string into a printable
		// form, using a lowercase hexadecimal encoding.
		// See also RFC2095.

	static std::string hmac( const std::string & key , const std::string & input ) ;
		// Computes a Hashed Message Authentication Code
		// using MD5 as the hash function.
		// See also RFC2104 [HMAC-MD5].

	static std::string hmac( const std::string & masked_key , const std::string & input , Masked ) ;
		// An hmac() overload using a masked key.

	static std::string mask( const std::string & key ) ;
		// Masks an HMAC key so that it can be stored more safely.

private:
	static std::string digest( const std::string & input_1 , const std::string * input_2 ) ;
	static std::string mask( const std::string & k64 , const std::string & pad ) ;
	static std::string xor_( const std::string & , const std::string & ) ;
	static std::string key64( std::string ) ;
	static std::string ipad() ;
	static std::string opad() ;
	Md5() ;
} ;

#endif
