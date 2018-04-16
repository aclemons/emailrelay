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
// gexecutableverifier.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstrings.h"
#include "gexecutableverifier.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gfile.h"
#include "groot.h"
#include "gstr.h"
#include "glocal.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ExecutableVerifier::ExecutableVerifier( GNet::ExceptionHandler & eh , const G::Path & path , bool compatible ) :
	m_path(path) ,
	m_task(*this,eh,"<<verifier exec error: __strerror__>>",G::Root::nobody()) ,
	m_compatible(compatible)
{
}

void GSmtp::ExecutableVerifier::verify( const std::string & to_address ,
	const std::string & from_address , const GNet::Address & ip ,
	const std::string & mechanism , const std::string & extra )
{
	G_DEBUG( "GSmtp::ExecutableVerifier::verify: to \"" << to_address << "\": from \"" << from_address << "\": "
		<< "ip \"" << ip.hostPartString() << "\": auth-mechanism \"" << mechanism << "\": "
		<< "auth-extra \"" << extra << "\"" ) ;

	G::Executable commandline( m_path.str() ) ;
	if( m_compatible )
	{
		std::string user = G::Str::head( to_address , to_address.find('@') , to_address ) ;
		std::string host = G::Str::tail( to_address , to_address.find('@') , std::string() ) ;
		commandline.add( to_address ) ;
		commandline.add( G::Str::upper(user) ) ;
		commandline.add( G::Str::upper(host) ) ;
		commandline.add( G::Str::upper(GNet::Local::canonicalName()) ) ;
		commandline.add( from_address ) ;
		commandline.add( ip.hostPartString() ) ;
		commandline.add( mechanism ) ;
		commandline.add( extra ) ;
	}
	else
	{
		commandline.add( to_address ) ;
		commandline.add( GNet::Local::canonicalName() ) ;
		commandline.add( from_address ) ;
		commandline.add( ip.hostPartString() ) ;
		commandline.add( mechanism ) ;
		commandline.add( extra ) ;
	}

	G_LOG( "GSmtp::ExecutableVerifier: executing " << commandline.displayString() ) ;
	m_to_address = to_address ;
	m_task.start( commandline ) ;
}

void GSmtp::ExecutableVerifier::onTaskDone( int exit_code , const std::string & response_in )
{
	std::string response( response_in ) ;
	G_LOG( "GSmtp::ExecutableVerifier: " << exit_code << ": \"" << G::Str::printable(response) << "\"" ) ;
	G::Str::trimRight( response , " \n\t" ) ;
	G::Str::replaceAll( response , "\r\n" , "\n" ) ;
	G::Str::replaceAll( response , "\r" , "" ) ;
	G::StringArray response_parts ;
	response_parts.reserve( 2U ) ;
	G::Str::splitIntoFields( response , response_parts , "\n" ) ;

	VerifierStatus status ;
	if( ( exit_code == 0 || exit_code == 1 ) && response_parts.size() >= 2 )
	{
		status.is_valid = true ;
		status.is_local = exit_code == 0 ;
		status.full_name = response_parts.at( 0U ) ;
		status.address = response_parts.at( 1U ) ;
	}
	else if( exit_code == 100 )
	{
		status.is_valid = false ;
		status.abort = true ;
	}
	else
	{
		status.is_valid = false ;
		status.temporary = exit_code == 3 ;
		status.reason = response.empty() ? G::Str::fromInt(exit_code) : response ;
		G::Str::replaceAll( status.reason , "\n" , " " ) ;
		status.reason = G::Str::printable( status.reason ) ;
		status.help = "rejected by external verifier program" ;
	}

	doneSignal().emit( m_to_address , status ) ;
}

G::Slot::Signal2<std::string,GSmtp::VerifierStatus> & GSmtp::ExecutableVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableVerifier::cancel()
{
}

/// \file gexecutableverifier.cpp
