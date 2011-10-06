//
// Copyright (C) 2001-2011 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gunpack.cpp
//

#include "gdef.h"
#include "gunpack.h"
#include "gfile.h"
#include "gstr.h"
#include "glog.h"
#include <fstream>
#include <stdexcept>
#include <fstream>
#include <iomanip>

// still compile if no zlib -- throw an error at run-time
#ifdef HAVE_ZLIB_H
 #include <zlib.h>
#else
 typedef char Bytef ;
 char * const Z_NULL = 0 ;
 const int Z_SYNC_FLUSH = 0 ;
 const int Z_STREAM_END = 0 ;
 const int Z_OK = 0 ;
 typedef struct z_stream {
 const char * next_in ; unsigned avail_in ; const char * next_out ; 
 unsigned avail_out ; const char * zalloc ; const char * zfree ; const char * opaque ; } ;
 int inflateInit( z_stream * ) { throw std::runtime_error("no zlib available at compile-time") ; return 0 ; }
 int inflate( z_stream * , int ) { return 0 ; }
 int inflateEnd( z_stream * ) { return 0 ; }
#endif

namespace
{
	void check( bool ok , const std::string & p )
	{
		if( ! ok )
			throw G::Unpack::PackingError(p) ;
	}
	void check( bool ok , const std::string & p1 , const std::string & p2 )
	{
		if( ! ok )
			throw G::Unpack::PackingError(std::string()+p2+": "+p1) ;
	}
	Bytef * zptr( char * p )
	{
		return reinterpret_cast<Bytef*>(p) ;
	}
	Bytef * czptr( const char * p )
	{
		return reinterpret_cast<Bytef*>(const_cast<char*>(p)) ;
	}
}

bool G::Unpack::isPacked( Path path )
{
	std::string error = packingError( path ) ;
	return error.empty() ;
}

std::string G::Unpack::packingError( Path path )
{
	try
	{
		std::streamsize exe_size = static_cast<std::streamsize>( G::Str::toULong( G::File::sizeString(path) ) ) ;
		if( exe_size <= 12L ) return "invalid file size" ;
		std::ifstream input( path.str().c_str() , std::ios_base::binary ) ;
		if( !input.good() ) return "cannot open" ;
		input.seekg( exe_size - 12UL ) ;
		if( !input.good() ) return "cannot seek" ;
		std::string str_offset = Str::readLineFrom( input ) ;
		if( str_offset.length() != 11U || str_offset.find_first_not_of(" 0123456789") != std::string::npos ) 
			return std::string() + "no offset string: [" + Str::printable(str_offset) + "]" ;
		std::streamsize offset = static_cast<std::streamsize>( G::Str::toULong( str_offset ) ) ;
		if( offset == 0L || offset >= exe_size ) 
			return std::string() + "invalid offset: [" + Str::printable(str_offset) + "]" ;
		char is_compressed_char = '\0' ;
		input.seekg( offset ) ;
		input >> is_compressed_char ;
		if( is_compressed_char != '0' && is_compressed_char != '1' ) return "invalid compression flag" ;
		return std::string() ;
	}
	catch( std::exception & e )
	{
		return e.what() ;
	}
}

G::Unpack::Unpack( Path path ) :
	m_path(path) ,
	m_max_size(0UL) ,
	m_input(NULL) ,
	m_offset(0)
{
	init() ;
}

G::Unpack::Unpack( Path path , NoThrow ) :
	m_path(path) ,
	m_max_size(0UL) ,
	m_input(NULL) ,
	m_offset(0)
{
	if( isPacked(path) )
		init() ;
}

