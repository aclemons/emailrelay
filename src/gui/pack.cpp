//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// pack.cpp
//
// Creates a self-extracting archive.
//
// usage: pack <output> <stub> <payload-in> <payload-out> [<payload-in> <payload-out> ...]
//
// The table of contents is stored in the output file
// afer the stub program. The final twelve bytes of the 
// output provide the offset of the table of contents.
// Each entry in the table of contents comprises: the
// compressed file size in decimal ascii, a space,
// the file name/path, a newline. The end of the table
// is marked by the entry "0 end\n".
//
// The packed files are compressed with zlib and then
// concatenated immediately following the table of 
// contents.
//

#include "gdef.h"
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
	static unsigned long size( const std::string & path )
	{
		std::stringstream ss ;
		ss << G::File::sizeString(path) ;
		unsigned long n ;
		ss >> n ;
		return n ;
	}
	File( const std::string & path_in , const std::string & path_out ) : 
		m_path_in(path_in) , m_path_out(path_out) ,
		m_data_out(0) , m_data_in(0) , m_data_in_size(0UL) , m_data_out_size(0UL)
	{
		m_data_in_size = File::size( path_in ) ;
		std::ifstream in( path_in.c_str() , std::ios::binary ) ;
		check( in.good() , "cannot open input file" , path_in ) ;
		m_data_in = new char_t [m_data_in_size] ;
		unsigned long i = 0UL ;
		for( ; in.good() && i < m_data_in_size ; i++ )
			m_data_in[i] = in.get() ;
		check( i == m_data_in_size , "input read error" , path_in ) ;
	}
	void compress()
	{
		unsigned long out_size = m_data_in_size + (m_data_in_size>>2U) + 100U ;
		m_data_out = new char_t [out_size] ;
		int rc = ::compress( m_data_out , &out_size , m_data_in , m_data_in_size ) ;
		check( rc == Z_OK , "compress error" ) ;
		m_data_out_size = out_size ;
	}
	void append( const std::string & path , unsigned long final_size = 0UL )
	{
		std::ofstream out( path.c_str() , std::ios::app | std::ios::binary ) ;
		check( out.good() , "open output error" ) ;
		out.write( (char*) m_data_out , m_data_out_size ) ;
		if( final_size != 0UL )
			out << std::setw(11U) << final_size << "\n" ;
		out.flush() ;
		check( out.good() , "write error" ) ;
	}
	~File() { delete [] m_data_in ; delete [] m_data_out ; }
	std::string m_path_in ;
	std::string m_path_out ;
	char_t * m_data_out ;
	char_t * m_data_in ;
	unsigned long m_data_in_size ;
	unsigned long m_data_out_size ;
} ;

typedef std::vector<std::pair<std::string,std::string> > StringPairs ;

int main( int argc , char * argv [] )
{
	try
	{
		// parse the command-line
		check( argc >= 5 && (argc % 2) == 1 , 
			"usage: pack <output> <stub> <payload-in> <payload-out> [<payload-in> ...]" ) ;
		std::string path_out( argv[1] ) ;
		std::string path_stub( argv[2] ) ;
		StringPairs payload ;
		for( int i = 3 ; i < argc ; i += 2 )
			payload.push_back( std::make_pair(argv[i],argv[i+1]) ) ;
		std::cout << "pack: creating " << path_out << std::endl ;

		// get the size of the stub
		unsigned long stub_size = File::size( path_stub ) ;

		// start building the output
		std::cout << "pack: copying stub: " << path_stub << ": " << stub_size << std::endl ;
		G::File::copy( path_stub , path_out ) ;

		// read and compress the files
		std::list<File*> list ;
		for( StringPairs::iterator p = payload.begin() ; p != payload.end() ; ++p )
		{
			File * file = new File( (*p).first , (*p).second ) ;
			list.push_back( file ) ;
			file->compress() ;
			std::cout 
				<< "pack: " << file->m_path_in << ": " 
				<< file->m_data_in_size << " -> " << file->m_data_out_size << std::endl ;
		}

		// write the table of contents
		std::cout << "pack: writing table of contents" << std::endl ;
		{
			std::ofstream out( path_out.c_str() , std::ios::app | std::ios::binary ) ;
			for( std::list<File*>::iterator p = list.begin() ; p != list.end() ; ++p )
				out << (*p)->m_data_out_size << " " << (*p)->m_path_out << "\n" ;
			out << "0 end\n" ;
			out.flush() ;
			check( out.good() , "write error" ) ;
		}

		// write the compressed data
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
	}
	return 1 ;
}

