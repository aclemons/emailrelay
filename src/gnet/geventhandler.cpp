//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// geventhandler.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "geventloop.h"
#include "gexception.h"
#include "gdebug.h"
#include "gassert.h"
#include "gdescriptor.h"
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
	throw G::Exception( "exception event" ) ;
}

// ===

GNet::EventHandlerList::EventHandlerList( const std::string & type ) :
	m_type(type) ,
	m_lock(0U) ,
	m_has_garbage(false)
{
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::begin() const
{
	return m_map.begin() ;
}

GNet::EventHandlerList::Iterator GNet::EventHandlerList::end() const
{
	return m_map.end() ;
}

bool GNet::EventHandlerList::contains( Descriptor fd ) const
{
	return m_map.find(fd) != m_map.end() ;
}

GNet::EventHandler * GNet::EventHandlerList::find( Descriptor fd )
{
	Map::iterator p = m_map.find( fd ) ;
	return p != m_map.end() ? (*p).second : NULL ;
}

void GNet::EventHandlerList::add( Descriptor fd , EventHandler * handler )
{
	G_ASSERT( handler != NULL ) ;
	G_DEBUG( "GNet::EventHandlerList::add: " << m_type << "-list: " << "adding " << fd ) ;

	m_map[fd] = handler ;
}

void GNet::EventHandlerList::remove( Descriptor fd )
{
	Map::iterator p = m_map.find( fd ) ;
	if( p != m_map.end() )
	{
		G_DEBUG( "GNet::EventHandlerList::remove: " << m_type << "-list: " << "removing " << fd ) ;
		if( m_lock )
		{
			(*p).second = NULL ;
			m_has_garbage = true ;
		}
		else
		{
			m_map.erase( p ) ;
		}
	}
}

void GNet::EventHandlerList::lock()
{
	m_lock++ ;
}

void GNet::EventHandlerList::unlock()
{
	G_ASSERT( m_lock != 0U ) ;
	m_lock-- ;
	if( m_lock == 0U && m_has_garbage )
		collectGarbage() ;
}

void GNet::EventHandlerList::collectGarbage()
{
	const Map::iterator end = m_map.end() ;
	for( Map::iterator p = m_map.begin() ; p != end ; )
	{
		Map::iterator test = p++ ;
		if( (*test).second == NULL )
			m_map.erase( test ) ;
	}
	m_has_garbage = false ;
}

/// \file geventhandler.cpp
