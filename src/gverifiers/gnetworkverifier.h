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
/// \file gnetworkverifier.h
///

#ifndef G_NETWORK_VERIFIER_H
#define G_NETWORK_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "grequestclient.h"
#include "gclientptr.h"
#include <string>

namespace GVerifiers
{
	class NetworkVerifier ;
}

//| \class GVerifiers::NetworkVerifier
/// A Verifier that talks to a remote address verifier over the network.
///
class GVerifiers::NetworkVerifier : public GSmtp::Verifier
{
public:
	NetworkVerifier( GNet::ExceptionSink , const GSmtp::Verifier::Config & config ,
		const std::string & server ) ;
			///< Constructor.

	~NetworkVerifier() override ;
		///< Destructor.

private: // overrides
	void verify( GSmtp::Verifier::Command , const std::string & rcpt_to_parameter ,
		const GSmtp::Verifier::Info & ) override ; // GSmtp::Verifier
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & doneSignal() override ; // GSmtp::Verifier
	void cancel() override ; // GSmtp::Verifier

public:
	NetworkVerifier( const NetworkVerifier & ) = delete ;
	NetworkVerifier( NetworkVerifier && ) = delete ;
	NetworkVerifier & operator=( const NetworkVerifier & ) = delete ;
	NetworkVerifier & operator=( NetworkVerifier && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void clientDeleted( const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<GSmtp::RequestClient> m_client_ptr ;
	std::string m_to_address ;
	GSmtp::Verifier::Command m_command ;
} ;

#endif
