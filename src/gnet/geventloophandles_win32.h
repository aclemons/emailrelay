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
/// \file geventloophandles_win32.h
///

#ifndef G_NET_EVENTLOOPHANDLES_WIN32_H
#define G_NET_EVENTLOOPHANDLES_WIN32_H

#include "gdef.h"
#include "geventloop_win32.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace GNet
{
	class EventLoopHandles ;
	class EventLoopHandlesImp ;
	static constexpr std::size_t wait_limit = (MAXIMUM_WAIT_OBJECTS-1) ; // sic (beware bad documentation)
}

//| \class GNet::EventLoopHandles
/// Wraps WaitForMultipleObjects(), holding an array of Windows handles.
/// The handles are obtained from a list of event-emitting items
/// maintained by the Windows event-loop implementation.
///
/// This class is factored out in order to allow for a multi-threaded
/// implementation supporting more than 63 handles. The trivial
/// single-threaded implementation is inline in this header.
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
class GNet::EventLoopHandles
{
public:
	using List = EventLoopImp::List ;
	using ListItem = EventLoopImp::ListItem ;
	using Rc = EventLoopImp::Rc ;
	using RcType = EventLoopImp::RcType ;

public:
	EventLoopHandles( List & , std::size_t threads ) ;
		///< Constructor. The implementation might populate the list with
		///< an initial set of handles of type 'other' for internal use.

	~EventLoopHandles() ;
		///< Destructor.

	void init( List & ) ;
		///< Initialises the handles from the event-loop list.

	void update( List & , bool full_update , Rc rc ) ;
		///< Copies in a fresh set of handles from the event-loop list.
		///< The list must be freshly garbage-collected so that all
		///< the handles are valid. This is called after every
		///< waitForMultipleObjects() once any returned event has
		///< been fully handled. If the list has changed as a result
		///< of handling the event then 'full-update' should be set
		///< to true, along with the index of the event that has just
		///< been handled in 'rc'.

	bool overflow( List & list , bool (*valid_fn)(const ListItem&) ) ;
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

	std::string help( const List & , bool on_add ) const ;
		///< Returns a helpful explanation for overflow().

	Rc waitForMultipleObjects( DWORD ms ) ;
		///< Waits for an event on any of the handles, up to some time
		///< limit. Returns an enumerated result together with the index
		///< of the first handle with an event (where relevant).

	std::size_t shuffle( List & , Rc rc ) ;
		///< Shuffles the external event-loop list and the internal
		///< handles as necessary to prevent starvation. Returns
		///< the new list index of the current event after shuffling
		///< (see Rc::index()).

	void handleInternalEvent( std::size_t list_index ) ;
		///< Called when the current event comes from a handle that was
		///< not added to the list by the event-loop, ie. type 'other'.

private:
	void initImp( List & ) ;
	void updateImp( List & , bool , Rc ) ;
	bool overflowImp( List & , bool (*fn)(const ListItem&) ) ;
	bool overflowImp( std::size_t ) const ;
	std::string helpImp( const List & , bool ) const ;
	Rc waitForMultipleObjectsImp( DWORD ms ) ;
	std::size_t shuffleImp( List & , Rc ) ;
	void handleInternalEventImp( std::size_t ) ;

private:
	std::vector<HANDLE> m_handles ;
	std::unique_ptr<EventLoopHandlesImp> m_imp ;
} ;

inline
void GNet::EventLoopHandles::init( List & list )
{
	if( m_imp )
	{
		initImp( list ) ;
	}
	else
	{
		m_handles.resize( list.size() ) ;
		for( std::size_t i = 0 ; i < list.size() ; i++ )
			m_handles[i] = list[i].m_handle ;
	}
}

inline
void GNet::EventLoopHandles::update( List & list , bool updated , Rc rc )
{
	if( m_imp )
	{
		updateImp( list , updated , rc ) ;
	}
	else if( updated )
	{
		m_handles.resize( list.size() ) ;
		for( std::size_t i = 0 ; i < list.size() ; i++ )
			m_handles[i] = list[i].m_handle ;
	}
}

inline
bool GNet::EventLoopHandles::overflow( List & list , bool (*fn)(const ListItem&) )
{
	if( m_imp )
	{
		return overflowImp( list , fn ) ;
	}
	else if( list.size() > wait_limit &&
		(1U+std::count_if(list.cbegin(),list.cbegin()+list.size()-1U,fn)) > wait_limit )
	{
		// allow automatic implementation switcheroo on first overflow
		return overflowImp( list , fn ) ;
	}
	else
	{
		return false ;
	}
}

inline
bool GNet::EventLoopHandles::overflow( std::size_t n ) const
{
	if( m_imp )
	{
		return overflowImp( n ) ;
	}
	else
	{
		return n > wait_limit ;
	}
}

inline
std::string GNet::EventLoopHandles::help( const List & list , bool on_add ) const
{
	if( m_imp )
	{
		return helpImp( list , on_add ) ;
	}
	else
	{
		return "too many open handles" ;
	}
}

inline
GNet::EventLoopImp::Rc GNet::EventLoopHandles::waitForMultipleObjects( DWORD ms )
{
	if( m_imp )
	{
		return waitForMultipleObjectsImp( ms ) ;
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

inline
std::size_t GNet::EventLoopHandles::shuffle( List & list , Rc rc )
{
	if( m_imp )
	{
		return shuffleImp( list , rc ) ;
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

inline
void GNet::EventLoopHandles::handleInternalEvent( std::size_t index )
{
	if( m_imp )
		handleInternalEventImp( index ) ;
}

#endif
