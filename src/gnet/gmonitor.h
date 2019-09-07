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
/// \file gmonitor.h
///

#ifndef G_NET_MONITOR__H
#define G_NET_MONITOR__H

#include "gdef.h"
#include "gslot.h"
#include "gconnection.h"
#include <iostream>
#include <utility>

namespace GNet
{
	class Monitor ;
	class MonitorImp ;
}

/// \class GNet::Monitor
/// A singleton for monitoring GNet::Client and GNet::ServerPeer
/// connections.
/// \see GNet::Client, GNet::ServerPeer
///
class GNet::Monitor
{
public:
	Monitor() ;
		///< Default constructor.

	~Monitor() ;
		///< Destructor.

	static Monitor * instance() ;
		///< Returns the singleton pointer. Returns nullptr if none.

	static void addClient( const Connection & simple_client ) ;
		///< Adds a client connection.

	static void removeClient( const Connection & simple_client ) ;
		///< Removes a client connection.

	static void addServerPeer( const Connection & server_peer ) ;
		///< Adds a server connection.

	static void removeServerPeer( const Connection & server_peer ) ;
		///< Removes a server connection.

	void report( std::ostream & stream ,
		const std::string & line_prefix = std::string() ,
		const std::string & eol = std::string("\n") ) const ;
			///< Reports itself onto a stream.

	void report( G::StringArray & out ) const ;
		///< Reports itself into a three-column table (ordered
		///< with the column index varying fastest).

	G::Slot::Signal2<std::string,std::string> & signal() ;
		///< Provides a callback signal which can be connect()ed
		///< to a slot.
		///<
		///< The signal emits events with two string parameters:
		///< the first is "in" or "out", and the second is
		///< "start" or "stop".

private:
	Monitor( const Monitor & ) g__eq_delete ;
	void operator=( const Monitor & ) g__eq_delete ;

private:
	static Monitor * & pthis() ;
	unique_ptr<MonitorImp> m_imp ;
	G::Slot::Signal2<std::string,std::string> m_signal ;
} ;

#endif
