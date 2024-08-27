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
/// \file geventloophandles.h
///

#ifndef G_NET_EVENTLOOPHANDLES_H
#define G_NET_EVENTLOOPHANDLES_H

#include "gdef.h"
#include "geventloop.h"
#include <memory>
#include <vector>
#include <functional>

namespace GNet
{
	class EventLoopHandles ;
	enum class EventLoopHandlesRcType
	{
		timeout ,
		event ,
		message ,
		failed ,
		overflow
	} ;
	struct EventLoopHandlesRc
	{
		using RcType = EventLoopHandlesRcType ;
		EventLoopHandlesRc( RcType type , std::size_t index = 0U ) noexcept : m_type(type) , m_index(index) , m_error(0) {}
		static EventLoopHandlesRc failure( DWORD error ) noexcept { EventLoopHandlesRc rc(RcType::failed) ; rc.m_error = error ; return rc ; }
		RcType type() const noexcept { return m_type ; }
		operator RcType () const noexcept { return m_type ; }
		std::size_t index() const noexcept { return m_index ; }
		//
		RcType m_type ;
		std::size_t m_index ; // ListItem index
		DWORD m_error ; // if 'failed'
	} ;
	struct EventLoopHandlesBase
	{
		using RcType = EventLoopHandlesRcType ;
		using Rc = EventLoopHandlesRc ;
		virtual ~EventLoopHandlesBase() ;
		virtual Rc wait( DWORD ms ) = 0 ;
		virtual void update( std::size_t list_size , std::function<HANDLE()> list_fn , bool full_update ) = 0 ;
		virtual bool overflow( std::size_t list_size , std::function<std::size_t()> list_size_fn ) const = 0 ;
		virtual void onClose( HANDLE ) = 0 ;
	} ;
	struct EventLoopConfig
	{
		EventLoopConfig() ;
		bool st_only ;
		bool update_all ;
		std::size_t st_wait_limit ;
		std::size_t mt_wait_limit ;
		std::size_t mt_thread_limit ;
	} ;
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
///   handles.update( m_list.size() , [&](){m_list...} ) ;
///   for(;;)
///   {
///     auto rc = handles.wait( timeout() ) ;
///     if( rc == RcType::event )
///     {
///       handleEvent( m_list[rc.index()] ) ;
///     }
///     if( m_list.isDirty() )
///       m_list.collectGarbage() ;
///     handles.update( m_list.size() , [&](){m_list...} , m_list.wasDirty() ) ;
///   }
/// }
/// void add( HANDLE h )
/// {
///   m_list.push_back( {h,...} ) ;
///   if( handles.overflow( m_list.size() , [&](){m_list...} ) ) { m_list.pop_back() ; throw ... } ;
/// }
/// void remove( HANDLE h )
/// {
///    handles.onClose( h ) ;
/// }
/// \endcode
///
class GNet::EventLoopHandles
{
public:
	using RcType = EventLoopHandlesRcType ;
	using Rc = EventLoopHandlesRc ;

	EventLoopHandles() ;
		///< Constructor.

	~EventLoopHandles() ;
		///< Destructor.

	Rc wait( DWORD ms ) ;
		///< Waits for an event on any of the handles, up to some time
		///< limit. Returns an enumerated result together with the index
		///< of the first handle with an event.

	void update( std::size_t list_size , std::function<HANDLE()> list_fn , bool full_update = true ) ;
		///< Copies in a fresh set of handles from the event-loop list.
		///< The list must be freshly garbage-collected so that all
		///< the handles are valid. This is called after every
		///< wait() once any returned event has been fully handled.
		///< If the list has changed as a result of handling the event
		///< then 'full-update' should be set to true.

	void onClose( HANDLE ) ;
		///< Called when a handle is about to be closed.

	bool overflow( std::size_t list_size_ceiling , std::function<std::size_t()> list_size_fn ) const ;
		///< Returns true if the number of entries in the event-loop list
		///< would cause an overflow. The first parameter is the total
		///< list size possibly including invalid handles that will be
		///< garbage-collected, and the second parameter is a possibly-slow
		///< function that returns the exact number of valid handles.
		///<
		///< The event loop should use this immediately after adding an
		///< item to the list and not just wait for the next go-round.
		///< This allows the overflow exception to be handled cleanly
		///< and in-context rather than having the next wait() return
		///< an overflow error and terminate the application.

private:
	EventLoopConfig m_config ;
	std::unique_ptr<EventLoopHandlesBase> m_mt ;
	std::vector<HANDLE> m_handles ;
	std::vector<std::size_t> m_indexes ;
} ;

#endif
