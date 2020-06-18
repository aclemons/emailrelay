//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfutureevent_win32.cpp
//

#include "gdef.h"
#include "gfutureevent.h"
#include "geventloop.h"
#include "gstr.h"
#include "glog.h"

#if ! GCONFIG_HAVE_WINDOWS_CREATE_EVENT_EX
HANDLE CreateEventEx( LPSECURITY_ATTRIBUTES sec , LPCTSTR name , DWORD flags , DWORD )
{
	BOOL manual_reset = ( flags & 1 ) ? TRUE : FALSE ;
	BOOL initial_state = ( flags & 2 ) ? TRUE : FALSE ;
	return CreateEvent( sec , manual_reset , initial_state , name ) ;
}
#endif

#ifndef CREATE_EVENT_MANUAL_RESET
#define CREATE_EVENT_MANUAL_RESET 1
#endif

class GNet::FutureEventImp : public EventHandler
{
public:
	using handle_type = FutureEvent::handle_type ;

	FutureEventImp( FutureEventHandler & handler , ExceptionSink es ) ;
		// Constructor.

	~FutureEventImp() override ;
		// Destructor.

	static bool send( handle_type , bool ) noexcept ;
		// Raises an event.

	handle_type handle() ;
		// Extracts the event-object handle.

private: // overrides
	void readEvent() override ; // GNet::EventHandler

public:
	FutureEventImp( const FutureEventImp & ) = delete ;
	FutureEventImp( FutureEventImp && ) = delete ;
	void operator=( const FutureEventImp & ) = delete ;
	void operator=( FutureEventImp && ) = delete ;

private:
	HANDLE dup() ;

private:
	struct Handle
	{
		Handle() = default ;
		~Handle() { if(h) ::CloseHandle(h) ; }
		void operator=( HANDLE h_ ) { h = h_ ; }
		bool operator==( HANDLE h_ ) const { return h == h_ ; }
		Handle( const Handle & ) = delete ;
		Handle( Handle && ) = delete ;
		void operator=( const Handle & ) = delete ;
		void operator=( Handle && ) = delete ;
		HANDLE h{0} ;
	} ;

private:
	FutureEventHandler & m_handler ;
	ExceptionSink m_es ;
	Handle m_h ;
	Handle m_h2 ;
} ;

GNet::FutureEventImp::FutureEventImp( FutureEventHandler & handler , ExceptionSink es ) :
	m_handler(handler) ,
	m_es(es)
{
	// (the event loop requires manual-reset because it re-tests the handle state after WFMO has been released)
	const DWORD access = DELETE | SYNCHRONIZE | EVENT_MODIFY_STATE | PROCESS_DUP_HANDLE ;
	m_h = CreateEventEx( nullptr , nullptr , CREATE_EVENT_MANUAL_RESET , access ) ;
	if( m_h == 0 )
		throw FutureEvent::Error( "CreateEventEx" ) ;

	m_h2 = dup() ;
	G_DEBUG( "GNet::FutureEventImp::ctor: h=" << m_h.h << " h2=" << m_h2.h ) ;

	EventLoop::instance().addRead( Descriptor(INVALID_SOCKET,m_h.h) , *this , es ) ;
}

GNet::FutureEventImp::~FutureEventImp()
{
	if( EventLoop::exists() )
		EventLoop::instance().dropRead( Descriptor(INVALID_SOCKET,m_h.h) ) ;
}

HANDLE GNet::FutureEventImp::dup()
{
	// duplicate the handle so that the kernel object is only deleted
	// once both handles are closed -- we need the main thread and the
	// worker thread to both keep the kernel event-object alive
	HANDLE h = 0 ;
	BOOL ok = ::DuplicateHandle(
		::GetCurrentProcess() , m_h.h ,
		::GetCurrentProcess() , &h ,
		0 , FALSE , DUPLICATE_SAME_ACCESS ) ;
	if( !ok )
	{
		DWORD e = ::GetLastError() ;
		throw FutureEvent::Error( "DuplicateHandle" , G::Str::fromUInt(static_cast<unsigned int>(e)) ) ;
	}
	return h ;
}

GNet::FutureEventImp::handle_type GNet::FutureEventImp::handle()
{
	HANDLE h2 = 0 ;
	std::swap( h2 , m_h2.h ) ;
	return h2 ;
}

bool GNet::FutureEventImp::send( handle_type handle , bool close ) noexcept
{
	bool ok = ::SetEvent( handle ) != 0 ;
	if( close )
		::CloseHandle( handle ) ; // kernel event-object still open
	return ok ;
}

void GNet::FutureEventImp::readEvent()
{
	G_DEBUG( "GNet::FutureEventImp::readEvent: future event: h=" << m_h.h ) ;
	m_handler.onFutureEvent() ;
}

// ==

GNet::FutureEvent::FutureEvent( FutureEventHandler & handler , ExceptionSink es ) :
	m_imp(std::make_unique<FutureEventImp>(handler,es))
{
}

GNet::FutureEvent::~FutureEvent()
= default ;

bool GNet::FutureEvent::send( handle_type handle , bool close ) noexcept
{
	return FutureEventImp::send( handle , close ) ;
}

GNet::FutureEvent::handle_type GNet::FutureEvent::handle()
{
	return m_imp->handle() ;
}

/// \file gfutureevent_win32.cpp
