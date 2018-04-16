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
// gfutureevent_unix.cpp
//

#include "gdef.h"
#include "gfutureevent.h"
#include "gmsg.h"
#include "geventloop.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/// \class GNet::FutureEventImp
/// A pimple-pattern implementation class used by GNet::FutureEvent.
///
class GNet::FutureEventImp : public EventHandler
{
public:
	typedef FutureEvent::Error Error ;
	typedef FutureEvent::handle_type handle_type ;

	FutureEventImp( FutureEventHandler & , ExceptionHandler & ) ;
		// Constructor.

	virtual ~FutureEventImp() ;
		// Destructor.

	static bool send( handle_type ) g__noexcept ;
		// Writes to the write socket.

	void receive() ;
		// Reads from the socket to clear the event.

	handle_type handle() ;
		// Returns the socket fd as a handle.

private:
	FutureEventImp( const FutureEventImp & ) ;
	void operator=( const FutureEventImp & ) ;
	static int init( int ) ;
	virtual void readEvent() ; // Override from GNet::EventHandler.

private:
	FutureEventHandler & m_handler ;
	int m_fd_read ;
	int m_fd_write ;
	bool m_triggered ;
} ;

GNet::FutureEventImp::FutureEventImp( FutureEventHandler & handler , ExceptionHandler & eh ) :
	m_handler(handler) ,
	m_fd_read(-1) ,
	m_fd_write(-1) ,
	m_triggered(false)
{
	int fds[2] ;
	int rc = ::socketpair( AF_UNIX , SOCK_DGRAM , 0 , fds ) ;
	if( rc != 0 )
		throw Error( "socketpair" ) ;
	m_fd_read = init( fds[0] ) ;
	m_fd_write = init( fds[1] ) ;
	EventLoop::instance().addRead( Descriptor(m_fd_read) , *this , eh ) ;
}

int GNet::FutureEventImp::init( int fd )
{
	int rc = ::fcntl( fd , F_SETFL , ::fcntl(fd,F_GETFL) | O_NONBLOCK ) ; G_IGNORE_VARIABLE(rc) ;
	return fd ;
}

GNet::FutureEventImp::~FutureEventImp()
{
	if( m_fd_read >= 0 )
	{
		if( EventLoop::exists() )
			EventLoop::instance().dropRead( Descriptor(m_fd_read) ) ;
		::close( m_fd_read ) ;
	}
	if( m_fd_write >= 0 )
	{
		::close( m_fd_write ) ;
	}
}

GNet::FutureEventImp::handle_type GNet::FutureEventImp::handle()
{
	int fd = m_fd_write ;
	m_fd_write = -1 ;
	return static_cast<handle_type>(fd) ;
}

void GNet::FutureEventImp::receive()
{
	char c = '\0' ;
	ssize_t rc = ::recv( m_fd_read , &c , 1 , 0 ) ;
	G_IGNORE_VARIABLE( rc ) ;
}

bool GNet::FutureEventImp::send( handle_type handle ) g__noexcept
{
	int fd = static_cast<int>(handle) ;
	char c = '\0' ;
	ssize_t rc = G::Msg::send( fd , &c , 1 , 0 ) ;
	::close( fd ) ; // sic
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

