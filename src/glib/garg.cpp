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
// garg.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gprocess.h"
#include "gpath.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <cstring>

bool G::Arg::m_first = true ;
std::string G::Arg::m_v0 ;
std::string G::Arg::m_cwd ;

G::Arg::Arg( int argc , char *argv[] )
{
	G_ASSERT( argc > 0 ) ;
	G_ASSERT( argv != nullptr ) ;
	for( int i = 0 ; i < argc ; i++ )
		m_array.push_back( argv[i] ) ;

	if( m_first )
	{
		m_v0 = std::string( argv[0] ) ;
		m_cwd = Process::cwd(true/*nothrow*/) ; // don't throw yet - we may "cd /" to deamonise
		m_first = false ;
	}
}

G::Arg::Arg( const StringArray & args ) :
	m_array(args)
{
}

G::Arg::Arg()
{
	// now use parse()
}

void G::Arg::parse( HINSTANCE , const std::string & command_line_tail )
{
	std::string proc_exe = Process::exe() ;
	if( proc_exe.empty() ) throw Exception( "cannot determine the path of this executable" ) ;
	m_array.clear() ;
	m_array.push_back( proc_exe ) ;
	parseCore( command_line_tail ) ;
}

void G::Arg::parse( const std::string & command_line )
{
	G_ASSERT( !command_line.empty() ) ;
	m_array.clear() ;
	parseCore( command_line ) ;
}

void G::Arg::reparse( const std::string & command_line_tail )
{
	while( m_array.size() > 1U ) m_array.pop_back() ;
	parseCore( command_line_tail ) ;
}

std::string G::Arg::v0()
{
	return m_v0 ;
}

G::StringArray G::Arg::array( unsigned int shift ) const
{
	StringArray result = m_array ;
	while( !result.empty() && shift-- )
		result.erase( result.begin() ) ;
	return result ;
}

bool G::Arg::contains( const std::string & option , size_type option_args , bool cs ) const
{
	return find( cs , option , option_args , nullptr ) ;
}

bool G::Arg::find( bool cs , const std::string & option , size_type option_args , size_type * index_p ) const
{
	for( size_type i = 1 ; i < m_array.size() ; i++ ) // start from v[1]
	{
		if( match(cs,option,m_array[i]) && (i+option_args) < m_array.size() )
		{
			if( index_p != nullptr )
				*index_p = i ;
			return true ;
		}
	}
	return false ;
}

bool G::Arg::match( bool cs , const std::string & s1 , const std::string & s2 )
{
	return cs ? (s1==s2) : (Str::upper(s1)==Str::upper(s2)) ;
}

bool G::Arg::remove( const std::string & option , size_type option_args )
{
	size_type i = 0U ;
	const bool found = find( true , option , option_args , &i ) ;
	if( found )
		removeAt( i , option_args ) ;
	return found ;
}

void G::Arg::removeAt( size_type option_index , size_type option_args )
{
	G_ASSERT( option_index > 0U && option_index < m_array.size() ) ;
	if( option_index > 0U && option_index < m_array.size() )
	{
		StringArray::iterator p = m_array.begin() ;
		for( size_type i = 0U ; i < option_index ; i++ ) ++p ; // (rather than cast)
		p = m_array.erase( p ) ;
		for( size_type i = 0U ; i < option_args && p != m_array.end() ; i++ )
			p = m_array.erase( p ) ;
	}
}

G::Arg::size_type G::Arg::index( const std::string & option , size_type option_args ) const
{
	size_type i = 0U ;
	const bool found = find( true , option , option_args , &i ) ;
	return found ? i : 0U ;
}

G::Arg::size_type G::Arg::c() const
{
	return m_array.size() ;
}

std::string G::Arg::v( size_type i ) const
{
	G_ASSERT( i < m_array.size() ) ;
	return m_array.at(i) ;
}

std::string G::Arg::prefix() const
{
	G_ASSERT( m_array.size() > 0U ) ;
	Path path( m_array.at(0U) ) ;
	return path.withoutExtension().basename() ;
}

const char * G::Arg::prefix( char * argv [] ) // noexcept
{
	const char * exe = argv[0] ;
	const char * p1 = std::strrchr( exe , '/' ) ;
	const char * p2 = std::strrchr( exe , '\\' ) ;
	p1 = p1 ? (p1+1U) : exe ;
	p2 = p2 ? (p2+1U) : exe ;
	return p1 > p2 ? p1 : p2 ;
}

void G::Arg::parseCore( const std::string & command_line )
{
	std::string s( command_line ) ;
	protect( s ) ;
	Str::splitIntoTokens( s , m_array , " " ) ;
	unprotect( m_array ) ;
	dequote( m_array ) ;
}

void G::Arg::protect( std::string & s )
{
	// replace all quoted spaces with a replacement -- this is not
	// great because it does not allow for escaped quotes etc, but
	// the relevant operating system's command-line parsing is
	// appallingly bad to start with
	bool in_quote = false ;
	const char quote = '"' ;
	const char space = ' ' ;
	const char replacement = '\0' ;
	for( std::string::size_type pos = 0U ; pos < s.length() ; pos++ )
	{
		if( s.at(pos) == quote ) in_quote = ! in_quote ;
		if( in_quote && s.at(pos) == space ) s[pos] = replacement ;
	}
}

void G::Arg::unprotect( StringArray & array )
{
	// restore replacements to spaces
	const char space = ' ' ;
	const char replacement = '\0' ;
	for( StringArray::iterator p = array.begin() ; p != array.end() ; ++p )
	{
		std::string & s = *p ;
		Str::replaceAll( s , std::string(1U,replacement) , std::string(1U,space) ) ;
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

std::string G::Arg::exe( bool do_throw )
{
	std::string proc_exe = Process::exe() ;
	if( proc_exe.empty() && ( m_v0.empty() || ( m_cwd.empty() && Path(m_v0).isRelative() ) ) )
	{
		if( do_throw )
		{
			throw Exception( "cannot determine the absolute path of the current executable" ,
				G::is_windows() ? "" : "try mounting procfs" ) ;
		}
		return std::string() ;
	}
	else if( proc_exe.empty() && Path(m_v0).isRelative() )
	{
		return Path::join(m_cwd,m_v0).collapsed().str() ;
	}
	else if( proc_exe.empty() )
	{
		return m_v0 ;
	}
	else
	{
		return proc_exe ;
	}
}

/// \file garg.cpp
