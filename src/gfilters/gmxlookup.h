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
/// \file gmxlookup.h
///

#ifndef G_MX_LOOKUP_H
#define G_MX_LOOKUP_H

#include "gdef.h"
#include "gmessagestore.h"
#include "gaddress.h"
#include "gsocket.h"
#include "geventhandler.h"
#include "gdatetime.h"
#include "gtimer.h"
#include "gslot.h"
#include <string>
#include <vector>

namespace GFilters
{
	class MxLookup ;
}

//| \class GFilters::MxLookup
/// A DNS MX lookup client.
///
/// Each nameserver is queried in turn with a 'ns_timeout' interval.
/// After the final nameserver has been queried there is a
/// 'restart_timeout' before the sequence starts again. There is no
/// overall timeout.
///
class GFilters::MxLookup : private GNet::EventHandler
{
public:
	struct Config /// A configuration structure for GFilters::MxLookup
	{
		Config() ;
		G::TimeInterval ns_timeout {1U,0} ;
		G::TimeInterval restart_timeout {15U,0} ;
	} ;

	static bool enabled() ;
		///< Returns true if implemented.

	explicit MxLookup( GNet::ExceptionSink , Config = {} ) ;
		///< Constructor.

	explicit MxLookup( GNet::ExceptionSink , Config , const std::vector<GNet::Address> & ns ) ;
		///< Constructor taking a list of nameservers.
		/// \see GNet::nameservers()

	void start( const GStore::MessageId & , const std::string & question_domain , unsigned int port ) ;
		///< Starts the lookup.

	G::Slot::Signal<GStore::MessageId,std::string,std::string> & doneSignal() noexcept ;
		///< Returns a reference to the completion signal. The signal
		///< parameters are (1) the original message id, (2) the answer
		///< port-25 transport address (if successful), and (3) the
		///< error reason (if not).

	void cancel() ;
		///< Cancels the lookup so the doneSignal() is not emitted.

private: // overrides
	void readEvent() override ;

private:
	void startTimer() ;
	void onTimeout() ;
	void sendMxQuestion( std::size_t , const std::string & ) ;
	void sendHostQuestion( std::size_t , const std::string & ) ;
	void fail( const std::string & ) ;
	void succeed( const std::string & ) ;
	void dropReadHandlers() ;
	GNet::DatagramSocket & socket( std::size_t ) ;
	void process( const char * , std::size_t ) ;
	void disable( std::size_t , const std::string & ) ;

private:
	GNet::ExceptionSink m_es ;
	Config m_config ;
	GStore::MessageId m_message_id ;
	std::string m_question ;
	unsigned int m_port {0U} ;
	std::string m_error ;
	std::size_t m_ns_index ;
	std::size_t m_ns_failures ;
	std::vector<GNet::Address> m_nameservers ;
	GNet::Timer<MxLookup> m_timer ;
	std::unique_ptr<GNet::DatagramSocket> m_socket4 ;
	std::unique_ptr<GNet::DatagramSocket> m_socket6 ;
	G::Slot::Signal<GStore::MessageId,std::string,std::string> m_done_signal ;
} ;

#endif
