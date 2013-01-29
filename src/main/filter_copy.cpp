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
//
// filter_copy.cpp
//
// A utility that can be installed as a "--filter" program to copy the message 
// envelope into all spool sub-directories for use by "--pop-by-name".
//
// If the envelope in the parent directory has been copied at least once then
// it is removed and the program exits with a value of 100.
// 

#include "gdef.h"
#include "filter.h"

bool filter_match( G::Path , std::string )
{
	return true ;
}

std::string filter_read_to( const std::string & )
{
	return std::string() ;
}

int main( int argc , char * argv [] )
{
	return filter_main( argc , argv ) ;
}

/// \file filter_copy.cpp
