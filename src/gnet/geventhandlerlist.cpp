//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "geventhandlerlist.h"
#include "gdebug.h"
#include <algorithm>

namespace
{
	struct fdless
	{
		typedef GNet::EventHandlerList::Value Value ;
		bool operator()( const Value & p1 , const Value & p2 )
		{
			return p1.first < p2.first ;
		}
	} ;
}

GNet::EventHandlerList::EventHandlerList( const std::string & type ) :
	m_type(type) ,
	m_lock(0U) ,
	m_has_garbage(false)
{
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler , ExceptionHandler * eh )
{
	G_ASSERT( handler != nullptr ) ; if( handler == nullptr ) return ;
	G_ASSERT( eh != nullptr ) ; if( eh == nullptr ) return ;

	G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: " << "adding " << fd << (m_lock?" (pending)":"") ) ;
	addImp( m_lock?m_pending_list:m_list , fd , handler , eh ) ;
}

void GNet::EventHandlerList::addImp( List & list , Descriptor fd , EventHandler * handler , ExceptionHandler * eh )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , fdless() ) ;
	if( range.first == range.second )
		list.insert( range.first , List::value_type(fd,handler,eh) ) ;
	else
		*range.first = List::value_type(fd,handler,eh) ;
}

void GNet::EventHandlerList::remove( Descriptor fd )
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

bool GNet::EventHandlerList::disable( List & list , Descriptor fd )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
	{
		(*range.first).second = nullptr ;
	}
	return found ;
}

bool GNet::EventHandlerList::remove( List & list , Descriptor fd )
{
	typedef std::pair<List::iterator,List::iterator> Range ;
	Range range = std::equal_range( list.begin() , list.end() , List::value_type(fd) , fdless() ) ;
	const bool found = range.first != range.second ;
	if( found )
		list.erase( range.first ) ;
	return found ;
}

void GNet::EventHandlerList::disarm( ExceptionHandler * eh )
{
	disarm( m_list , eh ) ;
	disarm( m_pending_list , eh ) ;
}

void GNet::EventHandlerList::disarm( List & list , ExceptionHandler * eh )
{
	for( List::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		if( (*p).third == eh )
			(*p).third = nullptr ;
	}
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::find( Descriptor fd ) const
{
	typedef std::pair<List::const_iterator,List::const_iterator> Range ;
	Range range = std::equal_range( m_list.begin() , m_list.end() , List::value_type(fd) , fdless() ) ;
	return range.first == range.second ? Iterator(m_list.end(),m_list.end()) : Iterator(range.first,m_list.end()) ;
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const
{
	typedef std::pair<List::const_iterator,List::const_iterator> Range ;
	Range range = std::equal_range( m_pending_list.begin() , m_pending_list.end() , List::value_type(fd) , fdless() ) ;
	if( range.first == range.second )
		range = std::equal_range( m_list.begin() , m_list.end() , List::value_type(fd) , fdless() ) ;
	return range.first != range.second && (*range.first).second != nullptr ;
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
	for( List::iterator p = m_pending_list.begin() ; p != end ; ++p )
	{
		if( (*p).second != nullptr )
		{
			G_DEBUG( "GNet::EventHandlerList::commitPending: " << m_type << "-list: " << "commiting " << (*p).first ) ;
			addImp( m_list , (*p).first , (*p).second , (*p).third ) ;
		}
	}
	m_pending_list.clear() ;
}

void GNet::EventHandlerList::collectGarbage()
{
	if( m_has_garbage )
	{
		m_has_garbage = false ;
		for( List::iterator p = m_list.begin() ; p != m_list.end() ; )
		{
			if( (*p).second == nullptr )
				p = m_list.erase( p ) ;
			else
				++p ;
		}
	}
}

// ==

void GNet::EventHandlerList::Iterator::raiseEvent( void (EventHandler::*method)() )
{
	try
	{
		if( m_p != m_end && handler() != nullptr )
			(handler()->*method)() ;
	}
	catch( std::exception & e )
	{
		if( m_p != m_end && eh() != nullptr )
			eh()->onException( e ) ;
		else
			throw ;
	}
}

void GNet::EventHandlerList::Iterator::raiseEvent( void (EventHandler::*method)(EventHandler::Reason) , EventHandler::Reason reason )
{
	try
	{
		if( m_p != m_end && handler() != nullptr )
			(handler()->*method)( reason ) ;
	}
	catch( std::exception & e )
	{
		if( m_p != m_end && eh() != nullptr )
			eh()->onException( e ) ;
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
