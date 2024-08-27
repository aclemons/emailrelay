//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gexception.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm>
#include <functional>

namespace GNet
{
	namespace EventLoopHandlesImp
	{
		constexpr std::size_t WAIT_LIMIT = (MAXIMUM_WAIT_OBJECTS-1) ; // 63
		std::unique_ptr<EventLoopHandlesBase> newEventLoopHandlesMt( const EventLoopConfig & ) ;
		bool overflowMt( const EventLoopConfig & , std::size_t , std::function<std::size_t()> ) ;
		HANDLE createEvent() noexcept ;
		void shuffle( std::vector<HANDLE> & handles , std::vector<std::size_t> & indexes , std::size_t offset )
		{
			if( (offset+1U) < handles.size() ) // if not already rightmost
			{
				std::rotate( handles.begin()+offset , handles.begin()+(offset+1U) , handles.end() ) ;
				std::rotate( indexes.begin()+offset , indexes.begin()+(offset+1U) , indexes.end() ) ;
			}
		}
	}
}

GNet::EventLoopHandles::EventLoopHandles()
{
	// by default start single-threaded
	namespace imp = EventLoopHandlesImp ;
	if( m_config.st_wait_limit == 0U && !m_config.st_only )
		m_mt = imp::newEventLoopHandlesMt( m_config ) ;
}

GNet::EventLoopHandles::~EventLoopHandles()
= default ;

GNet::EventLoopHandlesRc GNet::EventLoopHandles::wait( DWORD ms )
{
	namespace imp = EventLoopHandlesImp ;
	if( m_mt )
	{
		return m_mt->wait( ms ) ;
	}
	else
	{
		if( m_handles.size() > m_config.st_wait_limit )
			return { RcType::overflow } ;

		DWORD handles_n = static_cast<DWORD>( m_handles.size() ) ;
		HANDLE * handles_p = m_handles.empty() ? nullptr : m_handles.data() ;
		DWORD rc = MsgWaitForMultipleObjectsEx( handles_n , handles_p , ms , QS_ALLINPUT , 0 ) ;

		if( rc == WAIT_TIMEOUT )
		{
			return { RcType::timeout } ;
		}
		else if( rc >= WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) )
		{
			std::size_t offset = static_cast<std::size_t>(rc-WAIT_OBJECT_0) ;
			std::size_t index = m_indexes[offset] ;
			imp::shuffle( m_handles , m_indexes , offset ) ; // shuffle to avoid starvation -- move current handle to rhs
			return { RcType::event , index } ;
		}
		else if( rc == (WAIT_OBJECT_0+handles_n) )
		{
			return { RcType::message } ;
		}
		else // WAIT_FAILED
		{
			return Rc::failure( GetLastError() ) ;
		}
	}
}

void GNet::EventLoopHandles::update( std::size_t list_size , std::function<HANDLE()> list_fn , bool updated )
{
	namespace imp = EventLoopHandlesImp ;
	if( m_mt )
	{
		m_mt->update( list_size , list_fn , updated ) ;
	}
	else if( !m_config.st_only && list_size > m_config.st_wait_limit )
	{
		// switch to the multi-threaded implementation on first overflow
		m_mt = imp::newEventLoopHandlesMt( m_config ) ;
		m_mt->update( list_size , list_fn , true ) ;
	}
	else if( updated )
	{
		m_handles.resize( list_size ) ;
		m_indexes.resize( list_size ) ;
		for( std::size_t i = 0U ; i < list_size ; i++ )
		{
			m_handles[i] = list_fn() ;
			m_indexes[i] = i ;
		}
	}
}

void GNet::EventLoopHandles::onClose( HANDLE h )
{
	if( m_mt )
		m_mt->onClose( h ) ;
	else
		; // no-op
}

bool GNet::EventLoopHandles::overflow( std::size_t list_size , std::function<std::size_t()> list_size_fn ) const
{
	namespace imp = EventLoopHandlesImp ;
	if( m_mt )
		return m_mt->overflow( list_size , list_size_fn ) ;
	else if( !m_config.st_only )
		return imp::overflowMt( m_config , list_size , list_size_fn ) ;
	else
		return list_size > m_config.st_wait_limit && list_size_fn() > m_config.st_wait_limit ;
}

// --

GNet::EventLoopConfig::EventLoopConfig() :
	st_only(false) ,
	update_all(false) ,
	st_wait_limit(EventLoopHandlesImp::WAIT_LIMIT) ,
	mt_wait_limit(EventLoopHandlesImp::WAIT_LIMIT) ,
	mt_thread_limit(EventLoopHandlesImp::WAIT_LIMIT)
{
}

