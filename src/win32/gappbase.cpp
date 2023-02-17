//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gappbase.cpp
///

#include "gdef.h"
#include "gappbase.h"
#include "gappinst.h"
#include "gwindow.h"
#include "gconvert.h"
#include "gpump.h"
#include "glog.h"

GGui::ApplicationBase::ApplicationBase( HINSTANCE current , HINSTANCE previous ,
	const std::string & name ) :
		ApplicationInstance(current) ,
		m_name(name) ,
		m_previous(previous)
{
}

GGui::ApplicationBase::~ApplicationBase()
{
}

std::string GGui::ApplicationBase::createWindow( int show_style , bool do_show , int dx , int dy , bool no_throw )
{
	G_DEBUG( "GGui::ApplicationBase::createWindow" ) ;

	// first => register a window class
	if( m_previous == 0 )
	{
		initFirst() ;
	}

	// create the main window
	dx = dx ? dx : CW_USEDEFAULT ;
	dy = dy ? dy : CW_USEDEFAULT ;
	if( !create( className() , title() , windowStyle() ,
			CW_USEDEFAULT , CW_USEDEFAULT , // position (x,y)
			dx , dy , // size
			HNULL , // parent window
			HNULL , // menu handle: HNULL => use class's menu
			hinstance() ) )
	{
		if( no_throw )
			return wndProcExceptionString() ;
		else
			throw CreateError( wndProcExceptionString() ) ;
	}

	if( do_show )
	{
		show( show_style ) ;
		update() ;
	}
	return std::string() ;
}

bool GGui::ApplicationBase::firstInstance() const
{
	return m_previous == 0 ;
}

void GGui::ApplicationBase::initFirst()
{
	G_DEBUG( "GGui::ApplicationBase::initFirst" ) ;

	UINT icon_id = resource() ;
	HICON icon = icon_id ? LoadIcon(hinstance(),MAKEINTRESOURCE(icon_id)) : 0 ;
	if( icon == 0 )
		icon = classIcon() ;

	UINT menu_id = resource() ;

	bool ok = registerWindowClass( className() ,
		hinstance() ,
		classStyle() ,
		icon ,
		classCursor() ,
		backgroundBrush() ,
		menu_id ) ;

	if( !ok )
		throw RegisterError( className() ) ;
}

void GGui::ApplicationBase::close() const
{
	G_DEBUG( "GGui::ApplicationBase::close: sending wm-close" ) ;
	SendMessage( handle() , WM_CLOSE , 0 , 0 ) ;
}

void GGui::ApplicationBase::run( bool with_idle )
{
	if( with_idle )
		GGui::Pump::run( handle() , GGui::Cracker::wm_idle() ) ;
	else
		GGui::Pump::run() ;
}

void GGui::ApplicationBase::onDestroy()
{
	G_DEBUG( "GGui::ApplicationBase::onDestroy: application on-destroy" ) ;
	GGui::Pump::quit() ;
}

std::string GGui::ApplicationBase::title() const
{
	return m_name ;
}

std::string GGui::ApplicationBase::className() const
{
	return m_name ;
}

HBRUSH GGui::ApplicationBase::backgroundBrush()
{
	return GGui::Window::classBrush() ;
}

std::pair<DWORD,DWORD> GGui::ApplicationBase::windowStyle() const
{
	return GGui::Window::windowStyleMain() ;
}

DWORD GGui::ApplicationBase::classStyle() const
{
	return GGui::Window::classStyle() ;
}

UINT GGui::ApplicationBase::resource() const
{
	return 0 ;
}

void GGui::ApplicationBase::beep() const
{
	MessageBeep( MB_ICONEXCLAMATION ) ;
}

bool GGui::ApplicationBase::messageBoxQuery( const std::string & message )
{
	HWND hwnd = messageBoxHandle() ;
	unsigned int type = messageBoxType( hwnd , MB_YESNO | MB_ICONQUESTION ) ;
	return messageBoxCore( hwnd , type , title() , message ) ;
}

void GGui::ApplicationBase::messageBox( const std::string & message )
{
	HWND hwnd = messageBoxHandle() ;
	unsigned int type = messageBoxType( hwnd , MB_OK | MB_ICONEXCLAMATION ) ;
	messageBoxCore( hwnd , type , title() , message ) ;
}

void GGui::ApplicationBase::messageBox( const std::string & title , const std::string & message )
{
	HWND hwnd = HNULL ;
	unsigned int type = messageBoxType( hwnd , MB_OK | MB_ICONEXCLAMATION ) ;
	messageBoxCore( hwnd , type , title , message ) ;
}

bool GGui::ApplicationBase::messageBoxCore( HWND parent , unsigned int type ,
	const std::string & title , const std::string & message )
{
	std::wstring wtitle ;
	std::wstring wmessage ;
	G::Convert::convert( wtitle , title ) ;
	G::Convert::convert( wmessage , message ) ;
	int rc = MessageBoxW( parent , wmessage.c_str() , wtitle.c_str() , type ) ;
	return rc == IDOK || rc == IDYES ;
}

HWND GGui::ApplicationBase::messageBoxHandle() const
{
	HWND hwnd = GetActiveWindow() ; // eg. a dialog box
	if( hwnd == HNULL )
		hwnd = handle() ;
	return hwnd ;
}

unsigned int GGui::ApplicationBase::messageBoxType( HWND hwnd , unsigned int base_type )
{
	unsigned int type = base_type ;
	if( hwnd == HNULL )
		base_type |= ( MB_TASKMODAL | MB_SETFOREGROUND ) ;
	return type ;
}

