//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gtimerlist.h"
#include "gtest.h"
#include "gassert.h"
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
		return WAIT_OBJECT_0 == WaitForSingleObject( h , /*timeout-ms*/0 ) ? 1 : 0 ;
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

public:
	EventLoopImp() ;
	virtual ~EventLoopImp() ;
	virtual std::string run() override ;
	virtual bool running() const override ;
	virtual void quit( const std::string & ) override ;
	virtual void quit( const G::SignalSafe & ) override ;
	virtual void disarm( ExceptionHandler * ) override ;

private:
	virtual void addRead( Descriptor , EventHandler & , ExceptionSink ) override ;
	virtual void addWrite( Descriptor , EventHandler & , ExceptionSink ) override ;
	virtual void addOther( Descriptor , EventHandler & , ExceptionSink ) override ;
	virtual void dropRead( Descriptor ) override ;
	virtual void dropWrite( Descriptor ) override ;
	virtual void dropOther( Descriptor ) override ;
	virtual std::string report() const override ;

private:
	typedef std::vector<HANDLE> Handles ;
	typedef std::vector<SOCKET> Sockets ;
	EventLoopImp( const EventLoopImp & ) g__eq_delete ;
	void operator=( const EventLoopImp & ) g__eq_delete ;
	DWORD interval() ;
	void updateSocket( Descriptor ) ;
	void updateHandles() ;
	void loadHandles( Handles & , Sockets & ) ;
	static void getHandles( EventHandlerList & , Handles & , Sockets & ) ;
	long desiredEvents( Descriptor ) ;
	void onEventHandleEvent( size_t ) ;
	void checkOnAdd( Descriptor ) ;
	static void checkForOverflow( size_t ) ;

