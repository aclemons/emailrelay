//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gidentity.h
///

#ifndef G_IDENTITY_H
#define G_IDENTITY_H

#include "gdef.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include <string>
#include <iostream>
#include <new>

namespace G
{
	class Identity ;
}

//| \class G::Identity
/// A combination of user-id and group-id, with a very low-level interface
/// to the get/set/e/uid/gid functions. Uses getpwnam() to do username
/// lookups.
/// \see G::Process, G::Root
///
class G::Identity
{
public:
	G_EXCEPTION( NoSuchUser , tx("no such user") ) ;
	G_EXCEPTION( NoSuchGroup , tx("no such group") ) ;
	G_EXCEPTION( Error , tx("cannot read user database") ) ;

	explicit Identity( const std::string & username ,
		const std::string & group_name_override = {} ) ;
			///< Constructor for the named identity.
			///< Throws NoSuchUser on error.

	static Identity effective() noexcept ;
		///< Returns the current effective identity.

	static Identity real( bool with_cache = true ) noexcept ;
		///< Returns the calling process's real identity.

	static Identity root() noexcept ;
		///< Returns the superuser identity.

	static Identity invalid() noexcept ;
		///< Returns an invalid identity.

	static Identity invalid( SignalSafe ) noexcept ;
		///< Returns an invalid identity, with a
		///< signal-safe guarantee.

	bool isRoot() const noexcept ;
		///< Returns true if the userid is zero.

	std::string str() const ;
		///< Returns a string representation.

	uid_t userid() const noexcept ;
		///< Returns the user part.

	gid_t groupid() const noexcept ;
		///< Returns the group part.

	bool operator==( const Identity & ) const noexcept ;
		///< Comparison operator.

	bool operator!=( const Identity & ) const noexcept ;
		///< Comparison operator.

	static std::pair<uid_t,gid_t> lookupUser( const std::string & user ) ;
		///< Does a username lookup. Throws on error.

	static gid_t lookupGroup( const std::string & group ) ;
		///< Does a groupname lookup. Throws on error.

private:
	Identity() noexcept ;
	explicit Identity( SignalSafe ) noexcept ;

private:
	uid_t m_uid ;
	gid_t m_gid ;
	HANDLE m_h{0} ; // windows
} ;

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const Identity & identity )
	{
		return stream << identity.str() ;
	}
}

#endif
