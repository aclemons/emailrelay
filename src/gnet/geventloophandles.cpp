//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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

// From WaitForMultipleObjects() 'Remarks':
//  "To wait on more than MAXIMUM_WAIT_OBJECTS handles, used
//   one of the following methods: (1) Create a thread to wait
//   on MAXIMUM_WAIT_OBJECTS handles, then wait on that thread
//   plus the other handles. Use this technique to break the
//   handles into groups of MAXIMUM_WAIT_OBJECTS. (2) ..."

#include "gdef.h"
#if G_WINDOWS
#include "geventloophandles.h"
#include "glog.h"
#include <vector>
#include <array>

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

namespace GNet
{
	class WaitThread ;

	DWORD WINAPI threadFn( LPVOID arg ) ;
	template <typename T> static void moveToRhs( T , std::size_t , std::size_t ) ;

	static constexpr std::size_t wait_threads = 20U ; // 20*62=1240
	static constexpr std::size_t main_thread_wait_limit = wait_limit ;
	static constexpr std::size_t wait_thread_wait_limit = (MAXIMUM_WAIT_OBJECTS-1) ;

	static_assert( wait_threads <= main_thread_wait_limit , "" ) ;
	static_assert( wait_threads * (wait_thread_wait_limit-1U) > main_thread_wait_limit , "" ) ;
}

class GNet::WaitThread
{
public:
	using List = EventLoopImp::List ;
	static constexpr std::size_t wait_limit = wait_thread_wait_limit ;

public:
	WaitThread( std::size_t id , std::size_t list_offset ) ;
	~WaitThread() ;
	DWORD run() ;
	HANDLE hindicate() const ;
	HANDLE hthread() const ;
	std::size_t indication() ;
	void markIfDifferent( std::size_t , List::iterator , std::size_t n ) ;
	void updateIfMarked( std::size_t t , List::iterator , std::size_t n ) ;
	void start() ;
	void stop() ;
	std::size_t listOffset() const ;
	void shuffle( std::size_t ) ;
	bool marked() const noexcept ;

public:
	WaitThread() = delete ;
	WaitThread( const WaitThread & ) = delete ;
	WaitThread( WaitThread && ) = delete ;
	WaitThread & operator=( const WaitThread & ) = delete ;
	WaitThread & operator=( WaitThread && ) = delete ;

private:
	static constexpr int ThreadError = 1 ;
	struct CriticalSection
	{
		CriticalSection() noexcept { InitializeCriticalSection( &m_cs ) ; }
		~CriticalSection() { DeleteCriticalSection( &m_cs ) ; }
		CRITICAL_SECTION m_cs ;
	} ;
	struct Lock
	{
		Lock( CriticalSection & cs ) noexcept : m_cs(cs) { EnterCriticalSection( &cs.m_cs ) ; }
		~Lock() { LeaveCriticalSection( &m_cs.m_cs ) ; }
		CriticalSection & m_cs ;
	} ;
	struct Event
	{
		Event() noexcept : m_handle(CreateEventA(nullptr,TRUE,FALSE,nullptr)) {}
		~Event() { if( m_handle ) CloseHandle( m_handle ) ; }
		bool isSet() const noexcept { return WaitForSingleObject(m_handle,0) == WAIT_OBJECT_0 ; }
		void set() noexcept { SetEvent( m_handle ) ; }
		void clear() noexcept { ResetEvent( m_handle ) ; }
		bool wait( std::nothrow_t ) noexcept { return WaitForSingleObject( m_handle , INFINITE ) == WAIT_OBJECT_0 ; }
		void wait() { if( !wait(std::nothrow) ) throw WaitThread::ThreadError ; }
		static bool wait( Event & a , Event & b )
		{
			HANDLE h[2] = { a.m_handle , b.m_handle } ;
			DWORD rc = WaitForMultipleObjectsEx( 2U , h , FALSE , INFINITE , FALSE ) ;
			if( rc < WAIT_OBJECT_0 || rc > (WAIT_OBJECT_0+1U) ) throw WaitThread::ThreadError ;
			return rc == WAIT_OBJECT_0 ;
		}
		HANDLE h() const noexcept { return m_handle ; }
		HANDLE m_handle ;
	} ;

private:
	mutable CriticalSection m_mutex ;
	const std::size_t m_id ;
	const std::size_t m_list_offset{0U} ;
	HANDLE m_hthread{HNULL} ;
	Event m_start_req ;
	Event m_stop_req ;
	Event m_indicate ;
	Event m_stop_con ;
	std::size_t m_indication{0U} ;
	bool m_terminate{false} ;
	std::array<HANDLE,wait_limit> m_handles ;
	std::size_t m_handles_size ;
	bool m_marked{false} ;
} ;

