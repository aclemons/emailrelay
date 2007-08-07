//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdialog.cpp
//

#include "gdef.h"
#include "gdialog.h"
#include "gdebug.h"
#include "glog.h"
#include <algorithm> // find

BOOL CALLBACK gdialog_dlgproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	return GGui::Dialog::dlgProc( hwnd , message , wparam , lparam ) ;
}

GGui::DialogList GGui::Dialog::m_list ;

GGui::Dialog::Dialog( HINSTANCE hinstance , HWND hwnd_parent , const std::string & title ) :
	WindowBase(NULL) ,
	m_title(title) ,
	m_modal(false) ,
	m_focus_set(false) ,
	m_hinstance(hinstance) ,
	m_hwnd_parent(hwnd_parent) ,
	m_magic(Magic)
{
}

GGui::Dialog::Dialog( const ApplicationBase & app , bool top_level ) :
	WindowBase(NULL) ,
	m_title(app.title()) ,
	m_modal(false) ,
	m_focus_set(false) ,
	m_hinstance(app.hinstance()) ,
	m_hwnd_parent(top_level?NULL:app.handle()) ,
	m_magic(Magic)
{
}

void GGui::Dialog::privateInit( HWND hwnd )
{
	setHandle( hwnd ) ;
	::SetWindowText( handle() , m_title.c_str() ) ;
}

GGui::Dialog::~Dialog()
{
	G_DEBUG( "GGui::Dialog::~Dialog" ) ;
	cleanup() ;
	m_magic = 0 ;
}

GGui::DialogList::iterator GGui::Dialog::find( HWND h )
{
	return std::find( m_list.begin() , m_list.end() , DialogHandle(h) ) ;
}

void GGui::Dialog::cleanup()
{
	// if not already cleaned up
	if( handle() != NULL )
	{
		G_DEBUG( "GGui::Dialog::cleanup" ) ;

		// reset the object pointer
		::SetWindowLong( handle() , DWL_USER , LPARAM(0) ) ;

		// remove from the modal list
		G_ASSERT( (find(handle())!=m_list.end()) == !m_modal ) ;
		if( !m_modal )
		{
			G_DEBUG( "GGui::Dialog::dlgProc: removing modeless dialog box window " << handle() ) ;
			m_list.erase( find(handle()) ) ;
			G_ASSERT( find(handle()) == m_list.end() ) ; // assert only one
		}
	}
	setHandle( NULL ) ;
}

void GGui::Dialog::setFocus( int control )
{
	HWND hwnd_control = ::GetDlgItem( handle() , control ) ;
	if( hwnd_control != NULL )
	{
		m_focus_set = true ; // determines the WM_INITDIALOG return value
		::SetFocus( hwnd_control ) ;
	}
}

LRESULT GGui::Dialog::sendMessage( int control , unsigned message , WPARAM wparam , LPARAM lparam ) const
{
	HWND hwnd_control = ::GetDlgItem( handle() , control ) ;
	return ::SendMessage( hwnd_control , message , wparam , lparam ) ;
}

BOOL GGui::Dialog::dlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	if( message == WM_INITDIALOG )
	{
		Dialog * dialog = from_lparam( lparam ) ;
		::SetWindowLong( hwnd , DWL_USER , to_lparam(dialog) ) ;
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
			m_list.push_front( DialogHandle(hwnd) ) ;
			G_DEBUG( "GGui::Dialog::dlgProc: now " << m_list.size() << " modeless dialog box(es)" ) ;
		}

		return !dialog->privateFocusSet() ;
	}
	else
	{
		Dialog * dialog = from_long( ::GetWindowLong(hwnd,DWL_USER) ) ;
		if( dialog != NULL )
			return dialog->dlgProc( message , wparam , lparam ) ;
		else
			return 0 ; // WM_SETFONT etc.
	}
}

