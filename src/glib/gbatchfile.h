//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gbatchfile.h
///

#ifndef G_BATCH_FILE_H
#define G_BATCH_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include "gexception.h"
#include <iostream>
#include <string>
#include <vector>

namespace G
{
	class BatchFile ;
}

/// \class G::BatchFile
/// A class for reading and writing windows-style startup batch files
/// containing a single command-line, optionally using "start".
///
class G::BatchFile
{
public:
	G_EXCEPTION( Error , "batch file error" ) ;
	struct NoThrow /// Overload discriminator for G::BatchFile.
		{} ;

	explicit BatchFile( const G::Path & ) ;
		///< Constructor that reads from a file.

	BatchFile( const G::Path & , NoThrow ) ;
		///< Constructor that reads from a file that might be missing
		///< or empty.

	explicit BatchFile( std::istream & , const std::string & stream_name = std::string() ) ;
		///< Constructor that reads from a stream.

	std::string line() const ;
		///< Returns the main command-line from within the batchfile, with normalised
		///< spaces, without any "start" prefix, and including quotes.

	std::string name() const ;
		///< Returns the "start" window name, if any.

	const G::StringArray & args() const ;
		///< Returns the startup command-line broken up into de-quoted pieces.
		///< The first item in the list will be the executable.

	static void write( const G::Path & , const StringArray & args ,
		const std::string & start_window_name = std::string() ) ;
			///< Writes a startup batch file, including a "start" prefix.
			///< If the "start" window name is not supplied then it is
			///< derived from the command-line.

private:
	BatchFile( const BatchFile & ) ;
	void operator=( const BatchFile & ) ;
	static std::string quote( const std::string & ) ;
	static std::string percents( const std::string & ) ;
	static void dequote( std::string & ) ;
	std::string readFrom( std::istream & , const std::string & , bool ) ;
	static StringArray split( const std::string & ) ;
	static bool ignorable( const std::string & line ) ;
	static bool relevant( const std::string & line ) ;
	static std::string join( const std::string & file_name , unsigned int line_number ) ;

private:
	std::string m_line ;
	std::string m_name ;
	G::StringArray m_args ;
} ;

#endif
