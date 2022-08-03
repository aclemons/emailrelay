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
/// \file geventloophandles.h
///

#ifndef G_NET_EVENTLOOPHANDLES_H
#define G_NET_EVENTLOOPHANDLES_H

#include "gdef.h"
#include "geventloop.h"
#include "glog.h"
#include "gassert.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <sstream>

namespace GNet
{
	class EventLoopHandles ;
	class EventLoopHandlesMT ;
	class WaitThread ;

	DWORD WINAPI threadFn( LPVOID arg ) ;
	template <typename T> static void moveToRhs( T , std::size_t , std::size_t ) ;
	static constexpr std::size_t wait_limit = (MAXIMUM_WAIT_OBJECTS-1) ; // sic (beware bad documentation)
	static constexpr std::size_t wait_threads = 20U ; // tweakable up to ~30, 20*62=1240
	static constexpr std::size_t main_thread_wait_limit = wait_limit ;
	static constexpr std::size_t wait_thread_wait_limit = (MAXIMUM_WAIT_OBJECTS-1) ;
	static_assert( (wait_threads*2U) <= main_thread_wait_limit , "" ) ;
}

//| \class GNet::EventLoopHandles
/// Wraps WaitForMultipleObjects(), holding an array of Windows handles.
/// The handles are obtained from a list of event-emitting items
/// maintained by the Windows event-loop implementation.
///
/// \code
/// List m_list ;
/// void run()
/// {
///   Handles handles ;
///   handles.init( m_list ) ;
///   for(;;)
///   {
///     if( handles.overflow( m_list.size() ) ) throw ;
///     auto rc = handles.waitForMultipleObjects( timeout() ) ;
///     if( rc == RcType::event )
///     {
///       auto i = handles.shuffle( m_list , rc ) ;
///       handleEvent( m_list[i] ) ;
///     }
///     else if( rc == RcType::other )
///     {
///       handles.handleInternalEvent( rc.index() ) ;
///     }
///     if( m_list.isDirty() )
///       m_list.collectGarbage() ;
///     handles.update( m_list , m_list.wasDirty() , rc ) ;
///   }
/// }
/// void add( HANDLE h )
/// {
///   m_list.push_back( {h,...} ) ;
///   if( handles.overflow( m_list , ... ) ) throw ... ;
/// }
/// \endcode
///
/// This class allows for a multi-threaded implementation supporting
/// more than 63 handles. It initially uses a trivial single-threaded
/// implementation (with a null 'm_mt' data member), but switches
/// to a multi-threaded implementation when there are too many open
/// handles.
///
/// From WaitForMultipleObjects() 'Remarks':
/// "To wait on more than MAXIMUM_WAIT_OBJECTS handles, used
/// one of the following methods: (1) Create a thread to wait
/// on MAXIMUM_WAIT_OBJECTS handles, then wait on that thread
/// plus the other handles. Use this technique to break the
/// handles into groups of MAXIMUM_WAIT_OBJECTS. (2) ..."
///
class GNet::EventLoopHandles
{
public:
	enum class RcType // A type enumeration for GNet::EventLoopImp::Rc.
	{
		timeout ,
		event ,
		message ,
		failed ,
		other
	}  ;
	struct Rc /// A structure for the return code from WaitForMultiplObjects().
	{
		Rc( RcType type , std::size_t index = 0U , std::size_t imp_1 = 0U , std::size_t imp_2 = 0U ) noexcept ;
		inline RcType type() const noexcept ;
		inline operator RcType () const noexcept { return m_type ; } // (inline definition for msvc)
		inline std::size_t index() const noexcept ;
		RcType m_type ;
		std::size_t m_index ; // ListItem index
		std::size_t m_imp_1 ; // optional value used by EventLoopHandles implementation
		std::size_t m_imp_2 ; // optional value used by EventLoopHandles implementation
	} ;

public:
	template <typename TList> EventLoopHandles( TList & , std::size_t threads ) ;
		///< Constructor. The implementation might populate the list with
		///< an initial set of handles of type 'other' for internal use.

	~EventLoopHandles() ;
		///< Destructor.

	template <typename TList> void init( TList & ) ;
		///< Initialises the handles from the event-loop list.

	template <typename TList> void update( TList & , bool full_update , Rc rc ) ;
		///< Copies in a fresh set of handles from the event-loop list.
		///< The list must be freshly garbage-collected so that all
		///< the handles are valid. This is called after every
		///< waitForMultipleObjects() once any returned event has
		///< been fully handled. If the list has changed as a result
		///< of handling the event then 'full-update' should be set
		///< to true, along with the index of the event that has just
		///< been handled in 'rc'.

