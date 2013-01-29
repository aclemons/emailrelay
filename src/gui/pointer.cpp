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
// pointer.cpp
//

#include "gdef.h"
#include "pointer.h"
#include "mapfile.h"
#include "gstr.h"
#include "glog.h"

G::Path Pointer::file( const std::string & argv0_in )
{
	G::Path argv0( argv0_in ) ;
	std::string ext = argv0.basename().find('.') == std::string::npos ? ".cfg" : "" ;
	argv0.removeExtension() ;
	std::string name = argv0.basename() ;
	name.append( ext ) ;
	return G::Path( G::Path(argv0_in).dirname() , name ) ;
}

void Pointer::read( G::StringMap & map , std::istream & ss )
{
	const bool underscore_to_dash = true ;
	const bool to_lower = true ;
	G::StringMap stop_list ;
	stop_list["exec"] = "" ;
	return MapFile::read( map , ss , underscore_to_dash , to_lower , "DIR" ) ;
}

void Pointer::write( std::ostream & stream , const G::StringMap & map , const G::Path & exe )
{
	const bool dash_to_underscore = true ;
	const bool to_upper = true ;
	stream << "#!/bin/sh\n" ;
	for( G::StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		std::string key = (*p).first ;
		std::string value = (*p).second ;
		if( key.find("gui-dir") == 0U )
		{
			if( dash_to_underscore )
				G::Str::replaceAll( key , "-" , "_" ) ;
			if( to_upper )
				G::Str::toUpper( key ) ;
			MapFile::writeItem( stream , key , value ) ;
		}
	}
	if( exe != G::Path() )
	{
		stream << "exec \"`dirname \\\"$0\\\"`/" << exe.basename() << "\" \"$@\"\n" ;
	}
}

/// \file pointer.cpp
