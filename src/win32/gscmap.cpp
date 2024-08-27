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
/// \file gscmap.cpp
///

#include "gdef.h"
#include "gscmap.h"
#include "glog.h"
#include "gassert.h"

GGui::SubClassMap::SubClassMap()
{
}

void GGui::SubClassMap::add( HWND hwnd , SubClassMap::Proc proc , void *context )
{
	for( std::size_t i = 0U ; i < m_list.size() ; i++ )
	{
		if( m_list[i].hwnd == 0 || m_list[i].hwnd == hwnd )
		{
			m_list[i] = Slot( proc , hwnd , context ) ;
			return ;
		}
	}
	m_list.push_back( Slot(proc,hwnd,context) ) ;
}

GGui::SubClassMap::Proc GGui::SubClassMap::find( HWND hwnd , void **context_p )
{
	if( context_p != nullptr )
		*context_p = nullptr ;

	for( std::size_t i = 0U ; i < m_list.size() ; i++ )
	{
		if( m_list[i].hwnd == hwnd )
		{
			if( context_p != nullptr )
				*context_p = m_list[i].context ;
			return m_list[i].proc ;
		}
	}
	return 0 ;
}

void GGui::SubClassMap::remove( HWND hwnd )
{
	for( std::size_t i = 0U ; i < m_list.size() ; i++ )
	{
		if( m_list[i].hwnd == hwnd )
			m_list[i].hwnd = 0 ;
	}
}

