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
//
// commandline_simple.cpp
//
// This implementation reads a configuration file specified using a
// single command-line parameter. The file contains the long form
// of the documented command-line switches without the double-dash
// and using equals where necessary.
//
// eg.
//    $ ( echo port=2525 ; echo user=root ; echo log ; echo verbose ) > emailrelay.cfg
//    $ ./emailrelay emailrelay.cfg
//
// The motivation for this implementation is to reduce the size
// of the application binary, so a lot of sanity checking is
// left out -- caveat configurator.
//

#include "gdef.h"
#include "gsmtp.h"
#include "configuration.h"
#include "commandline.h"
#include "gstr.h"
#include <fstream>
#include <map>
#include <stdexcept>

class Main::CommandLineImp 
{
public:
	std::map<std::string,std::string> m ;
} ;

// ==

std::string Main::CommandLine::switchSpec( bool )
{
	return std::string() ;
}

Main::CommandLine::CommandLine( Main::Output & , const G::Arg & arg , const std::string & , 
	const std::string & , const std::string & ) :
		m_imp(NULL)
{
	bool ok = false ;
	if( arg.c() == 2U )
	{
		std::string path = arg.v(1U) ;
		std::ifstream f( path.c_str() ) ;
		ok = f.good() ;
		std::string key_value ;
		while( f )
		{
			if( m_imp == NULL ) m_imp = new CommandLineImp ;
			f >> key_value ;
			std::string key = key_value ;
			std::string value ;
			std::string::size_type pos = key_value.find('=') ;
			if( pos != std::string::npos && pos != 0U )
			{
				key = key_value.substr(0U,pos) ;
				value = G::Str::tail( key_value , pos ) ;
			}
			m_imp->m[key] = value ;
		}
	}
	if( !ok )
		throw std::runtime_error( "usage error (usage modified at configure-time, so not as documented)" ) ;
}

Main::CommandLine::~CommandLine()
{
	delete m_imp ;
}

Main::Configuration Main::CommandLine::cfg() const
{
	return Configuration( *this ) ;
}

std::string Main::CommandLine::value( const char * switch_ ) const
{
	return value( std::string(switch_) ) ;
}

unsigned int Main::CommandLine::value( const char * switch_ , unsigned int default_ ) const
{
	return value( std::string(switch_) , default_ ) ;
}

G::Strings Main::CommandLine::value( const char * switch_ , const char * separators ) const
{
	return value( std::string(switch_) , std::string(separators) ) ;
}

bool Main::CommandLine::contains( const std::string & s ) const
{
	return m_imp && m_imp->m.find(s) != m_imp->m.end() ;
}

bool Main::CommandLine::contains( const char * s ) const
{
	return contains( std::string(s) ) ;
}

std::string Main::CommandLine::value( const std::string & s ) const
{
	return contains(s) ? m_imp->m[s] : std::string() ;
}

unsigned int Main::CommandLine::value( const std::string & s , unsigned int default_ ) const
{
	return contains(s) ? G::Str::toUInt(value(s)) : default_ ;
}

G::Strings Main::CommandLine::value( const std::string & s , const std::string & sep ) const
{
	G::Strings result ;
	if( contains(s) )
		G::Str::splitIntoFields( value(s) , result , sep ) ;
	return result ;
}

G::Arg::size_type Main::CommandLine::argc() const
{
	return 1U ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return false ;
}

bool Main::CommandLine::hasSemanticError() const
{
	return false ;
}

void Main::CommandLine::showHelp( bool ) const
{
}

void Main::CommandLine::showUsageErrors( bool ) const
{
}

void Main::CommandLine::showSemanticError( bool ) const
{
}

void Main::CommandLine::logSemanticWarnings() const
{
}

void Main::CommandLine::showArgcError( bool ) const
{
}

void Main::CommandLine::showNoop( bool ) const
{
}

void Main::CommandLine::showError( const std::string & , bool ) const
{
}

void Main::CommandLine::showVersion( bool ) const
{
}

void Main::CommandLine::showBanner( bool , const std::string & ) const
{
}

void Main::CommandLine::showCopyright( bool , const std::string & ) const
{
}

/// \file commandline_simple.cpp