private:
	bool m_running ;
	Library m_library ;
	std::string m_reason ;
	EventHandlerList m_read_list ;
	EventHandlerList m_write_list ;
	EventHandlerList m_other_list ;
	std::vector<int> m_active ;
	Handles m_handles ;
	Sockets m_sockets ;
	Handles m_count_handles ;
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
		HANDLE * handles_p = handles_n ? &m_handles[0] : nullptr ;

		checkForOverflow( handles_n ) ;

		DWORD rc = ::MsgWaitForMultipleObjectsEx( handles_n , handles_p , interval() , QS_ALLINPUT , 0 ) ;
		bool updated = false ;
		{
			EventHandlerList::Lock lock_read( m_read_list , &updated ) ;
			EventHandlerList::Lock lock_write( m_write_list , &updated ) ;
			EventHandlerList::Lock lock_other( m_other_list , &updated ) ;

			if( rc == WAIT_TIMEOUT )
			{
				TimerList::instance().doTimeouts() ;
			}
			else if( rc >= WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) ) // socket etc event
			{
				// rc indicates left-most event, but we want to process all events in the
				// current iteration since leaving events for the next iteration leads
				// to starvation, so check forwards from the left-most using isActive()
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
				std::pair<bool,std::string> quit = GGui::Pump::runToEmpty() ;
				if( quit.first )
				{
					G_DEBUG( "GNet::EventLoopImp::run: quit" ) ;
					quit_reason = quit.second ;
					break ;
				}
			}
			else
			{
				DWORD e = GetLastError() ;
				throw Error( "msg-wait-for-multiple-objects failed" ,
					G::Str::fromUInt(static_cast<unsigned int>(handles_n)) ,
					G::Str::fromUInt(static_cast<unsigned int>(e)) ) ;
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
			EventHandler::Reason reason = EventHandler::Reason::other ;
			if( e == 0 ) reason = EventHandler::Reason::closed ;
			if( e == WSAENETDOWN ) reason = EventHandler::Reason::down ;
			if( e == WSAECONNRESET ) reason = EventHandler::Reason::reset ;
			if( e == WSAECONNABORTED ) reason = EventHandler::Reason::abort ;
			m_other_list.find(fdd).raiseEvent( &EventHandler::otherEvent , reason ) ;
		}
	}
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	checkOnAdd( fdd ) ;
	m_read_list.add( fdd , &handler , es ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	checkOnAdd( fdd ) ;
	m_write_list.add( fdd , &handler , es ) ;
	updateSocket( fdd ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	checkOnAdd( fdd ) ;
	m_other_list.add( fdd , &handler , es ) ;
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
	loadHandles( m_handles , m_sockets ) ;
	m_active.resize( m_handles.size() ) ;
}

void GNet::EventLoopImp::loadHandles( Handles & handles , Sockets & sockets )
{
	handles.clear() ;
	sockets.clear() ;
	getHandles( m_read_list , handles , sockets ) ;
	getHandles( m_write_list , handles , sockets ) ;
	getHandles( m_other_list , handles , sockets ) ;
}

void GNet::EventLoopImp::getHandles( EventHandlerList & list , Handles & handles , Sockets & sockets )
{
	G_ASSERT( handles.size() == sockets.size() ) ;
	typedef std::pair<Handles::iterator,Handles::iterator> Range ;
	for( EventHandlerList::Iterator p = list.begin() ; p != list.end() ; ++p )
	{
		// we need to avoid duplicate handles so populate as a sorted list
		HANDLE h = p.fd().h() ;
		SOCKET s = p.fd().fd() ;
		Range range = std::equal_range( handles.begin() , handles.end() , h ) ;
		if( range.first == range.second )
		{
			Handles::iterator h_pos = handles.insert( range.first , h ) ;
			Sockets::iterator s_pos = sockets.begin() + std::distance(handles.begin(),h_pos) ;
			sockets.insert( s_pos , s ) ;
		}
	}
}

void GNet::EventLoopImp::checkOnAdd( Descriptor new_fdd )
{
	// check the number of handles that will be waited for in the next
	// event-loop iteration assuming no other adds or removes happen
	// before we get there

	if( !new_fdd.h() ) return ;
	m_count_handles.clear() ;
	m_read_list.getHandles( m_count_handles ) ;
	m_write_list.getHandles( m_count_handles ) ;
	m_other_list.getHandles( m_count_handles ) ;
	bool is_new = std::binary_search( m_count_handles.begin() , m_count_handles.end() , new_fdd.h() ) ;
	size_t n = m_count_handles.size() + (is_new?1U:0U) ;
	checkForOverflow( n ) ;
}

void GNet::EventLoopImp::checkForOverflow( size_t n )
{
	size_t limit = MAXIMUM_WAIT_OBJECTS ;
	if( n >= limit )
		throw EventLoop::Overflow( "too many open handles" ) ;
}

long GNet::EventLoopImp::desiredEvents( Descriptor fdd )
{
	long mask = 0 ;
	const bool read = m_read_list.contains( fdd ) ;
	const bool write = m_write_list.contains( fdd ) ;
	const bool other = m_other_list.contains( fdd ) ;
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

DWORD GNet::EventLoopImp::interval()
{
	std::pair<G::TimeInterval,bool> interval_pair = TimerList::instance().interval() ;
	if( interval_pair.second )
	{
		return INFINITE ; // 0xffffffff
	}
	else
	{
		ULARGE_INTEGER u ; // (64-bit ULONGLONG Quad and 32-bit DWORD HighPart/LowPart)
		u.QuadPart = static_cast<ULONGLONG>( interval_pair.first.s ) ;
		if( u.HighPart ) u.HighPart = 0 , u.LowPart = 0xffffffff ;
		u.QuadPart = UInt32x32To64( u.LowPart , 1000U ) ; // s->ms
		u.QuadPart += ( interval_pair.first.us / 1000U ) ; // us->ms
		DWORD ms = u.HighPart ? 0xffffffff : u.LowPart ;
		if( ms == INFINITE ) --ms ;
		return ms ;
	}
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
}

void GNet::EventLoopImp::quit( const std::string & reason )
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
	WSADATA info ;
	WORD version = MAKEWORD( 2 , 2 ) ;
	int rc = ::WSAStartup( version , &info ) ;
	if( rc != 0 )
	{
		throw EventLoopImp::Error( "winsock startup failure" ) ;
	}
	if( LOBYTE(info.wVersion) != 2 || HIBYTE(info.wVersion) != 2 )
	{
		::WSACleanup() ;
		throw EventLoopImp::Error( "incompatible winsock version" ) ;
	}
}

GNet::EventLoopImp::Library::~Library()
{
	// ::WSACleanup() not
}

/// \file geventloop_win32.cpp
