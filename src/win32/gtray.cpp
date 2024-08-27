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
/// \file gtray.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gtray.h"
#include "gstr.h"
#include "gappinst.h"
#include "gassert.h"
#include <cstring>

GGui::Tray::Tray( unsigned int icon_id , const WindowBase & window ,
	const std::string & tip , unsigned int message )
{
	m_info = G::nowide::NOTIFYICONDATA_type {} ;
	G_ASSERT( m_info.uVersion == 0 ) ; // winxp

	m_info.cbSize = sizeof(m_info) ;
	m_info.hWnd = window.handle() ;
	m_info.uID = message ;
	m_info.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP ;
	m_info.uCallbackMessage = message ;

	int rc = G::nowide::shellNotifyIcon( ApplicationInstance::hinstance() , NIM_ADD , &m_info , icon_id , tip ) ;
	if( rc == 2 )
		throw IconError() ;
	else if( rc == 1 )
		throw Error() ;
}

GGui::Tray::~Tray()
{
	static_assert( noexcept(G::nowide::shellNotifyIcon(NIM_DELETE,&m_info,std::nothrow)) , "" ) ;
	m_info.uFlags = 0 ;
	m_info.uCallbackMessage = 0 ;
	m_info.hIcon = HNULL ;
	G::nowide::shellNotifyIcon( NIM_DELETE , &m_info , std::nothrow ) ;
}

