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
//
// passwd.cpp
//
// A utility which encrypts a password so that it can be pasted
// into the emailrelay secrets file(s) and used for CRAM-MD5
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
#include "gbase64.h"
#include "gssl.h"
#include "legal.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

static std::string as_dotted( const std::string & ) ;

namespace imp
{
	std::string hash_function ;
	std::string predigest( const std::string & padded_key )
	{
		GSsl::Digester d( GSsl::Library::instance()->digester(hash_function) ) ;
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
	size_t blocksize()
	{
		GSsl::Digester d( GSsl::Library::instance()->digester(hash_function) ) ;
		return d.blocksize() ;
	}
}

int main( int argc , char * argv [] )
{
	G::Arg arg( argc , argv ) ;
	try
	{
		G::GetOpt opt( arg ,
			"h!help!show usage help!!0!!1" "|"
			"H!hash!use the named hash function! (MD5)!1!function!1" "|"
			"p!password!defines the password! on the command-line (beware command-line history)!2!pwd!2" "|"
			"v!verbose!!!0!!0" "|"
			"d!dotted!use a dotted decimal format! for backwards compatibility!0!!1" "|"
			"t!tls!!!0!!0" "|"
			"T!tls-config!!!1!config!0" "|"
		) ;
		if( opt.hasErrors() )
		{
			opt.showErrors( std::cerr ) ;
			return EXIT_FAILURE ;
		}
		if( opt.contains("help") )
		{
			opt.options().showUsage( std::cout , arg.prefix() , std::string() , G::Options::introducerDefault() ,
				opt.contains("verbose")?G::Options::Level(99):G::Options::Level(1) ) ;

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
		std::string tls_lib_config = opt.value( "tls-config" , "" ) ;
		std::string hash_function = G::Str::upper( opt.value("hash","MD5") ) ;

		bool native = hash_function == "MD5" ;

		// get a list of digest functions from the tls library -- but we can
		// only use ones that have a working state() method
		G::StringArray list ;
		GSsl::Library ssl( !native||tls_lib , tls_lib_config ) ;
		if( !native || tls_lib )
			list = GSsl::Library::digesters( true ) ;

		if( (!native||tls_lib) && std::find(list.begin(),list.end(),hash_function) == list.end() )
			throw std::runtime_error( "invalid hash function" ) ;

		if( dotted && hash_function != "MD5" )
			throw std::runtime_error( "--dotted only works for md5" ) ;

		std::string password = opt.value( "password" , "" ) ;
		if( !opt.contains("password") )
		{
			password = G::Str::readLineFrom( std::cin ) ;
			G::Str::trim( password , " \t\n\r" ) ;
		}
		if( password.empty() )
		{
			std::cerr << arg.prefix() << ": invalid password" << std::endl ;
			return EXIT_FAILURE ;
		}

		std::string result ;
		if( dotted )
		{
			result = as_dotted( G::Hash::mask(G::Md5::predigest,G::Md5::digest2,G::Md5::blocksize(),password) ) ;
		}
		else if( hash_function == "MD5" && !tls_lib )
		{
			result = G::Base64::encode( G::Hash::mask(G::Md5::predigest,G::Md5::digest2,G::Md5::blocksize(),password) ) ;
		}
		else
		{
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
		for( int j = 0 ; j < 4 ; j++ )
		{
			G::Md5::big_t n = static_cast<unsigned char>( *p++ ) ;
			n <<= (8*j) ;
			d |= n ;
		}
		ss << (i==0?"":".") << d ;
	}
	return ss.str() ;
}

/// \file passwd.cpp
