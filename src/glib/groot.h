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
// groot.h
//

#ifndef G_ROOT_H
#define G_ROOT_H

#include "gdef.h"
#include "gidentity.h"
#include "gnoncopyable.h"

namespace G
{
	class Root ;
}

// Class: G::Root
// Description: A class which aquires special privileges.
// The implementation uses G::Process and G::Identity.
//
class G::Root : private G::noncopyable 
{
public:
	explicit Root( bool change_group = true ) ;
		// Constructor. Aquires special
		// privileges if possible.

	~Root() ;
		// Desctructor. Releases special privileges
		// if this instance aquired them.

	static void init( const std::string & nobody ) ;
		// Releases root or suid privileges. Used
		// at process start-up. The parameter
		// gives a non-privileged username which
		// is used if the real user-id is root.

	static Identity nobody() ;
		// Returns the 'nobody' identity.
		// Precondition: init() called

private:
	static Root * m_this ;
	static bool m_initialised ;
	static Identity m_special ;
	static Identity m_nobody ;
	bool m_change_group ;
} ;

#endif

