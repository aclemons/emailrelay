//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file servicecontrol_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "servicecontrol.h"
#include "ggettext.h"
#include <sstream>
#include <utility>
#include <stdexcept>
#include <new>

namespace ServiceControl
{
	struct Error ;
	class Manager ;
	class Service ;
	struct ScopeExitCloser ;
}

struct ServiceControl::Error : std::runtime_error
{
	Error( const std::string & s , DWORD ) ;
	DWORD m_error {0} ;
	static std::string decode( DWORD ) ;
} ;

struct ServiceControl::ScopeExitCloser
{
	explicit ScopeExitCloser( SC_HANDLE h ) : m_h(h) {}
	~ScopeExitCloser() { CloseServiceHandle( m_h ) ; }
	ScopeExitCloser( const ScopeExitCloser & ) = delete ;
	ScopeExitCloser( ScopeExitCloser && ) = delete ;
	ScopeExitCloser & operator=( const ScopeExitCloser & ) = delete ;
	ScopeExitCloser & operator=( ScopeExitCloser && ) = delete ;
	SC_HANDLE m_h ;
} ;

class ServiceControl::Manager
{
public:
	explicit Manager( DWORD access = SC_MANAGER_ALL_ACCESS ) ;
	~Manager() ;
	SC_HANDLE h() const ;

public:
	Manager( const Manager & ) = delete ;
	Manager( Manager && ) = delete ;
	Manager & operator=( const Manager & ) = delete ;
	Manager & operator=( Manager && ) = delete ;

private:
	SC_HANDLE m_h ;
} ;

class ServiceControl::Service
{
public:
	explicit Service( const Manager & manager , const std::string & name , DWORD access ) ;
	Service() = default ;
	~Service() ;
	void create( const Manager & manager , const std::string & name , const std::string & display_name ,
		DWORD start_type , const std::string & commandline ) ;
	void configure( const std::string & description , const std::string & display_name ) ;
	void start() ;
	void stop() ;
	void remove() ;
	bool stopped() const ;

public:
	Service( const Service & ) = delete ;
	Service( Service && ) = delete ;
	Service & operator=( const Service & ) = delete ;
	Service & operator=( Service && ) = delete ;

private:
	SC_HANDLE open( SC_HANDLE , const std::string & ) ;
	static void removeImp( SC_HANDLE ) ;
	static void removeImp( SC_HANDLE , std::nothrow_t ) ;
	void stop( SC_HANDLE ) ;
	void stop( SC_HANDLE , std::nothrow_t ) ;
	DWORD status() const ;
	SC_HANDLE h() const ;

private:
	SC_HANDLE m_h{0} ;
} ;

// ==

ServiceControl::Error::Error( const std::string & s , DWORD e ) :
	std::runtime_error(s+": "+decode(e)) ,
	m_error(e)
{
}

std::string ServiceControl::Error::decode( DWORD e )
{
	using G::txt ;
	switch( e )
	{
		case ERROR_ACCESS_DENIED: return txt("access denied") ;
		case ERROR_DATABASE_DOES_NOT_EXIST: return txt("service database does not exist") ;
		case ERROR_INVALID_PARAMETER: return txt("invalid parameter") ;
		case ERROR_CIRCULAR_DEPENDENCY: return txt("circular dependency") ;
		case ERROR_DUPLICATE_SERVICE_NAME: return txt("duplicate service name") ;
		case ERROR_INVALID_HANDLE: return txt("invalid handle") ;
		case ERROR_INVALID_NAME: return txt("invalid name") ;
		case ERROR_INVALID_SERVICE_ACCOUNT: return txt("invalid service account") ;
		case ERROR_SERVICE_EXISTS: return txt("service already exists") ;
		case ERROR_SERVICE_MARKED_FOR_DELETE: return txt("already marked for deletion") ;
		case ERROR_SERVICE_DOES_NOT_EXIST: return txt("no such service") ;
	}
	std::ostringstream ss ;
	ss << "error " << e ;
	return ss.str() ;
}

// ==

ServiceControl::Manager::Manager( DWORD access )
{
	m_h = G::nowide::openSCManagerW( access ) ;
	if( m_h == 0 )
	{
		DWORD e = GetLastError() ;
		throw Error( "cannot open service control manager" , e ) ;
	}
}

ServiceControl::Manager::~Manager()
{
	CloseServiceHandle( m_h ) ;
}

SC_HANDLE ServiceControl::Manager::h() const
{
	return m_h ;
}

// ==

ServiceControl::Service::Service( const Manager & manager , const std::string & name , DWORD /*access*/ ) :
	m_h(open(manager.h(),name))
{
}

ServiceControl::Service::~Service()
{
	if( m_h )
		CloseServiceHandle( m_h ) ;
}

SC_HANDLE ServiceControl::Service::open( SC_HANDLE hmanager , const std::string & name )
{
	SC_HANDLE h = G::nowide::openServiceW( hmanager , name ,
		DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_START ) ;

	if( h == 0 )
	{
		DWORD e = GetLastError() ;
		throw Error( "cannot open service" , e ) ;
	}
	return h ;
}

SC_HANDLE ServiceControl::Service::h() const
{
	return m_h ;
}

