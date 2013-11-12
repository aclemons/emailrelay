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
// geventloop_win32.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gstr.h"
#include "gevent.h"
#include "gpump.h"
#include "gappinst.h"
#include "gwinbase.h"
#include "gwinhid.h"
#include "gsetter.h"
#include "gexception.h"
#include "gtimer.h"
#include "gassert.h"
#include "gdebug.h"
#include "glog.h"
#include <stdexcept>

namespace GNet
{
	class Winsock ;
	class WinsockWindow ;
} ;

/// \class GNet::Winsock
/// A concrete implementation of GNet::EventLoop using WinSock.
/// 
///  Note that WinSock events are delivered as messages to the application's 
///  main message queue, and these messages have to be passed on the the WinSock 
///  layer.
/// 
class GNet::Winsock : public GNet::EventLoop 
{
public:
	Winsock() ;
		// Default constructor. Initialise with either init().

	virtual bool init() ;
		// Override from EventLoop. Initialses the WinSock library,
		// and creates an internal hidden window. The window is used
		// as a conduit for select events and timer events.
		// Returns false on error.

	std::string reason() const ;
		// Returns the reason for initialisation failure.

	std::string id() const ;
		// Returns the WinSock implementation's identification string. 
		// Returns the zero length string on error.

	virtual ~Winsock() ;
		// Destructor. Releases the WinSock resources if this is the last 
		// Winsock object.

	void onMessage( WPARAM wparam , LPARAM lparam ) ;
		// To be called when the window receives a winsock message.

	void onTimer() ;
		// To be called when the hidden window receives a WM_TIMER message.

	virtual std::string run() ;
		// Override from EventLoop. Calls GGui::Pump::run(). Returns
		// the quit reason if quit() was called. Throws a runtime_error 
		// if the hidden window's wndproc (nearly) threw an exception.

	virtual bool running() const ;
		// Override from EventLoop. Returns true if in run().

	virtual void quit( std::string ) ;
		// Override from EventLoop. Calls GGui::Pump::quit().

protected:
	virtual void addRead( Descriptor fd , EventHandler & handler ) ;
	virtual void addWrite( Descriptor fd , EventHandler & handler ) ;
	virtual void addException( Descriptor fd , EventHandler & handler ) ;
	virtual void dropRead( Descriptor fd ) ;
	virtual void dropWrite( Descriptor fd ) ;
	virtual void dropException( Descriptor fd ) ;
	virtual void setTimeout( G::DateTime::EpochTime , bool & ) ;

private:
	Winsock( const Winsock & other ) ;
	void operator=( const Winsock & other ) ;
	bool attach( unsigned int msg , unsigned int timer_id = 0U ) ;
	EventHandler * findHandler( EventHandlerList & list , Descriptor fd ) ;
	void update( Descriptor fd ) ;
	long desiredEvents( Descriptor fd ) ;

private:
	bool m_running ;
	GGui::Window * m_window ;
	HWND m_hwnd ;
	bool m_success ;
	std::string m_reason ;
	std::string m_id ;
	unsigned m_msg ;
	EventHandlerList m_read_list ;
	EventHandlerList m_write_list ;
	EventHandlerList m_exception_list ;
	unsigned int m_timer_id ;
} ;

/// \class GNet::WinsockWindow
/// An private implementation class used by GNet::Winsock
///  to hook into GGui::Window event processing.
/// 
class GNet::WinsockWindow : public GGui::WindowHidden 
{
public:
	WinsockWindow( Winsock & ws , HINSTANCE h ) ;
private:
	virtual void onWinsock( WPARAM , LPARAM ) ;
	virtual void onTimer( unsigned int ) ;
	Winsock & m_ws ;
} ;

// ===

GNet::WinsockWindow::WinsockWindow( Winsock & ws , HINSTANCE hinstance ) :
	GGui::WindowHidden(hinstance) ,
	m_ws(ws)
{
}

void GNet::WinsockWindow::onWinsock( WPARAM w , LPARAM l )
{
	m_ws.onMessage( w , l ) ;
}

