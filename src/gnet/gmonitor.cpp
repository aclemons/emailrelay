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
/// \file gmonitor.cpp
///

#include "gdef.h"
#include "gmonitor.h"
#include "ggettext.h"
#include "gstr.h"
#include "gassert.h"
#include <map>
#include <deque>
#include <algorithm> // std::swap()
#include <utility> // std::swap()

//| \class GNet::MonitorImp
/// A pimple-pattern implementation class for GNet::Monitor.
///
class GNet::MonitorImp
{
public:
	using Signal = G::Slot::Signal<const std::string&,const std::string&> ;
	explicit MonitorImp( Monitor & monitor ) ;
	void add( const Connection & , bool is_client ) ;
	void remove( const Connection & , bool is_client ) noexcept ;
	void add( const Listener & ) ;
	void remove( const Listener & ) noexcept ;
	void report( std::ostream & s , const std::string & px , const std::string & eol ) const ;
	void report( G::StringArray & ) const ;
	void emit( Signal & , const char * , const char * ) noexcept ;

private:
	struct ConnectionInfo
	{
		bool is_client ;
	} ;
	using ConnectionMap = std::map<const Connection*,ConnectionInfo> ;
	using ServerMap = std::map<const Listener*,Address> ;

public:
	~MonitorImp() = default ;
	MonitorImp( const MonitorImp & ) = delete ;
	MonitorImp( MonitorImp && ) = delete ;
	MonitorImp & operator=( const MonitorImp & ) = delete ;
	MonitorImp & operator=( MonitorImp && ) = delete ;

private:
	static void add( G::StringArray & , const std::string & , unsigned int , const std::string & , unsigned int , const std::string & ) ;
	static void add( G::StringArray & , const std::string & , const std::string & ) ;
	static void add( G::StringArray & , const std::string & , const std::string & , const std::string & ) ;

private:
	ConnectionMap m_connections ;
	ServerMap m_servers ;
	unsigned long m_client_adds {0UL} ;
	unsigned long m_client_removes {0UL} ;
	unsigned long m_server_peer_adds {0UL} ;
	unsigned long m_server_peer_removes {0UL} ;
} ;

GNet::Monitor * & GNet::Monitor::pthis() noexcept
{
	static GNet::Monitor * p = nullptr ;
	return p ;
}

GNet::Monitor::Monitor() :
	m_imp(std::make_unique<MonitorImp>(*this))
{
	G_ASSERT( pthis() == nullptr ) ;
	pthis() = this ;
}

GNet::Monitor::~Monitor()
{
	pthis() = nullptr ;
}

GNet::Monitor * GNet::Monitor::instance()
{
	return pthis() ;
}

G::Slot::Signal<const std::string&,const std::string&> & GNet::Monitor::signal()
{
	return m_signal ;
}

void GNet::Monitor::addClient( const Connection & client )
{
	if( pthis() )
	{
		pthis()->m_imp->add( client , true ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "out" , "start" ) ;
	}
}

void GNet::Monitor::removeClient( const Connection & client ) noexcept
{
	if( pthis() )
	{
		pthis()->m_imp->remove( client , true ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "out" , "stop" ) ;
	}
}

void GNet::Monitor::addServerPeer( const Connection & server_peer )
{
	if( pthis() )
	{
		pthis()->m_imp->add( server_peer , false ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "in" , "start" ) ;
	}
}


void GNet::Monitor::removeServerPeer( const Connection & server_peer ) noexcept
{
	if( pthis() )
	{
		pthis()->m_imp->remove( server_peer , false ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "in" , "stop" ) ;
	}
}

void GNet::Monitor::addServer( const Listener & server )
{
	if( pthis() )
	{
		pthis()->m_imp->add( server ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "listen" , "start" ) ;
	}
}

void GNet::Monitor::removeServer( const Listener & server ) noexcept
{
	if( pthis() )
	{
		pthis()->m_imp->remove( server ) ;
		pthis()->m_imp->emit( pthis()->m_signal , "listen" , "stop" ) ;
	}
}

void GNet::Monitor::report( std::ostream & s , const std::string & px , const std::string & eol ) const
{
	m_imp->report( s , px , eol ) ;
}

