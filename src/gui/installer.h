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
/// \file installer.h
///

#ifndef INSTALLER_H__
#define INSTALLER_H__

#include "gdef.h"
#include "gpath.h"
#include <string>

class InstallerImp ; 

/// \class Installer
/// A class that interprets a set of install variables
/// dump()ed out by the GPage class and then executes a series of 
/// installation tasks using an iteration interface.
///
class Installer 
{
public:
	Installer( G::Path argv0 , G::Path payload , bool install_mode ) ;
		///< Constructor. Initialise with start(). The argv0 and 
		///< payload paths can be the same.

	~Installer() ;
		///< Destructor.

	void start( std::istream & dump_stream ) ;
		///< Initialisation.

	bool next() ;
		///< Iterator. Returns true if there is something
		///< to run().

	std::string beforeText() ;
		///< Returns the current task description.

	std::string afterText() ;
		///< Returns the current task's status.

	void run() ;
		///< Runs the current task.

	bool done() const ;
		///< Returns true if next() returned false.

	bool failed() const ;
		///< Returns true if done() and failed.
		///< Precondition: done()

private:
	Installer( const Installer & ) ;
	void operator=( const Installer & ) ;
	void cleanup( const std::string & = std::string() ) ;

private:
	G::Path m_argv0 ;
	G::Path m_payload ;
	bool m_installing ;
	InstallerImp * m_imp ;
	std::string m_reason ;
} ;

#endif
