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
/// \file gspamclient.h
///

#ifndef G_SMTP_SPAM_CLIENT_H
#define G_SMTP_SPAM_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gclient.h"
#include "gtimer.h"
#include "gpath.h"
#include "gslot.h"
#include "gexception.h"
#include "gmemory.h"
#include <fstream>
#include <vector>

/// \namespace GSmtp
namespace GSmtp
{
	class SpamClient ;
}

/// \class GSmtp::SpamClient
/// A client class that interacts with a remote process
/// using a protocol somewhat similar to the spamassassin spamc/spamd
/// protocol.
///
class GSmtp::SpamClient : public GNet::Client 
{
public:
	G_EXCEPTION( ProtocolError , "protocol error" ) ;

	SpamClient( const GNet::ResolverInfo & host_and_service , 
		unsigned int connect_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	void request( const std::string & ) ;
		///< Issues a request. The base class's "event" signal emitted when 
		///< processing is complete with a first signal parameter of 
		///< "spam" and a second parameter giving the parsed response. 
		///<
		///< Every request will get a single response as long as this method 
		///< is not called re-entrantly from within the previous request's 
		///< response signal.

	bool busy() const ;
		///< Returns true after request() and before the subsequent
		///< event signal.

protected:
	virtual ~SpamClient() ;
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
		///< Final override from GNet::SocketProtocol.

private:
	typedef GNet::Client Base ;
	SpamClient( const SpamClient & ) ; // not implemented
	void operator=( const SpamClient & ) ; // not implemented
	void onTimeout() ;
	void sendContent() ;
	std::string headerResult() const ;
	bool nextContentLine( std::string & ) ;
	bool haveCompleteHeader() const ;
	std::string headerLine( const std::string & , const std::string & = std::string() ) const ;
	unsigned long headerBodyLength() const ;
	void addHeader( const std::string & ) ;
	void addBody( const std::string & ) ;
	std::string part( const std::string & , unsigned int , const std::string & = std::string() ) const ;
	void turnRound() ;

private:
	typedef std::vector<std::string> StringArray ;
	std::string m_path ;
	std::auto_ptr<std::ifstream> m_in ;
	unsigned long m_in_size ;
	unsigned long m_in_lines ;
	std::auto_ptr<std::ofstream> m_out ;
	unsigned long m_out_size ;
	unsigned long m_out_lines ;
	unsigned long m_n ;
	unsigned int m_header_out_index ;
	StringArray m_header_out ;
	StringArray m_header_in ;
	GNet::Timer<SpamClient> m_timer ;
} ;

#endif
