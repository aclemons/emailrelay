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
/// \file geventloophandles.cpp
///

#include "gdef.h"
#include "geventloophandles.h"

// threads are synchronised through 'event' objects, so it is
// not clear that there is any need for more synchronisation
// using critical sections
#define EXTRA_SAFE 0

#if 0
#pragma message( "unsafe trace code enabled" )
#include <sstream>
#include <io.h>
#define T_TRACE( expr ) do { std::ostringstream ss ; ss << expr << "\n" ; std::string s = ss.str() ; _write( 2 , s.data() , s.size() ) ; } while(0)
#define G_TRACE( expr ) G_LOG( expr )
#else
#define T_TRACE( expr )
#define G_TRACE( expr )
#endif

GNet::WaitThread::Event::Event() noexcept :
	m_handle(CreateEventA(nullptr,TRUE,FALSE,nullptr))
{
}

GNet::WaitThread::Event::~Event()
{
	if( m_handle )
		CloseHandle( m_handle ) ;
}

bool GNet::WaitThread::Event::isSet() const noexcept
{
	return WaitForSingleObject(m_handle,0) == WAIT_OBJECT_0 ;
}

void GNet::WaitThread::Event::set() noexcept
{
	SetEvent( m_handle ) ;
}

void GNet::WaitThread::Event::clear() noexcept
{
	ResetEvent( m_handle ) ;
}

void GNet::WaitThread::Event::wait( Lock & lock )
{
	bool ok = false ;
	lock.leave() ;
	ok = WaitForSingleObject( m_handle , wait_timeout_ms ) == WAIT_OBJECT_0 ;
	lock.enter() ;
	if( !ok )
		throw EventLoop::Error( "wait error" ) ;
}

bool GNet::WaitThread::Event::wait( Lock & lock , Event & a , Event & b )
{
	HANDLE h[2] = { a.m_handle , b.m_handle } ;
	lock.leave() ;
	DWORD rc = WaitForMultipleObjectsEx( 2U , h , FALSE , INFINITE , FALSE ) ;
	lock.enter() ;
	if( rc < WAIT_OBJECT_0 || rc > (WAIT_OBJECT_0+1U) )
		throw GetLastError() ;
	return rc == WAIT_OBJECT_0 ;
}

HANDLE GNet::WaitThread::Event::h() const noexcept
{
	return m_handle ;
}

void GNet::WaitThread::Lock::enter() noexcept
{
	#if EXTRA_SAFE
		EnterCriticalSection( &m_cs.m_cs ) ;
	#endif
}

void GNet::WaitThread::Lock::leave() noexcept
{
	#if EXTRA_SAFE
		LeaveCriticalSection( &m_cs.m_cs ) ;
	#endif
}

GNet::WaitThread::WaitThread( std::size_t id , std::size_t list_offset ) :
	m_id(id) ,
	m_list_offset(list_offset) ,
	m_handles_size(1U)
{
	m_handles[0] = m_stop_req.h() ;
	m_start_req.set() ;

	DWORD stack_size = 64000 ;
	m_hthread = CreateThread( nullptr , stack_size , &GNet::threadFn , this , 0 , nullptr ) ;
	if( m_hthread == HNULL )
		throw EventLoop::Error( "cannot create thread" ) ;
}

GNet::WaitThread::~WaitThread()
{
	if( m_hthread != HNULL )
	{
		{
			Lock lock( m_mutex ) ;
			m_terminate = true ;
			m_stop_req.set() ;
		}
		WaitForSingleObject( m_hthread , INFINITE ) ;
	}
}

void GNet::WaitThread::shuffle( std::size_t i_in )
{
	Lock lock( m_mutex ) ;
	std::size_t i = i_in + 1U ; // because m_handles[0] is internal
	moveToRhs( m_handles.begin() , i , m_handles_size ) ;
}

void GNet::WaitThread::stop()
{
	Lock lock( m_mutex ) ;
	m_stop_req.set() ;
	m_stop_con.wait( lock ) ;
	m_stop_con.clear() ;
}

void GNet::WaitThread::start()
{
	Lock lock( m_mutex ) ;
	m_start_req.set() ;
}

void GNet::WaitThread::mark() noexcept
{
	Lock lock( m_mutex ) ;
	m_marked = true ;
}

bool GNet::WaitThread::marked() const noexcept
{
	Lock lock( m_mutex ) ;
	return m_marked ;
}

std::size_t GNet::WaitThread::indication()
{
	Lock lock( m_mutex ) ;
	m_indicate.clear() ;
	return m_indication ;
}

