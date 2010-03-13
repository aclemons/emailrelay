//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// A concrete implementation of GNet::EventLoop
///  using the WinSock system.
/// 
///  Note that WinSock events are delivered as messages to the 
///  application's main message queue, and these messages have 
///  to be passed on the the WinSock layer.
/// 
class GNet::Winsock : public GNet::EventLoop 
{
public:
	Winsock() ;
		// Default constructor. Initialise with load() (optional)
		// and then either init() or attach().

	bool load( const char * dllpath ) ;
		// Loads the given WinSock DLL. Must be the first
		// method called after construction.
		// Returns false on error.

	virtual bool init() ;
		// Override from EventLoop. Initialses the
		// WinSock library, passing it the handle
		// of an internally-created hidden window.
		// Returns false on error.
		//
		// Use either init() for an internally-created
		// window, or attach().

	bool attach( HWND hwnd , unsigned int msg , unsigned int timer_id = 0U ) ;
		// Initialises the WinSock library, passing
		// it the specified window handle and
		// message number. WinSock events are sent
		// to that window. Returns false on error.
		//
		// For simple, synchronous programs
		// the window handle may be zero.
		//
		// Use either init() or attach().
		//
		// See also onMessage() and onTimer().

	std::string reason() const ;
		// Returns the reason for initialisation
		// failure.

	std::string id() const ;
		// Returns the WinSock implementation's
		// identification string. Returns the
		// zero length string on error.

	virtual ~Winsock() ;
		// Destructor. Releases the WinSock resources
		// if this is the last Winsock object.

	void onMessage( WPARAM wparam , LPARAM lparam ) ;
		// To be called when the attach()ed window
		// receives a message with a message-id
		// equal to attach() 'msg' parameter.

	void onTimer() ;
		// To be called when the attach()ed window
		// receives a WM_TIMER message.

	virtual void run() ;
		// Override from EventLoop. Calls GGui::Pump::run().

	virtual bool running() const ;
		// Override from EventLoop. Returns true if in run().

	virtual bool quit() ;
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
	EventHandler *findHandler( EventHandlerList & list , Descriptor fd ) ;
	void update( Descriptor fd ) ;
	long desiredEvents( Descriptor fd ) ;

private:
	bool m_running ;
	GGui::WindowBase * m_window ;
	bool m_success ;
	std::string m_reason ;
	std::string m_id ;
	HWND m_hwnd ;
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
	m_ws(ws) ,
	GGui::WindowHidden(hinstance)
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
	m_success(false) ,
	m_hwnd(0) ,
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

bool GNet::Winsock::load( const char * )
{
	; // not implemented
	return true ;
}

bool GNet::Winsock::init()
{
	HINSTANCE hinstance = GGui::ApplicationInstance::hinstance() ;
	m_window = new WinsockWindow( *this , hinstance ) ;
	if( m_window->handle() == 0 )
	{
		G_WARNING( "GNet::Winsock::init: cannot create hidden window" ) ;
		return false ;
	}
	return attach( m_window->handle() , GGui::Cracker::wm_winsock() , 1U ) ;
}

bool GNet::Winsock::attach( HWND hwnd , unsigned msg , unsigned int timer_id )
{
	m_hwnd = hwnd ;
	m_msg = msg ;
	m_timer_id = timer_id ;

	WSADATA info ;
	WORD version = MAKEWORD( 1 , 1 ) ;
	int rc = ::WSAStartup( version , &info ) ;
	if( rc != 0 )
	{
		m_reason = "winsock startup failure" ;
		return false ;
	}

	if( LOBYTE( info.wVersion ) != 1 ||
		HIBYTE( info.wVersion ) != 1 )
	{
		m_reason = "incompatible winsock version" ;
		::WSACleanup() ;
		return false ;
	}

	m_id = info.szDescription ;
	G_DEBUG( "GNet::Winsock::attach: winsock \"" << m_id << "\"" ) ;
	m_success = true ;
	return true ;
}

std::string GNet::Winsock::reason() const
{
	return m_reason ;
}

GNet::Winsock::~Winsock()
{
	if( m_success )
		::WSACleanup() ;
	delete m_window ;
}

void GNet::Winsock::addRead( Descriptor fd , EventHandler &handler )
{
	m_read_list.add( fd , &handler ) ;
	update( fd ) ;
}

void GNet::Winsock::addWrite( Descriptor fd , EventHandler &handler )
{
	m_write_list.add( fd , &handler ) ;
	update( fd ) ;
}

void GNet::Winsock::addException( Descriptor fd , EventHandler &handler )
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
	::WSAAsyncSelect( fd , m_hwnd , m_msg , desiredEvents(fd) ) ;
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

GNet::EventHandler * GNet::Winsock::findHandler( EventHandlerList &list , Descriptor fd )
{
	return list.find( fd ) ;
}

void GNet::Winsock::onMessage( WPARAM wparam , LPARAM lparam )
{
	int fd = wparam ;
	int event = WSAGETSELECTEVENT( lparam ) ;
	int err = WSAGETSELECTERROR( lparam ) ;

	G_DEBUG( "GNet::Winsock::processMessage: winsock select message: "
		<< "w=" << wparam << " l=" << lparam 
		<< " fd=" << fd << " evt=" << event << " err=" << err ) ;

	if( event & READ_EVENTS )
	{
		EventHandler *handler = findHandler( m_read_list , fd ) ;
		if( handler )
		{
			try
			{
				handler->readEvent();
			}
			catch( std::exception & e ) // strategy
			{
				handler->onException( e ) ;
			}
		}
	} 
	else if( event & WRITE_EVENTS )
	{
		EventHandler *handler = findHandler( m_write_list , fd ) ;
		if( handler )
		{
			try
			{
				handler->writeEvent();
			}
			catch( std::exception & e ) // strategy
			{
				handler->onException( e ) ;
			}
		}
	} 
	else if( event & EXCEPTION_EVENTS )
	{
		EventHandler *handler = findHandler( m_exception_list , fd ) ;
		if( handler )
		{
			try
			{
				handler->exceptionEvent();
			}
			catch( std::exception & e ) // strategy
			{
				handler->onException( e ) ;
			}
		}
	} 
	else if( err )
	{
		G_DEBUG( "GNet::Winsock::processMessage: winsock select error: " << err ) ;
	}
	else
	{
		G_DEBUG( "GNet::Winsock::processMessage: unwanted winsock event: " << event ) ;
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
		unsigned int rc = ::SetTimer( m_hwnd , m_timer_id , ms , NULL ) ;
		if( rc == 0U )
			throw G::Exception( "GNet::Winsock: SetTimer() failure" ) ;
		G_ASSERT( rc == m_timer_id ) ;
	}
	else
	{
		G_DEBUG( "GNet::Winsock::setTimeout: KillTimer()" ) ;
		::KillTimer( m_hwnd , m_timer_id ) ;
	}
}

void GNet::Winsock::run()
{
	G::Setter setter( m_running ) ;
	GGui::Pump::run() ;
}

bool GNet::Winsock::running() const
{
	return m_running ;
}

bool GNet::Winsock::quit()
{
	GGui::Pump::quit() ;
	return false ;
}