	template <typename TList> bool overflow( TList & list , bool (*valid_fn)(const typename TList::value_type&) ) ;
		///< Returns true if the number of valid entries in the event-loop
		///< list would cause an overflow, using the given function to
		///< ignore list items that are going to be garbage collected.
		///< The last item on the list is considered to be valid,
		///< regardless of what the tester function says.
		///<
		///< The event loop should use this immediately after adding
		///< an item to the list and not just wait for the next go-round.
		///< This allows the error condition to be handled cleanly
		///< without terminating the event-loop and exiting main().
		///<
		///< This overload allows the implementation to switch over
		///< automatically on first overflow to an implementation
		///< that supports more handles.

	bool overflow( std::size_t ) const ;
		///< An overload taking the number of valid entries in the
		///< event-loop list. This overload does not allow the
		///< implementation to switch over.

	template <typename TList> std::string help( const TList & , bool on_add ) const ;
		///< Returns a helpful explanation for overflow().

	Rc waitForMultipleObjects( DWORD ms ) ;
		///< Waits for an event on any of the handles, up to some time
		///< limit. Returns an enumerated result together with the index
		///< of the first handle with an event (where relevant).

	template <typename TList> std::size_t shuffle( TList & , Rc rc ) ;
		///< Shuffles the external event-loop list and the internal
		///< handles as necessary to prevent starvation. Returns
		///< the new list index of the current event after shuffling
		///< (see Rc::index()).

	void handleInternalEvent( std::size_t list_index ) ;
		///< Called when the current event comes from a handle that was
		///< not added to the list by the event-loop, ie. type 'other'.

private:
	template <typename TList> void init_MT( TList & ) ;
	template <typename TList> void update_MT( TList & , bool , Rc ) ;
	template <typename TList> bool overflow_MT( TList & , bool (*fn)(const typename TList::value_type&) ) ;
	bool overflow_MT( std::size_t ) const ;
	template <typename TList> std::string help_MT( const TList & , bool ) const ;
	Rc waitForMultipleObjects_MT( DWORD ms ) ;
	template <typename TList> std::size_t shuffle_MT( TList & , Rc ) ;
	void handleInternalEvent_MT( std::size_t ) ;

private:
	std::vector<HANDLE> m_handles ;
	std::unique_ptr<EventLoopHandlesMT> m_mt ;
} ;

