//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file installer.h
///

#ifndef G_MAIN_GUI_INSTALLER_H
#define G_MAIN_GUI_INSTALLER_H

#include "gdef.h"
#include "gpath.h"
#include "gssl.h"
#include "gmapfile.h"
#include "gexecutablecommand.h"
#include <string>

class InstallerImp ;

//| \class Installer
/// A class that interprets a set of install variables dump()ed out
/// by the GPage class and then executes a series of installation
/// tasks using an iteration interface.
///
/// The iteration model is:
/// \code
/// install.start( gpage_dump_stream ) ;
/// while( install.next() )
/// {
///   cout << install.beforeText() << "..." ; // eg. "doing something..."
///   install.run() ;
///   cout << install.afterText() << "\n" ; // eg. " ok" ;
/// }
/// if( failed() )
///   cout << "-- failed --\n" ;
/// \endcode
///
class Installer
{
public:
	Installer( bool install_mode , bool is_windows , bool is_mac , const G::Path & payload ) ;
		///< Constructor. Initialise with start().

	~Installer() ;
		///< Destructor.

	void start( std::istream & dump_stream ) ;
		///< Initialisation.

	bool next() ;
		///< Iterator. Returns true if there is something to run().

	struct Output
	{
		std::string action_utf8 ;
		std::string subject ;
		std::string result_utf8 ;
		std::string error ;
		std::string error_utf8 ;
	} ;

	Output output() ;
		///< Returns the current task description, including the
		///< result or error if run().

	std::string failedText() const ;
		///< Returns utf8 "failed".

	std::string finishedText() const ;
		///< Returns utf8 "finished".

	void run() ;
		///< Runs the current task.

	bool done() const ;
		///< Returns true if next() returned false.

	bool failed() const ;
		///< Returns true if done() and failed.
		///< Precondition: done()

	void back( int count = 1 ) ;
		///< Moves back, typically to retry after failed().

	bool canGenerateKey() ;
		///< Returns true if the installer has the necessary code
		///< built-in to generate a key and self-signed TLS
		///< certificate.

	G::Path addLauncher() ;
		///< Adds a special laucher task. This can be done after
		///< the main set of tasks has been done(), in which case
		///< next() will move the installer to the launcher task.
		///< Returns the log-file, if any.

public:
	Installer( const Installer & ) = delete ;
	Installer( Installer && ) = delete ;
	Installer & operator=( const Installer & ) = delete ;
	Installer & operator=( Installer && ) = delete ;

private:
	bool doneImp() ;

private:
	bool m_installing ;
	bool m_is_windows ;
	bool m_is_mac ;
	G::Path m_payload ;
	std::unique_ptr<InstallerImp> m_imp ;
} ;

#endif
