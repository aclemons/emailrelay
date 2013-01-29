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
/// \file gidentity.h
///

#ifndef G_IDENTITY_H
#define G_IDENTITY_H

#include "gdef.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include <string>
#include <iostream>

/// \namespace G
namespace G
{
	class Identity ;
	class IdentityUser ;
}

/// \class G::Identity
/// A very low-level interface to getpwnam() and the get/set/e/uid/gid functions.
/// \see G::Process, G::Root
///
class G::Identity  
{
public:
	G_EXCEPTION( NoSuchUser , "no such user" ) ;
	G_EXCEPTION( UidError , "cannot set uid" ) ;
	G_EXCEPTION( GidError , "cannot set gid" ) ;

	explicit Identity( const std::string & login_name ) ;
		///< Constructor for the named identity.
		///< Throws if NoSuchUser.

	static Identity effective() ;
		///< Returns the current effective identity.

	static Identity real() ;
		///< Returns the calling process's real identity.

	static Identity root() ;
		///< Returns the superuser identity.

	static Identity invalid() ;
		///< Returns an invalid identity.

	bool isRoot() const ;
		///< Returns true if the userid is zero.

	std::string str() const ;
		///< Returns a string representation.

	void setRealUser( bool do_throw = true ) ;
		///< Sets the real userid.

	void setEffectiveUser( bool do_throw = true ) ;
		///< Sets the effective userid.

	void setEffectiveUser( SignalSafe ) ;
		///< Sets the effective userid.
		///< A signal-safe, reentrant overload.

	void setRealGroup( bool do_throw = true ) ;
		///< Sets the real group id.

	void setEffectiveGroup( bool do_throw = true ) ;
		///< Sets the effective group id.

	void setEffectiveGroup( SignalSafe ) ;
		///< Sets the effective group id.
		///< A signal-safe, reentrant overload.

	bool operator==( const Identity & ) const ;
		///< Comparison operator.

	bool operator!=( const Identity & ) const ;
		///< Comparison operator.

private:
	Identity() ; // no throw

private:
	uid_t m_uid ;
	gid_t m_gid ;
	HANDLE m_h ; // windows
} ;

/// \class G::IdentityUser
/// A convenience class which, when used as a private base,
/// can improve readability when calling Identity 'set' methods.
///
class G::IdentityUser 
{
protected:
	static void setRealUserTo( Identity , bool do_throw = true ) ;
		///< Sets the real userid.

	static void setEffectiveUserTo( Identity , bool do_throw = true ) ;
		///< Sets the effective userid.

	static void setEffectiveUserTo( SignalSafe , Identity ) ;
		///< Sets the effective userid.

	static void setRealGroupTo( Identity , bool do_throw = true ) ;
		///< Sets the real group id.

	static void setEffectiveGroupTo( Identity , bool do_throw = true ) ;
		///< Sets the effective group id.

	static void setEffectiveGroupTo( SignalSafe , Identity ) ;
		///< Sets the effective group id.

private:
	IdentityUser() ; // not implemented
} ;

/// \namespace G
namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Identity & identity )
	{
		return stream << identity.str() ;
	}
}

#endif

