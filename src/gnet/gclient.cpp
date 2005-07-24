//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gsocket.h"
#include "gdatetime.h"
#include "gresolve.h"
#include "groot.h"
#include "gmonitor.h"
#include "gclient.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"

namespace
{
	const int c_retries = 10 ; // number of retries when using a privileged local port number
	const int c_port_start = 512 ;
	const int c_port_end = 1024 ;
	const size_t c_buffer_size = 1500U ; // see also gserver.h
	const char * c_cannot_connect_to = "cannot connect to " ;
}

namespace GNet
{
	class ClientResolver ;
}

// Class: GNet::ClientResolver
// Description: A resolver class which calls ClientImp::resolveCon() when done.
//
class GNet::ClientResolver : public GNet::Resolver 
{
private:
	ClientImp & m_client_imp ;

public:
	ClientResolver( ClientImp & imp ) ;
	void resolveCon( bool success , 
		const Address &address , std::string reason ) ;

private:
	ClientResolver( const ClientResolver & ) ;
	void operator=( const ClientResolver & ) ;
} ;

inline 
GNet::ClientResolver::ClientResolver( ClientImp & imp ) :
	m_client_imp(imp)
{
}

// ===

// Class: GNet::ClientImp
// Description: A pimple-pattern implementation class for GNet::Client.
//
class GNet::ClientImp : public GNet::EventHandler 
{
public:
	enum Status { Success , Failure , Retry , ImmediateSuccess } ;
	enum State { Idle , Resolving , Connecting , Connected , Failed , Disconnected } ;

public:
	ClientImp( Client & intaface , const Address & local_address , bool privileged , bool quit_on_disconnect ) ;
	virtual ~ClientImp() ;
	void resolveCon( bool ok , const Address & address , std::string reason ) ;
	void readEvent() ;
	void writeEvent() ;
	void exceptionEvent() ;
	bool connect( std::string host , std::string service , std::string * error , bool sync_dns ) ;
	std::string startConnecting( const Address & , const std::string & , bool & ) ;
	bool localBind( Address ) ;
	Status connectCore( Address , std::string * ) ;
	void disconnect() ;
	StreamSocket & s() ;
	const StreamSocket & s() const ;
	void run() ;
	void close() ;
	void blocked() ;
	bool connected() const ;
	void setState( State ) ;
	std::pair<bool,Address> localAddress() const ;
	std::pair<bool,Address> peerAddress() const ;
	std::string peerName() const ;

private:
	ClientImp( const ClientImp & ) ;
	void operator=( const ClientImp & ) ;
	static int getRandomPort() ;

private:
	ClientResolver m_resolver ;
	StreamSocket * m_s ;
	Address m_local_address ;
	Address m_remote_address ;
	std::string m_peer_name ;
	Client & m_interface ;
	bool m_privileged ;
	static bool m_first ;
	State m_state ;
	bool m_quit_on_disconnect ;
} ;

// ===

GNet::Client::Client( const Address & local_address , bool privileged , bool quit_on_disconnect ) :
	m_imp(NULL)
{
	G_DEBUG( "Client::ctor" ) ;
	m_imp = new ClientImp( *this , local_address , privileged , quit_on_disconnect ) ;
	if( Monitor::instance() ) Monitor::instance()->add( *this ) ;
}

GNet::Client::Client( bool privileged , bool quit_on_disconnect ) :
	m_imp(NULL)
{
	G_DEBUG( "Client::ctor" ) ;
	m_imp = new ClientImp( *this , Address(0U) , privileged , quit_on_disconnect ) ;
	if( Monitor::instance() ) Monitor::instance()->add( *this ) ;
}

GNet::Client::~Client()
{
	if( Monitor::instance() ) Monitor::instance()->remove( *this ) ;
	delete m_imp ;
}

bool GNet::Client::connect( std::string host , std::string service , std::string *error_p , bool sync_dns )
{
	return m_imp->connect( host , service , error_p , sync_dns ) ;
}

void GNet::Client::run()
{
	m_imp->run() ;
}

bool GNet::Client::connected() const
{
	return m_imp->connected() ;
}

void GNet::Client::blocked()
{
	m_imp->blocked() ;
}

void GNet::Client::disconnect()
{
	m_imp->disconnect() ;
}

std::pair<bool,GNet::Address> GNet::Client::localAddress() const
{
	return m_imp->localAddress() ;
}

std::pair<bool,GNet::Address> GNet::Client::peerAddress() const
{
	return m_imp->peerAddress() ;
}

std::string GNet::Client::peerName() const
{
	return m_imp->peerName() ;
}

//static
bool GNet::Client::canRetry( const std::string & error )
{
	return error.find( c_cannot_connect_to ) == 0U ;
}

// ===

bool GNet::ClientImp::m_first = true ;

