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
// content file is printed on the standard output (suitable for
// shell-script backticks).
//
// usage: submit [options] [--spool-dir <spool-dir>] [--from <envelope-from>] [<to> ...]
//
// Eg:
//  submit -d -F -t --content `echo Subject: motd | base64` --content "" --from me@here you@there < /etc/motd
//

#include "gdef.h"
#include "garg.h"
#include "gstr.h"
#include "goptional.h"
#include "gxtext.h"
#include "ggetopt.h"
#include "goptionsusage.h"
#include "gpath.h"
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

std::string versionNumber()
{
	return "2.5.1rc3" ;
}

static std::pair<G::Path,G::Path> writeFiles( const G::Path & spool_dir ,
	const G::StringArray & envelope_to_list , const std::string & from ,
	const std::string & from_auth_in , const std::string & from_auth_out ,
	const G::StringArray & content , std::istream & instream )
{
	// create the output file
	//
	std::string envelope_from = from.empty() ? "anonymous" : from ;
	GStore::FileStore file_store( spool_dir , "" , {} ) ;
	GStore::MessageStore & store = file_store ;
	GStore::MessageStore::SmtpInfo smtp_info ;
	smtp_info.auth = from_auth_in ;
	smtp_info.utf8address = !G::Str::isPrintableAscii( envelope_from ) ;
	std::unique_ptr<GStore::NewMessage> msg = store.newMessage( envelope_from , smtp_info , from_auth_out ) ;

	// add "To:" lines to the envelope
	//
	for( auto to : envelope_to_list )
	{
		G::Str::trim( to , {" \t\r\n",4U} ) ;
		bool is_local = false ;
		bool utf8address = !G::Str::isPrintableAscii( to ) ;
		msg->addTo( to , is_local , utf8address ) ;
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
	std::string auth_id = std::string() ;
	msg->prepare( auth_id , "127.0.0.1" , {} ) ;
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

	G::Options::add( opt , 'v' , "verbose" ,
		tx("prints the path of the created content file") , "" ,
		M::zero , "" , 1 , t_undef ) ;
			// Prints the full path of the content file.

	G::Options::add( opt , 's' , "spool-dir" ,
		tx("specifies the spool directory") , "" ,
		M::one , "dir" , 1 , t_undef ) ;
			// Specifies the spool directory.

	G::Options::add( opt , 'f' , "from" ,
		tx("sets the envelope sender") , "" ,
		M::one , "name" , 1 , t_undef ) ;
			// Sets the envelope 'from' address.

	G::Options::add( opt , 't' , "content-to" ,
		tx("add recipients as content headers") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds the envelope 'to' addresses as "To:" content headers.

	G::Options::add( opt , 'F' , "content-from" ,
		tx("add sender as content header") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds the envelope 'from' addresses as a "From:" content header.

	G::Options::add( opt , 'd' , "content-date" ,
		tx("add a date content header") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds a "Date:" content header if there is none.

	G::Options::add( opt , 'I' , "content-message-id" ,
		tx("add a message-id content header") , "" ,
		M::one , "domain-part" , 2 , t_undef ) ;
			// Adds a "Message-ID:" content header if there is none.

	G::Options::add( opt , 'c' , "copy" ,
		tx("copy into spool sub-directories") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Copies the envelope file into all sub-directories of the
			// main spool directory.

	G::Options::add( opt , 'n' , "filename" ,
		tx("prints the name of the created content file") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Prints the name of the content file.

	G::Options::add( opt , 'C' , "content" ,
		tx("sets a line of content") , "" ,
		M::many , "base64" , 3 , t_undef ) ;
			// Sets a line of content. This can be a header line, a blank line
			// or a line of the body text. The first blank line separates headers
			// from the body. The option value should be base64 encoded.

	G::Options::add( opt , 'N' , "no-stdin" ,
		tx("ignores the standard input stream") , "" ,
		M::zero , "" , 3 , t_undef ) ;
			// Ignores the standard-input. Typically used with "--content".

	G::Options::add( opt , 'a' , "auth" ,
		tx("sets the envelope authentication value") , "" ,
		M::one , "name" , 3 , t_undef ) ;
			// Sets the authentication value in the envelope file.

	G::Options::add( opt , 'i' , "from-auth-in" ,
		tx("sets the envelope from-auth-in value") , "" ,
		M::one , "name" , 3 , t_undef ) ;
			// Sets the 'from-auth-in' value in the envelope file.

	G::Options::add( opt , 'o' , "from-auth-out" ,
		tx("sets the envelope from-auth-out value") , "" ,
		M::one , "name" , 3 , t_undef ) ;
			// Sets the 'from-auth-out' value in the envelope file.

	G::Options::add( opt , 'V' , "version" ,
		tx("prints the version and exits") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Prints the version number and exits.

	return opt ;
}

static void run( const G::Arg & arg )
{
	G::GetOpt opt( arg , options() ) ;
	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr ) ;
	}
	else if( opt.contains("help") )
	{
		std::ostream & stream = std::cerr ;
		G::OptionsUsage::Config layout ;
		if( opt.contains("verbose") )
			layout.set_level_max( 3U ) ;
		else
			layout.set_level_max(1U).set_alt_usage() ;

		G::OptionsUsage(opt.options()).output( layout , stream , arg.prefix() , " <to-address> [<to-address> ...]" ) ;
		stream << "\n" ;

		stream << "If message content is read from the terminal then use ^" << (G::is_windows()?"Z":"D") << " to finish.\n" ;

		if( !opt.contains("verbose") )
		{
			stream << "\n" "For more options use \"--help -v\".\n" ;
		}
		stream
			<< "\n"
			<< Main::Legal::warranty("","\n")
			<< "\n"
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
			<< G::OptionsUsage(opt.options()).summary( {} , arg.prefix() , " <to-address> [<to-address> ...]" )
			<< std::endl ;
	}
	else
	{
		// unpack the command-line options
		bool opt_copy = opt.contains( "copy" ) ;
		bool opt_no_stdin = opt.contains( "no-stdin" ) ;
		std::string opt_spool_dir = opt.value( "spool-dir" , GStore::FileStore::defaultDirectory().str() ) ;
		std::string opt_from = opt.value( "from" ) ;
		G::StringArray opt_content = G::Str::splitIntoFields( opt.value("content") , ',' ) ;
		bool opt_add_content_date = opt.contains( "content-date" ) ;
		bool opt_add_content_from = opt.contains( "content-from" ) ;
		bool opt_add_content_to = opt.contains( "content-to" ) ;
		G::optional<std::string> opt_add_content_id = opt.optional( "content-message-id" ) ;
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
		for( const auto & part : opt_content )
		{
			content.push_back( part.size() <= 1U ? std::string() : G::Base64::decode(part,true) ) ;
		}
		if( !opt_no_stdin )
		{
			std::istream & stream = std::cin ;
			while( stream.good() )
			{
				std::string line = G::Str::readLineFrom( stream ) ;
				G::Str::trimRight( line , {"\r",1U} , 1U ) ;
				if( !stream || line.empty() )
					break ;
				content.push_back( line ) ;
			}
		}

		// find an 'envelope-from' address if necessary from the headers
		std::string from = opt_from ;
		bool have_from_in_headers = false ;
		if( from.empty() )
		{
			for( const auto & line : content )
			{
				if( G::Str::imatch( line.substr(0U,6U) , "From: " ) )
				{
					have_from_in_headers = true ;
					from = line.substr(5U) ;
					G::Str::trim( from , {" \t\r\n",4U} ) ;
					G::Str::replaceAll( from , "\r" , "" ) ;
					G::Str::replaceAll( from , std::string(1U,'\0') , "" ) ;
				}
				if( line.empty() )
					break ;
			}
		}

		// see if we have date and message-id headers
		bool have_date_in_headers = false ;
		bool have_id_in_headers = false ;
		if( opt_add_content_date || opt_add_content_id )
		{
			for( const auto & line : content )
			{
				have_date_in_headers = G::Str::imatch( line.substr(0U,6U) , "Date: " ) ;
				have_id_in_headers = G::Str::imatch( line.substr(0U,12U) , "Message-ID: " ) ;
				if( line.empty() || ( have_date_in_headers && have_id_in_headers ) )
					break ;
			}
		}

		// add synthetic content headers
		if( opt_add_content_date && !have_date_in_headers )
		{
			auto now = G::SystemTime::now() ;
			auto tm = now.local() ;
			G::Date date( tm ) ;
			std::string zone = G::DateTime::offsetString( G::DateTime::offset(now) ) ;
			std::string date_str = date.dd() + " " + date.monthName(true) + " " + date.yyyy() ;
			std::string time_str = G::Time(tm).hhmmss(":") ;
			content.insert( content.begin() , G::Str::join(" ","Date:",date_str,time_str,zone) ) ;
		}
		if( opt_add_content_to ) // add 'envelope-to' addresses as content To: headers
		{
			for( const auto & to : envelope_to_list )
				content.insert( content.begin() , "To: "+to ) ;
		}
		if( opt_add_content_from && !have_from_in_headers )
		{
			content.insert( content.begin() , "From: "+from ) ;
		}
		if( opt_add_content_id && !have_id_in_headers )
		{
			std::ostringstream ss ;
			ss << "Message-ID: <" << G::SystemTime::now() << "." << G::Process::Id() << "@" << opt_add_content_id.value() << ">" ;
			content.insert( content.begin() , ss.str() ) ;
		}

		// generate the two files
		std::stringstream empty ;
		G::Path new_content ;
		G::Path new_envelope ;
		std::tie( new_content , new_envelope ) = writeFiles( opt_spool_dir ,
			envelope_to_list , from ,
			opt_from_auth_in , opt_from_auth_out , content ,
			opt_no_stdin ? empty : std::cin ) ;

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

