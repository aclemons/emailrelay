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
/// \file gexecutableverifier.cpp
///

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

GVerifiers::ExecutableVerifier::ExecutableVerifier( GNet::ExceptionSink es , const G::Path & path , unsigned int timeout ) :
	m_timer(*this,&ExecutableVerifier::onTimeout,es) ,
	m_command(GSmtp::Verifier::Command::VRFY) ,
	m_path(path) ,
	m_timeout(timeout) ,
	m_task(*this,es,"<<verifier exec error: __strerror__>>",G::Root::nobody())
{
}

void GVerifiers::ExecutableVerifier::verify( GSmtp::Verifier::Command command , const std::string & to_address ,
	const GSmtp::Verifier::Info & info )
{
	m_command = command ;
	G_DEBUG( "GVerifiers::ExecutableVerifier::verify: to=[" << to_address << "]" ) ;

	G::ExecutableCommand commandline( m_path.str() , G::StringArray() ) ;
	commandline.add( to_address ) ;
	commandline.add( info.mail_from_parameter ) ;
	commandline.add( info.client_ip.displayString() ) ;
	commandline.add( info.domain ) ;
	commandline.add( G::Str::lower(info.auth_mechanism) ) ;
	commandline.add( info.auth_extra ) ;

	G_LOG( "GVerifiers::ExecutableVerifier: address verifier: executing " << commandline.displayString() ) ;
	m_to_address = to_address ;
	m_task.start( commandline ) ;
	if( m_timeout )
		m_timer.startTimer( m_timeout ) ;
}

void GVerifiers::ExecutableVerifier::onTimeout()
{
	m_task.stop() ;

	auto result = GSmtp::VerifierStatus::invalid( m_to_address , true , "timeout" , "timeout" ) ;

	m_done_signal.emit( m_command , result ) ;
}

void GVerifiers::ExecutableVerifier::onTaskDone( int exit_code , const std::string & result_in )
{
	m_timer.cancelTimer() ;
	auto status = GSmtp::VerifierStatus::invalid( m_to_address ) ;
	if( exit_code == 127 && G::Str::headMatch(result_in,"<<verifier exec error") )
	{
		G_WARNING( "GVerifiers::ExecutableVerifier: address verifier: exec error" ) ;
		status = GSmtp::VerifierStatus::invalid( m_to_address , false , "error" , "exec error" ) ;
	}
	else
	{
		std::string result( result_in ) ;
		G::Str::trimRight( result , {" \n\t",3U} ) ;
		G::Str::replaceAll( result , "\r\n" , "\n" ) ;
		G::Str::replaceAll( result , "\r" , "" ) ;

		G::StringArray result_parts ;
		result_parts.reserve( 2U ) ;
		G::Str::splitIntoFields( result , result_parts , '\n' ) ;
		std::size_t parts = result_parts.size() ;
		result_parts.resize( 2U ) ;

		G_LOG( "GVerifiers::ExecutableVerifier: address verifier: exit code " << exit_code << ": "
			<< "[" << G::Str::printable(result_parts[0]) << "] [" << G::Str::printable(result_parts[1]) << "]" ) ;

		if( exit_code == 0 && parts >= 2 )
		{
			std::string full_name = G::Str::printable( result_parts.at(0U) ) ;
			std::string mbox = G::Str::printable( result_parts.at(1U) ) ;
			status = GSmtp::VerifierStatus::local( m_to_address , full_name , mbox ) ;
		}
		else if( exit_code == 1 && parts >= 2 )
		{
			std::string address = G::Str::printable( result_parts.at(1U) ) ;
			status = GSmtp::VerifierStatus::remote( m_to_address , address ) ;
		}
		else if( exit_code == 100 )
		{
			status.abort = true ;
		}
		else
		{
			bool temporary = exit_code == 3 ;

			std::string response = parts > 0U ?
				G::Str::printable(result_parts.at(0U)) :
				std::string("mailbox unavailable") ;

			std::string reason = parts > 1U ?
				G::Str::printable(result_parts.at(1U)) :
				( "exit code " + G::Str::fromInt(exit_code) ) ;

			status = GSmtp::VerifierStatus::invalid( m_to_address ,
				temporary , response , reason ) ;
		}
	}
	doneSignal().emit( m_command , status ) ;
}

G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & GVerifiers::ExecutableVerifier::doneSignal()
{
	return m_done_signal ;
}

void GVerifiers::ExecutableVerifier::cancel()
{
}

