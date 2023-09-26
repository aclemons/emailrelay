//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gclient.h"
#include "gtimer.h"
#include "gpath.h"
#include "gslot.h"
#include "gexception.h"
#include "gexceptionsink.h"
#include <fstream>
#include <vector>

namespace GSmtp
{
	class SpamClient ;
}

//| \class GSmtp::SpamClient
/// A client class that interacts with a remote process using a
/// protocol somewhat similar to the spamassassin spamc/spamd
/// protocol. The interface is similar to GSmtp::RequestClient
/// but it is single-use: only one request() can be made per
/// object.
///
class GSmtp::SpamClient : public GNet::Client
{
public:
	G_EXCEPTION( Error , tx("spam client error") ) ;

	SpamClient( GNet::ExceptionSink , const GNet::Location & host_and_service ,
		bool read_only , unsigned int connection_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	void request( const std::string & file_path ) ;
		///< Starts sending a request that comprises a few http-like header
		///< lines followed by the contents of the given file. The response
		///< is spooled into a temporary file and then committed back to the
		///< same file.
		///<
		///< The base class's "event" signal will be emitted when processing
		///< is complete. In this case the first signal parameter will "spam"
		///< and the second will be the parsed response.
		///<
		///< See also GNet::Client::eventSignal().

	bool busy() const ;
		///< Returns true after request() and before the subsequent
		///< event signal.

	static void username( const std::string & ) ;
		///< Sets the username used in the network protocol.

private: // overrides
	void onConnect() override ; // Override from GNet::SimpleClient.
	bool onReceive( const char * , std::size_t , std::size_t , std::size_t , char ) override ; // Override from GNet::Client.
	void onSendComplete() override ; // Override from GNet::SimpleClient.
	void onSecure( const std::string & , const std::string & , const std::string & ) override ; // Override from GNet::SocketProtocolSink.
	void onDelete( const std::string & ) override ; // Override from GNet::HeapClient.

public:
	~SpamClient() override = default ;
	SpamClient( const SpamClient & ) = delete ;
	SpamClient( SpamClient && ) = delete ;
	SpamClient & operator=( const SpamClient & ) = delete ;
	SpamClient & operator=( SpamClient && ) = delete ;

private:
	void onTimeout() ;
	void start() ;

private:
	struct Request
	{
		explicit Request( Client & ) ;
		void send( const std::string & path , const std::string & username ) ;
		bool sendMore() ;
		Client * m_client ;
		std::ifstream m_stream ;
		std::string m_size ;
		std::vector<char> m_buffer ;
	} ;
	struct Response
	{
		explicit Response( bool read_only ) ;
		~Response() ;
		Response( const Response & ) = delete ;
		Response( Response && ) = delete ;
		Response & operator=( const Response & ) = delete ;
		Response & operator=( Response && ) = delete ;
		void add( const std::string & , const std::string & ) ;
		bool ok( const std::string & ) const ;
		bool complete() const ;
		std::string result() const ;
		bool m_read_only ;
		int m_state ;
		std::string m_path_tmp ;
		std::string m_path_final ;
		std::ofstream m_stream ;
		std::size_t m_content_length ;
		std::size_t m_size ;
		std::string m_result ;
	} ;

private:
	std::string m_path ;
	bool m_busy ;
	GNet::Timer<SpamClient> m_timer ;
	Request m_request ;
	Response m_response ;
	static std::string m_username ;
} ;

#endif
