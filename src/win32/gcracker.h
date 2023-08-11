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
/// \file gcracker.h
///

#ifndef G_GUI_CRACKER_H
#define G_GUI_CRACKER_H

#include "gdef.h"
#include "gwinbase.h"
#include "gstringarray.h"
#include <string>
#include <list>

namespace GGui
{
	class Cracker ;
}

//| \class GGui::Cracker
/// The Cracker class encapsulates a typical window procedure by
/// 'cracking' Windows messages into virtual functions.
///
class GGui::Cracker : public WindowBase
{
public:
	explicit Cracker( HWND hwnd ) ;
		///< Constructor. The window handle is passed on to
		///< the base class.

	virtual ~Cracker() ;
		///< Virtual destructor.

	LRESULT crack( unsigned int msg , WPARAM w , LPARAM l , bool & default_ ) ;
		///< Cracks the given message, calling virtual functions as
		///< appropriate. If the message is not processed then 'default_'
		///< is set to true: the user should then normally call
		///< DefWindowProc().

	static unsigned int wm_user() ;
		///< Returns the WM_USER message number. See onUser().

	static unsigned int wm_winsock() ;
		///< Returns a message number which is recommended for
		///< winsock messages. See onWinsock().

	static unsigned int wm_idle() ;
		///< Returns a message number which should be used for
		///< idle messages. See onIdle() and GGui::Pump.

	static unsigned int wm_tray() ;
		///< Returns a message number which should be used for
		///< system-tray notification messages.
		///< See GGui::Tray.

	static unsigned int wm_quit() ;
		///< Returns a message number which can be used
		///< as an alternative to WM_QUIT. See also GGui::Pump.

	static unsigned int wm_user_other() ;
		///< Returns a message number used for onUserOther().

protected:
	virtual bool onEraseBackground( HDC hdc ) ;
		///< Overridable. Called when the window receives a
		///< WM_ERASEBKGND message. The default implementation
		///< uses the brush from the window class registration.
		///< Returns true if the background was erased.

	virtual HBRUSH onControlColour( HDC hDC , HWND hWndControl , WORD type ) ;
		///< Overridable. Called when the window receives a
		///< WM_CTLCOLOR message.

	virtual void onSysColourChange() ;
		///< Overridable. Called when the window receives a
		///< WM_SYSCOLORCHANGE message.

	enum class SysCommand { scMaximise , scMinimise , scClose , scSize /*etc*/ } ;
	virtual bool onSysCommand( SysCommand sys_command ) ;
		///< Overridable. Called when the window receives a
		///< WM_SYSCOMMAND message. Returns true if processed.

	virtual bool onCreate() ;
		///< Overridable. Called when the window receives a
		///< WM_CREATE message. The main window should return
		///< false if the application should fail to start up.

	virtual bool onPaintMessage() ;
		///< Overridable. Called when the window receives a
		///< WM_PAINT message, before ::BeginPaint() is called.
		///< If the override returns true then the message is
		///< considered to be fully processed and onPaint()
		///< is not used.

	virtual void onPaint( HDC dc ) ;
		///< Overridable. Called when the window receives a
		///< WM_PAINT message, after ::BeginPaint().

	virtual bool onClose() ;
		///< Overridable. Called when the window receives a
		///< WM_CLOSE message. The main window should return
		///< true if the application should terminate.

	virtual void onDestroy() ;
		///< Overridable. Called when the window receives a
		///< WM_DESTROY message.

	virtual void onNcDestroy() ;
		///< Overridable. Called when the window receives a
		///< WM_NCDESTROY message.

	virtual void onMenuCommand( UINT id ) ;
		///< Overridable. Called when the window receives a
		///< WM_COMMAND message resulting from a menu action.

	virtual void onControlCommand( HWND hwnd , UINT message , UINT id ) ;
		///< Overridable. Called when the window receives a
		///< WM_COMMAND message from a control.

	virtual bool onDrop( const G::StringArray & files ) ;
		///< Overridable. Called when the window receives a
		///< WM_DROPFILES message. Returns false if the file list
		///< is ignored. See also: DragAcceptFiles().

	enum class SizeType { maximised , minimised , restored } ;
	virtual void onSize( SizeType type , unsigned int dx , unsigned int dy ) ;
		///< Overridable. Called on receipt of a WM_SIZE message.

	virtual void onMove( int x , int y ) ;
		///< Overridable. Called on receipt of a WM_MOVE message.

	virtual void onLooseFocus( HWND to ) ;
		///< Overridable. Called on receipt of a WM_KILLFOCUS message.

	virtual void onGetFocus( HWND from ) ;
		///< Overridable. Called on receipt of a WM_SETFOCUS message,
		///< indicating that this windows has just received input
		///< focus. Typically this is overridden with a call to
		///< ::SetFocus(), passing focus on to some more appropriate
		///< window.

	virtual bool onActivate( HWND other_window , bool by_mouse ) ;
		///< Overridable. Called on receipt of a WA_ACTIVE WM_ACTIVATE message.
		///< Returns true if processed.

