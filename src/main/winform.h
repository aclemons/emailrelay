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
/// \file winform.h
///

#ifndef WIN_FORM_H
#define WIN_FORM_H

#include "gdef.h"
#include "gstrings.h"
#include "gdialog.h"
#include "gstack.h"
#include "gcontrol.h"
#include "configuration.h"
#include <string>
#include <map>
#include <utility>

namespace Main
{
	class WinForm ;
	class WinApp ;
}

/// \class Main::WinForm
/// A class for the main user interface that is-a property-sheet stack
/// containing property-page dialog boxes. In practice each dialog box
/// is made up of a COMMCTL List View control (see GGui::ListView).
///
/// The WinForm manages all the the GGui::Stack callbacks, and also
/// asks GGui::Stack to post a user-other message to the WinApp when
/// it is complete.
///
class Main::WinForm : public GGui::Stack , private GGui::StackPageCallback
{
public:
	WinForm( WinApp & , const Configuration & cfg , bool confirm ) ;
		///< Constructor.

	void close() ;
		///< Closes the form and destroys its window.

	bool closed() const ;
		///< Returns true if close()d. If closed() there is no window and the
		///< WinForm object can be deleted.

	void setStatus( const std::string & , const std::string & , const std::string & ) ;
		///< Updates the 'status' property page. The parameters come
		///< from slot/signal parameters.

private:
	virtual void onInit( HWND , const std::string & ) override ; // StackPageCallback
	virtual void onDestroy( HWND ) override ; // StackPageCallback
	static void add( G::StringArray & s , const std::string & key , const std::string & value ) ;
	static void add( G::StringArray & list , std::string s ) ;
	static G::StringArray split( const std::string & s ) ;
	G::StringArray versionData() const ;
	G::StringArray cfgData() const ;
	G::StringArray statusData() const ;
	void getStatusData( G::StringArray & out ) const ;
	G::StringArray licenceData() const ;

private:
	typedef std::map<std::string,std::pair<std::string,std::string> > StatusMap ;
	WinApp & m_app ;
	bool m_closed ;
	unique_ptr<GGui::ListView> m_cfg_view ;
	unique_ptr<GGui::ListView> m_status_view ;
	unique_ptr<GGui::ListView> m_version_view ;
	unique_ptr<GGui::ListView> m_licence_view ;
	Configuration m_cfg ;
	StatusMap m_status_map ;
} ;

#endif
