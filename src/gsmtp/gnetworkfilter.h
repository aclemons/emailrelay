//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnetworkfilter.h
///

#ifndef G_SMTP_NETWORK_FILTER_H
#define G_SMTP_NETWORK_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gclientptr.h"
#include "gfilestore.h"
#include "grequestclient.h"
#include "geventhandler.h"

namespace GSmtp
{
	class NetworkFilter ;
}

//| \class GSmtp::NetworkFilter
/// A Filter class that passes the name of a message file to a
/// remote network server. The response of ok/abandon/fail is
/// delivered via the base class's doneSignal().
///
class GSmtp::NetworkFilter : public Filter
{
public:
	NetworkFilter( GNet::ExceptionSink , FileStore & ,
		const std::string & server_location ,
		unsigned int connection_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	~NetworkFilter() override ;
		///< Destructor.

private: // overrides
	std::string id() const override ; // Override from from GSmtp::Filter.
	bool simple() const override ; // Override from from GSmtp::Filter.
	G::Slot::Signal<int> & doneSignal() override ; // Override from from GSmtp::Filter.
	void start( const MessageId & ) override ; // Override from from GSmtp::Filter.
	void cancel() override ; // Override from from GSmtp::Filter.
	bool abandoned() const override ; // Override from from GSmtp::Filter.
	std::string response() const override ; // Override from from GSmtp::Filter.
	std::string reason() const override ; // Override from from GSmtp::Filter.
	bool special() const override ; // Override from from GSmtp::Filter.

public:
	NetworkFilter( const NetworkFilter & ) = delete ;
	NetworkFilter( NetworkFilter && ) = delete ;
	void operator=( const NetworkFilter & ) = delete ;
	void operator=( NetworkFilter && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void clientDeleted( const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	FileStore & m_file_store ;
	G::Slot::Signal<int> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<RequestClient> m_client_ptr ;
	std::string m_text ;
} ;

#endif
