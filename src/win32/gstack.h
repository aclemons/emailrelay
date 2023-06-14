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
/// \file gstack.h
///

#ifndef G_GUI_STACK_H
#define G_GUI_STACK_H

#include "gdef.h"
#include "gstr.h"
#include "gwinbase.h"
#include <utility>
#include <list>
#include <vector>
#include <prsht.h>

namespace GGui
{
	class StackPageCallback ;
	class Stack ;
}

//| \class GGui::StackPageCallback
/// A callback interface for GGui::Stack.
///
class GGui::StackPageCallback
{
public:
	virtual void onInit( HWND , int index ) ;
		///< Called as a page is initialised.

	virtual void onActive( int index ) ;
		///< Called as a page is activated.

	virtual void onInactive( int index ) ;
		///< Called as a page is deactivated.

	virtual bool onApply() ;
		///< Called when the 'close' button is pressed (or equivalent)
		///< to complete the dialog. Completion proceeds if this
		///< returns true, eventually resulting in an asynchronous
		///< completion notification with wparam 0 or 1. If this method
		///< returns false then the button has no effect and an
		///< 'apply-denied' notification is posted instead (wparam 2).

	virtual void onSysCommand( unsigned int id ) ;
		///< Called on WM_SYSCOMMAND. A notification message is
		///< also posted with a wparam of 3 and the id in lparam.

	virtual ~StackPageCallback() ;
} ;

//| \class GGui::Stack
/// A property sheet class that manages a set of property sheet pages,
/// with a 'close' button and a disabled 'cancel' button. Each property
/// sheet page is a dialog box.
///
class GGui::Stack : public WindowBase
{
public:
	Stack( StackPageCallback & , HINSTANCE , std::pair<DWORD,DWORD> style , bool set_style = true ) ;
		///< Contructor. Initialise with addPage() and then create().

	void create( HWND hparent , const std::string & title , int icon_id ,
		HWND notify_hwnd , unsigned int notify_message , bool fixed_size = true ) ;
			///< Creates the property sheet containing all the added pages and
			///< hooks into the message pump.
			///<
			///< If the notify parameters are non-zero then the notify window
			///< will receive an asynchronous message after the dialog completes
			///< (wparam 0 or 1), or after any onApply() callback returns false
			///< (wparam 2), or after a user-defined WM_SYSCOMMAND.

	void addPage( const std::string & title , int dialog_id ) ;
		///< Creates a property sheet page and adds it to to the internal
		///< list.

	virtual ~Stack() ;
		///< Destructor.

	static bool stackMessage( MSG & msg ) ;
		///< Used by the GGui::Pump message pump to bypass DispatchMessage() if
		///< a stack message, in the same way as GGui::Dialog::dialogMessage().
		///< Returns true if a stack message.

	static int sheetProc( HWND hwnd , UINT message , LPARAM lparam ) ;
		///< Implementation window procedure.

	static unsigned int pageProc( HWND hwnd , UINT message , PROPSHEETPAGE * page ) ;
		///< Implementation window procedure.

	static bool dlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
		///< Implementation window procedure.

	static LRESULT wndProc( HWND , UINT , WPARAM , LPARAM ) ;
		///< Implementation window procedure.

private:
	static Stack * getObjectPointer( HWND hwnd ) ;
	LRESULT wndProc( UINT , WPARAM , LPARAM , bool & ) ;
	void hook( HWND ) ;
	void unhook() ;
	void doOnApply( HWND ) ;
	void postNotifyMessage( WPARAM , LPARAM = 0 ) ;

public:
	Stack( const Stack & ) = delete ;
	Stack( Stack && ) = delete ;
	Stack & operator=( const Stack & ) = delete ;
	Stack & operator=( Stack && ) = delete ;

private:
	using PageInfo = std::pair<Stack*,int> ;
	static constexpr int MAGIC = 24938 ;
	int m_magic ;
	HINSTANCE m_hinstance ;
	StackPageCallback & m_callback ;
	std::pair<DWORD,DWORD> m_style ;
	bool m_set_style ;
	bool m_fixed_size ;
	std::list<PageInfo> m_pages ;
	std::vector<HPROPSHEETPAGE> m_hpages ;
	HWND m_notify_hwnd ;
	unsigned int m_notify_message ;
	static std::list<HWND> m_list ;
	LONG_PTR m_wndproc_orig ;
} ;

#endif
