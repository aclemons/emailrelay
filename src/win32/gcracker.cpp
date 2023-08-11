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
/// \file gcracker.cpp
///

#include "gdef.h"
#include "gcracker.h"
#include "gstringarray.h"
#include "glog.h"
#include "gassert.h"
#include <windowsx.h>
#include <vector>

GGui::Cracker::Cracker( HWND hwnd ) :
	WindowBase(hwnd)
{
}

GGui::Cracker::~Cracker()
{
}

LRESULT GGui::Cracker::crack( UINT message , WPARAM wparam , LPARAM lparam , bool & call_default )
{
	//G_DEBUG( "GGui::Cracker::crack: " << message << " " << wparam << " " << lparam ) ;
	call_default = false ;
	switch( message )
	{
		case WM_PAINT:
		{
			G_DEBUG( "Cracker::onPaint" ) ;
			bool done = onPaintMessage() ;
			if( ! done )
			{
				PAINTSTRUCT ps ;
				HDC dc = BeginPaint( handle() , &ps ) ;
				onPaint( dc ) ;
				EndPaint( handle() , &ps ) ;
			}
			return 0 ;
		}

		case WM_CLOSE:
		{
			G_DEBUG( "Cracker::onClose" ) ;
			if( onClose() )
				PostMessage( handle() , WM_DESTROY , 0 , 0 ) ;
			return 0 ;
		}

		case WM_DESTROY:
		{
			G_DEBUG( "Cracker::onDestroy: hwnd " << handle() ) ;
			onDestroy() ;
			return 0 ;
		}

		case WM_NCDESTROY:
		{
			G_DEBUG( "Cracker::onNcDestroy: hwnd " << handle() ) ;
			onNcDestroy() ;
			return 0 ;
		}

		case WM_CTLCOLORMSGBOX:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_MSGBOX ) ;
		}

		case WM_CTLCOLORDLG:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_DLG ) ;
		}

		case WM_CTLCOLOREDIT:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_EDIT ) ;
		}

		case WM_CTLCOLORLISTBOX:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_LISTBOX ) ;
		}

		case WM_CTLCOLORBTN:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_BTN ) ;
		}

		case WM_CTLCOLORSCROLLBAR:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_SCROLLBAR ) ;
		}

		case WM_CTLCOLORSTATIC:
		{
			return onControlColour_( wparam , lparam , CTLCOLOR_STATIC ) ;
		}

		case WM_SYSCOLORCHANGE:
		{
			onSysColourChange() ;
			return 0 ;
		}

		case WM_SYSCOMMAND:
		{
			bool processed = false ;
			WPARAM cmd = wparam & 0xfff0 ;
			if( cmd == SC_MAXIMIZE )
				processed = onSysCommand( SysCommand::scMaximise ) ;
			else if( cmd == SC_MINIMIZE )
				processed = onSysCommand( SysCommand::scMinimise ) ;
			else if( cmd == SC_SIZE )
				processed = onSysCommand( SysCommand::scSize ) ;
			else if( cmd == SC_CLOSE )
				processed = onSysCommand( SysCommand::scClose ) ;
			if( !processed )
				call_default = true ;
			return 0 ;
		}

		case WM_KILLFOCUS:
		{
			// wparam, not lparam
			HWND hwnd_other = hwnd_from( wparam ) ;
			onLooseFocus( hwnd_other ) ;
			return 0 ;
		}

		case WM_SETFOCUS:
		{
			HWND hwnd_other = hwnd_from( wparam ) ;
			onGetFocus( hwnd_other ) ;
			return 0 ;
		}

		case WM_CHAR:
		{
			unsigned int repeat_count = static_cast<unsigned int>(LOWORD(lparam)) ;
			WORD vkey = static_cast<WORD>(wparam) ;
			onChar( vkey , repeat_count ) ;
			return 0 ;
		}

		case WM_ERASEBKGND:
		{
			G_DEBUG( "Cracker::onEraseBackground" ) ;
			HDC hdc = hdc_from( wparam ) ;
			return onEraseBackground( hdc ) ;
		}

		case WM_DROPFILES:
		{
			G_DEBUG( "Cracker::onDrop" ) ;
			HDROP hdrop = hdrop_from( wparam ) ;
			int count = DragQueryFileA( hdrop , 0xFFFFFFFF , nullptr , 0 ) ;
			G::StringArray list ;
			std::vector<char> buffer( 32768U , '\0' ) ;
			for( int i = 0 ; i < count ; i++ )
			{
				unsigned int size = static_cast<unsigned int>(buffer.size()) ;
				if( DragQueryFileA( hdrop , i , &buffer[0] , size ) < size )
				{
					buffer.at(buffer.size()-1U) = '\0' ;
					G_DEBUG( "Cracker::onDrop: \"" << &buffer[0] << "\"" ) ;
					list.push_back( std::string(&buffer[0]) ) ;
				}
			}
			DragFinish( hdrop ) ;
			return !onDrop( list ) ;
		}

		case WM_SIZE:
		{
			SizeType type = SizeType::restored ;
			switch( wparam )
			{
				case SIZE_MAXIMIZED: type = SizeType::maximised ; break ;
				case SIZE_MINIMIZED: type = SizeType::minimised ; break ;
				case SIZE_RESTORED: type = SizeType::restored ; break ;
				case SIZE_MAXHIDE:
				case SIZE_MAXSHOW:
				default:
					return DefWindowProc( handle() , message , wparam , lparam ) ;
			}
			onSize( type , LOWORD(lparam) , HIWORD(lparam) ) ;
			return 0 ;
		}

		case WM_MOVE:
		{
			onMove( static_cast<INT16>(LOWORD(lparam)) , static_cast<INT16>(HIWORD(lparam)) ) ;
			return 0 ;
		}

		case WM_COMMAND:
		{
			UINT type = GET_WM_COMMAND_CMD( wparam , lparam ) ;
			const UINT menu = 0 ;
			const UINT accelerator = 1 ;

			if( type == menu || type == accelerator )
			{
				G_DEBUG( "Cracker::onMenuCommand" ) ;
				onMenuCommand( static_cast<UINT>(wparam) ) ;
			}

			else
			{
				HWND window = GET_WM_COMMAND_HWND( wparam , lparam ) ;
				UINT id = GET_WM_COMMAND_ID( wparam , lparam ) ;
				UINT command_message = type ;
				G_DEBUG( "Cracker::onControlCommand" ) ;
				onControlCommand( window , command_message , id ) ;
			}

			return 0 ;
		}

		case WM_ACTIVATE:
		{
			HWND window = reinterpret_cast<HWND>(lparam) ;
			bool processed = false ;
			if( LOWORD(wparam) )
			{
				bool by_mouse = LOWORD(wparam) == WA_CLICKACTIVE ;
				processed = onActivate( window , by_mouse ) ;
			}
			else
			{
				processed = onDeactivate( window ) ;
			}
			if( !processed )
				call_default = true ;
			return 0 ;
		}

		case WM_ACTIVATEAPP:
		{
			DWORD thread_id = static_cast<DWORD>( lparam ) ;
			bool processed = false ;
			if( wparam )
				processed = onActivateApp( thread_id ) ;
			else
				processed = onDeactivateApp( thread_id ) ;
			if( !processed )
				call_default = true ;
			return 0 ;
		}

		case WM_LBUTTONDBLCLK:
		{
			const unsigned int x = LOWORD(lparam) ;
			const unsigned int y = HIWORD(lparam) ;
			const unsigned int keys = static_cast<unsigned int>(wparam) ;
			onDoubleClick( x , y , keys ) ;
			return 0 ;
		}

		case WM_LBUTTONDOWN:
		{
			return doMouseButton( &Cracker::onLeftMouseButtonDown , MouseButton::Left ,
				MouseButtonDirection::Down , message , wparam , lparam ) ;
		}

		case WM_LBUTTONUP:
		{
			return doMouseButton( &Cracker::onLeftMouseButtonUp , MouseButton::Left ,
				MouseButtonDirection::Up , message , wparam , lparam ) ;
		}

		case WM_MBUTTONDOWN:
		{
			return doMouseButton( &Cracker::onMiddleMouseButtonDown , MouseButton::Middle ,
				MouseButtonDirection::Down , message , wparam , lparam ) ;
		}

		case WM_MBUTTONUP:
		{
			return doMouseButton( &Cracker::onMiddleMouseButtonUp , MouseButton::Middle ,
				MouseButtonDirection::Up , message , wparam , lparam ) ;
		}

		case WM_RBUTTONDOWN:
		{
			return doMouseButton( &Cracker::onRightMouseButtonDown , MouseButton::Right ,
				MouseButtonDirection::Down , message , wparam , lparam ) ;
		}

		case WM_RBUTTONUP:
		{
			return doMouseButton( &Cracker::onRightMouseButtonUp , MouseButton::Right ,
				MouseButtonDirection::Up , message , wparam , lparam ) ;
		}

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM( lparam ) ;
			int y = GET_Y_LPARAM( lparam ) ;
			const bool shift = !!( wparam & MK_SHIFT ) ;
			const bool control = !!( wparam & MK_CONTROL ) ;
			const bool left = !!( wparam & MK_LBUTTON ) ;
			const bool middle = !!( wparam & MK_MBUTTON ) ;
			const bool right = !!( wparam & MK_RBUTTON ) ;
			onMouseMove( x , y , shift , control , left , middle , right ) ;
			return 0 ;
		}

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO * mmi_p = reinterpret_cast<MINMAXINFO*>(lparam) ;
			G_ASSERT( mmi_p != nullptr ) ;
			int dx = mmi_p->ptMaxSize.x ;
			int dy = mmi_p->ptMaxSize.y ;
			onDimension( dx , dy ) ;
			mmi_p->ptMaxTrackSize.x = mmi_p->ptMaxSize.x = dx ;
			mmi_p->ptMaxTrackSize.y = mmi_p->ptMaxSize.y = dy ;
			return 0 ;
		}

		case WM_USER:
		{
			return onUser( wparam , lparam ) ;
		}

		case WM_USER+1U: // see wm_idle()
		{
			onIdle() ;
			return 0 ;
		}

		case WM_USER+2U: // see wm_tray()
		{
			if( lparam == WM_LBUTTONDBLCLK )
				onTrayDoubleClick() ;
			else if( lparam == WM_RBUTTONUP )
				onTrayRightMouseButtonUp() ;
			else if( lparam == WM_RBUTTONDOWN )
				onTrayRightMouseButtonDown() ;
			else if( lparam == WM_LBUTTONDOWN )
				onTrayLeftMouseButtonDown() ;
			return 1 ;
		}

		case WM_USER+3U: // see wm_quit()
		{
			// never gets here -- intercepted in GGui::Pump
			return 0 ;
		}

		case WM_USER+4U: // see wm_winsock()
		{
			onWinsock( wparam , lparam ) ;
			return 0 ;
		}

		case WM_USER+123U:
		{
			return onUserOther( wparam , lparam ) ;
		}

		case WM_TIMER:
		{
			onTimer( static_cast<unsigned int>(wparam) ) ;
			return 0 ;
		}

		case WM_INITMENUPOPUP:
		{
			onInitMenuPopup( hmenu_from(wparam) ,
				static_cast<unsigned int>(LOWORD(lparam)) , !!HIWORD(lparam) ) ;
			return 0 ;
		}

		case WM_QUERYNEWPALETTE:
		{
			bool realised = onPalette() ;
			return realised ? 1 : 0 ;
		}

		case WM_PALETTECHANGED:
		{
			HWND hwnd_other = hwnd_from( wparam ) ;
			if( handle() != hwnd_other )
				onPaletteChange() ;
			return 0 ;
		}
	}

	call_default = true ; // ie. call DefWindowProc()
	return 0L ; // ignored
}

