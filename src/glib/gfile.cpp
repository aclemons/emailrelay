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
/// \file gfile.cpp
///

#include "gdef.h"
#include "glimits.h"
#include "gfile.h"
#include "gprocess.h"
#include "glog.h"
#include <iostream>
#include <cstdio>

bool G::File::remove( const Path & path , std::nothrow_t ) noexcept
{
	int rc = std::remove( path.cstr() ) ;
	return rc == 0 ;
}

void G::File::remove( const Path & path )
{
	int rc = std::remove( path.cstr() ) ;
	int e = Process::errno_() ;
	if( rc != 0 )
	{
		G_WARNING( "G::File::remove: cannot delete file [" << path << "]: " << Process::strerror(e) ) ;
		throw CannotRemove( path.str() , Process::strerror(e) ) ;
	}
}

bool G::File::rename( const Path & from , const Path & to , std::nothrow_t ) noexcept
{
	return 0 == std::rename( from.cstr() , to.cstr() ) ;
}

void G::File::rename( const Path & from , const Path & to , bool ignore_missing )
{
	bool is_missing = false ;
	bool ok = rename( from.cstr() , to.cstr() , is_missing ) ;
	if( !ok && !(is_missing && ignore_missing) )
	{
		throw CannotRename( std::string() + "[" + from.str() + "] to [" + to.str() + "]" ) ;
	}
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << ok ) ;
}

bool G::File::rename( const char * from , const char * to , bool & enoent ) noexcept
{
	bool ok = 0 == std::rename( from , to ) ;
	int error = Process::errno_() ;
	enoent = ( !ok && error == ENOENT ) ;
	return ok ;
}

#ifndef G_LIB_SMALL
void G::File::copy( const Path & from , const Path & to )
{
	std::string reason = copy( from , to , 0 ) ;
	if( !reason.empty() )
		throw CannotCopy( std::string() + "[" + from.str() + "] to [" + to.str() + "]: " + reason ) ;
}
#endif

bool G::File::copy( const Path & from , const Path & to , std::nothrow_t )
{
	return copy(from,to,0).empty() ;
}

#ifndef G_LIB_SMALL
bool G::File::copyInto( const Path & from , const Path & to_dir , std::nothrow_t )
{
	G::Path to = to_dir + from.basename() ;
	bool ok = copy(from,to,0).empty() ;
	if( ok && isExecutable(from,std::nothrow) )
		ok = chmodx( to , std::nothrow ) ;
	return ok ;
}
#endif

std::string G::File::copy( const Path & from , const Path & to , int )
{
	std::ifstream in ; open( in , from ) ;
	if( !in.good() )
		return "cannot open input file" ;

	std::ofstream out ; open( out , to ) ;
	if( !out.good() )
		return "cannot open output file" ;

	out << in.rdbuf() ;

	if( in.fail() )
		return "read error" ;

	//bool empty = in.tellg() == std::streampos(0) ; // not uclibc++
	bool empty = false ;

	in.close() ;
	out.close() ;

	if( out.fail() && !empty )
		return "write error" ;

	return {} ;
}

void G::File::copy( std::istream & in , std::ostream & out , std::streamsize limit , std::size_t block )
{
	std::ios_base::iostate in_state = in.rdstate() ;

	block = block ? block : static_cast<std::string::size_type>(Limits<>::file_buffer) ;
	std::vector<char> buffer( block ) ;

	const auto b = static_cast<std::streamsize>(block) ;
	std::streamsize size = 0U ;
	while( ( limit == 0U || size < limit ) && in.good() && out.good() )
	{
		std::streamsize request = limit == 0U || (limit-size) > b ? b : (limit-size) ;
		in.read( &buffer[0] , request ) ;
		std::streamsize result = in.gcount() ;
		if( result == 0U )
			break ;
		out.write( &buffer[0] , result ) ;
		size += result ;
	}

	out.flush() ;

	// restore the input failbit because it might have been set by us reading an incomplete block at eof
	in.clear( (in.rdstate() & ~std::ios_base::failbit) | (in_state & std::ios_base::failbit) ) ;
}

bool G::File::exists( const Path & path )
{
	return path.empty() ? false : exists( path , false , true ) ;
}

bool G::File::exists( const Path & path , std::nothrow_t )
{
	return path.empty() ? false : exists( path , false , false ) ;
}

