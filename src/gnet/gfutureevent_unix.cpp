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
	typedef FutureEvent::handle_type handle_type ;

	FutureEventImp( FutureEventHandler & , ExceptionSink ) ;
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
	FutureEventImp( const FutureEventImp & ) g__eq_delete ;
	void operator=( const FutureEventImp & ) g__eq_delete ;
	static int init( int ) ;
	virtual void readEvent() ; // Override from GNet::EventHandler.

private:
	struct Fd
	{
		Fd() : fd(-1) {}
		~Fd() { if(fd!=-1) ::close(fd) ; }
		void operator=( int fd_ ) { fd = fd_ ; }
		int fd ;
		private: Fd( const Fd & ) g__eq_delete ;
		private: void operator=( const Fd & ) g__eq_delete ;
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
	int fds[2] = { -1 , -1 } ;
	int rc = ::socketpair( AF_UNIX , SOCK_DGRAM , 0 , fds ) ;
	if( rc != 0 )
		throw FutureEvent::Error( "socketpair" ) ;
	m_read = init( fds[0] ) ;
	m_write = init( fds[1] ) ;
	EventLoop::instance().addRead( Descriptor(m_read.fd) , *this , es ) ;
}

int GNet::FutureEventImp::init( int fd )
{
	int rc = ::fcntl( fd , F_SETFL , ::fcntl(fd,F_GETFL) | O_NONBLOCK ) ; G_IGNORE_VARIABLE(int,rc) ;
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

GNet::FutureEventImp::handle_type GNet::FutureEventImp::handle()
{
	int fd = -1 ;
	std::swap( m_write.fd , fd ) ;
	return static_cast<handle_type>(fd) ;
}

void GNet::FutureEventImp::receive()
{
	char c = '\0' ;
	ssize_t rc = ::recv( m_read.fd , &c , 1 , 0 ) ; G_IGNORE_VARIABLE(ssize_t,rc) ;
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

GNet::FutureEvent::FutureEvent( FutureEventHandler & handler , ExceptionSink es ) :
	m_imp(new FutureEventImp(handler,es))
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

