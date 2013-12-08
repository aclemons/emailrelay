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
// pack.cpp
//
// Creates a self-extracting archive.
//
// usage:
//  pack [-xapqd] [-f <list-file>] <output> {<stub>|NONE} <in> <out> [<in> <out> ...] [--dir] [<file> ... [--opt] ...]
//          -x : use iexpress (windows)
//          -d : pack into a simple directory tree
//          -a : convert all "*.txt"/"*.js" <out> files to CRLF
//          -p : plain storage with no compression
//          -q : quiet operation
//
// The table of contents is stored in the output file after the stub program.
// The final twelve bytes of the output provide the offset of the table of
// contents. Each entry in the table of contents comprises: the compressed file
// size in decimal ascii, a space, arbitrary flags string, a space, the file
// name/path, a newline. The end of the table is marked by a (0,-,end) entry.
//
// Currently each file's flags are set to "x" if the file is executable, or "-"
// otherwise.
//
// The packed files are compressed with zlib (unless using -p) and then
// concatenated immediately following the table of contents.
//
// Input files are specified in pairs: the input file to be packed and the final
// output path when unpacked. The "--dir" switch introduces a set of input files
// which are all to be unpacked into the same output directory.
//
// The "--opt" switch indicates that all subsequent files are optional; if they
// do not exist then the names are silently ignored.
//
// A list-file can be used instead of a long command-line (see bin/make-setup.sh)
// with each line in the list-file being alternately an input file (<in>) or an 
// output name (<out>).
//
// All file contents are read into memory before they are packed into the
// output.
//

#include "gdef.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gmemory.h"
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <iomanip>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> // system()

// still compile if no zlib but exit at run-time
#ifdef NO_ZLIB
 #ifdef HAVE_ZLIB_H
  #undef HAVE_ZLIB_H
 #endif
#endif
#ifdef HAVE_ZLIB_H
 #include <zlib.h>
#else
 typedef char Bytef ;
 const int Z_OK = 0 ;
 static int compress( char * , unsigned long * , const char * , unsigned long )
 { 
  throw std::runtime_error( "no zlib available at compile-time; use -p" ) ;
  return 0 ;
 }
#endif

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
	File( const std::string & path_in, const std::string & path_out, bool xtod = false ) ;
	void compress() ;
	void write( bool to_dir , const std::string & path , bool final = false , unsigned long final_size = 0UL ) ;
	~File() ;
	std::string m_path_in ;
	std::string m_path_out ;
	char_t * m_data_out ;
	char_t * m_data_in ;
	unsigned long m_data_in_size ;
	unsigned long m_data_out_size ;
	std::string m_flags ;
} ;

unsigned long File::size( const std::string & path )
{
	std::stringstream ss ;
	ss << G::File::sizeString(path) ;
	unsigned long n ;
	ss >> n ;
	return n ;
}

File::File( const std::string & path_in , const std::string & path_out , bool xtod ) :
	m_path_in(path_in) ,
	m_path_out(path_out) ,
	m_data_out(0) ,
	m_data_in(0) ,
	m_data_in_size(0UL) ,
	m_data_out_size(0UL)
{
	unsigned long file_size = File::size( path_in ) ;
	unsigned long buffer_size = xtod ? (file_size*2UL) : file_size ;
	m_data_in = new char_t [buffer_size] ;

	m_flags = G::File::executable(path_in) ? "x" : "-" ;

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
	m_data_out = m_data_in ;
	m_data_out_size = m_data_in_size ;

	check( get_count == file_size , "input read error" , path_in ) ;
}

void File::compress()
{
	unsigned long out_size = m_data_in_size + (m_data_in_size>>2U) + 100U ;
	m_data_out = new char_t [out_size] ;
	int rc = ::compress( m_data_out , &out_size , m_data_in , m_data_in_size ) ;
	check( rc == Z_OK , "compress error" ) ;
	m_data_out_size = out_size ;
}

