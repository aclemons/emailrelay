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
/// \file gtest.h
///
	
#ifndef G_TEST_H
#define G_TEST_H

#include "gdef.h"
#include <string>

/// \namespace G
namespace G
{
	class Test ;
}

/// \class G::Test
/// A static interface for enabling test features at run-time.
/// Typically does nothing in a release build.
///
class G::Test 
{
public:
	static bool enabled() ;
		///< Returns true if test features are enabled.

	static bool enabled( const char * name ) ;
		///< Returns true if the specified test feature is enabled.
} ;

#if ! defined(_DEBUG)
inline bool G::Test::enabled( const char * )
{
	return false ;
}
#endif

#endif
