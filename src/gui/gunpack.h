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
/// \file gunpack.h
///

#ifndef G_UNPACK_H
#define G_UNPACK_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include "gstrings.h"
#include "unpack.h"
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

/// \namespace G
namespace G
{
	class Unpack ;
}

/// \class G::Unpack
/// A class for unpacking self-extracting executables.
///
/// The executable is expected to have the following appended to it:
/// * an is-compressed flag byte in ascii: '1' or '0'
/// * one space-or-newline byte
/// * the directory of whitespace-separated (size,flags,path) tuples, ending with (0,-,end)
/// * the concatenated packed files, possibly compressed
/// * the original file size in 12 bytes of space-padded decimal ascii
///
/// A self-extracting executable containing a payload of 
/// zlib-deflated files can be constructed from the output 
/// of this shell script:
/// \code
///   #!/bin/sh
///   cat $1
///   echo 1
///   ls -l *.z | awk '{printf("%s - %s\n",$5,$9)}'
///   echo 0 - end
///   cat *.z
///   ls -l $1 | awk '{printf("%11d\n",$5)}'
/// \endcode
///
class G::Unpack 
{
public:
	G_EXCEPTION( PackingError , "unpacking error" ) ;
	G_EXCEPTION( NoSuchFile , "no such packed file" ) ;
	/// An overload discriminator class for the Unpack constructor.
	class NoThrow 
		{} ;

	static bool isPacked( Path ) ;
		///< Returns true if the given file contains a set of
		///< packed files.

	static int fileCount( Path ) ;
		///< Returns the number of packed files. Returns
		///< zero if not isPacked().

	explicit Unpack( Path argv0 ) ;
		///< Constructor.

	Unpack( Path argv0 , NoThrow ) ;
		///< Constructor. If the file is not a packed file then
		///< the names() method will return an empty list and
		///< unpack() will do nothing.

	~Unpack() ;
		///< Destructor.

	Path path() const ;
		///< Returns the path as passed in to the constructor.

	void unpack( const Path & to_base_dir ) ;
		///< Unpacks all the files.

	void unpack( const Path & to_base_dir , const std::string & name ) ;
		///< Unpacks one file. The final path of the unpacked file is
		///< the concatenation of the two parameters. Throws NoSuchFile if the
		///< name is invalid.

	void unpack( const std::string & name , const Path & dst ) ;
		///< Unpacks one file. The final path of the unpacked file
		///< is exactly the destination path given, idependent of
		///< the packed name. Throws NoSuchFile if the
		///< name is invalid.

	std::string flags( const std::string & name ) const ;
		///< Returns the flags associated with the given file.
		///< The flags are not interpreted by this class.

	Strings names() const ;
		///< Returns the list of file names.

	void unpackOriginal( const Path & dst ) ;
		///< Copies the unpacked executable to the given path.
		///< Does nothing if not packed. Throws on error.

	std::string unpackOriginal( const Path & dst , NoThrow ) ;
		///< Copies the unpacked executable to the given path.
		///< Does nothing if not packed. Returns a reason string 
		///< on error.

private:
	Unpack( const Unpack & ) ;
	void operator=( const Unpack & ) ;

private:
	Path m_path ;
	::Unpack * m_imp ;
} ;

#endif
