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
/// \file gwinbase.h
///

#ifndef G_WINBASE_H
#define G_WINBASE_H

#include "gdef.h"
#include "gsize.h"

/// \namespace GGui
namespace GGui
{
	class WindowBase ;
}

/// \class GGui::WindowBase
/// A low-level window class which
/// encapsulates a window handle and provides methods
/// to retrieve basic window attributes. Knows
/// nothing about window messages.
/// \see GGui::Cracker, GGui::Window, GGui::Dialog
///
class GGui::WindowBase 
{

public:
	explicit WindowBase( HWND hwnd ) ;
		///< Constructor.

	virtual ~WindowBase() ;
		///< Virtual destructor.

	HWND handle() const ;
		///< Returns the window handle.

	WindowBase &operator=( const WindowBase &other ) ;
		///< Assignment operator.

	WindowBase( const WindowBase &other ) ;
		///< Copy constructor.

	Size externalSize() const ;
		///< Returns the external size of the window.

	Size internalSize() const ;
		///< Returns the internal size of the window.
		///< (ie. the size of the client area) 

	std::string windowClass() const ;
		///< Returns the window's window-class name.

	HINSTANCE windowInstanceHandle() const ;
		///< Returns the window's application instance.
		///< See also: GGui::ApplicationInstance

protected:
	void setHandle( HWND hwnd ) ;
		///< Sets the window handle.

protected:
	HWND m_hwnd ;
} ;

#endif
