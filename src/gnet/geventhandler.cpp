//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glog.h"

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

GNet::EventHandlerList::EventHandlerList( std::string type ) :
	m_type(type) ,
	m_lock(0U) ,
	m_copied(false)
{
}

//static
bool GNet::EventHandlerList::contains( const EventHandlerListImp & list , Descriptor fd )
{
	const List::const_iterator end = list.end() ;
	for( List::const_iterator p = list.begin() ; p != end ; ++p )
	{
		if( (*p).m_fd == fd )
			return true ;
	}
	return false ;
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const
{
	return contains( m_list , fd ) ;
}

std::string GNet::EventHandlerList::asString() const
{
	return asString( m_list ) ;
}

std::string GNet::EventHandlerList::asString( const EventHandlerListImp & list ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( List::const_iterator p = list.begin() ; p != list.end() ; ++p )
	{
		ss << sep << fd( p ) ;
		sep = "," ;
	}
	return ss.str() ;
}

GNet::EventHandlerListImp & GNet::EventHandlerList::list()
{
	// lazy copy
	if( m_lock != 0U && !m_copied )
	{
		m_copy = m_list ;
		m_copied = true ;
	}
	
	return m_lock == 0U ? m_list : m_copy ;
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler )
{
	G_ASSERT( handler != NULL ) ;
	if( ! contains(list(),fd) )
	{
		G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: adding " << fd << (m_lock?" (deferred)":"") ) ;
		list().push_back( EventHandlerListItem(fd,handler) ) ;
	}
}

void GNet::EventHandlerList::remove( Descriptor fd )
{
	G_DEBUG( "GNet::EventHandlerList::remove: " << m_type << "-list: removing " << fd << (m_lock?" (deferred)":"") ) ;
	bool found = false ;
	for( List::iterator p = list().begin() ; p != list().end() ; ++p )
	{
		if( (*p).m_fd == fd )
		{
			list().erase( p ) ;
			found = true ;
			break ;
		}
	}
	//if( !found ) G_DEBUG( "GNet::EventHandlerList::remove: cannot find " << fd ) ;
}

GNet::EventHandler * GNet::EventHandlerList::find( Descriptor fd )
{
	const List::iterator end = m_list.end() ;
	for( List::iterator p = m_list.begin() ; p != end ; ++p )
	{
		if( (*p).m_fd == fd )
			return (*p).m_handler ;
	}
	//G_DEBUG( "GNet::EventHandlerList::find: cannot find entry for " << fd ) ;
	return NULL ;
}

void GNet::EventHandlerList::lock()
{
	m_lock++ ;
}

void GNet::EventHandlerList::unlock()
{
	m_lock-- ;
	if( m_lock == 0U && m_copied )
	{
		//G_DEBUG( "GNet::EventHandlerList::unlock: " << m_type << "-list: commiting: " << asString(m_copy) ) ;
		m_list = m_copy ;
		m_copied = false ;
	}
}

