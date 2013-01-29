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
// boot_unix.cpp
//

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"

bool Boot::able( G::Path dir )
{
	return
		dir != G::Path() &&
		G::File::exists( dir+".."+"rc3.d" ) &&
		G::Directory(dir).valid(true) &&
		G::Directory(dir).writeable() ; // (creates a probe file)
}

bool Boot::install( G::Path dir_boot , G::Path target , G::Strings )
{
	std::string name = std::string() + "S50" + target.basename() ;
	std::string to_relative = std::string() + "../" + dir_boot.basename() + "/" + target.basename() ;

	// 'target' is our squirrelled away copy of the start/stop script -- see Dir::bootcopy()
	G::Path to_absolute = dir_boot + "/" + target.basename() ;
	if( !G::File::exists(to_absolute) )
		G::File::copy( target , to_absolute , G::File::NoThrow() ) ;

	bool ok0 = G::File::link( to_relative , dir_boot+".."+"rc2.d"+name , G::File::NoThrow() ) ;
	bool ok1 = G::File::link( to_relative , dir_boot+".."+"rc3.d"+name , G::File::NoThrow() ) ;
	bool ok2 = G::File::link( to_relative , dir_boot+".."+"rc4.d"+name , G::File::NoThrow() ) ;
	bool ok3 = G::File::link( to_relative , dir_boot+".."+"rc5.d"+name , G::File::NoThrow() ) ;
	return ok0 && ok1 && ok2 && ok3 ;
}

bool Boot::uninstall( G::Path dir_boot , G::Path target , G::Strings )
{
	std::string name = std::string() + "S50" + target.basename() ;
	bool ok0 = G::File::remove( dir_boot+".."+"rc2.d"+name , G::File::NoThrow() ) ;
	bool ok1 = G::File::remove( dir_boot+".."+"rc3.d"+name , G::File::NoThrow() ) ;
	bool ok2 = G::File::remove( dir_boot+".."+"rc4.d"+name , G::File::NoThrow() ) ;
	bool ok3 = G::File::remove( dir_boot+".."+"rc5.d"+name , G::File::NoThrow() ) ;
	return ok0 && ok1 && ok2 && ok3 ;
}

/// \file boot_unix.cpp
