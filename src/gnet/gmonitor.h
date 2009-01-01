//
// Copyright (C) 2001-2009 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_GNET_MONITOR_H
#define G_GNET_MONITOR_H

#include "gdef.h"
#include "gslot.h"
#include "gnet.h"
#include "gnoncopyable.h"
#include "gclient.h"
#include "gserver.h"
#include <iostream>

/// \namespace GNet
namespace GNet
{
	class Monitor ;
	class MonitorImp ;
}

/// \class GNet::Monitor
/// A singleton for monitoring SimpleClient and ServerPeer connections.
/// \see GNet::SimpleClient, GNet::ServerPeer
///
class GNet::Monitor : public G::noncopyable 
{
public:
	Monitor() ;
		///< Default constructor.

	virtual ~Monitor() ;
		///< Destructor.

	static Monitor * instance() ;
		///< Returns the singleton pointer. Returns null if none.

	void add( const SimpleClient & client ) ;
		///< Adds a client.

	void remove( const SimpleClient & client ) ;
		///< Removes a client.

	void add( const ServerPeer & peer ) ;
		///< Adds a server peer.

	void remove( const ServerPeer & peer ) ;
		///< Removes a server peer.

	void report( std::ostream & stream , 
		const std::string & line_prefix = std::string() , 
		const std::string & eol = std::string("\n") ) ;
			///< Reports itself onto a stream.

	G::Signal2<std::string,std::string> & signal() ;
		///< Provides a callback signal which can be connect()ed
		///< to a slot.
		///<
		///< The signal emits events with two
		///< string parameters: the first
		///< is "in" or "out", and the second
		///< is "start" or "stop".

private:
	static Monitor * & pthis() ;
	MonitorImp * m_imp ;
	G::Signal2<std::string,std::string> m_signal ;
} ;

#endif
