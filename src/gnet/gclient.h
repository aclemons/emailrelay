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
/// \file gclient.h
///

#ifndef G_CLIENT_H 
#define G_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gbufferedclient.h"
#include "glinebuffer.h"
#include "gtimer.h"
#include "gslot.h"

/// \namespace GNet
namespace GNet
{
	class Client ;
}

/// \class GNet::Client
/// A BufferedClient class that adds slot/signal signalling,
/// connection/response timeouts, and input line buffering.
///
/// The following pure virtual functions must be implemented by
/// derived classes: onConnect(), onReceive(), onDelete(), and
/// onSendComplete().
///
/// The intention is that derived classes use the virtual functions
/// for event notification while the slot/signal event notifications
/// are used by the containing object (in particular so that the
/// container is informed when the client object deletes itself).
///
class GNet::Client : public GNet::BufferedClient , private GNet::TimeoutHandler 
{
public:
	explicit Client( const ResolverInfo & remote_info , unsigned int connection_timeout = 0U ,
		unsigned int response_timeout = 0U , const std::string & eol = std::string("\n") ,
		const Address & local_interface = Address(0U) , bool privileged = false , 
		bool sync_dns = synchronousDnsDefault() ) ;
			///< Constructor.

	virtual ~Client() ;
		///< Destructor.

	G::Signal2<std::string,bool> & doneSignal() ;
		///< Returns a signal that indicates that client processing
		///< is complete.
		///<
		///< The first signal parameter is a failure reason, or the
		///< empty string on success. The second parameter gives some
		///< idea of whether the connection can be retried later.

	G::Signal2<std::string,std::string> & eventSignal() ;
		///< Returns a signal that indicates that something interesting
		///< has happened.
		///<
		///< The first signal parameter is one of "connecting",
		///< "failed", "connected", "sending", or "done".

	G::Signal0 & connectedSignal() ;
		///< Returns a signal that incidcates that the client
		///< has successfully connected to the server.

protected:
	virtual bool onReceive( const std::string & ) = 0 ;
		///< Called when a complete line is received from the peer.
		///< Returns false if no more lines should be delivered.

	void clearInput() ;
		///< Clears any pending input from the server.

private:
	Client( const Client& ) ; // Not implemented.
	void operator=( const Client& ) ; // Not implemented.
    virtual void onConnectImp() ; // GNet::SimpleClient
	virtual void onData( const char * , SimpleClient::size_type ) ; // GNet::SimpleClient
    virtual void onDeleteImp( const std::string & , bool ) ; // GNet::HeapClient
	virtual void onConnecting() ; // GNet::HeapClient
	virtual void onSendImp() ; // GNet::BufferedClient
	virtual void onTimeout( GNet::AbstractTimer & ) ; // GNet::TimeoutHandler

private:
	G::Signal2<std::string,bool> m_done_signal ;
	G::Signal2<std::string,std::string> m_event_signal ;
	G::Signal0 m_connected_signal ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ConcreteTimer m_connection_timer ;
	GNet::ConcreteTimer m_response_timer ;
	GNet::LineBuffer m_line_buffer ;
} ;

#endif
