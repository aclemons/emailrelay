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
/// \file groot.h
///

#ifndef G_ROOT_H
#define G_ROOT_H

#include "gdef.h"
#include "gidentity.h"

namespace G
{
	class Root ;
}

/// \class G::Root
/// A class which acquires the process's special privileges on construction
/// and releases them on destruction. Despite the name of the class the special
/// privileges are not necessarily root privileges; they can be suid privileges.
///
/// The class must be initialised by calling a static init() method. If instances
/// are nested then the inner instances have no effect.
///
/// The effect of this class depends on whether the process's real-id is root
/// or not. If the real-id is root then the effective-id is switched to
/// some named 'ordinary' user's id at startup, and then back to what it
/// was (ie. root or the suid id) for the critical sections.  Otherwise,
/// the effective-id is switched to the real-id at startup and switched back
/// to what it was for the critical sections.
///
/// The implementation uses G::Process and G::Identity.
///
class G::Root
{
public:
	Root() ;
		///< Default constructor. Acquires special privileges by switching the user-id
		///< and possibly the group-id (see init()).
		///<
		///< Does nothing if the class has not been initialised by a call to init().
		///< Does nothing if there is another instance at an outer scope.
		///<
		///< The implementation uses G::Process::beSpecial().

	explicit Root( bool change_group ) ;
		///< Constructor overload with explicit control over whether to change the
		///< group-id or not.

	~Root() ;
		///< Desctructor. Releases special privileges if this instance acquired them.
		///<
		///< The implementation uses G::Process::beOrdinary().

	static void init( const std::string & non_root , bool default_change_group = true ) ;
		///< Initialises this class on process start-up by releasing root (or suid)
		///< privileges.
		///<
		///< The string parameter gives a non-privileged username which is used if the
		///< real user-id is root.
		///<
		///< The group-id behaviour of the default constructor is modified by the
		///< boolean parameter.

	static Identity nobody() ;
		///< Returns the 'nobody' identity corresponding to the init() user name.
		///< Precondition: init() called

	static Identity start( SignalSafe ) ;
		///< A signal-safe alternative to construction.

	static void stop( SignalSafe , Identity ) ;
		///< A signal-safe alternative to destruction.

private:
	Root( const Root & ) ;
	void operator=( const Root & ) ;

private:
	static Root * m_this ;
	static bool m_initialised ;
	static bool m_default_change_group ;
	static Identity m_special ;
	static Identity m_ordinary ;
	bool m_change_group ;
} ;

#endif
