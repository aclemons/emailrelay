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
/// \file submission.cpp
///

#include "gdef.h"
#include "submission.h"
#include "gaddress.h"
#include "garg.h"
#include "gstringarray.h"
#include "gpath.h"
#include "gverifier.h"
#include "gfilestore.h"
#include "gnewmessage.h"
#include "gmessagestore.h"
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace SubmissionImp
{
	void submit( G::Arg ) ;
	std::pair<G::Path,G::Path> writeFiles( const G::Path & spool_dir ,
		const std::string & from , const G::StringArray & envelope_to_list ,
		std::istream & instream ) ;
}

bool Main::Submission::enabled( const G::Arg & arg )
{
	return
		arg.prefix().find( "emailrelay" ) == 0U &&
		arg.prefix().find( "submit" ) == 11U ;
}

int Main::Submission::submit( const G::Arg & arg )
{
	SubmissionImp::submit( arg ) ;
	return 0 ;
}

void SubmissionImp::submit( G::Arg arg )
{
	if( arg.contains("-h") || arg.contains("--help") )
	{
		std::cout << arg.prefix() << " [-d <spool-dir>] [-f <envelope-from>]" << std::endl ;
	}
	else
	{
		unsigned int shift = 1U + (arg.contains("-d")?1U:0U) + (arg.contains("-f")?1U:0U) ;
		std::string spool_dir = arg.removeValue( "-d" , GStore::FileStore::defaultDirectory().str() ) ;
		std::string from = arg.removeValue( "-f" ) ;
		auto paths = writeFiles( spool_dir , from , arg.array(shift) , std::cin ) ;
		std::cout << paths.first << std::endl ;
	}
}

std::pair<G::Path,G::Path> SubmissionImp::writeFiles( const G::Path & spool_dir ,
	const std::string & from , const G::StringArray & envelope_to_list ,
	std::istream & instream )
{
	// create the output file
	//
	std::string envelope_from = from.empty() ? "anonymous" : from ;
	GStore::FileStore file_store( spool_dir , "" , {} ) ;
	GStore::MessageStore & store = file_store ;
	GStore::MessageStore::SmtpInfo smtp_info ;
	std::unique_ptr<GStore::NewMessage> msg = store.newMessage( envelope_from , smtp_info , {} ) ;

	// add "To:" lines to the envelope
	//
	for( auto to : envelope_to_list )
	{
		G::Str::trim( to , {" \t\r\n",4U} ) ;
		GSmtp::VerifierStatus status = GSmtp::VerifierStatus::remote( to ) ;
		msg->addTo( status.address , status.is_local , GStore::MessageStore::addressStyle(status.address) ) ;
	}

	// read and stream out more content body
	//
	std::string line ;
	while( G::Str::readLine( instream , line ) )
	{
		G::Str::trimRight( line , {"\r",1U} , 1U ) ;
		if( instream.fail() || line == "." )
			break ;
		msg->addContentLine( line ) ;
	}

	// commit the file
	//
	GNet::Address ip = GNet::Address::loopback( GNet::Address::Family::ipv4 ) ;
	std::string auth_id {} ;
	msg->prepare( auth_id , ip.hostPartString() , {} ) ;
	msg->commit( true ) ;

	return {
		file_store.contentPath(msg->id()) ,
		file_store.envelopePath(msg->id()) } ;
}