void G::Unpack::init()
{
	// get the file size
	G_DEBUG( "Unpack::unpack: \"" << m_path << "\"" ) ;
	std::string exe_size_string = G::File::sizeString( m_path ) ;
	std::streamsize exe_size = static_cast<std::streamsize>( G::Str::toULong( exe_size_string ) ) ;
	G_DEBUG( "Unpack::unpack: size: " << exe_size ) ;
	check( exe_size > 12L , "invalid exe size" , m_path.str() ) ;

	// seek to near the end
	m_input = new std::ifstream( m_path.str().c_str() , std::ios_base::binary ) ;
	std::istream & input = *m_input ;
	check( input.good() , "open error" , m_path.str() ) ;
	input.seekg( exe_size - 12L ) ;
	check( input.good() , "no offset" , m_path.str() ) ;

	// read the original size
	m_offset = 0 ;
	input >> m_offset ;
	G_DEBUG( "Unpack::unpack: offset " << m_offset << " (0x" << std::hex << m_offset << ")" ) ;
	check( m_offset != 0 , "not a packed file" , m_path.str() ) ;
	check( (m_offset+12) < exe_size , "invalid offset" , m_path.str() ) ;
	check( input.good() , "offset read error" , m_path.str() ) ;

	// read the is-compressed flag
	char is_compressed_char = '\0' ;
	input.seekg( m_offset ) ;
	input >> is_compressed_char ;
	check( is_compressed_char == '0' || is_compressed_char == '1' , "invalid compression type or format" ) ;
	m_is_compressed = is_compressed_char == '1' ;

	// seek to the directory
	input.seekg( m_offset + 2 ) ;
	check( input.good() , "seek error" ) ;

	// read the directory
	unsigned long file_offset = 0UL ;
	for(;;)
	{
		std::string file_path ;
		std::string flags ;
		unsigned long file_size = 0UL ;
		input >> file_size >> flags >> file_path ; 
		G::Str::replaceAll( file_path , std::string(1U,'\001') , " " ) ; // SOHs can be used for spaces in filenames
		G_DEBUG( "Unpack::unpack: [" << file_path << "] [" << file_size << "]" ) ;
		if( file_size == 0U ) 
		{
			check( file_path == "end" , "invalid internal directory" ) ;
			break ;
		}
		m_map.insert( Map::value_type(file_path,Entry(file_path,file_size,file_offset,flags)) ) ;
		file_offset += file_size ;
		if( file_size > m_max_size )
			m_max_size = file_size ;
	}

	// reserve a buffer
	G_DEBUG( "Unpack::unpack: max size: " << m_max_size ) ;
	check( m_max_size < 100000000UL , "too big" ) ; // sanity limit
	m_buffer.reserve( m_max_size + 1UL ) ;

	// eat the newline
	input.get() ;
	check( input.good() , "file-map read error" ) ;
	m_start = input.tellg() ;
}

G::Unpack::~Unpack()
{
	delete m_input ;
}

G::Path G::Unpack::path() const
{
	return m_path ;
}

G::Strings G::Unpack::names() const
{
	Strings result ;
	for( Map::const_iterator p = m_map.begin() ; p != m_map.end() ; ++p )
		result.push_back( (*p).first ) ;
	return result ;
}

std::string G::Unpack::flags( const std::string & name ) const
{
	Map::const_iterator p = m_map.find(name) ;
	if( p == m_map.end() ) throw NoSuchFile(name) ;
	return (*p).second.flags ;
}

void G::Unpack::unpack( const Path & to_dir )
{
	for( Map::iterator p = m_map.begin() ; p != m_map.end() ; ++p )
	{
		unpack( to_dir , *m_input , (*p).second ) ;
	}
}

void G::Unpack::unpack( const Path & to_dir , std::istream & input , const Entry & entry )
{
	unpack( input , entry.offset , entry.size , Path::join(to_dir,entry.path) ) ;
}

void G::Unpack::unpack( const Path & to_dir , const std::string & name )
{
	G_DEBUG( "Unpack::unpack: [" << name << "] (" << m_map.size() << ")" ) ;
	Map::iterator p = m_map.find(name) ;
	if( p == m_map.end() ) throw NoSuchFile(name) ;
	unpack( to_dir , *m_input , (*p).second ) ;
}

