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
/// \file gappinst.h
///

#ifndef G_GUI_APPINST_H
#define G_GUI_APPINST_H

#include "gdef.h"

namespace GGui
{
	class ApplicationInstance ;
}

//| \class GGui::ApplicationInstance
/// A class for storing the application's instance handle, as
/// obtained from WinMain().
///
/// Other low-level classes in this library use this interface to
/// obtain the application instance handle, rather than some
/// higher-level mechanism.
///
/// Programs that need a message pump, but want to avoid the
/// overhead of the full GUI application framework must, as an
/// absolute minimum, use this class to set the application
/// instance handle.
///
/// \see GGui::ApplicationBase
///
class GGui::ApplicationInstance
{
protected:
	explicit ApplicationInstance( HINSTANCE h ) ;
		///< Protected constructor that calls
		///< hinstance(h).

	~ApplicationInstance() ;
		///< Destructor.

public:
	static void hinstance( HINSTANCE h ) ;
		///< Sets the instance handle, which is
		///< subsequently returned by hinstance().

	static HINSTANCE hinstance() ;
		///< Returns the instance handle that was
		///< passed to the constructor. Returns
		///< zero if hinstance(h) has never been
		///< called.

private:
	static HINSTANCE m_hinstance ;
} ;

#endif
