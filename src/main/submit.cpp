//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// submit.cpp
//
// A utility which creates an email message in the E-MailRelay spool
// directory.
//
// It works a bit like sendmail...
// * envelope recipient addresses are taken from the command-line
// * content (header+body) is read from stdin up to "." or EOF
// * the envelope "From" field can be specified on the command-line
// * the envelope "From" field is defaulted from the header "From:" line
// * the header "From:" line is defaulted from the envelope "From" field
//
// If the verbose switch is used then the full path of the new
// content file is printed on the standard output (suitable
// for shell-script backticks).
//
// usage: submit [--verbose] [--spool-dir <spool-dir>] [--from <envelope-from>] [--help] [<to> ...]
//

#include "gdef.h"
#include "glocal.h"
#include "gaddress.h"
#include "geventloop.h"
#include "garg.h"
#include "gstr.h"
#include "gxtext.h"
#include "ggetopt.h"
#include "gpath.h"
#include "gverifier.h"
#include "gfilestore.h"
#include "gnewmessage.h"
#include "gexception.h"
#include "legal.h"
#include <exception>
#include <iostream>
#include <memory>
#include <cstdlib>

G_EXCEPTION_CLASS( NoBody , "no body text" ) ;

static std::string process( const G::Path & spool_dir , std::istream & stream ,
	const G::StringArray & to_list , std::string from ,
	const std::string & from_auth_in , const std::string & from_auth_out ,
	const G::StringArray & header )
{
	// look for a "From:" line in the header if not on the command-line
	//
	if( from.empty() )
	{
		for( const auto & line : header )
		{
			if( line.find("From: ") == 0U )
			{
				from = line.substr(5U) ;
				G::Str::trim( from , " \t\r\n" ) ;
				G::Str::replaceAll( from , "\r" , "" ) ;
				G::Str::replaceAll( from , std::string(1U,'\0') , "" ) ;
			}
		}
	}

	// default the envelope "From" if not on the command-line and not in the header
	//
	std::string envelope_from = from ;
	if( envelope_from.empty() )
		envelope_from = "anonymous" ;

	// create the output file
	//
	GSmtp::FileStore store( spool_dir , /*optimise_empty_test=*/true , /*max_size=*/0U , /*test_for_eight_bit=*/true ) ;
	std::unique_ptr<GSmtp::NewMessage> msg = store.newMessage( envelope_from , from_auth_in , from_auth_out ) ;

	// add "To:" lines to the envelope
	//
	for( auto to : to_list )
	{
		G::Str::trim( to , " \t\r\n" ) ;
		GSmtp::VerifierStatus status( to ) ;
		msg->addTo( status.address , status.is_local ) ;
	}

	// stream out the content header
	{
		bool has_from = false ;
		for( const auto & line : header )
		{
			if( line.find("From: ") == 0U )
				has_from = true ;

			msg->addTextLine( line ) ;
		}
		if( !has_from && !from.empty() )
		{
			msg->addTextLine( std::string("From: ")+from ) ;
		}
		msg->addTextLine( std::string() ) ;
	}

	// read and stream out the content body
	//
	while( stream.good() )
	{
		std::string line = G::Str::readLineFrom( stream ) ;
		G::Str::trimRight( line , "\r" , 1U ) ;
		if( !stream || line == "." )
			break ;
		msg->addTextLine( line ) ;
	}

	// commit the file
	//
	GNet::Address ip = GNet::Address::loopback( GNet::Address::Family::ipv4 ) ;
	std::string auth_id = std::string() ;
	std::string new_path = msg->prepare( auth_id , ip.hostPartString() , std::string() ) ;
	msg->commit( true ) ;
	return new_path ;
}

static G::Path appDir( const std::string & argv0 )
{
	G::Path this_exe = G::Arg::exe() ;
	if( this_exe == G::Path() )
		return G::Path(argv0).dirname() ;
	else if( this_exe.dirname().basename() == "MacOS" && this_exe.dirname().dirname().basename() == "Contents" )
		return this_exe.dirname().dirname().dirname() ;
	else
		return this_exe.dirname() ;
}

