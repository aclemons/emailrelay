//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// commandline.h
//

#ifndef G_MAIN_COMMAND_LINE_H
#define G_MAIN_COMMAND_LINE_H

#include "gdef.h"
#include "gsmtp.h"
#include "garg.h"
#include "configuration.h"
#include "output.h"
#include "ggetopt.h"
#include <string>
#include <iostream>

namespace Main
{
	class CommandLine ;
}

// Class: Main::CommandLine
// Description: A class which deals with the command-line interface
// to the process (both input and output).
//
class Main::CommandLine 
{
public: 
	static std::string switchSpec( bool is_windows ) ;
		// Returns an o/s-specific G::GetOpt switch specification string.

	CommandLine( Main::Output & output , const G::Arg & arg , const std::string & spec , 
		const std::string & version ) ;
			// Constructor.

	Configuration cfg() const ;
		// Returns a Configuration object.

	bool contains( const std::string & switch_ ) const ;
		// Returns true if the command line contained the give switch.

	std::string value( const std::string & switch_ ) const ;
		// Returns the given switch's value.

	unsigned int argc() const ;
		// Returns the number of non-switch arguments on the command line.

	bool hasUsageErrors() const ;
		// Returns true if the command line has usage errors (eg. invalid switch).

	bool hasSemanticError() const ;
		// Returns true if the command line has logical errors (eg. conflicting switches).

	void showHelp( bool error_stream = false ) const ;
		// Writes help text.

	void showUsageErrors( bool error_stream = true ) const ;
		// Writes the usage errors.

	void showSemanticError( bool error_stream = true ) const ;
		// Writes the logic errors.

	void showArgcError( bool error_stream = true ) const ;
		// Writes a too-many-arguments error message.

	void showNoop( bool error_stream = false ) const ;
		// Writes a nothing-to-do message.

	void showVersion( bool error_stream = false ) const ;
		// Writes the version number.

	void showBanner( bool error_stream = false ) const ;
		// Writes a startup banner.

	void showCopyright( bool error_stream = false ) const ;
		// Writes a copyright message.

private:
	void showWarranty( bool error_stream ) const ;
	void showShortHelp( bool error_stream ) const ;
	std::string semanticError() const ;
	void showUsage( bool e ) const ;
	void showExtraHelp( bool error_stream ) const ;
	static std::string switchSpec_unix() ;
	static std::string switchSpec_windows() ;

private:
	Output & m_output ;
	std::string m_version ;
	G::Arg m_arg ;
	G::GetOpt m_getopt ;

public:
	class Show // A private implementation class used by Main::CommandLine.
	{
		public: Show( Main::Output & , bool e ) ;
		public: std::ostream & s() ;
		public: ~Show() ;
		private: Show( const Show & ) ; // not implemented
		private: void operator=( const Show & ) ; // not implemented
		private: std::ostringstream m_ss ;
		private: Main::Output & m_output ;
		private: bool m_e ;
		private: static Show * m_this ;
	} ;
} ;

#endif
