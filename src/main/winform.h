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
/// \file winform.h
///

#ifndef G_MAIN_WIN_FORM_H
#define G_MAIN_WIN_FORM_H

#include "gdef.h"
#include "gstringarray.h"
#include "gdialog.h"
#include "gstack.h"
#include "gcontrol.h"
#include "goption.h"
#include "gslot.h"
#include "configuration.h"
#include <string>
#include <map>
#include <utility>

namespace Main
{
	class WinForm ;
	class WinApp ;
}

//| \class Main::WinForm
/// A class for the main user interface that is-a property-sheet stack
/// containing property-page dialog boxes. In practice each dialog box
/// is made up of a COMMCTL List View control (see GGui::ListView).
///
class Main::WinForm : public GGui::Stack , private GGui::StackPageCallback
{
public:
	WinForm( HINSTANCE , const G::StringArray & cfg_data ,
		HWND parent , HWND notify , std::pair<DWORD,DWORD> style , bool allow_apply ,
		bool with_icon , bool with_system_menu_quit ) ;
			///< Constructor. The 'cfg_data' parameter should normally
			///< come from Main::Configuration::display().

	void minimise() ;
		///< Minimises the form, but dependent on the ctor window style.

	void restore() ;
		///< Reverses minimise().

	void close() ;
		///< Closes the form and destroys its window.

	bool visible() const ;
		///< Returns true if not close()d and not minimise()d.

	bool closed() const ;
		///< Returns true if close()d. If closed() there is no window and the
		///< WinForm object can be deleted.

	void setStatus( const std::string & , const std::string & , const std::string & , const std::string & ) ;
		///< Updates the 'status' property page. The parameters come
		///< from slot/signal parameters.

private: // overrides
	virtual void onInit( HWND , int ) override ; // Override from StackPageCallback.
	virtual bool onApply() override ; // Override from StackPageCallback.

public:
	~WinForm() = default ;
	WinForm( const WinForm & ) = delete ;
	WinForm( WinForm && ) = delete ;
	WinForm & operator=( const WinForm & ) = delete ;
	WinForm & operator=( WinForm && ) = delete ;

private:
	static void add( G::StringArray & s , const std::string & key , const std::string & value ) ;
	static void add( G::StringArray & list , std::string s ) ;
	static G::StringArray split( const std::string & s ) ;
	G::StringArray versionData() const ;
	G::StringArray statusData() const ;
	void getStatusData( G::StringArray & out ) const ;
	G::StringArray licenceData() const ;
	void addSystemMenuItem( const char * name , unsigned int id ) ;
	static std::string timestamp() ;

private:
	using StatusMap = std::map<std::string,std::pair<std::string,std::string>> ;
	HWND m_hnotify ;
	bool m_allow_apply ;
	bool m_closed ;
	std::unique_ptr<GGui::ListView> m_cfg_view ;
	std::unique_ptr<GGui::ListView> m_status_view ;
	std::unique_ptr<GGui::ListView> m_version_view ;
	std::unique_ptr<GGui::ListView> m_licence_view ;
	G::StringArray m_cfg_data ; // key,value,key,value...
	StatusMap m_status_map ;
} ;

#endif
