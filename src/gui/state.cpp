//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// state.cpp
//

#include "gstr.h"
#include "gpath.h"
#include "gstrings.h"
#include "glog.h"
#include "state.h"

G::Path State::file( const std::string & argv0_in )
{
	G::Path argv0( argv0_in ) ;
	std::string ext = argv0.basename().find('.') == std::string::npos ? ".state" : "" ;
	argv0.removeExtension() ;
	std::string name = argv0.basename() ;
	name.append( ext ) ;
	return G::Path( G::Path(argv0_in).dirname() , name ) ;
}

void State::write( std::ostream & stream , const std::string & contents , const G::Path & exe )
{
	stream << "#!/bin/sh\n" << contents ;
	if( exe != G::Path() )
		stream << "exec \"`dirname \\\"$0\\\"`/" << exe.basename() << "\" \"$@\"\n" ;
}

void State::write( std::ostream & stream , const Map & map , const G::Path & exe , const std::string & stop )
{
	stream << "#!/bin/sh\n" ;
	for( Map::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		if( stop.empty() || G::Str::lower((*p).first).find(G::Str::lower(stop)) == std::string::npos )
			write( stream , (*p).first , (*p).second , "" , "\n" ) ;
	}
	if( exe != G::Path() )
		stream << "exec \"`dirname \\\"$0\\\"`/" << exe.basename() << "\" \"$@\"\n" ;
}

void State::write( std::ostream & stream , const std::string & key , const std::string & value , 
	const std::string & prefix , const std::string & eol )
{
	std::string k = key ;
	G::Str::replaceAll( k , "-" , "_" ) ;
	G::Str::toUpper( k ) ;
	const char * qq = value.find(' ') == std::string::npos ? "" : "\"" ;
	stream << prefix << k << "=" << qq << value << qq << eol ;
}

State::Map State::read( std::istream & ss )
{
	Map map ;
	std::string line ;
	while( ss.good() )
	{
		G::Str::readLineFrom( ss , "\n" , line ) ;
		if( line.empty() || line.find('#') == 0U || line.find_first_not_of(" \t\r") == std::string::npos )
			continue ;

		if( !ss )
			break ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " =\t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = part.size() == 1U ? std::string() : line.substr(part[0].length()+1U) ;
		value = G::Str::trimmed( value , G::Str::ws() ) ;
		if( value.length() >= 2U && value.at(0U) == '"' && value.at(value.length()-1U) == '"' )
			value = value.substr(1U,value.length()-2U) ;

		std::string key = part[0] ;
		G_DEBUG( "State::read: \"" << key << "\" = \"" << value << "\"" ) ;
		map.insert( Map::value_type(key,value) ) ;
	}
	return map ;
}

State::State( const Map & map ) :
	m_map(map)
{
}

G::Path State::value( const std::string & key , const G::Path & default_ ) const
{
	return G::Path( value(key,default_.str()) ) ;
}

std::string State::value( const std::string & key , const std::string & default_ ) const
{
	std::string k = key ;
	G::Str::replaceAll( k , "-" , "_" ) ;
	G::Str::toUpper( k ) ;

	Map::const_iterator p = m_map.find( k ) ;
	std::string result = p == m_map.end() ? default_ : (*p).second ;
	G_DEBUG( "State::value: [" << key << "]=\"" << result << "\"" ) ;
	return result ;
}

std::string State::value( const std::string & key , const char * default_ ) const
{
	return value( key , std::string(default_) ) ;
}

bool State::value( const std::string & key , bool default_ ) const
{
	std::string s = value( key , default_ ? "Y" : "N" ) ;
	return !s.empty() && ( s.at(0U) == 'y' || s.at(0U) == 'Y' ) ;
}

/// \file state.cpp
