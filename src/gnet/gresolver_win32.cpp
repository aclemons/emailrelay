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
// gresolver_win32.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gwinhid.h"
#include "gappinst.h"
#include "grequest.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"

/// \class GNet::ResolverImp
/// A pimple-pattern implementation class for GNet::Resolver.
/// 
class GNet::ResolverImp : public GGui::WindowHidden 
{
public:
	ResolverImp( Resolver & resolver , EventHandler * ) ;
		// Constructor.

	virtual ~ResolverImp() ;
		// Destructor.

	bool valid() ;
		// Returns true if the object is valid.

	bool resolveReq( std::string host_part, std::string service_part , bool udp ) ;
		// Issues a resolve request for the given host and service names.

	bool busy() const ;
		// Returns true if resolving is currently in progress.

private:
	void operator=( const ResolverImp & ) ; // not implemented
	ResolverImp( const ResolverImp & ) ; // not implemented
	void cleanup() ;
	void saveHost( const Address &address , const std::string & fqdn ) ;
	void saveService( const Address &address ) ;
	virtual LRESULT onUser( WPARAM wparam , LPARAM lparam ) ;
	void onUserImp( WPARAM wparam , LPARAM lparam ) ;

private:
	Resolver & m_if ;
	EventHandler * m_event_handler ;
	HostRequest * m_host_request ;
	ServiceRequest * m_service_request ;
	std::string m_host ;
	std::string m_service ;
	bool m_udp ;
	Address m_result ;
	std::string m_fqdn ;
} ;

// ===

GNet::ResolverImp::ResolverImp( Resolver & resolver , EventHandler * event_handler ) :
	GGui::WindowHidden( GGui::ApplicationInstance::hinstance() ) ,
	m_if(resolver) ,
	m_event_handler(event_handler) ,
	m_host_request(NULL) ,
	m_service_request(NULL) ,
	m_result(Address::invalidAddress())
{
}

GNet::ResolverImp::~ResolverImp()
{
	cleanup() ;
}

bool GNet::ResolverImp::valid()
{
	return handle() != 0 ;
}

bool GNet::ResolverImp::resolveReq( std::string host_part, std::string service_part , bool udp )
{
	G_ASSERT( !busy() ) ;
	if( busy() )
		return false ;

	m_host = host_part ;
	m_service = service_part ;
	m_udp = udp ;

	m_host_request = new HostRequest( host_part , handle() , Cracker::wm_user() ) ;
	if( !m_host_request->valid() )
	{
		std::string reason = m_host_request->reason() ; // not used
		cleanup() ;
		return false ;
	}

	return true ;
}

void GNet::ResolverImp::cleanup()
{
	delete m_host_request ;
	m_host_request = NULL ;

	delete m_service_request ;
	m_service_request = NULL ;
}

bool GNet::ResolverImp::busy() const
{
	return 
		m_service_request != NULL ||
		m_host_request != NULL ;
}

void GNet::ResolverImp::saveHost( const Address & address , const std::string & fqdn )
{
	m_result = address ;
	m_fqdn = fqdn ;
}

void GNet::ResolverImp::saveService( const Address & address )
{
	G_DEBUG( "GNet::ResolveImp::saveService: " << address.displayString() ) ;
	m_result.setPort( address.port() ) ;
}

LRESULT GNet::ResolverImp::onUser( WPARAM wparam , LPARAM lparam )
{
	try
	{
		G_DEBUG( "GNet::ResolverImp::onUser: wparam = " << wparam << ", lparam = " << lparam ) ;
		onUserImp( wparam , lparam ) ;
	}
	catch( std::exception & e ) // strategy
	{
		if( m_event_handler != NULL )
			m_event_handler->onException( e ) ;
		else
			throw ;
	}
	return 0 ;
}

void GNet::ResolverImp::onUserImp( WPARAM wparam , LPARAM lparam )
{
	if( m_host_request != NULL ) 
	{
		if( m_host_request->onMessage( wparam , lparam ) )
		{
			saveHost( m_host_request->result() , m_host_request->fqdn() ) ;
			cleanup() ;

			m_service_request = new ServiceRequest( m_service , m_udp , handle() , Cracker::wm_user() ) ;
			if( !m_service_request->valid() )
			{
				std::string reason = m_service_request->reason() ;
				cleanup() ;
				m_if.resolveCon( false, Address::invalidAddress(), reason ) ;
			}
		}
		else
		{
			std::string reason = m_host_request->reason() ;
			cleanup() ;
			m_if.resolveCon( false , Address::invalidAddress() , reason ) ;
		}
	}
	else if( m_service_request != NULL ) 
	{
		if( m_service_request->onMessage( wparam , lparam ) )
		{
			saveService( m_service_request->result() ) ;
			Address address( m_result ) ;
			cleanup() ;
			m_if.resolveCon( true , address , m_fqdn ) ; // success
		}
		else
		{
			std::string reason = m_service_request->reason() ;
			cleanup() ;
			m_if.resolveCon( false , Address::invalidAddress() , reason ) ;
		}
	}
}

// ===

GNet::Resolver::Resolver( EventHandler & event_handler ) :
	m_imp(NULL)
{
	ResolverImp * imp = new ResolverImp(*this,&event_handler) ;
	if( !imp->valid() )
		delete imp ;
	else
		m_imp = imp ;
}

GNet::Resolver::~Resolver()
{
	delete m_imp ;
}

bool GNet::Resolver::resolveReq( std::string name , bool udp )
{
	if( m_imp == NULL ) 
		return false ;

	std::string host_part ;
	std::string service_part ;
	if( !parse(name,host_part,service_part) )
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

void GNet::Resolver::resolveCon( bool , const Address &, std::string )
{
	// no-op
}

bool GNet::Resolver::busy() const
{
	return m_imp != NULL ? m_imp->busy() : true ;
}