unsigned int GGui::Cracker::wm_user()
{
	return WM_USER ;
}

unsigned int GGui::Cracker::wm_idle()
{
	return WM_USER+1U ;
}

unsigned int GGui::Cracker::wm_tray()
{
	return WM_USER+2U ;
}

unsigned int GGui::Cracker::wm_quit()
{
	return WM_USER+3U ;
}

unsigned int GGui::Cracker::wm_winsock()
{
	return WM_USER+4U ;
}

unsigned int GGui::Cracker::wm_user_other()
{
	return WM_USER+123U ;
}

LRESULT GGui::Cracker::onControlColour_( WPARAM wparam , LPARAM lparam , WORD id )
{
	return reinterpret_cast<LRESULT>( onControlColour( reinterpret_cast<HDC>(wparam) ,
		reinterpret_cast<HWND>(lparam) , id ) ) ;
}

// trivial default implementations of virtual functions...

HBRUSH GGui::Cracker::onControlColour( HDC , HWND , WORD )
{
	return 0 ;
}

void GGui::Cracker::onSysColourChange()
{
}

bool GGui::Cracker::onSysCommand( SysCommand )
{
	return false ;
}

bool GGui::Cracker::onCreate()
{
	return true ;
}

bool GGui::Cracker::onPaintMessage()
{
	return false ;
}

