//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gprotocolmessagestore.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_STORE_H
#define G_SMTP_PROTOCOL_MESSAGE_STORE_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gprocessor.h"
#include "gslot.h"
#include <string>
#include <memory>

/// \namespace GSmtp
namespace GSmtp
{
	class ProtocolMessageStore ;
}

/// \class GSmtp::ProtocolMessageStore
/// A concrete implementation of the ProtocolMessage interface 
/// that stores incoming messages in the message store.
/// \see GSmtp::ProtocolMessageForward
///
class GSmtp::ProtocolMessageStore : public GSmtp::ProtocolMessage 
{
public:
	ProtocolMessageStore( MessageStore & store , std::auto_ptr<Processor> ) ;
		///< Constructor.

	virtual ~ProtocolMessageStore() ;
		///< Destructor.

	virtual G::Signal3<bool,unsigned long,std::string> & doneSignal() ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual void reset() ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual void clear() ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual bool setFrom( const std::string & from_user ) ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual bool addTo( const std::string & to_user , VerifierStatus to_status ) ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual void addReceived( const std::string & ) ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual bool addText( const std::string & ) ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual std::string from() const ;
		///< Final override from GSmtp::ProtocolMessage.

	virtual void process( const std::string & auth_id , const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) ;
			///< Final override from GSmtp::ProtocolMessage.

private:
	void operator=( const ProtocolMessageStore & ) ; // not implemented
	void preprocessorDone( bool ) ;

private:
	MessageStore & m_store ;
	std::auto_ptr<Processor> m_processor ;
	std::auto_ptr<NewMessage> m_msg ;
	std::string m_from ;
	G::Signal3<bool,unsigned long,std::string> m_done_signal ;
} ;

#endif

