//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gresolve_unix.cpp
//

#include "gdef.h"
#include "gresolve.h"
#include "gexception.h"
#include "gsocket.h"
#include "gevent.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

// Class: GNet::ResolverImp
// Description: A pimple-pattern implementation class for GNet::Resolver.
//
class GNet::ResolverImp : public GNet::EventHandler 
{
public:
	ResolverImp( Resolver & resolver , unsigned int port ) ;
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

private:
	Address m_address ;
	Resolver & m_outer ;
	StreamSocket * m_s ;
	std::string m_request ;
} ;

// ===

GNet::ResolverImp::ResolverImp( Resolver & resolver , unsigned int port ) :
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

	m_request = 
		host_part + std::string(":") + 
		service_part + std::string(":") +
		( udp ? std::string("udp") : std::string("tcp") ) +
		std::string("\n") ;

	m_s = new StreamSocket ;
	if( ! m_s->valid() || ! m_s->bind() || ! m_s->connect(m_address) )
	{
		delete m_s ;
		m_s = NULL ;
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
        if( peer_pair.first )
        {
		m_s->addReadHandler( *this ) ;
		m_s->dropWriteHandler() ;
		m_s->write( m_request.c_str() , m_request.length() ) ;
	}
	else
	{
		end() ;
		m_outer.resolveCon( false , Address::invalidAddress() , 
			std::string("cannot connect to the resolver daemon at ") + m_address.displayString() ) ;
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
		std::string result( buffer , rc ) ;
		G_DEBUG( "GNet::ResolverImp::readEvent: \"" << result << "\"" ) ;
		G::Str::trim( result , " \n" ) ;
		size_t pos = result.find( ' ' ) ;
		std::string head = pos == std::string::npos ? result : result.substr(0U,pos) ;
		std::string tail = pos == std::string::npos ? std::string() : result.substr(pos+1U) ;
		if( Address::validString(head) )
		{
			G::Str::trim( tail , " \n" ) ;
			m_outer.resolveCon( true , Address(result) , tail ) ;
		}
		else
		{
			std::string reason = result ;
			reason = G::Str::isPrintableAscii( reason ) ? reason : std::string("dns error") ;
			m_outer.resolveCon( false , Address::invalidAddress() , reason ) ;
		}
	}
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

GNet::Resolver::Resolver() :
	m_imp(NULL)
{
	const unsigned int port = 208U ;
	m_imp = new ResolverImp( *this , port ) ;
}

GNet::Resolver::~Resolver()
{
	delete m_imp ;
}

bool GNet::Resolver::resolveReq( std::string name , bool udp )
{
	if( m_imp == NULL ) 
		return false ;

	size_t colon = name.find( ":" ) ;
	if( colon == std::string::npos )
	{
		return false ;
	}

	std::string host_part = name.substr( 0U , colon ) ;
	std::string service_part = name.substr( colon+1U ) ; 

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