void GGui::Cracker::onPaint( HDC )
{
}

bool GGui::Cracker::onClose()
{
	return true ;
}

void GGui::Cracker::onDestroy()
{
}

void GGui::Cracker::onNcDestroy()
{
}

void GGui::Cracker::onMenuCommand( UINT )
{
}

void GGui::Cracker::onControlCommand( HWND , UINT , UINT )
{
}

bool GGui::Cracker::onDrop( const G::StringArray & )
{
	return false ;
}

void GGui::Cracker::onSize( SizeType , unsigned int , unsigned int )
{
}

void GGui::Cracker::onMove( int , int )
{
}

void GGui::Cracker::onGetFocus( HWND )
{
}

void GGui::Cracker::onLooseFocus( HWND )
{
}

void GGui::Cracker::onChar( WORD , unsigned int )
{
}

void GGui::Cracker::onDimension( int & , int & )
{
}

void GGui::Cracker::onDoubleClick( unsigned int , unsigned int , unsigned int )
{
}

void GGui::Cracker::onTrayDoubleClick()
{
}

void GGui::Cracker::onTrayLeftMouseButtonDown()
{
}

void GGui::Cracker::onTrayRightMouseButtonDown()
{
}

void GGui::Cracker::onTrayRightMouseButtonUp()
{
}

