//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// pack.cpp
//
// Creates a self-extracting archive.
//
// usage: pack [-a] [-p] <output> <stub> <payload-in> <payload-out> [<in> <out> ...] [--dir] [<file> ... [--opt] ...]
//
// The table of contents is stored in the output file after
// the stub program. The final twelve bytes of the output provide 
// the offset of the table of contents. Each entry in the table 
// of contents comprises: the compressed file size in decimal 
// ascii, a space, the file name/path, a newline. The end of 
// the table is marked by the entry "0 end\n".
//
// The packed files are compressed with zlib (unless using -p)
// and then concatenated immediately following the table of 
// contents.
//
// The "--dir" switch introduces a set of input files which
// are to be unpacked into the specified output directory.
//
// The "--opt" switch indicates that all subsequent files
// are optional; if they do not exist then they are silently
// ignored.
//

#include "gdef.h"
#include "garg.h"
#include "gpath.h"
#include "gfile.h"
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <iomanip>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

typedef Bytef char_t ; // zlib char
typedef std::vector<std::string> Strings ;

void check( bool ok , const std::string & p1 , const std::string & p2 = std::string() )
{
	if( !ok )
	{
		std::string p = p2.empty() ? p1 : ( p1 + ": " + p2 ) ;
		throw std::runtime_error(p) ;
	}
}

struct File
{
	static unsigned long size( const std::string & path ) ;
	File( bool plain , const std::string & path_in , const std::string & path_out , bool xtod = false ) ;
	void compress() ;
	void append( const std::string & path , unsigned long final_size = 0UL ) ;
	~File() ;
	std::string m_path_in ;
	std::string m_path_out ;
	bool m_plain ;
	char_t * m_data_out ;
	char_t * m_data_in ;
	unsigned long m_data_in_size ;
	unsigned long m_data_out_size ;
} ;

unsigned long File::size( const std::string & path )
{
	std::stringstream ss ;
	ss << G::File::sizeString(path) ;
	unsigned long n ;
	ss >> n ;
	return n ;
}

File::File( bool plain , const std::string & path_in , const std::string & path_out , bool xtod ) : 
	m_path_in(path_in) , m_path_out(path_out) , m_plain(plain) ,
	m_data_out(0) , m_data_in(0) , m_data_in_size(0UL) , m_data_out_size(0UL)
{
	unsigned long file_size = File::size( path_in ) ;
	unsigned long buffer_size = xtod ? (file_size*2UL) : file_size ;
	m_data_in = new char_t [buffer_size] ;

	std::ifstream in( path_in.c_str() , std::ios::binary ) ;
	check( in.good() , "cannot open input file" , path_in ) ;

	unsigned long get_count = 0UL ;
	unsigned long i = 0UL ;
	for( ; in.good() && get_count < file_size ; i++ )
	{
		m_data_in[i] = in.get() ;
		get_count++ ;

		if( xtod && m_data_in[i] == '\n' )
		{
			m_data_in[i++] = '\r' ;
			m_data_in[i] = '\n' ;
		}
	}
	m_data_in_size = i ;

	check( get_count == file_size , "input read error" , path_in ) ;
}

void File::compress()
{
	if( m_plain )
	{
		m_data_out = m_data_in ;
		m_data_out_size = m_data_in_size ;
	}
	else
	{
		unsigned long out_size = m_data_in_size + (m_data_in_size>>2U) + 100U ;
		m_data_out = new char_t [out_size] ;
		int rc = ::compress( m_data_out , &out_size , m_data_in , m_data_in_size ) ;
		check( rc == Z_OK , "compress error" ) ;
		m_data_out_size = out_size ;
	}
}

void File::append( const std::string & path , unsigned long final_size )
{
	std::ofstream out( path.c_str() , std::ios::app | std::ios::binary ) ;
	check( out.good() , "open output error" ) ;
	out.write( (char*) m_data_out , m_data_out_size ) ;
	if( final_size != 0UL )
		out << std::setw(11U) << final_size << "\n" ;
	out.flush() ;
	check( out.good() , "write error" ) ;
}

