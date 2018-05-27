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
///
/// \file gnetworkverifier.h
///

#ifndef G_SMTP_NETWORK_VERIFIER_H
#define G_SMTP_NETWORK_VERIFIER_H

#include "gdef.h"
#include "gsmtp.h"
#include "gverifier.h"
#include "grequestclient.h"
#include "gclientptr.h"
#include <string>

namespace GSmtp
{
	class NetworkVerifier ;
}

/// \class GSmtp::NetworkVerifier
/// A Verifier that talks to a remote verifier over the network.
///
class GSmtp::NetworkVerifier : public Verifier
{
public:
	NetworkVerifier( GNet::ExceptionHandler & , const std::string & server ,
		unsigned int connection_timeout , unsigned int response_timeout , bool compatible ) ;
			///< Constructor.

	virtual ~NetworkVerifier() ;
		///< Destructor.

	virtual void verify( const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) override ;
			///< Override from GSmtp::Verifier.

	virtual G::Slot::Signal2<std::string,VerifierStatus> & doneSignal() override ;
		///< Override from GSmtp::Verifier.

	virtual void cancel() override ;
		///< Override from GSmtp::Verifier.

private:
	NetworkVerifier( const NetworkVerifier & ) ;
	void operator=( const NetworkVerifier & ) ;
	void clientEvent( std::string , std::string ) ;

private:
	GNet::ExceptionHandler & m_exception_handler ;
	G::Slot::Signal2<std::string,VerifierStatus> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	bool m_lazy ;
	bool m_compatible ;
	GNet::ClientPtr<RequestClient> m_client ;
	std::string m_to_address ;
} ;

#endif
