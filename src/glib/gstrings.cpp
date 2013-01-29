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
// gstrings.cpp
//

#include "gdef.h"
#include "gstrings.h"
#include "gstr.h"
#include <string>
#include <list>
#include <map>
#include <stdexcept>

G::StringMapReader::StringMapReader( const G::StringMap & map_ ) : 
	m_map(map_) 
{
}

const std::string & G::StringMapReader::at( const std::string & key ) const
{
	StringMap::const_iterator p = m_map.find( key ) ;
	if( p == m_map.end() ) 
		throw std::out_of_range(std::string()+"key ["+key+"] not found in ["+G::Str::join(keys(14U,"..."),",")+"]") ;
	return (*p).second ;
}

const std::string & G::StringMapReader::at( const std::string & key , const std::string & default_ ) const
{ 
	StringMap::const_iterator p = m_map.find( key ) ;
	return p == m_map.end() ? default_ : (*p).second ;
}

G::Strings G::StringMapReader::keys( unsigned int limit , const char * elipsis ) const
{
	Strings result ;
	unsigned int i = 0U ;
	StringMap::const_iterator p = m_map.begin() ;
	for( ; p != m_map.end() && limit > 0U && i < limit ; ++p , i++ )
		result.push_back( (*p).first ) ;
	if( p != m_map.end() && elipsis != NULL )
		result.push_back( elipsis ) ;
	return result ;
}

/// \file gstrings.cpp
