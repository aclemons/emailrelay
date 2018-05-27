//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_GUI_STACK__H
#define G_GUI_STACK__H

#include "gdef.h"
#include "gwinbase.h"
#include <list>
#include <vector>
#include <prsht.h>

namespace GGui
{
	class StackPageCallback ;
	class Stack ;
	class StackImp ;
}

/// \class GGui::StackPageCallback
/// A callback interface for GGui::Stack. Each callback function
/// refers to a property sheet page dialog box by its window
/// handle.
///
class GGui::StackPageCallback
{
public:
	virtual void onInit( HWND hdialog , const std::string & page_title ) ;
	virtual void onClose( HWND hdialog ) ;
	virtual void onDestroy( HWND hdialog ) ;
	virtual void onNcDestroy( HWND hdialog ) ;
	virtual void onActive( HWND hdialog , int index ) ;
	virtual void onInactive( HWND hdialog , int index ) ;
	virtual ~StackPageCallback() ;
} ;

/// \class GGui::Stack
/// A property sheet class that manages a set of property sheet pages,
/// with a 'close' button and a disabled 'cancel' button. Each property
/// sheet page is a dialog box.
///
class GGui::Stack : public WindowBase
{
public:
	typedef std::vector<HPROPSHEETPAGE> Pages ;

	Stack( StackPageCallback & , HINSTANCE ) ;
		///< Contructor. Initialise with addPage() and create().

	void addPage( const std::string & title , int dialog_id , int icon_id = 0 ) ;
		///< Creates a property sheet page and adds it to
		///< to the internal list.

	void create( HWND parent , const std::string & title , int icon_id ,
		unsigned int notify_message_id = 0U ) ;
			///< Creates the property sheet containing all the added pages.
			///< If the notify message id is non-zero then the parent window
			///< will receive a posted message when the stack completes.

	virtual ~Stack() ;
		///< Destructor.

	static bool stackMessage( MSG & msg ) ;
		///< Passes the message to any matching property sheet. Returns
		///< true if processed. This should only be used in the message
		///< pump, like GGui::Dialog::dialogMessage(), to bypass
		///< DispatchMessage().

private:
	friend class StackImp ;
	static int sheetProc( HWND hwnd , UINT message , LPARAM lparam ) ;
	static unsigned int pageProc( HWND hwnd , UINT message , PROPSHEETPAGE * page ) ;
	static bool dlgProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
	static Stack * from_lparam( LPARAM lparam ) ;
	static LPARAM to_lparam( const Stack * p ) ;
	static void setptr( HWND hwnd , const Stack * This ) ;
	static GGui::Stack * getptr( HWND hwnd ) ;
	static std::string convert( const char * p , const std::string & default_ = std::string() ) ;
	void onCompleteImp( bool ) ;

private:
	Stack( const Stack & ) ;
	void operator=( const Stack & ) ;

private:
	enum { MAGIC = 549386 } ;
	int m_magic ;
	HINSTANCE m_hinstance ;
	StackPageCallback & m_callback ;
	std::vector<HPROPSHEETPAGE> m_pages ;
	static std::list<HWND> m_list ;
	HWND m_notify_hwnd ;
	unsigned int m_notify_message ;
} ;

#endif