// --

GNet::EventLoopHandlesBase::~EventLoopHandlesBase()
= default ;

// ==
// multithreaded implementation...

#include <thread>
#include <atomic>
#include <array>

namespace GNet
{
	class EventLoopHandlesMt ;
	struct EventLoopThread ;
}

struct GNet::EventLoopThread
{
	static constexpr std::size_t WAIT_LIMIT = EventLoopHandlesImp::WAIT_LIMIT ;
	static constexpr std::size_t margin = 3U ;

	explicit EventLoopThread( int id ) ;
	~EventLoopThread() ;
	void run( G::LogOutput::Config , int ) noexcept ;
	static HANDLE createEvent() noexcept ;
	static void shuffle( HANDLE * , std::size_t * , std::size_t size , std::size_t offset ) noexcept ;

	// constant...
	const int m_id ;
	const HANDLE m_hquitreq {HNULL} ; // m_handle[0]
	const HANDLE m_hstartreq {HNULL} ; // m_handle[1]
	const HANDLE m_hstartcon {HNULL} ;
	const HANDLE m_hstopreq {HNULL} ; // m_handle[2]
	const HANDLE m_hstopcon {HNULL} ;
	const HANDLE m_heventind {HNULL} ;

	// main-thread only...
	bool m_stop {false} ;
	std::size_t m_thread_handle_count {0U} ;
	std::array<HANDLE,WAIT_LIMIT> m_thread_handles ; // not shuffled

	// this-thread only...
	bool m_stopped {true} ;
	bool m_failed {false} ;

	// atomic...
	std::atomic<std::size_t> m_event_value {0U} ;

	// updated while idle...
	std::size_t m_wait_handle_count {0U} ;
	std::array<HANDLE,WAIT_LIMIT> m_wait_handles ; // quit, start, stop, eventloop-handle, ...
	std::array<std::size_t,WAIT_LIMIT> m_indexes ; // -1, -1, -1, eventloop-index, ...

	std::thread m_thread ;
} ;

class GNet::EventLoopHandlesMt : public GNet::EventLoopHandlesBase
{
public:
	using RcType = EventLoopHandlesRcType ;
	using Rc = EventLoopHandlesRc ;

	explicit EventLoopHandlesMt( const EventLoopConfig & config ) ;
		// Constructor.

	~EventLoopHandlesMt() override ;
		// Destructor.

	static std::size_t capacityLimit( EventLoopConfig ) noexcept ;
		// Returns the capacity limit.

public: // overrides
	Rc wait( DWORD ms ) override ;
	void update( std::size_t list_size , std::function<HANDLE()> list_fn , bool updated ) override ;
	void onClose( HANDLE ) override ;
	bool overflow( std::size_t list_size , std::function<std::size_t()> list_size_fn ) const override ;

private:
	void addThread() ;
	void loadHandles( std::size_t , std::function<HANDLE()> ) ;
	static EventLoopConfig sanitise( EventLoopConfig config ) noexcept ;
	static void setEvent( HANDLE ) ;
	static void resetEvent( HANDLE ) ;
	static void waitFor( HANDLE , bool = true ) ;

private:
	template <std::size_t margin> struct GridPosition // row-column iterator
	{
		explicit GridPosition( std::size_t offset_limit_in ) :
			offset_limit(offset_limit_in)
		{
		}
		std::size_t offset_limit ;
		std::size_t index {0U} ;
		std::size_t offset {margin} ;
		void operator++()
		{
			offset++ ;
			if( offset == offset_limit )
			{
				offset = margin ;
				index++ ;
			}
		}
		bool isLhs() const
		{
			return offset == margin ;
		}
		void toLhs()
		{
			if( !isLhs() )
				nextRow() ;
		}
		std::size_t width( std::size_t remainder ) const
		{
			G_ASSERT( offset == margin ) ;
			return std::min( remainder+margin , offset_limit ) ;
		}
		std::size_t nextRow()
		{
			G_ASSERT( offset_limit > offset ) ;
			std::size_t addend = offset_limit - offset ;
			offset = margin ;
			index++ ;
			return addend ;
		}
	} ;

private:
	using Position = GridPosition<EventLoopThread::margin> ;
	EventLoopConfig m_config ;
	bool m_overflow {false} ;
	std::size_t m_capacity {0U} ;
	const std::size_t m_capacity_limit ;
	std::vector<std::unique_ptr<EventLoopThread>> m_threads ;
	std::vector<HANDLE> m_thread_handles ; // indication handles (shuffled)
	std::vector<std::size_t> m_thread_indexes ; // (shuffled)
	std::vector<HANDLE> m_eventloop_handles ;
} ;

