//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// winform.h
//

#ifndef WIN_FORM_H
#define WIN_FORM_H

#include "gdef.h"
#include "gdialog.h"
#include "gcontrol.h"
#include "configuration.h"
#include <string>

namespace Main
{
	class WinForm ;
	class WinApp ;
}

class Main::WinForm : public GGui::Dialog 
{
public: 
	WinForm( WinApp & , const Configuration & cfg , bool confirm ) ;
		// Constructor.

	void close() ;
		// Closes the form.

private:
	virtual bool onInit() ;
	virtual void onNcDestroy() ;
	virtual void onCommand( unsigned int id ) ;
	std::string text() const ;

private:
	WinApp & m_app ;
	GGui::EditBox m_edit_box ;
	Configuration m_cfg ;
	bool m_confirm ;
} ;

#endif
