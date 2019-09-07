//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gexecutableverifier.h"
#include "gexecutablecommand.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gfile.h"
#include "groot.h"
#include "gstr.h"
#include "glocal.h"
#include "glog.h"

GSmtp::ExecutableVerifier::ExecutableVerifier( GNet::ExceptionSink es , const G::Path & path ) :
	m_path(path) ,
	m_task(*this,es,"<<verifier exec error: __strerror__>>",G::Root::nobody())
{
}

void GSmtp::ExecutableVerifier::verify( const std::string & to_address ,
	const std::string & from_address , const GNet::Address & ip ,
	const std::string & auth_mechanism , const std::string & auth_extra )
{
	G_DEBUG( "GSmtp::ExecutableVerifier::verify: to \"" << to_address << "\": from \"" << from_address << "\": "
		<< "ip \"" << ip.hostPartString() << "\": auth-mechanism \"" << auth_mechanism << "\": "
		<< "auth-extra \"" << auth_extra << "\"" ) ;

	G::ExecutableCommand commandline( m_path.str() , G::StringArray() ) ;
	commandline.add( to_address ) ;
	commandline.add( from_address ) ;
	commandline.add( ip.displayString() ) ;
	commandline.add( GNet::Local::canonicalName() ) ;
	commandline.add( G::Str::lower(auth_mechanism) ) ;
	commandline.add( auth_extra ) ;

	G_LOG( "GSmtp::ExecutableVerifier: address verifier: executing " << commandline.displayString() ) ;
	m_to_address = to_address ;
	m_task.start( commandline ) ;
}

void GSmtp::ExecutableVerifier::onTaskDone( int exit_code , const std::string & response_in )
{
	std::string response( response_in ) ;
	G::Str::trimRight( response , " \n\t" ) ;
	G::Str::replaceAll( response , "\r\n" , "\n" ) ;
	G::Str::replaceAll( response , "\r" , "" ) ;

	G::StringArray response_parts ;
	response_parts.reserve( 2U ) ;
	G::Str::splitIntoFields( response , response_parts , "\n" ) ;
	size_t parts = response_parts.size() ;
	response_parts.resize( 2U ) ;

	G_LOG( "GSmtp::ExecutableVerifier: address verifier: exit code " << exit_code << " "
		<< "[" << G::Str::printable(response_parts[0]) << "] [" << G::Str::printable(response_parts[1]) << "]" ) ;

	VerifierStatus status ;
	if( ( exit_code == 0 || exit_code == 1 ) && parts >= 2 )
	{
		status.is_valid = true ;
		status.is_local = exit_code == 0 ;
		status.full_name = G::Str::printable( response_parts.at(0U) ) ;
		status.address = G::Str::printable( response_parts.at(1U) ) ;
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

		status.response = parts > 0U ?
			G::Str::printable(response_parts.at(0U)) :
			std::string("mailbox unavailable") ;

		status.reason = parts > 1U ?
			G::Str::printable(response_parts.at(1U)) :
			( "exit code " + G::Str::fromInt(exit_code) ) ;
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