BOOL GGui::Dialog::dlgProc( UINT message , WPARAM wparam , LPARAM lparam )
{
	switch( message )
	{
		case WM_VSCROLL:
		case WM_HSCROLL:
		{
			HWND hwnd_scrollbar = reinterpret_cast<HWND>(HIWORD(lparam)) ; // may be zero
			if( wparam == SB_THUMBPOSITION || wparam == SB_THUMBTRACK )
			{
				unsigned position = LOWORD(lparam) ;
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
			onCommand( wparam ) ;
			return 1 ;
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

#if 0 // Windows bug -- WM_SETCURSOR is useless in a dialog box
		case WM_SETCURSOR:
		{
			onSetCursor( reinterpret_cast<HWND>(wparam) , LOWORD(lparam) , HIWORD(lparam) ) ;
			return 0 ;
		}
#endif

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
			onNcDestroy() ; // override could do "delete this"
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
	if( handle() == NULL ) return ;
	G_DEBUG( "GGui::Dialog::privateEnd: " << i ) ;
	if( m_modal )
		::EndDialog( handle() , i ) ;
	else
		::DestroyWindow( handle() ) ;
}

void GGui::Dialog::onClose()
{
	privateEnd(1) ;
}

void GGui::Dialog::onCommand( UINT id )
{
	if( id == IDOK )
		privateEnd(1) ;
}

bool GGui::Dialog::run( int resource_id )
{
	return run( MAKEINTRESOURCE(resource_id) ) ;
}

bool GGui::Dialog::run( const char * f_name )
{		
	G_DEBUG( "GGui::Dialog::run" ) ;
	
	if( handle() != NULL )
	{
		G_DEBUG( "GGui::Dialog::run: already running" ) ;
		return false ;
	}

	m_modal = true ;
	int rc = ::DialogBoxParam( m_hinstance , f_name ,
		m_hwnd_parent , dlgproc_export_fn() , to_lparam(this) ) ;
		
	if( rc == -1 )
	{
		int error = ::GetLastError() ;
		G_DEBUG( "GGui::Dialog::run: cannot create dialog box: " << error ) ;
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
	return runModeless( MAKEINTRESOURCE(resource_id) , visible ) ;
}

bool GGui::Dialog::runModeless( const char * f_name , bool visible )
{		
	G_DEBUG( "GGui::Dialog::runModeless" ) ;

	if( handle() != NULL )
	{
		G_DEBUG( "GGui::Dialog::runModeless: already running" ) ;
		return false ;
	}

	m_modal = false ;
	HWND hwnd = ::CreateDialogParam( m_hinstance , f_name ,
		m_hwnd_parent , dlgproc_export_fn() , to_lparam(this) ) ;
		
	if( hwnd == NULL )
	{
		G_DEBUG( "GGui::Dialog::runModless: cannot create dialog box" ) ;
		return false ;
	}
	G_DEBUG( "GGui::Dialog::runModeless: hwnd " << hwnd ) ;
	G_ASSERT( hwnd == handle() ) ;
	
	if( visible )
		::ShowWindow( hwnd , SW_SHOW ) ; // in case not WS_VISIBLE style

	return true ;
}

bool GGui::Dialog::dialogMessage( MSG &msg )
{
	for( GGui::DialogList::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( ::IsDialogMessage( (*p).h , &msg ) )
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
	WNDCLASS class_info ;
	::GetClassInfo( hinstance , old_class_name.c_str() , &class_info ) ;

	// register a new class
	//
	class_info.hIcon = hicon ;
	class_info.lpszClassName = new_class_name.c_str() ;
	ATOM rc = ::RegisterClass( &class_info ) ;

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

void GGui::Dialog::onScrollPosition( HWND , unsigned )
{
}

void GGui::Dialog::onScroll( HWND , bool )
{
}

void GGui::Dialog::onScrollMessage( unsigned , WPARAM , LPARAM ) 
{
}

bool GGui::Dialog::isValid() 
{ 
	return m_magic == Magic ; 
}

BOOL GGui::Dialog::onControlColour_( WPARAM wparam , LPARAM lparam , WORD type )
{
	return reinterpret_cast<BOOL>( 
		onControlColour( reinterpret_cast<HDC>(wparam) , reinterpret_cast<HWND>(lparam) , type ) ) ;
}

GGui::Dialog * GGui::Dialog::from_lparam( LPARAM lparam )
{
	return reinterpret_cast<Dialog*>( reinterpret_cast<void*>(lparam) ) ;
}

GGui::Dialog * GGui::Dialog::from_long( LONG l )
{
	return reinterpret_cast<Dialog*>( reinterpret_cast<void*>(l) ) ;
}

LPARAM GGui::Dialog::to_lparam( Dialog * p )
{
	return reinterpret_cast<LPARAM>( reinterpret_cast<void*>(p) ) ;
}

DLGPROC GGui::Dialog::dlgproc_export_fn()
{
	return reinterpret_cast<DLGPROC>(gdialog_dlgproc_export) ;
}

/// \file gdialog.cpp