std::unique_ptr<GNet::EventLoopHandlesBase> GNet::EventLoopHandlesImp::newEventLoopHandlesMt( const EventLoopConfig & config )
{
	return std::make_unique<EventLoopHandlesMt>( config ) ;
}

GNet::EventLoopHandlesMt::EventLoopHandlesMt( const EventLoopConfig & config ) :
	m_config(sanitise(config)) ,
	m_capacity_limit(capacityLimit(m_config))
{
	addThread() ;
	addThread() ;
}

GNet::EventLoopHandlesMt::~EventLoopHandlesMt()
{
	for( const auto & thread : m_threads )
		SetEvent( thread->m_hquitreq ) ;
	for( auto & thread : m_threads )
		thread->m_thread.join() ;
}

GNet::EventLoopConfig GNet::EventLoopHandlesMt::sanitise( EventLoopConfig config ) noexcept
{
	config.mt_wait_limit = std::max( config.mt_wait_limit , EventLoopThread::margin+1U ) ;
	return config ;
}

void GNet::EventLoopHandlesMt::update( std::size_t list_size , std::function<HANDLE()> list_fn , bool updated )
{
	if( list_size > m_capacity_limit )
		m_overflow = true ;
	if( m_overflow )
		return ;

	if( updated )
	{
		// load handles from the event-loop
		m_eventloop_handles.resize( list_size ) ;
		for( std::size_t i = 0U ; i < list_size ; i++ )
			m_eventloop_handles[i] = list_fn() ;

		// make enough threads
		while( list_size > m_capacity )
			addThread() ;

		// identify threads that need updating
		if( m_config.update_all )
		{
			for( auto & t : m_threads )
				t->m_stop = true ;
		}
		else
		{
			Position pos( m_config.mt_wait_limit ) ;
			for( std::size_t i = 0U ; i < list_size ; )
			{
				G_ASSERT( pos.index < m_threads.size() ) ;
				EventLoopThread & t = *m_threads.at( pos.index ) ;
				if( ( pos.isLhs() && pos.width(list_size-i) != t.m_thread_handle_count ) ||
					t.m_thread_handles.at(pos.offset) != m_eventloop_handles[i] )
				{
					t.m_stop = true ;
					i += pos.nextRow() ;
				}
				else
				{
					i++ ;
					++pos ;
				}
			}
			for( pos.toLhs() ; pos.index < m_threads.size() ; pos.nextRow() )
			{
				EventLoopThread & t = *m_threads.at( pos.index ) ;
				if( t.m_thread_handle_count != EventLoopThread::margin )
					t.m_stop = true ;
			}
		}

		// 'stop' the threads so they can be updated
		for( auto & thread_ptr : m_threads )
		{
			if( thread_ptr->m_stop )
				setEvent( thread_ptr->m_hstopreq ) ;
		}
		for( auto & thread_ptr : m_threads )
		{
			if( thread_ptr->m_stop )
				waitFor( thread_ptr->m_hstopcon ) ;
		}

		// update each stopped thread's handles
		Position pos( m_config.mt_wait_limit ) ;
		for( std::size_t i = 0U ; i < list_size ; )
		{
			G_ASSERT( pos.index < m_threads.size() ) ;
			EventLoopThread & t = *m_threads.at( pos.index ) ;
			if( t.m_stop )
			{
				HANDLE h = m_eventloop_handles[i] ;
				if( pos.isLhs() )
				{
					t.m_thread_handle_count = pos.width( list_size - i ) ;
					t.m_wait_handle_count = t.m_thread_handle_count ;
				}
				t.m_thread_handles.at(pos.offset) = h ;
				t.m_wait_handles[pos.offset] = h ;
				t.m_indexes[pos.offset] = i ;
				i++ ;
				++pos ;
			}
			else
			{
				i += pos.nextRow() ;
			}
		}
		for( pos.toLhs() ; pos.index < m_threads.size() ; pos.nextRow() )
		{
			EventLoopThread & t = *m_threads.at( pos.index ) ;
			t.m_thread_handle_count = EventLoopThread::margin ;
			if( t.m_stop )
				t.m_wait_handle_count = EventLoopThread::margin ;
		}
	}

	// 'start' the 'stopped' threads
	for( auto & thread_ptr : m_threads )
	{
		if( thread_ptr->m_stop )
			setEvent( thread_ptr->m_hstartreq ) ;
	}
	for( auto & thread_ptr : m_threads )
	{
		if( thread_ptr->m_stop )
		{
			thread_ptr->m_stop = false ;
			waitFor( thread_ptr->m_hstartcon ) ;
		}
	}
}

