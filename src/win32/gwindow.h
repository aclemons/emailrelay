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
/// \file gwindow.h
///

#ifndef G_GUI_WINDOW_H
#define G_GUI_WINDOW_H

#include "gdef.h"
#include "gcracker.h"
#include "gsize.h"
#include <string>

namespace GGui
{
	class Window ;
}

//| \class GGui::Window
/// A window class. Window messages should be processed by overriding
/// the 'onWhatever()' virtual functions inherited from GGui::Cracker.
///
/// Recall that messages are passed through a global message queue
/// and dispatched to window procedures. Windows are created with
/// reference to a window class that is registered with a name and a
/// window procedure. A window is identified by a handle that points
/// to a hidden window structure containing a reference to the
/// window class. Windows messages have a message-id and two
/// parameters. The message-id is typically used in a big switch
/// statement within the window procedure. The GGui::Cracker
/// class contains a standardised window procedure that dispatches
/// to virtual functions.
///
/// This class uses the window's first SetWindowLongPtr() value
/// to point to the Window object. This is set during WM_CREATE
/// and reset in the destructor. After destruction of the Window
/// object all window messages are handled by DefWindowProc().
///
/// \see WindowBase
///
class GGui::Window : public Cracker
{
public:
	explicit Window( HWND hwnd = 0 ) ;
		///< Constructor. Normally the default constructor is used, followed
		///< by create(). Use registerWindowClass() before calling any
		///< create() function.

	virtual ~Window() ;
		///< Destructor. The Windows window is _not_ destroyed; its
		///< SetWindowLongPtr() value is reset and any window messages
		///< are handled by DefWindowProc().

	static bool registerWindowClass( const std::string & class_name ,
		HINSTANCE hinstance , UINT class_style , HICON icon ,
		HCURSOR cursor , HBRUSH background , UINT menu_resource_id = 0 ) ;
			///< Registers a Windows window class specifically for GGui::Window
			///< windows. The Windows window class points to a window procedure
			///< and in this case the window procedure is implemented by
			///< GGui::Window::wndProc(). In processing the WM_CREATE message
			///< the wndProc() method sets the SetWindowLongPtr() value to
			///< point to the GGui::Window object and in subsequent messages
			///< this is used to deliver the message to GGui::Cracker::crack().
			///<
			///< (Compare this to GGui::Dialog where the window (dialog)
			///< procedure is passed in when the dialog box is created; when
			///< the WM_INITDIALOG message is processed the dialog procedure
			///< sets the window long to point to the GGui::Dialog object.
			///< The WM_INITDIALOG message handling also maintains a list of
			///< modeless dialog boxes so that the message pump short-ciruits
			///< the DispatchMessage() call. The GGUi::Dialog dialog procedure
			///< does its own limited message cracking independently of the
			///< GGui::Cracker class.)
			///<
			///< All GGui::Window objects must have their Windows window class
			///< registered in this way before any window is created.
			///<
			///< Returns true on success. Fails beningly if the class is already
			///< registered.
			///<
			///< Typical values for the 'class_style', 'icon', 'cursor' and
			///< 'background' parameters can be obtained from the static methods
			///< classStyle(), classIcon() , classCursor() and classBrush().
			///<
			///< See also ::RegisterClass() and struct WNDCLASS.

	bool create( const std::string & class_name ,
		const std::string & title , std::pair<DWORD,DWORD> window_style ,
		int x , int y , int dx , int dy ,
		HWND parent , HMENU menu_or_child_id , HINSTANCE hinstance ) ;
			///< Creates the window. Returns true on success.
			///<
			///< The given window class name must be the name of a window
			///< class previously registered through registerWindowClass().
			///<
			///< Typical values for the 'window_style' parameter can
			///< be obtained from the static methods windowStyleMain(),
			///< windowStylePopup() and windowStyleChild(). A value of
			///< zero defaults to windowStylePopupNoButton().
			///<
			///< The window size and location parameters may be specified
			///< using CW_USEDEFAULT.
			///<
			///< The 'parent' parameter may be null for POPUP windows
			///< only.
			///<
			///< The 'menu_or_child_id' is a unique child-window identifier
			///< if the window style is CHILD. Otherwise it is a menu handle.
			///< A null menu handle means that the window-class menu is used,
			///< as defined through registerWindowClass().
			///<
			///< See also ::CreateWindow().

