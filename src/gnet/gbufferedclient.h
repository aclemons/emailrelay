//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gbufferedclient.h
///

#ifndef G_BUFFERED_CLIENT_H 
#define G_BUFFERED_CLIENT_H 

#include "gdef.h"
#include "gnet.h"
#include "gheapclient.h"
#include "gexception.h"
#include "gsender.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class BufferedClient ;
}

/// \class GNet::BufferedClient
/// A HeapClient class that does buffered sending with flow control.
///
class GNet::BufferedClient : public GNet::HeapClient 
{
public:
	G_EXCEPTION( SendError , "peer disconnected" ) ;

	explicit BufferedClient( const ResolverInfo & remote_info , const Address & local_interface = Address(0U) , 
		bool privileged = false , bool sync_dns = synchronousDnsDefault() ) ;
			///< Constructor.

	bool send( const std::string & data , std::string::size_type offset = 0U ) ;
		///< Sends data starting at the given offset.
		///< Returns true if all the data is sent.
		///<
		///< If flow control is asserted then a write-event 
		///< handler is installed and false is returned. 
		///< The onSendComplete() callback will be called
		///< when the data has been fully sent.
		///<
		///< Throws on error, eg. if disconnected.

protected:
	virtual ~BufferedClient() ;
		///< Destructor.

	virtual void onSendComplete() = 0 ;
		///< Called when all residual data has been sent.

	virtual void onSendImp() ;
		///< Called just before send() returns. The default 
		///< implementation does nothing. Overridable. 
		///< Overrides typically start a response timer.

	virtual void onWriteable() ;
		///< Final override from SimpleClient.

private:
	void logFlowControlReleased() const ;
	void logFlowControlAsserted() const ;

private:
	BufferedClient( const BufferedClient& ) ; // not implemented
	void operator=( const BufferedClient& ) ; // not implemented

private:
	Sender m_sender ;
} ;

#endif
