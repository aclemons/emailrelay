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
/// \file geventloop_win32.cpp
///

#include "gdef.h"
#include "geventloop.h"
#include "geventloophandles.h"
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
#include <memory>
#include <new>

namespace GNet
{
	class EventLoopImp ;
}

class GNet::EventLoopImp : public EventLoop
{
public:
	EventLoopImp() ;
		// Default constructor.

	virtual ~EventLoopImp() ;
		// Destructor.

private: // overrides
	std::string run() override ;
	bool running() const override ;
	void quit( const std::string & ) override ;
	void quit( const G::SignalSafe & ) override ;
	void disarm( ExceptionHandler * ) noexcept override ;
	void addRead( Descriptor , EventHandler & , EventState ) override ;
	void addWrite( Descriptor , EventHandler & , EventState ) override ;
	void addOther( Descriptor , EventHandler & , EventState ) override ;
	void dropRead( Descriptor ) noexcept override ;
	void dropWrite( Descriptor ) noexcept override ;
	void dropOther( Descriptor ) noexcept override ;
	void drop( Descriptor ) noexcept override ;

public:
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	EventLoopImp & operator=( const EventLoopImp & ) = delete ;
	EventLoopImp & operator=( EventLoopImp && ) = delete ;

private:
	void runOnce() ;

private:
	using Rc = EventLoopHandles::Rc ;
	using RcType = EventLoopHandles::RcType ;
	static constexpr long READ_EVENTS = (FD_READ | FD_ACCEPT | FD_OOB) ;
	static constexpr long WRITE_EVENTS = (FD_WRITE) ;
	static constexpr long EXCEPTION_EVENTS = (FD_CLOSE | FD_CONNECT) ;
	using s_type = G::TimeInterval::s_type ;
	using us_type = G::TimeInterval::us_type ;
	struct Library
	{
		Library() ;
		~Library() ;
		Library( const Library & ) = delete ;
		Library( Library && ) = delete ;
		Library & operator=( const Library & ) = delete ;
		Library & operator=( Library && ) = delete ;
	} ;

public:
	enum class ListItemType // A type enumeration for the list of event sources.
	{
		socket ,
		simple
	} ;
	struct ListItem /// A structure holding an event handle and its event handlers.
	{
		explicit ListItem( Descriptor fdd ) :
			m_type(fdd.fd()==INVALID_SOCKET?ListItemType::simple:ListItemType::socket) ,
			m_socket(fdd.fd()) ,
			m_handle(fdd.h())
		{
		}
		ListItemType m_type {ListItemType::socket} ;
		SOCKET m_socket {INVALID_SOCKET} ;
		HANDLE m_handle {HNULL} ;
		long m_events {0L} ; // 0 => new or garbage
		EventState m_es {EventState::Private(),nullptr,nullptr} ;
		EventHandler * m_handler {nullptr} ;
	} ;
	using List = std::vector<ListItem> ;

private:
	ListItem * find( Descriptor ) ;
	ListItem & findOrCreate( Descriptor ) ;
	void fdupdate( Descriptor , long ) ;
	bool fdupdate( Descriptor , long , std::nothrow_t ) noexcept ;
	void handleSimpleEvent( ListItem & ) ;
	void handleSocketEvent( std::size_t ) ;
	void checkForOverflow( const ListItem & ) ;
	bool collectGarbage() ;
	DWORD ms() ;
	static DWORD ms( s_type , us_type ) noexcept ;
	static bool isValid( const ListItem & ) ;
	static bool isInvalid( const ListItem & ) ;

private:
	Library m_library ;
	List m_list ;
	std::unique_ptr<EventLoopHandles> m_handles ;
	bool m_running ;
	bool m_dirty ;
	bool m_quit ;
	std::string m_quit_reason ;
	EventState m_es_current {EventState::Private(),nullptr,nullptr} ;
} ;

std::unique_ptr<GNet::EventLoop> GNet::EventLoop::create()
{
	return std::make_unique<EventLoopImp>() ;
}

// ===

GNet::EventLoopImp::EventLoopImp() :
	m_running(false) ,
	m_dirty(false) ,
	m_quit(false)
{
	m_handles = std::make_unique<EventLoopHandles>() ;
}

GNet::EventLoopImp::~EventLoopImp()
{
}

void GNet::EventLoopImp::disarm( ExceptionHandler * eh ) noexcept
{
	if( m_es_current.eh() == eh )
		m_es_current.disarm() ;

	for( auto & item : m_list )
	{
		if( item.m_es.eh() == eh )
			item.m_es.disarm() ;
	}
}

bool GNet::EventLoopImp::running() const
{
	return m_running ;
}

std::string GNet::EventLoopImp::run()
{
	collectGarbage() ; // in case re-run() after an exception

	std::size_t i = 0U ;
	List * list_p = &m_list ;
	m_handles->update( m_list.size() , [list_p,&i](){return (*list_p)[i++].m_handle;} ) ;

	G::ScopeExitSetFalse running( m_running = true ) ;
	m_dirty = false ;
	m_quit = false ;
	while( !m_quit )
	{
		runOnce() ;
	}
	std::string quit_reason = m_quit_reason ;
	m_quit_reason.clear() ;
	m_quit = false ;
	return quit_reason ;
}

