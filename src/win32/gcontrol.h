//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_CONTROL_H
#define G_CONTROL_H

#include "gdef.h"
#include "gdialog.h"
#include "gstrings.h"
#include <string>
#include <list>

/// \namespace GGui
namespace GGui
{
	class Control ;
	class ListBox ;
	class EditBox ;
	class CheckBox ;
	class Button ;
}

/// \class GGui::Control
/// A base class for dialog box control objects.
/// Normally a dialog box object (derived from Dialog) will
/// have Control-derived objects embedded within it to
/// represent some of the dialog box controls. Supports
/// sub-classing.
/// \see EditBox, ListBox, CheckBox, Button
///
class GGui::Control 
{
public:
	class NoRedraw
	{
		private: Control & m_control ;
		public: NoRedraw(Control&) ;
		public: ~NoRedraw() ;
		private: void operator=( const NoRedraw & ) ;
		private: NoRedraw( const NoRedraw & ) ;
	} ;
	
	Control( Dialog & dialog , int id ) ;
		///< Constructor. The lifetime of the Control
		///< object should not exceed that of the given
		///< dialog box; normally the control object
		///< should be a member of the dialog object.

	void invalidate() ;
		///< Called by the Dialog class when this
		///< control's dialog box object becomes invalid.
		
	bool valid() const ;
		///< Returns true if this control is usable.

	Dialog & dialog() const ;
		///< Returns a reference to the control's
		///< dialog box.
		
	int id() const ;
		///< Returns the control's identifier.

	HWND handle() const ;
		///< Returns the control's window handle.
				
	virtual ~Control() ;
		///< Virtual destructor.
		
	LRESULT sendMessage( unsigned message , WPARAM wparam = 0 , LPARAM lparam = 0 ) const ;
		///< Sends a window message to this control.
		
	bool subClass() ;
		///< Subclasses the control so that all received messages	
		///< are passed to onMessage().

	LRESULT wndProc( unsigned message , WPARAM wparam , LPARAM lparam , WNDPROC super_class ) ;
		///< Not for general use. Called from the exported
		///< window procedure.
		
protected:	
	virtual LRESULT onMessage( unsigned message , WPARAM wparam , 
		LPARAM lparam , WNDPROC super_class , bool & forward ) ;
			///< Overridable. Called on receipt of a window message
			///< sent to a sub-classed control. 
			///<
			///< If the implementation sets 'forward' true,
			///< or leaves it alone, then the message is
			///< passed on to the super-class handler and
			///< the implementation's return value is ignored.
			///< This means that an empty implementation
			///< does not affect the control's behaviour.
			///<
			///< If the implementation handles the message
			///< fully it should reset 'forward' and return
			///< an appropriate value; if it does something 
			///< but still needs the super-class behaviour 
			///< then it should leave 'forward' alone.
			///<
			///< The 'super_class' parameter is normally only
			///< needed if the super-class handler's
			///< return value needs to be modified or if the
			///< sub-class behaviour must be done after
			///< the super-class behaviour.

private:
	Control( const Control & ) ; // not implemented
	void operator=( const Control & ) ; // not implemented

private:
	friend class Control::NoRedraw ;
	bool m_valid ;
	Dialog & m_dialog ;
	int m_id ;
	HWND m_hwnd ;
	unsigned m_no_redraw_count ;
} ;

/// \class GGui::ListBox
/// A list box class.
///
class GGui::ListBox : public GGui::Control 
{
public:
	ListBox( Dialog & dialog , int id ) ;
		///< Constructor.
		
	virtual ~ListBox() ;
		///< Virtual destructor.
	
	void set( const G::Strings & list ) ;
		///< Puts a list of strings into the
		///< given list box control.
		
	int getSelection() ;
		///< Returns the currently selected item
		///< in a list box. Returns -1 if none.
		
	void setSelection( int index ) ;
		///< Sets the list box selection. For no
		///< selection index should be -1.
		
	std::string getItem( int index ) const ;
		///< Returns the specified item in a
		///< list box.
		
	unsigned entries() const ;
		///< Returns the number of list box entries.

private:
	void operator=( const ListBox & ) ; // not implemented
	ListBox( const ListBox & ) ; // not implemented
} ;

/// \class GGui::EditBox
/// An edit box class.
///
class GGui::EditBox : public GGui::Control 
{
public:
	EditBox( Dialog & dialog , int id ) ;
		///< Constructor.
	
	virtual ~EditBox() ;
		///< Virtual destructor.
	
	void set( const G::Strings &list ) ;
		///< Puts a list of strings into the
		///< given multi-line edit box control.
		///< Does no scrolling.
		
	void set( const std::string & string ) ;
		///< Sets the text of the edit control.

	std::string get() const ;
		///< Gets the text of a single-line edit control.
	
	unsigned lines() ;
		///< Returns the number of lines. This will 
		///< in general be more than the number of
		///< strings passed to set(G::Strings) because of
		///< line wrapping.

	void scrollToEnd() ;
		///< Scrolls forward so that _no_ text is
		///< visible. Normally used before scrollBack().

	void scrollBack( int lines ) ;
		///< Scrolls back 'lines' lines. Does nothing
		///< if 'lines' is zero or negative.

	unsigned linesInWindow() ;
		///< Return the (approximate) number of lines
		///< which will fit inside the edit box window.

	unsigned scrollPosition() ;
		///< Returns a value representing the vertical
		///< scroll position.
		///<
		///< See also SBM_SETPOS.

	unsigned scrollRange() ;
		///< Returns a value representing the vertical
		///< scroll range. This is similar to lines(),
		///< but it is never zero.
		///<
		///< See also SBM_SETRANGE.

private:
	unsigned windowHeight() ; // not const
	unsigned characterHeight() ; // not const
	EditBox( const EditBox & ) ; // not implemented
	void operator=( const EditBox & ) ; // not implemented

private:
	unsigned m_character_height ;
} ;

/// \class GGui::CheckBox
/// A check box class.
///
class GGui::CheckBox : public GGui::Control 
{
public:
	CheckBox( Dialog & dialog , int id ) ;
		///< Constructor.
	
	virtual ~CheckBox() ;
		///< Virtual destructor.
		
	bool get() const ;
		///< Returns the state of a boolean check box.

	void set( bool b ) ;
		///< Sets the state of a boolean check box.

private:
	void operator=( const CheckBox ) ; // not implemented
	CheckBox( const CheckBox & ) ; // not implemented
} ;
	
/// \class GGui::Button
/// A button class.
///
class GGui::Button : public GGui::Control 
{
public:
	Button( Dialog & dialog , int id ) ;
		///< Constructor.
	
	virtual ~Button() ;
		///< Virtual destructor.

	bool enabled() const ;
		///< Returns true if the button is enabled.
				
	void enable( bool b = true ) ;
		///< Enables the button. Disables it if 'b'
		///< is false.
		
	void disable() ;
		///< Disables the button, greying it.

private:
	void operator=( const Button & ) ; // not implemented
	Button( const Button & ) ; // not implemented
} ;
	
#endif
