//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gtest.cpp
//

#include "gdef.h"
#include "gtest.h"
#include "glog.h"
#include "genvironment.h"
#include <string>
#include <set>

#if defined(_DEBUG) || defined(G_TEST_ENABLED)
namespace
{
	bool done = false ;
	std::string spec ;
}
void G::Test::set( const std::string & s )
{
	done = true ;
	spec = s ;
}
bool G::Test::enabled( const char * name )
{
	if( !done )
	{
		spec = Environment::get("G_TEST",std::string()) ;
		if( !spec.empty() )
			spec = "," + spec + "," ;
	}
	done = true ;

	bool result = spec.empty() ? false : ( spec.find(","+std::string(name)+",") != std::string::npos ) ;
	if( result )
	{
		static std::set<std::string> warned ;
		if( warned.find(name) == warned.end() )
		{
			warned.insert( name ) ;
			G_WARNING( "G::Test::enabled: test case enabled: [" << name << "]" ) ;
		}
	}
	return result ;
}
bool G::Test::enabled()
{
	return true ;
}
#else
void G::Test::set( const std::string & )
{
}
bool G::Test::enabled()
{
	return false ;
}
#endif
/// \file gtest.cpp
