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
/// \file gexception.cpp
///

#include "gdef.h"
#include "gexception.h"
#include "gstr.h"

G::Exception::Exception( std::initializer_list<string_view> args ) :
	std::runtime_error(join(args))
{
}

G::Exception::Exception( string_view what ) :
	Exception{what}
{
}

G::Exception::Exception( string_view what , string_view more ) :
	Exception{what,more}
{
}

G::Exception::Exception( string_view what , string_view more1 , string_view more2 ) :
	Exception{what,more1,more2}
{
}

G::Exception::Exception( string_view what , string_view more1 , string_view more2 ,
	string_view more3 ) :
		Exception{what,more1,more2,more3}
{
}

G::Exception::Exception( string_view what , string_view more1 , string_view more2 ,
	string_view more3 , string_view more4 ) :
		Exception{what,more1,more2,more3,more4}
{
}

std::string G::Exception::join( std::initializer_list<string_view> args )
{
	std::string result ;
	for( auto arg : args )
		result.append(": ",result.empty()||arg.empty()?0U:2U).append(arg.data(),arg.size()) ;
	return result ;
}

