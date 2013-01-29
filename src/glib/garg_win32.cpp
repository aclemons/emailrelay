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
// garg_win32.cpp
//

#include "gdef.h"
#include "garg.h"
#include "glimits.h"
#include "gstr.h"
#include "gdebug.h"

void G::Arg::setExe()
{
	if( m_array.empty() )
		m_array.push_back( moduleName(NULL) ) ;
	else
		m_array[0] = moduleName( NULL ) ;
}

std::string G::Arg::moduleName( HINSTANCE hinstance )
{
	char buffer[limits::path] = { 0 } ;
	DWORD size = limits::path ;
	size = ::GetModuleFileNameA( hinstance , buffer , size-1U ) ;
	return std::string( buffer , size ) ;
}

void G::Arg::parse( HINSTANCE hinstance , const std::string & command_line )
{
	m_array.clear() ;
	m_array.push_back( moduleName(hinstance) ) ;
	parseCore( command_line ) ;
	setPrefix() ;
}

void G::Arg::reparse( const std::string & command_line )
{
	while( m_array.size() > 1U ) m_array.pop_back() ;
	parseCore( command_line ) ;
}

void G::Arg::parseCore( const std::string & command_line )
{
	std::string s( command_line ) ;
	protect( s ) ;
	G::Str::splitIntoTokens( s , m_array , " " ) ;
	unprotect( m_array ) ;
	dequote( m_array ) ;
}

void G::Arg::protect( std::string & s )
{
	// replace all quoted spaces with a replacement
	// (could do better: escaped quotes, tabs, single quotes)
	G_DEBUG( "protect: before: " << Str::printable(s) ) ;
	bool in_quote = false ;
	const char quote = '"' ;
	const char space = ' ' ;
	const char replacement = '\0' ;
	for( std::string::size_type pos = 0U ; pos < s.length() ; pos++ )
	{
		if( s.at(pos) == quote ) in_quote = ! in_quote ;
		if( in_quote && s.at(pos) == space ) s[pos] = replacement ;
	}
	G_DEBUG( "protect: after: " << Str::printable(s) ) ;
}

void G::Arg::unprotect( StringArray & array )
{
	// restore replacements to spaces
	const char space = ' ' ;
	const char replacement = '\0' ;
	for( StringArray::iterator p = array.begin() ; p != array.end() ; ++p )
	{
		std::string & s = *p ;
		G::Str::replaceAll( s , std::string(1U,replacement) , std::string(1U,space) ) ;
	}
}

void G::Arg::dequote( StringArray & array )
{
	// remove quotes if first and last characters (or equivalent)
	char qq = '\"' ;
	for( StringArray::iterator p = array.begin() ; p != array.end() ; ++p )
	{
		std::string & s = *p ;
		if( s.length() > 1U )
		{
			std::string::size_type start = s.at(0U) == qq ? 0U : s.find("=\"") ;
			if( start != std::string::npos && s.at(start) != qq ) ++start ;
			std::string::size_type end = s.at(s.length()-1U) == qq ? (s.length()-1U) : std::string::npos ;
			if( start != std::string::npos && end != std::string::npos && start != end )
			{
				s.erase( end , 1U ) ; // first!
				s.erase( start , 1U ) ;
			}
		}
	}
}

/// \file garg_win32.cpp
