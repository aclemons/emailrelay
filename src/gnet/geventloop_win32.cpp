//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstr.h"
#include "gevent.h"
#include "gappinst.h"
#include "gpump.h"
#include "gexception.h"
#include "gtimer.h"
#include "gtest.h"
#include "gassert.h"
#include "gdebug.h"
#include "glog.h"
#include <stdexcept>
#include <vector>
#include <algorithm>

#if ! GCONFIG_HAVE_WINDOWS_CREATE_WAITABLE_TIMER_EX
HANDLE CreateWaitableTimerEx( LPSECURITY_ATTRIBUTES sec , LPCTSTR name , DWORD flags , DWORD )
{
	BOOL manual_reset = ( flags & 1 ) ? TRUE : FALSE ;
	return CreateWaitableTimer( sec , manual_reset , name ) ;
}
#endif

namespace GNet
{
	class EventLoopImp ;
}

namespace
{
	const long READ_EVENTS = (FD_READ | FD_ACCEPT | FD_OOB) ;
	const long WRITE_EVENTS = (FD_WRITE) ; // no need for "FD_CONNECT"
	const long EXCEPTION_EVENTS = (FD_CLOSE) ;
	int isActive( HANDLE h )
	{
		return WAIT_OBJECT_0 == WaitForSingleObject( h , 0 ) ? 1 : 0 ;
	}
}

class GNet::EventLoopImp : public EventLoop
{
public:
	struct Library
	{
		Library() ;
		~Library() ;
	} ;
	struct TimerObject
	{
		TimerObject() ;
		~TimerObject() ;
		HANDLE h() ;
		HANDLE m_handle ;
	} ;

public:
	EventLoopImp() ;
	virtual ~EventLoopImp() ;
	virtual std::string run() override ;
	virtual bool running() const override ;
	virtual void quit( std::string ) override ;
	virtual void quit( const G::SignalSafe & ) override ;
	virtual void disarm( ExceptionHandler * ) override ;

protected:
	virtual void addRead( Descriptor , EventHandler & , ExceptionHandler & ) override ;
	virtual void addWrite( Descriptor , EventHandler & , ExceptionHandler & ) override ;
	virtual void addOther( Descriptor , EventHandler & , ExceptionHandler & ) override ;
	virtual void dropRead( Descriptor ) override ;
	virtual void dropWrite( Descriptor ) override ;
	virtual void dropOther( Descriptor ) override ;
	virtual void setTimeout( G::EpochTime ) override ;
	virtual std::string report() const override ;

private:
	EventLoopImp( const EventLoopImp & ) ;
	void operator=( const EventLoopImp & ) ;
	void updateSocket( Descriptor ) ;
	void updateHandles() ;
	void addHandles( EventHandlerList & ) ;
	long desiredEvents( Descriptor ) ;
	void onEventHandleEvent( size_t ) ;

private:
	typedef std::vector<HANDLE> Handles ;
	typedef std::vector<SOCKET> Sockets ;
	bool m_running ;
	Library m_library ;
	TimerObject m_timer_object ;
	std::string m_reason ;
	EventHandlerList m_read_list ;
	EventHandlerList m_write_list ;
	EventHandlerList m_other_list ;
	std::vector<int> m_active ;
	Handles m_handles ;
	Sockets m_sockets ;
} ;

// ===

GNet::EventLoop * GNet::EventLoop::create()
{
	return new EventLoopImp ;
}

// ===

GNet::EventLoopImp::EventLoopImp() :
	m_running(false) ,
	m_read_list("read") ,
	m_write_list("write") ,
	m_other_list("other")
{
}

GNet::EventLoopImp::~EventLoopImp()
{
}

std::string GNet::EventLoopImp::run()
{
	EventLoop::Running running( m_running ) ;
	updateHandles() ;
	std::string quit_reason ;
	for(;;)
	{
		DWORD handles_n = static_cast<DWORD>(m_handles.size()) ;
		G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::run: wait: " << handles_n << " handles" ) ;
		DWORD rc = ::MsgWaitForMultipleObjectsEx( handles_n , &m_handles[0] , INFINITE , QS_ALLINPUT , 0 ) ;
		bool updated = false ;
		{
			EventHandlerList::Lock lock_read( m_read_list , &updated ) ;
			EventHandlerList::Lock lock_write( m_write_list , &updated ) ;
			EventHandlerList::Lock lock_other( m_other_list , &updated ) ;

			if( rc == WAIT_OBJECT_0 ) // timer
			{
				G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::run: event: timeout" ) ;
				TimerList::instance().doTimeouts() ;
			}
			else if( rc > WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) ) // socket etc event
			{
				G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::run: event: socket etc" ) ;
				size_t index = static_cast<size_t>(rc-WAIT_OBJECT_0) ;
				std::transform( m_handles.begin()+index , m_handles.end() , m_active.begin()+index , isActive ) ;
				for( ; index < handles_n ; index++ )
				{
					if( m_active[index] )
						onEventHandleEvent( index ) ;
				}
			}
			else if( rc == (WAIT_OBJECT_0+handles_n) ) // window message
			{
				G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::run: event: window message" ) ;
				std::pair<bool,std::string> quit = GGui::Pump::runToEmpty() ;
				if( quit.first )
				{
					G_DEBUG( "GNet::EventLoopImp::run: quit" ) ;
					quit_reason = quit.second ;
					break ;
				}
			}
			else if( rc == WAIT_TIMEOUT )
			{
				; // never gets here
			}
			else
			{
				DWORD e = GetLastError() ;
				throw Error( "msg-wait-for-multiple-objects failed" , G::Str::fromUInt(static_cast<unsigned int>(e)) ) ;
			}
		}
		if( updated )
			updateHandles() ;
	}
	return quit_reason ;
}

