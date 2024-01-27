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
/// \file gtray.cpp
///

#include "gdef.h"
#include "gtray.h"
#include "gstr.h"
#include "gappinst.h"
#include "gassert.h"
#include <cstring>

GGui::Tray::Tray( unsigned int icon_id , const WindowBase & window ,
	const std::string & tip , unsigned int message )
{
	m_info = NOTIFYICONDATA{} ;
	G_ASSERT( m_info.uVersion == 0 ) ; // winxp

	m_info.cbSize = sizeof(m_info) ;
	m_info.hWnd = window.handle() ;
	m_info.uID = message ;
	m_info.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP ;
	m_info.uCallbackMessage = message ;
	m_info.hIcon = LoadIcon( ApplicationInstance::hinstance() , MAKEINTRESOURCE(icon_id) ) ;
	G::Str::strncpy_s( m_info.szTip , sizeof(m_info.szTip) , tip.c_str() , G::Str::truncate ) ;

	//m_info.dwState = 0 ;
	//m_info.dwStateMask = 0 ;
	//m_info.szInfo ...
	//m_info.uVersion = 0 ;
	//m_info.szInfoTitle ...
	//m_info.dwInfoFlags = 0 ;
	//m_info.guidItem = ...
	//m_info.hBalloonIcon = 0 ;

	if( m_info.hIcon == HNULL )
		throw IconError() ;

	bool ok = !! Shell_NotifyIconA( NIM_ADD , &m_info ) ;
	if( !ok )
		throw Error() ;
}

GGui::Tray::~Tray()
{
	m_info.uFlags = 0 ;
	m_info.uCallbackMessage = 0 ;
	m_info.hIcon = HNULL ;
	bool ok = !! Shell_NotifyIconA( NIM_DELETE , &m_info ) ;
	ok = !!ok ;
}

