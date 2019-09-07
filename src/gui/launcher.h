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
/// \file launcher.h
///

#ifndef G_LAUNCHER_H
#define G_LAUNCHER_H

#include "gdef.h"
#include "gpath.h"
#include "gexecutablecommand.h"
#include "qt.h"
#include <string>

/// \class Launcher
/// A qt widget that launches the server process and displays
/// any initial output. This is mainly intended for MacOS.
///
/// The launcher button on the stack of pages is only visible
/// when running in mac mode, and it is only enabled on the
/// last ("progress") page once the install has finished.
///
class Launcher : private QDialog
{Q_OBJECT
public:
	explicit Launcher( QWidget & parent , const G::ExecutableCommand & command_line ) ;
		///< Constructor. The command-line is

	virtual ~Launcher() ;
		///< Destructor.

private:
	Launcher( const Launcher & ) ;
	void operator=( const Launcher & ) ;
	void addLine( const std::string & ) ;

private slots:
	void poke() ;

private:
	QPushButton * m_ok_button ;
	QTextEdit * m_text_edit ;
	QString m_text ;
	QTimer * m_timer ;
	G::ExecutableCommand m_command_line ;
	G::Path m_log_file ;
	std::string m_shell_command ;
	unsigned int m_poke_count ;
	int m_line_count ;
} ;

#endif
