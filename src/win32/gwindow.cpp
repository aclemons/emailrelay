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
/// \file gwindow.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gwindow.h"
#include "gpump.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"

namespace GGui
{
	namespace WindowImp
	{
		LRESULT CALLBACK wndProc( HWND , UINT , WPARAM , LPARAM ) ;
		std::pair<DWORD,DWORD> make_style( DWORD first , DWORD second = 0 )
		{
			return { first , second } ;
		}
		Window * instance( CREATESTRUCT * cs )
		{
			return cs ? reinterpret_cast<Window*>(cs->lpCreateParams) : nullptr ;
		}
	}
}

GGui::Window::Window( HWND hwnd ) :
	Cracker(hwnd)
{
	G_DEBUG( "GGui::Window::ctor: this = " << this ) ;
}

GGui::Window::~Window()
{
	static_assert( noexcept(handle()) , "" ) ;
	static_assert( noexcept(G::nowide::setWindowLongPtr(handle(),0,0)) , "" ) ;
	if( handle() )
		G::nowide::setWindowLongPtr( handle() , 0 , 0 ) ;
}

bool GGui::Window::registerWindowClass( const std::string & class_name ,
	HINSTANCE hinstance , UINT style ,
	HICON icon , HCURSOR cursor ,
	HBRUSH background , UINT menu_resource_id )
{
	G_DEBUG( "GGui::Window::registerWindowClass: \"" << class_name << "\"" ) ;

	// see also IsWindowUnicode()

	G::nowide::WNDCLASS_type wc {} ;
	wc.style = style ;
	wc.lpfnWndProc = WindowImp::wndProc ;
	static_assert( sizeof(LONG_PTR) <= 8 , "" ) ;
	wc.cbClsExtra = 8 ; // for SetClassLong(0) -- not used here
	wc.cbWndExtra = 8 ; // for SetWindowLong(0) -- used for a pointer to the Window object -- keeps GWLP_USERDATA free
	wc.hInstance = hinstance ;
	wc.hIcon = icon ;
	wc.hCursor = cursor ;
	wc.hbrBackground = background ;

	return G::nowide::registerClass( wc , class_name , menu_resource_id ) != 0 ;
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

	void * vp = reinterpret_cast<void*>(this) ;
	HWND hwnd = G::nowide::createWindowEx( extended_style , class_name , title ,
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

LRESULT CALLBACK GGui::WindowImp::wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	return Window::wndProc( hwnd , message , wparam , lparam ) ;
}

LRESULT GGui::Window::wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	if( message == WM_CREATE )
	{
		Window * window = WindowImp::instance( reinterpret_cast<CREATESTRUCT*>(lparam) ) ;
		try
		{
			void * vp = reinterpret_cast<void*>( window ) ;
			LONG_PTR wl = reinterpret_cast<LONG_PTR>( vp ) ;
			G::nowide::setWindowLongPtr( hwnd , 0 , wl ) ;
			window->setHandle( hwnd ) ;
			bool ok = window->onCreate() ;
			return ok ? 0 : -1 ;
		}
		catch( std::exception & e )
		{
			window->m_reason = e.what() ;
			return -1 ;
		}
	}
	else
	{
		GGui::Window * window = GGui::Window::instance( hwnd ) ;
		if( window )
		{
			LRESULT result = 0 ;
			bool call_default = false ;
			try
			{
				result = window->crack( message , wparam , lparam , call_default ) ; // GGui::Cracker
			}
			catch( std::exception & e )
			{
				if( message != WM_NCDESTROY )
					window->m_reason = e.what() ;
				call_default = true ; // moot
			}
			if( call_default )
				result = G::nowide::defWindowProc( hwnd , message , wparam , lparam ) ;
			return result ;
		}
		else
		{
			return G::nowide::defWindowProc( hwnd , message , wparam , lparam ) ;
		}
	}
}

GGui::Window * GGui::Window::instance( HWND hwnd )
{
	LONG_PTR wl = G::nowide::getWindowLongPtr( hwnd , 0 ) ;
	void * vp = reinterpret_cast<void*>(wl) ;
	return reinterpret_cast<Window*>(vp) ;
}

LRESULT GGui::Window::sendUserString( HWND hwnd , const char * string )
{
	G_ASSERT( string != nullptr ) ;
	return G::nowide::sendMessage( hwnd , Cracker::wm_user_other() , 0 , reinterpret_cast<LPARAM>(string) ) ;
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
	return G::nowide::loadIconApplication() ;
}

HCURSOR GGui::Window::classCursor()
{
	return G::nowide::loadCursorArrow() ;
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

std::string GGui::Window::reason() const
{
	return m_reason ;
}

void GGui::Window::setReason( const std::string & s1 , const std::string & s2 )
{
	m_reason = s1 + std::string(": ",s1.empty()||s2.empty()?0U:2U) + s2 ;
}

