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
/// \file gdialog.cpp
///

#include "gdef.h"
#include "gdialog.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm> // find

INT_PTR CALLBACK gdialog_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		return GGui::Dialog::dlgProc( hwnd , message , wparam , lparam ) ;
	}
	catch(...) // callback
	{
		return FALSE ;
	}
}

GGui::Dialog::DialogList GGui::Dialog::m_list ;

GGui::Dialog::Dialog( HINSTANCE hinstance , HWND hwnd_parent , const std::string & title ) :
	WindowBase(HNULL) ,
	m_title(title) ,
	m_modal(false) ,
	m_focus_set(false) ,
	m_hinstance(hinstance) ,
	m_hwnd_parent(hwnd_parent) ,
	m_magic(Magic)
{
}

GGui::Dialog::Dialog( const ApplicationBase & app , bool top_level ) :
	WindowBase(HNULL) ,
	m_title(app.title()) ,
	m_modal(false) ,
	m_focus_set(false) ,
	m_hinstance(app.hinstance()) ,
	m_hwnd_parent(top_level?HNULL:app.handle()) ,
	m_magic(Magic)
{
}

void GGui::Dialog::privateInit( HWND hwnd )
{
	setHandle( hwnd ) ;
	SetWindowTextA( handle() , m_title.c_str() ) ;
}

GGui::Dialog::~Dialog()
{
	G_DEBUG( "GGui::Dialog::~Dialog" ) ;
	cleanup() ;
	m_magic = 0 ;
}

GGui::Dialog::DialogList::iterator GGui::Dialog::find( HWND h )
{
	return std::find( m_list.begin() , m_list.end() , h ) ;
}

void GGui::Dialog::cleanup()
{
	// if not already cleaned up
	if( handle() != HNULL )
	{
		G_DEBUG( "GGui::Dialog::cleanup" ) ;

		// reset the object pointer
		SetWindowLongPtr( handle() , DWLP_USER , LONG_PTR(0) ) ;

		// remove from the modeless list
		if( !m_modal && find(handle()) != m_list.end() )
		{
			G_DEBUG( "GGui::Dialog::cleanup: removing modeless dialog box window " << handle() ) ;
			m_list.erase( find(handle()) ) ;
			G_ASSERT( find(handle()) == m_list.end() ) ; // assert only one
		}
	}
	setHandle( HNULL ) ;
}

void GGui::Dialog::setFocus( int control )
{
	HWND hwnd_control = GetDlgItem( handle() , control ) ;
	if( hwnd_control != HNULL )
	{
		m_focus_set = true ; // determines the WM_INITDIALOG return value
		SetFocus( hwnd_control ) ;
	}
}

LRESULT GGui::Dialog::sendMessage( int control , unsigned int message , WPARAM wparam , LPARAM lparam ) const
{
	HWND hwnd_control = GetDlgItem( handle() , control ) ;
	return SendMessage( hwnd_control , message , wparam , lparam ) ;
}

INT_PTR GGui::Dialog::dlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	if( message == WM_INITDIALOG )
	{
		Dialog * dialog = fromLongParam( lparam ) ;
		SetWindowLongPtr( hwnd , DWLP_USER , toLongPtr(dialog) ) ;
		dialog->privateInit( hwnd ) ;
		G_DEBUG( "GGui::Dialog::dlgProc: WM_INITDIALOG" ) ;
		if( !dialog->onInit() )
		{
			dialog->privateEnd( 0 ) ;
			return 0 ;
		}

		// add to the static list of modeless dialogs
		if( !dialog->m_modal )
		{
			G_DEBUG( "GGui::Dialog::dlgProc: adding modeless dialog box window " << hwnd ) ;
			m_list.push_front( hwnd ) ;
			G_DEBUG( "GGui::Dialog::dlgProc: now " << m_list.size() << " modeless dialog box(es)" ) ;
		}

		return !dialog->privateFocusSet() ;
	}
	else
	{
		Dialog * dialog = fromLongPtr( GetWindowLongPtr(hwnd,DWLP_USER) ) ;
		if( dialog != nullptr )
			return dialog->dlgProcImp( message , wparam , lparam ) ;
		else
			return 0 ; // WM_SETFONT etc.
	}
}

