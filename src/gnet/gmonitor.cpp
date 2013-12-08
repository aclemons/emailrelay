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
//
// gmonitor.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "glimits.h"
#include "gmonitor.h"
#include "gstr.h"
#include "gassert.h"
#include <map>
#include <deque>
#include <algorithm> // std::swap
#include <utility> // std::swap

/// \class GNet::MonitorImp
/// A pimple pattern implementation class for GNet::Monitor.
/// 
class GNet::MonitorImp 
{
public:
	explicit MonitorImp( Monitor & monitor ) ;
	void add( const Connection & , bool is_client ) ;
	void remove( const Connection & , bool is_client ) ;
	void report( std::ostream & s , const std::string & px , const std::string & eol ) const ;
	std::string certificateId( const std::string & certificiate ) ;
	std::pair<std::string,bool> findCertificate( const std::string & certificate ) ;

private:
	struct ConnectionInfo
	{
		bool is_client ;
		explicit ConnectionInfo( bool is_client_ ) : is_client(is_client_) {}
	} ;
	struct CertificateInfo
	{
		std::string certificate ;
		int id ;
		CertificateInfo( const std::string & c , int id_ ) : certificate(c) , id(id_) {}
		bool match( std::string s ) const { return s == certificate ; }
	} ;
	struct CertificateMatch
	{
		const std::string & m_s ;
		explicit CertificateMatch( const std::string & s ) : m_s(s) {}
		bool operator()( const CertificateInfo & o ) const { return o.match(m_s) ; }
	} ;
	typedef std::map<const Connection*,ConnectionInfo> ConnectionMap ;
	typedef std::deque<CertificateInfo> Certificates ;
	ConnectionMap m_connections ;
	Certificates m_certificates ;
	int m_id_generator ;
	unsigned long m_client_adds ;
	unsigned long m_client_removes ;
	unsigned long m_server_peer_adds ;
	unsigned long m_server_peer_removes ;
} ;

GNet::MonitorImp::MonitorImp( Monitor & monitor ) :
	m_id_generator(0) ,
	m_client_adds(0UL) ,
	m_client_removes(0UL) ,
	m_server_peer_adds(0UL) ,
	m_server_peer_removes(0UL)
{
}

// ===

GNet::Monitor * & GNet::Monitor::pthis()
{
	static GNet::Monitor * p = NULL ;
	return p ;
}

GNet::Monitor::Monitor() :
	m_imp( new MonitorImp(*this) )
{
	G_ASSERT( pthis() == NULL ) ;
	pthis() = this ;
}

GNet::Monitor::~Monitor()
{
	delete m_imp ;
	pthis() = NULL ;
}

GNet::Monitor * GNet::Monitor::instance()
{
	return pthis() ;
}

G::Signal2<std::string,std::string> & GNet::Monitor::signal()
{
	return m_signal ;
}

void GNet::Monitor::addClient( const Connection & client )
{
	m_imp->add( client , true ) ;
	m_signal.emit( "out" , "start" ) ;
}

void GNet::Monitor::removeClient( const Connection & client )
{
	m_imp->remove( client , true ) ;
	m_signal.emit( "out" , "end" ) ;
}

void GNet::Monitor::addServerPeer( const Connection & server_peer )
{
	m_imp->add( server_peer , false ) ;
	m_signal.emit( "in" , "start" ) ;
}


void GNet::Monitor::removeServerPeer( const Connection & server_peer )
{
	m_imp->remove( server_peer , false ) ;
	m_signal.emit( "in" , "end" ) ;
}

std::pair<std::string,bool> GNet::Monitor::findCertificate( const std::string & certificate )
{
	return m_imp->findCertificate( certificate ) ;
}

void GNet::Monitor::report( std::ostream & s , const std::string & px , const std::string & eol ) const
{
	m_imp->report( s , px , eol ) ;
}

// ==

void GNet::MonitorImp::add( const Connection & connection , bool is_client )
{
	bool inserted = m_connections.insert(ConnectionMap::value_type(&connection,ConnectionInfo(is_client))).second ;
	if( inserted )
	{
		if( is_client )
			m_client_adds++ ;
		else
			m_server_peer_adds++ ;
	}
}

void GNet::MonitorImp::remove( const Connection & connection , bool is_client )
{
	bool removed = 0U != m_connections.erase( &connection ) ;
	if( removed )
	{
		if( is_client )
			m_client_removes++ ;
		else
			m_server_peer_removes++ ;
	}
}

void GNet::MonitorImp::report( std::ostream & s , const std::string & px , const std::string & eol ) const
{
	s << px << "OUT started: " << m_client_adds << eol ;
	s << px << "OUT finished: " << m_client_removes << eol ;
	{
		for( ConnectionMap::const_iterator p = m_connections.begin() ; p != m_connections.end() ; ++p )
		{
			if( (*p).second.is_client )
			{
				s << px
					<< "OUT: "
					<< (*p).first->localAddress().second.displayString() << " -> " 
					<< (*p).first->peerAddress().second.displayString() << eol ;
			}
		}
	}

	s << px << "IN started: " << m_server_peer_adds << eol ;
	s << px << "IN finished: " << m_server_peer_removes << eol ;
	{
		for( ConnectionMap::const_iterator p = m_connections.begin() ; p != m_connections.end() ; ++p )
		{
			if( !(*p).second.is_client )
			{
				s << px
					<< "IN: "
					<< (*p).first->localAddress().second.displayString() << " <- " 
					<< (*p).first->peerAddress().second.displayString() << eol ;
			}
		}
	}
}

std::pair<std::string,bool> GNet::MonitorImp::findCertificate( const std::string & certificate )
{
	std::pair<std::string,bool> result( std::string() , false ) ;
	if( certificate.empty() )
		return result ;

	// lru cache
	Certificates::iterator p = std::find_if( m_certificates.begin() , m_certificates.end() , CertificateMatch(certificate) ) ;
	if( p != m_certificates.end() )
	{
		CertificateInfo tmp = *p ;
		result.first = G::Str::fromInt( (*p).id ) ;
		result.second = false ;
		p = std::remove_if( m_certificates.begin() , m_certificates.end() , CertificateMatch(certificate) ) ;
		G_ASSERT( p != m_certificates.end() ) ;
		*p = tmp ;
	}
	else
	{
		const size_t limit = G::limits::net_certificate_cache_size ;
		if( m_certificates.size() == limit && !m_certificates.empty() )
			m_certificates.pop_front() ;
		int id = ++m_id_generator ;
		m_certificates.push_back( CertificateInfo(certificate,id) ) ;
		result.first = G::Str::fromInt( id ) ;
		result.second = true ;
	}
	return result ;
}