void GNet::EventLoopHandlesMt::addThread()
{
	int thread_id = static_cast<int>( m_threads.size() ) ;
	m_threads.emplace_back( std::make_unique<EventLoopThread>(thread_id) ) ;
	m_thread_handles.push_back( m_threads.back()->m_heventind ) ;
	m_thread_indexes.push_back( m_threads.size()-1U ) ;
	m_capacity += (m_config.mt_wait_limit - EventLoopThread::margin) ;
	if( m_threads.size() > 1U )
		G_LOG( "GNet::EventLoopHandlesMt::ctor: event loop using " << m_threads.size() << " threads" ) ;
}

GNet::EventLoopHandlesRc GNet::EventLoopHandlesMt::wait( DWORD ms )
{
	namespace imp = EventLoopHandlesImp ;
	if( m_overflow )
		return { RcType::overflow } ;

	DWORD handles_n = static_cast<DWORD>( m_thread_handles.size() ) ;
	HANDLE * handles_p = m_thread_handles.data() ;
	DWORD rc = MsgWaitForMultipleObjectsEx( handles_n , handles_p , ms , QS_ALLINPUT , 0 ) ;
	if( rc == WAIT_TIMEOUT )
	{
		return { RcType::timeout } ;
	}
	else if( rc >= WAIT_OBJECT_0 && rc < (WAIT_OBJECT_0+handles_n) )
	{
		std::size_t offset = static_cast<std::size_t>(rc-WAIT_OBJECT_0) ;
		std::size_t thread_index = m_thread_indexes.at( offset ) ;
		std::size_t event_handle_index = m_threads.at(thread_index)->m_event_value ;
		m_threads.at(thread_index)->m_stop = true ;

		if( event_handle_index == ~std::size_t(0) ) // thread died
			return Rc::failure( ERROR_HANDLE_EOF ) ;

		G_ASSERT( event_handle_index < m_eventloop_handles.size() ) ;
		if( event_handle_index >= m_eventloop_handles.size() )
			return Rc::failure( ERROR_GEN_FAILURE ) ;

		resetEvent( m_thread_handles[offset] ) ;

		imp::shuffle( m_thread_handles , m_thread_indexes , offset ) ;

		return { RcType::event , event_handle_index } ;
	}
	else if( rc == (WAIT_OBJECT_0+handles_n) )
	{
		return { RcType::message } ;
	}
	else // WAIT_FAILED
	{
		return Rc::failure( GetLastError() ) ;
	}
}

void GNet::EventLoopHandlesMt::onClose( HANDLE h )
{
	// stop the relevant thread early so that it is never
	// waiting on a handle that has now been closed -- this
	// is not necessary for winsock handles, but it is not
	// clear whether it might needed be for other handle types
	//
	for( auto & thread_ptr : m_threads )
	{
		if( !thread_ptr->m_stop )
		{
			std::size_t n = thread_ptr->m_thread_handle_count ;
			HANDLE * start = thread_ptr->m_thread_handles.data() ;
			HANDLE * end = start + n ;
			HANDLE * p = std::find( start , end , h ) ;
			if( p != end )
			{
				setEvent( thread_ptr->m_hstopreq ) ;
				waitFor( thread_ptr->m_hstopcon ) ;
				thread_ptr->m_stop = true ;
				break ;
			}
		}
	}
}

bool GNet::EventLoopHandlesImp::overflowMt( const EventLoopConfig & config , std::size_t list_size , std::function<std::size_t()> list_size_fn )
{
	return list_size > EventLoopHandlesMt::capacityLimit(config) && list_size_fn() > EventLoopHandlesMt::capacityLimit(config) ;
}

bool GNet::EventLoopHandlesMt::overflow( std::size_t list_size , std::function<std::size_t()> list_size_fn ) const
{
	return list_size > m_capacity_limit && list_size_fn() > m_capacity_limit ;
}

std::size_t GNet::EventLoopHandlesMt::capacityLimit( EventLoopConfig config ) noexcept
{
	config = sanitise( config ) ;
	return config.mt_thread_limit * (config.mt_wait_limit - EventLoopThread::margin) ;
}

HANDLE GNet::EventLoopHandlesImp::createEvent() noexcept
{
	return CreateEventW( NULL , TRUE , FALSE , NULL ) ;
}

void GNet::EventLoopHandlesMt::setEvent( HANDLE h )
{
	if( !SetEvent( h ) )
		throw EventLoop::Error( "set-event failed" ) ;
}

