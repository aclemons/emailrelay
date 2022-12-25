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

Main::DemoVerifier::DemoVerifier( GNet::ExceptionSink es , Main::Run & , Main::Unit & , const std::string & spec ) :
	m_timer(*this,&DemoVerifier::onTimeout,es)
{
}

Main::DemoVerifier::~DemoVerifier()
= default ;

void Main::DemoVerifier::verify( Command command , const std::string & rcpt_to_parameter ,
	const std::string & /*mail_from_parameter*/ , const G::BasicAddress & /*client_ip*/ ,
	const std::string & /*auth_mechanism*/ , const std::string & /*auth_extra*/ )
{
	m_command = command ;
	m_address = rcpt_to_parameter ;
	m_timer.startTimer(1U) ;
}

G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & Main::DemoVerifier::doneSignal()
{
	return m_done_signal ;
}

void Main::DemoVerifier::cancel()
{
	m_timer.cancelTimer() ;
}

void Main::DemoVerifier::onTimeout()
{
	m_done_signal.emit( m_command , GSmtp::VerifierStatus::remote(m_address) ) ;
}

