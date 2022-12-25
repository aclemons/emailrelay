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
#include <array>
#include <algorithm>
#include <functional>
#include <sstream>

namespace GNet
{
	class EventLoopHandles ;
	class EventLoopHandlesMt ;
	class EventLoopHandlesMtImp ;
	class WaitThread ;

	DWORD WINAPI handlesThreadFn( LPVOID arg ) ;
	static constexpr std::size_t LIMIT = (MAXIMUM_WAIT_OBJECTS-1) ; // sic (beware bad documentation)
	static constexpr std::size_t THREADS = 20U ;
	static constexpr DWORD STACK_SIZE = 64000 ;
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
///     auto rc = handles.wait( timeout() ) ;
///     if( rc == RcType::event )
///     {
///       auto i = handles.shuffle( m_list , rc ) ;
///       handleEvent( m_list[i] ) ;
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
	struct Rc /// A structure for the return code from WaitForMultipleObjects().
	{
		Rc( RcType type , std::size_t index = 0U , std::size_t extra_1 = 0U , std::size_t extra_2 = 0U ) noexcept ;
		RcType type() const noexcept ;
		operator RcType () const noexcept { return m_type ; } // (inline definition for msvc)
		std::size_t index() const noexcept ;
		//
		RcType m_type ;
		std::size_t m_index ; // ListItem index
		std::size_t m_extra_hpos ; // opaque value: handle position
		std::size_t m_extra_tid ; // opaque value: thread id
	} ;

public:
	EventLoopHandles() ;
		///< Constructor. The implementation immediately after construction
		///< is single-threaded.

	~EventLoopHandles() ;
		///< Destructor.

	Rc wait( DWORD ms ) ;
		///< Waits for an event on any of the handles, up to some time
		///< limit. Returns an enumerated result together with the index
		///< of the first handle with an event.

	template <typename TList> void init( TList & ) ;
		///< Initialises the handles from the event-loop list.

	template <typename TList> void update( const TList & , bool full_update , Rc rc ) ;
		///< Copies in a fresh set of handles from the event-loop list.
		///< The list must be freshly garbage-collected so that all
		///< the handles are valid. This is called after every
		///< wait() once any returned event has been fully handled.
		///< If the list has changed as a result of handling the event
		///< then 'full-update' should be set to true, along with the
		///< index of the event that has just been handled in 'rc'.

	template <typename TList> std::size_t shuffle( TList & , Rc rc ) ;
		///< Shuffles the external event-loop list and the internal
		///< handles as necessary to prevent starvation. Returns
		///< the new list index of the current event after shuffling
		///< (see Rc::index()).

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

private:
	Rc wait_MT( DWORD ms ) ;
	template <typename TList> void init_MT( const TList & , bool (*fn)(const typename TList::value_type&) ) ;
	template <typename TList> void update_MT( const TList & , bool updated , std::size_t thread_index ) ;
	template <typename TList> std::size_t shuffle_MT( TList & , Rc ) ;
	template <typename TList> bool overflow_MT( TList & , bool (*fn)(const typename TList::value_type&) ) ;
	bool overflow_MT( std::size_t ) const ;
	template <typename TList> std::string help_MT( const TList & , bool ) const ;

private:
	std::vector<HANDLE> m_handles ;
	std::unique_ptr<EventLoopHandlesMt> m_mt ;
} ;

class GNet::WaitThread
{
public:
	struct Args
	{
		std::size_t id ;
		HANDLE sem ;
		HANDLE * h ; // null-terminated array
		HANDLE stop ;
		HANDLE req ;
		HANDLE ind ;
		HANDLE rsp ;
		HANDLE con ;
	} ;

public:
	explicit WaitThread( Args ) ;
	~WaitThread() ;
	std::size_t id() const noexcept ;
	const Args & args() const noexcept ;
	HANDLE hthread() const noexcept ;

public: //(private)
	DWORD run() ;

public:
	WaitThread() = delete ;
	WaitThread( const WaitThread & ) = delete ;
	WaitThread( WaitThread && ) = delete ;
	WaitThread & operator=( const WaitThread & ) = delete ;
	WaitThread & operator=( WaitThread && ) = delete ;

private:
	void copyHandles( HANDLE * ) noexcept ;

private:
	Args m_args ;
	HANDLE m_hthread ;
	HANDLE m_handles[LIMIT] ;
	DWORD m_handle_count ;
} ;