	virtual bool onDeactivate( HWND other_window ) ;
		///< Overridable. Called on receipt of a WA_INACTIVE WM_ACTIVATE message.
		///< Returns true if processed.

	virtual bool onActivateApp( DWORD ) ;
		///< Overridable. Called on receipt of a TRUE WM_ACTIVATEAPP message.
		///< Returns true if processed.

	virtual bool onDeactivateApp( DWORD ) ;
		///< Overridable. Called on receipt of a FALSE WM_ACTIVATEAPP message.
		///< Returns true if processed.

	virtual void onChar( WORD vkey , unsigned int repeat_count ) ;
		///< Overridable. Called on receipt of a WM_CHAR message.

	virtual void onDimension( int &dx , int &dy ) ;
		///< Overridable. Called on receipt of a WM_MINMAXINFO message.

	virtual void onDoubleClick( unsigned int x , unsigned int y , unsigned int keys ) ;
		///< Overridable. Called when the left mouse button is
		///< double clicked (but depending on the window
		///< class-style).

	virtual void onTrayDoubleClick() ;
		///< Overridable. Called when the left mouse button is double
		///< clicked on the window's system-tray icon.
		/// \see GGui::Tray

	virtual void onTrayLeftMouseButtonDown() ;
		///< Overridable. Called when the left mouse button is clicked
		///< on the window's system-tray icon.
		/// \see GGui::Tray

	virtual void onTrayRightMouseButtonDown() ;
		///< Overridable. Called when the right mouse button is clicked
		///< on the window's system-tray icon.
		/// \see GGui::Tray

	virtual void onTrayRightMouseButtonUp() ;
		///< Overridable. Called when the right mouse button is released
		///< on the window's system-tray icon.
		/// \see GGui::Tray

	virtual void onTimer( unsigned int id ) ;
		///< Overridable. Called on receipt of a WM_TIMER message.

	virtual LRESULT onUser( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a WM_USER message.

	virtual LRESULT onUserOther( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a wm_user_other()
		///< message.

	virtual void onWinsock( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a wm_winsock()
		///< message.

	virtual void onInitMenuPopup( HMENU hmenu , unsigned int position , bool system_menu ) ;
		///< Overridable. Called just before a popup menu
		///< is displayed. Overrides will normally enable
		///< and/or grey out menu items as appropriate.

	enum class MouseButton { Left , Middle , Right } ;
	enum class MouseButtonDirection { Up , Down } ;

	virtual void onMouseMove( int x , int y ,
		bool shift_key_down , bool control_key_down ,
		bool left_button_down , bool middle_button_down ,
		bool right_button_down ) ;
			///< Overridable. Called on receipt of a mouse-move message.
			///< The origin is top left and co-ordindates can be negative.

	virtual void onMouseButton( MouseButton , MouseButtonDirection ,
		int x , int y , bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse
			///< button-down/button-up message. The origin is top left
			///< and co-ordindates can be negative. Called before
			///< the separate functions below.

	virtual void onLeftMouseButtonDown( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse left-button-down
			///< message. The origin is top left.

	virtual void onLeftMouseButtonUp( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse left-button-up
			///< message. The origin is top left and co-ordindates can be negative.

	virtual void onMiddleMouseButtonDown( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse middle-button-down
			///< message. The origin is top left.

	virtual void onMiddleMouseButtonUp( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse middle-button-up
			///< message. The origin is top left and co-ordindates can be negative.

	virtual void onRightMouseButtonDown( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse right-button-down
			///< message. The origin is top left.

	virtual void onRightMouseButtonUp( int x , int y ,
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse right-button-up
			///< message. The origin is top left and co-ordindates can be negative.

	virtual bool onPalette() ;
		///< Called when the window gets focus, allowing it to realise
		///< its own palette into the system-wide hardware palette. If
		///< the window has a palette it should realise it and return
		///< true. If it has no palette it should return false.
		///<
		/// \see WM_QUERYNEWPALETTE

	virtual void onPaletteChange() ;
		///< Called when some other window changes the system-wide
		///< hardware palette.
		///<
		///< If a window has a palette, but it ignores this message,
		///< then the window's colors will be bogus while another
		///< application has input focus.
		///<
		/// \see WM_PALETTECHANGED

	virtual void onIdle() ;
		///< Called when a wm_idle() message is posted.

public:
	Cracker( const Cracker & ) = delete ;
	Cracker( Cracker && ) = delete ;
	Cracker & operator=( const Cracker & ) = delete ;
	Cracker & operator=( Cracker && ) = delete ;

private:
	using Fn = void (Cracker::*)(int,int,bool,bool) ;
	LRESULT doMouseButton( Fn fn , MouseButton , MouseButtonDirection , unsigned int , WPARAM , LPARAM ) ;
	LRESULT onControlColour_( WPARAM hDC , LPARAM hWndControl , WORD type ) ;
	static HWND hwnd_from( WPARAM ) ;
	static HDC hdc_from( WPARAM ) ;
	static HDROP hdrop_from( WPARAM ) ;
	static HMENU hmenu_from( WPARAM ) ;
} ;

#endif
