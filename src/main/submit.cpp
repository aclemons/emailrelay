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
#include "gnet.h"
#include "gsmtp.h"
#include "glocal.h"
#include "gaddress.h"
#include "geventloop.h"
#include "garg.h"
#include "gstrings.h"
#include "gstr.h"
#include "ggetopt.h"
#include "gpath.h"
#include "gverifier.h"
#include "gfilestore.h"
#include "gnewmessage.h"
#include "gexception.h"
#include "gexecutable.h"
#include "legal.h"
#include <exception>
#include <iostream>
#include <memory>
#include <cstdlib>

G_EXCEPTION( NoBody , "no body text" ) ;

static std::string process( const G::Path & spool_dir , std::istream & stream , 
	const G::Strings & to_list , std::string from , G::Strings header )
{
	// look for a "From:" line in the header if not on the command-line
	//
	if( from.empty() )
	{
		for( G::Strings::const_iterator header_p = header.begin() ; header_p != header.end() ; ++header_p )
		{
			const std::string & line = *header_p ;
			if( line.find("From: ") == 0U )
			{
				from = line.substr(5U) ;
				G::Str::trim( from , " \t\r\n" ) ;
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
	GSmtp::FileStore store( spool_dir ) ;
	std::auto_ptr<GSmtp::NewMessage> msg = store.newMessage( envelope_from ) ;

	// add "To:" lines to the envelope
	//
	for( G::Strings::const_iterator to_p = to_list.begin() ; to_p != to_list.end() ; ++to_p )
	{
		std::string to = *to_p ;
		G::Str::trim( to , " \t\r\n" ) ;
		GSmtp::VerifierStatus status( to ) ;
		msg->addTo( status.address , status.is_local ) ;
	}

	// stream out the header
	{
		bool has_from = false ;
		for( G::Strings::const_iterator header_p = header.begin() ; header_p != header.end() ; ++header_p )
		{
			if( (*header_p).find("From: ") == 0U ) 
				has_from = true ;

			msg->addText( *header_p ) ;
		}
		if( !has_from && !from.empty() )
		{
			msg->addText( std::string("From: ")+from ) ;
		}
		msg->addText( std::string() ) ;
	}

	// read and stream out the content
	while( stream.good() )
	{
		std::string line = G::Str::readLineFrom( stream ) ;
		G::Str::trimRight( line , "\r" , 1U ) ;
		if( !stream || line == "." )
			break ;
		msg->addText( line ) ;
	}

	// commit the file
	//
	GNet::Address ip = GNet::Local::localhostAddress() ;
	std::string auth_id = std::string() ;
	std::string new_path = msg->prepare( auth_id , ip.hostString() , std::string() , std::string() ) ;
	msg->commit() ;
	return new_path ;
}

static void run( const G::Arg & arg )
{
	G::GetOpt opt( arg , 
		"v/verbose/prints the path of the created content file//0//1|"
		"s/spool-dir/specifies the spool directory//1/dir/1|"
		"f/from/sets the envelope sender//1/name/1|"
		"h/help/shows this help//0//1" ) ;

	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr , arg.prefix() ) ;
	}
	else if( opt.contains("help") )
	{
		std::ostream & stream = std::cerr ;
		opt.showUsage( stream , arg.prefix() , std::string() + " <to-address> [<to-address> ...]" ) ;
		stream
			<< std::endl
			<< Main::Legal::warranty("","\n")
			<< std::endl
			<< Main::Legal::copyright()
			<< std::endl ;
	}
	else if( opt.args().c() == 1U )
	{
		std::cerr << opt.usageSummary( arg.prefix() , " <to-address> [<to-address> ...]" ) ;
	}
	else
	{
		G::Path spool_dir = GSmtp::MessageStore::defaultDirectory() ;
		if( opt.contains("spool-dir") )
			spool_dir = opt.value("spool-dir") ;

		std::string from ;
		if( opt.contains("from") )
			from = opt.value("from") ;

		G::Strings to_list ;
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
		G::Strings header ;
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

		std::string new_path = process( spool_dir , stream , to_list , from , header ) ;
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