INT_PTR GGui::Dialog::dlgProcImp( UINT message , WPARAM wparam , LPARAM lparam )
{
	switch( message )
	{
		case WM_VSCROLL:
		case WM_HSCROLL:
		{
			HWND hwnd_scrollbar = reinterpret_cast<HWND>(HIWORD(lparam)) ; // may be zero
			if( wparam == SB_THUMBPOSITION || wparam == SB_THUMBTRACK )
			{
				unsigned int position = LOWORD(lparam) ;
				onScrollPosition( hwnd_scrollbar , position ) ;
			}
			else
			{
				bool vertical = message == WM_VSCROLL ;
				onScroll( hwnd_scrollbar , vertical ) ;
			}
			onScrollMessage( message , wparam , lparam ) ;
			return 0 ;
		}

		case WM_COMMAND:
		{
			WORD hi_word = HIWORD( wparam ) ;
			WORD lo_word = LOWORD( wparam ) ; // IDOK, IDCANCEL, ...
			if( hi_word == 0 )
				onCommand( static_cast<unsigned int>(lo_word) ) ; // TODO add parameters
			return 1 ;
		}

		case WM_NOTIFY:
		{
			// TODO control messages
			return 0 ;
		}

		case WM_CTLCOLORDLG:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_DLG ) ;
		}

		case WM_CTLCOLORMSGBOX:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_MSGBOX ) ;
		}

		case WM_CTLCOLOREDIT:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_EDIT ) ;
		}

		case WM_CTLCOLORBTN:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_BTN ) ;
		}

		case WM_CTLCOLORLISTBOX:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_LISTBOX ) ;
		}

		case WM_CTLCOLORSCROLLBAR:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_SCROLLBAR ) ;
		}

		case WM_CTLCOLORSTATIC:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_STATIC ) ;
		}

		case WM_SETCURSOR:
		{
			// no-op -- WM_SETCURSOR is useless in a dialog box
			return 0 ;
		}

		case WM_CLOSE:
		{
			onClose() ;
			return 1 ;
		}

		case WM_DESTROY:
		{
			onDestroy() ;
			return 1 ;
		}

		case WM_NCDESTROY:
		{
			G_DEBUG( "GGui::Dialog::dlgProc: WM_NCDESTROY" ) ;
			cleanup() ;
			onNcDestroy() ; // an override could do "delete this"
			return 1 ;
		}
	}
	return 0 ;
}

HBRUSH GGui::Dialog::onControlColour( HDC , HWND , WORD )
{
	return 0 ;
}

void GGui::Dialog::onDestroy()
{
}

void GGui::Dialog::onNcDestroy()
{
}

void GGui::Dialog::end()
{
	privateEnd( 1 ) ;
}

void GGui::Dialog::privateEnd( int i )
{
	if( handle() != HNULL )
	{
		G_DEBUG( "GGui::Dialog::privateEnd: " << i ) ;
		if( m_modal )
			EndDialog( handle() , i ) ;
		else
			DestroyWindow( handle() ) ;
	}
}

void GGui::Dialog::onClose()
{
	privateEnd( 1 ) ;
}

void GGui::Dialog::onCommand( UINT id )
{
	if( id == IDOK )
		privateEnd( 1 ) ;
}

bool GGui::Dialog::run( int resource_id )
{
	return runStart() && runCore( MAKEINTRESOURCE(resource_id) ) ;
}

bool GGui::Dialog::run( const char * template_name )
{
	return runStart() && runCore( template_name ) ;
}

bool GGui::Dialog::runStart()
{
	G_DEBUG( "GGui::Dialog::run" ) ;
	if( handle() != HNULL )
	{
		G_DEBUG( "GGui::Dialog::run: already running" ) ;
		return false ;
	}
	return true ;
}

bool GGui::Dialog::runCore( const char * resource )
{
	m_modal = true ;
	INT_PTR end_dialog_value = DialogBoxParamA( m_hinstance , resource ,
		m_hwnd_parent , toDlgProc(gdialog_export) , toLongParam(this) ) ;
	int rc = static_cast<int>(end_dialog_value) ;
	return runEnd( rc ) ;
}

bool GGui::Dialog::runCore( const wchar_t * resource )
{
	m_modal = true ;
	INT_PTR end_dialog_value = DialogBoxParamW( m_hinstance , resource ,
		m_hwnd_parent , toDlgProc(gdialog_export) , toLongParam(this) ) ;
	int rc = static_cast<int>(end_dialog_value) ;
	return runEnd( rc ) ;
}

