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
// gresolver.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gresolverfuture.h"
#include "geventloop.h"
#include "gtimer.h"
#include "gfutureevent.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"

/// \class GNet::ResolverImp
/// A private "pimple" implementation class used by GNet::Resolver to do
/// asynchronous name resolution. The object contains a worker thread using
/// the future/promise pattern. Its lifetime is dependent on the worker
/// thread, so the best the GNet::Resolver class can do to cancel a resolve
/// request is to ask it to delete itself and then forget about it.
///
class GNet::ResolverImp : private FutureEventHandler
{
public:
	ResolverImp( Resolver & , ExceptionSink , const Location & ) ;
		// Constructor.

	~ResolverImp() override ;
		// Destructor. The destructor will block if the worker thread is
		// still busy.

	void doDelete() ;
		// Schedules a 'delete this'. The callback is disarmed.

	static void start( ResolverImp * , FutureEvent::handle_type ) ;
		// Static worker-thread function to do name resolution. Calls
		// ResolverFuture::run() to do the work and then FutureEvent::send()
		// to signal the main thread. The event plumbing then results in a
		// call to Resolver::done() on the main thread.

	static std::size_t count() ;
		// Returns the number of objects.

private: // overrides
	void onFutureEvent() override ; // GNet::FutureEventHandler

public:
	ResolverImp( const ResolverImp & ) = delete ;
	ResolverImp( ResolverImp && ) = delete ;
	void operator=( const ResolverImp & ) = delete ;
	void operator=( ResolverImp && ) = delete ;

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
	bool m_busy ;
	static std::size_t m_instance_count ;
} ;

std::size_t GNet::ResolverImp::m_instance_count = 0U ;

GNet::ResolverImp::ResolverImp( Resolver & resolver , ExceptionSink es , const Location & location ) :
	m_resolver(&resolver) ,
	m_future_event(new FutureEvent(*this,es)) ,
	m_timer(*this,&ResolverImp::onTimeout,es) ,
	m_location(location) ,
	m_future(location.host(),location.service(),location.family(),location.dgram(),true) ,
	m_thread(ResolverImp::start,this,m_future_event->handle()) ,
	m_busy(true)
{
	m_instance_count++ ;
}

GNet::ResolverImp::~ResolverImp()
{
	m_timer.cancelTimer() ;
	if( m_thread.joinable() )
	{
		G_WARNING( "ResolverImp::dtor: waiting for getaddrinfo thread to complete" ) ;
		m_thread.join() ;
	}
	m_instance_count-- ;
}

std::size_t GNet::ResolverImp::count()
{
	return m_instance_count ;
}

void GNet::ResolverImp::start( ResolverImp * This , FutureEvent::handle_type handle )
{
	// thread function, spawned from ctor and join()ed from dtor
	try
	{
		This->m_future.run() ;
		FutureEvent::send( handle ) ;
	}
	catch(...) // worker thread outer function
	{
		FutureEvent::send( handle ) ; // nothrow
	}
}

void GNet::ResolverImp::onFutureEvent()
{
	G_DEBUG( "GNet::ResolverImp::onFutureEvent: future event: ptr=" << m_resolver ) ;
	G_ASSERT( m_busy ) ; if( !m_busy ) return ;
	if( m_resolver == nullptr ) return ;

	m_thread.join() ; // worker thread is finishing, so no delay here
	m_busy = false ;
	Resolver * resolver = m_resolver ;
	m_resolver = nullptr ;
	m_timer.startTimer( 0U ) ;

	Pair result = m_future.get() ;
	if( !m_future.error() )
		m_location.update( result.first , result.second ) ;

	G_DEBUG( "GNet::ResolverImp::onFutureEvent: [" << m_future.reason() << "][" << m_location.displayString() << "]" ) ;
	resolver->done( std::string(m_future.reason()) , Location(m_location) ) ; // must take copies
}

void GNet::ResolverImp::doDelete()
{
	m_resolver = nullptr ;
	m_timer.startTimer( 0U ) ;
}

void GNet::ResolverImp::onTimeout()
{
	if( m_busy )
		m_timer.startTimer( 1U ) ;
	else
		delete this ;
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
	const std::size_t sanity_limit = 50U ; // dtor blocks after this limit
	if( m_imp != nullptr && ResolverImp::count() < sanity_limit )
	{
		// release the imp to an independent lifetime until its getaddrinfo() completes
		G_DEBUG( "GNet::Resolver::dtor: releasing still-busy thread: " << ResolverImp::count() ) ;
		m_imp->doDelete() ;
		G_IGNORE_RETURN( m_imp.release() ) ; // deleted in onTimeout() -- dtor blocks in thread::join()
	}
}

std::string GNet::Resolver::resolve( Location & location )
{
	// synchronous resolve
	using Pair = ResolverFuture::Pair ;
	G_DEBUG( "GNet::Resolver::resolve: resolve request [" << location.displayString() << "] (" << location.family() << ")" ) ;
	ResolverFuture future( location.host() , location.service() , location.family() , location.dgram() ) ;
	future.run() ; // blocks until complete
	Pair result = future.get() ;
	if( future.error() )
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve error [" << future.reason() << "]" ) ;
		return future.reason() ;
	}
	else
	{
		G_DEBUG( "GNet::Resolver::resolve: resolve result [" << result.first.displayString() << "][" << result.second << "]" ) ;
		location.update( result.first , result.second ) ;
		return std::string() ;
	}
}

GNet::Resolver::AddressList GNet::Resolver::resolve( const std::string & host , const std::string & service , int family , bool dgram )
{
	// synchronous resolve
	G_DEBUG( "GNet::Resolver::resolve: resolve-request [" << host << "/" << service << "/" << (family==AF_UNSPEC?"ip":(family==AF_INET?"ipv4":"ipv6")) << "]" ) ;
	ResolverFuture future( host , service , family , dgram ) ;
	future.run() ;
	AddressList list ;
	future.get( list ) ;
	G_DEBUG( "GNet::Resolver::resolve: resolve result: list of " << list.size() ) ;
	return list ;
}

void GNet::Resolver::start( const Location & location )
{
	// asynchronous resolve
	if( !EventLoop::instance().running() ) throw Error("no event loop") ;
	if( busy() ) throw BusyError() ;
	G_DEBUG( "GNet::Resolver::start: resolve start [" << location.displayString() << "]" ) ;
	m_imp = std::make_unique<ResolverImp>( *this , m_es , location ) ;
}

void GNet::Resolver::done( const std::string & error , const Location & location )
{
	// callback from the event loop after worker thread is done
	G_DEBUG( "GNet::Resolver::done: resolve done: error=[" << error << "] location=[" << location.displayString() << "]" ) ;
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

