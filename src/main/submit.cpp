//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// directory:
//
// * envelope recipient addresses are taken from the command-line
// * envelope recipient addresses are taken from "To:,cc:,bcc:" headers if none on the command-line
// * the envelope "From" address can be specified on the command-line
// * the envelope "From" address is taken from the first "From:,Sender:" header address if not on the command-line
// * a header "From:" line is added if missing using the envelope "From" address
// * content (header+body) is read from stdin or "--input-file" up to EOF (or "." if isatty())
//
// If the verbose switch is used then the full path of the new content file
// is printed on the standard output.
//
// If there are multiple BCC addressees then more than one message will be
// submitted.
//
// usage: submit [options] [--spool-dir <spool-dir>] [--from <envelope-from>] [<envelope-to> [<envelope-to> ...]]
//
// Eg:
//  submit -d -F -t --content `echo Subject: motd | base64` --content = --from me@here you@there < /etc/motd
//

#include "gdef.h"
#include "submitparser.h"
#include "garg.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "goptional.h"
#include "gxtext.h"
#include "ggetopt.h"
#include "goptionsusage.h"
#include "gpath.h"
#include "gfilestore.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gnewmessage.h"
#include "gmessagestore.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gbase64.h"
#include "gexception.h"
#include "legal.h"
#include <exception>
#include <algorithm>
#include <iterator>
#include <array>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdlib>

#ifdef G_WINDOWS
#include <io.h>
static bool isatty_( int fd ) { return _isatty(fd) ; }
#else
#if GCONFIG_HAVE_UNISTD_H
static bool isatty_( int fd ) { return isatty(fd) ; }
#else
static bool isatty_( int ) { return false ; }
#endif
#endif

std::string versionNumber()
{
	return "2.6" ;
}

enum class Parts
{
	All ,
	ToAndCc ,
	OneBcc
} ;

struct SubmitMessage
{
	G::StringArray m_envelope_to_list ;
	G::StringArray m_envelope_bcc_list ;
	G::StringArray m_content_bcc_list ;
	std::string m_envelope_from ;
	std::string m_from_auth_in ;
	std::string m_from_auth_out ;
	G::StringArray m_content ;
} ;

GStore::MessageStore::AddressStyle addressStyle( std::string_view address , std::string_view type )
{
	auto address_style = GStore::MessageStore::addressStyle( address ) ;
	if( address_style == GStore::MessageStore::AddressStyle::Invalid )
		throw std::runtime_error( "invalid " + G::sv_to_string(type) + " address: [" + G::Str::printable(address) + "]" ) ;
	return address_style ;
}

void showInputHelp()
{
	if( G::is_windows() && isatty_(0) && isatty_(1) )
	{
		static bool done = false ;
		if( !done )
			std::cout << "Type e-mail content with ^Z at the end or ^C to quit..." << std::endl ;
		done = true ;
	}
}

std::string appDir()
{
	// see also Main::Run::appDir()
	G::Path this_exe = G::Arg::exe() ;
	if( this_exe.dirname().basename() == "MacOS" && this_exe.dirname().dirname().basename() == "Contents" )
		return this_exe.dirname().dirname().dirname().str() ;
	else
		return this_exe.dirname().str() ;
}

void parseSender( std::string_view line , G::StringArray & out , std::string_view on_line_number )
{
	std::string_view header_field_body = G::Str::tailView( line , ":" ) ;
	SubmitParser::parseAddress( header_field_body , out , on_line_number ) ; // RFC-6854 - address not mailbox
}

void parseFrom( std::string_view line , G::StringArray & out , std::string_view on_line_number )
{
	std::string_view header_field_body = G::Str::tailView( line , ":" ) ;
	SubmitParser::parseAddressList( header_field_body , out , false , on_line_number ) ; // RFC-6854 -- address-list not mailbox-list
}

