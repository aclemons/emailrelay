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
/// \file gscope.h
///

#ifndef G_SCOPE_H
#define G_SCOPE_H

#include "gdef.h"
#include <functional>
#include <utility>

namespace G
{
	class ScopeExit ;
	class ScopeExitSetFalse ;
}

//| \class G::ScopeExit
/// A class that calls an exit function at the end of its scope.
/// Eg:
/// \code
/// {
///   int fd = open( ... ) ;
///   ScopeExit closer( [&](){close(fd);} ) ;
///   int nread = read( fd , ... ) ;
/// }
/// \endcode
///
class G::ScopeExit
{
public:
	explicit ScopeExit( std::function<void()> fn ) ;
		///< Constructor.

	~ScopeExit() ;
		///< Destructor. Calls the exit function unless
		///< release()d.

	void release() noexcept ;
		///< Deactivates the exit function.

public:
	ScopeExit( const ScopeExit & ) = delete ;
	ScopeExit( ScopeExit && ) = delete ;
	void operator=( const ScopeExit & ) = delete ;
	void operator=( ScopeExit && ) = delete ;

private:
	std::function<void()> m_fn ;
} ;

//| \class G::ScopeExitSetFalse
/// A class that sets a boolean variable to false at the
/// end of its scope.
/// Eg:
/// \code
/// {
///   ScopeExitSetFalse _( m_busy = true ) ;
///   ...
/// }
/// \endcode
///
class G::ScopeExitSetFalse
{
public:
	explicit ScopeExitSetFalse( bool & bref ) noexcept ;
		///< Constructor.

	~ScopeExitSetFalse() noexcept ;
		///< Destructor, sets the bound value to false.

public:
	ScopeExitSetFalse( const ScopeExitSetFalse & ) = delete ;
	ScopeExitSetFalse( ScopeExitSetFalse && ) = delete ;
	void operator=( const ScopeExitSetFalse & ) = delete ;
	void operator=( ScopeExitSetFalse && ) = delete ;

private:
	bool & m_bref ;
} ;

inline
G::ScopeExit::ScopeExit( std::function<void()> fn ) :
	m_fn(std::move(fn))
{
}

inline
void G::ScopeExit::release() noexcept
{
	m_fn = nullptr ;
}

inline
G::ScopeExit::~ScopeExit()
{
	if( m_fn )
		m_fn() ;
}

inline
G::ScopeExitSetFalse::ScopeExitSetFalse( bool & bref ) noexcept :
	m_bref(bref)
{
}

inline
G::ScopeExitSetFalse::~ScopeExitSetFalse() noexcept
{
	m_bref = false ;
}

#endif
