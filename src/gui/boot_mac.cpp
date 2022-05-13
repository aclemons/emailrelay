//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file boot_mac.cpp
///

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"
#include <stdexcept>

bool Boot::installable( const G::Path & dir )
{
	// nasty side-effect required because Library/StartupItems may not exist
	if( !dir.empty() )
		G::File::mkdirs( dir , std::nothrow , 6 ) ;

	return
		!dir.empty() &&
		G::Directory(dir).valid(true) &&
		G::Directory(dir).writeable() ; // (creates a probe file)
}

bool Boot::install( const G::Path & dir_boot , const std::string & , const G::Path & , const G::Path & exe )
{
	G::Path plist_src = exe.dirname() + "StartupParameters.plist" ;
	G::File::mkdirs( dir_boot + exe.basename() , std::nothrow , 6 ) ;
	return
		G::File::copy( exe , dir_boot + exe.basename() + exe.basename() , std::nothrow ) &&
		G::File::copy( plist_src , dir_boot + exe.basename() + plist_src.basename() , std::nothrow ) ;
}

bool Boot::uninstall( const G::Path & dir_boot , const std::string & , const G::Path & , const G::Path & exe )
{
	return
		G::File::remove( dir_boot + exe.basename() + exe.basename() , std::nothrow ) &&
		G::File::remove( dir_boot + exe.basename() + "StartupParameters.plist" , std::nothrow ) &&
		G::File::remove( dir_boot + exe.basename() , std::nothrow ) ;
}

bool Boot::installed( const G::Path & dir_boot , const std::string & name )
{
	return G::File::exists( dir_boot + name + name , std::nothrow ) ; // not tested
}

void Boot::launchable( const G::Path & , const std::string & )
{
	return false ;
}

void Boot::launch( const G::Path & , const std::string & )
{
	// TODO
	throw std::runtime_error( "startup failed: not implemented" ) ;
}

