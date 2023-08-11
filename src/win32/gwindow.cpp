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
/// \file gwindow.cpp
///

#include "gdef.h"
#include "gwindow.h"
#include "gpump.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"

extern "C" LRESULT CALLBACK gwindow_wndproc_export( HWND , UINT , WPARAM , LPARAM ) ;

GGui::Window::Window( HWND hwnd ) :
	Cracker(hwnd)
{
	G_DEBUG( "GGui::Window::ctor: this = " << this ) ;
}

GGui::Window::~Window()
{
	if( handle() )
		SetWindowLongPtr( handle() , 0 , 0 ) ;
}

bool GGui::Window::registerWindowClass( const std::string & class_name_in ,
	HINSTANCE hinstance , UINT style ,
	HICON icon , HCURSOR cursor ,
	HBRUSH background , UINT menu_resource_id )
{
	G_DEBUG( "GGui::Window::registerWindowClass: \"" << class_name_in << "\"" ) ;

	LPTSTR menu_name = menu_resource_id ? MAKEINTRESOURCE(menu_resource_id) : 0 ;

	std::basic_string<TCHAR> class_name ;
	G::Convert::convert( class_name , class_name_in ) ;

	WNDCLASS c ;
	c.style = style ;
	c.lpfnWndProc = gwindow_wndproc_export ;
	c.cbClsExtra = 8 ; // reserved
	c.cbWndExtra = 8 ; // for pointer to Window
	c.hInstance = hinstance ;
	c.hIcon = icon ;
	c.hCursor = cursor ;
	c.hbrBackground = background ;
	c.lpszMenuName = menu_name ;
	c.lpszClassName = class_name.c_str() ;

	return RegisterClass( &c ) != 0 ;
}

bool GGui::Window::create( const std::string & class_name ,
	const std::string & title , std::pair<DWORD,DWORD> style_pair ,
	int x , int y , int dx , int dy ,
	HWND parent , HMENU menu , HINSTANCE hinstance )
{
	G_ASSERT( handle() == HNULL ) ;
	G_DEBUG( "GGui::Window::create: \"" << class_name << "\", \"" << title << "\"" ) ;

	DWORD style = style_pair.first ;
	DWORD extended_style = style_pair.second ;

	void *vp = reinterpret_cast<void*>(this) ;
	HWND hwnd = CreateWindowExA( extended_style , class_name.c_str() , title.c_str() ,
		style , x , y , dx , dy , parent , menu , hinstance , vp ) ;
	setHandle( hwnd ) ; // GGui::WindowBase

	G_DEBUG( "GGui::Window::create: handle " << handle() ) ;
	return handle() != HNULL ;
}

void GGui::Window::update()
{
	G_ASSERT( handle() != HNULL ) ;
	UpdateWindow( handle() ) ;
}

void GGui::Window::show( int style )
{
	G_ASSERT( handle() != HNULL ) ;
	ShowWindow( handle() , style ) ;
}

void GGui::Window::invalidate( bool erase )
{
	InvalidateRect( handle() , nullptr , erase ) ;
}

GGui::Window * GGui::Window::instance( HWND hwnd )
{
	G_ASSERT( hwnd != HNULL ) ;
	LONG_PTR wl = GetWindowLongPtr( hwnd , 0 ) ;
	void * vp = reinterpret_cast<void*>(wl) ;
	return reinterpret_cast<Window*>(vp) ;
}

extern "C" LRESULT CALLBACK gwindow_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		return GGui::Window::wndProc( hwnd , message , wparam , lparam ) ;
	}
	catch( std::exception & e )
	{
		// never gets here
		GDEF_IGNORE_PARAM( e ) ;
		G_DEBUG( "gwindow_wndproc_export: exception absorbed: " << e.what() ) ;
		return 0 ;
	}
	catch(...) // callback
	{
		// never gets here
		return 0 ;
	}
}

bool GGui::Window::wndProcException() const
{
	return !m_reason.empty() ;
}

std::string GGui::Window::wndProcExceptionString()
{
	std::string reason = m_reason ;
	m_reason.resize(0) ;
	return reason ;
}

LRESULT GGui::Window::wndProc( HWND hwnd , UINT msg , WPARAM wparam , LPARAM lparam )
{
	Window * window = msg == WM_CREATE ?
		instance( reinterpret_cast<CREATESTRUCT*>(lparam) ) :
		instance( hwnd ) ;

	LRESULT result = msg == WM_CREATE ? -1 : 0 ;
	try
	{
		result = wndProcCore( window , hwnd , msg , wparam , lparam ) ;
	}
	catch( std::exception & e )
	{
		G_DEBUG( "GGui::Window::wndProc: exception from window message handler: " << e.what() ) ;
		if( msg == WM_CREATE )
		{
			G_ERROR( "GGui::Window::wndProc: exception from window creation message handler: " << e.what() ) ;
			window->m_reason = e.what() ;
		}
		else if( msg != WM_NCDESTROY )
		{
			try
			{
				window->onWindowException( e ) ; // overridable - posts wm_quit by default
			}
			catch( std::exception & ee )
			{
				// exception from exception handler - save it for wndProcException()
				window->m_reason = ee.what() ;
			}
		}
	}
	return result ;
}

