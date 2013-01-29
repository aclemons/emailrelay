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
/// \file winform.h
///

#ifndef WIN_FORM_H
#define WIN_FORM_H

#include "gdef.h"
#include "gstrings.h"
#include "gdialog.h"
#include "gcontrol.h"
#include "configuration.h"
#include <string>

/// \namespace Main
namespace Main
{
	class WinForm ;
	class WinApp ;
}

class Main::WinForm : public GGui::Dialog 
{
public: 
	WinForm( WinApp & , const Configuration & cfg , bool confirm ) ;
		///< Constructor.

	void close() ;
		///< Closes the form.

private:
	virtual bool onInit() ;
	virtual void onNcDestroy() ;
	virtual void onCommand( unsigned int id ) ;
	std::string text() const ;
	static std::string str( const Configuration & , const std::string & line_prefix , const std::string & eol ) ;
	static std::string yn( bool b ) ;
	static std::string na() ;
	static std::string na( const std::string & s ) ;
	static std::string any( const G::Strings & s ) ;

private:
	WinApp & m_app ;
	GGui::EditBox m_edit_box ;
	Configuration m_cfg ;
	bool m_confirm ;
} ;

#endif
