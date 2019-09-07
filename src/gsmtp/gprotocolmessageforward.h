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
///
/// \file gprotocolmessageforward.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_FORWARD__H
#define G_SMTP_PROTOCOL_MESSAGE_FORWARD__H

#include "gdef.h"
#include "glocation.h"
#include "gclientptr.h"
#include "gprotocolmessage.h"
#include "gprotocolmessagestore.h"
#include "gsecrets.h"
#include "gsmtpclient.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gverifierstatus.h"
#include "gcall.h"
#include <string>
#include <memory>

namespace GSmtp
{
	class ProtocolMessageForward ;
}

/// \class GSmtp::ProtocolMessageForward
/// A concrete implementation of the ProtocolMessage interface that stores
/// incoming messages in the message store and then immediately forwards
/// them on to the downstream server.
///
/// The implementation delegates to an instance of the ProtocolMessageStore
/// class (ie. its sibling class) to do the storage, and to an instance
/// of the GSmtp::Client class to do the forwarding.
///
/// \see GSmtp::ProtocolMessageStore
///
class GSmtp::ProtocolMessageForward : public ProtocolMessage
{
public:
	ProtocolMessageForward( GNet::ExceptionSink , MessageStore & store ,
		unique_ptr<ProtocolMessage> pm ,
		const GSmtp::Client::Config & client_config ,
		const GAuth::Secrets & client_secrets ,
		const std::string & remote_server_address ) ;
			///< Constructor. The 'store' and 'client-secrets' references
			///< are kept.

	virtual ~ProtocolMessageForward() ;
		///< Destructor.

protected:
	G::Slot::Signal4<bool,unsigned long,std::string,std::string> & storageDoneSignal() ;
		///< Returns the signal which is used to signal that the storage
		///< is complete. Derived classes can use this to
		///< intercept the storage-done signal emit()ed by
		///< the ProtocolMessageStore object.

	void processDone( bool , unsigned long , std::string , std::string ) ;
		///< Called by derived classes that have intercepted
		///< the storageDoneSignal() when their own post-storage
		///< processing is complete.

private: // overrides
	virtual G::Slot::Signal4<bool,unsigned long,std::string,std::string> & doneSignal() override ; // Override from GSmtp::ProtocolMessage.
	virtual void reset() override ; // Override from GSmtp::ProtocolMessage.
	virtual void clear() override ; // Override from GSmtp::ProtocolMessage.
	virtual bool setFrom( const std::string & from_user , const std::string & ) override ; // Override from GSmtp::ProtocolMessage.
	virtual bool addTo( const std::string & to_user , VerifierStatus to_status ) override ; // Override from GSmtp::ProtocolMessage.
	virtual void addReceived( const std::string & ) override ; // Override from GSmtp::ProtocolMessage.
	virtual bool addText( const char * , size_t ) override ; // Override from GSmtp::ProtocolMessage.
	virtual std::string from() const override ; // Override from GSmtp::ProtocolMessage.
	virtual void process( const std::string & auth_id, const std::string & peer_socket_address ,
		const std::string & peer_certificate ) override ; // Override from GSmtp::ProtocolMessage.

private:
	void operator=( const ProtocolMessageForward & ) g__eq_delete ;
	void clientDone( std::string ) ; // GNet::Client::doneSignal()
	void messageDone( std::string ) ; // GSmtp::Client::messageDoneSignal()
	std::string forward( unsigned long , bool & ) ;

private:
	GNet::ExceptionSink m_es ;
	MessageStore & m_store ;
	G::CallStack m_call_stack ;
	GNet::Location m_client_location ;
	Client::Config m_client_config ;
	const GAuth::Secrets & m_client_secrets ;
	unique_ptr<ProtocolMessage> m_pm ;
	GNet::ClientPtr<GSmtp::Client> m_client_ptr ;
	unsigned long m_id ;
	G::Slot::Signal4<bool,unsigned long,std::string,std::string> m_done_signal ;
} ;

#endif
