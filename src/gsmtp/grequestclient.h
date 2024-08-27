//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file grequestclient.h
///

#ifndef G_REQUEST_CLIENT_H
#define G_REQUEST_CLIENT_H

#include "gdef.h"
#include "gclient.h"
#include "gtimer.h"
#include "gpath.h"
#include "gslot.h"
#include "gexception.h"

namespace GSmtp
{
	class RequestClient ;
}

//| \class GSmtp::RequestClient
/// A network client class that interacts with a remote server using a
/// stateless line-based request/response protocol.
///
/// Line buffering uses newline as end-of-line, and trailing carriage-returns
/// are trimmed from the input.
///
/// The received network responses are delivered via the GNet::Client
/// class's event signal (GNet::Client::eventSignal()).
///
class GSmtp::RequestClient : public GNet::Client
{
public:
	G_EXCEPTION( ProtocolError , tx("protocol error") )

	RequestClient( GNet::EventState , const std::string & key , const std::string & ok ,
		const GNet::Location & host_and_service , unsigned int connection_timeout ,
		unsigned int response_timeout , unsigned int idle_timeout ) ;
			///< Constructor.  The 'key' parameter is used in the callback
			///< signal. The 'ok' parameter is a response string that is
			///< converted to the empty string.

	void request( const std::string & ) ;
		///< Issues a request. A newline is added to the request string,
		///< so append a carriage-return if required.
		///<
		///< If not currently connected then the request is queued up until
		///< the connection is made.
		///<
		///< The base class's "event" signal will be emitted when processing
		///< is complete. In this case the first signal parameter will be the
		///< "key" string specified in the constructor call and the second
		///< will be the parsed response.
		///<
		///< See also GNet::Client::eventSignal().
		///<
		///< Every request will get a single response as long as this method
		///< is not called re-entrantly from within the previous request's
		///< response signal.

	bool busy() const ;
		///< Returns true after request() and before the subsequent
		///< event signal.

private: // overrides
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // Override from GNet::Client.
	void onSendComplete() override ; // Override from GNet::BufferedClient.
	void onDelete( const std::string & ) override ; // Override from GNet::Client.
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocolSink.
	void onConnect() override ; // Override from GNet::SimpleClient.

public:
	~RequestClient() override = default ;
	RequestClient( const RequestClient & ) = delete ;
	RequestClient( RequestClient && ) = delete ;
	RequestClient & operator=( const RequestClient & ) = delete ;
	RequestClient & operator=( RequestClient && ) = delete ;

private:
	void onTimeout() ;
	std::string requestLine( const std::string & ) const ;
	std::string result( std::string ) const ;

private:
	std::string m_eol ;
	std::string m_key ;
	std::string m_ok ;
	std::string m_request ;
	GNet::Timer<RequestClient> m_timer ;
} ;

#endif