void ServiceControl::Service::create( const Manager & manager , const std::string & name ,
	const std::string & display_name , DWORD start_type , const std::string & commandline )
{
	m_h = G::nowide::createServiceW( manager.h() , name , display_name , start_type , commandline ) ;
	if( m_h == 0 )
	{
		DWORD e = GetLastError() ;
		if( e == ERROR_SERVICE_EXISTS )
		{
			// remove it
			{
				SC_HANDLE h = open( manager.h() , name ) ;
				ScopeExitCloser closer( h ) ;
				stop( h , std::nothrow ) ;
				removeImp( h , std::nothrow ) ;
			}

			// try again
			m_h = G::nowide::createServiceW( manager.h() , name , display_name , start_type , commandline ) ;
			if( m_h == 0 )
				e = GetLastError() ;
		}
		if( m_h == 0 )
			throw Error( "cannot create service" , e ) ;
	}
}

void ServiceControl::Service::configure( const std::string & description_in , const std::string & display_name )
{
	std::string description = description_in ;
	if( description.empty() )
		description = display_name + " service" ;

	static constexpr std::size_t limit = 2048U ;
	if( (description.length()+5U) > limit )
	{
		description.resize( limit-5U ) ;
		description.append( "..." ) ;
	}

	G::nowide::changeServiceConfigW( m_h , description ) ; // ignore errors
}

void ServiceControl::Service::stop()
{
	stop( m_h , std::nothrow ) ;
}

void ServiceControl::Service::stop( SC_HANDLE h , std::nothrow_t )
{
	SERVICE_STATUS status ;
	bool stop_ok = !!ControlService( h , SERVICE_CONTROL_STOP , &status ) ;
	if( stop_ok )
		Sleep( 1000 ) ; // arbitrary sleep to allow the service to actually stop
}

void ServiceControl::Service::remove()
{
	removeImp( m_h ) ;
}

void ServiceControl::Service::removeImp( SC_HANDLE h , std::nothrow_t )
{
	DeleteService( h ) ;
}

void ServiceControl::Service::removeImp( SC_HANDLE h )
{
	bool delete_ok = !!DeleteService( h ) ;
	if( !delete_ok )
	{
		DWORD e = GetLastError() ;
		throw Error( "cannot remove the service" , e ) ;
	}
}

DWORD ServiceControl::Service::status() const
{
	SERVICE_STATUS_PROCESS status ;
	DWORD n = 0 ;
	auto rc = QueryServiceStatusEx( m_h , SC_STATUS_PROCESS_INFO ,
		reinterpret_cast<LPBYTE>(&status) , sizeof(status) , &n ) ;
	if( !rc )
	{
		DWORD e = GetLastError() ;
		throw Error( "cannot get current status" , e ) ;
	}
	return status.dwCurrentState ;
}

bool ServiceControl::Service::stopped() const
{
	return status() == SERVICE_STOPPED ;
}

void ServiceControl::Service::start()
{
	if( !G::nowide::startServiceW(m_h) )
	{
		DWORD e = GetLastError() ;
		throw Error( "cannot start the service" , e ) ;
	}
}

// ==

std::pair<std::string,DWORD> service_install( const std::string & commandline , const std::string & name ,
	const std::string & display_name , const std::string & description , bool autostart )
{
	using namespace ServiceControl ;
	try
	{
		if( name.empty() || display_name.empty() )
			throw Error( "invalid zero-length service name" , ERROR_INVALID_NAME ) ;

		Manager manager ;
		Service service ;
		DWORD start_type = autostart ? SERVICE_AUTO_START : SERVICE_DEMAND_START ;
		service.create( manager , name , display_name , start_type , commandline ) ;
		service.configure( description , display_name ) ;
		return {{},0} ;
	}
	catch( Error & e )
	{
		return {e.what(),e.m_error} ;
	}
	catch( std::exception & e )
	{
		return {e.what(),ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
	catch(...)
	{
		return {"failed",ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
}

bool service_installed( const std::string & name )
{
	using namespace ServiceControl ;
	try
	{
		Manager manager( SC_MANAGER_CONNECT ) ;
		Service service( manager , name , SERVICE_QUERY_CONFIG ) ;
		return true ;
	}
	catch(...)
	{
		return false ;
	}
}

std::pair<std::string,DWORD> service_remove( const std::string & name )
{
	using namespace ServiceControl ;
	try
	{
		Manager manager( SC_MANAGER_ALL_ACCESS ) ;
		Service service( manager , name , DELETE | SERVICE_STOP ) ;
		service.stop() ;
		service.remove() ;
		return {{},0} ;
	}
	catch( Error & e )
	{
		return {e.what(),e.m_error} ;
	}
	catch( std::exception & e )
	{
		return {e.what(),ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
	catch(...)
	{
		return {"failed",ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
}

std::pair<std::string,DWORD> service_start( const std::string & name )
{
	using namespace ServiceControl ;
	try
	{
		Manager manager( SC_MANAGER_ALL_ACCESS ) ;
		Service service( manager , name , SERVICE_START ) ;
		if( !service.stopped() )
			throw Error( "already running" , ERROR_SERVICE_ALREADY_RUNNING ) ;
		service.start() ;
		return {{},0} ;
	}
	catch( Error & e )
	{
		return {e.what(),e.m_error} ;
	}
	catch( std::exception & e )
	{
		return {e.what(),ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
	catch(...)
	{
		return {"failed",ERROR_SERVICE_SPECIFIC_ERROR} ;
	}
}

