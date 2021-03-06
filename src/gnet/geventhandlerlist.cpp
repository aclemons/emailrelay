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
// geventhandlerlist.cpp
//

#include "gdef.h"
#include "gnetdone.h"
#include "geventhandlerlist.h"
#include "geventloggingcontext.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

namespace GNet
{
	namespace EventHandlerListImp /// An implementation namespace for GNet::EventHandlerList.
	{
		struct fdless /// A functor that compares two file descriptors.
		{
			using Value = GNet::EventHandlerList::Value ;
			bool operator()( const Value & p1 , const Value & p2 ) noexcept
			{
				return p1.m_fd < p2.m_fd ;
			}
		} ;
	}
}

GNet::EventHandlerList::EventHandlerList( const std::string & type ) :
	m_type(type) ,
	m_lock(0U) ,
	m_has_garbage(false)
{
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler , ExceptionSink es )
{
	G_ASSERT( handler != nullptr ) ; if( handler == nullptr ) return ;
	G_ASSERT( es.eh() != nullptr ) ; if( es.eh() == nullptr ) return ;

	G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: " << "adding " << fd << (m_lock?" (pending)":"") ) ;
	addImp( m_lock?m_pending_list:m_list , fd , handler , es ) ;
}

void GNet::EventHandlerList::addImp( List & list , Descriptor fd , EventHandler * handler , ExceptionSink es )
{
	namespace imp = EventHandlerListImp ;
	using Range = std::pair<List::iterator,List::iterator> ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , imp::fdless() ) ;
	if( range.first == range.second )
		list.insert( range.first , List::value_type(fd,handler,es) ) ;
	else
		*range.first = List::value_type(fd,handler,es) ;
}

void GNet::EventHandlerList::remove( Descriptor fd ) noexcept
{
	G_DEBUG( "GNet::EventHandlerList::remove: " << m_type << "-list: " << "removing " << fd ) ;
	if( m_lock )
	{
		if( disable(m_list,fd) ) m_has_garbage = true ;
		disable( m_pending_list , fd ) ;
	}
	else
	{
		remove( m_list , fd ) ;
	}
}

bool GNet::EventHandlerList::disable( List & list , Descriptor fd ) noexcept
{
	namespace imp = EventHandlerListImp ;
	using Range = std::pair<List::iterator,List::iterator> ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , imp::fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
	{
		(*range.first).m_event_handler = nullptr ;
	}
	return found ;
}

bool GNet::EventHandlerList::remove( List & list , Descriptor fd ) noexcept
{
	namespace imp = EventHandlerListImp ;
	using Range = std::pair<List::iterator,List::iterator> ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , imp::fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
		list.erase( range.first ) ; // noexcept since Value::op=() does not throw
	return found ;
}

void GNet::EventHandlerList::disarm( ExceptionHandler * eh ) noexcept
{
	disarm( m_list , eh ) ;
	disarm( m_pending_list , eh ) ;
}

