//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gfs.h
//

#ifndef G_FS_H
#define G_FS_H

#include "gdef.h"

namespace G
{
	class FileSystem ;
}

// Class: G::FileSystem
// Description: Provides information about the
// local operating system's file system
// conventions.
// See also: Path, File
//
class G::FileSystem 
{
public:
	static const char *nullDevice() ;
	static bool allowsSpaces() ;
	static char slash() ; 					// separator
	static char nonSlash() ; 				// to be converted to slash()
	static bool caseSensitive() ;
	static bool usesDriveLetters() ;		// <drive>:
	static bool leadingDoubleSlash() ;		// win32 network paths
} ;

#endif
