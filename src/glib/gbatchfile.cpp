//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gbatchfile.cpp
//

#include "gdef.h"
#include "gbatchfile.h"
#include "garg.h"
#include "gstr.h"
#include "gdebug.h"
#include <stdexcept>
#include <fstream>

G::BatchFile::BatchFile( const G::Path & path )
{
	std::ifstream stream( path.str().c_str() ) ;
	if( !stream.good() )
		throw Error( "cannot open batch file" , path.str() ) ;
	m_line = readFrom( stream , path.str() , true ) ;
	m_args = split( m_line ) ;
}

G::BatchFile::BatchFile( const G::Path & path , NoThrow )
{
	std::ifstream stream( path.str().c_str() ) ;
	if( stream.good() )
	{
		m_line = readFrom( stream , path.str() , false ) ;
		m_args = split( m_line ) ;
	}
}

G::BatchFile::BatchFile( std::istream & stream , const std::string & name )
{
	m_line = readFrom( stream , name , true ) ;
	m_args = split( m_line ) ;
}

bool G::BatchFile::ignorable( const std::string & trimmed_line )
{
	return
		trimmed_line.empty() ||
		Str::lower(trimmed_line+" ").find("@echo ") == 0U ||
		Str::lower(trimmed_line+" ").find("rem ") == 0U ;
}

bool G::BatchFile::relevant( const std::string & trimmed_line )
{
	return ! ignorable( trimmed_line ) ;
}

std::string G::BatchFile::join( const std::string & file_name , unsigned int line_number )
{
	std::ostringstream ss ;
	ss << file_name << "(" << line_number << ")" ;
	return ss.str() ;
}

std::string G::BatchFile::readFrom( std::istream & stream , const std::string & stream_name , bool strict )
{
	std::string line ;
	unsigned int line_number = 1U ;
	for( ; stream.good() ; line_number++ )
	{
		std::string s = Str::readLineFrom( stream ) ;
		if( !stream ) break ;
		Str::trim( s , Str::ws() ) ;
		Str::replaceAll( s , "\t" , " " ) ;
		s = Str::unique( s ) ;
		if( relevant(s) )
		{
			if( line.empty() )
				line = s ;
			else
				throw Error( "too many lines in batch file" , join(stream_name,line_number) ) ;
		}
	}

	if( strict && line.empty() )
		throw Error( "batch file is empty" , join(stream_name,line_number) ) ;

	// strip off any "start" prefix -- allow the "start" to have a quoted window
	// title but dont expect any "start" options such as "/min"
	if( !line.empty() )
	{
		typedef std::string::size_type size_type ;
		size_type const npos = std::string::npos ;

		std::string start = "start " ;
		size_type start_pos = Str::lower(line).find(start) ;
		size_type command_pos = start_pos == npos ? 0U : line.find_first_not_of(" ",start_pos+start.size()) ;

		bool named = start_pos != npos && line.at(command_pos) == '"' ;
		if( named )
		{
			size_t name_start_pos = command_pos ;
			size_t name_end_pos = line.find( "\"" , name_start_pos+1U ) ;
			if( name_end_pos == npos )
				throw Error( "mismatched quotes in batch file" , stream_name ) ;
			if( (name_end_pos+2U) >= line.size() || line.at(name_end_pos+1U) != ' ' )
				throw Error( "invalid window name in batch file" , stream_name ) ;

			m_name = line.substr( name_start_pos+1U , name_end_pos-(name_start_pos+1U) ) ;
			dequote( m_name ) ;
			Str::trim( m_name , Str::ws() ) ;

			command_pos = line.find_first_not_of(" ",name_end_pos+2U) ;
		}

		if( command_pos != npos )
		{
			line.erase( 0U , command_pos ) ;
			Str::trimLeft( line , Str::ws() ) ;
		}
	}

	// percent characters are doubled up in batch files so un-double them here
	G::Str::replaceAll( line , "%%" , "%" ) ;

	return line ;
}

std::string G::BatchFile::line() const
{
	return m_line ;
}

std::string G::BatchFile::name() const
{
	return m_name ;
}

const G::StringArray & G::BatchFile::args() const
{
	return m_args ;
}

void G::BatchFile::dequote( std::string & s )
{
	if( s.size() >= 2U && s.find("\"") == 0U && (s.rfind("\"")+1U) == s.size() )
		s = s.substr( 1U , s.size()-2U ) ;
}

void G::BatchFile::write( const G::Path & path , const StringArray & args , const std::string & name_in )
{
	G_ASSERT( args.size() > 0U ) ;
	if( args.size() == 0U )
		throw Error( "invalid contents for startup batch file" ) ;

	std::string name = name_in ;
	if( name.empty() )
	{
		name = args.at(0U) ;
		dequote( name ) ;
		name = Path(name).withoutExtension().basename() ;
	}

	std::ofstream stream( path.str().c_str() , std::ios::binary | std::ios::out | std::ios::trunc ) ;
	if( !stream.good() )
		throw Error( "cannot create batch file" , path.str() ) ;

	stream << "start \"" << name << "\"" ;
	for( StringArray::const_iterator p = args.begin() ; p != args.end() ; ++p )
	{
		stream << " " << percents(quote(*p)) ;
	}
	stream << "\r\n" ;

	stream.close() ;
	if( stream.fail() )
		throw Error( "cannot write batch file" , path.str() ) ;
}

G::StringArray G::BatchFile::split( const std::string & line )
{
	// get G::Arg to deal with the quotes
	Arg args ;
	if( !line.empty() )
		args.parse( line ) ;
	return args.array() ;
}

std::string G::BatchFile::percents( const std::string & s )
{
	std::string result( s ) ;
	G::Str::replaceAll( result , "%" , "%%" ) ;
	return result ;
}

std::string G::BatchFile::quote( const std::string & s )
{
	return
		s.find("\"") == std::string::npos && s.find_first_of(" \t") != std::string::npos ?
			"\"" + s + "\"" : s ;
}

/// \file gbatchfile.cpp