File::~File() 
{ 
	if( m_data_out != m_data_in )
		delete [] m_data_out ; 
	delete [] m_data_in ; 
}

typedef std::vector<std::pair<std::string,std::string> > StringPairs ;

int main( int argc , char * argv [] )
{
	std::string path_out ;
	try
	{
		// check the command-line
		//
		G::Arg arg( argc , argv ) ;
		bool plain = arg.contains( "-p" ) ; if( plain ) arg.remove( "-p" ) ;
		bool auto_xtod = arg.contains( "-a" ) ; if( auto_xtod ) arg.remove( "-a" ) ;
		const char * usage = "usage: pack [-p] <output> <stub> <payload-in> <payload-out> [<payload-in> ...]" ;
		check( arg.c() >= 5 , usage ) ;
		path_out = arg.v(1) ;
		std::string path_stub( arg.v(2) ) ;
		std::cout << "pack: creating " << path_out << std::endl ;

		// build the payload list -- use "--dir" to introduce a list
		// of single payload files rather than a list of in/out pairs
		// and use "--opt" to ignore wildcarded filenames
		//
		std::string dir ;
		bool dir_mode = false ;
		bool opt_mode = false ;
		StringPairs payload ;
		for( unsigned int i = 3 ; i < arg.c() ; i++ )
		{
			if( arg.v(i) == "--dir" )
			{
				dir_mode = true ;
				dir = arg.v(i+1) ; 
				i++ ;
			}
			else if( arg.v(i) == "--opt" )
			{
				opt_mode = true ;
			}
			else if( dir_mode )
			{
				// in dir mode take each parameter as an input path to be output
				// to the specified directory -- if also in opt mode and there 
				// is still a wildcard in the name then the shell could not find 
				// a match so silently ignore it

				if( !opt_mode || arg.v(i).find('*') == std::string::npos )
					payload.push_back( std::make_pair(arg.v(i),G::Path(dir,G::Path(arg.v(i)).basename()).str()) ) ;
			}
			else
			{
				// take in/out pair
				G::Path p1( arg.v(i) ) ;
				G::Path p2( arg.v(i+1) ) ;
				payload.push_back( std::make_pair(p1.str(),p2.str()) ) ;
				i++ ;
			}
		}

		// start building the output
		//
		unsigned long stub_size = File::size( path_stub ) ;
		std::cout << "pack: copying stub: " << path_stub << ": " << stub_size << std::endl ;
		G::File::copy( path_stub , path_out ) ;

		// read and compress the files
		//
		std::list<File*> list ;
		for( StringPairs::iterator p = payload.begin() ; p != payload.end() ; ++p )
		{
			bool is_txt = G::Path((*p).second).extension() == "txt" ;
			File * file = new File( plain , (*p).first , (*p).second , auto_xtod && is_txt ) ;
			list.push_back( file ) ;
			file->compress() ;
			std::cout 
				<< "pack: compression: " << file->m_path_in << ": " 
				<< file->m_data_in_size << " -> " << file->m_data_out_size << std::endl ;
		}

		// write the table of contents
		//
		std::cout << "pack: writing table of contents" << std::endl ;
		{
			std::ofstream out( path_out.c_str() , std::ios::app | std::ios::binary ) ;
			out << (plain?"0":"1") << '\0' ;
			for( std::list<File*>::iterator p = list.begin() ; p != list.end() ; ++p )
				out << (*p)->m_data_out_size << " " << (*p)->m_path_out << "\n" ;
			out << "0 end\n" ;
			out.flush() ;
			check( out.good() , "write error" ) ;
		}

		// write the compressed data
		//
		for( std::list<File*>::iterator p = list.begin() ; p != list.end() ; ++p )
		{
			File * file = (*p) ;
			std::list<File*>::iterator next = p ; ++next ;
			bool last = next == list.end() ;
			std::cout << "pack: appending " << file->m_path_out << std::endl ;
			(*p)->append( path_out , last ? stub_size : 0UL ) ;
		}
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
		G::File::remove( path_out , G::File::NoThrow() ) ;
	}
	return 1 ;
}

/// \file pack.cpp
