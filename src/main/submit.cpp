//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file submit.cpp
///
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
// usage: submit [options] [--spool-dir <spool-dir>] [--from <envelope-from>] [<to> ...]
//

#include "gdef.h"
#include "glocal.h"
#include "gaddress.h"
#include "geventloop.h"
#include "garg.h"
#include "gstr.h"
#include "gxtext.h"
#include "ggetopt.h"
#include "goptionsoutput.h"
#include "gpath.h"
#include "gverifier.h"
#include "gfilestore.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gnewmessage.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gbase64.h"
#include "gexception.h"
#include "legal.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdlib>

G_EXCEPTION_CLASS( NoBody , tx("no body text") ) ;

std::string versionNumber()
{
	return "2.4dev1" ;
}

static std::pair<G::Path,G::Path> writeFiles( const G::Path & spool_dir ,
	const G::StringArray & envelope_to_list , const std::string & from ,
	const std::string & from_auth_in , const std::string & from_auth_out ,
	const G::StringArray & content , std::istream & instream )
{
	// create the output file
	//
	std::string envelope_from = from.empty() ? "anonymous" : from ;
	GSmtp::FileStore file_store( spool_dir , {} ) ;
	GSmtp::MessageStore & store = file_store ;
	GSmtp::MessageStore::SmtpInfo smtp_info ;
	smtp_info.auth = from_auth_in ;
	std::unique_ptr<GSmtp::NewMessage> msg = store.newMessage( envelope_from , smtp_info , from_auth_out ) ;

	// add "To:" lines to the envelope
	//
	for( auto to : envelope_to_list )
	{
		G::Str::trim( to , {" \t\r\n",4U} ) ;
		GSmtp::VerifierStatus status = GSmtp::VerifierStatus::remote( to ) ;
		msg->addTo( status.address , status.is_local ) ;
	}

	// stream out the content header section
	{
		bool eoh_in_content = false ;
		for( const auto & line : content )
		{
			eoh_in_content = eoh_in_content || line.empty() ;
			msg->addContentLine( line ) ;
		}
		if( !eoh_in_content )
			msg->addContentLine( std::string() ) ;
	}

	// read and stream out more content body
	//
	while( instream.good() )
	{
		std::string line = G::Str::readLineFrom( instream ) ;
		G::Str::trimRight( line , {"\r",1U} , 1U ) ;
		if( instream.fail() || line == "." )
			break ;
		msg->addContentLine( line ) ;
	}

	// commit the file
	//
	GNet::Address ip = GNet::Address::loopback( GNet::Address::Family::ipv4 ) ;
	std::string auth_id = std::string() ;
	msg->prepare( auth_id , ip.hostPartString() , std::string() ) ;
	msg->commit( true ) ;

	return {
		file_store.contentPath(msg->id()) ,
		file_store.envelopePath(msg->id()) } ;
}

static void copyIntoSubDirectories( const G::Path & envelope_path )
{
	G::Directory spool_dir( envelope_path.simple() ? G::Path(".") : envelope_path.dirname() ) ;
	std::string envelope_filename = envelope_path.basename() ;
	G::Path src = spool_dir.path() + envelope_filename ;

	G::Process::Umask set_umask( G::Process::Umask::Mode::Tighter ) ; // 0117 => -rw-rw----
	unsigned int dir_count = 0U ;
	unsigned int copy_count = 0U ;
	G::DirectoryIterator iter( spool_dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() )
		{
			dir_count++ ;
			G::Path dst = iter.filePath() + envelope_filename ;
			bool ok = G::File::copy( src , dst , std::nothrow ) ;
			if( ok ) copy_count++ ;
		}
	}
	if( dir_count && dir_count == copy_count )
		G::File::remove( src , std::nothrow ) ;
}

static std::string appDir()
{
	G::Path this_exe = G::Arg::exe() ;
	if( this_exe.dirname().basename() == "MacOS" && this_exe.dirname().dirname().basename() == "Contents" )
		return this_exe.dirname().dirname().dirname().str() ;
	else
		return this_exe.dirname().str() ;
}