void GNet::WinsockWindow::onTimer( unsigned int timer_id )
{
	G_DEBUG( "GNet::WinsockWindow::onTimer: " << timer_id ) ;
	m_ws.onTimer() ;
}

// ===

GNet::EventLoop * GNet::EventLoop::create()
{
	return new Winsock ;
}

// ===

GNet::Winsock::Winsock() :
	m_running(false) ,
	m_window(NULL) ,
	m_hwnd(0) ,
	m_success(false) ,
	m_msg(0) ,
	m_read_list("read") ,
	m_write_list("write") ,
	m_exception_list("exception") ,
	m_timer_id(1U)
{
}

std::string GNet::Winsock::id() const
{
	return m_id ;
}

bool GNet::Winsock::init()
{
	HINSTANCE hinstance = GGui::ApplicationInstance::hinstance() ;
	m_window = new WinsockWindow( *this , hinstance ) ;
	m_hwnd = m_window->handle() ;
	m_msg = GGui::Cracker::wm_winsock() ;
	m_timer_id = 1U ;
	if( m_hwnd == 0 )
	{
		G_WARNING( "GNet::Winsock::init: cannot create hidden window" ) ;
		return false ;
	}
	return attach( m_msg , m_timer_id ) ;
}

bool GNet::Winsock::attach( unsigned msg , unsigned int timer_id )
{
	WSADATA info ;
	WORD version = MAKEWORD( 2 , 2 ) ;
	int rc = ::WSAStartup( version , &info ) ;
	if( rc != 0 )
	{
		m_reason = "winsock startup failure" ;
		return false ;
	}

	if( LOBYTE( info.wVersion ) != 2 ||
		HIBYTE( info.wVersion ) != 2 )
	{
		m_reason = "incompatible winsock version" ;
		::WSACleanup() ;
		return false ;
	}

	m_id = info.szDescription ;
	G_DEBUG( "GNet::Winsock::attach: winsock \"" << G::Str::printable(m_id) << "\"" ) ;
	m_success = true ;
	return true ;
}

std::string GNet::Winsock::reason() const
{
	return m_reason ;
}

GNet::Winsock::~Winsock()
{
	bool strict = false ; // leave winsock library initialised
	if( m_success && strict )
		::WSACleanup() ;
	delete m_window ;
}

void GNet::Winsock::addRead( Descriptor fd , EventHandler & handler )
{
	m_read_list.add( fd , &handler ) ;
	update( fd ) ;
}

void GNet::Winsock::addWrite( Descriptor fd , EventHandler & handler )
{
	m_write_list.add( fd , &handler ) ;
	update( fd ) ;
}

void GNet::Winsock::addException( Descriptor fd , EventHandler & handler )
{
	m_exception_list.add( fd , &handler ) ;
	update( fd ) ;
}

void GNet::Winsock::dropRead( Descriptor fd )
{
	m_read_list.remove( fd ) ;
	update( fd ) ;
}

void GNet::Winsock::dropWrite( Descriptor fd )
{
	m_write_list.remove( fd ) ;
	update( fd ) ;
}

void GNet::Winsock::dropException( Descriptor fd )
{
	m_exception_list.remove( fd ) ;
	update( fd ) ;
}

namespace
{
	const long READ_EVENTS = (FD_READ | FD_ACCEPT | FD_OOB) ;
	const long WRITE_EVENTS = (FD_WRITE) ; // no need for "FD_CONNECT"
	const long EXCEPTION_EVENTS = (FD_CLOSE) ;
} ;

void GNet::Winsock::update( Descriptor fd )
{
	G_ASSERT( m_success ) ;
	G_ASSERT( m_hwnd != 0 ) ;
	G_ASSERT( m_msg != 0 ) ;
	::WSAAsyncSelect( fd.fd() , m_hwnd , m_msg , desiredEvents(fd) ) ;
}

long GNet::Winsock::desiredEvents( Descriptor fd )
{
	long mask = 0 ;

	if( m_read_list.contains(fd) )
		mask |= READ_EVENTS ;

	if( m_write_list.contains(fd) )
		mask |= WRITE_EVENTS ;

	if( m_exception_list.contains(fd) )
		mask |= EXCEPTION_EVENTS ;

	return mask ;
}

