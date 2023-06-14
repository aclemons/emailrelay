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
/// \file gresolver.cpp
///

#include "gdef.h"
#include "gresolver.h"
#include "gresolverfuture.h"
#include "geventloop.h"
#include "gtimer.h"
#include "gfutureevent.h"
#include "gcleanup.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"

//| \class GNet::ResolverImp
/// A private "pimple" implementation class used by GNet::Resolver to do
/// asynchronous name resolution. The ResolverImp object contains a worker
/// thread that runs ResolverFuture::run(). The ResolverImp object's
/// lifetime is dependent on the worker thread, so the best the
/// GNet::Resolver class can do to cancel a resolve request is to
/// ask the ResolverImp to delete itself and then forget about it.
///
class GNet::ResolverImp : private FutureEventHandler
{
public:
	ResolverImp( Resolver & , ExceptionSink , const Location & ) ;
		// Constructor.

	~ResolverImp() override ;
		// Destructor. The destructor will block if the worker thread is
		// still busy.

	bool zombify() ;
		// Disarms the callback and schedules a 'delete this' for when
		// the workder thread has finished.

	static void start( ResolverImp * , HANDLE ) noexcept ;
		// Static worker-thread function to do name resolution. Calls
		// ResolverFuture::run() to do the work and then FutureEvent::send()
		// to signal the main thread. The event plumbing then results in a
		// call to Resolver::done() on the main thread.

	static std::size_t zcount() noexcept ;
		// Returns the number of zombify()d objects.

private: // overrides
	void onFutureEvent() override ; // GNet::FutureEventHandler

public:
	ResolverImp( const ResolverImp & ) = delete ;
	ResolverImp( ResolverImp && ) = delete ;
	ResolverImp & operator=( const ResolverImp & ) = delete ;
	ResolverImp & operator=( ResolverImp && ) = delete ;

private:
	void onTimeout() ;

private:
	using Pair = ResolverFuture::Pair ;
	Resolver * m_resolver ;
	std::unique_ptr<FutureEvent> m_future_event ;
	Timer<ResolverImp> m_timer ;
	Location m_location ;
	ResolverFuture m_future ;
	G::threading::thread_type m_thread ;
	static std::size_t m_zcount ;
} ;

std::size_t GNet::ResolverImp::m_zcount = 0U ;

GNet::ResolverImp::ResolverImp( Resolver & resolver , ExceptionSink es , const Location & location ) :
	m_resolver(&resolver) ,
	m_future_event(std::make_unique<FutureEvent>(static_cast<FutureEventHandler&>(*this),es)) ,
	m_timer(*this,&ResolverImp::onTimeout,es) ,
	m_location(location) ,
	m_future(location.host(),location.service(),location.family(),/*dgram=*/false,true)
{
	G_ASSERT( G::threading::works() ) ; // see Resolver::start()
	G::Cleanup::Block block_signals ;
	m_thread = G::threading::thread_type( ResolverImp::start , this , m_future_event->handle() ) ;
}

GNet::ResolverImp::~ResolverImp()
{
	try
	{
		// (should be already join()ed)
		if( m_thread.joinable() )
			m_thread.join() ;
	}
	catch(...)
	{
	}
}

std::size_t GNet::ResolverImp::zcount() noexcept
{
	return m_zcount ;
}

void GNet::ResolverImp::start( ResolverImp * This , HANDLE handle ) noexcept
{
	// thread function, spawned from ctor and join()ed from dtor
	try
	{
		This->m_future.run() ;
		FutureEvent::send( handle ) ;
	}
	catch(...) // worker thread outer function
	{
		// never gets here -- run and send are noexcept
		FutureEvent::send( handle ) ;
	}
}

void GNet::ResolverImp::onFutureEvent()
{
	G_DEBUG( "GNet::ResolverImp::onFutureEvent: future event: ptr=" << m_resolver ) ;

	Pair result = m_future.get() ;
	if( !m_future.error() )
		m_location.update( result.first , result.second ) ;

	if( m_thread.joinable() )
		m_thread.join() ; // worker thread is finishing, so no delay here

	Resolver * resolver = m_resolver ;
	m_resolver = nullptr ;
	if( resolver )
		resolver->done( std::string(m_future.reason()) , Location(m_location) ) ; // must take copies
}

