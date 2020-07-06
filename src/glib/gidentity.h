//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

namespace G
{
	class Identity ;
}

/// \class G::Identity
/// A combination of user-id and group-id, with a very low-level interface
/// to the get/set/e/uid/gid functions. Uses getpwnam() to do username
/// lookups.
/// \see G::Process, G::Root
///
class G::Identity
{
public:
	struct NoThrow /// Overload discriminator for G::Identity.
		{} ;
	G_EXCEPTION( NoSuchUser , "no such user" ) ;
	G_EXCEPTION( NoSuchGroup , "no such group" ) ;
	G_EXCEPTION( Error , "cannot read user database" ) ;
	G_EXCEPTION( UidError , "cannot set uid" ) ;
	G_EXCEPTION( GidError , "cannot set gid" ) ;

	explicit Identity( const std::string & login_name ,
		const std::string & group_name_override = std::string() ) ;
			///< Constructor for the named identity.
			///< Throws NoSuchUser on error.

	static Identity effective() noexcept ;
		///< Returns the current effective identity.

	static Identity real() noexcept ;
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

	void setRealUser() const ;
		///< Sets the real userid. Throws on error.

	bool setRealUser( NoThrow ) const noexcept ;
		///< Sets the real userid.

	void setEffectiveUser() const ;
		///< Sets the effective userid. Throws on error.

	bool setEffectiveUser( NoThrow ) const noexcept ;
		///< Sets the effective userid.

	bool setEffectiveUser( SignalSafe ) const noexcept ;
		///< Sets the effective userid.
		///< A signal-safe, reentrant overload.

	void setRealGroup() const ;
		///< Sets the real group id. Throws on error.

	bool setRealGroup( NoThrow ) const noexcept ;
		///< Sets the real group id.

	void setEffectiveGroup() const ;
		///< Sets the effective group id. Throws on error.

	bool setEffectiveGroup( NoThrow ) const noexcept ;
		///< Sets the effective group id.

	bool setEffectiveGroup( SignalSafe ) const noexcept ;
		///< Sets the effective group id.
		///< A signal-safe, reentrant overload.

	bool operator==( const Identity & ) const noexcept ;
		///< Comparison operator.

	bool operator!=( const Identity & ) const noexcept ;
		///< Comparison operator.

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
