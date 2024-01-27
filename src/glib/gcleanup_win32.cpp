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
/// \file gcleanup_win32.cpp
///

#include "gdef.h"
#include "gcleanup.h"
#include <cstring> // _strdup()

void G::Cleanup::init()
{
	// no-op
}

void G::Cleanup::add( bool (*)(SignalSafe,const char*) , const char * )
{
	// not implemented
}

void G::Cleanup::atexit( bool )
{
	// not implemented
}

void G::Cleanup::block() noexcept
{
	// not implemented
}

void G::Cleanup::release() noexcept
{
	// not implemented
}

namespace
{
	const char * strdup_ignore_leaks( const char * p )
	{
		return _strdup( p ) ; // NOLINT
	}
}

const char * G::Cleanup::strdup( const char * p )
{
	return strdup_ignore_leaks( p ) ;
}

const char * G::Cleanup::strdup( const std::string & s )
{
	return strdup_ignore_leaks( s.c_str() ) ;
}
