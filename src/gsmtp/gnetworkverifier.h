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
/// \file gnetworkverifier.h
///

#ifndef G_SMTP_NETWORK_VERIFIER_H
#define G_SMTP_NETWORK_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "grequestclient.h"
#include "gclientptr.h"
#include <string>

namespace GSmtp
{
	class NetworkVerifier ;
}

//| \class GSmtp::NetworkVerifier
/// A Verifier that talks to a remote address verifier over the network.
///
class GSmtp::NetworkVerifier : public Verifier
{
public:
	NetworkVerifier( GNet::ExceptionSink , const std::string & server ,
		unsigned int connection_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	~NetworkVerifier() override ;
		///< Destructor.

private: // overrides
	void verify( Command , const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) override ; // Override from GSmtp::Verifier.
	G::Slot::Signal<Command,const VerifierStatus&> & doneSignal() override ; // Override from GSmtp::Verifier.
	void cancel() override ; // Override from GSmtp::Verifier.

public:
	NetworkVerifier( const NetworkVerifier & ) = delete ;
	NetworkVerifier( NetworkVerifier && ) = delete ;
	void operator=( const NetworkVerifier & ) = delete ;
	void operator=( NetworkVerifier && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void clientDeleted( const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	G::Slot::Signal<Command,const VerifierStatus&> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<RequestClient> m_client_ptr ;
	std::string m_to_address ;
	Command m_command ;
} ;

#endif
