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
/// \file gwinhid.h
///

#ifndef G_WINHID_H
#define G_WINHID_H

#include "gdef.h"
#include "gwindow.h"
#include <string>

/// \namespace GGui
namespace GGui
{
	class WindowHidden ;
}

/// \class GGui::WindowHidden
/// A derivation of GGui::Window for
/// a hidden window (without a parent).
///
class GGui::WindowHidden : public GGui::Window 
{
public:
	explicit WindowHidden( HINSTANCE hinstance ) ;
		///< Default constructor. Registers the window
		///< class if necessary and creates the window.

	virtual ~WindowHidden() ;
		///< Virtual destructor. The Windows window
		///< is destroyed (unlike for the base class).

private:
	virtual void onNcDestroy() ;
	std::string windowClassName() ;
	WindowHidden( const WindowHidden & ) ; // not implemented
	void operator=( const WindowHidden & ) ; // not implemented

private:
	bool m_destroyed ;
	static bool m_registered ;
} ;

#endif