GNet::ClientImp::ClientImp( Client & intaface , const Address & local_address ,
	bool privileged , bool quit_on_disconnect ) :
		m_resolver(*this) ,
		m_s(NULL) ,
		m_local_address(local_address) ,
		m_remote_address(Address::invalidAddress()) ,
		m_interface(intaface) ,
		m_privileged(privileged) ,
		m_state(Idle) ,
		m_quit_on_disconnect(quit_on_disconnect)
{
	G_DEBUG( "ClientImp::ctor" ) ;
}

int GNet::ClientImp::getRandomPort() 
{
	if( m_first )
	{
		std::srand( static_cast<unsigned int>(G::DateTime::now()) ) ;
		m_first = false ;
	}

	int r = std::rand() ;
	if( r < 0 ) r = -r ;
	r = r % (c_port_end - c_port_start) ;
	return r + c_port_start ;
}

GNet::StreamSocket & GNet::ClientImp::s()
{ 
	G_ASSERT( m_s != NULL ) ;
	return *m_s ;
}

const GNet::StreamSocket & GNet::ClientImp::s() const
{ 
	G_ASSERT( m_s != NULL ) ;
	return *m_s ;
}

GNet::ClientImp::~ClientImp()
{
	setState( Disconnected ) ; // for quit()
	close() ;
}

void GNet::ClientImp::disconnect()
{
	setState( Disconnected ) ;
	close() ;
}

void GNet::ClientImp::close()
{
	delete m_s ;
	m_s = NULL ;
}

bool GNet::ClientImp::connected() const
{
	return m_state == Connected ;
}

bool GNet::ClientImp::connect( std::string host , std::string service , 
	std::string *error_p , bool sync_dns )
{
	G_DEBUG( "GNet::ClientImp::connect: \"" << host << "\", \"" << service << "\"" ) ;

	std::string dummy_error_string ;
	if( error_p == NULL )
		error_p = &dummy_error_string ;
	std::string &error = *error_p ;

	if( sync_dns )
	{
		std::pair<Resolver::HostInfo,std::string> pair = Resolver::resolve( host , service ) ;
		std::string & resolve_reason = pair.second ;
		if( resolve_reason.length() != 0U )
		{
			error = resolve_reason ;
			setState( Failed ) ;
			return false ;
		}
		bool immediate = false ;
		std::string connect_reason = startConnecting( pair.first.address, pair.first.canonical_name, immediate);
		if( connect_reason.length() != 0U )
		{
			error = connect_reason ;
			setState( Failed ) ;
			return false ;
		}
		if( immediate )
		{
			G_DEBUG( "GNet::Client::connect: immediate connection" ) ; // delete soon
			s().addReadHandler( *this ) ;
			s().addExceptionHandler( *this ) ;
			setState( Connected ) ;
			m_interface.onConnect( s() ) ; // from within connect() ?
		}
		else
		{
			setState( Connecting ) ;
		}
	}
	else
	{
		std::string address_string( host ) ;
		address_string.append( ":" ) ;
		address_string.append( service.c_str() ) ;
		if( !m_resolver.resolveReq( address_string ) )
		{
			error = "invalid host/service: " ;
			error.append( address_string.c_str() ) ;
			setState( Failed ) ;
			return false ;
		}
		setState( Resolving ) ;
	}
	return true ;
}

void GNet::ClientImp::resolveCon( bool success , const Address &address , 
	std::string resolve_reason )
{
	if( success )
	{
		G_DEBUG( "GNet::ClientImp::resolveCon: " << address.displayString() ) ;
		std::string peer_name = resolve_reason ;
		bool immediate = false ;
		std::string connect_reason = startConnecting( address , peer_name , immediate ) ;
		if( connect_reason.length() )
		{
			close() ;
			setState( Failed ) ;
			m_interface.onError( connect_reason ) ;
		}
		setState( immediate ? Connected : Connecting ) ;
	}
	else
	{
		resolve_reason = std::string("resolver error: ") + resolve_reason ;
		close() ;
		setState( Failed ) ;
		m_interface.onError( resolve_reason ) ;
	}
}

std::string GNet::ClientImp::startConnecting( const Address & remote_address , 
	const std::string & peer_name , bool & immediate )
{
	// save the target address
	G_DEBUG( "GNet::ClientImp::startConnecting: " << remote_address.displayString() ) ;
	m_remote_address = remote_address ;
	m_peer_name = peer_name ;

	// create and open a socket
	//
	m_s = new StreamSocket ;
	if( !s().valid() )
	{
		return std::string( "cannot open socket" ) ;
	}

	// specifiy this as a 'write' event handler for the socket
	// (before the connect() in case it is reentrant)
	//
	s().addWriteHandler( *this ) ;

	// bind a local address to the socket and connect
	//
	Status status = Failure ;
	std::string error ;
	if( m_privileged )
	{
		for( int i = 0 ; i < c_retries ; i++ )
		{
			int port = getRandomPort() ;
			m_local_address.setPort( port ) ;
			G_DEBUG( "GNet::ClientImp::resolveCon: trying to bind " << m_local_address.displayString() ) ;
			status = localBind(m_local_address) ? Success : Retry ;
			if( status == Retry )
				continue ;

			status = connectCore( remote_address , &error ) ;
			if( status != Retry )
				break ;
		}
	}
	else
	{
		status = connectCore( remote_address , &error ) ;
	}

	// deal with immediate connection (typically if connecting locally)
	//
	immediate = status == ImmediateSuccess ;
	if( status != Success )
		s().dropWriteHandler() ;

	if( status == Success ) 
		error = std::string() ;

	return error ;
}

