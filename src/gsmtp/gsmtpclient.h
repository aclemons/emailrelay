//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gsmtpclient.h
///

#ifndef G_SMTP_CLIENT_H
#define G_SMTP_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gsecrets.h"
#include "glinebuffer.h"
#include "gclient.h"
#include "gclientprotocol.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gprocessor.h"
#include "gsocket.h"
#include "gslot.h"
#include "gtimer.h"
#include "gstrings.h"
#include "gexe.h"
#include "gexception.h"
#include <memory>
#include <iostream>

/// \namespace GSmtp
namespace GSmtp
{
	class Client ;
	class ClientProtocol ;
}

/// \class GSmtp::Client
/// A class which acts as an SMTP client, extracting
/// messages from a message store and forwarding them to
/// a remote SMTP server.
///
class GSmtp::Client : public GNet::Client , private GSmtp::ClientProtocol::Sender 
{
public:
	G_EXCEPTION( NotConnected , "not connected" ) ;

	/// A structure containing GSmtp::Client configuration parameters.
	struct Config 
	{
		G::Executable storedfile_preprocessor ;
		GNet::Address local_address ;
		ClientProtocol::Config client_protocol_config ;
		unsigned int connection_timeout ;
		Config( G::Executable , GNet::Address , ClientProtocol::Config , unsigned int ) ;
	} ;

	Client( const GNet::ResolverInfo & remote , MessageStore & store , const Secrets & secrets , Config config ) ;
		///< Constructor for sending messages from the message
		///< store. All the references are kept.
		///<
		///< All instances must be on the heap since they delete
		///< themselves after raising the done signal.
		///<
		///< The doneSignal() is used to indicate that all message 
		///< processing has finished or that the server connection has
		///< been lost.

	Client( const GNet::ResolverInfo & remote , std::auto_ptr<StoredMessage> message , 
		const Secrets & secrets , Config config ) ;
			///< Constructor for sending a single message.
			///< The references are kept.
			///<
			///< All instances must be on the heap since they delete
			///< themselves after raising the done signal.
			///<
			///< The doneSignal() is used to indicate that all
			///< message processing has finished or that the 
			///< server connection has been lost.
			///<
			///< With this constructor (designed for proxying) the 
			///< message is fail()ed if the connection to the 
			///< downstream server cannot be made.

	virtual ~Client() ;
		///< Destructor.

private:
	virtual void onConnect() ; // GNet::SimpleClient
	virtual bool onReceive( const std::string & ) ; // GNet::Client
	virtual void onDelete( const std::string & , bool ) ; // GNet::HeapClient
	virtual void onSendComplete() ; // GNet::BufferedClient
	virtual bool protocolSend( const std::string & , size_t ) ; // ClientProtocol::Sender
	void protocolDone( bool , bool , std::string ) ; // see ClientProtocol::doneSignal()
	void preprocessorStart() ;
	void preprocessorDone( bool ) ;
	static std::string crlf() ;
	bool sendNext() ;
	void start( StoredMessage & ) ;
	void messageFail( const std::string & reason ) ;
	void messageDestroy() ;

private:
	MessageStore * m_store ;
	Processor m_storedfile_preprocessor ;
	std::auto_ptr<StoredMessage> m_message ;
	MessageStore::Iterator m_iter ;
	ClientProtocol m_protocol ;
	bool m_force_message_fail ;
} ;

#endif