void parseRecipients( std::string_view line , G::StringArray & out , G::StringArray * content_out_p , std::string_view on_line_number )
{
	std::string_view header_field_body = G::Str::tailView( line , ":" ) ;
	SubmitParser::parseAddressList( header_field_body , out , false , on_line_number ) ;
	if( content_out_p )
		SubmitParser::parseAddressList( header_field_body , *content_out_p , true , on_line_number ) ;
}

bool match( std::string_view line , std::string_view key )
{
	std::size_t pos = 0U ;
	return
		line.size() > key.size() &&
		G::Str::imatch( line.substr(0U,key.size()) , key ) &&
		( pos = line.find_first_not_of( " \t" , key.size() , 2U ) ) != std::string::npos && // NOLINT
		line.at(pos) == ':' ;
}

std::string unfold( const G::StringArray & lines , std::size_t & i )
{
	std::string line = lines.at( i++ ) ;
	while( i < lines.size() && lines[i].find_first_of(" \t") == 0U )
		line.append( lines.at(i++) ) ;
	i-- ;
	return line ;
}

std::unique_ptr<GStore::NewMessage> createMessage( GStore::MessageStore & store , const SubmitMessage & message ,
	Parts parts , const std::string & envelope_bcc = {} , const std::string & content_bcc = {} )
{
	// create the message files
	std::string envelope_from = message.m_envelope_from.empty() ? "anonymous" : message.m_envelope_from ;
	GStore::MessageStore::SmtpInfo smtp_info ;
	smtp_info.auth = message.m_from_auth_in ;
	smtp_info.address_style = addressStyle( envelope_from , "sender" ) ;
	std::unique_ptr<GStore::NewMessage> store_msg = store.newMessage( envelope_from , smtp_info , message.m_from_auth_out ) ;

	// add recipients to the envelope
	if( parts == Parts::All || parts == Parts::ToAndCc )
	{
		for( const auto & to : message.m_envelope_to_list )
		{
			auto address_style = addressStyle( to , "recipient" ) ;
			store_msg->addTo( to , /*is_local=*/false , address_style ) ;
		}
	}
	if( parts == Parts::OneBcc )
	{
		auto address_style = addressStyle( envelope_bcc , "bcc" ) ;
		store_msg->addTo( envelope_bcc , /*is_local=*/false , address_style ) ;
	}
	else if( parts == Parts::All )
	{
		for( const auto & bcc : message.m_envelope_bcc_list )
		{
			auto address_style = addressStyle( bcc , "bcc-recipient" ) ;
			store_msg->addTo( bcc , /*is_local=*/false , address_style ) ;
		}
	}

	// stream out the header section
	{
		bool eoh_in_content = false ;
		for( std::size_t i = 0U ; i < message.m_content.size() ; i++ )
		{
			const std::string & line = message.m_content[i] ;
			if( match( line , "bcc" ) && parts == Parts::OneBcc )
			{
				unfold( message.m_content , i ) ; // ignore it
				store_msg->addContentLine( "bcc: " + content_bcc ) ; // set ours
			}
			else if( match( line , "bcc" ) && parts == Parts::ToAndCc )
			{
				unfold( message.m_content , i ) ;
			}
			else
			{
				eoh_in_content = eoh_in_content || line.empty() ;
				store_msg->addContentLine( line ) ;
			}
		}
		if( !eoh_in_content )
			store_msg->addContentLine( std::string() ) ;
	}

	return store_msg ; // no body and not yet prepare()d or commit()ed
}

void copyIntoSubDirectories( const G::Path & envelope_path )
{
	G::Directory spool_dir( envelope_path.simple() ? G::Path(".") : envelope_path.dirname() ) ;
	std::string envelope_filename = envelope_path.basename() ;
	G::Path src = spool_dir.path() / envelope_filename ;

	G::Process::Umask set_umask( G::Process::Umask::Mode::Tighter ) ; // 0117 => -rw-rw----
	unsigned int dir_count = 0U ;
	unsigned int copy_count = 0U ;
	G::DirectoryIterator iter( spool_dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() )
		{
			dir_count++ ;
			G::Path dst = iter.filePath() / envelope_filename ;
			bool ok = G::File::copy( src , dst , std::nothrow ) ;
			if( ok ) copy_count++ ;
		}
	}
	if( dir_count && dir_count == copy_count )
		G::File::remove( src , std::nothrow ) ;
}

