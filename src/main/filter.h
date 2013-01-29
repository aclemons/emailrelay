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
/// \file filter.h
///

#ifndef G_MAIN_FILTER_H
#define G_MAIN_FILTER_H

#include "gdef.h"
#include "gpath.h"
#include <string>

void filter_help( const std::string & prefix ) ;
bool filter_run( const std::string & content ) ;
int filter_main( int argc , char * argv [] ) ;

bool filter_match( G::Path , std::string ) ;
std::string filter_read_to( const std::string & ) ;

#endif