HANDLE GNet::WaitThread::hindicate() const
{
	Lock lock( m_mutex ) ;
	return m_indicate.h() ;
}

HANDLE GNet::WaitThread::hthread() const
{
	Lock lock( m_mutex ) ;
	return m_hthread ;
}

std::size_t GNet::WaitThread::listOffset() const
{
	Lock lock( m_mutex ) ;
	return m_list_offset ;
}

std::size_t GNet::WaitThread::id() const noexcept
{
	return m_id ;
}

GNet::EventLoopHandles::~EventLoopHandles()
= default ;

GNet::EventLoopHandles::Rc GNet::EventLoopHandles::waitForMultipleObjects( DWORD ms )
{
	if( m_mt )
	{
		return waitForMultipleObjects_MT( ms ) ;
	}
	else
	{
		DWORD handles_n = static_cast<DWORD>( m_handles.size() ) ;
		HANDLE * handles_p = m_handles.empty() ? nullptr : &m_handles[0] ;
		DWORD rc = MsgWaitForMultipleObjectsEx( handles_n , handles_p , ms , QS_ALLINPUT , 0 ) ;

		if( rc == WAIT_TIMEOUT )
		{
			return { RcType::timeout } ;
		}
		else if( rc >= WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) )
		{
			return { RcType::event , static_cast<std::size_t>(rc-WAIT_OBJECT_0) } ;
		}
		else if( rc == (WAIT_OBJECT_0+handles_n) )
		{
			return { RcType::message } ;
		}
		else if( rc == WAIT_FAILED )
		{
			return { RcType::failed } ;
		}
		else
		{
			return { RcType::other } ;
		}
	}
}

bool GNet::EventLoopHandles::overflow( std::size_t n ) const
{
	if( m_mt )
	{
		return overflow_MT( n ) ;
	}
	else
	{
		return n > wait_limit ;
	}
}

void GNet::EventLoopHandles::handleInternalEvent( std::size_t index )
{
	if( m_mt )
		handleInternalEvent_MT( index ) ;
}

GNet::EventLoopHandlesMT::EventLoopHandlesMT( std::size_t threads )
{
	threads = std::min( main_thread_wait_limit/2U , std::max(std::size_t(1U),threads) ) ;

	// create the threads
	std::size_t offset = 0U ;
	for( std::size_t i = 0 ; i < threads ; i++ , offset += (WaitThread::wait_limit-1U) )
	{
		m_threads.push_back( std::make_unique<WaitThread>( i , offset ) ) ;
		m_handles.push_back( m_threads.back()->hthread() ) ;
	}

	// add their indication handles
	for( std::size_t i = 0 ; i < threads ; i++ )
	{
		m_handles.push_back( m_threads[i]->hindicate() ) ;
		m_index.push_back( i ) ; // (shuffled along with m_handles)
	}

	m_list_limit = offset ; // pre-calculate for overflow()
	G_ASSERT( m_list_limit == (threads*(WaitThread::wait_limit-1U)) ) ;

	G_LOG( "GNet::EventLoopHandlesMT::ctor: multi-threaded event loop: "
		<< threads << " threads, max " << m_list_limit << " handles" ) ;

	G_ASSERT( m_full_update ) ;
}

GNet::EventLoopHandlesMT::~EventLoopHandlesMT()
= default ;

GNet::EventLoopHandles::Rc GNet::EventLoopHandlesMT::waitForMultipleObjects( DWORD ms )
{
	G_ASSERT( !m_threads.empty() ) ;
	DWORD handles_n = static_cast<DWORD>( m_handles.size() ) ;
	HANDLE * handles_p = &m_handles[0] ;
	DWORD rc = MsgWaitForMultipleObjectsEx( handles_n , handles_p , ms , QS_ALLINPUT , 0 ) ;

	if( rc == WAIT_TIMEOUT )
	{
		return { RcType::timeout } ;
	}
	else if( rc >= WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) )
	{
		std::size_t handle_index = static_cast<std::size_t>(rc-WAIT_OBJECT_0) ;
		if( handle_index < m_threads.size() ) // if event on thread handle
			throw EventLoop::Error( "thread failed" ) ;

		std::size_t hind_index = handle_index - m_threads.size() ;
		std::size_t thread_index = m_index[hind_index] ;

		std::size_t indication = m_threads[thread_index]->indication() ; // with ind.clear()
		if( indication == 0U )
			throw EventLoop::Error() ;

		G_TRACE( "GNet::EventLoopHandlesMT::wait: event indicated: t=" << thread_index << " h=" << indication ) ;
		std::size_t list_index = m_threads[thread_index]->listOffset() + (indication-1U) ;
		return { RcType::event , list_index , handle_index , thread_index } ;
	}
	else if( rc == (WAIT_OBJECT_0+handles_n) )
	{
		return { RcType::message } ;
	}
	else if( rc == WAIT_FAILED )
	{
		return { RcType::failed } ;
	}
	else
	{
		return { RcType::other } ;
	}
}

