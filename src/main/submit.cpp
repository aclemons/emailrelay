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
// usage: submit [--spool-dir <spool-dir>] [--from <envelope-from>] [--help] [<to> ...]
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
#include "gexe.h"
#include "legal.h"
#include <exception>
#include <iostream>
#include <memory>

G_EXCEPTION( NoBody , "no body text" ) ;

static void process( const G::Path & path , std::istream & stream , 
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
	GSmtp::FileStore store( path , G::Executable() , G::Executable() ) ;
	std::auto_ptr<GSmtp::NewMessage> msg = store.newMessage( envelope_from ) ;

	// add "To:" lines to the envelope
	//
	G::Executable no_exe ;
	GSmtp::Verifier verifier( no_exe , false , false ) ;
	for( G::Strings::const_iterator to_p = to_list.begin() ; to_p != to_list.end() ; ++to_p )
	{
		std::string to = *to_p ;
		G::Str::trim( to , " \t\r\n" ) ;
		GSmtp::Verifier::Status status = verifier.verify( to , "" , GNet::Address::localhost(0U) , "" , "" ) ;
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

	// stream out the content
	while( stream.good() )
	{
		std::string line = G::Str::readLineFrom( stream ) ;
		if( line == "." )
			break ;
		msg->addText( line ) ;
	}

	// commit the file
	//
	GNet::Address ip = GNet::Local::localhostAddress() ;
	std::string auth_id = std::string() ;
	msg->store( auth_id , ip.hostString() ) ;
}

static void run( const G::Arg & arg )
{
	G::GetOpt getopt( arg , 
		"s/spool-dir/specifies the spool directory/1/dir/1|"
		"f/from/sets the envelope sender/1/name/1|"
		"h/help/shows this help/0//1" ) ;

	if( getopt.hasErrors() )
	{
		getopt.showErrors( std::cerr , arg.prefix() ) ;
	}
	else if( getopt.contains("help") )
	{
		std::ostream & stream = std::cerr ;
		getopt.showUsage( stream , arg.prefix() , std::string(" <to-address> [<to-address> ...]") ) ;
		stream
			<< std::endl
			<< Main::Legal::warranty("","\n")
			<< std::endl
			<< Main::Legal::copyright()
			<< std::endl ;
	}
	else if( getopt.args().c() == 1U )
	{
		std::cerr 
			<< getopt.usageSummary( arg.prefix() , " <to-address> [<to-address> ...]" ) 
			<< std::endl ;
	}
	else
	{
		std::auto_ptr<GNet::EventLoop> event_loop( GNet::EventLoop::create() ) ;
		event_loop->init() ;

		G::Path path = GSmtp::MessageStore::defaultDirectory() ;
		if( getopt.contains("spool-dir") )
			path = getopt.value("spool-dir") ;

		std::string from ;
		if( getopt.contains("from") )
			from = getopt.value("from") ;

		G::Strings to_list ;
		G::Arg a = getopt.args() ;
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
			if( line == "." )
				throw NoBody() ;
			if( line.empty() )
				break ;
			header.push_back( line ) ;
		} 

		process( path , stream , to_list , from , header ) ;
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