bool GGui::Dialog::runEnd( int rc )
{
	if( rc == -1 )
	{
		DWORD error = GetLastError() ;
		G_DEBUG( "GGui::Dialog::run: cannot create dialog box: " << error ) ;
		GDEF_IGNORE_VARIABLE( error ) ;
		return false ;
	}
	else if( rc == 0 )
	{
		// onInit() returned false
		G_DEBUG( "GGui::Dialog::run: dialog creation aborted" ) ;
		return false ;
	}
	return true ;
}

bool GGui::Dialog::runModeless( int resource_id , bool visible )
{
	return runStart() && runModelessCore( MAKEINTRESOURCE(resource_id) , visible ) ;
}

bool GGui::Dialog::runModeless( const char * resource_name , bool visible )
{
	return runStart() && runModelessCore( resource_name , visible ) ;
}

bool GGui::Dialog::runModelessCore( const char * resource , bool visible )
{
	m_modal = false ;
	HWND hwnd = CreateDialogParamA( m_hinstance , resource ,
		m_hwnd_parent , toDlgProc(gdialog_export) , toLongParam(this) ) ;
	return runModelessEnd( hwnd , visible ) ;
}

bool GGui::Dialog::runModelessCore( const wchar_t * resource , bool visible )
{
	m_modal = false ;
	HWND hwnd = CreateDialogParamW( m_hinstance , resource ,
		m_hwnd_parent , toDlgProc(gdialog_export) , toLongParam(this) ) ;
	return runModelessEnd( hwnd , visible ) ;
}

bool GGui::Dialog::runModelessEnd( HWND hwnd , bool visible )
{
	if( hwnd == HNULL )
	{
		G_DEBUG( "GGui::Dialog::runModless: cannot create dialog box" ) ;
		return false ;
	}
	G_DEBUG( "GGui::Dialog::runModeless: hwnd " << hwnd ) ;
	G_ASSERT( hwnd == handle() ) ;

	if( visible )
		ShowWindow( hwnd , SW_SHOW ) ; // in case not WS_VISIBLE style

	return true ;
}

bool GGui::Dialog::dialogMessage( MSG & msg )
{
	for( HWND hdialog : m_list )
	{
		if( IsDialogMessage( hdialog , &msg ) )
			return true ;
	}
	return false ;
}

GGui::SubClassMap & GGui::Dialog::map()
{
	return m_map ;
}

bool GGui::Dialog::registerNewClass( HICON hicon , const std::string & new_class_name ) const
{
	std::string old_class_name = windowClass() ;
	HINSTANCE hinstance = windowInstanceHandle() ;

	// get our class info
	//
	WNDCLASSA class_info ;
	GetClassInfoA( hinstance , old_class_name.c_str() , &class_info ) ;

	// register a new class
	//
	class_info.hIcon = hicon ;
	class_info.lpszClassName = new_class_name.c_str() ;
	ATOM rc = RegisterClassA( &class_info ) ;

	return rc != 0 ;
}

bool GGui::Dialog::privateFocusSet() const
{
	return m_focus_set ;
}

bool GGui::Dialog::onInit()
{
	return true ;
}

void GGui::Dialog::onScrollPosition( HWND , unsigned int )
{
}

void GGui::Dialog::onScroll( HWND , bool )
{
}

void GGui::Dialog::onScrollMessage( unsigned int , WPARAM , LPARAM )
{
}

bool GGui::Dialog::isValid()
{
	return m_magic == Magic ;
}

INT_PTR GGui::Dialog::onControlColour_( WPARAM wparam , LPARAM lparam , WORD type )
{
	HBRUSH rc = onControlColour( reinterpret_cast<HDC>(wparam) , reinterpret_cast<HWND>(lparam) , type ) ;
	return rc ? 1 : 0 ;
}

LPARAM GGui::Dialog::toLongParam( Dialog * p )
{
	return reinterpret_cast<LPARAM>( static_cast<void*>(p) ) ;
}

LONG_PTR GGui::Dialog::toLongPtr( Dialog * p )
{
	return reinterpret_cast<LONG_PTR>( static_cast<void*>(p) ) ;
}

GGui::Dialog * GGui::Dialog::fromLongParam( LPARAM lparam )
{
	return reinterpret_cast<Dialog*>( reinterpret_cast<void*>(lparam) ) ;
}

GGui::Dialog * GGui::Dialog::fromLongPtr( LONG_PTR p )
{
	return static_cast<Dialog*>( reinterpret_cast<void*>(p) ) ;
}


