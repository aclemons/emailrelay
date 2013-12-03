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
/// \file grequestclient.h
///

#ifndef G_REQUEST_CLIENT_H
#define G_REQUEST_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gclient.h"
#include "gtimer.h"
#include "gpath.h"
#include "gslot.h"
#include "gexception.h"

/// \namespace GSmtp
namespace GSmtp
{
	class RequestClient ;
}

/// \class GSmtp::RequestClient
/// A client class that interacts with a remote process
/// with a stateless line-based request/response protocol. 
///
class GSmtp::RequestClient : public GNet::Client 
{
public:
	G_EXCEPTION( ProtocolError , "protocol error" ) ;

	RequestClient( const std::string & key , const std::string & ok , const std::string & eol , 
		const GNet::ResolverInfo & host_and_service , unsigned int connect_timeout , unsigned int response_timeout ) ;
			///< Constructor. The key parameter is used in the callback
			///< signal; the (optional) ok parameter is a response
			///< string that is considered to be a success response;
			///< the eol parameter is the response end-of-line.

	void request( const std::string & ) ;
		///< Issues a request. The base class's "event" signal emitted when 
		///< processing is complete with a first signal parameter of the
		///< "key" string specified in the constructor call and a second 
		///< parameter giving the parsed response. 
		///<
		///< Every request will get a single response as long as this method 
		///< is not called re-entrantly from within the previous request's 
		///< response signal.

	bool busy() const ;
		///< Returns true after request() and before the subsequent
		///< event signal.

protected:
	virtual ~RequestClient() ;
		///< Destructor.

	virtual void onConnect() ; 
		///< Final override from GNet::SimpleClient.

	virtual bool onReceive( const std::string & ) ; 
		///< Final override from GNet::Client.

	virtual void onSendComplete() ; 
		///< Final override from GNet::BufferedClient.

	virtual void onDelete( const std::string & , bool ) ; 
		///< Final override from GNet::HeapClient.

	virtual void onDeleteImp( const std::string & , bool ) ;
		///< Final override from GNet::Client.

	virtual void onSecure( const std::string & ) ;
		///< Final override from GNet::SocketProtocolSink.

private:
	typedef GNet::Client Base ;
	RequestClient( const RequestClient & ) ; // not implemented
	void operator=( const RequestClient & ) ; // not implemented
	void onTimeout() ;
	std::string requestLine( const std::string & ) const ;
	std::string result( std::string ) const ;

private:
	std::string m_key ;
	std::string m_ok ;
	std::string m_eol ;
	std::string m_request ;
	GNet::Timer<RequestClient> m_timer ;
} ;

#endif
