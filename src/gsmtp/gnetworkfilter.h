//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SMTP_NETWORK_FILTER__H
#define G_SMTP_NETWORK_FILTER__H

#include "gdef.h"
#include "gfilter.h"
#include "gclientptr.h"
#include "grequestclient.h"
#include "geventhandler.h"

namespace GSmtp
{
	class NetworkFilter ;
}

/// \class GSmtp::NetworkFilter
/// A Filter class that passes the name of a message file to a
/// remote network server. The response of ok/abandon/fail is
/// delivered via the base class's doneSignal().
///
class GSmtp::NetworkFilter : public Filter
{
public:
	NetworkFilter( GNet::ExceptionSink , const std::string & server_location ,
		unsigned int connection_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	virtual ~NetworkFilter() ;
		///< Destructor.

private: // overrides
	virtual std::string id() const override ; // Override from from GSmtp::Filter.
	virtual bool simple() const override ; // Override from from GSmtp::Filter.
	virtual G::Slot::Signal1<int> & doneSignal() override ; // Override from from GSmtp::Filter.
	virtual void start( const std::string & path ) override ; // Override from from GSmtp::Filter.
	virtual void cancel() override ; // Override from from GSmtp::Filter.
	virtual bool abandoned() const override ; // Override from from GSmtp::Filter.
	virtual std::string response() const override ; // Override from from GSmtp::Filter.
	virtual std::string reason() const override ; // Override from from GSmtp::Filter.
	virtual bool special() const override ; // Override from from GSmtp::Filter.

private:
	NetworkFilter( const NetworkFilter & ) g__eq_delete ;
	void operator=( const NetworkFilter & ) g__eq_delete ;
	void clientEvent( std::string , std::string , std::string ) ;
	void clientDeleted( std::string ) ;

private:
	GNet::ExceptionSink m_es ;
	G::Slot::Signal1<int> m_done_signal ;
	GNet::Location m_location ;
	unsigned int m_connection_timeout ;
	unsigned int m_response_timeout ;
	GNet::ClientPtr<RequestClient> m_client_ptr ;
	std::string m_text ;
} ;

#endif
