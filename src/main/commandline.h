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
/// \file commandline.h
///

#ifndef G_MAIN_COMMAND_LINE_H
#define G_MAIN_COMMAND_LINE_H

#include "gdef.h"
#include "gsmtp.h"
#include "garg.h"
#include "configuration.h"
#include "output.h"
#include "gstrings.h"
#include <string>

/// \namespace Main
namespace Main
{
	class CommandLine ;
	class CommandLineImp ;
}

/// \class Main::CommandLine
/// A class which deals with the command-line interface
/// to the process, both input and output. The input side is mostly
/// done by providing a Configuration object via the cfg() method.
///
class Main::CommandLine 
{
public: 
	static std::string switchSpec( bool is_windows ) ;
		///< Returns an o/s-specific G::GetOpt switch specification string.

	CommandLine( Main::Output & output , const G::Arg & arg , const std::string & spec , 
		const std::string & version , const std::string & capabilities ) ;
			///< Constructor.

	~CommandLine() ;
		///< Destructor.

	Configuration cfg() const ;
		///< Returns a Configuration object.

	bool contains( const std::string & switch_ ) const ;
		///< Returns true if the command line contained the give switch.

	bool contains( const char * switch_ ) const ;
		///< Returns true if the command line contained the give switch.

	std::string value( const std::string & switch_ ) const ;
		///< Returns the given switch's string value.

	std::string value( const char * switch_ ) const ;
		///< Returns the given switch's string value.

	unsigned int value( const std::string & switch_ , unsigned int default_ ) const ;
		///< Returns the given switch's integer value.

	unsigned int value( const char * switch_ , unsigned int default_ ) const ;
		///< Returns the given switch's integer value.

	G::Strings value( const std::string & switch_ , const std::string & separators ) const ;
		///< Returns the given switch's list-of-string value.

	G::Strings value( const char * switch_ , const char * separators ) const ;
		///< Returns the given switch's list-of-string value.

	G::Arg::size_type argc() const ;
		///< Returns the number of non-switch arguments on the command line.

	bool hasUsageErrors() const ;
		///< Returns true if the command line has usage errors (eg. invalid switch).

	bool hasSemanticError() const ;
		///< Returns true if the command line has logical errors (eg. conflicting switches).

	void showHelp( bool error_stream = false ) const ;
		///< Writes help text.

	void showUsageErrors( bool error_stream = true ) const ;
		///< Writes the usage errors.

	void showSemanticError( bool error_stream = true ) const ;
		///< Writes the logic errors.

	void logSemanticWarnings() const ;
		///< Emits warnings about conflicting switches.

	void showArgcError( bool error_stream = true ) const ;
		///< Writes a too-many-arguments error message.

	void showNoop( bool error_stream = false ) const ;
		///< Writes a nothing-to-do message.

	void showError( const std::string & reason , bool error_stream = true ) const ;
		///< Writes a failed message.

	void showVersion( bool error_stream = false ) const ;
		///< Writes the version number.

	void showBanner( bool error_stream = false , const std::string & = std::string() ) const ;
		///< Writes a startup banner.

	void showCopyright( bool error_stream = false , const std::string & = std::string() ) const ;
		///< Writes a copyright message.

	void showCapabilities( bool error_stream = false , const std::string & = std::string() ) const ;
		///< Writes a capability line.

private:
	CommandLine( const CommandLine & ) ; // not implemented
	void operator=( const CommandLine & ) ; // not implemented

private:
	CommandLineImp * m_imp ;
} ;

#endif
