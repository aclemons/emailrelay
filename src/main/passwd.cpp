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
/// \file passwd.cpp
///
// A utility which hashes a password so that it can be pasted
// into the emailrelay secrets file(s) and used for CRAM-xxx
// authentication.
//
// The password should be supplied on the standard input so that
// it is not visible in the command-line history.
//

#include "gdef.h"
#include "gstr.h"
#include "gssl.h"
#include "garg.h"
#include "gmd5.h"
#include "ghash.h"
#include "ggetopt.h"
#include "goptions.h"
#include "goptionsusage.h"
#include "gbase64.h"
#include "gxtext.h"
#include "ggettext.h"
#include "gssl.h"
#include "legal.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

static std::string as_dotted( const std::string & ) ;

namespace PasswdImp
{
	std::string hash_function ;
	std::string predigest( const std::string & padded_key )
	{
		GSsl::Digester d( GSsl::Library::instance()->digester(hash_function,std::string(),true) ) ;
		d.add( padded_key ) ;
		return d.state().substr( 0U , d.valuesize() ) ;
	}
	std::string digest2( const std::string & data_1 , const std::string & data_2 )
	{
		GSsl::Digester d( GSsl::Library::instance()->digester(hash_function) ) ;
		d.add( data_1 ) ;
		d.add( data_2 ) ;
		return d.value() ;
	}
	std::size_t blocksize()
	{
		GSsl::Digester d( GSsl::Library::instance()->digester(hash_function) ) ;
		return d.blocksize() ;
	}
}