void GNet::EventLoopImp::runOnce()
{
	G_ASSERT( std::find_if(m_list.begin(),m_list.end(),[](const ListItem & i){return i.m_handle==HNULL;}) == m_list.end() ) ;
	G_ASSERT( std::find_if(m_list.begin(),m_list.end(),[](const ListItem & i){return i.m_events==0L;}) == m_list.end() ) ;
	G_ASSERT( !m_dirty ) ;

	EventLoopHandles & handles = *m_handles ;

	auto rc = handles.wait( ms() ) ;
	if( rc == RcType::overflow )
	{
		throw Overflow() ;
	}
	else if( rc == RcType::timeout )
	{
		TimerList::instance().doTimeouts() ;
	}
	else if( rc == RcType::event )
	{
		ListItem & list_item = m_list[rc.index()] ;
		if( list_item.m_type == ListItemType::socket )
			handleSocketEvent( rc.index() ) ;
		else
			handleSimpleEvent( list_item ) ;
	}
	else if( rc == RcType::message )
	{
		std::pair<bool,std::string> quit = GGui::Pump::runToEmpty() ;
		if( quit.first )
		{
			G_DEBUG( "GNet::EventLoopImp::run: quit" ) ;
			m_quit_reason = quit.second ;
			m_quit = true ;
		}
	}
	else if( rc == RcType::failed )
	{
		throw Error( "wait failed (" + std::to_string(rc.m_error) + ")" ) ;
	}
	else // rc == RcType::other
	{
		; // no-op
	}

	// remove from m_list where logically deleted
	bool was_dirty = collectGarbage() ;

	// let the handles object see the new, garbage-collected list
	std::size_t i = 0U ;
	List * list_p = &m_list ;
	handles.update( m_list.size() , [list_p,&i](){return (*list_p)[i++].m_handle;} , was_dirty ) ;
}

bool GNet::EventLoopImp::collectGarbage()
{
	bool was_dirty = m_dirty ; // ie. m_list updated by adds and/or drops during event handling
	if( m_dirty )
	{
		m_list.erase( std::remove_if(m_list.begin(),m_list.end(),&EventLoopImp::isInvalid) , m_list.end() ) ;
		m_dirty = false ;
	}
	return was_dirty ;
}

DWORD GNet::EventLoopImp::ms()
{
	if( TimerList::ptr() )
	{
		auto pair = TimerList::instance().interval() ;
		if( pair.second )
			return INFINITE ;
		else if( pair.first.s() == 0 && pair.first.us() == 0U )
			return 0 ;
		else
			return std::max( DWORD(1) , ms(pair.first.s(),pair.first.us()) ) ;
	}
	else
	{
		return INFINITE ;
	}
}

DWORD GNet::EventLoopImp::ms( s_type s , us_type us ) noexcept
{
	constexpr DWORD dword_max = 0xffffffff ;
	constexpr DWORD dword_max_1 = dword_max - 1 ;
	static_assert( INFINITE == dword_max , "" ) ;
	constexpr auto s_max = static_cast<s_type>( dword_max/1000U - 1U ) ;
	return
		s >= s_max ?
			dword_max_1 :
			static_cast<DWORD>( (s*1000U) + ((us+999U)/1000U) ) ;
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
		p = m_list.begin() + m_list.size() - 1U ;
		(*p).m_events = 0 ; // => newly created
	}
	return *p ;
}

