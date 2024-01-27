//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gcleanup.h
///

#ifndef G_CLEANUP_H
#define G_CLEANUP_H

#include "gdef.h"
#include "gpath.h"
#include "gsignalsafe.h"
#include "gexception.h"

namespace G
{
	class Cleanup ;
}

//| \class G::Cleanup
/// A static interface for registering cleanup functions that are called when
/// the process terminates abnormally. On unix this relates to signals like
/// SIGTERM, SIGINT etc.
///
class G::Cleanup
{
public:
	G_EXCEPTION( Error , tx("cleanup error") ) ;
	struct Block /// A RAII class to temporarily block signal delivery.
	{
		explicit Block( bool active = true ) noexcept ;
		~Block() ;
		bool m_active ;
		Block( const Block & ) = delete ;
		Block( Block && ) = delete ;
		Block & operator=( const Block & ) = delete ;
		Block & operator=( Block && ) = delete ;
	} ;

	static void init() ;
		///< An optional early-initialisation function. May be called more than once.

	static void add( bool (*fn)(SignalSafe,const char*) , const char * arg ) ;
		///< Adds the given handler to the list of handlers that are to be called
		///< when the process terminates abnormally. The handler function must be
		///< fully reentrant, hence the SignalSafe dummy parameter. The 'arg'
		///< pointer is kept.

	static void atexit( bool active = true ) ;
		///< Ensures that the cleanup functions are also called via atexit(), in
		///< addition to abnormal-termination signals.
		///<
		///< This can be useful when ancient third-party library code (eg. Xlib)
		///< might call exit(), but be careful to disable these exit handlers
		///< before normal termination by calling atexit(false).

	static void block() noexcept ;
		///< Temporarily blocks signals until release()d. This should be used
		///< before creating threads so that only the main thread does signal
		///< handling.

	static void release() noexcept ;
		///< Releases block()ed signals.

	static const char * strdup( const char * ) ;
		///< A strdup() function that makes it clear in the stack trace
		///< that leaks are expected.

	static const char * strdup( const std::string & ) ;
		///< A strdup() function that makes it clear in the stack trace
		///< that leaks are expected.

public:
	Cleanup() = delete ;
} ;

inline
G::Cleanup::Block::Block( bool active ) noexcept :
	m_active(active)
{
	if( m_active )
		Cleanup::block() ;
}

inline
G::Cleanup::Block::~Block()
{
	if( m_active )
		Cleanup::release() ;
}

#endif