void File::write( bool to_dir , const std::string & path , bool final , unsigned long final_size )
{
	std::auto_ptr<std::ofstream> out_ptr ;
	if( to_dir )
	{
		std::string base_dir = path ;
		G::Path dst = G::Path::join( base_dir , m_path_out ) ;
		if( !G::Directory(dst.dirname()).valid() )
			G::File::mkdirs( dst.dirname() ) ;
		out_ptr <<= new std::ofstream( dst.str().c_str() , std::ios::out|std::ios::trunc|std::ios::binary ) ;
	}
	else
	{
		out_ptr <<= new std::ofstream( path.c_str() , std::ios::app|std::ios::binary ) ;
	}
	std::ofstream & out = *out_ptr.get() ;
	check( out.good() , "open output error" ) ;
	out.write( (char*) m_data_out , m_data_out_size ) ;
	if( final && !to_dir )
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

void iexpress( const std::string & path_out , const std::string & path_stub , const StringPairs & file_list )
{
	std::string sedfile_path = G::Process::Id().str() + ".sed" ;
	std::cout << "pack: creating iexpress sed file: " << sedfile_path << std::endl ;
	std::ofstream sedfile( sedfile_path.c_str() ) ;
	if( !sedfile.good() )
		throw std::runtime_error( "cannot create sedfile" ) ;

	const char * eol = "\n" ; // since opened in text mode
	sedfile << "[Version]" << eol ;
	sedfile << "Class=IEXPRESS" << eol ;
	sedfile << "SEDVersion=3" << eol ;
	sedfile << "[Options]" << eol ;
	sedfile << "PackagePurpose=InstallApp" << eol ;
	sedfile << "ShowInstallProgramWindow=0" << eol ;
	sedfile << "HideExtractAnimation=0" << eol ;
	sedfile << "UseLongFileName=1" << eol ;
	sedfile << "InsideCompressed=0" << eol ;
	sedfile << "CAB_FixedSize=0" << eol ;
	sedfile << "CAB_ResvCodeSigning=0" << eol ;
	sedfile << "RebootMode=N" << eol ;
	sedfile << "InstallPrompt=%InstallPrompt%" << eol ;
	sedfile << "DisplayLicense=%DisplayLicense%" << eol ;
	sedfile << "FinishMessage=%FinishMessage%" << eol ;
	sedfile << "TargetName=%TargetName%" << eol ;
	sedfile << "FriendlyName=%FriendlyName%" << eol ;
	sedfile << "AppLaunched=%AppLaunched%" << eol ;
	sedfile << "PostInstallCmd=%PostInstallCmd%" << eol ;
	sedfile << "AdminQuietInstCmd=%AdminQuietInstCmd%" << eol ;
	sedfile << "UserQuietInstCmd=%UserQuietInstCmd%" << eol ;
	sedfile << "SourceFiles=SourceFiles" << eol ;
	sedfile << "[Strings]" << eol ;
	sedfile << "InstallPrompt=" << eol ;
	sedfile << "DisplayLicense=" << eol ;
	sedfile << "FinishMessage=" << eol ;
	sedfile << "TargetName=" << path_out << eol ;
	sedfile << "FriendlyName=E-MailRelay" << eol ;
	sedfile << "AppLaunched=" << path_stub << eol ;
	sedfile << "PostInstallCmd=<None>" << eol ;
	sedfile << "AdminQuietInstCmd=" << eol ;
	sedfile << "UserQuietInstCmd=" << eol ;

	typedef std::map<std::string,int> Map ;
	typedef std::vector<int> List ;
	typedef std::vector<List> Grid ;
	Map dirs ;
	Map strings ;
	Grid grid ;
	int string_index = 0 ;
	int dir_index = 0 ;
	for( StringPairs::const_iterator file_p = file_list.begin() ; file_p != file_list.end() ; ++file_p )
	{
		std::string path = (*file_p).first ;
		std::string dir = G::Path(path).dirname().str() ;
		std::string basename = G::Path(path).basename() ;
		if( basename != (*file_p).second )
			throw std::runtime_error( std::string() + 
				"iexpress does not support file renaming on extraction: " + 
				basename + " != " + (*file_p).second ) ;
		if( strings.find(basename) != strings.end() )
			throw std::runtime_error( std::string() + 
				"iexpress does not support different files with the same basename: " +
				basename ) ;
		strings[basename] = string_index ;
		if( dirs.find(dir) == dirs.end() )
		{
			dirs[dir] = dir_index ;
			grid.push_back( List() ) ;
			dir_index++ ;
		}
		grid[dirs[dir]].push_back(string_index) ;
		string_index++ ;
	}

	{
		// (currently in [Strings])
		for( Map::iterator map_p = strings.begin() ; map_p != strings.end() ; ++map_p )
		{
			sedfile << "FILE" << (*map_p).second << "=" << (*map_p).first << eol ;
		}
	}

	sedfile << "[SourceFiles]" << eol ;
	{
		for( Map::iterator map_p = dirs.begin() ; map_p != dirs.end() ; ++map_p )
		{
			int dir_index = (*map_p).second ;
			std::string dir = (*map_p).first ;
			dir = dir.empty() ? "." : dir ;
			sedfile << "SourceFiles" << dir_index << "=" << dir << eol ;
		}
	}
	for( Map::iterator map_p = dirs.begin() ; map_p != dirs.end() ; ++map_p )
	{
		int dir_index = (*map_p).second ;
		sedfile << "[SourceFiles" << dir_index << "]" << eol ;
		List & list = grid[dir_index] ;
		for( List::iterator list_p = list.begin() ; list_p != list.end() ; ++list_p )
		{
			int string_index = (*list_p) ;
			sedfile << "%FILE" << string_index << "%=" << eol ;
		}
	}

	sedfile.flush() ;
	if( !sedfile.good() )
		throw std::runtime_error( "cannot write sedfile" ) ;
	sedfile.close() ;

	std::string cmd = std::string() + "cmd /c \"iexpress /N " + sedfile_path + "\"" ;
	std::cout << "pack: running iexpress: " << cmd << std::endl ;
	int rc = system( cmd.c_str() ) ;
	if( rc != 0 )
		throw std::runtime_error( "failed to run iexpress" ) ;
}

void pack( const std::string & path_out , const std::string & path_stub , const StringPairs & file_list , 
	bool cfg_plain , bool cfg_quiet , bool cfg_auto_xtod , bool cfg_to_directory )
{
	// start off the output
	//
	if( cfg_to_directory && !G::Directory(path_out).valid() )
		G::File::mkdir( path_out ) ;

	// start building the output
	//
	long stub_size = 0L ;
	if( !path_stub.empty() && path_stub != "NONE" )
	{
		stub_size = File::size( path_stub ) ;
		std::cout << "pack: copying stub: " << path_stub << ": " << stub_size << std::endl ;
		if( cfg_to_directory )
			G::File::copy( path_stub , G::Path(path_out,G::Path(path_stub).basename()) ) ;
		else
			G::File::copy( path_stub , path_out ) ;
	}

	// read and possibly compress the files
	//
	std::list<File*> list ;
	for( StringPairs::const_iterator p = file_list.begin() ; p != file_list.end() ; ++p )
	{
		bool is_txt = 
			G::Path((*p).second).extension() == "txt" ||
			G::Path((*p).second).extension() == "js" ;
		File * file = new File( (*p).first , (*p).second , cfg_auto_xtod && is_txt ) ;
		list.push_back( file ) ;
		if( !cfg_plain )
		{
			file->compress() ;
			if( !cfg_quiet )
			{
				std::cout
					<< "pack: compressing " 
					<< file->m_path_in << ": " << file->m_data_in_size << " -> " 
					<< file->m_data_out_size << std::endl ;
			}
		}
	}

	// write the table of contents
	//
	if( !cfg_to_directory )
	{
		std::cout << "pack: writing table of contents" << std::endl ;
		std::ofstream out( path_out.c_str() , std::ios::app | std::ios::binary ) ;
		out << (cfg_plain?"0":"1") << '\0' ;
		for( std::list<File*>::iterator p = list.begin() ; p != list.end() ; ++p )
		{
			std::string out_name = (*p)->m_path_out ;
			G::Str::replaceAll( out_name , " " , "\001" ) ;
			out << (*p)->m_data_out_size << " " << (*p)->m_flags << " " << out_name << "\n" ;
		}
		out << "0 - end\n" ;
		out.flush() ;
		check( out.good() , "write error" ) ;
	}

	// write the data
	//
	for( std::list<File*>::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		File * file = (*p) ;
		std::list<File*>::iterator next = p ; ++next ;
		bool last = next == list.end() ;
		if( !cfg_quiet )
			std::cout << "pack: writing " << file->m_path_out << std::endl ;
		(*p)->write( cfg_to_directory , path_out , last , last ? stub_size : 0UL ) ;
	}
}

int main( int argc , char * argv [] )
{
	std::string path_out ;
	try
	{
		// parse the command-line
		//
		G::Arg arg( argc , argv ) ;
		bool cfg_use_iexpress = arg.contains( "-x" ) ; if( cfg_use_iexpress ) arg.remove( "-x" ) ;
		bool cfg_to_directory = arg.contains( "-d" ) ; if( cfg_to_directory ) arg.remove( "-d" ) ;
		bool cfg_plain = arg.contains( "-p" ) ; if( cfg_plain ) arg.remove( "-p" ) ;
		bool cfg_auto_xtod = arg.contains( "-a" ) ; if( cfg_auto_xtod ) arg.remove( "-a" ) ;
		bool cfg_quiet = arg.contains( "-q" ) ; if( cfg_quiet ) arg.remove( "-q" ) ;
		std::string list_file ;
		if( arg.contains("-f",1U) )
		{
			list_file = arg.v( arg.index("-f",1U)+1U ) ;
			arg.remove( "-f" , 1U ) ;
		}
		const char * usage =
			"usage: pack [-xapqd] [-f <list-file>] <output> <stub> <file-in> <file-out> [<file-in> ...]" ;
		check( arg.c() >= 3 , usage ) ;
		path_out = arg.v(1) ;
		std::string path_stub( arg.v(2) ) ;
		std::cout << "pack: creating [" << path_out << "]" << std::endl ;

		// build the file list -- use "--dir" to introduce a list
		// of single files rather than a list of in/out pairs
		// and use "--opt" to ignore wildcarded filenames
		//
		std::string dir ;
		bool dir_mode = false ;
		bool opt_mode = false ;
		StringPairs file_list ;
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
					file_list.push_back( std::make_pair(arg.v(i),G::Path(dir,G::Path(arg.v(i)).basename()).str()) ) ;
			}
			else
			{
				// take in/out pair
				G::Path p1( arg.v(i) ) ;
				G::Path p2( arg.v(i+1) ) ;
				file_list.push_back( std::make_pair(p1.str(),p2.str()) ) ;
				i++ ;
			}
		}
		if( !list_file.empty() )
		{
			unsigned int n = file_list.size() ;
			std::cout << "pack: reading file list from \"" << list_file << "\"" << std::endl ;
			std::ifstream f( list_file.c_str() ) ;
			std::string previous ;
			for( int i = 0 ; f.good() ; i++ )
			{
				std::string line = G::Str::readLineFrom( f , "\n" ) ;
				if( !f ) break ;
				G::Str::trim( line , G::Str::ws() ) ;
				if( (i % 2U) == 1U )
					file_list.push_back( std::make_pair(previous,line) ) ;
				previous = line ;
			}
			std::cout << "pack: read " << (file_list.size()-n) << " files from file list" << std::endl ;
		}

		if( cfg_use_iexpress )
		{
			iexpress( path_out , path_stub , file_list ) ;
		}
		else
		{
			pack( path_out , path_stub , file_list , 
				cfg_plain , cfg_quiet , cfg_auto_xtod , cfg_to_directory ) ;
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