static G::Options options()
{
	using G::tx ;
	using M = G::Option::Multiplicity ;
	G::Options opt ;
	unsigned int t_undef = 0U ;

	G::Options::add( opt , 'h' , "help" ,
		tx("show usage help") , "" ,
		M::zero , "" , 1 , t_undef ) ;
			// Shows help text and exits.

	G::Options::add( opt , 'H' , "hash" ,
		tx("use the named hash function! such as MD5") , "" ,
		M::one , "function" , 1 , t_undef ) ;
			// Specifies the hash function, such as MD5 or SHA1.
			// MD5 is the default, and a hash function of NONE does
			// simple xtext encoding. Other hash function may or may
			// not be available, depending on the build.

	G::Options::add( opt , 'p' , "password" ,
		tx("defines the password! on the command-line") , "" ,
		M::one , "pwd" , 2 , t_undef ) ;
			// Specifies the password to be hashed. Beware of leaking
			// sensitive passwords via command-line history or the
			// process-table when using this option.

	G::Options::add( opt , 'b' , "base64" ,
		tx("interpret the password as base64-encoded") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// The input password is interpreted as being Base64 encoded.

	G::Options::add( opt , 'd' , "dotted" ,
		tx("use a dotted decimal format! for backwards compatibility") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Generates a dotted decimal format, for backwards compatibility.

	G::Options::add( opt , 'v' , "verbose" , "verbose" , "" , M::zero , "" , 0 , t_undef ) ;
		// Verbose logging. (undocumented)

	G::Options::add( opt , 't' , "tls" , "tls" , "" , M::zero , "" , 0 , t_undef ) ;
		// Enables the TLS library even if using a hash function of
		// MD5 or NONE. (undocumented)

	G::Options::add( opt , 'T' , "tls-config" , "tls-config" , "" , M::one , "config" , 0 , t_undef ) ;
		// Configures the TLS library with the given configuration
		// string. (undocumented)

	return opt ;
}

int main( int argc , char * argv [] )
{
	G::Arg arg( argc , argv ) ;
	try
	{
		G::GetOpt opt( arg , options() ) ;
		if( opt.hasErrors() )
		{
			opt.showErrors( std::cerr ) ;
			return EXIT_FAILURE ;
		}
		if( opt.contains("help") )
		{
			G::OptionsUsage::Config layout ;
			if( !opt.contains("verbose") )
				layout.set_level_max( 1U ) ;

			G::OptionsUsage(opt.options()).output( layout , std::cout , arg.prefix() ) ;

			std::cout
				<< "\n"
				<< Main::Legal::warranty("","\n")
				<< Main::Legal::copyright() << std::endl ;
			return EXIT_SUCCESS ;
		}
		if( opt.args().c() != 1U )
		{
			std::cerr
				<< arg.prefix() << ": too many command-line arguments "
					"(the password is read from the standard input)" << std::endl
				<< "usage: " << arg.prefix() << std::endl
				<< std::endl
				<< Main::Legal::warranty("  ","\n")
				<< "    " << Main::Legal::copyright() << std::endl ;
			return EXIT_FAILURE ;
		}
		bool dotted = opt.contains( "dotted" ) ;
		bool tls_lib = opt.contains( "tls" ) ;
		std::string tls_lib_config = opt.value( "tls-config" , "mbedtls,ignoreextra" ) ; // prefer mbedtls digesters
		std::string hash_function = G::Str::upper( opt.value("hash","MD5") ) ;

		bool xtext = hash_function == "NONE" ;
		bool native = hash_function == "MD5" || xtext ;

		// get a list of digest functions from the tls library -- but we can
		// only use ones that have a working state() method
		G::StringArray list ;
		GSsl::Library ssl( !native||tls_lib , tls_lib_config ) ;
		if( !native || tls_lib )
			list = GSsl::Library::digesters( true ) ;

		if( (!native||tls_lib) && !xtext && std::find(list.begin(),list.end(),hash_function) == list.end() )
			throw std::runtime_error( "invalid hash function" ) ;

		if( dotted && hash_function != "MD5" )
			throw std::runtime_error( "--dotted only works for md5" ) ;

		std::string password = opt.value( "password" , "" ) ;
		if( !opt.contains("password") )
		{
			password = G::Str::readLineFrom( std::cin ) ;
			G::Str::trim( password , {" \t\n\r",4U} ) ;
		}
		if( password.empty() )
		{
			std::cerr << arg.prefix() << ": invalid password" << std::endl ;
			return EXIT_FAILURE ;
		}
		if( opt.contains("base64") )
			password = G::Base64::decode( password , /*throw_on_invalid=*/true ) ;

		std::string result ;
		if( dotted )
		{
			result = as_dotted( G::Hash::mask(G::Md5::predigest,G::Md5::digest2,G::Md5::blocksize(),password) ) ;
		}
		else if( hash_function == "NONE" )
		{
			result = G::Xtext::encode( password ) ;
		}
		else if( hash_function == "MD5" && !tls_lib )
		{
			result = G::Base64::encode( G::Hash::mask(G::Md5::predigest,G::Md5::digest2,G::Md5::blocksize(),password) ) ;
		}
		else
		{
			namespace imp = PasswdImp ;
			imp::hash_function = hash_function ;
			result = G::Base64::encode( G::Hash::mask(imp::predigest,imp::digest2,imp::blocksize(),password) ) ;
		}

		std::cout << result << std::endl ;
		return EXIT_SUCCESS ;
	}
	catch( std::exception & e )
	{
		std::cerr << arg.prefix() << ": exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << arg.prefix() << ": unknown exception" << std::endl ;
	}
	return EXIT_FAILURE ;
}

std::string as_dotted( const std::string & masked_key )
{
	std::ostringstream ss ;
	std::string mk = masked_key + std::string(64U,'\0') ;
	std::string::iterator p = mk.begin() ;
	for( int i = 0 ; i < 8 ; i++ )
	{
		G::Md5::big_t d = 0U ;
		for( unsigned int j = 0U ; j < 4U ; j++ )
		{
			G::Md5::big_t n = static_cast<unsigned char>( *p++ ) ;
			n <<= (j*8U) ;
			d |= n ;
		}
		ss << (i==0?"":".") << d ;
	}
	return ss.str() ;
}