G::Path path( const std::string & s_in , const std::string & argv0 )
{
	G::Path result( s_in ) ;
	if( s_in.find("@app") == 0U )
	{
		G::Path app_dir = appDir( argv0 ) ;
		if( app_dir != G::Path() )
		{
			std::string s = s_in ;
			G::Str::replace( s , "@app" , app_dir.str() ) ;
			result = G::Path( s ) ;
		}
	}
	return result ;
}

static void run( const G::Arg & arg )
{
	G::GetOpt opt( arg ,
		"v!verbose!prints the path of the created content file!!0!!1|"
		"s!spool-dir!specifies the spool directory!!1!dir!1|"
		"f!from!sets the envelope sender!!1!name!1|"
		"a!auth!sets the envelope authentication value!!1!name!2|"
		"i!from-auth-in!sets the envelope from-auth-in value!!1!name!2|"
		"o!from-auth-out!sets the envelope from-auth-out value!!1!name!2|"
		"h!help!shows this help!!0!!1" ) ;

	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr ) ;
	}
	else if( opt.contains("help") )
	{
		std::ostream & stream = std::cerr ;
		opt.options().showUsage( stream , arg.prefix() , " <to-address> [<to-address> ...]" ,
			opt.contains("verbose") ? G::Options::introducerDefault() : ("abbreviated "+G::Options::introducerDefault()) ,
			opt.contains("verbose") ? G::Options::levelDefault() : G::Options::Level(1U) ) ;
		stream
			<< std::endl
			<< Main::Legal::warranty("","\n")
			<< std::endl
			<< Main::Legal::copyright()
			<< std::endl ;
	}
	else if( opt.args().c() == 1U )
	{
		std::cerr << opt.options().usageSummary( arg.prefix() , " <to-address> [<to-address> ...]" ) ;
	}
	else
	{
		G::Path spool_dir = GSmtp::MessageStore::defaultDirectory() ;
		if( opt.contains("spool-dir") )
			spool_dir = path( opt.value("spool-dir") , arg.v(0U) ) ;

		std::string from ;
		if( opt.contains("from") )
			from = opt.value("from") ;

		G::StringArray to_list ;
		G::Arg a = opt.args() ;
		for( unsigned int i = 1U ; i < a.c() ; i++ )
		{
			std::string to = a.v(i) ;

			// remove leading backslashes
			if( to.length() >= 1U && *(to.begin()) == '\\' )
				to = to.substr(1U) ;

			to_list.push_back( to ) ;
		}

		std::istream & stream = std::cin ;
		G::StringArray header ;
		while( stream.good() )
		{
			std::string line = G::Str::readLineFrom( stream ) ;
			G::Str::trimRight( line , "\r" , 1U ) ;
			if( line == "." )
				throw NoBody() ;
			if( !stream || line.empty() )
				break ;
			header.push_back( line ) ;
		}

		std::string from_auth_in = opt.contains("from-auth-in") ?
			( opt.value("from-auth-in","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-in","")) ) :
			std::string() ;

		std::string from_auth_out = opt.contains("from-auth-out") ?
			( opt.value("from-auth-out","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-out","")) ) :
			std::string() ;

		std::string new_path = process( spool_dir , stream , to_list , from , from_auth_in , from_auth_out , header ) ;
		if( opt.contains("verbose") )
			std::cout << new_path << std::endl ;
	}
}

int main( int argc , char * argv[] )
{
	G::Arg arg( argc , argv ) ;
	try
	{
		run( arg ) ;
		return EXIT_SUCCESS ;
	}
	catch( std::exception & e )
	{
		std::cerr << arg.prefix() << ": exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << arg.prefix() << ": exception" << std::endl ;
	}
	return EXIT_FAILURE ;
}

/// \file submit.cpp
