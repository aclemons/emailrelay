//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gprotocolmessageforward.h
//

#ifndef G_SMTP_PROTOCOL_MESSAGE_FORWARD_H
#define G_SMTP_PROTOCOL_MESSAGE_FORWARD_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprotocolmessage.h"
#include "gprotocolmessagestore.h"
#include "gsecrets.h"
#include "gsmtpclient.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include <string>
#include <memory>

namespace GSmtp
{
	class ProtocolMessageForward ;
}

// Class: GSmtp::ProtocolMessageForward
// Description: A concrete implementation of the ProtocolMessage 
// interface which stores incoming messages in the message store 
// and then immediately forwards them on to the downstream server.
// 
// The implementation delegates to an instance of the ProtocolMessageStore 
// class (ie. its sibling class) to do the storage, and to an instance 
// of the Client class to do the forwarding.
//
// See also: GSmtp::ProtocolMessageStore
//
class GSmtp::ProtocolMessageForward : public GSmtp::ProtocolMessage 
{
public:
	ProtocolMessageForward( MessageStore & store , const Secrets & client_secrets , 
		const std::string & server_address , unsigned int response_timeout ,
		unsigned int connection_timeout ) ;
			// Constructor. The 'store' and 'client-secrets' references
			// are kept.

	virtual ~ProtocolMessageForward() ;
		// Destructor.

	virtual G::Signal3<bool,unsigned long,std::string> & doneSignal() ;
		// See ProtocolMessage.

	virtual G::Signal3<bool,bool,std::string> & preparedSignal() ;
		// See ProtocolMessage.

	virtual void clear() ;
		// See ProtocolMessage.

	virtual bool setFrom( const std::string & from_user ) ;
		// See ProtocolMessage.

	virtual bool prepare() ;
		// See ProtocolMessage.

	virtual bool addTo( const std::string & to_user , Verifier::Status to_status ) ;
		// See ProtocolMessage.

	virtual void addReceived( const std::string & ) ;
		// See ProtocolMessage.

	virtual void addText( const std::string & ) ;
		// See ProtocolMessage.

	virtual std::string from() const ;
		// See ProtocolMessage.

	virtual void process( const std::string & auth_id , const std::string & client_ip ) ;
		// See ProtocolMessage.

protected:
	G::Signal3<bool,unsigned long,std::string> & storageDoneSignal() ;
		// Returns the signal which is used to signal that the storage
		// is complete.

	void processDone( bool , unsigned long , std::string ) ; 
		// ...

private:
	void operator=( const ProtocolMessageForward & ) ; // not implemented
	void clientDone( std::string ) ; // Client::doneSignal()
	bool forward( unsigned long , bool & , std::string * ) ;

private:
	MessageStore & m_store ;
	const Secrets & m_client_secrets ;
	ProtocolMessageStore m_pm ;
	std::string m_server ;
	std::auto_ptr<Client> m_client ;
	unsigned long m_id ;
	unsigned int m_response_timeout ;
	unsigned int m_connection_timeout ;
	G::Signal3<bool,unsigned long,std::string> m_done_signal ;
	G::Signal3<bool,bool,std::string> m_prepared_signal ;
} ;

#endif
