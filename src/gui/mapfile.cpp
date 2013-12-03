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
// mapfile.cpp
//

#include "gdef.h"
#include "mapfile.h"
#include "gstr.h"
#include "gconvert.h"
#include "gpath.h"
#include "gprocess.h"
#include "gdatetime.h"
#include "gdate.h"
#include "gtime.h"
#include "gfile.h"
#include "glog.h"
#include <iterator>

G::StringMap MapFile::read( std::istream & ss )
{
	G::StringMap map ;
	read( map , ss , false , false ) ;
	return map ;
}

void MapFile::read( G::StringMap & map , std::istream & ss , bool underscore_to_dash , bool to_lower ,
	const std::string & section_prefix , bool in_section_predicate )
{
	G_DEBUG( "MapFile::read: start" ) ;
	std::string line ;
	while( ss.good() )
	{
		G::Str::readLineFrom( ss , "\n" , line ) ;
		if( line.empty() )
			continue ;
		if( !ss )
			break ;

		std::string::size_type pos_interesting = line.find_first_not_of(" \t\r#") ;
		if( pos_interesting == std::string::npos )
			continue ;

		std::string::size_type pos_hash = line.find("#") ;
		if( pos_hash != std::string::npos && pos_hash < pos_interesting )
			continue ;

		//std::string::size_type pos_assign = line.find('=') ;
		//if( pos_assign == std::string::npos )
			//continue ;

		bool in_section = section_prefix.empty() || line.find(section_prefix) == 0U ;
		if( in_section != in_section_predicate )
			continue ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " =\t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = part.size() == 1U ? std::string() : line.substr(part[0].length()+1U) ;
		value = G::Str::trimmed( value , G::Str::ws() ) ;
		if( value.length() >= 2U && value.at(0U) == '"' && value.at(value.length()-1U) == '"' )
			value = value.substr(1U,value.length()-2U) ;

		std::string key_in = part[0] ;
		std::string key = key_in ;
		if( underscore_to_dash )
			G::Str::replaceAll( key , "_" , "-" ) ;
		if( to_lower )
			G::Str::toLower( key ) ;
		G_DEBUG( "MapFile::read: " << key << "=[" << G::Str::printable(value) << "]" ) ;
// TODO - ifdef
#ifdef G_WINDOWS
		G::Convert::convert( value , G::Convert::utf8(value) , G::Convert::ThrowOnError() ) ;
#endif
		map[key] = value ;
	}
	G_DEBUG( "MapFile::read: end" ) ;
}

void MapFile::writeItem( std::ostream & stream , const std::string & key , const std::string & value )
{
	const char * qq = value.find(' ') == std::string::npos ? "" : "\"" ;
// TODO - ifdef
#ifdef G_WINDOWS
	G::Convert::utf8 utf8_value ;
	G::Convert::convert( utf8_value , value ) ;
	stream << key << "=" << qq << utf8_value.s << qq << "\n" ;
#else
	stream << key << "=" << qq << value << qq << "\n" ;
#endif
}

std::string MapFile::quote( const std::string & s )
{
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

void MapFile::edit( const G::Path & path , const G::StringMap & map_in , const std::string & section_prefix ,
	bool in_section_predicate , const G::StringMap & stop_list , bool make_backup , bool allow_read_error ,
	bool allow_write_error )
{
	typedef std::list<std::string> List ;
	G::StringMap map = purge( map_in , stop_list ) ;
	List lines = MapFile::lines( path , allow_read_error ) ;
	commentOut( lines , section_prefix , in_section_predicate ) ;
	replace( lines , map ) ;
	if( make_backup ) backup( path ) ;
	save( path , lines , allow_write_error ) ;
}

G::StringMap MapFile::purge( const G::StringMap & map_in , const G::StringMap & stop_list )
{
	G::StringMap result ;
	for( G::StringMap::const_iterator p = map_in.begin() ; p != map_in.end() ; ++p )
	{
		if( stop_list.find( (*p).first ) == stop_list.end() )
			result.insert( *p ) ;
	}
	return result ;
}

MapFile::List MapFile::lines( const G::Path & path , bool allow_read_error )
{
	List line_list ;
	std::ifstream file_in( path.str().c_str() ) ;
	if( !file_in.good() && !allow_read_error )
		throw std::runtime_error( std::string() + "cannot read \"" + path.str() + "\"" ) ;
	while( file_in.good() )
	{
		std::string line = G::Str::readLineFrom( file_in , "\n" ) ;
		if( !file_in ) break ;
		line_list.push_back( line ) ;
	}
	return line_list ;
}

void MapFile::commentOut( List & line_list , const std::string & section_prefix , bool in_section_predicate )
{
	for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		std::string & line = *line_p ;
		if( line.empty() || line.at(0U) == '#' )
			continue ;
		bool in_section = section_prefix.empty() || line.find(section_prefix) == 0U ;
		if( in_section == in_section_predicate )
		{
			line = std::string(1U,'#') + line ;
		}
	}
}

void MapFile::replace( List & line_list , const G::StringMap & map )
{
	for( G::StringMap::const_iterator map_p = map.begin() ; map_p != map.end() ; ++map_p )
	{
		bool found = false ;
		for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
		{
			std::string & line = (*line_p) ;
			if( line.empty() ) continue ;
			G::StringArray part ;
			G::Str::splitIntoTokens( line , part , G::Str::ws()+"#" ) ;
			if( part.size() == 0U ) continue ;
			if( part.at(0U) == (*map_p).first )
			{
				line = (*map_p).first + " " + quote((*map_p).second) ;
				found = true ;
				break ;
			}
		}

		if( !found )
		{
			line_list.push_back( (*map_p).first + " " + quote((*map_p).second) ) ;
		}
	}
}

void MapFile::backup( const G::Path & path )
{
	// ignore errors
	G::DateTime::BrokenDownTime now = G::DateTime::local( G::DateTime::now() ) ;
	std::string timestamp = G::Date(now).string(G::Date::yyyy_mm_dd) + G::Time(now).hhmmss() ;
	G::Path backup( path.dirname() , path.basename() + "." + timestamp ) ;
	G::Process::Umask umask( G::Process::Umask::Tightest ) ;
	G::File::copy( path , backup , G::File::NoThrow() ) ;
}

void MapFile::save( const G::Path & path , List & line_list , bool allow_write_error )
{
	std::ofstream file_out( path.str().c_str() ) ;
	std::copy( line_list.begin() , line_list.end() , std::ostream_iterator<std::string>(file_out,"\n") ) ;
	if( !file_out.good() && !allow_write_error )
		throw std::runtime_error( std::string() + "cannot write \"" + path.str() + "\"" ) ;
}

/// \file mapfile.cpp