bool G::File::exists( const Path & path , bool error_return_value , bool do_throw )
{
	bool enoent = false ;
	bool eaccess = false ;
	bool rc = existsImp( path.cstr() , enoent , eaccess ) ; // o/s-specific
	if( !rc && enoent )
	{
		return false ;
	}
	else if( !rc && do_throw )
	{
		throw StatError( path.str() , eaccess?"permission denied":"" ) ;
	}
	else if( !rc )
	{
		return error_return_value ;
	}
	return true ;
}

bool G::File::isLink( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() , true ) ;
	return 0 == s.error && s.is_link ;
}

G::File::Stat G::File::stat( const Path & path , bool read_symlink )
{
	return statImp( path.cstr() , read_symlink ) ;
}

bool G::File::isDirectory( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() ) ;
	return 0 == s.error && s.is_dir ;
}

bool G::File::isExecutable( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() ) ;
	return 0 == s.error && s.is_executable ;
}

#ifndef G_LIB_SMALL
bool G::File::isEmpty( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() ) ;
	return 0 == s.error && s.is_empty ;
}
#endif

std::string G::File::sizeString( const Path & path )
{
	Stat s = statImp( path.cstr() ) ;
	return s.error ? std::string() : std::to_string(s.size) ;
}

G::SystemTime G::File::time( const Path & path )
{
	Stat s = statImp( path.cstr() ) ;
	if( s.error )
		throw TimeError( path.str() , Process::strerror(s.error) ) ;
	return SystemTime( s.mtime_s , s.mtime_us ) ;
}

#ifndef G_LIB_SMALL
G::SystemTime G::File::time( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() ) ;
	if( s.error )
		return SystemTime( 0 ) ;
	return SystemTime( s.mtime_s , s.mtime_us ) ;
}
#endif

bool G::File::chmodx( const Path & path , std::nothrow_t )
{
	return chmodx( path , false ) ;
}

#ifndef G_LIB_SMALL
void G::File::chmodx( const Path & path )
{
	chmodx( path , true ) ;
}
#endif

bool G::File::mkdir( const Path & dir , std::nothrow_t )
{
	return 0 == mkdirImp( dir ) ;
}

#ifndef G_LIB_SMALL
void G::File::mkdir( const Path & dir )
{
	int e = mkdirImp( dir ) ;
	if( e )
		throw CannotMkdir( dir.str() , Process::strerror(e) ) ;
}
#endif

bool G::File::mkdirsr( const Path & path , int & e , int & limit )
{
	// (recursive)

	if( exists(path) )
		return true ;

	if( path.str().empty() )
		return true ;

	bool ok = mkdirsr( path.dirname() , e , limit ) ; // (recursion)
	if( !ok )
		return false ;

	e = mkdirImp( path ) ;
	if( e == 0 && --limit < 0 )
	{
		e = ENOENT ; // sort of
		return false ;
	}

	return e == 0 ;
}

#ifndef G_LIB_SMALL
bool G::File::mkdirs( const Path & path , std::nothrow_t , int limit )
{
	int e = 0 ;
	return mkdirsr( path , e , limit ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::File::mkdirs( const Path & path , int limit )
{
	int e = 0 ;
	if( !mkdirsr(path,e,limit) && e != EEXIST )
		throw CannotMkdir( path.str() , e ? G::Process::strerror(e) : std::string() ) ;
}
#endif

#ifndef G_LIB_SMALL
int G::File::compare( const Path & path_1 , const Path & path_2 , bool ignore_whitespace )
{
	std::ifstream file_1 ; open( file_1 , path_1 ) ;
	std::ifstream file_2 ; open( file_2 , path_2 ) ;
	constexpr int eof = std::char_traits<char>::eof() ; // EOF
	if( !file_1.good() && !file_2.good() ) return -1 ;
	if( !file_1.good() ) return -1 ;
	if( !file_2.good() ) return 1 ;
	int result = 0 ;
	int a = eof ;
	int b = eof ;
	auto isspace = [](int c){ return c == ' ' || c == '\t' || c == '\n' || c == '\r' ; } ;
	for(;;)
	{
		do { a = file_1.get() ; } while( ignore_whitespace && isspace(a) ) ;
		do { b = file_2.get() ; } while( ignore_whitespace && isspace(b) ) ;
		if( a == eof && b == eof )
			break ;
		if( a != b )
		{
			result = a < b ? -1 : 1 ;
			break ;
		}
	}
	return result ;
}
#endif

