//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glog.cpp
//

#include "gdef.h"
#include "glog.h"
#include "glogoutput.h"

namespace G
{
	class LogImp ;
}

// Class: LogImp
// Description: An implementation class used by G::Log.
//
class G::LogImp 
{
public:
	static std::ostringstream &s() ;
	static bool active() ;
	static void empty() ;
	static const char *m_file ;
	static int m_line ;
	static std::ostringstream *m_ss ;
} ;

const char *G::LogImp::m_file = NULL ;
std::ostringstream *G::LogImp::m_ss = NULL ;
int G::LogImp::m_line = 0 ;

std::ostringstream & G::LogImp::s()
{
	if( m_ss == NULL )
		m_ss = new std::ostringstream ;
	return *m_ss ;
}

void G::LogImp::empty()
{
	delete m_ss ;
	m_ss = NULL ;
	m_ss = new std::ostringstream ;
}

bool G::LogImp::active()
{
	LogOutput * output = G::LogOutput::instance() ;
	if( output == NULL )
	{
		return false ;
	}
	else
	{
		bool a = output->enable(true) ;
		output->enable(a) ;
		return a ;
	}
}

// ===

G::Log::End G::Log::end( G::Log::Severity severity )
{
	return End(severity) ;
}

G::Log::Stream & G::Log::stream()
{
	return G::LogImp::s() ;
}

void G::Log::onEnd( G::Log::Severity severity )
{
	if( G::LogImp::active() )
	{
		G::LogOutput::output( severity , G::LogImp::m_file , G::LogImp::m_line , 
			G::LogImp::s().str().c_str() ) ;
	}

	G::LogImp::empty() ; // empty the stream
	G::LogImp::m_file = NULL ;
	G::LogImp::m_line = 0 ;
}

void G::Log::setFile( const char *file )
{
	G::LogImp::m_file = file ;
}

void G::Log::setLine( int line )
{
	G::LogImp::m_line = line ;
}

