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
/// \file genvironment_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "genvironment.h"

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	return nowide::getenv( name , default_ ) ;
}

G::Path G::Environment::getPath( const std::string & name , const G::Path & default_ )
{
	return G::Path( nowide::getenv( name , default_.str() ) ) ;
}

G::Environment G::Environment::minimal( bool )
{
	return Environment( {} ) ;
}

void G::Environment::put( const std::string & name , const std::string & value )
{
	nowide::putenv( name , value ) ;
}

