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
/// \file gcontrol.h
///

#ifndef G_GUI_CONTROL_H
#define G_GUI_CONTROL_H

#include "gdef.h"
#include "gdialog.h"
#include "gstringarray.h"
#include <string>
#include <list>
#include <vector>
#include <new>

namespace GGui
{
	class Control ;
	class ListBox ;
	class ListView ;
	class EditBox ;
	class CheckBox ;
	class Button ;
}

//| \class GGui::Control
/// A base class for dialog box control objects. Normally a dialog
/// box object (derived from Dialog) will have Control-derived
/// objects embedded within it to represent some of the dialog
/// box controls and to manage their contents.
///
/// Windows dialog boxes have controls as child windows and the
/// dialog box hides nearly all the controls' window messages.
/// The dialog box's dialog procedure gets residual messages,
/// including notifications for individual controls via WM_COMMAND
/// and WM_NOTIFY. However, the Dialog class does not currently
/// expose controls' WM_COMMAND or WM_NOTIFY messages, so this
/// Control interface only allows getting and setting of control
/// contents, without any callbacks. On the other hand, Windows
/// sub-classing _is_ supported so all control window messages
/// can be delivered to Control::onMessage().
///
/// \see EditBox, ListBox, CheckBox, Button, ListView
///
class GGui::Control
{
public:
	class NoRedraw
	{
		private: Control & m_control ;
		public: explicit NoRedraw( Control & ) ;
		public: ~NoRedraw() ;
		public: NoRedraw & operator=( const NoRedraw & ) = delete ;
		public: NoRedraw & operator=( NoRedraw && ) = delete ;
		public: NoRedraw( const NoRedraw & ) = delete ;
		public: NoRedraw( NoRedraw && ) = delete ;
	} ;

	Control( const Dialog & dialog , int id ) ;
		///< Constructor. The lifetime of the Control object should not
		///< exceed that of the given dialog box; normally the control
		///< object should be a member of the dialog object. The
		///< control will not generally be usable until its containing
		///< dialog is running (with a valid Dialog::handle()).

	Control( HWND hdialog , int id , HWND hcontrol ) ;
		///< Constructor overload for property sheets. (The hcontrol parameter
		///< is returned by Control::handle().)

	void invalidate() ;
		///< Called by the Dialog class when this control's dialog
		///< box object becomes invalid.

	bool valid() const ;
		///< Returns true if this control is usable.

	HWND hdialog() const noexcept ;
		///< Returns the dialog box's window handle.

	int id() const ;
		///< Returns the control's identifier.

	HWND handle() const ;
		///< Returns the control's window handle.

	HWND handle( std::nothrow_t ) const noexcept ;
		///< Non-chacheing, noexcept overload.

	virtual ~Control() ;
		///< Destructor.

	LRESULT sendMessage( unsigned message , WPARAM wparam = 0 , LPARAM lparam = 0 ) const ;
		///< Sends a window message to this control.

	LRESULT sendMessageString( unsigned message , WPARAM wparam , const std::string & s ) const ;
		///< Sends a window message to this control with a string-like lparam.

	std::string sendMessageGetString( unsigned int message , WPARAM wparam ) const ;
		///< Sends a window message to this control with a string returned via lparam.

	void subClass( SubClassMap & ) ;
		///< Subclasses the control so that all received messages
		///< are passed to onMessage(). The parameter should normally
		///< come from Dialog::map().

	LRESULT wndProc( unsigned message , WPARAM wparam , LPARAM lparam , WNDPROC super_class ) ;
		///< Not for general use. Called from the exported window
		///< procedure.

protected:
	virtual LRESULT onMessage( unsigned message , WPARAM wparam ,
		LPARAM lparam , WNDPROC super_class , bool & forward ) ;
			///< Overridable. Called on receipt of a window message sent to
			///< a sub-classed control.
			///<
			///< If the implementation sets 'forward' true, or leaves it
			///< alone, then the message is passed on to the super-class
			///< handler and the implementation's return value is ignored.
			///< This means that an empty implementation does not affect
			///< the control's behaviour.
			///<
			///< If the implementation handles the message fully it should
			///< reset 'forward' and return an appropriate value; if it
			///< does something but still needs the super-class behaviour
			///< then it should leave 'forward' alone.
			///<
			///< The 'super_class' parameter is normally only needed if
			///< the super-class handler's return value needs to be modified
			///< or if the sub-class behaviour must be done after the
			///< super-class behaviour.

	static void load( DWORD ) ;
		///< Loads common-control library code for the given control types.

public:
	Control( const Control & ) = delete ;
	Control( Control && ) = delete ;
	Control & operator=( const Control & ) = delete ;
	Control & operator=( Control && ) = delete ;

private:
	static void load() ;

private:
	friend class Control::NoRedraw ;
	const Dialog * m_dialog ;
	HWND m_hdialog ;
	bool m_valid ;
	int m_id ;
	mutable HWND m_hwnd ; // control
	unsigned m_no_redraw_count ;
} ;

//| \class GGui::ListBox
/// A list box class.
///
class GGui::ListBox : public Control
{
public:
	ListBox( Dialog & dialog , int id ) ;
		///< Constructor.

