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
/// \file commandline.h
///

#ifndef G_MAIN_COMMAND_LINE_H
#define G_MAIN_COMMAND_LINE_H

#include "gdef.h"
#include "garg.h"
#include "goption.h"
#include "goptions.h"
#include "goptionmap.h"
#include "output.h"
#include "output.h"
#include "gstringarray.h"
#include <string>
#include <vector>

namespace Main
{
	class CommandLine ;
}

//| \class Main::CommandLine
/// A class which deals with the command-line interface to the process, both
/// input from command-line parameters and feedback to (eg.) stdout.
///
/// Higher-level access to command-line options is provided by Main::Configuration.
///
class Main::CommandLine
{
public:
	CommandLine( Main::Output & output , const G::Arg & arg , const G::Options & options_spec ,
		const std::string & version ) ;
			///< Constructor.

	~CommandLine() ;
		///< Destructor.

	std::size_t configurations() const ;
		///< Returns the number of separate configurations contained in the
		///< one command-line.

	const G::OptionMap & options( std::size_t i ) const ;
		///< Exposes the i'th option map sub-object.

	bool argcError() const ;
		///< Returns true if the command line has non-option argument errors.

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

	void showVersion( bool error_stream = false ) const ;
		///< Writes the version number.

	void showBanner( bool error_stream = false , const std::string & = {} ) const ;
		///< Writes a startup banner.

	void showCopyright( bool error_stream = false , const std::string & = {} ) const ;
		///< Writes a copyright message.

	void showSemanticError( const std::string & semantic_error ) const ;
		///< Displays the given semantic error. See Configuration::semanticError().

	void showSemanticWarnings( const G::StringArray & semantic_warnings ) const ;
		///< Displays the given semantic warnings. See Configuration::semanticWarnings().

	void logSemanticWarnings( const G::StringArray & semantic_warnings ) const ;
		///< Logs the given semantic warnings. See Configuration::semanticWarnings().

public:
	CommandLine( const CommandLine & ) = delete ;
	CommandLine( CommandLine && ) = delete ;
	CommandLine & operator=( const CommandLine & ) = delete ;
	CommandLine & operator=( CommandLine && ) = delete ;

private:
	void showUsage( bool e ) const ;
	void showShortHelp( bool e ) const ;
	void showExtraHelp( bool e ) const ;
	void showWarranty( bool e = false , const std::string & eot = {} ) const ;
	void showSslCredit( bool e = false , const std::string & eot = {} ) const ;
	void showSslVersion( bool e = false , const std::string & eot = {} ) const ;
	void showThreading( bool e = false , const std::string & eot = {} ) const ;
	void showUds( bool e = false , const std::string & eod = {} ) const ;
	static std::string onParse( const G::StringArray & , std::size_t i , const std::string & ) ;

private:
	Output & m_output ;
	G::Options m_options_spec ;
	G::StringArray m_errors ;
	std::vector<G::OptionMap> m_option_maps ;
	std::string m_version ;
	std::string m_arg_prefix ;
	bool m_verbose {false} ;
} ;

#endif
