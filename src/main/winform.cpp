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
// winform.cpp
//
//

#include "gdef.h"
#include "gmonitor.h"
#include "run.h"
#include "winform.h"
#include "news.h"
#include "winapp.h"
#include "legal.h"
#include "resource.h"
#include <sstream>

Main::WinForm::WinForm( WinApp & app , const Main::Configuration & cfg , bool confirm ) :
	GGui::Dialog(app) ,
	m_app(app) ,
	m_cfg(cfg) ,
	m_edit_box(*this,IDC_EDIT1) ,
	m_confirm(confirm)
{
}

bool Main::WinForm::onInit()
{
	m_edit_box.set( text() ) ;
	return true ;
}

std::string Main::WinForm::text() const
{
	const std::string crlf( "\r\n" ) ;

	std::ostringstream ss ;
	ss
		<< "E-MailRelay V" << Main::Run::versionNumber() << crlf << crlf
		<< Main::News::text(crlf+crlf)
		<< Main::Legal::warranty("",crlf) << crlf 
		<< Main::Legal::copyright() << crlf << crlf 
		<< "Configuration..." << crlf 
		<< m_cfg.str("* ",crlf) 
		;

	if( GNet::Monitor::instance() )
	{
		ss << crlf << "Network connections..." << crlf ;
		GNet::Monitor::instance()->report( ss , "* " , crlf ) ;
	}

	return ss.str() ;
}

void Main::WinForm::close()
{
	end() ;
}

void Main::WinForm::onNcDestroy()
{
	m_app.formDone() ;
}

void Main::WinForm::onCommand( unsigned int id )
{
	if( id == IDOK && ( !m_confirm || m_app.confirm() ) )
	{
		m_app.formOk() ;
	}
}

