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
/// \file gpath.cpp
///

#include "gdef.h"
#include "gpath.h"
#include "gstr.h"
#include "gstringarray.h"
#include "gstringview.h"
#include "gassert.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()

namespace G
{
	namespace PathImp
	{
		enum class Platform { Unix , Windows } ;
		template <Platform> struct PathPlatform /// A class template specialised by o/s in the implementation of G::Path.
			{} ;
		template <> struct G::PathImp::PathPlatform<Platform::Unix> ;
		template <> struct G::PathImp::PathPlatform<Platform::Windows> ;
	}
}

template <>
struct G::PathImp::PathPlatform<G::PathImp::Platform::Windows>
{
	static string_view sep() noexcept
	{
		return { "\\" , 1U } ;
	}
	static std::size_t slashpos( const std::string & s ) noexcept
	{
		return s.rfind('\\') ;
	}
	static bool simple( const std::string & s ) noexcept
	{
		return s.find('/') == std::string::npos && s.find('\\') == std::string::npos ;
	}
	static bool isdrive( const std::string & s ) noexcept
	{
		return s.length() == 2U && s[1] == ':' ;
	}
	static bool absolute( const std::string & s ) noexcept
	{
		// TODO use rootsize()
		return
			( s.length() >= 3U && s[1] == ':' && s[2] == '\\' ) ||
			( s.length() >= 1U && s[0] == '\\' ) ;
	}
	static std::size_t rootsizeImp( const std::string & s , std::size_t chars , std::size_t parts ) noexcept
	{
		G_ASSERT( s.length() >= chars ) ;
		G_ASSERT( parts == 1U || parts == 2U ) ;
		std::size_t pos = s.find( '\\' , chars ) ;
		if( parts == 2U && pos != std::string::npos )
			pos = s.find( '\\' , pos+1U ) ;
		return pos == std::string::npos ? s.length() : pos ;
	}
	static std::size_t rootsize( const std::string & s ) noexcept
	{
		if( s.empty() )
			return 0U ;
		if( s.length() >= 3U && s.at(1U) == ':' && s.at(2U) == '\\' )
			return 3U ; // C:|...
		if( s.length() >= 2U && s.at(1U) == ':' )
			return 2U ; // C:...
		if( s.find(R"(\\?\UNC\)",0U,8U) == 0U )
			return rootsizeImp(s,8U,2U) ; // ||?|UNC|server|volume|...
		if( s.find(R"(\\?\)",0U,4U) == 0U && s.size() > 5U && s.at(5U) == ':' )
			return rootsizeImp(s,4U,1U) ; // ||?|C:|...
		if( s.find(R"(\\?\)",0U,4U) == 0U )
			return rootsizeImp(s,4U,2U) ; // ||?|server|volume|...
		if( s.find(R"(\\.\)",0U,4U) == 0U )
			return rootsizeImp(s,4U,1U) ; // ||.|dev|...
		if( s.find("\\\\",0U,2U) == 0U )
			return rootsizeImp(s,2U,2U) ; // ||server|volume|...
		if( s.find('\\') == 0U )
			return 1U ; // |...
		return 0U ;
	}
	static void normalise( std::string & s )
	{
		Str::replace( s , '/' , '\\' ) ;
		bool special = s.find("\\\\",0U,2U) == 0U ;
		while( Str::replaceAll( s , "\\\\"_sv , "\\"_sv ) ) {;}
		if( special ) s.insert( 0U , 1U , '\\' ) ;

		while( s.length() > 1U )
		{
			std::size_t pos = s.rfind('\\') ;
			if( pos == std::string::npos ) break ;
			if( (pos+1U) != s.length() ) break ;
			if( pos < rootsize(s) ) break ;
			s.resize( pos ) ;
		}
	}
	static std::string null()
	{
		return "NUL" ;
	}
} ;

