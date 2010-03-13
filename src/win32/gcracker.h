//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_CRACKER_H
#define G_CRACKER_H

#include "gdef.h"
#include "gwinbase.h"
#include "gstrings.h"
#include <string>
#include <list>

/// \namespace GGui
namespace GGui
{
	class Cracker ;
}

/// \class GGui::Cracker
/// The Cracker class encapsulates a typical
/// window procedure by 'cracking' Windows messages
/// into virtual functions. 
///
class GGui::Cracker : public GGui::WindowBase 
{
public:

	explicit Cracker( HWND hwnd ) ;
		///< Constructor.

	virtual ~Cracker() ;
		///< Virtual destructor.

	Cracker( const Cracker &other ) ;
		///< Copy constructor.

	Cracker & operator=( const Cracker & other ) ;
		///< Assignment operator.

	LRESULT crack( unsigned msg , WPARAM w , LPARAM l , bool &defolt ) ;
		///< Cracks the given message, calling
		///< virtual functions as appropriate.
		///< If the message is not processed
		///< then 'defolt' is set to true: the
		///< user should then normally call
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
		///< Overridable. Called when the window
		///< receives a WM_ERASEBKGND message. The default
		///< implementation uses the brush from the
		///< window class registration. Returns true
		///< if the background was erased.

	virtual HBRUSH onControlColour( HDC hDC , HWND hWndControl , WORD type ) ;
		///< Overridable. Called when the window
		///< receives a WM_CTLCOLOR message.
		///<
		///< Under Win32 all the various MW_CTLCOLOR
		///< messages call this method.

	virtual void onSysColourChange() ;
		///< Overridable. Called when the window
		///< receives a WM_SYSCOLORCHANGE message.

	enum SysCommand { scMaximise , scMinimise , scClose , scSize /*etc*/ } ;
	virtual bool onSysCommand( SysCommand sys_command ) ;
		///< Overridable. Called when the window
		///< receives a WM_SYSCOMMAND message.
		///< Returns true if processed.

	virtual bool onCreate() ;
		///< Overridable. Called when the window
		///< receives a WM_CREATE message.
		///< The main window should return false
		///< if the application should fail to start up.

	virtual bool onPaintMessage() ;
		///< Overridable. Called when the window
		///< receives a WM_PAINT message, before
		///< ::BeginPaint() is called.
		///< If the override returns true then
		///< the message is considered to be
		///< fully processed and onPaint()
		///< is not used.

	virtual void onPaint( HDC dc ) ;
		///< Overridable. Called when the window
		///< receives a WM_PAINT message,
		///< after ::BeginPaint().

	virtual bool onClose() ;
		///< Overridable. Called when the window
		///< receives a WM_CLOSE message. The main
		///< window should return true if the
		///< application should terminate.

	virtual void onDestroy() ;
		///< Overridable. Called when the window
		///< receives a WM_DESTROY message.

	virtual void onNcDestroy() ;
		///< Overridable. Called when the window
		///< receives a WM_NCDESTROY message.

	virtual void onMenuCommand( UINT id ) ;
		///< Overridable. Called when the window
		///< receives a WM_COMMAND message resulting
		///< from a menu action.

	virtual void onControlCommand( HWND hwnd , UINT message , UINT id ) ;
		///< Overridable. Called when the window
		///< receives a WM_COMMAND message from a
		///< control.

	virtual bool onDrop( const G::Strings & files ) ;  
		///< Overridable. Called when the window
		///< receives a WM_DROPFILES message.
		///< Returns false if the file list
		///< is ignored. See also: DragAcceptFiles().

	enum SizeType { maximised , minimised , restored } ;
	virtual void onSize( SizeType type , unsigned dx , unsigned dy ) ;
		///< Overridable. Called on receipt of
		///< a WM_SIZE message.

	virtual void onMove( int x , int y ) ;
		///< Overridable. Called on receipt of
		///< a WM_MOVE message.

	virtual void onLooseFocus( HWND to ) ;
		///< Overridable. Called on receipt of
		///< a WM_KILLFOCUS message.

	virtual void onGetFocus( HWND from ) ;
		///< Overridable. Called on receipt of
		///< a WM_SETFOCUS message, indicating that
		///< this windows has just received input focus.
		///< Typically this is overridden with a
		///< call to ::SetFocus(), passing focus on to
		///< some more appropriate window.

