//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsocketprotocol.h
///

#ifndef G_SOCKET_PROTOCOL_H
#define G_SOCKET_PROTOCOL_H

#include "gdef.h"
#include "gnet.h"
#include "gsocket.h"
#include "gexception.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class SocketProtocol ;
	class SocketProtocolImp ;
	class SocketProtocolSink ;
}

/// \class GNet::SocketProtocol
/// A class for doing read() and write() on
/// a connected socket and installing and removing event 
/// handlers as appropriate. This abstraction allows 
/// for using SSL.
///
class GNet::SocketProtocol 
{
public:
	typedef SocketProtocolSink Sink ;
	G_EXCEPTION_CLASS( ReadError , "read error: disconnected" ) ;
	G_EXCEPTION( SendError , "peer disconnected" ) ;

	SocketProtocol( EventHandler & , Sink & , StreamSocket & ) ;
		///< Constructor. The references are kept.

	~SocketProtocol() ;
		///< Destructor.

	void readEvent() ;
		///< Called on receipt of a read event. Delivers
		///< data via the sink interface. Throws ReadError 
		///< on error.

	bool writeEvent() ;
		///< Called on receipt of a write event. Sends
		///< more pending data down the connection.
		///< Returns true if all pending data was
		///< sent. Throws SendError on error.

	bool send( const std::string & data , std::string::size_type offset = 0U ) ;
		///< Sends data. Returns false if flow control asserted.
		///< Throws SendError on error.

	static bool sslCapable() ;
		///< Returns true if the implementation supports SSL.

	void sslConnect() ;
		///< Initiates the SSL protocol.

	void sslAccept() ;
		///< Accepts the SSL protocol.

	bool sslEnabled() const ;
		///< Returns true if SSL is active.

private:
	SocketProtocol( const SocketProtocol & ) ;
	void operator=( const SocketProtocol & ) ;

private:
	SocketProtocolImp * m_imp ;
} ;

/// \class GNet::SocketProtocolSink
/// An interface used by GNet::SocketProtocol
/// to deliver data from a socket.
///
class GNet::SocketProtocolSink 
{
public:
	virtual ~SocketProtocolSink() ;
		///< Destructor.

protected:
	friend class SocketProtocolImp ;

	virtual void onData( const char * , std::string::size_type ) = 0 ;
		///< Called when data is read from the socket.

	virtual void onSecure() = 0 ;
		///< Called once the secure socket protocol has
		///< been successfully negotiated.
} ;

#endif
