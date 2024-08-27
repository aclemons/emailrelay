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
/// \file gscmap.h
///

#ifndef G_GUI_SCMAP_H
#define G_GUI_SCMAP_H

#include "gdef.h"
#include <vector>

namespace GGui
{
	class SubClassMap ;
}

//| \class GGui::SubClassMap
/// A class for mapping sub-classed window handles to their old
/// window procedures. Note that a sub-class map is only required
/// for standard windows such as standard controls or standard
/// dialog boxes; when subclassing our own windows it is better
/// to store the old window procedure function pointer using
/// SetWindowLong().
///
class GGui::SubClassMap
{
public:
	using Proc = WNDPROC ; // see CallWindowProc

	SubClassMap() ;
		///< Constructor.

	void add( HWND hwnd , Proc proc , void * context = nullptr ) ;
		///< Adds the given entry to the map.

	Proc find( HWND hwnd , void ** context_p = nullptr ) ;
		///< Finds the entry in the map whith the given
		///< window handle. Optionally returns the context
		///< pointer by reference.

	void remove( HWND hwnd ) ;
		///< Removes the given entry from the map. Typically
		///< called when processing a WM_NCDESTROY message.

public:
	SubClassMap( const SubClassMap & ) = delete ;
	SubClassMap( SubClassMap && ) = delete ;
	SubClassMap & operator=( const SubClassMap & ) = delete ;
	SubClassMap & operator=( SubClassMap && ) = delete ;

private:
	struct Slot
	{
		Proc proc {0} ;
		HWND hwnd {0} ;
		void * context {nullptr} ;
		Slot() = default ;
		Slot( Proc proc_in , HWND hwnd_in , void * context_in ) : proc(proc_in) , hwnd(hwnd_in) , context(context_in) {}
	} ;
	std::vector<Slot> m_list ;
} ;

#endif