template <>
struct G::PathImp::PathPlatform<G::PathImp::Platform::Unix> /// A unix specialisation of G::PathImp::PathPlatform used by G::Path.
{
	static string_view sep() noexcept
	{
		return { "/" , 1U } ;
	}
	static std::size_t slashpos( const std::string & s ) noexcept
	{
		return s.rfind('/') ;
	}
	static bool simple( const std::string & s ) noexcept
	{
		return s.find('/') == std::string::npos ;
	}
	static bool isdrive( const std::string & ) noexcept
	{
		return false ;
	}
	static void normalise( std::string & s )
	{
		while( Str::replaceAll( s , "//"_sv , "/"_sv ) ) {;}
		while( s.length() > 1U && s.at(s.length()-1U) == '/' ) s.resize(s.length()-1U) ;
	}
	static bool absolute( const std::string & s ) noexcept
	{
		return !s.empty() && s[0] == '/' ;
	}
	static std::size_t rootsize( const std::string & s ) noexcept
	{
		return s.empty() || s[0] != '/' ? 0U : 1U ;
	}
	static std::string null()
	{
		return "/dev/null" ;
	}
} ;

namespace G
{
	namespace PathImp
	{
		static bool use_posix = !G::is_windows() ; // gdef.h // NOLINT bogus cert-err58-cpp
		using U = PathPlatform<Platform::Unix> ;
		using W = PathPlatform<Platform::Windows> ;
		string_view sep() { return use_posix ? U::sep() : W::sep() ; }
		void normalise( std::string & s ) { use_posix ? U::normalise(s) : W::normalise(s) ; }
		bool simple( const std::string & s ) { return use_posix ? U::simple(s) : W::simple(s) ; }
		bool isdrive( const std::string & s ) { return use_posix ? U::isdrive(s) : W::isdrive(s) ; }
		bool absolute( const std::string & s ) { return use_posix ? U::absolute(s) : W::absolute(s); }
		std::string null() { return use_posix ? U::null() : W::null() ; }
		std::size_t rootsize( const std::string & s ) { return use_posix ? U::rootsize(s) : W::rootsize(s) ; }
		std::size_t slashpos( const std::string & s ) { return use_posix ? U::slashpos(s) : W::slashpos(s) ; }
	}
}

namespace G
{
	namespace PathImp
	{
		std::size_t dotpos( const std::string & s ) noexcept
		{
			const std::size_t npos = std::string::npos ;
			const std::size_t sp = slashpos( s ) ;
			const std::size_t dp = s.rfind( '.' ) ;
			if( dp == npos )
				return npos ;
			else if( sp == npos )
				return dp ;
			else if( dp < sp )
				return npos ;
			else
				return dp ;
		}

		void splitInto( const std::string & str , StringArray & a )
		{
			std::size_t rs = rootsize( str ) ;
			if( str.empty() )
			{
			}
			else if( rs != 0U ) // ie. absolute or like "c:foo"
			{
				std::string root = str.substr( 0U , rs ) ;
				Str::splitIntoTokens( Str::tail(str,rs-1U,std::string()) , a , sep() ) ;
				a.insert( a.begin() , root ) ;
			}
			else
			{
				Str::splitIntoTokens( str , a , sep() ) ;
			}
		}

		bool purge( StringArray & a )
		{
			const std::string dot( 1U , '.' ) ;
			a.erase( std::remove( a.begin() , a.end() , std::string() ) , a.end() ) ;
			std::size_t n = a.size() ;
			a.erase( std::remove( a.begin() , a.end() , dot ) , a.end() ) ;
			const bool all_dots = a.empty() && n != 0U ;
			return all_dots ;
		}

		std::string join( StringArray::const_iterator p , StringArray::const_iterator end )
		{
			std::string str ;
			int i = 0 ;
			for( ; p != end ; ++p , i++ )
			{
				bool drive = isdrive( str ) ;
				bool last_is_slash = !str.empty() &&
					( str.at(str.length()-1U) == '/' || str.at(str.length()-1U) == '\\' ) ;
				if( i == 1 && (drive || last_is_slash) )
					;
				else if( i != 0 )
					str.append( sep().data() , sep().size() ) ;
				str.append( *p ) ;
			}
			return str ;
		}

