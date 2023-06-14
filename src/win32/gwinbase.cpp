//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gwinbase.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gwinbase.h"
#include "glog.h"
#include "gassert.h"
#include <windowsx.h>
#include <vector>
#include <cstring>

GGui::WindowBase::WindowBase( HWND hwnd ) :
	m_hwnd(hwnd)
{
}

GGui::WindowBase::~WindowBase()
{
}

void GGui::WindowBase::setHandle( HWND hwnd )
{
	m_hwnd = hwnd ;
}

GGui::Size GGui::WindowBase::internalSize() const
{
	RECT rect ;
	if( GetClientRect( m_hwnd , &rect ) )
	{
		G_ASSERT( rect.left == 0 ) ;
		G_ASSERT( rect.top == 0 ) ;
		return Size( rect.right , rect.bottom ) ;
	}
	else
	{
		return Size() ;
	}
}

GGui::Size GGui::WindowBase::externalSize() const
{
	RECT rect ;
	if( GetWindowRect( m_hwnd , &rect ) )
	{
		G_ASSERT( rect.right >= rect.left ) ;
		G_ASSERT( rect.bottom >= rect.top ) ;
		return Size( rect.right - rect.left , rect.bottom - rect.top ) ;
	}
	else
	{
		return Size() ;
	}
}

std::string GGui::WindowBase::windowClass() const
{
	std::vector<char> buffer( 257U ) ; // atom size limit
	buffer[0U] = '\0' ;
	GetClassNameA( m_hwnd , &buffer[0] , static_cast<int>(buffer.size()-1U) ) ;
	buffer[buffer.size()-1U] = '\0' ;

	if( (std::strlen(&buffer[0])+1U) == buffer.size() )
	{
		G_WARNING( "GGui::WindowBase::windowClass: possible truncation: "
			<< "\"" << G::Str::printable(std::string(&buffer[0])) << "\"" ) ;
	}

	return std::string( &buffer[0] ) ;
}

HINSTANCE GGui::WindowBase::windowInstanceHandle() const
{
	return reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd,GWLP_HINSTANCE)) ;
}

