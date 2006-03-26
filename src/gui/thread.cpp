//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// thread.cpp
//

#include "qt.h"
#include "thread.h"
#include "gprocess.h"
#include "gstr.h"
#include "gdebug.h"
#include <iostream>

Thread::Thread( G::Path tool , const G::Strings & args ) :
	m_tool(tool) ,
	m_args(args) ,
	m_rc(1)
{
}

Thread::~Thread()
{
}

QString Thread::text() const
{
	QMutexLocker lock( &m_mutex ) ;
	return m_text ;
}

void Thread::run()
{
	G_DEBUG( "Thread::run" ) ;
	try
	{
		G::Process::ChildProcess child = G::Process::spawn( m_tool , m_args ) ;
		for(;;)
		{
			std::string line = child.read() ;
			G_DEBUG( "Thread::run: [" << G::Str::toPrintableAscii(line) << "]" ) ;
			if( line.empty() ) 
			{
				break ;
			}
			{
				QMutexLocker lock( &m_mutex ) ;
				m_text.append( line.c_str() ) ;
			}
			emit changeSignal() ;
		}
		m_rc = child.wait() ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "Thread::run: exception: " << e.what() ) ;
	}
	catch(...)
	{
		G_ERROR( "Thread::run: exception" ) ;
	}
	G_DEBUG( "Thread::run: done: " << m_rc ) ;
	emit doneSignal(m_rc) ;
}


