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
/// \file gcleanup.h
///

#ifndef G_CLEANUP_H
#define G_CLEANUP_H

#include "gdef.h"
#include "gpath.h"
#include "gsignalsafe.h"
#include "gexception.h"

/// \namespace G
namespace G
{
	class Cleanup ;
}

/// \class G::Cleanup
/// An interface for registering cleanup functions
/// which are called when the process terminates abnormally.
///
class G::Cleanup 
{
public:
	G_EXCEPTION( Error , "cleanup error" ) ;

	static void init() ;
		///< An optional early-initialisation function.
		///< May be called more than once.

	static void add( void (*fn)(SignalSafe,const char*) , const char * arg ) ;
		///< Adds the given handler to the list which 
		///< are to be called when the process
		///< terminates abnormally. The handler 
		///< function must be fully reentrant.
		///< The 'arg' pointer is kept.

private:
	Cleanup() ; // not implemeneted
} ;

#endif

