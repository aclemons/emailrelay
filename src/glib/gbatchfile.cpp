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
/// \file gbatchfile.cpp
///

#include "gdef.h"
#include "gbatchfile.h"
#include "gcodepage.h"
#include "gfile.h"
#include "garg.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <stdexcept>
#include <fstream>

G::BatchFile::BatchFile( const Path & path )
{
	init( path ) ;
}

G::BatchFile::BatchFile( const Path & path , std::nothrow_t )
{
	try
	{
		init( path ) ;
	}
	catch( Error & )
	{
		clear() ;
	}
}

void G::BatchFile::init( const Path & path )
{
	std::ifstream stream ;
	File::open( stream , path , File::Text() ) ;
	if( !stream.good() )
		throw Error( "cannot open batch file" , path.str() ) ;
	m_raw_line = readFrom( stream , path.str() ) ;
	auto parse_result = parse( m_raw_line ) ;
	if( !parse_result.error.empty() )
		throw Error( parse_result.error , path.str() ) ;
	m_name = parse_result.name ;
	m_line = parse_result.line ;
	m_args = split( m_line ) ;
}

void G::BatchFile::clear()
{
	m_raw_line.clear() ;
	m_name.clear() ;
	m_line.clear() ;
	m_args.clear() ;
}

bool G::BatchFile::empty() const
{
	return m_line.empty() ;
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
	return !ignorable( trimmed_line ) ;
}

std::string G::BatchFile::readFrom( std::istream & stream , const std::string & stream_name , bool do_throw )
{
	std::string line ;
	while( stream.good() )
	{
		std::string s = Str::readLineFrom( stream ) ;
		if( !stream ) break ;
		Str::trim( s , Str::ws() ) ;
		Str::replaceAll( s , "\t" , " " ) ;
		s = Str::unique( s , ' ' ) ;
		if( relevant(s) )
		{
			if( line.empty() )
				line = s ;
			else if( do_throw )
				throw Error( "too many lines in batch file" , stream_name ) ;
			else
				return {} ;
		}
	}

	if( line.empty() )
	{
		if( do_throw )
			throw Error( "batch file is empty" , stream_name ) ;
		return {} ;
	}

	return line ;
}

G::BatchFile::ParseResult G::BatchFile::parse( const std::string & line_in )
{
	// strip off any "start" prefix -- allow the "start" to have a quoted window
	// title but dont expect any "start" options such as "/min"

	ParseResult result ;
	std::string line = line_in ;
	if( !line.empty() )
	{
		using size_type = std::string::size_type ;
		size_type const npos = std::string::npos ;
		std::string ws = sv_to_string( Str::ws() ) ;

		std::string start = "start " ;
		size_type start_pos = Str::lower(line).find(start) ;
		size_type command_pos = start_pos == npos ? 0U : line.find_first_not_of( ws , start_pos+start.size() ) ;

		bool named = start_pos != npos && line.at(command_pos) == '"' ;
		if( named )
		{
			std::size_t name_start_pos = command_pos ;
			std::size_t name_end_pos = line.find( '\"' , name_start_pos+1U ) ;
			if( name_end_pos == npos )
				return {{},{},"mismatched quotes in batch file"} ;
			if( (name_end_pos+2U) >= line.size() || line.at(name_end_pos+1U) != ' ' )
				return {{},{},"invalid window name in batch file"} ;

			result.name = line.substr( name_start_pos+1U , name_end_pos-(name_start_pos+1U) ) ;
			dequote( result.name ) ;
			Str::trim( result.name , Str::ws() ) ;

			command_pos = line.find_first_not_of( ws , name_end_pos+2U ) ;
		}

		if( command_pos != npos )
			line.erase( 0U , command_pos ) ;
	}

	// percent characters are doubled up in batch files so un-double them here
	Str::replaceAll( line , "%%" , "%" ) ;

	result.line = CodePage::fromCodePageOem( line ) ; // to UTF-8
	return result ;
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

std::size_t G::BatchFile::lineArgsPos() const
{
	constexpr char qq = '\"' ;
	std::size_t i = 0U ;
	for( bool in_quote = false ; i < m_line.size() ; i++ )
	{
		char c = m_line[i] ;
		if( c == qq && !in_quote )
			in_quote = true ;
		else if( c == qq )
			in_quote = false ;
		else if( Str::ws().find(c) != std::string::npos && !in_quote )
			break ;
	}
	return i ;
}

void G::BatchFile::dequote( std::string & s )
{
	if( s.size() >= 2U && s.find('\"') == 0U && (s.rfind('\"')+1U) == s.size() )
		s = s.substr( 1U , s.size()-2U ) ;
}

void G::BatchFile::write( const Path & path , const StringArray & args , const std::string & name_in , bool make_backup )
{
	G_ASSERT( !args.empty() ) ;
	if( args.empty() )
		throw Error( "invalid contents for startup batch file" ) ; // need at least the executable

	std::string name = name_in ;
	if( name.empty() )
	{
		name = args.at(0U) ;
		dequote( name ) ;
		name = Path(name).withoutExtension().basename() ;
	}

	std::string start_line ;
	{
		std::ostringstream ss ;
		ss << "start \"" << CodePage::toCodePageOem(name) << "\"" ;
		for( const auto & arg : args )
		{
			ss << " " << percents(quote(CodePage::toCodePageOem(arg))) ;
		}
		start_line = ss.str() ;
	}

	if( make_backup )
	{
		BatchFile on_disk( path , std::nothrow ) ;
		if( start_line != on_disk.m_raw_line )
			G::File::backup( path , std::nothrow ) ;
	}

	std::ofstream stream ;
	File::open( stream , path ) ;
	if( !stream.good() )
		throw Error( "cannot create batch file" , path.str() ) ;

	stream << start_line << "\r\n" ;
	stream.close() ;
	if( stream.fail() )
		throw Error( "cannot write batch file" , path.str() ) ;
}

G::StringArray G::BatchFile::split( const std::string & line )
{
	// get G::Arg to deal with the quotes
	return Arg(line).array() ;
}

std::string G::BatchFile::percents( const std::string & s )
{
	std::string result( s ) ;
	Str::replaceAll( result , "%" , "%%" ) ;
	return result ;
}

std::string G::BatchFile::quote( const std::string & s )
{
	return
		s.find('\"') == std::string::npos && s.find_first_of(" \t") != std::string::npos ?
			"\"" + s + "\"" : s ;
}

