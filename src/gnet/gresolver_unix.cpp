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
//
// gresolver_unix.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "glinebuffer.h"
#include "gsender.h"
#include "gexception.h"
#include "gsocket.h"
#include "gevent.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

namespace
{
	const unsigned int c_port = 208U ;
}

/// \class GNet::ResolverImp
/// A pimple-pattern implementation class for GNet::Resolver.
/// 
class GNet::ResolverImp : public GNet::EventHandler 
{
public:
	ResolverImp( EventHandler & event_handler , Resolver & resolver , unsigned int port ) ;
	virtual ~ResolverImp() ;
	bool resolveReq( std::string host_part, std::string service_part , bool udp ) ;
	void cancelReq() ;
	bool busy() const ;

private:
	void operator=( const ResolverImp & ) ;
	ResolverImp( const ResolverImp & ) ;
	void end() ;
	void readEvent() ;
	void writeEvent() ;
	void onException( std::exception & ) ;

private:
	EventHandler & m_event_handler ;
	Sender m_sender ;
	LineBuffer m_line_buffer ;
	Address m_address ;
	Resolver & m_outer ;
	StreamSocket * m_s ;
	std::string m_request ;
} ;

// ===

GNet::ResolverImp::ResolverImp( EventHandler & event_handler , Resolver & resolver , unsigned int port ) :
	m_event_handler(event_handler) ,
	m_sender(event_handler) ,
	m_address(Address::localhost(port)) ,
	m_outer(resolver) ,
	m_s(NULL)
{ 
}

GNet::ResolverImp::~ResolverImp() 
{
	delete m_s ;
}

bool GNet::ResolverImp::resolveReq( std::string host_part, std::string service_part , bool udp )
{
	if( m_s != NULL ) 
		return false ; // still busy

	m_request = host_part + ":" + service_part + ":" + ( udp ? "udp" : "tcp" ) + "\n" ;
	m_s = new StreamSocket ;
	if( ! m_s->valid() || ! m_s->connect(m_address) )
	{
		StreamSocket * s = m_s ;
		m_s = NULL ;
		delete s ;
		return false ;
	}
	else
	{
		m_s->addWriteHandler( *this ) ;
		return true ;
	}
}

void GNet::ResolverImp::writeEvent()
{
	G_ASSERT( m_s != NULL ) ;
	std::pair<bool,Address> peer_pair = m_s->getPeerAddress() ;
	bool connected = peer_pair.first ;

	if( !connected )
	{
		end() ;
		m_outer.resolveCon( false , Address::invalidAddress() , 
			std::string("cannot connect to the resolver daemon at ") + m_address.displayString() ) ;
	}
	else 
	{
		if( m_sender.busy() )
		{
			m_sender.resumeSending( *m_s ) ;
		}
		else
		{
			m_s->addReadHandler( *this ) ;
			m_s->dropWriteHandler() ;
			m_sender.send( *m_s , m_request ) ;
		}
		if( m_sender.failed() )
		{
			end() ;
			m_outer.resolveCon( false , Address::invalidAddress() , 
				std::string("cannot communicate with resolver daemon at ") + m_address.displayString() ) ;
		}
	}
}

void GNet::ResolverImp::readEvent()
{
	G_ASSERT( m_s != NULL ) ;

	static char buffer[200U] ;
	ssize_t rc = m_s->read( buffer , sizeof(buffer) ) ;
	G_DEBUG( "GNet::ResolverImp::readEvent: " << rc << " byte(s)" ) ;

	end() ;
	if( rc == 0 )
	{
		m_outer.resolveCon( false , Address::invalidAddress() , "disconnected" ) ;
	}
	else
	{
		std::string::size_type n = static_cast<std::string::size_type>(rc) ;
		m_line_buffer.add( buffer , n ) ;
		if( m_line_buffer.more() )
		{
			std::string result = m_line_buffer.line() ;
			G_DEBUG( "GNet::ResolverImp::readEvent: \"" << result << "\"" ) ;
			G::Str::trim( result , " \n\r" ) ;
			std::string::size_type pos = result.find( ' ' ) ;
			std::string head = pos == std::string::npos ? result : result.substr(0U,pos) ;
			std::string tail = pos == std::string::npos ? std::string() : result.substr(pos+1U) ;
			if( Address::validString(head) )
			{
				G::Str::trim( tail , " \n" ) ;
				m_outer.resolveCon( true , Address(head) , tail ) ;
			}
			else
			{
				std::string reason = result ;
				reason = G::Str::isPrintableAscii( reason ) ? reason : std::string("dns error") ;
				m_outer.resolveCon( false , Address::invalidAddress() , reason ) ;
			}
		}
	}
}

void GNet::ResolverImp::onException( std::exception & e )
{
	m_event_handler.onException( e ) ;
}

void GNet::ResolverImp::cancelReq()
{
	end() ;
}

void GNet::ResolverImp::end()
{
	delete m_s ;
	m_s = NULL ;
}

bool GNet::ResolverImp::busy() const
{
	return m_s != NULL ;
}

// ===

GNet::Resolver::Resolver( EventHandler & event_handler ) :
	m_imp(new ResolverImp(event_handler,*this,c_port))
{
}

GNet::Resolver::~Resolver()
{
	delete m_imp ;
}

bool GNet::Resolver::resolveReq( std::string name , bool udp )
{
	std::string host_part ;
	std::string service_part ;
	if( ! parse(name,host_part,service_part) )
		return false ;

	return m_imp->resolveReq( host_part , service_part , udp ) ;
}

bool GNet::Resolver::resolveReq( std::string host_part, std::string service_part , bool udp )
{
	if( m_imp == NULL ) 
		return false ;

	if( host_part.length() == 0 )
		host_part = "0.0.0.0" ;

	if( service_part.length() == 0 )
		service_part = "0" ;

	return m_imp->resolveReq( host_part , service_part , udp ) ;
}

void GNet::Resolver::resolveCon( bool , const Address & , std::string )
{
	// no-op
}

bool GNet::Resolver::busy() const
{
	return m_imp->busy() ;
}

void GNet::Resolver::cancelReq()
{
	m_imp->cancelReq() ;
}

