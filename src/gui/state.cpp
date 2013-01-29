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
// state.cpp
//

#include "gdef.h"
#include "state.h"
#include "gstrings.h"
#include "gstr.h"
#include "gpath.h"
#include "glog.h"

State::State( const Map & config_map_in , const Map & dir_map_in )
{
	G::StringMapReader dir_map( dir_map_in ) ;
	m_map["dir-install"] = dir_map.at("dir-install") ;
	m_map["dir-config"] = dir_map.at("dir-config") ;
	m_map["dir-spool"] = dir_map.at("dir-spool") ;
	m_map["dir-pid"] = dir_map.at("dir-pid") ;
	m_map["dir-boot"] = dir_map.at("dir-boot") ;
	m_map["dir-desktop"] = dir_map.at("dir-desktop") ;
	m_map["dir-menu"] = dir_map.at("dir-menu") ;
	m_map["dir-login"] = dir_map.at("dir-login") ;

	for( Map::const_iterator p = config_map_in.begin() ; p != config_map_in.end() ; p++ )
	{
		std::string key = (*p).first ;
		std::string value = (*p).second ;
		if( key.length() > 4U && key.find("gui-") == 0U )
		{
			m_map[key.substr(4U)] = value ;
		}
	}
}

std::string State::value( const std::string & key , const std::string & default_ ) const
{
	Map::const_iterator p = m_map.find( key ) ;
	std::string result = ( p == m_map.end() || (*p).second.empty() ) ? default_ : (*p).second ;
	G_DEBUG( "State::value: [" << key << "]=\"" << result << "\"" ) ;
	return result ;
}

std::string State::value( const std::string & key , const char * default_ ) const
{
	return value( key , std::string(default_) ) ;
}

G::Path State::value( const std::string & key , const G::Path & default_ ) const
{
	return G::Path( value(key,default_.str()) ) ;
}

bool State::value( const std::string & key , bool default_ ) const
{
	std::string s = value( key , default_ ? "Y" : "N" ) ;
	return !s.empty() && ( s.at(0U) == 'y' || s.at(0U) == 'Y' ) ;
}

/// \file state.cpp
