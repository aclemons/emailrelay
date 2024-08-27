//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file garg.cpp
///

#include "gdef.h"
#include "garg.h"
#include "gprocess.h"
#include "gscope.h"
#include "gpath.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <cstring>

G::Path G::Arg::m_v0 ;
G::Path G::Arg::m_cwd ;

G::Arg::Arg( int argc , char **argv )
{
	G_ASSERT( argc > 0 ) ;
	G_ASSERT( argv != nullptr ) ;

	for( int i = 0 ; i < argc ; i++ )
		m_array.emplace_back( argv[i] ) ;

	if( m_v0.empty() && !m_array.empty() )
	{
		m_v0 = Path( m_array[0] ) ;
		m_cwd = Process::cwd( std::nothrow ) ; // don't throw yet - we may "cd /" to deamonise
	}
}

G::Arg::Arg( const StringArray & args ) :
	m_array(args)
{
}

#ifndef G_LIB_SMALL
G::Arg::Arg( const Path & argv0 , const std::string & command_line_tail )
{
	if( argv0.empty() )
		throw Exception( "invalid path for this executable" ) ;
	parseImp( command_line_tail ) ;
	m_array.insert( m_array.begin() , argv0.str() ) ;
}
#endif

G::Arg::Arg( const std::string & command_line )
{
	G_ASSERT( !command_line.empty() ) ;
	parseImp( command_line ) ;
}

G::Arg G::Arg::windows()
{
	Arg arg( (Windows()) ) ;
	if( m_v0.empty() && !arg.m_array.empty() )
	{
		m_v0 = Path( arg.m_array[0] ) ;
		m_cwd = Process::cwd( std::nothrow ) ;
	}
	return arg ;
}

#ifndef G_LIB_SMALL
G::Path G::Arg::v0()
{
	return m_v0 ;
}
#endif

G::StringArray G::Arg::array( unsigned int shift ) const
{
	StringArray result = m_array ;
	while( !result.empty() && shift-- )
		result.erase( result.begin() ) ;
	return result ;
}

bool G::Arg::contains( std::string_view option , std::size_t option_args , bool cs ) const
{
	return find( cs , option , option_args , nullptr ) != 0U ;
}

#ifndef G_LIB_SMALL
std::size_t G::Arg::count( std::string_view option ) const
{
	return find( true , option , 0U , nullptr ) ;
}
#endif

std::size_t G::Arg::find( bool cs , std::string_view option , std::size_t option_args ,
	std::size_t * index_p ) const
{
	std::size_t count = 0U ;
	for( std::size_t i = 1U ; i < m_array.size() ; i++ ) // start from v[1]
	{
		if( strmatch(cs,option,m_array[i]) && (i+option_args) < m_array.size() )
		{
			count++ ;
			if( index_p != nullptr )
				*index_p = i ;
			i += option_args ;
		}
	}
	return count ;
}

std::size_t G::Arg::match( const std::string & prefix ) const
{
	for( std::size_t i = 1U ; i < m_array.size() ; i++ )
	{
		if( Str::headMatch(m_array[i],prefix) )
		{
			return i ;
		}
	}
	return 0U ;
}

bool G::Arg::strmatch( bool cs , std::string_view s1 , std::string_view s2 )
{
	return cs ? (s1==s2) : Str::imatch(s1,s2) ;
}

bool G::Arg::remove( std::string_view option , std::size_t option_args )
{
	std::size_t i = 0U ;
	const bool found = find( true , option , option_args , &i ) != 0U ;
	if( found )
		removeAt( i , option_args ) ;
	return found ;
}

std::string G::Arg::removeValue( std::string_view option , const std::string & default_ )
{
	std::size_t option_args = 1U ;
	std::size_t i = 0U ;
	const bool found = find( true , option , option_args , &i ) != 0U ;
	return found ? removeAt( i , option_args ) : default_ ;
}

std::string G::Arg::removeAt( std::size_t option_index , std::size_t option_args )
{
	std::string value ;
	if( option_index > 0U && (option_index+option_args) < m_array.size() )
	{
		value = v( option_index + (option_args?1U:0U) , std::string() ) ;
		auto p = m_array.begin() ;
		for( std::size_t i = 0U ; i < option_index ; i++ ) ++p ; // (rather than cast)
		p = m_array.erase( p ) ;
		for( std::size_t i = 0U ; i < option_args && p != m_array.end() ; i++ )
			p = m_array.erase( p ) ;
	}
	return value ;
}

std::size_t G::Arg::index( std::string_view option , std::size_t option_args ,
	std::size_t default_ ) const
{
	std::size_t i = 0U ;
	const bool found = find( true , option , option_args , &i ) != 0U ;
	return found ? i : default_ ;
}

std::size_t G::Arg::c() const
{
	return m_array.size() ;
}

std::string G::Arg::v( std::size_t i ) const
{
	G_ASSERT( i < m_array.size() ) ;
	return m_array.at(i) ;
}

std::string G::Arg::v( std::size_t i , const std::string & default_ ) const
{
	return i < m_array.size() ? m_array.at(i) : default_ ;
}

std::string G::Arg::prefix() const
{
	G_ASSERT( !m_array.empty() ) ;
	Path path( m_array.at(0U) ) ;
	return path.withoutExtension().basename() ;
}

const char * G::Arg::prefix( char ** argv ) noexcept
{
	if( argv == nullptr ) return "" ;
	const char * exe = argv[0] ;
	if( exe == nullptr || exe[0] == '\0' ) return "" ;
	const char * p1 = std::strrchr( exe , '/' ) ;
	const char * p2 = std::strrchr( exe , '\\' ) ;
	p1 = p1 == nullptr ? exe : (p1+1U) ;
	p2 = p2 == nullptr ? exe : (p2+1U) ;
	return p1 > p2 ? p1 : p2 ;
}

void G::Arg::parseImp( const std::string & command_line )
{
	constexpr std::string_view ws( " \t" , 2U ) ;
	constexpr std::string_view nbws( "\0\0" , 2U ) ;
	static_assert( ws.size() == nbws.size() , "" ) ;
	const char esc = '\\' ;
	const char qq = '\"' ;
	Str::splitIntoTokens( Str::dequote(command_line,qq,esc,ws,nbws) , m_array , ws , esc ) ;
	Str::replace( m_array , '\0' , ' ' ) ;
}

G::Path G::Arg::exe()
{
	return exeImp( true ) ;
}

#ifndef G_LIB_SMALL
G::Path G::Arg::exe( std::nothrow_t )
{
	return exeImp( false ) ;
}
#endif

G::Path G::Arg::exeImp( bool do_throw )
{
	Path process_exe = Process::exe() ;
	if( !process_exe.empty() )
	{
		return process_exe ;
	}
	else if( m_v0.isAbsolute() )
	{
		return m_v0 ;
	}
	else if( !m_cwd.empty() )
	{
		return Path::join(m_cwd,m_v0).collapsed() ;
	}
	else if( do_throw )
	{
		throw Exception( "cannot determine the absolute path of the current executable" ,
			G::is_windows() ? "" : "try mounting procfs" ) ;
	}
	else
	{
		return {} ;
	}
}

#ifndef G_LIB_SMALL
G::StringArray::const_iterator G::Arg::cbegin() const
{
	return m_array.size() <= 1U ? cend() : std::next( m_array.cbegin() ) ;
}
#endif

G::StringArray::const_iterator G::Arg::cend() const
{
	return m_array.cend() ;
}

