//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file filter.cpp
///
// A utility that can be installed as a "--filter" program to copy the message
// envelope into all spool sub-directories for use by "--pop-by-name".
//
// If the envelope in the parent directory is successfully copied to at least
// once sub-directory then it is removed from the parent directory and the
// program exits with a value of 100.
//
// Fails if there are no sub-directories to copy in to.
//

#include "gdef.h"
#include "garg.h"
#include "gfile.h"
#include "gstr.h"
#include "gprocess.h"
#include "gexception.h"
#include "gdirectory.h"
#include "legal.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <set>
#include <string>

G_EXCEPTION_CLASS( FilterError , tx("filter error") ) ;

static void help( const std::string & prefix )
{
	std::cout
		<< "usage: " << prefix << " { <emailrelay-content-file> | [-v] -d <spool-dir> }\n"
		<< "\n"
		<< "Copies the corresponding emailrelay envelope file into all \n"
		<< "sub-directories of the spool directory. Exits with a \n"
		<< "value of 100 if copied once or more. Intended for use \n"
		<< "with \"emailrelay --pop-by-name --filter=...\".\n"
		<< "\n"
		<< "With \"-d\" all envelope files are copied.\n"
		<< "\n"
		<< Main::Legal::warranty(std::string(),"\n") << "\n"
		<< Main::Legal::copyright() << std::endl ;
}

struct Filter
{
public:
	Filter() = default;
	explicit Filter( bool verbose ) : m_verbose(verbose) {}
	void process_content( const std::string & ) ;
	void process_envelope() ;
	void throwFailures( bool ) ;
	bool ok() const { return m_failures.empty() ; }
	bool envelopeDeleted() const { return m_envelope_deleted ; }
	void setEnvelope( const std::string & name , const G::Path & path )
	{
		m_envelope_name = name ;
		m_envelope_path = path ;
	}

private:
	std::string m_envelope_name ;
	G::Path m_envelope_path ; // "<spool-dir>/<envelope-name>[.new]"
	bool m_envelope_deleted{false} ;
	int m_directory_count{0} ;
	std::set<std::string> m_failures ;
	bool m_verbose{false} ;
	bool m_dryrun{false} ;
} ;

void Filter::process_envelope()
{
	// the umask inherited from the emailrelay server does not give
	// group access, so loosen it up to "-???rw-???" and note that
	// the spool directory should have sticky group ownership
	// which gets inherited by sub-directories and all message
	// files
	//
	G::Process::Umask::loosenGroup() ;

	// copy the envelope into all sub-directories
	//
	G::Directory spool_dir( m_envelope_path.simple() ? std::string(".") : m_envelope_path.dirname() ) ;
	G::DirectoryIterator iter( spool_dir ) ;
	int copies = 0 ;
	int failures = 0 ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() )
		{
			G::Path subdir = iter.filePath() ;
			copies++ ;
			G::Path target = G::Path( subdir , m_envelope_name ) ;
			bool copied = m_dryrun ? true : G::File::copy( m_envelope_path , target , std::nothrow ) ;
			if( m_verbose )
				std::cout << (copied?"copied":"failed") << ": " << m_envelope_path << " " << target << "\n" ;
			if( !copied )
			{
				failures++ ;
				m_failures.insert( iter.fileName() ) ;
			}
		}
	}
	if( m_directory_count == 0 )
		m_directory_count = copies ;

	// delete the original envelope (ignore errors)
	//
	if( copies > 0 && failures == 0 )
	{
		m_envelope_deleted = m_dryrun ? true : G::File::remove( m_envelope_path , std::nothrow ) ;
	}
}