G::Path pathValue( const std::string & s )
{
	G::Path path( s ) ;
	path.replace( "@app" , appDir() ) ;
	return path ;
}

void submit( const G::GetOpt & opt )
{
	// unpack the command-line options
	SubmitMessage message ;
	message.m_envelope_from = opt.value( "from" ) ;
	message.m_from_auth_in = opt.contains("from-auth-in") ?
		( opt.value("from-auth-in","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-in","")) ) :
		std::string() ;
	message.m_from_auth_out = opt.contains("from-auth-out") ?
		( opt.value("from-auth-out","").empty() ? std::string("<>") : G::Xtext::encode(opt.value("from-auth-out","")) ) :
		std::string() ;
	G::Path opt_input_file = pathValue( opt.value("input-file") ) ;
	auto opt_content_base64 = G::Str::splitIntoFields( opt.value("content") , ',' ) ;
	bool opt_read_stdin = !opt.contains( "no-stdin" ) ;
	bool opt_body = opt.contains( "body" ) ;
	bool opt_copy = opt.contains( "copy" ) ;
	bool opt_bcc_split = opt.contains( "bcc-split" ) ;
	bool add_date_header = opt.contains( "content-date" ) ;
	bool opt_add_from_header = opt.contains( "content-from" ) ;
	bool opt_add_to_header = opt.contains( "content-to" ) ;
	bool opt_add_content_message_id = opt.contains( "content-message-id" ) ;
	std::string opt_message_id_domain = opt.value( "content-message-id" , "local" ) ;
	G::Path opt_spool_dir = pathValue( opt.value( "spool-dir" , GStore::FileStore::defaultDirectory().str() ) ) ;

	// take the command-line arguments as envelope-to addresses
	message.m_envelope_to_list = G::Str::splitIntoTokens( opt.value("to") , "," ) ;
	G::StringArray args = opt.args().array( 1U ) ;
	for( const auto & arg : args )
		message.m_envelope_to_list.push_back( arg ) ;
	std::for_each( message.m_envelope_to_list.begin() , message.m_envelope_to_list.end() ,
		[](std::string &to){if(!to.empty()&&to[0]=='\\')to=to.substr(1U);} ) ;

	// open the input file
	std::ifstream input_file ;
	if( !opt_input_file.empty() )
	{
		input_file.open( opt_input_file.iopath() ) ;
		if( !input_file.good() )
			throw std::runtime_error( "cannot open input file [" + opt_input_file.str() + "]" ) ;
	}
	std::istream & stream = opt_input_file.empty() ? std::cin : input_file ;

	// read in headers from the command-line
	auto content_p = opt_content_base64.cbegin() ;
	for( ; content_p != opt_content_base64.cend() ; ++content_p )
	{
		std::string line = (*content_p).size() <= 1U ? std::string() : G::Base64::decode((*content_p),true) ;
		if( line.empty() ) break ;
		message.m_content.push_back( line ) ;
	}

	// read in headers from file
	if( ( opt_read_stdin || !opt_input_file.empty() ) && !opt_body )
	{
		showInputHelp() ;
		while( stream.good() )
		{
			std::string line = G::Str::readLineFrom( stream ) ;
			G::Str::trimRight( line , {"\r",1U} , 1U ) ;
			if( !stream || line.empty() )
				break ;
			message.m_content.push_back( line ) ;
		}
	}

	// if no 'envelope-from' address supplied then get it from the headers
	if( message.m_envelope_from.empty() )
	{
		G::StringArray from_list ;
		G::StringArray sender_list ;
		std::string on_line_number ;
		for( std::size_t i = 0U ; i < message.m_content.size() ; i++ )
		{
			const std::string & line = message.m_content[i] ;
			on_line_number = "line " + std::to_string(i+1U) ;
			if( match( line , "From" ) )
			{
				parseFrom( unfold(message.m_content,i) , from_list , on_line_number ) ;
			}
			else if( match( line , "Sender" ) )
			{
				parseSender( unfold(message.m_content,i) , sender_list , on_line_number ) ;
			}
			if( line.empty() )
				break ;
		}
		if( from_list.size() == 1U ) // RFC-5322 3.6.2
			message.m_envelope_from = from_list.at(0U) ;
		else if( sender_list.size() == 1U )
			message.m_envelope_from = sender_list.at(0U) ;
		else if( !from_list.empty() )
			message.m_envelope_from = from_list.at(0U) ;
		else if( !sender_list.empty() )
			message.m_envelope_from = sender_list.at(0U) ;
		G_LOG_S( "submit: content: from/sender: [" << message.m_envelope_from << "]" ) ;
	}

	// if no 'envelope-to' addresses supplied then get them from the headers
	if( message.m_envelope_to_list.empty() )
	{
		G::StringArray envelope_cc_list ;
		std::string on_line_number ;
		for( std::size_t i = 0U ; i < message.m_content.size() ; i++ )
		{
			const std::string & line = message.m_content[i] ;
			on_line_number = "line " + std::to_string(i+1U) ;
			if( match( line , "To" ) )
				parseRecipients( unfold(message.m_content,i) , message.m_envelope_to_list , nullptr , on_line_number ) ;
			else if( match( line , "cc" ) )
				parseRecipients( unfold(message.m_content,i) , envelope_cc_list , nullptr , on_line_number ) ;
			else if( match( line , "bcc" ) )
				parseRecipients( unfold(message.m_content,i) , message.m_envelope_bcc_list , &message.m_content_bcc_list , on_line_number ) ;
			if( line.empty() )
				break ;
		}
		std::copy( envelope_cc_list.begin() , envelope_cc_list.end() , std::back_inserter(message.m_envelope_to_list) ) ;
		G_LOG_S( "submit: content: to/cc: [" << G::Str::join(",",message.m_envelope_to_list) << "]" ) ;
		G_LOG_S( "submit: content: bcc: [" << G::Str::join(",",message.m_envelope_bcc_list) << "]" ) ;
	}

	// add "Date:" header if requested and none already
	bool have_date_header = std::find_if( message.m_content.cbegin() , message.m_content.cend() ,
		[](const std::string &line){return match(line,"Date");} ) != message.m_content.cend() ;
	if( add_date_header && !have_date_header )
	{
		auto now = G::SystemTime::now() ;
		auto tm = now.local() ;
		G::Date date( tm ) ;
		std::string zone = G::DateTime::offsetString( G::DateTime::offset(now) ) ;
		std::string date_str = date.dd() + " " + date.monthName(true) + " " + date.yyyy() ;
		std::string time_str = G::Time(tm).hhmmss(":") ;
		message.m_content.insert( message.m_content.begin() , G::Str::join(" ","Date:",date_str,time_str,zone) ) ;
	}

	// add "Message-ID:" header if requested and none already
	bool have_id_header = std::find_if( message.m_content.cbegin() , message.m_content.cend() ,
		[](const std::string &line){return match(line,"Message-ID");} ) != message.m_content.cend() ;
	if( opt_add_content_message_id && !have_id_header )
	{
		std::ostringstream ss ;
		ss << "Message-ID: <" << G::SystemTime::now() << "." << G::Process::Id() << "@" << opt_message_id_domain << ">" ;
		message.m_content.insert( message.m_content.begin() , ss.str() ) ;
		G_LOG_S( "submit: added: message-id: [" << ss.str() << "]" ) ;
	}

	// replace all "From:/Sender:" headers if requested
	if( opt_add_from_header )
	{
		message.m_content.erase( std::find_if( message.m_content.begin() , message.m_content.end() ,
			[](const std::string & line){return match(line,"From")||match(line,"Sender");} ) , message.m_content.end() ) ;
		std::string new_content_from = message.m_envelope_from.empty() ? "anonymous:;" : message.m_envelope_from ;
		message.m_content.insert( message.m_content.begin() , "From: "+new_content_from ) ;
		G_LOG_S( "submit: added: from: [" << new_content_from << "]" ) ;
	}

	// replace all "To:/cc:/bcc:" headers with command-line envelope-to arguments, if requested
	if( opt_add_to_header )
	{
		if( message.m_envelope_to_list.empty() )
			throw std::runtime_error( "content-to option used but no envelope-to addresses have been defined" ) ;
		message.m_content.erase( std::find_if( message.m_content.begin() , message.m_content.end() ,
			[](const std::string & line){return match(line,"To")||match(line,"cc")||match(line,"bcc");} ) , message.m_content.end() ) ;
		message.m_content.insert( message.m_content.begin() , "To: "+G::Str::join(",",message.m_envelope_to_list) ) ;
		G_LOG_S( "submit: added: to: [" << G::Str::join(",",message.m_envelope_to_list) << "]" ) ;
	}

	// add remaining command-line body text
	for( ; content_p != opt_content_base64.cend() ; ++content_p )
	{
		std::string line = (*content_p).size() <= 1U ? std::string() : G::Base64::decode((*content_p),true) ;
		message.m_content.push_back( line ) ;
	}

	// create new message files
	GStore::FileStore file_store( opt_spool_dir , "" , {} ) ;
	std::vector<std::unique_ptr<GStore::NewMessage>> store_messages ;
	if( !opt_bcc_split || message.m_envelope_bcc_list.size() <= 1U )
	{
		store_messages.push_back( createMessage( file_store , message , Parts::All ) ) ;
	}
	else
	{
		// RFC-5322 p24 ("In the second case ...")
		store_messages.push_back( createMessage( file_store , message , Parts::ToAndCc ) ) ;
		for( std::size_t bcc_index = 0U ; bcc_index < message.m_envelope_bcc_list.size() ; bcc_index++ )
		{
			std::string envelope_bcc = message.m_envelope_bcc_list[bcc_index] ;
			std::string content_bcc = message.m_content_bcc_list.at( bcc_index ) ;
			store_messages.push_back( createMessage( file_store , message , Parts::OneBcc , envelope_bcc , content_bcc ) ) ;
		}
	}

	// read the message body/bodies from the input stream
	if( opt_read_stdin || !opt_input_file.empty() )
	{
		showInputHelp() ;
		std::string line ;
		while( std::getline(stream,line) )
		{
			G::Str::trimRight( line , {"\r",1U} , 1U ) ;
			if( isatty_(0) && line == "." )
				break ;
			for( auto & store_message : store_messages )
				store_message->addContentLine( line ) ;
		}
	}

	// commit the message files
	for( auto & store_message : store_messages )
	{
		store_message->prepare( {} , "127.0.0.1" , {} ) ;
		store_message->commit( true ) ;
		G::Path new_content = file_store.contentPath( store_message->id() ) ;
		G::Path new_envelope = file_store.envelopePath( store_message->id() ) ;

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

G::Options options()
{
	using G::tx ;
	using M = G::Option::Multiplicity ;
	G::Options opt ;
	unsigned int t_undef = 0U ;

	G::Options::add( opt , 'h' , "help" ,
		tx("shows usage help and exits") , "" ,
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

	G::Options::add( opt , 'x' , "input-file" ,
		tx("reads from the specified file, not standard input") , "" ,
		M::one , "file" , 1 , t_undef ) ;
			// Reads the specified input file, ignoring the
			// standard input stream.

	G::Options::add( opt , 'f' , "from" ,
		tx("sets the envelope-from address") , "" ,
		M::one , "envelope-from-address" , 1 , t_undef ) ;
			// Sets the envelope 'from' address. If not supplied the envelope 'from'
			// address is derived from the originator headers in the message content,
			// or "anonymous" if none.

	G::Options::add( opt , '\0' , "to" ,
		tx("adds an envelope-to address") , "" ,
		M::many , "envelope-to-address" , 2 , t_undef ) ;
			// Adds an envelope 'to' address. Trailing command-line arguments can also
			// be used for envelope 'to' addresses.

	G::Options::add( opt , 't' , "content-to" ,
		tx("adds a 'To:' header using the envelope-to addresses, replacing any existing recipients") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds a "To:" content header using the envelope-to addresses given
			// by "--to" and/or trailing command-line arguments, replacing any
			// existing "To:/cc:/bcc:" headers.

	G::Options::add( opt , 'F' , "content-from" ,
		tx("adds a 'From:' header using the envelope-from address, replacing any existing originators") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds a "From:" content header using the envelope-from address given
			// by "--from", replacing any existing "From:/Sender:" headers.

	G::Options::add( opt , 'b' , "bcc-split" ,
		tx("separate messages if more that one bcc recipient") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Submits messages separately for each "Bcc:" recipient.

	G::Options::add( opt , 'd' , "content-date" ,
		tx("adds a 'Date:' header if none") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Adds a "Date:" content header if there is none.

	G::Options::add( opt , 'I' , "content-message-id" ,
		tx("adds a 'Message-id:' header if none") , "" ,
		M::zero_or_one , "domain-part" , 2 , t_undef ) ;
			// Adds a "Message-ID:" content header if there is none.

	G::Options::add( opt , 'c' , "copy" ,
		tx("copies the envelope file into all sub-directories of the main spool directory") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Copies the envelope file into all sub-directories of the
			// main spool directory.

	G::Options::add( opt , 'n' , "filename" ,
		tx("prints the name of the created content file") , "" ,
		M::zero , "" , 2 , t_undef ) ;
			// Prints the name of the content file.

	G::Options::add( opt , 'C' , "content" ,
		tx("adds a line of content") , "" ,
		M::many , "base64" , 3 , t_undef ) ;
			// Adds a line of content. This can be a header line, a blank line
			// or a line of the body text. The first blank line separates headers
			// from the body. The option value should be base64 encoded.

	G::Options::add( opt , 'N' , "no-stdin" ,
		tx("ignores the standard input stream") , "" ,
		M::zero , "" , 3 , t_undef ) ;
			// Ignores the standard-input. Typically used with "--content".
			// Not needed with "--input-file".

	G::Options::add( opt , 'B' , "body" ,
		tx("treats the input stream or --input-file as body text") , "" ,
		M::zero , "" , 3 , t_undef ) ;
			// Treats the standard input or --input-file as body text with
			// no headers.

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

void run( const G::Arg & arg )
{
	G::GetOpt opt( arg , options() ) ;
	if( opt.hasErrors() )
	{
		opt.showErrors( std::cerr ) ;
	}
	else if( opt.contains("help") )
	{
		std::ostream & stream = std::cout ;
		G::OptionsUsage::Config layout ;
		if( opt.contains("verbose") )
			layout.set_level_max( 3U ) ;
		else
			layout.set_level_max(1U).set_alt_usage() ;

		G::OptionsUsage(opt.options()).output( layout , stream , arg.prefix() , " [<envelope-to-address> ...]" ) ;
		stream << "\n" ;

		stream << "If message content is read from the terminal use ^" << (G::is_windows()?"Z":"D") << " to finish.\n" ;

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
	else
	{
		submit( opt ) ;
	}
}

int main( int argc , char * argv[] )
{
	#ifdef G_WINDOWS
		G::Arg arg = G::Arg::windows() ;
	#else
		G::Arg arg( argc , argv ) ;
	#endif

	try
	{
		bool log_s = arg.remove( "--debug" ) ;
		bool debug = arg.remove( "--debug" ) ;
		G::LogOutput log_output( arg.prefix() , {log_s,debug} ) ;
		run( arg ) ;
		return EXIT_SUCCESS ;
	}
	catch( std::exception & e )
	{
		std::cerr << arg.prefix() << ": " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << arg.prefix() << ": exception" << std::endl ;
	}
	return EXIT_FAILURE ;
}