	virtual ~ListBox() ;
		///< Destructor.

	void set( const G::StringArray & list ) ;
		///< Puts a list of strings into the list box control.

	int getSelection() ;
		///< Returns the currently selected item. Returns -1
		///< if none.

	void setSelection( int index ) ;
		///< Sets the list box selection. For no selection the
		///< index should be -1.

	std::string getItem( int index ) const ;
		///< Returns the specified item in the list box.

	unsigned entries() const ;
		///< Returns the number of list box entries.

public:
	ListBox( const ListBox & ) = delete ;
	ListBox( ListBox && ) = delete ;
	ListBox & operator=( const ListBox & ) = delete ;
	ListBox & operator=( ListBox && ) = delete ;
} ;

//| \class GGui::ListView
/// A simple write-only list-view class. The list-view resource
/// is expected to use "report" mode so that a multi-column list
/// box is displayed.
///
class GGui::ListView : public Control
{
public:
	ListView( Dialog & dialog , int id ) ;
		///< Constructor.

	ListView( HWND hdialog , int id , HWND hcontrol = HNULL ) ;
		///< Constructor overload for property sheets.

	virtual ~ListView() ;
		///< Destructor.

	void set( const G::StringArray & list , unsigned int columns = 1U , unsigned int width_px = 0U ) ;
		///< Puts a set of strings into the list view control.
		///< The first 'columns' strings are the column headers,
		///< and the following strings fill the grid.

	void update( const G::StringArray & list , unsigned int columns = 1U ) ;
		///< Updates the grid contents, ignoring the column headers
		///< that start the list.

public:
	ListView( const ListView & ) = delete ;
	ListView( ListView && ) = delete ;
	ListView & operator=( const ListView & ) = delete ;
	ListView & operator=( ListView && ) = delete ;

private:
	static LPTSTR ptext( const std::string & s ) ;
} ;

//| \class GGui::EditBox
/// An edit box class.
///
class GGui::EditBox : public Control
{
public:
	EditBox( Dialog & dialog , int id ) ;
		///< Constructor.

	virtual ~EditBox() ;
		///< Destructor.

	void set( const G::StringArray & list ) ;
		///< Puts a list of strings into the multi-line edit
		///< box control. Does no scrolling.

	void set( const std::string & string ) ;
		///< Sets the text of the edit control.

	std::string get() const ;
		///< Gets the text of a single-line edit control.

	unsigned lines() ;
		///< Returns the number of lines. This will in general
		///< be more than the number of strings passed to set()
		///< because of line wrapping.

	void scrollToEnd() ;
		///< Scrolls forward so that _no_ text is visible.
		///< Normally used before scrollBack().

	void scrollBack( int lines ) ;
		///< Scrolls back 'lines' lines. Does nothing if
		///< 'lines' is zero or negative.

	unsigned linesInWindow() ;
		///< Return the (approximate) number of lines that will
		///< fit inside the edit box window.

	unsigned scrollPosition() ;
		///< Returns a value representing the vertical scroll
		///< position.
		///<
		///< See also SBM_SETPOS.

	unsigned scrollRange() ;
		///< Returns a value representing the vertical scroll
		///< range. This is similar to lines(), but it is never
		///< zero.
		///<
		///< See also SBM_SETRANGE.

	void setTabStops( const std::vector<int> & ) ;
		///< Sets tab stop positions, measured in "dialog box
		///< template units" (the default being 32).

public:
	EditBox( const EditBox & ) = delete ;
	EditBox( EditBox && ) = delete ;
	EditBox & operator=( const EditBox & ) = delete ;
	EditBox & operator=( EditBox && ) = delete ;

private:
	unsigned windowHeight() ; // not const
	unsigned characterHeight() ; // not const

private:
	unsigned m_character_height ;
} ;

//| \class GGui::CheckBox
/// A check box class.
///
class GGui::CheckBox : public Control
{
public:
	CheckBox( Dialog & dialog , int id ) ;
		///< Constructor.

	virtual ~CheckBox() ;
		///< Destructor.

	bool get() const ;
		///< Returns the state of a boolean check box.

	void set( bool b ) ;
		///< Sets the state of a boolean check box.

public:
	CheckBox( const CheckBox & ) = delete ;
	CheckBox( CheckBox && ) = delete ;
	CheckBox & operator=( const CheckBox & ) = delete ;
	CheckBox & operator=( CheckBox && ) = delete ;
} ;

//| \class GGui::Button
/// A button class.
///
class GGui::Button : public Control
{
public:
	Button( Dialog & dialog , int id ) ;
		///< Constructor.

	virtual ~Button() ;
		///< Destructor.

	bool enabled() const ;
		///< Returns true if the button is enabled.

	void enable( bool b = true ) ;
		///< Enables the button. Disables it if 'b'
		///< is false.

	void disable() ;
		///< Disables the button, greying it.

public:
	Button( const Button & ) = delete ;
	Button( Button && ) = delete ;
	Button & operator=( const Button & ) = delete ;
	Button & operator=( Button && ) = delete ;
} ;

#endif