void GNet::EventLoopHandlesMT::startCurrentThread( Rc rc )
{
	if( rc.type() == RcType::event )
		m_threads.at(rc.m_imp_2)->start() ;
}

void GNet::EventLoopHandlesMT::markCurrentThread( Rc rc )
{
	if( rc.type() == RcType::event )
		m_threads.at(rc.m_imp_2)->mark() ;
}

void GNet::EventLoopHandlesMT::stopMarkedThreads()
{
	for( std::size_t t = 0U ; t < m_threads.size() ; t++ )
	{
		if( m_threads[t]->marked() )
		{
			G_TRACE( "GNet::EventLoopHandlesMT::stopMarkedThreads: restarting thread " << t ) ;
			m_threads[t]->stop() ; // and wait for ind(0) and ind.clear()
		}
	}
}

void GNet::EventLoopHandlesMT::startMarkedThreads()
{
	for( std::size_t t = 0 ; t < m_threads.size() ; t++ )
	{
		if( m_threads[t]->marked() )
		{
			m_threads[t]->start() ;
		}
	}
}

bool GNet::EventLoopHandlesMT::overflow( std::size_t list_size ) const noexcept
{
	return list_size > m_list_limit ;
}

void GNet::EventLoopHandlesMT::handleInternalEvent( std::size_t )
{
	// no-op -- this is not needed because we map events that are
	// detected on the internal handles to refer to external handles
}

DWORD WINAPI GNet::threadFn( LPVOID arg )
{
	WaitThread * wt = static_cast<WaitThread*>( arg ) ;
	DWORD rc = 1 ;
	try
	{
		rc = wt->run() ;
	}
	catch( DWORD e )
	{
		rc = e ;
	}
	catch(...) // thread function
	{
	}
	T_TRACE( "thread " << wt->id() << " finished: " << rc ) ;
	return rc ;
}

DWORD GNet::WaitThread::run()
{
	// thread function -- uses only inline Event methods and no runtime library

#if EXTRA_SAFE
	HANDLE handles[wait_limit] ;
	DWORD handles_size = 0U ;
#endif
	for( bool terminated = false ; !terminated ; )
	{
		// wait for the main thread to start us once the handles have been updated
		T_TRACE( "thread " << m_id << ": stopped" ) ;
		for( bool started = false ; !started && !terminated ; )
		{
			Lock lock( m_mutex ) ;
			bool start = !Event::wait( lock , m_stop_req , m_start_req ) ;
			if( start )
			{
				m_start_req.clear() ;
				started = true ;
			}
			else
			{
				// if we get a stop request while we are already stopped just
				// acknowledge it and continue waiting for the start request
				T_TRACE( "thread " << m_id << ": stop request while stopped" ) ;
				m_stop_req.clear() ;
				m_stop_con.set() ;
				if( m_terminate )
					terminated = true ;
			}
		}

#if EXTRA_SAFE
		// copy the handles -- probably a pessimisation
		{
			Lock lock( m_mutex ) ;
			CopyMemory( handles , &m_handles[0] , sizeof(HANDLE)*m_handles_size ) ;
			handles_size = static_cast<DWORD>( m_handles_size ) ;
		}
#else
		HANDLE * handles = &m_handles[0] ;
		DWORD handles_size = static_cast<DWORD>( m_handles_size ) ;
#endif

		// wait on our stop event together with the event-loop's handles
		T_TRACE( "thread " << m_id << ": waiting: " << handles_size << " handles" ) ;
		DWORD rc = WaitForMultipleObjectsEx( handles_size , handles , FALSE , INFINITE , FALSE ) ;
		if( rc < WAIT_OBJECT_0 || rc >= (WAIT_OBJECT_0+handles_size) )
		{
			DWORD e = GetLastError() ;
			//if( e == ERROR_INVALID_HANDLE ) continue ;
			throw e ;
		}

		std::size_t index = static_cast<std::size_t>( rc - WAIT_OBJECT_0 ) ;
		if( index != 0U )
		{
			// indicate a handle event
			T_TRACE( "thread " << m_id << ": indicating " << index ) ;
			Lock lock( m_mutex ) ;
			m_indication = index ;
			m_indicate.set() ;
		}
	}
	return 0 ; // m_terminate'd
}