	virtual void onChar( WORD vkey , unsigned repeat_count ) ; 
		///< Overridable. Called on receipt of
		///< a WM_CHAR message.

	virtual void onDimension( int &dx , int &dy ) ;
		///< Overridable. Called on receipt of
		///< a WM_MINMAXINFO message.

	virtual void onDoubleClick( unsigned x , unsigned y , unsigned keys ) ;
		///< Overridable. Called when the left mouse
		///< button is double clicked (but depending
		///< on the window class-style).

	virtual void onTrayDoubleClick() ;
		///< Overridable. Called when the left mouse
		///< button is double clicked on the window's
		///< system-tray icon.
		///< See also: GGui::Tray

	virtual void onTrayLeftMouseButtonDown() ;
		///< Overridable. Called when the left mouse
		///< button is clicked on the window's
		///< system-tray icon.
		///< See also: GGui::Tray

	virtual void onTrayRightMouseButtonDown() ;
		///< Overridable. Called when the right mouse
		///< button is clicked on the window's
		///< system-tray icon.
		///< See also: GGui::Tray

	virtual void onTrayRightMouseButtonUp() ;
		///< Overridable. Called when the right mouse
		///< button is released on the window's
		///< system-tray icon.
		///< See also: GGui::Tray

	virtual void onTimer( unsigned id ) ;
		///< Overridable. Called on receipt of a WM_TIMER
		///< message.

	virtual LRESULT onUser( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a WM_USER
		///< message.

	virtual LRESULT onUserOther( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a wm_user_other()
		///< message.

	virtual void onWinsock( WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of a wm_winsock()
		///< message.

	virtual void onInitMenuPopup( HMENU hmenu , unsigned position , bool system_menu ) ;
		///< Overridable. Called just before a popup menu
		///< is displayed. Overrides will normally enable
		///< and/or grey out menu items as appropriate.

	virtual void onMouseMove( unsigned x , unsigned y , 
		bool shift_key_down , bool control_key_down ,
		bool left_button_down , bool middle_button_down ,
		bool right_button_down ) ;
			///< Overridable. Called on receipt of a mouse-move message.

	enum MouseButton { Mouse_Left , Mouse_Middle , Mouse_Right } ;
	enum MouseButtonDirection { Mouse_Up , Mouse_Down } ;

	virtual void onMouseButton( MouseButton , MouseButtonDirection , 
		int x , int y , bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse 
			///< button-down/button-up message. Called
			///< before the separate functions below.

	virtual void onLeftMouseButtonDown( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse left-button-down
			///< message.

	virtual void onLeftMouseButtonUp( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse left-button-up
			///< message.

	virtual void onMiddleMouseButtonDown( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse middle-button-down
			///< message.

	virtual void onMiddleMouseButtonUp( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse middle-button-up
			///< message.

	virtual void onRightMouseButtonDown( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse right-button-down
			///< message.

	virtual void onRightMouseButtonUp( int x , int y , 
		bool shift_key_down , bool control_key_down ) ;
			///< Overridable. Called on receipt of a mouse right-button-up
			///< message.

	virtual bool onPalette() ;
		///< Called when the window gets focus, allowing
		///< it to realise its own palette into the
		///< system-wide hardware palette. If the
		///< window has a palette it should realise
		///< it and return true. If it has no palette
		///< it should return false.
		///<
		///< See also: WM_QUERYNEWPALETTE

	virtual void onPaletteChange() ;
		///< Called when some other window changes
		///< the system-wide hardware palette.
		///<
		///< If a window has a palette, but it ignores this
		///< message, then the window's colors will be bogus 
		///< while another application has input focus.
		///<
		///< See also: WM_PALETTECHANGED

	virtual bool onIdle() ;
		///< Called whenever the event loop becomes empty.
		///< If true is returned then it is called again
		///< (as long as the queue is still empty).
		///< See also: GGui::Pump

private:
	typedef void (Cracker::*Fn)(int,int,bool,bool) ;
	LRESULT doMouseButton( Fn fn , MouseButton , MouseButtonDirection , unsigned int , WPARAM , LPARAM ) ;
} ;

#endif
