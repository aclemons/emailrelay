//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsize.h
//

#ifndef G_SIZE_H
#define G_SIZE_H

#include "gdef.h"
#include <iostream>

namespace GGui
{
	class Size ;
}

// Class: GGui::Size
// Description: A structure representing the size of a rectangle (typically
// a GUI window).
//
class GGui::Size 
{
public:
	unsigned long dx ;
	unsigned long dy ;
	Size() ;
	Size( unsigned long dx , unsigned long dy ) ;
	void streamOut( std::ostream & ) const ;
} ;

inline 
GGui::Size::Size() :
	dx(0) ,
	dy(0)
{
}

inline 
GGui::Size::Size( unsigned long dx_ , unsigned long dy_ ) :
	dx(dx_) ,
	dy(dy_)
{
}

inline
void GGui::Size::streamOut( std::ostream & s ) const
{
	s << "(" << dx << "," << dy << ")" ;
}

namespace GGui
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Size & size )
	{
		size.streamOut( stream ) ;
		return stream ;
	}
}

#endif
