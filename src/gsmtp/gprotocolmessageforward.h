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
/// \file gprotocolmessageforward.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_FORWARD_H
#define G_SMTP_PROTOCOL_MESSAGE_FORWARD_H

#include "gdef.h"
#include "glocation.h"
#include "gclientptr.h"
#include "gprotocolmessage.h"
#include "gprotocolmessagestore.h"
#include "gsmtpforward.h"
#include "gsaslclientsecrets.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gfilterfactorybase.h"
#include "gverifierstatus.h"
#include "gcall.h"
#include <string>
#include <memory>

namespace GSmtp
{
	class ProtocolMessageForward ;
}

//| \class GSmtp::ProtocolMessageForward
/// A concrete implementation of the ProtocolMessage interface that stores
/// incoming messages in the message store and then immediately forwards
/// them on to the downstream server.
///
/// The implementation delegates to an instance of the ProtocolMessageStore
/// class (ie. its sibling class) to do the storage, and to an instance
/// of the GSmtp::Forward class to do the forwarding.
///
/// \see GSmtp::ProtocolMessageStore
///
class GSmtp::ProtocolMessageForward : public ProtocolMessage
{
public:
	ProtocolMessageForward( GNet::ExceptionSink ,
		GStore::MessageStore & store , FilterFactoryBase & ,
		std::unique_ptr<ProtocolMessage> pm ,
		const GSmtp::Client::Config & client_config ,
		const GAuth::SaslClientSecrets & client_secrets ,
		const std::string & forward_to , int forward_to_family ) ;
			///< Constructor.

	~ProtocolMessageForward() override ;
		///< Destructor.

private: // overrides
	ProtocolMessage::ProcessedSignal & processedSignal() noexcept override ; // GSmtp::ProtocolMessage
	void reset() override ; // GSmtp::ProtocolMessage
	void clear() override ; // GSmtp::ProtocolMessage
	GStore::MessageId setFrom( const std::string & from_user , const FromInfo & ) override ; // GSmtp::ProtocolMessage
	bool addTo( const ToInfo & ) override ; // GSmtp::ProtocolMessage
	void addReceived( const std::string & ) override ; // GSmtp::ProtocolMessage
	GStore::NewMessage::Status addContent( const char * , std::size_t ) override ; // GSmtp::ProtocolMessage
	std::size_t contentSize() const override ; // GSmtp::ProtocolMessage
	std::string from() const override ; // GSmtp::ProtocolMessage
	ProtocolMessage::FromInfo fromInfo() const override ; // GSmtp::ProtocolMessage
	std::string bodyType() const override ; // GSmtp::ProtocolMessage
	void process( const std::string & auth_id, const std::string & peer_socket_address ,
		const std::string & peer_certificate ) override ; // GSmtp::ProtocolMessage

public:
	ProtocolMessageForward( const ProtocolMessageForward & ) = delete ;
	ProtocolMessageForward( ProtocolMessageForward && ) = delete ;
	ProtocolMessageForward & operator=( const ProtocolMessageForward & ) = delete ;
	ProtocolMessageForward & operator=( ProtocolMessageForward && ) = delete ;

private:
	void clientDone( const std::string & ) ; // GNet::Client::doneSignal()
	void messageDone( const Client::MessageDoneInfo & ) ; // GSmtp::Client::messageDoneSignal()
	void protocolMessageProcessed( const ProtocolMessage::ProcessedInfo & ) ; // GSmtp::ProtocolMessage::processedSignal()
	std::string forward( const GStore::MessageId & , bool & ) ;

private:
	GNet::ExceptionSink m_es ;
	GStore::MessageStore & m_store ;
	FilterFactoryBase & m_ff ;
	G::CallStack m_call_stack ;
	GNet::Location m_client_location ;
	Client::Config m_client_config ;
	const GAuth::SaslClientSecrets & m_client_secrets ;
	std::unique_ptr<ProtocolMessage> m_pm ;
	GNet::ClientPtr<GSmtp::Forward> m_client_ptr ;
	GStore::MessageId m_id ;
	ProtocolMessage::ProcessedSignal m_processed_signal ;
} ;

#endif
