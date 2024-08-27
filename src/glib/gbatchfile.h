//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstringarray.h"
#include "gexception.h"
#include <iostream>
#include <string>
#include <vector>
#include <new>

namespace G
{
	class BatchFile ;
}

//| \class G::BatchFile
/// A class for reading and writing windows-style startup batch files
/// containing a single command-line, optionally using "start".
///
/// Eg:
/// \code
///  @echo off
///  rem a windows batch file
///  start "my app" "c:\my app\run.exe" arg-one "arg two"
/// \endcode
///
/// Batch files always use the OEM code page on Windows but at
/// this interface it's all UTF-8.
///
class G::BatchFile
{
public:
	G_EXCEPTION_CLASS( Error , tx("batch file error") )

	explicit BatchFile( const Path & ) ;
		///< Constructor that reads from a file.

	BatchFile( const Path & , std::nothrow_t ) ;
		///< Constructor that reads from a file that might be missing
		///< or empty. The line() will be empty if construction fails.

	std::string line() const ;
		///< Returns the main command-line from within the batchfile, with normalised
		///< spaces, without any "start" prefix, and including quotes.

	bool empty() const ;
		///< Returns true if line() is empty.

	std::string name() const ;
		///< Returns the "start" window name, if any.

	const StringArray & args() const ;
		///< Returns the startup command-line broken up into de-quoted pieces.
		///< The first item in the list will be the executable.

	std::size_t lineArgsPos() const ;
		///< Returns the position in line() where the arguments start.

	static void write( const Path & new_batch_file , const StringArray & args ,
		const std::string & start_window_name = {} , bool do_backup = false ) ;
			///< Writes a startup batch file, including a "start" prefix.
			///< If the "start" window name is not supplied then it is
			///< derived from the command-line. The 'args' must not
			///< contain double-quote characters. The first 'args' item
			///< is the target executable.

private:
	void init( const Path & ) ;
	void clear() ;
	static std::string quote( const std::string & ) ;
	static std::string percents( const std::string & ) ;
	static void dequote( std::string & ) ;
	std::string readFrom( std::istream & , const std::string & , bool do_throw = true ) ;
	struct ParseResult
	{
		std::string name ;
		std::string line ;
		std::string error ;
	} ;
	static ParseResult parse( const std::string & ) ;
	static StringArray split( const std::string & ) ;
	static bool ignorable( const std::string & line ) ;
	static bool relevant( const std::string & line ) ;
	static void backup( const Path & ) ;

private:
	std::string m_raw_line ;
	std::string m_line ;
	std::string m_name ;
	StringArray m_args ;
} ;

#endif
