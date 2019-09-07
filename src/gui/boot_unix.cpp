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
// boot_unix.cpp
//

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"

// TODO - other init systems

bool Boot::able( const G::Path & dir )
{
	return
		dir != G::Path() &&
		G::Directory(dir).valid(true) &&
		G::Directory(dir).writeable() ; // (creates a probe file)
}

bool Boot::install( const G::Path & dir_boot , const std::string & name , const G::Path & startstop_src , const G::Path & )
{
	G::Path startstop_dst = dir_boot + name ;
	//if( !G::File::exists(startstop_dst) ) // moot
	G::File::copy( startstop_src , startstop_dst , G::File::NoThrow() ) ;

	std::string symlink = "../" + dir_boot.basename() + "/" + name ;
	std::string linkname = "S50" + name ;
	bool ok0 = G::File::link( symlink , dir_boot+".."+"rc2.d"+linkname , G::File::NoThrow() ) ;
	bool ok1 = G::File::link( symlink , dir_boot+".."+"rc3.d"+linkname , G::File::NoThrow() ) ;
	bool ok2 = G::File::link( symlink , dir_boot+".."+"rc4.d"+linkname , G::File::NoThrow() ) ;
	bool ok3 = G::File::link( symlink , dir_boot+".."+"rc5.d"+linkname , G::File::NoThrow() ) ;
	return ok0 && ok1 && ok2 && ok3 ;
}

bool Boot::uninstall( const G::Path & dir_boot , const std::string & name , const G::Path & , const G::Path & )
{
	std::string linkname = "S50" + name ;
	bool ok0 = G::File::remove( dir_boot+".."+"rc2.d"+linkname , G::File::NoThrow() ) ;
	bool ok1 = G::File::remove( dir_boot+".."+"rc3.d"+linkname , G::File::NoThrow() ) ;
	bool ok2 = G::File::remove( dir_boot+".."+"rc4.d"+linkname , G::File::NoThrow() ) ;
	bool ok3 = G::File::remove( dir_boot+".."+"rc5.d"+linkname , G::File::NoThrow() ) ;
	return ok0 && ok1 && ok2 && ok3 ;
}

bool Boot::installed( const G::Path & dir_boot , const std::string & name )
{
	return G::File::exists( dir_boot.dirname() + "rc2.d" + ( "S50" + name ) , G::File::NoThrow() ) ;
}

/// \file boot_unix.cpp
