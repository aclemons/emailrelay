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
/// \file gfile.cpp
///

#include "gdef.h"
#include "glimits.h"
#include "gfile.h"
#include "gprocess.h"
#include "gdate.h"
#include "gtime.h"
#include "gdatetime.h"
#include "glog.h"
#include <iostream>
#include <cstdio>

bool G::File::renameImp( const char * from , const char * to , int * e ) noexcept
{
	bool ok = from && to && 0 == std::rename( from , to ) ;
	if( e )
		*e = ok ? 0 : ( (from && to) ? Process::errno_() : EINVAL ) ;
	return ok ;
}

bool G::File::rename( const Path & from , const Path & to , std::nothrow_t ) noexcept
{
	static_assert( noexcept(from.cstr()) , "" ) ;
	return renameImp( from.cstr() , to.cstr() , nullptr ) ;
}

void G::File::rename( const Path & from , const Path & to , bool ignore_missing )
{
	int e = 0 ;
	bool ok = renameImp( from.cstr() , to.cstr() , &e ) ;
	bool is_missing = !ok && e == ENOENT ;
	if( !ok && !(is_missing && ignore_missing) )
	{
		throw CannotRename( std::string() + "[" + from.str() + "] to [" + to.str() + "]" ) ;
	}
	G_DEBUG( "G::File::rename: \"" << from << "\" -> \"" << to << "\": success=" << ok ) ;
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
	G::Path to = to_dir / from.basename() ;
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
		in.read( buffer.data() , request ) ;
		std::streamsize result = in.gcount() ;
		if( result == 0U )
			break ;
		out.write( buffer.data() , result ) ;
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

#ifndef G_LIB_SMALL
bool G::File::isLink( const Path & path , std::nothrow_t )
{
	Stat s = statImp( path.cstr() , /*symlink_nofollow=*/true ) ;
	return 0 == s.error && s.is_link ;
}
#endif

G::File::Stat G::File::stat( const Path & path , bool symlink_nofollow )
{
	return statImp( path.cstr() , symlink_nofollow ) ;
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

bool G::File::mkdirsImp( const Path & path_in , int & e , int limit )
{
	if( path_in.empty() )
		return true ;

	auto parts = path_in.split() ;
	Path path ;
	for( const auto & part : parts )
	{
		path.pathAppend( part ) ;
		if( path.isRoot() )
			continue ;
		e = mkdirImp( path ) ;
		if( e == EEXIST )
			continue ;
		else if( e )
			break ;
		if( --limit <= 0 )
		{
			e = E2BIG ;
			break ;
		}
	}
	return e == 0 || e == EEXIST ;
}

#ifndef G_LIB_SMALL
bool G::File::mkdirs( const Path & path , std::nothrow_t , int limit )
{
	int e = 0 ;
	return mkdirsImp( path , e , limit ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::File::mkdirs( const Path & path , int limit )
{
	int e = 0 ;
	if( !mkdirsImp(path,e,limit) )
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

#ifndef G_LIB_SMALL
G::Path G::File::backup( const Path & path , std::nothrow_t )
{
	constexpr char prefix = G::is_windows() ? '~' : '.' ;
	constexpr char sep = '~' ;
	constexpr unsigned int limit = 100U ;
	Path backup_path ;
	for( unsigned int version = 1U  ; version <= limit ; version++ )
	{
		backup_path = path.dirname() /
			std::string(1U,prefix).append(path.basename()).append(1U,sep).append(std::to_string(version==limit?1:version)) ;
		if( !exists( backup_path , std::nothrow ) || version == limit )
			break ;
	}
	Process::Umask umask( Process::Umask::Mode::Tightest ) ;
	bool copied = File::copy( path , backup_path , std::nothrow ) ;
	return copied ? backup_path : G::Path() ;
}
#endif