#ifndef G_LIB_SMALL
void GNet::Monitor::report( G::StringArray & out ) const
{
	m_imp->report( out ) ;
}
#endif

// ==

GNet::MonitorImp::MonitorImp( Monitor & )
{
}

void GNet::MonitorImp::add( const Connection & connection , bool is_client )
{
	bool inserted = m_connections.insert(ConnectionMap::value_type(&connection,ConnectionInfo{is_client})).second ;
	if( inserted )
	{
		if( is_client )
			m_client_adds++ ;
		else
			m_server_peer_adds++ ;
	}
}

void GNet::MonitorImp::add( const Listener & server )
{
	m_servers.insert( ServerMap::value_type(&server,server.address()) ) ;
}

void GNet::MonitorImp::emit( Signal & s , const char * a , const char * b ) noexcept
{
	try
	{
		s.emit( std::string(a) , std::string(b) ) ;
	}
	catch(...)
	{
	}
}

void GNet::MonitorImp::remove( const Connection & connection , bool is_client ) noexcept
{
	bool removed = 0U != m_connections.erase( &connection ) ; // noexcept since trivial Compare
	if( removed )
	{
		if( is_client )
			m_client_removes++ ;
		else
			m_server_peer_removes++ ;
	}
}

void GNet::MonitorImp::remove( const Listener & server ) noexcept
{
	m_servers.erase( &server ) ; // noexcept since trivial Compare
}

void GNet::MonitorImp::report( std::ostream & s , const std::string & px , const std::string & eol ) const
{
	using G::txt ;
	for( const auto & server : m_servers )
	{
		s << px << txt("LISTEN: ") << server.second.displayString(true) << eol ;
	}

	s << px << txt("OUT started: ") << m_client_adds << eol ;
	s << px << txt("OUT finished: ") << m_client_removes << eol ;
	{
		for( const auto & connection : m_connections )
		{
			if( connection.second.is_client )
			{
				s << px
					<< txt("OUT: ")
					<< connection.first->localAddress().displayString() << " -> "
					<< connection.first->connectionState() << eol ;
			}
		}
	}

	s << px << txt("IN started: ") << m_server_peer_adds << eol ;
	s << px << txt("IN finished: ") << m_server_peer_removes << eol ;
	{
		for( const auto & connection : m_connections )
		{
			if( !connection.second.is_client )
			{
				s << px
					<< txt("IN: ")
					<< connection.first->localAddress().displayString() << " <- "
					<< connection.first->peerAddress().displayString() << eol ;
			}
		}
	}
}

void GNet::MonitorImp::report( G::StringArray & out ) const
{
	for( const auto & server : m_servers )
		add( out , "Listening address" , server.second.displayString() ) ;

	using G::txt ;
	add( out , txt("Outgoing connections") , m_client_adds , txt("started") , m_client_removes , txt("finished") ) ;
	add( out , txt("Incoming connections") , m_server_peer_adds , txt("started") , m_server_peer_removes , txt("finished") ) ;
	for( const auto & connection : m_connections )
	{
		if( connection.second.is_client )
		{
			add( out , txt("Outgoing connection") ,
				connection.first->localAddress().displayString() ,
				connection.first->connectionState() ) ;
		}
	}
	for( const auto & connection : m_connections )
	{
		if( !connection.second.is_client )
			add( out , txt("Incoming connection") ,
				connection.first->localAddress().displayString() ,
				connection.first->peerAddress().displayString() ) ;
	}
}

void GNet::MonitorImp::add( G::StringArray & out , const std::string & key ,
	unsigned int value_1 , const std::string & suffix_1 ,
	unsigned int value_2 , const std::string & suffix_2 )
{
	add( out , key ,
		G::Str::fromUInt(value_1).append(1U,' ').append(suffix_1) ,
		G::Str::fromUInt(value_2).append(1U,' ').append(suffix_2) ) ;
}

void GNet::MonitorImp::add( G::StringArray & out , const std::string & key , const std::string & value )
{
	add( out , key , value , std::string() ) ;
}

void GNet::MonitorImp::add( G::StringArray & out , const std::string & key , const std::string & value_1 ,
	const std::string & value_2 )
{
	out.push_back( key ) ;
	out.push_back( value_1 ) ;
	out.push_back( value_2 ) ;
}