bool GNet::ClientImp::localBind( Address local_address )
{
	G::Root claim_root ;
	bool bound = s().bind(local_address) ;
	G_DEBUG( "GNet::ClientImp::bind: bound local address " << local_address.displayString() ) ;
	return bound ;
}

GNet::ClientImp::Status GNet::ClientImp::connectCore( Address remote_address , std::string *error_p )
{
	G_ASSERT( error_p != NULL ) ;
	std::string &error = *error_p ;

	// initiate the connection
	//
	bool immediate = false ;
	if( !s().connect( remote_address , &immediate ) )
	{
		G_DEBUG( "GNet::ClientImp::connect: immediate failure" ) ;
		error = c_cannot_connect_to ;
		error.append( remote_address.displayString().c_str() ) ;

		// we should return Failure here, but Microsoft's stack
		// will happily bind the same local address more than once,
		// so it is the connect that fails, not the bind, if
		// the port was already in use
		//
		return Retry ;
	}
	else 
	{
		return immediate ? ImmediateSuccess : Success ;
	}
}

void GNet::ClientImp::blocked()
{
	s().addWriteHandler( *this ) ;
}

void GNet::ClientImp::writeEvent()
{
	G_DEBUG( "GNet::ClientImp::writeEvent" ) ;

	if( m_state == Connected )
	{
		s().dropWriteHandler() ;
		m_interface.onWriteable() ;
	}
	else if( m_state == Connecting && s().hasPeer() )
	{
		s().addReadHandler( *this ) ;
		s().addExceptionHandler( *this ) ;
		s().dropWriteHandler() ;

		setState( Connected ) ;
		m_interface.onConnect( s() ) ;
	}
	else if( m_state == Connecting )
	{
		std::string message( c_cannot_connect_to ) ;
		message.append( m_remote_address.displayString().c_str() ) ;
		setState( Failed ) ;
		close() ;
		m_interface.onError( message ) ;
	}
}

void GNet::ClientImp::readEvent()
{
	char buffer[c_buffer_size] ;
	ssize_t n = s().read( buffer , sizeof(buffer) ) ;

	if( n == 0 || ( n == -1 && !s().eWouldBlock() ) )
	{
		close() ;
		setState( Disconnected ) ;
		m_interface.onDisconnect() ;
	}
	else if( n != -1 )
	{
		G_ASSERT( static_cast<size_t>(n) <= sizeof(buffer) ) ;
		//G_DEBUG( "GNet::ClientImp::readEvent: " << n << " byte(s)" ) ;
		m_interface.onData( buffer , n ) ;
	}
	else
	{
		; // no-op (windows)
	}
}

void GNet::ClientImp::exceptionEvent()
{
	G_DEBUG( "GNet::ClientImp::exceptionEvent" ) ;
	close() ;
	setState( Failed ) ;
	m_interface.onDisconnect() ;
}

void GNet::ClientImp::setState( State new_state )
{
	if( m_quit_on_disconnect &&
		(new_state == Disconnected || new_state == Failed) &&
		(m_state != Disconnected && m_state != Failed) )
	{
		G_DEBUG( "GNet::ClientImp::setState: " << m_state 
			<< " -> " << new_state << ": quitting the event loop" ) ;
		EventLoop::instance().quit() ;
	}
	m_state = new_state ;
}

void GNet::ClientImp::run()
{
	EventLoop::instance().run() ;
}

std::pair<bool,GNet::Address> GNet::ClientImp::localAddress() const
{
	return 
		m_s != NULL ?
			s().getLocalAddress() :
			std::make_pair(false,GNet::Address::invalidAddress()) ;
}

std::pair<bool,GNet::Address> GNet::ClientImp::peerAddress() const
{
	return 
		m_s != NULL ?
			s().getPeerAddress() :
			std::make_pair(false,GNet::Address::invalidAddress()) ;
}

std::string GNet::ClientImp::peerName() const
{
	return m_peer_name ;
}

// ===

void GNet::ClientResolver::resolveCon( bool success , const Address &address , 
	std::string reason )
{
	m_client_imp.resolveCon( success , address , reason ) ;
}

