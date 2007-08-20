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

/// \namespace GSmtp
namespace GSmtp
{
	class SpamClient ;
}

/// \class GSmtp::SpamClient
/// A client class that interacts with a remote process
/// with a spamd protocol. 
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

private:
	typedef GNet::Client Base ;
	SpamClient( const SpamClient & ) ; // not implemented
	void operator=( const SpamClient & ) ; // not implemented
	virtual void onConnect() ; // GNet::SimpleClient
	virtual bool onReceive( const std::string & ) ; // GNet::Client
	virtual void onSendComplete() ; // GNet::BufferedClient
	virtual void onDelete( const std::string & , bool ) ; // GNet::HeapClient
	virtual void onDeleteImp( const std::string & , bool ) ; // GNet::Client
	void onTimeout() ;
	void sendContent() ;
	std::string result() const ;

private:
	std::string m_path ;
	std::auto_ptr<std::ifstream> m_in ;
	std::auto_ptr<std::ofstream> m_out ;
	GNet::Timer<SpamClient> m_timer ;
} ;

#endif
