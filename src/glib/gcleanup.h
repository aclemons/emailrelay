//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	class CleanupImp ;
}

//| \class G::Cleanup
/// A static interface for registering cleanup functions that are called when
/// the process terminates abnormally. On unix this relates to signals like
/// SIGTERM, SIGINT etc.
///
class G::Cleanup
{
public:
	G_EXCEPTION( Error , tx("cleanup error") )
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
	struct Arg /// Opaque leaky string pointer wrapper created by G::Cleanup::arg().
	{
		const char * str() const noexcept ;
		bool isPath() const noexcept ;
		private:
		friend class G::CleanupImp ;
		const char * m_ptr {nullptr} ;
		bool m_is_path {false} ;
	} ;
	using Fn = bool (*)(const Arg &) GDEF_FSIG_NOEXCEPT ; // noexcept if c++17

	static void init() ;
		///< An optional early-initialisation function. May be called more than once.

	static void add( Fn , Arg arg ) ;
		///< Adds the given handler to the list of handlers that are to be called
		///< when the process terminates abnormally. In principle the handler
		///< function should be fully reentrant and signal-safe.
		///<
		///< The 'arg' value should come from arg(). The Arg object contains a
		///< copy of the data passed to it. It uses memory allocated on the heap
		///< which is never freed because it has to remain valid even as the
		///< process is terminating.
		///<
		///< Once the handler returns true it is removed from the list of
		///< handlers; if it returns false then it may be retried.

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

	static Arg arg( const char * ) ;
		///< Duplicates a c-string for add(). The duped pointer will be passed
		///< to the handler.

	static Arg arg( const std::string & ) ;
		///< Duplicates a string for add(). The duplicate's data() pointer will
		///< be passed to the handler.

	static Arg arg( const Path & ) ;
		///< Duplicates a path for add(). The path's string pointer will be
		///< passed to the handler.

	static Arg arg( std::nullptr_t ) ;
		///< Duplicates an empty string for add().

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
