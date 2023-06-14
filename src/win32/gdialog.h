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
/// \file gdialog.h
///

#ifndef G_GUI_DIALOG_H
#define G_GUI_DIALOG_H

#include "gdef.h"
#include "gwinbase.h"
#include "gappbase.h"
#include "gscmap.h"
#include <string>

namespace GGui
{
	class Dialog ;
}

//| \class GGui::Dialog
/// A dialog box class for modal and modeless operation.
/// \see GGui::Control
///
class GGui::Dialog : public WindowBase
{
public:
	Dialog( HINSTANCE hinstance , HWND hwnd_parent , const std::string & title = {} ) ;
		///< Constructor. After contruction just call run() or
		///< runModeless() with the appropriate dialog resource
		///< id or name. The hdialog returned by WindowBase::handle()
		///< will be HNULL until the dialog box is run()ning.

	explicit Dialog( const GGui::ApplicationBase & app , bool top_level = false ) ;
		///< Contructor for a dialog box which takes some
		///< of its attributes (eg. its title) from the main
		///< application window.
		///<
		///< Normally the dialog is a child of the application
		///< window, but if the top-level parameter is set then
		///< the dialog box is given no parent and therefore
		///< appears on the task bar.

	virtual ~Dialog() ;
		///< Virtual destructor. If the dialog box is running it
		///< is left running but in a headless chicken mode.

	static bool dialogMessage( MSG & msg ) ;
		///< Processes messages for all modeless dialog boxes.
		///< This should be put in the application's main message
		///< loop (as the GGui::Pump class does). Returns true
		///< if the message was used up.

	bool run( int resource_id ) ;
		///< Runs the dialog modally; only returns once the user's
		///< dialog interaction has been completed.
		///<
		///< Returns false if the dialog could not be created or
		///< if onInit() returned false.

	bool run( const char * resource_name ) ;
		///< An override taking a resource name rather than a
		///< resource id.

	bool runModeless( int resource_id , bool visible = true ) ;
		///< Runs the dialog modelessly; normally modeless Dialog
		///< objects will be allocated on the heap and deleted
		///< with "delete this" within onNcDestroy().
		///<
		///< Returns false if the dialog could not be created or
		///< if onInit() returned false.

	bool runModeless( const char * resource_name , bool visible = true ) ;
		///< An override taking a resource name rather than a
		///< resource id.

	static INT_PTR dlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
		///< Called directly from the exported dialog procedure.

	void setFocus( int control ) ;
		///< Sets focus to the specified control.

	LRESULT sendMessage( int control , unsigned message ,
		WPARAM wparam = 0 , LPARAM lparam = 0 ) const ;
			///< Sends a message to the specified control.

	SubClassMap & map() ;
		///< Used by GGui::Control. (The sub-class map allows the Control
		///< class to map from a sub-classed control's window handle to
		///< the control object's address and the address of the super-
		///< class window procedure.)

	bool registerNewClass( HICON hicon , const std::string & class_name ) const;
		///< Registers a new window-class based on this
		///< dialog box's window-class, but with the specified
		///< icon. (See "Custom Dialog Boxes" in MSDN.)
		///< Use after runModeless() and before end().
		///< Returns false on error.

	void end() ;
		///< Starts the dialog box termination sequence.
		///< Usually called from the override of onClose() or
		///< onCommand().

	bool isValid() ;
		///< Returns true if the object passes its internal
		///< consistency checks. Used in debugging.

protected:
	virtual bool onInit() ;
		///< Overridable. Called on receipt of a WM_INITDIALOG
		///< message. Returns false to abort the dialog
		///< box creation.

	virtual void onCommand( UINT id ) ;
		///< Overridable. Called on receipt of a WM_COMMAND
		///< message. The id is typically IDOK or IDCANCEL.

	virtual HBRUSH onControlColour( HDC hDC , HWND hwnd_control , WORD type ) ;
		///< Overridable. Called on receipt of a WM_CTLCOLOR
		///< message.

	virtual void onClose() ;
		///< Overridable. Called on receipt of a WM_CLOSE
		///< message.

	virtual void onScrollPosition( HWND hwnd_scrollbar , unsigned position ) ;
		///< Overridable. Called on receipt of thumb-track and
		///< thumb-position messages.

	virtual void onScroll( HWND hwnd_scrollbar , bool vertical ) ;
		///< Overridable. Called on receipt of scroll messages excluding
		///< thumb-track and thumb-position messages.

	virtual void onScrollMessage( unsigned message ,
		WPARAM wparam , LPARAM lparam ) ;
		///< Overridable. Called on receipt of all scroll messages.
		///< This combines onScroll() and onScrollPosition().

	virtual void onDestroy() ;
		///< Overridable. Called on receipt of a WM_DESTROY
		///< message.

	virtual void onNcDestroy() ;
		///< Overridable. Called on receipt of a WM_NCDESTROY
		///< message. The override may do a "delete this" if
		///< necessary.

public:
	Dialog( const Dialog & ) = delete ;
	Dialog( Dialog && ) = delete ;
	Dialog & operator=( const Dialog & ) = delete ;
	Dialog & operator=( Dialog && ) = delete ;

private:
	using DialogList = std::list<HWND> ;
	INT_PTR dlgProcImp( UINT message , WPARAM wparam , LPARAM lparam ) ;
	void privateInit( HWND hwnd ) ;
	void privateEnd( int n ) ;
	bool privateFocusSet() const ;
	void cleanup() ;
	DialogList::iterator find( HWND h ) ;
	INT_PTR onControlColour_( WPARAM wparam , LPARAM lparam , WORD type ) ;
	bool runStart() ;
	bool runCore( const char * ) ;
	bool runCore( const wchar_t * ) ;
	bool runEnd( int ) ;
	bool runModelessCore( const char * , bool ) ;
	bool runModelessCore( const wchar_t * , bool ) ;
	bool runModelessEnd( HWND , bool ) ;
	static LPARAM toLongParam( Dialog * p ) ;
	static LONG_PTR toLongPtr( Dialog * p ) ;
	static Dialog * fromLongParam( LPARAM ) ;
	static Dialog * fromLongPtr( LONG_PTR ) ;
	template <typename T> static DLGPROC toDlgProc( T ) ;

private:
	static constexpr int Magic = 4567 ;
	std::string m_title ;
	bool m_modal ;
	bool m_focus_set ;
	HINSTANCE m_hinstance ;
	HWND m_hwnd_parent ;
	int m_magic ;
	SubClassMap m_map ;
	static DialogList m_list ;
} ;

namespace GGui
{
	template <typename T>
	DLGPROC Dialog::toDlgProc( T export_fn )
	{
		return reinterpret_cast<DLGPROC>( export_fn ) ;
	}
}

#endif
