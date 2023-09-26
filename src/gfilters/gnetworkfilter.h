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
/// \file gnetworkfilter.h
///

#ifndef G_NETWORK_FILTER_H
#define G_NETWORK_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gclientptr.h"
#include "gfilestore.h"
#include "grequestclient.h"
#include "geventhandler.h"
#include "goptional.h"
#include <utility>

namespace GFilters
{
	class NetworkFilter ;
}

//| \class GFilters::NetworkFilter
/// A Filter class that passes the name of a message file to a
/// remote network server. The response of ok/abandon/fail is
/// delivered via the base class's doneSignal().
///
class GFilters::NetworkFilter : public GSmtp::Filter , private GNet::ExceptionHandler
{
public:
	NetworkFilter( GNet::ExceptionSink , GStore::FileStore & , Filter::Type ,
		const Filter::Config & , const std::string & server_location ) ;
			///< Constructor.

	~NetworkFilter() override ;
		///< Destructor.

private: // overrides
	std::string id() const override ; // GSmtp::Filter
	bool quiet() const override ; // GSmtp::Filter
	G::Slot::Signal<int> & doneSignal() noexcept override ; // GSmtp::Filter
	void start( const GStore::MessageId & ) override ; // GSmtp::Filter
	void cancel() override ; // GSmtp::Filter
	Result result() const override ; // GSmtp::Filter
	std::string response() const override ; // GSmtp::Filter
	int responseCode() const override ; // GSmtp::Filter
	std::string reason() const override ; // GSmtp::Filter
	bool special() const override ; // GSmtp::Filter
	void onException( GNet::ExceptionSource * , std::exception & , bool ) override ; // GNet::ExceptionHandler

public:
	NetworkFilter( const NetworkFilter & ) = delete ;
	NetworkFilter( NetworkFilter && ) = delete ;
	NetworkFilter & operator=( const NetworkFilter & ) = delete ;
	NetworkFilter & operator=( NetworkFilter && ) = delete ;

private:
	void clientEvent( const std::string & , const std::string & , const std::string & ) ;
	void sendResult( const std::string & ) ;
	void onTimeout() ;
	std::pair<std::string,int> responsePair() const ;

private:
	GNet::ExceptionSink m_es ;
	GStore::FileStore & m_file_store ;
	GNet::ClientPtr<GSmtp::RequestClient> m_client_ptr ;
	GNet::Timer<NetworkFilter> m_timer ;
	G::Slot::Signal<int> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	G::optional<std::string> m_text ;
	Result m_result ;
} ;

#endif
