//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsetter.h
//

#ifndef G_SETTER_H
#define G_SETTER_H

#include "gdef.h"
#include "gnoncopyable.h"

namespace G
{
	class Setter ;
}

// Class: G::Setter
// Description: A class to manage a boolean flag 
// while in scope.
//
class G::Setter : public G::noncopyable 
{
public: 
	explicit Setter( bool & b ) ;
		// Constructor. Sets the flag.

	~Setter() ;
		// Destructor. Resets the flag.

private:
	bool & m_b ;
} ;

inline
G::Setter::Setter( bool & b ) : 
	m_b(b) 
{ 
	m_b = true ; 
}

inline
G::Setter::~Setter() 
{ 
	m_b = false ; 
}

#endif

