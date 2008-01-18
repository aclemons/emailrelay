//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// boot_mac.cpp
//

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"
#include <stdexcept>

bool Boot::able( G::Path dir , G::Path )
{
	// nasty side-effect required because Library/StartupItems may not exist
	if( dir != G::Path() )
		G::File::mkdirs( dir , G::File::NoThrow() , 6 ) ;

	return 
		dir != G::Path() && 
		G::Directory(dir).valid(true) &&
		G::Directory(dir).writeable() ; // (creates a probe file)
}

bool Boot::install( G::Path dir_boot , G::Path target , G::Strings )
{
	bool ok =
		G::File::mkdir( dir_boot + target.basename() , G::File::NoThrow() ) &&
		G::File::copy( target , dir_boot + target.basename() + target.basename() , G::File::NoThrow() ) ;

	G::Path plist_path = dir_boot + target.basename() + "StartupParameters.plist" ;
	std::ofstream plist( plist_path.str().c_str() ) ;
	plist <<
		"{\n" 
		"  Description = \"xxx\";\n"
		"  Provides = (\"xxx\");\n"
		"  Requires = (\"xxx\");\n"
		"  Uses = (\"xxx\");\n"
		"  OrderPreference = (\"xxx\");\n"
		"  Messages = {\n"
		"    restart = \"...\";\n"
		"    start = \"...\";\n"
		"    stop = \"...\";\n"
		"}\n"
	;
	plist << std::flush ;
	ok = plist.good() && ok ;
	return ok ;
}

bool Boot::uninstall( G::Path dir_boot , G::Path target , G::Strings )
{
	return
		G::File::remove( dir_boot + target.basename() + target.basename() , G::File::NoThrow() ) &&
		G::File::remove( dir_boot + target.basename() + "StartupParameters.plist" , G::File::NoThrow() ) &&
		G::File::remove( dir_boot + target.basename() , G::File::NoThrow() ) ;
}

/// \file boot_mac.cpp