void G::Unpack::unpack( const std::string & name , const Path & dst )
{
	G_DEBUG( "Unpack::unpack: [" << name << "] (" << m_map.size() << ")" ) ;
	Map::iterator p = m_map.find(name) ;
	if( p == m_map.end() ) throw NoSuchFile(name) ;
	unpack( *m_input , (*p).second.offset , (*p).second.size , dst ) ;
}

void G::Unpack::unpack( std::istream & input, unsigned long entry_offset, unsigned long entry_size, const Path & dst )
{
	// sync up
	input.seekg( m_start + entry_offset ) ;
	m_buffer.clear() ;

	// read file data
	G_DEBUG( "Unpack::unpack: reading " << entry_size << " bytes at offset " << (m_start+entry_offset) 
		<< "(0x" << std::hex << (m_start+entry_offset) << ") for \"" << dst << "\"" ) ;
	unsigned long i = 0UL ;
	for( ; i < entry_size && input.good() ; i++ )
		m_buffer.push_back( input.get() ) ;
	check( input.good() , "read error" ) ;
	check( i == entry_size , "read error (2)" ) ;
	check( m_buffer.size() == entry_size , "read error (3)" ) ;

	// continue
	unpack( dst , m_buffer ) ;
}

void G::Unpack::unpack( const Path & dst , const std::vector<char> & buffer )
{
	std::ofstream output( dst.str().c_str() , std::ios_base::binary ) ;
	unpack( output , buffer ) ;
	check( output.good() , std::string() + "cannot create \"" + dst.str() + "\"" ) ;
}

void G::Unpack::unpack( std::ostream & output , const std::vector<char> & buffer_in )
{
	char buffer_out[1024U*8U] ;

	z_stream z ;
	z.next_in = czptr(&buffer_in[0]) ;
	z.avail_in = buffer_in.size() ;
	z.next_out = zptr(buffer_out) ;
	z.avail_out = sizeof(buffer_out) ;
	z.zalloc = Z_NULL ;
	z.zfree = Z_NULL ;
	z.opaque = 0 ;
	int rc = inflateInit( &z ) ;
	check( rc == Z_OK , "inflateInit() error" ) ;
	try
	{
		for(;;)
		{
			z.next_out = zptr(buffer_out) ;
			z.avail_out = sizeof(buffer_out) ;
			rc = inflate( &z , Z_SYNC_FLUSH ) ;
			check( rc == Z_OK || rc == Z_STREAM_END , "inflate() error" ) ;
			unsigned int n = sizeof(buffer_out) - z.avail_out ;
			output.write( buffer_out , n ) ;
			if( rc == Z_STREAM_END ) break ;
		}
	}
	catch( ... )
	{
		inflateEnd( &z ) ;
		throw ;
	}
	rc = inflateEnd( &z ) ;
	check( rc == Z_OK , "inflateEnd() error" ) ;
}

void G::Unpack::unpackOriginal( const Path & dst )
{
	std::string reason = unpackOriginal( dst , NoThrow() ) ;
	if( !reason.empty() )
		throw PackingError( reason ) ;
}

std::string G::Unpack::unpackOriginal( const Path & dst , NoThrow )
{
	if( m_input == NULL || m_offset == 0 )
		return std::string() ;

	std::istream & input = *m_input ;
	input.seekg( 0UL ) ;
	if( !input.good() ) 
		return "cannot open file for reading" ;

	std::ofstream out( dst.str().c_str() , std::ios::binary | std::ios::out | std::ios::trunc ) ;
	if( !out.good() ) 
		return std::string() + "cannot open file for reading: " + dst.str() ;

	copy( input , out , m_offset ) ;
	if( !input.good() )
		return "cannot read file" ;

	out.flush() ;
	if( !out.good() ) 
		return std::string() + "cannot write: " + dst.str() ;

	return std::string() ;
}

void G::Unpack::copy( std::istream & in , std::ostream & out , std::streamsize size )
{
	File::copy( in , out , size ) ;
}

/// \file gunpack.cpp
