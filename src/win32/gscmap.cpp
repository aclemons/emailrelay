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
// gscmap.cpp
//

#include "gdef.h"
#include "gscmap.h"
#include "gdebug.h"
#include "glog.h"

GGui::SubClassMap::SubClassMap() : 
	m_high_water(0U)
{
}

GGui::SubClassMap::~SubClassMap()
{
}

void GGui::SubClassMap::add( HWND hwnd , SubClassMap::Proc proc , void *context )
{
	for( unsigned i = 0 ; i < SlotsLimit ; i++ )
	{
		if( m_list[i].hwnd == hwnd )
		{
			G_ASSERT( !"GGui::SubClassMap::add: duplicate window handle" ) ;
		}
		if( m_list[i].hwnd == 0 )
		{
			m_list[i].proc = proc ;
			m_list[i].hwnd = hwnd ;
			m_list[i].context = context ;
			if( i >= m_high_water )
				m_high_water = i+1 ;
			return ;
		}
	}
	G_DEBUG( "GGui::SubClassMap::add: too many subclassed windows" ) ;
	G_ASSERT( !"GGui::SubClassMap::add: too many subclasses windows" ) ;
}

GGui::SubClassMap::Proc GGui::SubClassMap::find( HWND hwnd , void **context_p )
{
	if( context_p != NULL )
		*context_p  = NULL ;
		
	for( unsigned i = 0 ; i < m_high_water ; i++ )
	{
		if( m_list[i].hwnd == hwnd )
		{
			if( context_p != NULL )
				*context_p = m_list[i].context ;
			return m_list[i].proc ;
		}
	}
	G_DEBUG( "GGui::SubClassMap::find: cannot find window " << hwnd ) ;
	G_ASSERT( false ) ;
	return 0 ;
}

void GGui::SubClassMap::remove( HWND hwnd )
{
	unsigned int count = 0U ;
	for( unsigned int i = 0U ; i < m_high_water ; i++ )
	{
		if( m_list[i].hwnd == hwnd )
		{
			m_list[i].hwnd = 0 ;
			count++ ;
			if( count > 1U )
				G_ASSERT( !"GGui::SubClassMap::remove: duplicate" ) ;
		}
	}
	if( count == 0U )
	{
		G_DEBUG( "GGui::SubClassMap::remove: cannot find window " << hwnd ) ;
		G_ASSERT( false ) ;
	}
}

/// \file gscmap.cpp