GGui::Window * GGui::Window::instance( CREATESTRUCT * cs )
{
	G_ASSERT( cs != nullptr ) ;
	return reinterpret_cast<Window*>(cs->lpCreateParams) ;
}

LRESULT GGui::Window::wndProcCore( Window * window , HWND hwnd , UINT msg , WPARAM wparam , LPARAM lparam )
{
	LRESULT result = 0 ;
	if( msg == WM_CREATE )
	{
		G_ASSERT( window != nullptr ) ;
		G_DEBUG( "GGui::Window::wndProc: WM_CREATE: ptr " << window ) ;
		G_DEBUG( "GGui::Window::wndProc: WM_CREATE: hwnd " << hwnd ) ;
		void * vp = reinterpret_cast<void*>(window) ;
		LONG_PTR wl = reinterpret_cast<LONG_PTR>(vp) ;
		SetWindowLongPtr( hwnd , 0 , wl ) ;
		window->setHandle( hwnd ) ;
		result = window->onCreate() ? 0 : -1 ;
	}
	else
	{
		bool call_default = window == nullptr ;
		if( window != nullptr )
			result = window->crack( msg , wparam , lparam , call_default ) ;
		if( call_default )
			result = DefWindowProc( hwnd , msg , wparam , lparam ) ;
	}
	return result ;
}

LRESULT GGui::Window::sendUserString( HWND hwnd , const char * string )
{
	G_ASSERT( string != nullptr ) ;
	return SendMessage( hwnd , Cracker::wm_user_other() , 0 , reinterpret_cast<LPARAM>(string) ) ;
}

LRESULT GGui::Window::onUserOther( WPARAM , LPARAM lparam )
{
	return onUserString( reinterpret_cast<char *>(lparam) ) ;
}

LRESULT GGui::Window::onUserString( const char * )
{
	return 0 ;
}

void GGui::Window::destroy()
{
	DestroyWindow( handle() ) ;
}

namespace GGui
{
	namespace WindowImp
	{
		std::pair<DWORD,DWORD> make_style( DWORD first , DWORD second = 0 )
		{
			return { first , second } ;
		}
	}
}

std::pair<DWORD,DWORD> GGui::Window::windowStyleMain()
{
	return WindowImp::make_style( WS_OVERLAPPEDWINDOW ) ;
}

std::pair<DWORD,DWORD> GGui::Window::windowStylePopup()
{
	return
		WindowImp::make_style(
			WS_THICKFRAME |
			WS_POPUP |
			WS_SYSMENU |
			WS_CAPTION |
			WS_VISIBLE |
		0 ) ;
}

std::pair<DWORD,DWORD> GGui::Window::windowStyleChild()
{
	return WindowImp::make_style( WS_CHILDWINDOW ) ;
}

std::pair<DWORD,DWORD> GGui::Window::windowStylePopupNoButton()
{
	return WindowImp::make_style( WS_POPUP , WS_EX_TOOLWINDOW ) ;
}

UINT GGui::Window::classStyle( bool redraw )
{
	return
		redraw ?
			(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW) :
			(CS_DBLCLKS) ;
}

HBRUSH GGui::Window::classBrush()
{
	return (HBRUSH)( 1 + COLOR_BACKGROUND ) ;
}

HICON GGui::Window::classIcon()
{
	return LoadIcon( HNULL , IDI_APPLICATION ) ;
}

HCURSOR GGui::Window::classCursor()
{
	return LoadCursor( HNULL , IDC_ARROW ) ;
}

GGui::Size GGui::Window::borderSize( bool has_menu )
{
	// note that the Win3.1 and Win32 help files
	// have different definitions of these metrics
	// wrt. the +/- CYBORDER -- needs more testing

	// See also AdjustWindowRect, AdjustWindowRectEx and
	// MSDN 4 "Ask Dr GUI #10".

	Size size ;
	size.dx = GetSystemMetrics( SM_CXFRAME ) * 2 ;
	size.dy = GetSystemMetrics( SM_CYFRAME ) * 2 ;
	size.dy += GetSystemMetrics( SM_CYCAPTION ) - GetSystemMetrics( SM_CYBORDER ) ;
	if( has_menu )
		size.dy += GetSystemMetrics( SM_CYMENU ) + GetSystemMetrics( SM_CYBORDER ) ;
	return size ;
}

void GGui::Window::resize( Size new_size , bool repaint )
{
	// note that GetWindowRect() returns coordinates relative to the
	// top left corner of the screen -- MoveWindow() takes coordinates
	// relative to the screen for top-level windows, but relative to
	// the parent window for child windows

	RECT rect ;
	if( GetWindowRect( handle() , &rect ) )
	{
		HWND parent = GetParent( handle() ) ;
		const bool child_window = parent != 0 ;
		if( child_window )
		{
			rect.left = 0 ;
			rect.top = 0 ;
		}

		MoveWindow( handle() , rect.left , rect.top , new_size.dx , new_size.dy , repaint ) ;
	}
}

void GGui::Window::onWindowException( std::exception & e )
{
	// overridable - default implementation...
	G_DEBUG( "GGui::Window::onWindowException: quitting event loop: hwnd " << handle() << ": " << e.what() ) ;
	Pump::quit( e.what() ) ;
}

