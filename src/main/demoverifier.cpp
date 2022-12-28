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
/// \file demoverifier.cpp
///

#include "gdef.h"
#include "demoverifier.h"
#include "run.h"
#include "unit.h"
#include "gstr.h"

Main::DemoVerifier::DemoVerifier( GNet::ExceptionSink es , Main::Run & run ,
	Main::Unit & unit , const std::string & spec ) :
		m_run(run) ,
		m_unit(unit) ,
		m_timer(*this,&DemoVerifier::onTimeout,es) ,
		m_result(GSmtp::VerifierStatus::invalid({}))
{
}

Main::DemoVerifier::~DemoVerifier()
= default ;

void Main::DemoVerifier::verify( Command command , const std::string & rcpt_to_parameter ,
	const std::string & /*mail_from_parameter*/ , const G::BasicAddress & /*client_ip*/ ,
	const std::string & /*auth_mechanism*/ , const std::string & /*auth_extra*/ )
{
	// squirrel away the RCPT/VRFY enum
	m_command = command ;

	// parse the RCPT-TO parameter
	std::string user = G::Str::lower( G::Str::head( rcpt_to_parameter , "@" , false ) ) ;
	std::string domain = G::Str::lower( G::Str::tail( rcpt_to_parameter , "@" ) ) ;

	// verify as valid-local, valid-remote or invalid
	std::string this_domain = m_unit.domain() ;
	if( domain == this_domain && ( user == "postmaster" || user == "webmaster" ) )
	{
		// (note that messages to local recipients are not forwarded)
		m_result = GSmtp::VerifierStatus::local( rcpt_to_parameter ,
			"Postmaster" , "<postmaster@"+this_domain+">" ) ;
	}
	else if( user == "alice" )
	{
		m_result = GSmtp::VerifierStatus::remote( rcpt_to_parameter ) ;
	}
	else
	{
		m_result = GSmtp::VerifierStatus::invalid( rcpt_to_parameter , false ,
			"rejected" , "not postmaster or alice" ) ;
	}

	// asynchronous completion via a timer
	m_timer.startTimer(1U) ;
}

Main::DemoVerifier::Signal & Main::DemoVerifier::doneSignal()
{
	return m_done_signal ;
}

void Main::DemoVerifier::cancel()
{
	m_timer.cancelTimer() ;
}

void Main::DemoVerifier::onTimeout()
{
	m_done_signal.emit( m_command , m_result ) ;
}

