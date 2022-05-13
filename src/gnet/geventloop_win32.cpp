//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventloop_win32.cpp
///

#include "gdef.h"
#include "geventloop_win32.h"
#include "geventloophandles_win32.h"
#include "gevent.h"
#include "gpump.h"
#include "gexception.h"
#include "gtimer.h"
#include "gtimerlist.h"
#include "gscope.h"
#include "gstr.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

// ===

GNet::EventLoopImp::EventLoopImp() :
	m_running(false) ,
	m_dirty(true) ,
	m_quit(false)
{
	std::size_t threads = 0U ; // 0 => become multi-threaded automatically
	m_handles = std::make_unique<EventLoopHandles>( m_list , threads ) ;
}

GNet::EventLoopImp::~EventLoopImp()
{
}

std::string GNet::EventLoopImp::run()
{
	G::ScopeExitSetFalse running( m_running = true ) ;

	EventLoopHandles & handles = *m_handles ;
	handles.init( m_list ) ;

	std::string quit_reason ;
	m_dirty = false ;
	m_quit = false ;
	while( !m_quit )
	{
		if( handles.overflow( m_list.size() ) )
			throw Overflow( handles.help(m_list,false) ) ;

		auto rc = handles.waitForMultipleObjects( ms() ) ;

		if( rc == RcType::timeout )
		{
			TimerList::instance().doTimeouts() ;
		}
		else if( rc == RcType::event )
		{
			// rc indicates left-most event -- move it to the rhs to avoid starvation
			std::size_t list_index = handles.shuffle( m_list , rc ) ;

			ListItem & list_item = m_list[list_index] ;
			if( list_item.m_type == ListItemType::socket )
				handleSocketEvent( list_index ) ;
			else if( list_item.m_type == ListItemType::simple )
				handleSimpleEvent( list_item ) ;
			else
				handles.handleInternalEvent( list_index ) ;
		}
		else if( rc == RcType::message )
		{
			std::pair<bool,std::string> quit = GGui::Pump::runToEmpty() ;
			if( quit.first )
			{
				G_DEBUG( "GNet::EventLoopImp::run: quit" ) ;
				quit_reason = quit.second ;
				m_quit = true ;
			}
		}
		else if( rc == RcType::failed )
		{
			DWORD e = GetLastError() ;
			throw Error( "wait-for-multiple-objects failed" ,
				G::Str::fromUInt(static_cast<unsigned int>(e)) ) ;
		}

		bool updated = m_dirty ;
		if( m_dirty )
		{
			m_list.erase( std::remove_if(m_list.begin(),m_list.end(),&EventLoopImp::isInvalid) , m_list.end() ) ;
			m_dirty = false ;
		}
		handles.update( m_list , updated , rc ) ;
	}
	return quit_reason ;
}

bool GNet::EventLoopImp::isInvalid( const ListItem & item )
{
	return item.m_events == 0L ;
}

bool GNet::EventLoopImp::isValid( const ListItem & item )
{
	return item.m_events != 0L ;
}

void GNet::EventLoopImp::handleSimpleEvent( ListItem & item )
{
	ResetEvent( item.m_handle ) ; // manual-reset event-object -- see GNet::FutureEvent
	item.m_read_emitter.raiseReadEvent( Descriptor(INVALID_SOCKET,item.m_handle) ) ;
}

void GNet::EventLoopImp::handleSocketEvent( std::size_t index )
{
	ListItem * item = &m_list[index] ;

	WSANETWORKEVENTS events_info ;
	int rc = WSAEnumNetworkEvents( item->m_socket , item->m_handle , &events_info ) ;
	bool e_not_sock = false ;
	if( rc != 0 && !(e_not_sock = (WSAGetLastError()==WSAENOTSOCK)) )
		throw Error( "enum-network-events failed" ) ;

	G_ASSERT_OR_DO( !e_not_sock , throw Error("enum-network-events failed: not a socket") ) ;

	// we might do more than one raiseEvent() here and m_list can change
	// between each call, potentially invalidating our ListItem pointer --
	// however we use garbage collection and no inserts on m_list so we
	// can recover a valid pointer from the index

	long events = events_info.lNetworkEvents ;
	if( events & READ_EVENTS )
	{
		item->m_read_emitter.raiseReadEvent( item->fd() ) ;
		item = nullptr ;
	}
	if( events & WRITE_EVENTS )
	{
		item = item ? item : &m_list[index] ;
		item->m_write_emitter.raiseWriteEvent( item->fd() ) ;
		item = nullptr ;
	}
	if( events & EXCEPTION_EVENTS )
	{
		static_assert( EXCEPTION_EVENTS == FD_CLOSE , "" ) ;
		item = item ? item : &m_list[index] ;
		int e = events_info.iErrorCode[FD_CLOSE_BIT] ;
		EventHandler::Reason reason = EventHandler::Reason::other ;
		if( e == 0 ) reason = EventHandler::Reason::closed ;
		if( e == WSAENETDOWN ) reason = EventHandler::Reason::down ;
		if( e == WSAECONNRESET ) reason = EventHandler::Reason::reset ;
		if( e == WSAECONNABORTED ) reason = EventHandler::Reason::abort ;
		item->m_other_emitter.raiseOtherEvent( item->fd() , reason ) ;
	}
}

