//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// geventhandler.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "geventloop.h"
#include "gdebug.h"
#include "gassert.h"
#include "gdescriptor.h"
#include "glog.h"
#include <algorithm> // std::find

namespace
{
	struct Eq
	{
		GNet::Descriptor m_fd ;
		explicit Eq( GNet::Descriptor fd ) : m_fd(fd) {}
		bool operator()( const GNet::EventHandlerListItem & item ) const 
		{ 
			return item.m_fd == m_fd ;
		}
	} ;
	struct NotNull
	{
		GNet::Descriptor m_fd ;
		explicit NotNull( GNet::Descriptor fd ) : m_fd(fd) {}
		bool operator()( const GNet::EventHandlerListItem & item ) const 
		{ 
			return item.m_fd == m_fd && item.m_handler != NULL ;
		}
	} ;
}

GNet::EventHandlerListItem::EventHandlerListItem( Descriptor fd , EventHandler * handler ) :
	m_fd(fd) , 
	m_handler(handler) 
{
}

bool operator!=( const GNet::EventHandlerListItem & a , const GNet::EventHandlerListItem & b )
{
	return a.m_fd != b.m_fd ;
}

// ===

GNet::EventHandler::~EventHandler()
{
}

void GNet::EventHandler::readEvent()
{
	G_DEBUG( "GNet::EventHandler::readEvent: no override" ) ;
}

void GNet::EventHandler::writeEvent()
{
	G_DEBUG( "GNet::EventHandler::writeEvent: no override" ) ;
}

void GNet::EventHandler::exceptionEvent()
{
	G_DEBUG( "GNet::EventHandler::exceptionEvent: no override" ) ;
}

// ===

GNet::EventHandlerList::EventHandlerList( const std::string & type ) :
	m_type(type) ,
	m_lock(0U)
{
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::begin() const
{
	return m_list.begin() ;
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::end() const
{
	return m_list.end() ;
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const
{
	const List::const_iterator end = m_list.end() ;
	return std::find_if( m_list.begin() , end , NotNull(fd) ) != end ;
}

GNet::EventHandler * GNet::EventHandlerList::find( Descriptor fd )
{
	const List::iterator end = m_list.end() ;
	List::iterator p = std::find_if( m_list.begin() , end , NotNull(fd) ) ;
	return p != end ? (*p).m_handler : NULL ;
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler )
{
	G_ASSERT( handler != NULL ) ;
	G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: " << "adding " << fd ) ;

	const List::iterator end = m_list.end() ;
	List::iterator p = std::find_if( m_list.begin() , end , Eq(fd) ) ;
	if( p != end )
	{
		G_ASSERT( (*p).m_handler == NULL ) ; // assert not re-adding same fd
		(*p).m_handler = handler ;
	}
	else
	{
		m_list.push_back( EventHandlerListItem(fd,handler) ) ;
	}
}

void GNet::EventHandlerList::remove( Descriptor fd )
{
	G_DEBUG( "GNet::EventHandlerList::remove: " << m_type << "-list: " << "removing " << fd ) ;

	const List::iterator end = m_list.end() ;
	List::iterator p = std::find_if( m_list.begin() , end , Eq(fd) ) ;
	if( p != end )
	{
		if( m_lock )
			(*p).m_handler = NULL ;
		else
			m_list.erase( p ) ;
	}
}

void GNet::EventHandlerList::lock()
{
	m_lock++ ;
}

void GNet::EventHandlerList::unlock()
{
	m_lock-- ;
	if( m_lock == 0U )
	{
		// collect garbage
		const List::iterator end = m_list.end() ;
		for( List::iterator p = m_list.begin() ; p != end ; )
		{
			if( (*p).m_handler == NULL )
				p = m_list.erase( p ) ;
			else
				++p ;
		}
	}
}

