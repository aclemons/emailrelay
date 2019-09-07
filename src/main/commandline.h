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
///
/// \file commandline.h
///

#ifndef G_MAIN_COMMAND_LINE_H
#define G_MAIN_COMMAND_LINE_H

#include "gdef.h"
#include "garg.h"
#include "ggetopt.h"
#include "output.h"
#include "configuration.h"
#include "output.h"
#include "gstrings.h"
#include <string>

namespace Main
{
	class CommandLine ;
}

/// \class Main::CommandLine
/// A class which deals with the command-line interface to the process, both
/// input from command-line parameters and feedback to (eg.) stdout.
///
/// Higher-level access to command-line options is provided by the Configuration class.
///
class Main::CommandLine
{
public:
	CommandLine( Main::Output & output , const G::Arg & arg , const std::string & spec ,
		const std::string & version ) ;
			///< Constructor.

	~CommandLine() ;
		///< Destructor.

	const G::OptionMap & map() const ;
		///< Exposes the option-map sub-object.

	const G::Options & options() const ;
		///< Exposes the options sub-object.

	G::Arg::size_type argc() const ;
		///< Returns the number of non-option arguments on the command line.

	bool hasUsageErrors() const ;
		///< Returns true if the command line has usage errors (eg. invalid option).

	void showHelp( bool error_stream = false ) const ;
		///< Writes help text.

	void showUsageErrors( bool error_stream = true ) const ;
		///< Writes the usage errors.

	void showArgcError( bool error_stream = true ) const ;
		///< Writes a too-many-arguments error message.

	void showNothingToSend( bool error_stream = false ) const ;
		///< Writes a nothing-to-send message.

	void showNothingToDo( bool error_stream = false ) const ;
		///< Writes a nothing-to-do message.

	void showFinished( bool error_stream = false ) const ;
		///< Writes an all-done message.

	void showError( const std::string & reason , bool error_stream = true ) const ;
		///< Writes a failed message.

	void showVersion( bool error_stream = false ) const ;
		///< Writes the version number.

	void showBanner( bool error_stream = false , const std::string & = std::string() ) const ;
		///< Writes a startup banner.

	void showCopyright( bool error_stream = false , const std::string & = std::string() ) const ;
		///< Writes a copyright message.

	void showSemanticError( const std::string & semantic_error ) const ;
		///< Displays the given semantic error. See Configuration::semanticError().

	void showSemanticWarnings( const G::StringArray & semantic_warnings ) const ;
		///< Displays the given semantic warnings. See Configuration::semanticWarnings().

	void logSemanticWarnings( const G::StringArray & semantic_warnings ) const ;
		///< Logs the given semantic warnings. See Configuration::semanticWarnings().

private:
	CommandLine( const CommandLine & ) g__eq_delete ;
	void operator=( const CommandLine & ) g__eq_delete ;
	void showUsage( bool e ) const ;
	void showShortHelp( bool e ) const ;
	void showExtraHelp( bool e ) const ;
	void showWarranty( bool e = false , const std::string & eot = std::string() ) const ;
	void showSslCredit( bool e = false , const std::string & eot = std::string() ) const ;
	void showSslVersion( bool e = false , const std::string & eot = std::string() ) const ;
	void showThreading( bool e = false , const std::string & eot = std::string() ) const ;
	bool sanityCheck( const G::Path & ) ;

private:
	Output & m_output ;
	std::string m_version ;
	G::Arg m_arg ;
	G::GetOpt m_getopt ;
	std::string m_insanity ;
} ;

#endif