class GNet::EventLoopHandlesImp
{
public:
	using List = EventLoopImp::List ;
	using ListItem = EventLoopImp::ListItem ;
	using Rc = EventLoopImp::Rc ;
	using RcType = EventLoopImp::RcType ;

public:
	EventLoopHandlesImp( List & , std::size_t ) ;
	~EventLoopHandlesImp() ;
	void init( List & ) ;
	void update( List & , bool , Rc ) ;
	bool overflow( List & , bool (*fn)(const ListItem&) ) const ;
	bool overflow( std::size_t ) const noexcept ;
	std::string help( const List & , bool ) const ;
	Rc waitForMultipleObjects( DWORD ) ;
	std::size_t shuffle( List & , Rc ) ;
	void handleInternalEvent( std::size_t ) ;

public:
	EventLoopHandlesImp( const EventLoopHandlesImp & ) = delete ;
	EventLoopHandlesImp( EventLoopHandlesImp && ) = delete ;
	void operator=( const EventLoopHandlesImp & ) = delete ;
	void operator=( EventLoopHandlesImp && ) = delete ;

private:
	using UpdateFn = void (WaitThread::*)(std::size_t,List::iterator,std::size_t) ;
	void startMarkedThreads() ;
	void stopMarkedThreads() ;
	void forEachThread( UpdateFn , List & ) ;

private:
	std::vector<HANDLE> m_handles ; // hthread-s then hind-s
	std::vector<std::size_t> m_index ; // map from m_handles position to m_thread index
	std::vector<std::unique_ptr<WaitThread>> m_threads ;
	std::size_t m_list_limit{0U} ;
	bool m_full_update{true} ;
} ;

template <typename T>
void GNet::moveToRhs( T begin , std::size_t i , std::size_t n )
{
	if( (i+1U) < n )
	{
		T first = begin + i ;
		T last = begin + n ;
		T new_first = first + 1 ;
		std::rotate( first , new_first , last ) ;
	}
}

// ==