class GNet::WaitThread
{
public:
	static constexpr std::size_t wait_limit = wait_thread_wait_limit ;
	static constexpr DWORD wait_timeout_ms = 60000U ;

public:
	WaitThread( std::size_t id , std::size_t list_offset ) ;
	~WaitThread() ;
	DWORD run() ;
	HANDLE hindicate() const ;
	HANDLE hthread() const ;
	std::size_t id() const noexcept ;
	std::size_t indication() ;
	template <typename Iterator> void markIfDifferent( std::size_t , Iterator , std::size_t n ) ;
	template <typename Iterator> void updateIfMarked( std::size_t t , Iterator , std::size_t n ) ;
	void start() ;
	void stop() ;
	std::size_t listOffset() const ;
	void shuffle( std::size_t ) ;
	void mark() noexcept ;
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
		CriticalSection() noexcept ;
		~CriticalSection() ;
		CRITICAL_SECTION m_cs ;
	} ;
	struct Lock
	{
		Lock( CriticalSection & cs ) noexcept ;
		~Lock() ;
		void enter() noexcept ;
		void leave() noexcept ;
		CriticalSection & m_cs ;
	} ;
	struct Event
	{
		Event() noexcept ;
		~Event() ;
		bool isSet() const noexcept ;
		void set() noexcept ;
		void clear() noexcept ;
		void wait( Lock & lock ) ;
		static bool wait( Lock & lock , Event & a , Event & b ) ;
		HANDLE h() const noexcept ;
		const HANDLE m_handle ;
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

class GNet::EventLoopHandlesMT
{
public:
	using Rc = EventLoopHandles::Rc ;
	using RcType = EventLoopHandles::RcType ;

public:
	explicit EventLoopHandlesMT( std::size_t ) ;
	~EventLoopHandlesMT() ;
	template <typename TList> void init( TList & ) ;
	template <typename TList> void update( TList & , bool , Rc ) ;
	template <typename TList> bool overflow( TList & , bool (*fn)(const typename TList::value_type&) ) const ;
	bool overflow( std::size_t ) const noexcept ;
	template <typename TList> std::string help( const TList & , bool ) const ;
	Rc waitForMultipleObjects( DWORD ) ;
	template <typename TList> std::size_t shuffle( TList & , Rc ) ;
	void handleInternalEvent( std::size_t ) ;

public:
	EventLoopHandlesMT( const EventLoopHandlesMT & ) = delete ;
	EventLoopHandlesMT( EventLoopHandlesMT && ) = delete ;
	void operator=( const EventLoopHandlesMT & ) = delete ;
	void operator=( EventLoopHandlesMT && ) = delete ;

private:
	void startMarkedThreads() ;
	void stopMarkedThreads() ;
	template <typename TList, typename TUpdateFn> void forEachThread( TUpdateFn , TList & ) ;
	void startCurrentThread( Rc ) ;
	void markCurrentThread( Rc ) ;

private:
	std::vector<HANDLE> m_handles ; // hthread-s then hind-s
	std::vector<std::size_t> m_index ; // map from m_handles position to m_thread index
	std::vector<std::unique_ptr<WaitThread>> m_threads ;
	std::size_t m_list_limit{0U} ;
	bool m_full_update{true} ;
} ;

// ===

inline
GNet::EventLoopHandles::Rc::Rc( RcType type , std::size_t index , std::size_t imp_1 , std::size_t imp_2 ) noexcept :
	m_type(type) ,
	m_index(index) ,
	m_imp_1(imp_1) ,
	m_imp_2(imp_2)
{
}

inline
GNet::EventLoopHandles::RcType GNet::EventLoopHandles::Rc::type() const noexcept
{
	return m_type ;
}

inline
std::size_t GNet::EventLoopHandles::Rc::index() const noexcept
{
	return m_index ;
}

// --

inline
GNet::WaitThread::CriticalSection::CriticalSection() noexcept
{
	InitializeCriticalSection( &m_cs ) ;
}

inline
GNet::WaitThread::CriticalSection::~CriticalSection()
{
	DeleteCriticalSection( &m_cs ) ;
}

// --

inline
GNet::WaitThread::Lock::Lock( CriticalSection & cs ) noexcept :
	m_cs(cs)
{
	enter() ;
}

inline
GNet::WaitThread::Lock::~Lock()
{
	leave() ;
}

// --

template <typename TList>
GNet::EventLoopHandles::EventLoopHandles( TList & list , std::size_t threads )
{
	if( threads )
		m_mt = std::make_unique<EventLoopHandlesMT>( threads ) ;
}

template <typename TList>
void GNet::EventLoopHandles::init( TList & list )
{
	if( m_mt )
	{
		init_MT( list ) ;
	}
	else
	{
		m_handles.resize( list.size() ) ;
		for( std::size_t i = 0 ; i < list.size() ; i++ )
			m_handles[i] = list[i].m_handle ;
	}
}

template <typename TList>
void GNet::EventLoopHandles::update( TList & list , bool updated , Rc rc )
{
	if( m_mt )
	{
		update_MT( list , updated , rc ) ;
	}
	else if( updated )
	{
		m_handles.resize( list.size() ) ;
		for( std::size_t i = 0 ; i < list.size() ; i++ )
			m_handles[i] = list[i].m_handle ;
	}
}

template <typename TList>
bool GNet::EventLoopHandles::overflow( TList & list , bool (*fn)(const typename TList::value_type&) )
{
	if( m_mt )
	{
		return overflow_MT( list , fn ) ;
	}
	else if( list.size() > wait_limit &&
		(1U+std::count_if(list.cbegin(),list.cbegin()+list.size()-1U,fn)) > wait_limit )
	{
		// allow automatic implementation switcheroo on first overflow
		return overflow_MT( list , fn ) ;
	}
	else
	{
		return false ;
	}
}

template <typename TList>
std::string GNet::EventLoopHandles::help( const TList & list , bool on_add ) const
{
	if( m_mt )
	{
		return help_MT( list , on_add ) ;
	}
	else
	{
		return "too many open handles" ;
	}
}

template <typename TList>
void GNet::EventLoopHandles::init_MT( TList & list )
{
	m_mt->init( list ) ;
}

template <typename TList>
void GNet::EventLoopHandles::update_MT( TList & list , bool updated , Rc rc )
{
	m_mt->update( list , updated , rc ) ;
}

template <typename TList>
bool GNet::EventLoopHandles::overflow_MT( TList & list , bool (*fn)(const typename TList::value_type&) )
{
	if( !m_mt )
	{
		G_LOG( "GNet::EventLoopHandles: large number of open handles: switching event-loop" ) ;
		m_mt = std::make_unique<EventLoopHandlesMT>( wait_threads ) ;

		///< no need for HandlesImp::init() here because there will soon be garbage
		///< collection of the list and a full update() -- the new threads will
		///< start by waiting on an empty set of event-loop handles plus
		///< their stop event
	}
	return m_mt->overflow( list , fn ) ;
}

inline
bool GNet::EventLoopHandles::overflow_MT( std::size_t n ) const
{
	return m_mt->overflow( n ) ;
}

template <typename TList>
std::string GNet::EventLoopHandles::help_MT( const TList & list , bool on_add ) const
{
	return m_mt->help( list , on_add ) ;
}

template <typename TList>
std::size_t GNet::EventLoopHandles::shuffle_MT( TList & list , Rc rc )
{
	return m_mt->shuffle( list , rc ) ;
}

inline
GNet::EventLoopHandles::Rc GNet::EventLoopHandles::waitForMultipleObjects_MT( DWORD ms )
{
	return m_mt->waitForMultipleObjects( ms ) ;
}

inline
void GNet::EventLoopHandles::handleInternalEvent_MT( std::size_t index )
{
	m_mt->handleInternalEvent( index ) ;
}

template <typename TList>
std::size_t GNet::EventLoopHandles::shuffle( TList & list , Rc rc )
{
	if( m_mt )
	{
		return shuffle_MT( list , rc ) ;
	}
	else
	{
		G_ASSERT( !m_handles.empty() ) ;
		G_ASSERT( list.size() == m_handles.size() ) ;
		G_ASSERT( rc.index() < m_handles.size() ) ;
		std::size_t index = rc.index() ;
		std::vector<HANDLE> & handles = m_handles ;
		if( (index+1U) < handles.size() ) // if not already rightmost
		{
			std::size_t index_1 = index + 1U ;
			std::rotate( handles.begin()+index , handles.begin()+index_1 , handles.end() ) ;
			std::rotate( list.begin()+index , list.begin()+index_1 , list.end() ) ;
			index = handles.size() - 1U ;
		}
		return index ;
	}
}

// --

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

// --

template <typename Iterator>
void GNet::WaitThread::updateIfMarked( std::size_t t , Iterator list_p , std::size_t n )
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

template <typename Iterator>
void GNet::WaitThread::markIfDifferent( std::size_t , Iterator list_p , std::size_t n )
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

// --

template <typename TList>
void GNet::EventLoopHandlesMT::init( TList & list )
{
	update( list , true , Rc(RcType::other) ) ;
}

template <typename TList>
std::size_t GNet::EventLoopHandlesMT::shuffle( TList & list , Rc rc )
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

template <typename TList>
void GNet::EventLoopHandlesMT::update( TList & list , bool updated , Rc rc )
{
	// stop threads and hand out blocks of handles
	if( updated || m_full_update )
	{
		m_full_update = false ;
		forEachThread( &WaitThread::markIfDifferent<typename TList::iterator> , list ) ; // see what threads need restarting
		stopMarkedThreads() ;
		forEachThread( &WaitThread::updateIfMarked<typename TList::iterator> , list ) ;
		markCurrentThread( rc ) ;
		startMarkedThreads() ;
	}
	else
	{
		startCurrentThread( rc ) ;
	}
}

template <typename TList, typename TUpdateFn>
void GNet::EventLoopHandlesMT::forEachThread( TUpdateFn update_memfn , TList & list )
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

template <typename TList>
bool GNet::EventLoopHandlesMT::overflow( TList & list , bool (*fn)(const typename TList::value_type&) ) const
{
	if( list.empty() ) return false ;
	std::size_t list_count = std::count_if( list.cbegin() , list.cbegin()+list.size()-1U , fn ) + 1U ;
	return list_count > m_list_limit ;
}

template <typename TList>
std::string GNet::EventLoopHandlesMT::help( const TList & list , bool on_add ) const
{
	std::ostringstream ss ;
	ss << "too many open handles (" << list.size() << (on_add?"/":">") << m_list_limit << ")" ;
	return ss.str() ;
}

#endif
