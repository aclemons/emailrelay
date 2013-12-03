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
// gresolver_unix.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "glinebuffer.h"
#include "gsimpleclient.h"
#include "gsocketprotocol.h"
#include "gresolverinfo.h"
#include "gexception.h"
#include "gsocket.h"
#include "gevent.h"
#include "gmemory.h"
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
///  Note that the implementation uses GNet::SimpleClient even though 
///  GNet::SimpleClient uses a resolver. This is possible because
///  this class passes a fully-resolved ResolverInfo object to the
///  client and the client class only instantiates a resolver
///  when necessary.
/// 
class GNet::ResolverImp : public GNet::SimpleClient 
{
public:
	ResolverImp( EventHandler & event_handler , Resolver & resolver , unsigned int port ) ;
		// Constructor.

	virtual ~ResolverImp() ;
		// Destructor.

	bool resolveReq( std::string host_part, std::string service_part , bool udp ) ;
		// Issues a resolve request for the given host and service names.

	bool busy() const ;
		// Returns true if resolving is currently in progress.

protected:
	virtual void onConnect() ;
	virtual void onSendComplete() ;
	virtual void onData( const char * , std::string::size_type ) ;
	virtual void onSecure( const std::string & ) ;
	virtual void onException( std::exception & ) ;

private:
	void operator=( const ResolverImp & ) ; // not implemented
	ResolverImp( const ResolverImp & ) ; // not implemented
	static ResolverInfo resolverInfo( unsigned int ) ;

private:
	EventHandler & m_event_handler ;
	Resolver & m_outer ;
	LineBuffer m_line_buffer ;
	std::string m_request ;
} ;

// ===

GNet::ResolverImp::ResolverImp( EventHandler & event_handler , Resolver & resolver , unsigned int port ) :
	SimpleClient(resolverInfo(port)) ,
	m_event_handler(event_handler) ,
	m_outer(resolver)
{ 
}

GNet::ResolverImp::~ResolverImp() 
{
}

GNet::ResolverInfo GNet::ResolverImp::resolverInfo( unsigned int port )
{
	ResolverInfo info( "localhost" , "0" ) ;
	info.update( Address::localhost(port) , "localhost" ) ;
	return info ;
}

bool GNet::ResolverImp::resolveReq( std::string host_part, std::string service_part , bool udp )
{
	if( ! m_request.empty() )
		return false ; // still busy
	m_request = host_part + ":" + service_part + ":" + ( udp ? "udp" : "tcp" ) + "\n" ;

	if( connected() )
		send( m_request ) ;
	else
		connect() ;

	return true ;
}

void GNet::ResolverImp::onConnect()
{
	if( ! m_request.empty() )
		send( m_request ) ;
}

void GNet::ResolverImp::onSendComplete()
{
}

void GNet::ResolverImp::onSecure( const std::string & )
{
}

void GNet::ResolverImp::onData( const char * p , std::string::size_type n )
{
	m_line_buffer.add( p , n ) ;
	while( m_line_buffer.more() )
	{
		m_request.erase() ;

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

void GNet::ResolverImp::onException( std::exception & e )
{
	if( busy() )
	{
		m_request.erase() ;
		m_outer.resolveCon( false , Address::invalidAddress() , e.what() ) ;
	}
	else
	{
		m_event_handler.onException( e ) ;
	}
}

bool GNet::ResolverImp::busy() const
{
	return ! m_request.empty() ;
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