void GNet::EventLoopHandlesMt::resetEvent( HANDLE h )
{
	if( !ResetEvent( h ) )
		throw EventLoop::Error( "reset-event failed" ) ;
}

void GNet::EventLoopHandlesMt::waitFor( HANDLE h , bool with_reset )
{
	DWORD rc = WaitForSingleObject( h , 60000 ) ;
	if( rc != WAIT_OBJECT_0 )
		throw EventLoop::Error( "wait-event failed" ) ;
	if( with_reset )
		resetEvent( h ) ;
}

// --

GNet::EventLoopThread::EventLoopThread( int id ) :
	m_id(id) ,
	m_hquitreq(createEvent()) ,
	m_hstartreq(createEvent()) ,
	m_hstartcon(createEvent()) ,
	m_hstopreq(createEvent()) ,
	m_hstopcon(createEvent()) ,
	m_heventind(createEvent())
{
	static_assert( margin == 3U , "" ) ;
	m_thread_handles[0] = 0 ;
	m_thread_handles[1] = 0 ;
	m_thread_handles[2] = 0 ;
	m_thread_handle_count = margin ;

	m_wait_handles[0] = m_hquitreq ;
	m_wait_handles[1] = m_hstartreq ;
	m_wait_handles[2] = m_hstopreq ;
	m_wait_handle_count = margin ;

	m_indexes[0] = ~(std::size_t(0U)) ;
	m_indexes[1] = ~(std::size_t(0U)) ;
	m_indexes[2] = ~(std::size_t(0U)) ;

	m_stopped = true ;
	m_thread = std::thread( std::bind(&EventLoopThread::run,this,G::LogOutput::Instance::config(),G::LogOutput::Instance::fd()) ) ;
}

GNet::EventLoopThread::~EventLoopThread()
{
	CloseHandle( m_hquitreq ) ;
	CloseHandle( m_hstartreq ) ;
	CloseHandle( m_hstartcon ) ;
	CloseHandle( m_hstopreq ) ;
	CloseHandle( m_hstopcon ) ;
	CloseHandle( m_heventind ) ;
}

void GNet::EventLoopThread::run( G::LogOutput::Config /*log_output_config*/ , int /*log_output_fd*/ ) noexcept
{
	//G::LogOutput log_output( "thread-"+std::to_string(m_id) , log_output_config , log_output_fd ) ;
	m_failed = false ;
	while( !m_failed )
	{
		DWORD handles_n = static_cast<DWORD>( m_stopped ? margin : m_wait_handle_count ) ;
		DWORD rc = WaitForMultipleObjects( handles_n , m_wait_handles.data() , FALSE , INFINITE ) ;
		std::size_t offset = static_cast<std::size_t>(rc-WAIT_OBJECT_0) ;
		if( rc < WAIT_OBJECT_0 || rc >= (WAIT_OBJECT_0+handles_n) )
		{
			m_failed = true ;
		}
		else if( offset == 0U ) // quitreq
		{
			break ;
		}
		else if( offset == 1U ) // startreq
		{
			m_stopped = false ;
			if( !ResetEvent( m_heventind ) ) m_failed = true ;
			if( !ResetEvent( m_wait_handles[1] ) ) m_failed = true ;
			if( !SetEvent( m_hstartcon ) ) m_failed = true ;
		}
		else if( offset == 2U ) // stopreq
		{
			m_stopped = true ;
			if( !ResetEvent( m_heventind ) ) m_failed = true ;
			if( !ResetEvent( m_wait_handles[2] ) ) m_failed = true ;
			if( !SetEvent( m_hstopcon ) ) m_failed = true ;
		}
		else if( offset < handles_n )
		{
			m_event_value = m_indexes[offset] ;
			m_stopped = true ;
			shuffle( m_wait_handles.data() , m_indexes.data() , m_wait_handle_count , offset ) ;
			if( !SetEvent( m_heventind ) ) m_failed = true ;
		}
		else
		{
			m_failed = true ;
		}
	}
	if( m_failed )
	{
		m_event_value = ~std::size_t(0) ;
		SetEvent( m_heventind ) ;
	}
}

void GNet::EventLoopThread::shuffle( HANDLE * handles , std::size_t * indexes ,
	std::size_t size , std::size_t offset ) noexcept
{
	if( (offset+1U) < size )
	{
		std::rotate( handles+offset , handles+offset+1U , handles+size ) ;
		std::rotate( indexes+offset , indexes+offset+1U , indexes+size ) ;
	}
}

HANDLE GNet::EventLoopThread::createEvent() noexcept
{
	return EventLoopHandlesImp::createEvent() ;
}

