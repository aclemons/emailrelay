//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file boot_unix.cpp
///

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gstr.h"
#include "gfile.h"
#include "gexecutablecommand.h"
#include "gnewprocess.h"
#include "gdirectory.h"
#include <sstream>
#include <stdexcept>

bool Boot::able( const G::Path & dir )
{
	// check /etc/init.d is writable
	return
		!dir.empty() &&
		G::Directory(dir).valid(true) &&
		G::Directory(dir).writeable() ; // (creates a probe file)
}

void Boot::install( const G::Path & dir_boot , const std::string & name , const G::Path & startstop_src , const G::Path & )
{
	G::Path startstop_dst = dir_boot + name ;
	bool ok0 = G::File::copy( startstop_src , startstop_dst , std::nothrow ) ;

	std::string symlink = "../" + dir_boot.basename() + "/" + name ;
	std::string linkname = "S50" + name ;
	bool ok1 = G::File::link( symlink , dir_boot+".."+"rc2.d"+linkname , std::nothrow ) ;
	bool ok2 = G::File::link( symlink , dir_boot+".."+"rc3.d"+linkname , std::nothrow ) ;
	bool ok3 = G::File::link( symlink , dir_boot+".."+"rc4.d"+linkname , std::nothrow ) ;
	bool ok4 = G::File::link( symlink , dir_boot+".."+"rc5.d"+linkname , std::nothrow ) ;
	bool ok = ok0 && ok1 && ok2 && ok3 && ok4 ;
	if( !ok )
		throw std::runtime_error( "failed to create symlinks [" + (dir_boot+".."+"rc2.d"+linkname).str() + "] etc" ) ;
}

bool Boot::uninstall( const G::Path & dir_boot , const std::string & name , const G::Path & , const G::Path & )
{
	std::string linkname = "S50" + name ;
	bool ok0 = G::File::remove( dir_boot+".."+"rc2.d"+linkname , std::nothrow ) ;
	bool ok1 = G::File::remove( dir_boot+".."+"rc3.d"+linkname , std::nothrow ) ;
	bool ok2 = G::File::remove( dir_boot+".."+"rc4.d"+linkname , std::nothrow ) ;
	bool ok3 = G::File::remove( dir_boot+".."+"rc5.d"+linkname , std::nothrow ) ;
	return ok0 && ok1 && ok2 && ok3 ;
}

bool Boot::installed( const G::Path & dir_boot , const std::string & name )
{
	return G::File::exists( dir_boot.dirname() + "rc2.d" + ( "S50" + name ) , std::nothrow ) ;
}

void Boot::launch( const G::Path & , const std::string & name )
{
	G::ExecutableCommand cmd( "/usr/sbin/service" , {name,"start"} , false ) ;
	G::NewProcess task(
		G::NewProcessConfig( cmd.exe() )
			.set_args( cmd.args() )
			.set_fd_stdout( G::NewProcess::Fd::devnull() )
			.set_fd_stderr( G::NewProcess::Fd::pipe() )
			.set_exec_error_format( "failed to execute ["+cmd.exe().str()+"]: __strerror__" ) ) ;

	int rc = task.waitable().wait().get() ;
	std::string output = G::Str::printable( G::Str::trimmed(task.waitable().output(),G::Str::ws()) ) ;
	if( rc != 0 && output.empty() )
		output = "error" ;

	if( rc != 0 )
		throw std::runtime_error( "failed to run ["+cmd.displayString()+"]: " + output ) ;
}