		std::string join( const StringArray & a )
		{
			return join( a.begin() , a.end() ) ;
		}
	}
}

// ==

#ifndef G_LIB_SMALL
void G::Path::setPosixStyle()
{
	PathImp::use_posix = true ;
}
#endif

#ifndef G_LIB_SMALL
void G::Path::setWindowsStyle()
{
	PathImp::use_posix = false ;
}
#endif

G::Path::Path()
= default;

G::Path::Path( const std::string & path ) :
	m_str(path)
{
	PathImp::normalise( m_str ) ;
}

G::Path::Path( const char * path ) :
	m_str(path)
{
	PathImp::normalise( m_str ) ;
}

G::Path::Path( string_view path ) :
	m_str(sv_to_string(path))
{
	PathImp::normalise( m_str ) ;
}

G::Path::Path( const Path & path , const std::string & tail ) :
	m_str(path.m_str)
{
	pathAppend( tail ) ;
	PathImp::normalise( m_str ) ;
}

#ifndef G_LIB_SMALL
G::Path::Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 ) :
	m_str(path.m_str)
{
	pathAppend( tail_1 ) ;
	pathAppend( tail_2 ) ;
	PathImp::normalise( m_str ) ;
}
#endif

#ifndef G_LIB_SMALL
G::Path::Path( const Path & path , const std::string & tail_1 , const std::string & tail_2 ,
	const std::string & tail_3 ) :
		m_str(path.m_str)
{
	pathAppend( tail_1 ) ;
	pathAppend( tail_2 ) ;
	pathAppend( tail_3 ) ;
	PathImp::normalise( m_str ) ;
}
#endif

G::Path::Path( std::initializer_list<std::string> args )
{
	if( args.size() )
	{
		m_str = *args.begin() ;
		for( const auto * p = args.begin()+1 ; p != args.end() ; ++p )
			pathAppend( *p ) ;
		PathImp::normalise( m_str ) ;
	}
}

G::Path G::Path::nullDevice()
{
	return { PathImp::null() } ;
}

bool G::Path::simple() const
{
	return dirname().empty() ;
}

bool G::Path::isAbsolute() const noexcept
{
	return PathImp::absolute( m_str ) ;
}

bool G::Path::isRelative() const noexcept
{
	return !isAbsolute() ;
}

std::string G::Path::basename() const
{
	StringArray a ;
	PathImp::splitInto( m_str , a ) ;
	PathImp::purge( a ) ;
	return a.empty() ? std::string() : a.at(a.size()-1U) ;
}

G::Path G::Path::dirname() const
{
	StringArray a ;
	PathImp::splitInto( m_str , a ) ;
	PathImp::purge( a ) ;
	if( a.empty() ) return {} ;
	a.pop_back() ;
	return join( a ) ;
}

G::Path G::Path::withoutExtension() const
{
	std::string::size_type sp = PathImp::slashpos( m_str ) ;
	std::string::size_type dp = PathImp::dotpos( m_str ) ;
	if( dp != std::string::npos )
	{
		std::string result = m_str ;
		result.resize( dp ) ;
		if( (sp == std::string::npos && dp == 0U) || ((sp+1U) == dp) )
			result.append(".") ; // special case
		return { result } ;
	}
	else
	{
		return *this ;
	}
}

G::Path G::Path::withExtension( const std::string & ext ) const
{
	std::string result = m_str ;
	std::string::size_type dp = PathImp::dotpos( m_str ) ;
	if( dp != std::string::npos )
		result.resize( dp ) ;
	result.append( 1U , '.' ) ;
	result.append( ext ) ;
	return { result } ;
}

#ifndef G_LIB_SMALL
G::Path G::Path::withoutRoot() const
{
	if( isAbsolute() )
	{
		StringArray a ;
		PathImp::splitInto( m_str , a ) ;
		G_ASSERT( !a.empty() ) ;
		a.erase( a.begin() ) ;
		return a.empty() ? Path("."_sv) : join( a ) ;
	}
	else
	{
		return *this ;
	}
}
#endif