GNet::EventLoopImp::ListItem * GNet::EventLoopImp::find( Descriptor fdd )
{
	const HANDLE h = fdd.h() ;
	auto p = std::find_if( m_list.begin() , m_list.end() , [h](const ListItem & i){return i.m_handle==h;} ) ;
	return p == m_list.end() ? nullptr : &(*p) ;
}

GNet::EventLoopImp::ListItem & GNet::EventLoopImp::findOrCreate( Descriptor fdd )
{
	G_ASSERT( fdd.h() != HNULL ) ;
	const HANDLE h = fdd.h() ;
	auto p = std::find_if( m_list.begin() , m_list.end() , [h](const ListItem & i){return i.m_handle==h;} ) ;
	if( p == m_list.end() )
	{
		m_list.emplace_back( fdd ) ;
		ListItem & item = m_list.back() ;
		item.m_events = 0 ; // => newly created
		return item ;
	}
	else
	{
		return *p ;
	}
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ;
	item.m_events |= READ_EVENTS ;
	item.m_read_emitter = EventEmitter( &handler , es ) ;
	updateSocket( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ;
	item.m_events |= WRITE_EVENTS ;
	item.m_write_emitter = EventEmitter( &handler , es ) ;
	updateSocket( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fdd , EventHandler & handler , ExceptionSink es )
{
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ;
	item.m_events |= EXCEPTION_EVENTS ;
	item.m_other_emitter = EventEmitter( &handler , es ) ;
	updateSocket( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::checkForOverflow( const ListItem & item )
{
	G_ASSERT_OR_DO( m_handles != nullptr , return ) ;
	const bool is_new = item.m_events == 0L ;
	if( is_new && m_handles->overflow( m_list , &EventLoopImp::isValid ) )
	{
		m_list.pop_back() ;
		throw Overflow( m_handles->help(m_list,true) ) ;
	}
}

void GNet::EventLoopImp::dropRead( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~READ_EVENTS ;
		item->m_read_emitter.reset() ;
		updateSocket( fdd , item->m_events , std::nothrow ) ;
		m_dirty |= ( item->m_events == 0L ) ;
	}
}

void GNet::EventLoopImp::dropWrite( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~WRITE_EVENTS ;
		item->m_write_emitter.reset() ;
		updateSocket( fdd , item->m_events , std::nothrow ) ;
		m_dirty |= ( item->m_events == 0L ) ;
	}
}

void GNet::EventLoopImp::dropOther( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~EXCEPTION_EVENTS ;
		item->m_other_emitter.reset() ;
		updateSocket( fdd , item->m_events , std::nothrow ) ;
		m_dirty |= ( item->m_events == 0L ) ;
	}
}

void GNet::EventLoopImp::updateSocket( Descriptor fdd , long events )
{
	G_ASSERT( fdd.h() != 0 ) ;
	if( fdd.fd() != INVALID_SOCKET ) // see GNet::FutureEvent
	{
		int rc = WSAEventSelect( fdd.fd() , fdd.h() , events ) ;
		if( rc != 0 )
			throw Error( "wsa-event-select failed" ) ;
	}
}

bool GNet::EventLoopImp::updateSocket( Descriptor fdd , long events , std::nothrow_t ) noexcept
{
	int rc = 0 ;
	if( fdd.fd() != INVALID_SOCKET )
		rc = WSAEventSelect( fdd.fd() , fdd.h() , events ) ;
	return rc == 0 ;
}

void GNet::EventLoopImp::disarm( ExceptionHandler * p ) noexcept
{
	for( auto & item : m_list )
	{
		if( item.m_read_emitter.es().eh() == p )
			item.m_read_emitter.disarm() ;
		if( item.m_write_emitter.es().eh() == p )
			item.m_write_emitter.disarm() ;
		if( item.m_other_emitter.es().eh() == p )
			item.m_other_emitter.disarm() ;
	}
}

DWORD GNet::EventLoopImp::ms()
{
	auto pair = TimerList::instance().interval() ;
	if( pair.second )
		return INFINITE ;
	else if( pair.first.s() == 0 && pair.first.us() == 0U )
		return 0 ;
	else
		return std::max( DWORD(1) , ms(pair.first.s(),pair.first.us()) ) ;
}

DWORD GNet::EventLoopImp::ms( s_type s , us_type us ) noexcept
{
	constexpr DWORD dword_max = 0xffffffff ;
	constexpr DWORD dword_max_1 = dword_max - 1 ;
	static_assert( INFINITE == dword_max , "" ) ;
	constexpr s_type s_max = static_cast<s_type>( dword_max/1000U - 1U ) ;
	return
		s >= s_max ?
			dword_max_1 :
			static_cast<DWORD>( (s*1000U) + ((us+999U)/1000U) ) ;
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
	// (quit without processing window messages)
	m_quit = true ;
}

// ==

GNet::EventLoopImp::Library::Library()
{
	WSADATA info ;
	WORD version = MAKEWORD( 2 , 2 ) ;
	int rc = WSAStartup( version , &info ) ;
	if( rc != 0 )
	{
		throw EventLoopImp::Error( "winsock startup failure" ) ;
	}
	if( LOBYTE(info.wVersion) != 2 || HIBYTE(info.wVersion) != 2 )
	{
		WSACleanup() ;
		throw EventLoopImp::Error( "incompatible winsock version" ) ;
	}
}

GNet::EventLoopImp::Library::~Library()
{
	// WSACleanup() not
}