void GGui::Cracker::onTimer( unsigned int )
{
}

void GGui::Cracker::onInitMenuPopup( HMENU , unsigned int , bool )
{
}

void GGui::Cracker::onIdle()
{
}

LRESULT GGui::Cracker::onUser( WPARAM , LPARAM )
{
	return 0 ;
}

LRESULT GGui::Cracker::onUserOther( WPARAM , LPARAM )
{
	return 0 ;
}

bool GGui::Cracker::onActivate( HWND , bool )
{
	return false ;
}

bool GGui::Cracker::onDeactivate( HWND )
{
	return false ;
}

bool GGui::Cracker::onActivateApp( DWORD )
{
	return false ;
}

bool GGui::Cracker::onDeactivateApp( DWORD )
{
	return false ;
}

bool GGui::Cracker::onEraseBackground( HDC hdc )
{
	WPARAM wparam = reinterpret_cast<WPARAM>(hdc) ;
	return !! DefWindowProc( handle() , WM_ERASEBKGND , wparam , 0L ) ;
}

void GGui::Cracker::onMouseMove( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ ,
	bool /*left_button_down*/ , bool /*middle_button_down*/ ,
	bool /*right_button_down*/ )
{
}

void GGui::Cracker::onMouseButton( MouseButton , MouseButtonDirection ,
	int , int , bool , bool )
{
}

void GGui::Cracker::onLeftMouseButtonDown( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

void GGui::Cracker::onLeftMouseButtonUp( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

void GGui::Cracker::onMiddleMouseButtonDown( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

void GGui::Cracker::onMiddleMouseButtonUp( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

void GGui::Cracker::onRightMouseButtonDown( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

void GGui::Cracker::onRightMouseButtonUp( int /*x*/ , int /*y*/ ,
	bool /*shift_key_down*/ , bool /*control_key_down*/ )
{
}

bool GGui::Cracker::onPalette()
{
	return false ;
}

void GGui::Cracker::onPaletteChange()
{
}

void GGui::Cracker::onWinsock( WPARAM , LPARAM )
{
}

LRESULT GGui::Cracker::doMouseButton( Fn fn , MouseButton button ,
	MouseButtonDirection  direction , unsigned int ,
	WPARAM wparam , LPARAM lparam )
{
	int x = GET_X_LPARAM( lparam ) ;
	int y = GET_Y_LPARAM( lparam ) ;
	bool shift = !!( wparam & MK_SHIFT ) ;
	bool control = !!( wparam & MK_CONTROL ) ;
	onMouseButton( button , direction , x , y , shift , control ) ;
	(this->*fn)( x , y , shift , control ) ;
	return 0 ;
}

HWND GGui::Cracker::hwnd_from( WPARAM wparam )
{
	return reinterpret_cast<HWND>(wparam) ;
}

HDC GGui::Cracker::hdc_from( WPARAM wparam )
{
	return reinterpret_cast<HDC>(wparam) ;
}

HDROP GGui::Cracker::hdrop_from( WPARAM wparam )
{
	return reinterpret_cast<HDROP>(wparam) ;
}

HMENU GGui::Cracker::hmenu_from( WPARAM wparam )
{
	return reinterpret_cast<HMENU>(wparam) ;
}
