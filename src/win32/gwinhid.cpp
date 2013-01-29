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
// gwinhid.cpp
//	

#include "gdef.h"
#include "gwinhid.h"
#include "gdebug.h"
#include <sstream>

bool GGui::WindowHidden::m_registered = false ;

GGui::WindowHidden::WindowHidden( HINSTANCE hinstance ) :
	m_destroyed(false)
{
	std::string window_class_name = windowClassName() ;

	if( !m_registered )
	{
		bool success = registerWindowClass( window_class_name , 
			hinstance , classStyle() , classIcon() , classCursor() , 
			classBrush() ) ;

		G_ASSERT( success || !"GGui::WindowHidden: window class registration error" ) ;

		m_registered = true ;
	}

	{
		bool success = create( window_class_name , 
			std::string() , // title
			windowStyleHidden() , // window style
			0 , 0 , 10 , 10 , // x,y,dx,dy
			0 , // parent
			0 , // menu
			hinstance ) ;

		G_ASSERT( success || !"GGui::WindowHidden: window creation error" ) ;
	}
}

GGui::WindowHidden::~WindowHidden()
{
	if( !m_destroyed && handle() != 0 )
		destroy() ;

	G_ASSERT( !::IsWindow(handle()) ) ;
}

void GGui::WindowHidden::onNcDestroy()
{
	m_destroyed = true ;
}

std::string GGui::WindowHidden::windowClassName()
{
	// a fixed class name would create problems for 
	// Win16 since class names are system-wide -- we
	// need a class name which is unique to this
	// executable or DLL and common to all processes 
	// created from it

	WNDPROC wndproc = windowProcedure() ;
	std::ostringstream ss ;
	ss << "GGui::WindowHidden." << reinterpret_cast<unsigned long>(wndproc) ;
	return ss.str() ;
}

/// \file gwinhid.cpp
