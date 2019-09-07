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
//
// launcher.cpp
//

#include "gdef.h"
#include "launcher.h"
#include "gprocess.h"
#include "gfile.h"
#include "gtest.h"
#include "gstr.h"
#include <fstream>
#include <sstream>

namespace
{
	G::Path logFile()
	{
		return G::Path( "/tmp" , "launcher." + G::Process::Id().str() + ".tmp" ) ;
	}
	std::string shellCommand( const G::ExecutableCommand & command_line , const G::Path & log_file )
	{
		G::StringArray parts = command_line.args() ;
		parts.insert( parts.begin() , command_line.exe().str() ) ;
		for( G::StringArray::iterator p = parts.begin() ; p != parts.end() ; ++p )
		{
			*p = G::Str::escaped( *p , '\\' , G::Str::meta().substr(1U) , G::Str::meta().substr(1U) ) ;
			if( (*p).find(' ') != std::string::npos || (*p).find('\t') != std::string::npos )
				*p = "\"" + *p + "\"" ;
		}
		return G::Str::join(" ",parts) + " 2>" + log_file.str() + " &" ;
	}
}

Launcher::Launcher( QWidget & parent , const G::ExecutableCommand & command_line ) :
	QDialog( &parent , Qt::Dialog ) ,
	m_ok_button(nullptr) ,
	m_text_edit(nullptr) ,
	m_command_line(command_line) ,
	m_log_file(logFile()) ,
	m_poke_count(0U) ,
	m_line_count(-1)
{
	m_ok_button = new QPushButton(tr("Ok")) ;

	m_text_edit = new QTextEdit ; // or QPlainTextEdit
	m_text_edit->setReadOnly(true) ;
	m_text_edit->setWordWrapMode(QTextOption::WrapAnywhere) ;

	// prepare the shell command-line
	if( G::Test::enabled("launcher-test") ) m_command_line = G::ExecutableCommand("./launcher-test.sh") ;
	m_command_line.add( "--daemon" ) ;
	m_shell_command = shellCommand( m_command_line , m_log_file ) ;

	// display the launcher command-line
	addLine( m_command_line.displayString() ) ;
	addLine( m_shell_command ) ;

	QHBoxLayout * button_layout = new QHBoxLayout ;
	button_layout->addStretch( 1 ) ;
	button_layout->addWidget( m_ok_button ) ;
	button_layout->addStretch( 1 ) ;

	QVBoxLayout * layout = new QVBoxLayout ;
	layout->addWidget( m_text_edit ) ;
	layout->addLayout( button_layout ) ;
	setLayout( layout ) ;

	connect( m_ok_button , SIGNAL(clicked()) , this , SLOT(close()) ) ;

	// start a periodic timer to display the log file
	m_timer = new QTimer( this ) ;
	connect( m_timer , SIGNAL(timeout()) , this , SLOT(poke()) ) ;
	m_timer->start( 120 ) ; // ms

	setModal( true ) ;
	show() ;
}

Launcher::~Launcher()
{
	G::File::remove( m_log_file , G::File::NoThrow() ) ;
	if( m_timer != nullptr )
		m_timer->stop() ;
}

void Launcher::poke()
{
	m_poke_count++ ;
	if( m_poke_count == 1U )
	{
		int rc = system( m_shell_command.c_str() ) ; G_IGNORE_VARIABLE(int,rc) ;
	}
	else if( m_poke_count == 100U )
	{
		addLine( "[stopped monitoring]" ) ;
		m_timer->stop() ;
		return ;
	}

	std::ifstream file( m_log_file.str().c_str() ) ;
	int n = 0 ;
	for( ; file.good() ; n++ )
	{
		std::string line = G::Str::readLineFrom( file ) ;
		if( file.good() && n > m_line_count )
		{
			addLine( line ) ;
			m_line_count = n ;
		}
	}
}

void Launcher::addLine( const std::string & line_in )
{
	std::string line = line_in + "\n" ;
	m_text.append( QString::fromLocal8Bit(line.data(),static_cast<int>(line.size())) ) ;
	m_text_edit->setFontFamily("courier") ;
	m_text_edit->setPlainText( m_text ) ;
}

/// \file launcher.cpp