GNet::EventHandler * GNet::Winsock::findHandler( EventHandlerList & list , Descriptor fd )
{
	return list.find( fd ) ;
}

void GNet::Winsock::onMessage( WPARAM wparam , LPARAM lparam )
{
	G_ASSERT( wparam == static_cast<WPARAM>(static_cast<int>(wparam)) ) ;
	int fd = static_cast<int>(wparam) ;
	int event = WSAGETSELECTEVENT( lparam ) ;
	int err = WSAGETSELECTERROR( lparam ) ;

	G_DEBUG( "GNet::Winsock::processMessage: winsock select message: "
		<< "fd=" << fd << " evt=" << event << " err=" << err ) ;

	if( event & WRITE_EVENTS ) // first in case just connect()ed
	{
		EventHandler * handler = findHandler( m_write_list , Descriptor(fd) ) ;
		if( handler )
		{
			try
			{
				handler->writeEvent();
			}
			catch( std::exception & e )
			{
				G_DEBUG( "GNet::Winsock::onMessage: write-event handler exception being passed back: " << e.what() ) ;
				handler->onException( e ) ;
			}
		}
	} 
	if( event & READ_EVENTS )
	{
		EventHandler * handler = findHandler( m_read_list , Descriptor(fd) ) ;
		if( handler )
		{
			try
			{
				handler->readEvent();
			}
			catch( std::exception & e )
			{
				G_DEBUG( "GNet::Winsock::onMessage: read-event handler exception being passed back: " << e.what() ) ;
				handler->onException( e ) ;
			}
		}
	} 
	if( event & EXCEPTION_EVENTS )
	{
		EventHandler * handler = findHandler( m_exception_list , Descriptor(fd) ) ;
		if( handler )
		{
			try
			{
				handler->exceptionEvent();
			}
			catch( std::exception & e )
			{
				G_DEBUG( "GNet::Winsock::onMessage: exn-event handler exception being passed back: " << e.what() ) ;
				handler->onException( e ) ;
			}
		}
	} 
	if( err )
	{
		// only 'network down' for FD_READ/FD_WRITE
		G_WARNING( "GNet::Winsock::processMessage: winsock select error: " << err ) ;
	}
}

void GNet::Winsock::onTimer()
{
	G_DEBUG( "GNet::Winsock::onTimer" ) ;
	::KillTimer( m_hwnd , m_timer_id ) ; // since periodic
	TimerList::instance().doTimeouts() ;
}

void GNet::Winsock::setTimeout( G::DateTime::EpochTime t , bool & )
{
	G_DEBUG( "GNet::Winsock::setTimeout: " << t ) ;
	if( t != 0U )
	{
		G::DateTime::EpochTime now = G::DateTime::now() ;
		unsigned int interval = t > now ? static_cast<unsigned int>(t-now) : 0U ;
		unsigned long ms = interval ;
		ms *= 1000UL ;
		G_DEBUG( "GNet::Winsock::setTimeout: SetTimer(): " << ms << "ms" ) ;
		::KillTimer( m_hwnd , m_timer_id ) ;
		INT_PTR rc = ::SetTimer( m_hwnd , m_timer_id , ms , NULL ) ;
		if( rc == 0 )
			throw G::Exception( "GNet::Winsock: SetTimer() failure" ) ;
		G_ASSERT( rc == m_timer_id ) ;
	}
	else
	{
		G_DEBUG( "GNet::Winsock::setTimeout: KillTimer()" ) ;
		::KillTimer( m_hwnd , m_timer_id ) ;
	}
}

std::string GNet::Winsock::run()
{
	G::Setter setter( m_running ) ;
	std::string quit_reason = GGui::Pump::run() ;
	if( m_window->wndProcException() )
		throw std::runtime_error( m_window->wndProcExceptionString() ) ;
	return quit_reason ;
}

bool GNet::Winsock::running() const
{
	return m_running ;
}

void GNet::Winsock::quit( std::string reason )
{
	GGui::Pump::quit( reason ) ;
}