void GNet::EventLoopImp::onEventHandleEvent( size_t handle_index )
{
	Descriptor fdd( m_sockets.at(handle_index) , m_handles.at(handle_index) ) ;
	if( fdd.fd() == INVALID_SOCKET ) // future event
	{
		G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::onEventHandleEvent: event on non-socket handle" ) ;
		GNet::EventHandlerList::Iterator p = m_read_list.find( fdd ) ;
		if( p != m_read_list.end() ) // the FutureEvent may have disappeared and invalidated the handle
		{
			::ResetEvent( fdd.h() ) ; // manual-reset event-object, similar to a socket handle
			p.raiseEvent( &EventHandler::readEvent ) ;
		}
	}
	else
	{
		WSANETWORKEVENTS events_info ;
		int rc = ::WSAEnumNetworkEvents( fdd.fd() , fdd.h() , &events_info ) ;
		if( rc != 0 )
			throw Error( "wsa-enum-network-events failed" ) ;

		long event = events_info.lNetworkEvents ;
		G_DEBUG_GROUP( "event" ,
			"GNet::EventLoopImp::onEventHandleEvent: network events: " << fdd << ": " << std::hex << event ) ;

		if( event & WRITE_EVENTS )
		{
			m_write_list.find(fdd).raiseEvent( &EventHandler::writeEvent ) ;
		}
		if( event & READ_EVENTS )
		{
			m_read_list.find(fdd).raiseEvent( &EventHandler::readEvent ) ;
		}
		if( event & EXCEPTION_EVENTS )
		{
			int e = events_info.iErrorCode[FD_CLOSE_BIT] ; G_ASSERT( EXCEPTION_EVENTS == FD_CLOSE ) ;
			G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::onEventHandleEvent: fd-close reason: " << e ) ;
			EventHandler::Reason reason = EventHandler::reason_other ;
			if( e == 0 ) reason = EventHandler::reason_closed ;
			if( e == WSAENETDOWN ) reason = EventHandler::reason_down ;
			if( e == WSAECONNRESET ) reason = EventHandler::reason_reset ;
			if( e == WSAECONNABORTED ) reason = EventHandler::reason_abort ;
			m_other_list.find(fdd).raiseEvent( &EventHandler::otherEvent , reason ) ;
		}
	}
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , ExceptionHandler & eh )
{
	m_read_list.add( fdd , &handler , &eh ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , ExceptionHandler & eh )
{
	m_write_list.add( fdd , &handler , &eh ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fdd , EventHandler & handler , ExceptionHandler & eh )
{
	m_other_list.add( fdd , &handler , &eh ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::dropRead( Descriptor fdd )
{
	m_read_list.remove( fdd ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::dropWrite( Descriptor fdd )
{
	m_write_list.remove( fdd ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::dropOther( Descriptor fdd )
{
	m_other_list.remove( fdd ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::updateSocket( Descriptor fdd )
{
	G_ASSERT( fdd.h() != 0 ) ;
	if( fdd.fd() != INVALID_SOCKET ) // see GNet::FutureEvent
	{
		int rc = ::WSAEventSelect( fdd.fd() , fdd.h() , desiredEvents(fdd) ) ;
		if( rc != 0 )
			throw Error( "wsa-event-select failed" ) ;
	}
}

void GNet::EventLoopImp::updateHandles()
{
	m_handles.clear() ;
	m_sockets.clear() ;
	m_handles.push_back( m_timer_object.h() ) ;
	m_sockets.push_back( 0 ) ;
	addHandles( m_read_list ) ;
	addHandles( m_write_list ) ;
	addHandles( m_other_list ) ;
	m_active.resize( m_handles.size() ) ;
}

void GNet::EventLoopImp::addHandles( EventHandlerList & list )
{
	G_ASSERT( m_handles.size() >= 1U ) ;
	G_ASSERT( m_handles.size() == m_sockets.size() ) ;
	typedef std::pair<Handles::iterator,Handles::iterator> Range ;
	for( EventHandlerList::Iterator p = list.begin() ; p != list.end() ; ++p )
	{
		// we need to avoid duplicates so populate as a sorted list
		HANDLE h = p.fd().h() ;
		SOCKET s = p.fd().fd() ;
		Range range = std::equal_range( m_handles.begin()+1 , m_handles.end() , h ) ;
		if( range.first == range.second )
		{
			G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::addHandles: wait: " << Descriptor(s,h) ) ;
			Handles::iterator h_pos = m_handles.insert( range.first , h ) ;
			Sockets::iterator s_pos = m_sockets.begin() + std::distance(m_handles.begin(),h_pos) ;
			m_sockets.insert( s_pos , s ) ;
		}
	}
}

long GNet::EventLoopImp::desiredEvents( Descriptor fdd )
{
	long mask = 0 ;
	const bool read = m_read_list.contains( fdd ) ;
	const bool write = m_write_list.contains( fdd ) ;
	const bool other = m_other_list.contains( fdd ) ;
	G_DEBUG_GROUP( "event" , "GNet::EventLoopImp::updateSocket: socket events: " << fdd << ": r=" << read << " w=" << write << " o=" << other ) ;
	if( read ) mask |= READ_EVENTS ;
	if( write ) mask |= WRITE_EVENTS ;
	if( other ) mask |= EXCEPTION_EVENTS ;
	return mask ;
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p )
{
	m_read_list.disarm( p ) ;
	m_write_list.disarm( p ) ;
	m_other_list.disarm( p ) ;
}

namespace
{
	struct LargeInteger
	{
		LargeInteger( time_t s , unsigned int us )
		{
			// time_t is signed and possibly 64 bits, but s is positive and not huge
			ULARGE_INTEGER u ;
			u.QuadPart = static_cast<ULONGLONG>( s ) ;
			if( u.HighPart ) u.HighPart = 0 , u.LowPart = 0xffffffff ;
			u.QuadPart = UInt32x32To64( u.LowPart , 10000000U ) ; // s->100ns
			u.QuadPart += ( us * 10U ) ; // us->100ns
			n.QuadPart = static_cast<LONGLONG>(u.QuadPart) ;
		}
		void negate()
		{
			n.QuadPart = -n.QuadPart ;
		}
		LARGE_INTEGER n ;
	} ;
}

void GNet::EventLoopImp::setTimeout( G::EpochTime utc_time )
{
	if( utc_time.s )
	{
		G::EpochTime now = G::DateTime::now() ;
		G::EpochTime interval = utc_time > now ? (utc_time-now) : G::EpochTime(0U) ;

		LargeInteger due_time( interval.s , interval.us ) ;
		due_time.negate() ;

		INT_PTR rc = ::SetWaitableTimer( m_timer_object.h() , &due_time.n , 0 , NULL , NULL , FALSE ) ;
		if( rc == 0 )
			throw G::Exception( "SetWaitableTimer() failure" ) ;
	}
	else
	{
		::CancelWaitableTimer( m_timer_object.h() ) ;
	}
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
}

void GNet::EventLoopImp::quit( std::string reason )
{
	GGui::Pump::quit( reason ) ;
}

void GNet::EventLoopImp::quit( const G::SignalSafe & )
{
	// not implemented
}

std::string GNet::EventLoopImp::report() const
{
	return std::string() ;
}

// ==

GNet::EventLoopImp::Library::Library()
{
	typedef GNet::EventLoopImp::Error Error ;
	WSADATA info ;
	WORD version = MAKEWORD( 2 , 2 ) ;
	int rc = ::WSAStartup( version , &info ) ;
	if( rc != 0 )
	{
		throw Error( "winsock startup failure" ) ;
	}
	if( LOBYTE(info.wVersion) != 2 || HIBYTE(info.wVersion) != 2 )
	{
		::WSACleanup() ;
		throw Error( "incompatible winsock version" ) ;
	}
}

GNet::EventLoopImp::Library::~Library()
{
	// ::WSACleanup() not
}

// ==

GNet::EventLoopImp::TimerObject::TimerObject()
{
	typedef GNet::EventLoopImp::Error Error ;
	m_handle = ::CreateWaitableTimerEx( NULL , NULL , 0 , SYNCHRONIZE | TIMER_MODIFY_STATE ) ;
	if( m_handle == NULL )
		throw Error( "cannot create timer object" ) ;
}

GNet::EventLoopImp::TimerObject::~TimerObject()
{
	::CloseHandle( m_handle ) ;
}

HANDLE GNet::EventLoopImp::TimerObject::h()
{
	return m_handle ;
}

/// \file geventloop_win32.cpp