bool GNet::ResolverImp::zombify()
{
	m_resolver = nullptr ;
	m_timer.startTimer( 0U ) ;
	m_zcount++ ;
	return true ;
}

void GNet::ResolverImp::onTimeout()
{
	if( m_thread.joinable() )
	{
		m_timer.startTimer( 1U ) ;
	}
	else
	{
		delete this ;
		m_zcount-- ;
	}
}

// ==

GNet::Resolver::Resolver( Resolver::Callback & callback , ExceptionSink es ) :
	m_callback(callback) ,
	m_es(es)
{
	// lazy imp construction
}

GNet::Resolver::~Resolver()
{
	if( m_imp && m_imp->zombify() )
	{
		G_DEBUG( "GNet::Resolver::dtor: zcount=" << ResolverImp::zcount() ) ;
		if( ResolverImp::zcount() == 100U )
			G_WARNING_ONCE( "GNet::Resolver::dtor: large number of threads waiting for dns results" ) ;

		// release the imp to the timer-list until its getaddrinfo() thread completes
		GDEF_IGNORE_RETURN m_imp.release() ;
	}
}

std::string GNet::Resolver::resolve( Location & location )
{
	// synchronous resolve
	using Pair = ResolverFuture::Pair ;
	G_DEBUG( "GNet::Resolver::resolve: resolve request [" << location.displayString() << "]"
		<< " (" << location.family() << ")" ) ;
	ResolverFuture future( location.host() , location.service() , location.family() , /*dgram=*/false ) ;
	future.run() ; // blocks until complete
	Pair result = future.get() ;
	if( future.error() )
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve error [" << future.reason() << "]" ) ;
		return future.reason() ;
	}
	else
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve result [" << result.first.displayString() << "]"
			<< "[" << result.second << "]" ) ;
		location.update( result.first , result.second ) ;
		return {} ;
	}
}

#ifndef G_LIB_SMALL
GNet::Resolver::AddressList GNet::Resolver::resolve( const std::string & host , const std::string & service ,
	int family , bool dgram )
{
	// synchronous resolve
	G_DEBUG( "GNet::Resolver::resolve: resolve-request [" << host << "/"
		<< service << "/" << (family==AF_UNSPEC?"ip":(family==AF_INET?"ipv4":"ipv6")) << "]" ) ;
	ResolverFuture future( host , service , family , dgram ) ;
	future.run() ;
	AddressList list ;
	future.get( list ) ;
	G_DEBUG( "GNet::Resolver::resolve: resolve result: list of " << list.size() ) ;
	return list ;
}
#endif

void GNet::Resolver::start( const Location & location )
{
	// asynchronous resolve
	if( !EventLoop::instance().running() ) throw Error( "no event loop" ) ;
	if( !async() ) throw Error( "not multi-threaded" ) ; // precondition
	if( busy() ) throw BusyError() ;
	G_DEBUG( "GNet::Resolver::start: resolve start [" << location.displayString() << "]" ) ;
	m_imp = std::make_unique<ResolverImp>( *this , m_es , location ) ;
}

void GNet::Resolver::done( const std::string & error , const Location & location )
{
	// callback from the event loop after worker thread is done
	G_DEBUG( "GNet::Resolver::done: resolve done: error=[" << error << "] "
		<< "location=[" << location.displayString() << "]" ) ;
	m_imp.reset() ;
	m_callback.onResolved( error , location ) ;
}

bool GNet::Resolver::busy() const
{
	return m_imp != nullptr ;
}

bool GNet::Resolver::async()
{
	if( G::threading::works() )
	{
		return EventLoop::instance().running() ;
	}
	else
	{
		G_DEBUG( "GNet::Resolver::async: not multi-threaded: using synchronous domain name lookup");
		return false ;
	}
}