	void update() ;
		///< Does ::UpdateWindow().

	void show( int style = SW_SHOW ) ;
		///< Does ::ShowWindow().

	void destroy() ;
		///< Does ::DestroyWindow().

	void invalidate( bool erase = true ) ;
		///< Invalidates the window so that it redraws.

	static Size borderSize( bool has_menu ) ;
		///< Returns the size of the border of a _typical_ main window.
		///< The actual border size will depend on the window style and
		///< its size (since the menu bar changes height at run-time).

	void resize( Size new_size , bool repaint = true ) ;
		///< Resizes the window. The top-left corner stays put.

	static Window * instance( HWND hwnd ) ;
		///< Maps from a window handle to a Window object. The handle
		///< must be that of a Window window.

	static LRESULT sendUserString( HWND hwnd , const char * string ) ;
		///< Sends a string to a specified window. The other window
		///< will receive a onUserString() message.

	static UINT classStyle( bool redraw = false ) ;
		///< Returns a general-purpose value for
		///< resisterWindowClass(class_style).

	static std::pair<DWORD,DWORD> windowStyleMain() ;
		///< Returns a value for create() window_style for a typical
		///< 'main' window.
		///<
		///< The create() parent window parameter should be null, and
		///< the x,y,dx,dy parameters will normally be CW_USEDEFAULT.

	static std::pair<DWORD,DWORD> windowStylePopup() ;
		///< Returns a value for create() window_style for a typical
		///< 'popup' window ie. (in this case) a window which typically
		///< acts like a modeless dialog box -- it can be independetly
		///< activated, has a title bar but no minimise/maximise buttons,
		///< and stays on top of its parent (if any).
		///<
		///< The create() parent window parameter may be null -- if
		///< null then this window will be independently iconised with
		///< a separate button on the toolbar.

	static std::pair<DWORD,DWORD> windowStyleChild() ;
		///< Returns a value for create() window_style for a typical
		///< 'child' window.
		///<
		///< The create() parent window parameter cannot be null.

	static std::pair<DWORD,DWORD> windowStylePopupNoButton() ;
		///< Returns a value for create() window_style for a non-visible
		///< popup window with the 'exclude-from-toolbar' extended
		///< style.

	static HBRUSH classBrush() ;
		///< Returns a default value for registerWindowClass(background).

	static HICON classIcon() ;
		///< Returns a default for registerWindowClass(hicon).

	static HCURSOR classCursor() ;
		///< Returns a default for registerWindowClass(hcursor).

	static LRESULT wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
		///< Called directly from the global, exported window
		///< procedure. The implementation locates the particular
		///< Window object and calls its non-static wndProc()
		///< method.

	bool wndProcException() const ;
		///< Returns true if an exception was thrown and caught
		///< just before the wndproc returned.

	std::string wndProcExceptionString() ;
		///< Returns and clears the exception string.

protected:
	virtual LRESULT onUserString( const char *string ) ;
		///< Overridable. Called when the window receives a message
		///< from sendUserString().

	virtual void onWindowException( std::exception & e ) = 0 ;
		///< Called if an exception is being thrown out of the
		///< window procedure. The default implementation
		///< posts a quit message.

public:
	Window( const Window & ) = delete ;
	Window( Window && ) = delete ;
	Window & operator=( const Window & ) = delete ;
	Window & operator=( Window && ) = delete ;

private:
	static LRESULT wndProcCore( Window * , HWND , UINT , WPARAM , LPARAM ) ;
	virtual LRESULT onUserOther( WPARAM , LPARAM ) ;
	static Window * instance( CREATESTRUCT * ) ;

private:
	std::string m_reason ;
} ;

#endif