static void run( const G::Arg & arg )
{
	G::GetOpt opt( arg ,

		"v!verbose!prints the path of the created content file!!0!!1|"
		"s!spool-dir!specifies the spool directory!!1!dir!1|"
		"f!from!sets the envelope sender!!1!name!1|"

		"t!content-to!add recipients as content headers!!0!!2|"
		"F!content-from!add sender as content header!!0!!2|"
		"d!content-date!add a date content header!!0!!2|"
		"c!copy!copy into spool sub-directories!!0!!2|"
		"n!filename!prints the name of the created content file!!0!!2|"

		"C!content!sets a line of content! and ignores stdin!2!base64!3|"
		"a!auth!sets the envelope authentication value!!1!name!3|"
		"i!from-auth-in!sets the envelope from-auth-in value!!1!name!3|"
		"o!from-auth-out!sets the envelope from-auth-out value!!1!name!3|"

		"h!help!shows this help!!0!!2|"
		"V!version!prints the version and exits!!0!!2|"
	) ;

	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr ) ;
	}
	else if( opt.contains("help") )
	{
		std::ostream & stream = std::cerr ;
		G::OptionsOutputLayout layout ;
		if( opt.contains("verbose") )
			layout.set_level( 3U ) ;
		else
			layout.set_level(1U).set_alt_usage() ;
		G::OptionsOutput(opt.options()).showUsage( layout , stream , arg.prefix() , " <to-address> [<to-address> ...]" ) ;
		if( !opt.contains("verbose") )
			stream << "\nFor complete usage information run \"emailrelay-submit --help --verbose\"" << std::endl ;
		stream
			<< std::endl
			<< Main::Legal::warranty("","\n")
			<< std::endl
			<< Main::Legal::copyright()
			<< std::endl ;
	}
	else if( opt.contains("version") )
	{
		std::cout << versionNumber() << std::endl ;
	}
	else if( opt.args().c() == 1U )
	{
		std::cerr
			<< G::OptionsOutput(opt.options()).usageSummary( {} , arg.prefix() , " <to-address> [<to-address> ...]" )
			<< std::endl ;
	}
	else
	{
		// unpack the command-line options
		bool opt_copy = opt.contains( "copy" ) ;
		std::string opt_spool_dir = opt.value( "spool-dir" , GSmtp::MessageStore::defaultDirectory().str() ) ;
		std::string opt_from = opt.value( "from" ) ;
		G::StringArray opt_content = G::Str::splitIntoTokens( opt.value("content") , "," ) ;
		bool opt_content_date = opt.contains( "content-date" ) ;
		bool opt_content_from = opt.contains( "content-from" ) ;
		bool opt_content_to = opt.contains( "content-to" ) ;
		std::string opt_from_auth_in = opt.contains("from-auth-in") ?
			( opt.value("from-auth-in","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-in","")) ) :
			std::string() ;
		std::string opt_from_auth_out = opt.contains("from-auth-out") ?
			( opt.value("from-auth-out","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-out","")) ) :
			std::string() ;

		// prepare the envelope-to-list from command-line args
		G::StringArray envelope_to_list = opt.args().array(1U) ;
		std::for_each( envelope_to_list.begin() , envelope_to_list.end() ,
			[](std::string &to){if(!to.empty()&&to[0]=='\\')to=to.substr(1U);} ) ;

		// allow @app in the spool-dir
		if( opt_spool_dir.find("@app") == 0U )
			G::Str::replace( opt_spool_dir , "@app" , appDir() ) ;
		G::Path spool_dir( opt_spool_dir ) ;

		// read in the content headers
		G::StringArray content ; // or just headers
		if( opt.contains("content") )
		{
			for( const auto & part : opt_content )
				content.push_back( part.size() <= 1U ? std::string() : G::Base64::decode(part,true) ) ;
		}
		else
		{
			std::istream & stream = std::cin ;
			while( stream.good() )
			{
				std::string line = G::Str::readLineFrom( stream ) ;
				G::Str::trimRight( line , {"\r",1U} , 1U ) ;
				if( line == "." )
					throw NoBody() ;
				if( !stream || line.empty() )
					break ;
				content.push_back( line ) ;
			}
		}

		// find an 'envelope-from' address if necessary from the headers
		std::string from = opt_from ;
		bool from_in_headers = false ;
		if( from.empty() )
		{
			for( const auto & line : content )
			{
				if( line.find("From: ") == 0U )
				{
					from_in_headers = true ;
					from = line.substr(5U) ;
					G::Str::trim( from , {" \t\r\n",4U} ) ;
					G::Str::replaceAll( from , "\r" , "" ) ;
					G::Str::replaceAll( from , std::string(1U,'\0') , "" ) ;
				}
				if( line.empty() )
					break ;
			}
		}

		// add synthetic content headers
		if( opt_content_date )
		{
			bool have_date = false ;
			for( const auto & line : content )
			{
				have_date = line.find("Date: ") == 0U ;
				if( have_date || line.empty() )
					break ;
			}
			if( !have_date )
			{
				auto now = G::SystemTime::now() ;
				auto tm = now.local() ;
				G::Date date( tm ) ;
				std::string zone = G::DateTime::offsetString( G::DateTime::offset(now) ) ;
				std::string date_str = date.dd() + " " + date.monthName(true) + " " + date.yyyy() ;
				std::string time_str = G::Time(tm).hhmmss(":") ;
				content.insert( content.begin() , G::Str::join(" ","Date:",date_str,time_str,zone) ) ;
			}
		}
		if( opt_content_to ) // add 'envelope-to' addresses as content To: headers
		{
			for( const auto & to : envelope_to_list )
				content.insert( content.begin() , "To: "+to ) ;
		}
		if( opt_content_from && !from_in_headers )
		{
			content.insert( content.begin() , "From: "+from ) ;
		}

		// generate the two files
		std::stringstream empty ;
		G::Path new_content ;
		G::Path new_envelope ;
		std::tie( new_content , new_envelope ) = writeFiles( opt_spool_dir ,
			envelope_to_list , from ,
			opt_from_auth_in , opt_from_auth_out , content ,
			opt.contains("content") ? empty : std::cin ) ;

		// copy into spool-dir subdirectories (cf. emailrelay-filter-copy)
		if( opt_copy )
			copyIntoSubDirectories( new_envelope ) ;

		// print the content filename
		if( opt.contains("verbose") )
			std::cout << new_content << std::endl ;
		else if( opt.contains("filename") )
			std::cout << new_content.basename() << std::endl ;
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