G::Path & G::Path::pathAppend( const std::string & tail )
{
	if( tail.empty() )
	{
	}
	else if( PathImp::simple(tail) )
	{
		m_str.append( sv_to_string(PathImp::sep()) + tail ) ;
	}
	else
	{
		Path result = join( *this , tail ) ;
		result.swap( *this ) ;
	}
	return *this ;
}

std::string G::Path::extension() const
{
	std::string::size_type pos = PathImp::dotpos(m_str) ;
	return
		pos == std::string::npos || (pos+1U) == m_str.length() ?
			std::string() :
			m_str.substr( pos+1U ) ;
}

G::StringArray G::Path::split() const
{
	StringArray a ;
	PathImp::splitInto( m_str , a ) ;
	if( PathImp::purge(a) ) a.emplace_back( "." ) ;
	return a ;
}

G::Path G::Path::join( const G::StringArray & a )
{
	if( a.empty() ) return {} ;
	return { PathImp::join(a) } ;
}

G::Path G::Path::join( const G::Path & p1 , const G::Path & p2 )
{
	if( p1.empty() )
	{
		return p2 ;
	}
	else if( p2.empty() )
	{
		return p1 ;
	}
	else
	{
		StringArray a1 = p1.split() ;
		StringArray a2 = p2.split() ;
		a2.insert( a2.begin() , a1.begin() , a1.end() ) ;
		return join( a2 ) ;
	}
}

G::Path G::Path::collapsed() const
{
	const std::string dots = ".." ;

	StringArray a = split() ;
	auto start = a.begin() ;
	auto end = a.end() ;
	if( start != end && isAbsolute() )
		++start ;

	while( start != end )
	{
		// step over leading dots -- cannot collapse
		while( start != end && *start == dots )
			++start ;

		// find collapsable dots
		auto p_dots = std::find( start , end , dots ) ;
		if( p_dots == end )
			break ; // no collapsable dots remaining

		G_ASSERT( p_dots != a.begin() ) ;
		G_ASSERT( a.size() >= 2U ) ;

		// remove the preceding element and then the dots
		bool at_start = std::next(start) == p_dots ;
		auto p = a.erase( a.erase(--p_dots) ) ;

		// re-initialise where invalidated
		end = a.end() ;
		if( at_start )
			start = p ;
	}

	return join( a ) ;
}

bool G::Path::operator==( const Path & other ) const
{
	return m_str == other.m_str ; // noexcept only in c++14
}

bool G::Path::operator!=( const Path & other ) const
{
	return m_str != other.m_str ; // noexcept only in c++14
}

void G::Path::swap( Path & other ) noexcept
{
	using std::swap ;
	swap( m_str , other.m_str ) ;
}

#ifndef G_LIB_SMALL
bool G::Path::less( const G::Path & a , const G::Path & b )
{
	StringArray a_parts = a.split() ;
	StringArray b_parts = b.split() ;
	return std::lexicographical_compare(
		a_parts.begin() , a_parts.end() ,
		b_parts.begin() , b_parts.end() ,
		[](const std::string & a_,const std::string & b_){return a_.compare(b_) < 0;} ) ;
}
#endif

#ifndef G_LIB_SMALL
G::Path G::Path::difference( const G::Path & root_in , const G::Path & path_in )
{
	StringArray path_parts ;
	StringArray root_parts ;
	if( !root_in.empty() ) root_parts = root_in.collapsed().split() ;
	if( !path_in.empty() ) path_parts = path_in.collapsed().split() ;
	if( root_parts.size() == 1U && root_parts.at(0U) == "." ) root_parts.clear() ;
	if( path_parts.size() == 1U && path_parts.at(0U) == "." ) path_parts.clear() ;

	if( path_parts.size() < root_parts.size() )
		return {} ;

	auto p = std::mismatch( root_parts.begin() , root_parts.end() , path_parts.begin() ) ;

	if( p.first == root_parts.end() && p.second == path_parts.end() )
		return { "." } ;
	else if( p.first != root_parts.end() )
		return {} ;
	else
		return { PathImp::join(p.second,path_parts.end()) } ;
}
#endif