void GNet::EventHandlerList::disarm( List & list , ExceptionHandler * eh ) noexcept
{
	for( auto & value : list )
	{
		if( value.m_es.eh() == eh )
			value.m_es.reset() ;
	}
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::find( Descriptor fd ) const
{
	namespace imp = EventHandlerListImp ;
	using Range = std::pair<List::const_iterator,List::const_iterator> ;
	Range range = std::equal_range( m_list.begin() , m_list.end() , List::value_type(fd) , imp::fdless() ) ;
	return range.first == range.second ? Iterator(m_list.end(),m_list.end()) : Iterator(range.first,m_list.end()) ;
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const noexcept
{
	namespace imp = EventHandlerListImp ;
	using Range = std::pair<List::const_iterator,List::const_iterator> ;
	Range range = std::equal_range( m_pending_list.begin() , m_pending_list.end() , List::value_type(fd) , imp::fdless() ) ;
	if( range.first == range.second )
		range = std::equal_range( m_list.begin() , m_list.end() , List::value_type(fd) , imp::fdless() ) ;
	return range.first != range.second && (*range.first).m_event_handler != nullptr ;
}

void GNet::EventHandlerList::getHandles( std::vector<HANDLE> & out ) const
{
	getHandles( m_list , out ) ;
	getHandles( m_pending_list , out ) ;
}

void GNet::EventHandlerList::getHandles( const List & list , std::vector<HANDLE> & out )
{
	using iterator = std::vector<HANDLE>::iterator ;
	using Range = std::pair<iterator,iterator> ;
	for( const auto & value : list )
	{
		HANDLE h = value.m_fd.h() ;
		if( !h || value.m_event_handler == nullptr ) continue ;
		Range range = std::equal_range( out.begin() , out.end() , h ) ;
		if( range.first == range.second )
			out.insert( range.first , h ) ;
	}
}

void GNet::EventHandlerList::lock()
{
	m_lock++ ;
}

bool GNet::EventHandlerList::unlock()
{
	G_ASSERT( m_lock != 0U ) ;
	m_lock-- ;
	bool updated = false ;
	if( m_lock == 0U )
	{
		updated = !m_pending_list.empty() || m_has_garbage ;
		commitPending() ;
		collectGarbage() ;
	}
	return updated ;
}

void GNet::EventHandlerList::commitPending()
{
	const List::iterator end = m_pending_list.end() ;
	for( auto p = m_pending_list.begin() ; p != end ; ++p )
	{
		if( (*p).m_event_handler != nullptr )
		{
			G_DEBUG( "GNet::EventHandlerList::commitPending: " << m_type << "-list: " << "commiting " << (*p).m_fd ) ;
			addImp( m_list , (*p).m_fd , (*p).m_event_handler , (*p).m_es ) ;
		}
	}
	m_pending_list.clear() ;
}

void GNet::EventHandlerList::collectGarbage()
{
	if( m_has_garbage )
	{
		m_has_garbage = false ;
		for( auto p = m_list.begin() ; p != m_list.end() ; )
		{
			if( (*p).m_event_handler == nullptr )
				p = m_list.erase( p ) ;
			else
				++p ;
		}
	}
}

// ==

void GNet::EventHandlerList::Iterator::raiseEvent( void (EventHandler::*method)() )
{
	// TODO c++11 use std::make_exception_ptr and std::rethrow_exception
	EventLoggingContext set_logging_context( (m_p!=m_end&&handler()&&es().set()) ? es().esrc() : nullptr ) ;
	try
	{
		if( m_p != m_end && handler() != nullptr )
			(handler()->*method)() ;
	}
	catch( GNet::Done & e ) // (caught separately to avoid requiring rtti)
	{
		if( m_p != m_end && es().set() )
			es().call( e , true ) ; // call onException()
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( m_p != m_end && es().set() )
			es().call( e , false ) ; // call onException()
		else
			throw ;
	}
}

void GNet::EventHandlerList::Iterator::raiseEvent( void (EventHandler::*method)(EventHandler::Reason) , EventHandler::Reason reason )
{
	EventLoggingContext set_logging_context( (m_p!=m_end&&handler()&&es().set()) ? es().esrc() : nullptr ) ;
	try
	{
		if( m_p != m_end && handler() != nullptr )
			(handler()->*method)( reason ) ;
	}
	catch( GNet::Done & e ) // (caught separately to avoid requiring rtti)
	{
		if( m_p != m_end && es().set() )
			es().call( e , true ) ; // call onException()
		else
			throw ;
	}
	catch( std::exception & e )
	{
		if( m_p != m_end && es().set() )
			es().call( e , false ) ; // call onException()
		else
			throw ;
	}
}

// ==

GNet::EventHandlerList::Lock::Lock( EventHandlerList & list , bool * invalid_p ) :
	m_list(list) ,
	m_invalid_p(invalid_p)
{
	m_list.lock() ;
}

GNet::EventHandlerList::Lock::~Lock()
{
	if( m_list.unlock() && m_invalid_p != nullptr )
		*m_invalid_p = true ;
}

/// \file geventhandlerlist.cpp