class GNet::EventLoopHandlesMt
{
public:
	using Rc = EventLoopHandles::Rc ;
	using RcType = EventLoopHandles::RcType ;
	struct HandlePtr /// output iterator for depositing handles
	{
		std::size_t i ;
		HANDLE * p ;
		explicit HandlePtr( HANDLE * start ) noexcept : i(2U) , p(start+2) {}
		HandlePtr operator++(int) noexcept { HandlePtr copy(*this) ; advance() ; return copy ; }
		HandlePtr & operator++() noexcept { advance() ; return *this ; }
		void advance() noexcept { ++p ; ++i ; if( i == LIMIT ) { p += 2 ; i = 2U ; } ; }
		HANDLE & operator*() noexcept { return *p ; }
	} ;

public:
	static bool enabled() noexcept ;
	EventLoopHandlesMt() ;
	~EventLoopHandlesMt() ;
	Rc wait( DWORD ) ;
	HandlePtr handles() noexcept ;
	void init() ;
	void update( bool updated , std::size_t thread_id ) ;
	std::size_t shuffle( Rc ) ;
	bool overflow( std::size_t ) const noexcept ;
	std::string help( bool on_add , std::size_t ) const ;

public:
	EventLoopHandlesMt( const EventLoopHandlesMt & ) = delete ;
	EventLoopHandlesMt( EventLoopHandlesMt && ) = delete ;
	EventLoopHandlesMt & operator=( const EventLoopHandlesMt & ) = delete ;
	EventLoopHandlesMt & operator=( EventLoopHandlesMt && ) = delete ;

private:
	void updateThreads() ;
	static bool isSet( HANDLE h ) noexcept ;
	template <typename T> static void moveToRhs( T begin , std::size_t i , std::size_t n ) ;

private:
	std::vector<std::unique_ptr<WaitThread>> m_threads ;
	std::vector<HANDLE> m_main_handles ; // hthread-s then hind-s
	std::vector<std::size_t> m_shuffle_map ; // thread id at each hind position
	bool m_a {true} ; // current/previous toggle
	std::array<HANDLE,THREADS*LIMIT> m_thread_handles_a ;
	std::array<HANDLE,THREADS*LIMIT> m_thread_handles_b ;
	std::array<std::size_t,THREADS> m_rr ;
	std::array<HANDLE,THREADS> m_rsp_handles ;
	std::array<HANDLE,THREADS> m_con_handles ;
	HANDLE m_semaphore ;
	// shared..
	std::array<HANDLE,THREADS*LIMIT> m_thread_handles ;
} ;

// ===

inline
GNet::EventLoopHandles::Rc::Rc( RcType type , std::size_t index , std::size_t extra_1 , std::size_t extra_2 ) noexcept :
	m_type(type) ,
	m_index(index) ,
	m_extra_hpos(extra_1) ,
	m_extra_tid(extra_2)
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

// ==

template <typename TList>
void GNet::EventLoopHandles::init( TList & list )
{
	G_ASSERT( m_mt.get() == nullptr ) ;
	m_handles.resize( list.size() ) ;
	std::size_t i = 0U ;
	for( const auto & list_item : list )
		m_handles[i++] = list_item.m_handle ;
}

inline
GNet::EventLoopHandles::Rc GNet::EventLoopHandles::wait( DWORD ms )
{
	if( m_mt )
	{
		return wait_MT( ms ) ;
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

template <typename TList>
void GNet::EventLoopHandles::update( const TList & list , bool updated , Rc rc )
{
	if( m_mt )
	{
		std::size_t thread_index = rc.m_extra_tid ;
		update_MT( list , updated , thread_index ) ;
	}
	else
	{
		if( updated )
		{
			m_handles.resize( list.size() ) ;
			std::size_t i = 0U ;
			for( const auto & list_item : list )
				m_handles[i++] = list_item.m_handle ;
		}
	}
}

inline
bool GNet::EventLoopHandles::overflow( std::size_t n ) const
{
	if( m_mt )
	{
		return overflow_MT( n ) ;
	}
	else
	{
		return n > LIMIT ;
	}
}

template <typename TList>
bool GNet::EventLoopHandles::overflow( TList & list , bool (*fn)(const typename TList::value_type&) )
{
	if( m_mt )
	{
		return overflow_MT( list , fn ) ;
	}
	else
	{
		bool is_overflow =
			list.size() > LIMIT &&
			static_cast<std::size_t>(1+std::count_if(list.cbegin(),list.cbegin()+list.size()-1U,fn)) > LIMIT ;

		if( is_overflow && !EventLoopHandlesMt::enabled() )
		{
			return true ;
		}
		else if( is_overflow )
		{
			// automatic implementation switcheroo on first overflow
			G_LOG( "GNet::EventLoopHandles: large number of open handles: switching event-loop" ) ;
			m_mt = std::make_unique<EventLoopHandlesMt>() ;
			init_MT( list , fn ) ;
			return false ;
		}
		else
		{
			return false ;
		}
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

// ==

template <typename TList>
void GNet::EventLoopHandles::init_MT( const TList & list , bool (*fn)(const typename TList::value_type&) )
{
	auto out = m_mt->handles() ;
	for( auto list_p = list.begin() ; list_p != list.end() ; ++list_p )
	{
		bool valid = std::next(list_p) == list.end() || fn(*list_p) ;
		if( valid )
			*out++ = (*list_p).m_handle ;
	}
	m_mt->init() ;
}

template <typename TList>
void GNet::EventLoopHandles::update_MT( const TList & list , bool updated , std::size_t thread_index )
{
	if( updated )
	{
		auto out = m_mt->handles() ;
		for( auto list_p = list.begin() ; list_p != list.end() ; ++list_p )
			*out++ = (*list_p).m_handle ;
	}
	m_mt->update( updated , thread_index ) ;
}

template <typename TList>
bool GNet::EventLoopHandles::overflow_MT( TList & list , bool (*fn)(const typename TList::value_type&) )
{
	std::size_t n = static_cast<std::size_t>(1+std::count_if(list.cbegin(),list.cbegin()+list.size()-1U,fn)) ;
	return m_mt->overflow( n ) ;
}

inline
bool GNet::EventLoopHandles::overflow_MT( std::size_t n ) const
{
	return m_mt->overflow( n ) ;
}

template <typename TList>
std::string GNet::EventLoopHandles::help_MT( const TList & list , bool on_add ) const
{
	return m_mt->help( on_add , list.size() ) ;
}

template <typename TList>
std::size_t GNet::EventLoopHandles::shuffle_MT( TList & , Rc rc )
{
	return m_mt->shuffle( rc ) ;
}

inline
GNet::EventLoopHandles::Rc GNet::EventLoopHandles::wait_MT( DWORD ms )
{
	return m_mt->wait( ms ) ;
}

#endif