GNet::WaitThread::WaitThread( std::size_t id , std::size_t list_offset ) :
	m_id(id) ,
	m_list_offset(list_offset) ,
	m_handles_size(1U)
{
	m_handles[0] = m_stop_req.h() ;

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

DWORD WINAPI GNet::threadFn( LPVOID arg )
{
	try
	{
		WaitThread * wt = static_cast<WaitThread*>( arg ) ;
		return wt->run() ;
	}
	catch(...) // thread function
	{
	}
	return 1 ;
}

DWORD GNet::WaitThread::run()
{
	m_start_req.set() ;
	for(;;)
	{
		// wait for the main thread to start us once the handles have been updated
		T_TRACE( "thread " << m_id << ": stopped" ) ;
		for(;;)
		{
			bool start = Event::wait( m_start_req , m_stop_req ) ;
			if( start )
			{
				m_start_req.clear() ;
				break ;
			}
			else
			{
				// if we get a stop request while we are already stopped just
				// acknowledge it and continue waiting for the start request
				T_TRACE( "thread " << m_id << ": stop request while stopped" ) ;
				m_stop_req.clear() ;
				m_stop_con.set() ;
				if( m_terminate ) return 0 ;
			}
		}

		// wait on our stop event together with the event-loop's handles
		T_TRACE( "thread " << m_id << ": waiting: " << m_handles_size << " handles" ) ;
		DWORD handles_n = static_cast<DWORD>( m_handles_size ) ;
		HANDLE * handles_p = handles_n ? &m_handles[0] : nullptr ;
		DWORD rc = WaitForMultipleObjectsEx( handles_n , handles_p , FALSE , INFINITE , FALSE ) ;
		if( rc < WAIT_OBJECT_0 || rc >= (WAIT_OBJECT_0+handles_n) )
			throw ThreadError ;

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

void GNet::WaitThread::shuffle( std::size_t i_in )
{
	std::size_t i = i_in + 1U ; // because m_handles[0] is internal
	Lock lock( m_mutex ) ;
	moveToRhs( m_handles.begin() , i , m_handles_size ) ;
}

void GNet::WaitThread::stop()
{
	m_stop_req.set() ;
	m_stop_con.wait() ;
	m_stop_con.clear() ;
}

void GNet::WaitThread::start()
{
	m_start_req.set() ;
}

void GNet::WaitThread::updateIfMarked( std::size_t t , List::iterator list_p , std::size_t n )
{
	Lock lock( m_mutex ) ;
	if( m_marked )
	{
		G_ASSERT( (n+1U) <= WaitThread::wait_limit ) ;
		G_ASSERT( m_handles.size() == WaitThread::wait_limit ) ;
		G_ASSERT( m_handles_size > 0U ) ;
		HANDLE * out = &m_handles[0] + 1 ;
		for( std::size_t i = 0U ; i < n ; i++ , ++list_p )
		{
			*out++ = (*list_p).m_handle ;
		}
		m_handles_size = n + 1U ;
	}
}

void GNet::WaitThread::markIfDifferent( std::size_t , List::iterator list_p , std::size_t n )
{
	Lock lock( m_mutex ) ;
	if( m_handles_size != (n+1U) )
	{
		m_marked = true ;
	}
	else
	{
		m_marked = false ;
		HANDLE * hp = &m_handles[0] + 1 ;
		for( std::size_t i = 0U ; i < n ; i++ , ++hp , ++list_p )
		{
			if( *hp != (*list_p).m_handle )
			{
				m_marked = true ;
				break ;
			}
		}
	}
}

bool GNet::WaitThread::marked() const noexcept
{
	volatile bool result = false ;
	{
		Lock lock( m_mutex ) ;
		result = m_marked ;
	}
	return result ;
}

std::size_t GNet::WaitThread::indication()
{
	volatile std::size_t result = 0U ;
	{
		Lock lock( m_mutex ) ;
		result = m_indication ;
		m_indicate.clear() ;
	}
	return result ;
}

HANDLE GNet::WaitThread::hindicate() const
{
	return m_indicate.h() ;
}

HANDLE GNet::WaitThread::hthread() const
{
	return m_hthread ;
}

std::size_t GNet::WaitThread::listOffset() const
{
	return m_list_offset ;
}

// ==

GNet::EventLoopHandlesImp::EventLoopHandlesImp( List & , std::size_t threads )
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

	G_LOG( "GNet::EventLoopHandlesImp::ctor: multi-threaded event loop: "
		<< threads << " threads, max " << m_list_limit << " handles" ) ;

	G_ASSERT( m_full_update ) ;
}

GNet::EventLoopHandlesImp::~EventLoopHandlesImp()
{
}

void GNet::EventLoopHandlesImp::init( List & list )
{
	update( list , true , Rc(RcType::other) ) ;
}

GNet::EventLoopImp::Rc GNet::EventLoopHandlesImp::waitForMultipleObjects( DWORD ms )
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

		G_TRACE( "GNet::EventLoopHandlesImp::wait: event indicated: t=" << thread_index << " h=" << indication ) ;
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

std::size_t GNet::EventLoopHandlesImp::shuffle( List & list , Rc rc )
{
	G_ASSERT( rc.type() == RcType::event ) ;
	G_ASSERT( m_handles.size() == (m_threads.size()*2U) ) ;
	G_ASSERT( m_index.size() == m_threads.size() ) ;
	G_ASSERT( m_index[rc.m_imp_1-m_threads.size()] == rc.m_imp_2 ) ;

	std::size_t list_index = rc.index() ;
	std::size_t handle_index = rc.m_imp_1 ;
	std::size_t hind_index = handle_index - m_threads.size() ;
	std::size_t thread_index = m_index[hind_index] ;
	std::size_t list_thread_start = m_threads[thread_index]->listOffset() ;
	std::size_t list_thread_end = std::min( list_thread_start+WaitThread::wait_limit-1U , list.size() ) ;

	G_ASSERT( rc.m_imp_2 == thread_index ) ;
	G_ASSERT( list_index >= list_thread_start ) ;
	G_ASSERT( handle_index < m_handles.size() ) ;
	G_ASSERT( handle_index >= m_threads.size() ) ;
	G_ASSERT( m_handles[handle_index] == m_threads[thread_index]->hindicate() ) ;

	// shift the list item within its thread block
	moveToRhs( list.begin() , list_index , list_thread_end ) ;

	// shift the handle within the thread
	m_threads[thread_index]->shuffle( list_index-list_thread_start ) ;

	// shift the thread indication handle and its index
	moveToRhs( m_handles.begin() , handle_index , m_threads.size() ) ;
	moveToRhs( m_index.begin() , handle_index , m_threads.size() ) ;

	G_ASSERT( list_thread_end != 0U ) ;
	return list_thread_end - 1U ; // since now rhs of thread block
}

void GNet::EventLoopHandlesImp::update( List & list , bool updated , Rc rc )
{
	if( updated || m_full_update )
	{
		// stop threads, hand out blocks of handles and restart them
		G_TRACE( "GNet::EventLoopHandlesImp::update: update" ) ;
		forEachThread( &WaitThread::markIfDifferent , list ) ; // see what threads need restarting
		stopMarkedThreads() ;
		forEachThread( &WaitThread::updateIfMarked , list ) ;
		startMarkedThreads() ;
		m_full_update = false ;
	}
	else
	{
		// if the list has not changed from the last go-round then
		// we just need to release the current event's thread so
		// that it starts waiting again
		if( rc.type() == RcType::event )
		{
			G_TRACE( "GNet::EventLoopHandlesImp::update: no update: releasing thread " << rc.m_imp_2 ) ;
			std::size_t thread_index = rc.m_imp_2 ;
			m_threads.at(thread_index)->start() ;
		}
	}
}

void GNet::EventLoopHandlesImp::stopMarkedThreads()
{
	for( std::size_t t = 0U ; t < m_threads.size() ; t++ )
	{
		if( m_threads[t]->marked() )
		{
			G_TRACE( "GNet::EventLoopHandlesImp::stopMarkedThreads: restarting thread " << t ) ;
			m_threads[t]->stop() ; // and wait for ind(0) and ind.clear()
		}
	}
}

void GNet::EventLoopHandlesImp::startMarkedThreads()
{
	for( std::size_t t = 0 ; t < m_threads.size() ; t++ )
	{
		if( m_threads[t]->marked() )
		{
			m_threads[t]->start() ;
		}
	}
}

void GNet::EventLoopHandlesImp::forEachThread( UpdateFn update_memfn , List & list )
{
	// hand out blocks of handles to each thread
	std::size_t navail = list.size() ;
	auto list_p = list.begin() ;
	for( std::size_t t = 0 ; t < m_threads.size() ; t++ )
	{
		std::size_t n = std::min( navail , WaitThread::wait_limit-1U ) ;
		((*m_threads[t]).*update_memfn)( t , list_p , n ) ;
		navail -= n ;
		if( navail )
			list_p += (WaitThread::wait_limit-1U) ;
	}
}

bool GNet::EventLoopHandlesImp::overflow( List & list , bool (*fn)(const ListItem&) ) const
{
	if( list.empty() ) return false ;
	std::size_t list_count = std::count_if( list.cbegin() , list.cbegin()+list.size()-1U , fn ) + 1U ;
	return list_count > m_list_limit ;
}

bool GNet::EventLoopHandlesImp::overflow( std::size_t list_size ) const noexcept
{
	return list_size > m_list_limit ;
}

std::string GNet::EventLoopHandlesImp::help( const List & list , bool on_add ) const
{
	std::ostringstream ss ;
	ss << "too many open handles (" << list.size() << (on_add?"/":">") << m_list_limit << ")" ;
	return ss.str() ;
}

void GNet::EventLoopHandlesImp::handleInternalEvent( std::size_t )
{
	// no-op -- this is not needed because we map events that are
	// detected on the internal handles to refer to external handles
}

// ==

GNet::EventLoopHandles::EventLoopHandles( List & list , std::size_t threads )
{
	if( threads )
		m_imp = std::make_unique<EventLoopHandlesImp>( list , threads ) ;
}

GNet::EventLoopHandles::~EventLoopHandles()
{
}

void GNet::EventLoopHandles::initImp( List & list )
{
	m_imp->init( list ) ;
}

void GNet::EventLoopHandles::updateImp( List & list , bool updated , Rc rc )
{
	m_imp->update( list , updated , rc ) ;
}

bool GNet::EventLoopHandles::overflowImp( List & list , bool (*fn)(const ListItem&) )
{
	if( !m_imp )
	{
		G_LOG( "GNet::EventLoopHandles: large number of open handles: switching event-loop" ) ;
		m_imp = std::make_unique<EventLoopHandlesImp>( list , wait_threads ) ;

		// no need for HandlesImp::init() because there will soon be garbage
		// collection of the list and a full update() -- the new threads will
		// start by waiting on an empty set of event-loop handles plus
		// their stop event
		//m_imp->init( list , fn ) ;
	}
	return m_imp->overflow( list , fn ) ;
}

bool GNet::EventLoopHandles::overflowImp( std::size_t n ) const
{
	return m_imp->overflow( n ) ;
}

std::string GNet::EventLoopHandles::helpImp( const List & list , bool on_add ) const
{
	return m_imp->help( list , on_add ) ;
}

std::size_t GNet::EventLoopHandles::shuffleImp( List & list , Rc rc )
{
	return m_imp->shuffle( list , rc ) ;
}

GNet::EventLoopImp::Rc GNet::EventLoopHandles::waitForMultipleObjectsImp( DWORD ms )
{
	return m_imp->waitForMultipleObjects( ms ) ;
}

void GNet::EventLoopHandles::handleInternalEventImp( std::size_t index )
{
	m_imp->handleInternalEvent( index ) ;
}

#endif
