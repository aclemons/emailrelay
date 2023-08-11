//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfutureevent_unix.cpp
///

#include "gdef.h"
#include "gfutureevent.h"
#include "gprocess.h"
#include "gmsg.h"
#include "geventloop.h"
#include <array>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//| \class GNet::FutureEventImp
/// A pimple-pattern implementation class used by GNet::FutureEvent.
///
class GNet::FutureEventImp : public EventHandler
{
public:
	FutureEventImp( FutureEventHandler & , ExceptionSink ) ;
		// Constructor.

	~FutureEventImp() override ;
		// Destructor.

	static bool send( HANDLE , bool ) noexcept ;
		// Writes to the write socket.

	void receive() ;
		// Reads from the socket to clear the event.

	HANDLE handle() noexcept ;
		// Extracts the socket fd as a handle.

public:
	FutureEventImp( const FutureEventImp & ) = delete ;
	FutureEventImp( FutureEventImp && ) = delete ;
	FutureEventImp & operator=( const FutureEventImp & ) = delete ;
	FutureEventImp & operator=( FutureEventImp && ) = delete ;

private: // overrides
	void readEvent() override ; // Override from GNet::EventHandler.

private:
	static int init( int ) ;

private:
	struct Fd
	{
		Fd() = default;
		~Fd() { if(fd!=-1) ::close(fd) ; }
		Fd &  operator=( int fd_ ) { fd = fd_ ; return *this ; }
		int fd{-1} ;
		Fd( const Fd & ) = delete ;
		Fd( Fd && ) = delete ;
		Fd & operator=( const Fd & ) = delete ;
		Fd & operator=( Fd && ) = delete ;
	} ;

private:
	FutureEventHandler & m_handler ;
	Fd m_read ;
	Fd m_write ;
	bool m_triggered ;
} ;

GNet::FutureEventImp::FutureEventImp( FutureEventHandler & handler , ExceptionSink es ) :
	m_handler(handler) ,
	m_triggered(false)
{
	std::array<int,2U> fds {{ -1 , -1 }} ;
	int rc = ::socketpair( AF_UNIX , SOCK_DGRAM , 0 , &fds[0] ) ;
	if( rc != 0 )
	{
		int e = G::Process::errno_() ;
		throw FutureEvent::Error( "socketpair" , G::Process::strerror(e) ) ;
	}
	m_read = init( fds[0] ) ;
	m_write = init( fds[1] ) ;
	EventLoop::instance().addRead( Descriptor(m_read.fd) , *this , es ) ;
}

int GNet::FutureEventImp::init( int fd )
{
	GDEF_IGNORE_RETURN ::fcntl( fd , F_SETFL , ::fcntl(fd,F_GETFL) | O_NONBLOCK ) ; // NOLINT
	return fd ;
}

GNet::FutureEventImp::~FutureEventImp()
{
	if( m_read.fd >= 0 )
	{
		if( EventLoop::exists() )
			EventLoop::instance().dropRead( Descriptor(m_read.fd) ) ;
	}
}

HANDLE GNet::FutureEventImp::handle() noexcept
{
	int fd = -1 ;
	std::swap( m_write.fd , fd ) ;
	return static_cast<HANDLE>(fd) ;
}

void GNet::FutureEventImp::receive()
{
	char c = '\0' ;
	GDEF_IGNORE_RETURN ::recv( m_read.fd , &c , 1 , 0 ) ;
}

bool GNet::FutureEventImp::send( HANDLE handle , bool close ) noexcept
{
	int fd = static_cast<int>(handle) ;
	char c = '\0' ;
	ssize_t rc = G::Msg::send( fd , &c , 1 , 0 ) ;
	if( close )
		::close( fd ) ; // just after send() is okay
	const bool ok = rc == 1 ;
	return ok ;
}

void GNet::FutureEventImp::readEvent()
{
	receive() ;
	if( !m_triggered )
	{
		m_triggered = true ;
		m_handler.onFutureEvent() ;
	}
}

// ==

GNet::FutureEvent::FutureEvent( FutureEventHandler & handler , ExceptionSink es ) :
	m_imp(std::make_unique<FutureEventImp>(handler,es))
{
}

GNet::FutureEvent::~FutureEvent()
= default ;

bool GNet::FutureEvent::send( HANDLE handle , bool close ) noexcept
{
	return FutureEventImp::send( handle , close ) ;
}

HANDLE GNet::FutureEvent::handle() noexcept
{
	return m_imp->handle() ;
}

