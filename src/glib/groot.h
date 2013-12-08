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
/// \file groot.h
///

#ifndef G_ROOT_H
#define G_ROOT_H

#include "gdef.h"
#include "gidentity.h"
#include "gnoncopyable.h"

/// \namespace G
namespace G
{
	class Root ;
}

/// \class G::Root
/// A class which acquires the process's
/// special privileges on construction and releases
/// them on destruction. Despite the name of the class
/// the special privileges are not necessarily root
/// privileges.
///
/// If instances are nested then the inner instances
/// have no effect.
///
/// The implementation uses G::Process and G::Identity.
///
/// The class must be initialised by calling a static
/// init() method.
///
class G::Root : private G::noncopyable 
{
public:
	explicit Root( bool change_group = true ) ;
		///< Constructor. Acquires special
		///< privileges if possible.

	~Root() ;
		///< Desctructor. Releases special privileges
		///< if this instance acquired them.

	static void init( const std::string & nobody ) ;
		///< Initialises this class on process start-up by
		///< releasing root or suid privileges.
		///<
		///< The parameter gives a non-privileged username 
		///< which is used if the real user-id is root.

	static Identity nobody() ;
		///< Returns the 'nobody' identity.
		///< Precondition: init() called

	static Identity start( SignalSafe ) ;
		///< A signal-safe alternative to construction.

	static void stop( SignalSafe , Identity ) ;
		///< A signal-safe alternative to destruction.

private:
	static Root * m_this ;
	static bool m_initialised ;
	static Identity m_special ;
	static Identity m_nobody ;
	bool m_change_group ;
} ;

#endif

