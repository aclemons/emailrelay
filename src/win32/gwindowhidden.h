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
/// \file gwindowhidden.h
///

#ifndef G_WINDOW_HIDDEN__H
#define G_WINDOW_HIDDEN__H

#include "gdef.h"
#include "gwindow.h"
#include <string>

namespace GGui
{
	class WindowHidden ;
}

/// \class GGui::WindowHidden
/// A derivation of GGui::Window for a hidden window (without a parent).
///
class GGui::WindowHidden : public Window
{
public:
	WindowHidden( HINSTANCE hinstance , bool do_create ) ;
		///< Constructor. Registers the window class if necessary and
		///< optionally creates the Windows window. Only create the
		///< Windows window from within the constructor if default
		///< handling of the window messages is okay, because the
		///< derived class's vtable will not be complete.

	virtual ~WindowHidden() ;
		///< Virtual destructor. The Windows window is destroyed
		///< (unlike for the base class).

	void createHiddenWindow() ;
		///< Creates the Windows window. Typically called from the
		///< most-derived class's constructor.

private:
	WindowHidden( const WindowHidden & ) ; // not implemented
	void operator=( const WindowHidden & ) ; // not implemented
	virtual void onNcDestroy() ;
	std::string windowClassName() ;

private:
	static bool m_registered ;
	bool m_created ;
	bool m_destroyed ;
	HINSTANCE m_hinstance ;
	std::string m_window_class_name ;
} ;

#endif