void GNet::EventLoopImp::addRead( Descriptor fdd , EventHandler & handler , EventState es )
{
	G_ASSERT( fdd.h() != HNULL ) ;
	handler.setDescriptor( fdd ) ; // see EventHandler::dtor
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ; // dirty if new Item created
	item.m_events |= READ_EVENTS ;
	item.m_handler = &handler ;
	item.m_es = es ;
	fdupdate( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::addWrite( Descriptor fdd , EventHandler & handler , EventState es )
{
	G_ASSERT( fdd.h() != HNULL ) ;
	handler.setDescriptor( fdd ) ; // see EventHandler::dtor
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ;
	item.m_events |= WRITE_EVENTS ;
	item.m_handler = &handler ;
	item.m_es = es ;
	fdupdate( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::addOther( Descriptor fdd , EventHandler & handler , EventState es )
{
	G_ASSERT( fdd.h() != HNULL ) ;
	handler.setDescriptor( fdd ) ; // see EventHandler::dtor
	ListItem & item = findOrCreate( fdd ) ;
	checkForOverflow( item ) ;
	m_dirty |= ( item.m_events == 0L ) ;
	item.m_events |= EXCEPTION_EVENTS ;
	item.m_handler = &handler ;
	item.m_es = es ;
	fdupdate( fdd , item.m_events ) ;
}

void GNet::EventLoopImp::dropRead( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~READ_EVENTS ;
		fdupdate( fdd , item->m_events , std::nothrow ) ;
		if( item->m_events == 0L ) // logically deleted
		{
			m_dirty = true ;
			m_handles->onClose( fdd.h() ) ;
		}
	}
}

void GNet::EventLoopImp::dropWrite( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~WRITE_EVENTS ;
		fdupdate( fdd , item->m_events , std::nothrow ) ;
		if( item->m_events == 0L )
		{
			m_dirty = true ;
			m_handles->onClose( fdd.h() ) ;
		}
	}
}

void GNet::EventLoopImp::dropOther( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events &= ~EXCEPTION_EVENTS ;
		fdupdate( fdd , item->m_events , std::nothrow ) ;
		if( item->m_events == 0L )
		{
			m_dirty = true ;
			m_handles->onClose( fdd.h() ) ;
		}
	}
}

void GNet::EventLoopImp::drop( Descriptor fdd ) noexcept
{
	ListItem * item = find( fdd ) ;
	if( item )
	{
		item->m_events = 0U ;
		fdupdate( fdd , item->m_events , std::nothrow ) ;
		item->m_handler = nullptr ;
		m_dirty = true ;
		m_handles->onClose( fdd.h() ) ;
	}
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
	m_es_current = item.m_es ; // see disarm()
	EventEmitter::raiseReadEvent( item.m_handler , m_es_current ) ;
}

void GNet::EventLoopImp::handleSocketEvent( std::size_t index )
{
	ListItem * item = &m_list[index] ;

	WSANETWORKEVENTS events_info ;
	bool e_not_sock = false ;
	int rc = WSAEnumNetworkEvents( item->m_socket , item->m_handle , &events_info ) ;
	if( rc != 0 )
		e_not_sock = WSAGetLastError() == WSAENOTSOCK ;
	if( rc != 0 && !e_not_sock )
		throw Error( "enum-network-events failed" ) ;
	if( e_not_sock )
		throw Error( "enum-network-events failed: not a socket" ) ;

	// we might do more than one raiseEvent() here and m_list can change
	// between each call, potentially invalidating our ListItem pointer --
	// however we use garbage collection and no inserts on m_list so we
	// can recover a valid pointer from the index

	long events = events_info.lNetworkEvents ;
	if( events & READ_EVENTS )
	{
		m_es_current = item->m_es ;
		EventEmitter::raiseReadEvent( item->m_handler , m_es_current ) ;
		item = nullptr ;
	}
	if( events & WRITE_EVENTS )
	{
		item = item ? item : &m_list[index] ;
		m_es_current = item->m_es ;
		EventEmitter::raiseWriteEvent( item->m_handler , m_es_current ) ;
		item = nullptr ;
	}
	if( events & EXCEPTION_EVENTS )
	{
		static_assert( EXCEPTION_EVENTS == (FD_CLOSE|FD_CONNECT) , "" ) ;
		item = item ? item : &m_list[index] ;
		if( events_info.lNetworkEvents & FD_CONNECT )
		{
			int e = events_info.iErrorCode[FD_CONNECT_BIT] ;
			if( e )
			{
				m_es_current = item->m_es ;
				EventEmitter::raiseOtherEvent( item->m_handler , m_es_current , EventHandler::Reason::failed ) ;
			}
		}
		else
		{
			int e = events_info.iErrorCode[FD_CLOSE_BIT] ;
			EventHandler::Reason reason = EventHandler::Reason::other ;
			if( e == 0 ) reason = EventHandler::Reason::closed ;
			if( e == WSAENETDOWN ) reason = EventHandler::Reason::down ;
			if( e == WSAECONNRESET ) reason = EventHandler::Reason::reset ;
			if( e == WSAECONNABORTED ) reason = EventHandler::Reason::abort ;
			m_es_current = item->m_es ;
			EventEmitter::raiseOtherEvent( item->m_handler , m_es_current , reason ) ;
		}
	}
}

void GNet::EventLoopImp::checkForOverflow( const ListItem & item )
{
	G_ASSERT( m_handles != nullptr ) ;
	const bool is_new = item.m_events == 0L ;

	List * list_p = &m_list ;
	std::function<std::size_t()> count_valid_fn = [list_p](){
		std::size_t n = 0 ;
		for( auto p = list_p->cbegin() ; p != list_p->cend() ; ++p )
		{
			if( std::next(p) == list_p->cend() || isValid(*p) ) // always count the one we've just added
				n++ ;
		}
		return n ;
	} ;

	if( is_new && m_handles->overflow( m_list.size() , count_valid_fn ) )
	{
		m_list.pop_back() ;
		throw Overflow() ;
	}
}

void GNet::EventLoopImp::fdupdate( Descriptor fdd , long events )
{
	G_ASSERT( fdd.h() != 0 ) ;
	if( fdd.fd() != INVALID_SOCKET ) // see GNet::FutureEvent
	{
		int rc = WSAEventSelect( fdd.fd() , fdd.h() , events ) ;
		if( rc != 0 )
			throw Error( "wsa-event-select failed" ) ;
	}
}

bool GNet::EventLoopImp::fdupdate( Descriptor fdd , long events , std::nothrow_t ) noexcept
{
	int rc = 0 ;
	if( fdd.fd() != INVALID_SOCKET )
		rc = WSAEventSelect( fdd.fd() , fdd.h() , events ) ;
	return rc == 0 ;
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

