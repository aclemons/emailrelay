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

class GNet::FutureEventImp : public EventHandler
{
public:
	typedef FutureEvent::Error Error ;
	typedef FutureEvent::handle_type handle_type ;

	FutureEventImp( FutureEventHandler & handler , ExceptionHandler & eh ) ;
		// Constructor.

	virtual ~FutureEventImp() ;
		// Destructor.

	static bool send( handle_type hwnd ) g__noexcept ;
		// Raises an event.

	handle_type handle() ;
		// Returns the event-object handle.

private:
	FutureEventImp( const FutureEventImp & ) ;
	void operator=( const FutureEventImp & ) ;
	HANDLE dup() ;
	virtual void readEvent() override ; // GNet::EventHandler

private:
	FutureEventHandler & m_handler ;
	ExceptionHandler & m_eh ;
	HANDLE m_h ;
	HANDLE m_h2 ;
} ;

GNet::FutureEventImp::FutureEventImp( FutureEventHandler & handler , ExceptionHandler & eh ) :
	m_handler(handler) ,
	m_eh(eh) ,
	m_h(0) ,
	m_h2(0)
{
	// (the event loop requires manual-reset because it re-tests the state of the handles after WFMO has been released)
	m_h = ::CreateEventEx( NULL , NULL , CREATE_EVENT_MANUAL_RESET , DELETE | SYNCHRONIZE | EVENT_MODIFY_STATE | PROCESS_DUP_HANDLE ) ;
	if( m_h == 0 )
		throw Error( "CreateEventEx" ) ;

	m_h2 = dup() ;
	G_DEBUG( "GNet::FutureEventImp::ctor: h=" << m_h << " h2=" << m_h2 ) ;

	EventLoop::instance().addRead( Descriptor(INVALID_SOCKET,m_h) , *this , eh ) ;
}

GNet::FutureEventImp::~FutureEventImp()
{
	if( EventLoop::exists() )
		EventLoop::instance().dropRead( Descriptor(INVALID_SOCKET,m_h) ) ;
	::CloseHandle( m_h ) ;
	if( m_h2 )
		::CloseHandle( m_h2 ) ;
}

HANDLE GNet::FutureEventImp::dup()
{
	// duplicate the handle so that the kernel object is only deleted
	// once both handles are closed -- we need the main thread and the
	// worker thread to both keep the kernel event-object alive
	HANDLE h = 0 ;
	BOOL ok = ::DuplicateHandle(
		::GetCurrentProcess() , m_h ,
		::GetCurrentProcess() , &h ,
		0 , FALSE , DUPLICATE_SAME_ACCESS ) ;
	if( !ok )
	{
		DWORD e = ::GetLastError() ;
		throw Error( "DuplicateHandle" , G::Str::fromUInt(static_cast<unsigned int>(e)) ) ;
	}
	return h ;
}

GNet::FutureEventImp::handle_type GNet::FutureEventImp::handle()
{
	HANDLE h2 = m_h2 ;
	m_h2 = 0 ;
	return h2 ;
}

bool GNet::FutureEventImp::send( handle_type handle ) g__noexcept
{
	bool ok = ::SetEvent( handle ) != 0 ;
	::CloseHandle( handle ) ; // kernel event-object still open
	return ok ;
}

void GNet::FutureEventImp::readEvent()
{
	G_DEBUG( "GNet::FutureEventImp::readEvent: future event: h=" << m_h ) ;
	m_handler.onFutureEvent() ;
}

// ==

GNet::FutureEvent::FutureEvent( FutureEventHandler & handler , ExceptionHandler & eh ) :
	m_imp(new FutureEventImp(handler,eh))
{
}

GNet::FutureEvent::~FutureEvent()
{
}

bool GNet::FutureEvent::send( handle_type handle ) g__noexcept
{
	return FutureEventImp::send( handle ) ;
}

GNet::FutureEvent::handle_type GNet::FutureEvent::handle()
{
	return m_imp->handle() ;
}

// ==

GNet::FutureEventHandler::~FutureEventHandler()
{
}

/// \file gfutureevent_win32.cpp
