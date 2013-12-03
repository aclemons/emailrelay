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
/// \file gprotocolmessageforward.h
///

#ifndef G_SMTP_PROTOCOL_MESSAGE_FORWARD_H
#define G_SMTP_PROTOCOL_MESSAGE_FORWARD_H

#include "gdef.h"
#include "gsmtp.h"
#include "gresolverinfo.h"
#include "gclientptr.h"
#include "gprotocolmessage.h"
#include "gprotocolmessagestore.h"
#include "gexecutable.h"
#include "gsecrets.h"
#include "gsmtpclient.h"
#include "gmessagestore.h"
#include "gnewmessage.h"
#include "gverifierstatus.h"
#include <string>
#include <memory>

/// \namespace GSmtp
namespace GSmtp
{
	class ProtocolMessageForward ;
}

/// \class GSmtp::ProtocolMessageForward
/// A concrete implementation of the ProtocolMessage 
/// interface which stores incoming messages in the message store 
/// and then immediately forwards them on to the downstream server.
/// 
/// The implementation delegates to an instance of the ProtocolMessageStore 
/// class (ie. its sibling class) to do the storage, and to an instance 
/// of the GSmtp::Client class to do the forwarding.
///
/// \see GSmtp::ProtocolMessageStore
///
class GSmtp::ProtocolMessageForward : public GSmtp::ProtocolMessage 
{
public:
	ProtocolMessageForward( MessageStore & store , 
		std::auto_ptr<ProtocolMessage> pm ,
		const GSmtp::Client::Config & client_config ,
		const GAuth::Secrets & client_secrets , 
		const std::string & server_address , 
		unsigned int connection_timeout ) ;
			///< Constructor. The 'store' and 'client-secrets' references
			///< are kept.

	virtual ~ProtocolMessageForward() ;
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

	virtual void process( const std::string & auth_id, const std::string & peer_socket_address ,
		const std::string & peer_socket_name , const std::string & peer_certificate ) ;
			///< Final override from GSmtp::ProtocolMessage.

protected:
	G::Signal3<bool,unsigned long,std::string> & storageDoneSignal() ;
		///< Returns the signal which is used to signal that the storage
		///< is complete. Derived classes can use this to
		///< intercept the storage-done signal emit()ed by
		///< the ProtocolMessageStore object.

	void processDone( bool , unsigned long , std::string ) ; 
		///< Called by derived classes that have intercepted
		///< the storageDoneSignal() when their own post-storage
		///< processing is complete.

private:
	void operator=( const ProtocolMessageForward & ) ; // not implemented
	void clientDone( std::string , bool ) ; // GNet::Client::doneSignal()
	void messageDone( std::string ) ; // GSmtp::Client::messageDoneSignal()
	bool forward( unsigned long , bool & , std::string * ) ;

private:
	MessageStore & m_store ;
	GNet::ResolverInfo m_client_resolver_info ;
	Client::Config m_client_config ;
	const GAuth::Secrets & m_client_secrets ;
	std::auto_ptr<ProtocolMessage> m_pm ;
	GNet::ClientPtr<GSmtp::Client> m_client ;
	unsigned long m_id ;
	unsigned int m_connection_timeout ;
	G::Signal3<bool,unsigned long,std::string> m_done_signal ;
} ;

#endif