void Filter::process_content( const std::string & content )
{
	// check the content file exists
	//
	G::Path content_path( content ) ;
	bool ok = G::File::exists( content_path ) ;
	if( !ok )
		throw FilterError( "no such file" ) ;

	// build the envelope name
	//
	m_envelope_name = content_path.basename() ;
	unsigned int n = G::Str::replaceAll( m_envelope_name , "content" , "envelope" ) ;
	if( n != 1U )
		throw FilterError( "invalid filename" ) ;

	// check the envelope file exists
	//
	G::Path dir_path = content_path.dirname() ;
	m_envelope_path = G::Path( dir_path , m_envelope_name + ".new" ) ;
	if( ! G::File::exists(m_envelope_path) )
	{
		// fall back to no extension in case we are run manually for some reason
		G::Path envelope_path_alt = G::Path( dir_path , m_envelope_name ) ;
		if( G::File::exists(envelope_path_alt) )
			m_envelope_path = envelope_path_alt ;
		else
			throw FilterError( std::string() + "no envelope file \"" + m_envelope_path.str() + "\"" ) ;
	}

	// copy the envelope into subdirectories
	//
	process_envelope() ;
}

void Filter::throwFailures( bool one )
{
	if( ! m_failures.empty() )
	{
		std::ostringstream ss ;
		if( one )
		{
			ss << "failed to copy envelope file " << m_envelope_path.str() << " into " ;
		}
		else
		{
			ss << "failed to copy one or more envelope files into " ;
		}
		if( m_failures.size() == 1U )
		{
			ss << "the \"" << *m_failures.begin() << "\" sub-directory" ;
		}
		else
		{
			ss << m_failures.size() << " sub-directories, including \"" << *m_failures.begin() << "\"" ;
		}
		throw FilterError( ss.str() ) ;
	}
	if( one && m_directory_count == 0 ) // probably a permissioning problem
	{
		throw FilterError( "no sub-directories to copy into" , G::is_windows() ? "" : "check permissions" ) ;
	}
}

static bool run_one( const std::string & content )
{
	// copy the envelope
	//
	Filter filter ;
	filter.process_content( content ) ;
	filter.throwFailures( true ) ;
	return filter.envelopeDeleted() ;
}

static bool run_all( const std::string & spool_dir , bool verbose )
{
	Filter filter( verbose ) ;
	G::Directory dir( spool_dir ) ;
	G::DirectoryIterator iter( dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( !iter.isDir() && G::Str::headMatch(iter.fileName(),"emailrelay") && iter.filePath().extension() == "envelope" )
		{
			filter.setEnvelope( iter.fileName() , iter.filePath() ) ;
			filter.process_envelope() ;
		}
	}
	filter.throwFailures( false ) ;
	return filter.ok() ;
}

int main( int argc , char * argv [] )
{
	bool fancy = true ;
	try
	{
		G::Arg args( argc , argv ) ;

		if( args.c() <= 1U )
			throw FilterError( "usage error: must be run by emailrelay with the full path of a message content file" ) ;

		if( args.contains("-d",1U) && args.remove("-d",0U) )
		{
			fancy = false ;
			bool verbose = args.remove( "-v" ) ;
			return run_all( args.v(1U) , verbose ) ? 1 : 0 ;
		}
		else if( args.contains("--spool-dir",1U) && args.remove("--spool-dir",0U) )
		{
			fancy = false ;
			bool verbose = args.remove( "-v" ) ;
			return run_all( args.v(1U) , verbose ) ? 1 : 0 ;
		}
		else if( args.v(1U) == "--help" )
		{
			help( args.prefix() ) ;
			return 1 ;
		}
		else
		{
			return run_one( args.v(1U) ) ? 100 : 0 ;
		}
	}
	catch( std::exception & e )
	{
		if( fancy )
		{
			std::cout << "<<filter failed>>" << std::endl ;
			std::cout << "<<" << e.what() << ">>" << std::endl ;
		}
		else
		{
			std::cerr << e.what() << std::endl ;
		}
	}
	catch(...)
	{
		if( fancy )
		{
			std::cout << "<<filter failed>>" << std::endl ;
			std::cout << "<<" << "exception" << ">>" << std::endl ;
		}
		else
		{
			std::cerr << "error: exception" << std::endl ;
		}
	}
	return 1 ;
}

