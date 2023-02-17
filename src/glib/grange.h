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
/// \file grange.h
///

#ifndef G_RANGE_H
#define G_RANGE_H

#include "gdef.h"
#include "gstr.h"
#include "gexception.h"

namespace G
{
	namespace Range /// Utility functions for pair-of-integer ranges.
	{
		inline std::pair<int,int> range( const std::string & spec_part )
		{
			if( spec_part.empty() )
			{
				return { -1 , -1 } ;
			}
			else if( spec_part.find('-') == std::string::npos )
			{
				int value = static_cast<int>( G::Str::toUInt( spec_part ) ) ;
				return { value , value } ;
			}
			else
			{
				return {
					static_cast<int>(G::Str::toUInt(G::Str::head(spec_part,"-",false))) ,
					static_cast<int>(G::Str::toUInt(G::Str::tail(spec_part,"-",true)))
				} ;
			}
		}
		inline std::pair<int,int> range( int a , int b ) { return { a , b } ; }
		inline std::pair<int,int> from( int n ) { return { n , -1 } ; }
		inline std::pair<int,int> none() { return { -1 , -1 } ; }
		inline std::pair<int,int> all() { return { 0 , -1 } ; }
		inline std::string str( std::pair<int,int> range )
		{
			return G::Str::fromInt(range.first).append(1U,'-').append(G::Str::fromInt(range.second<0?9999:range.second)) ;
		}
		inline bool within( std::pair<int,int> range , int uid )
		{
			return uid >= 0 && uid >= range.first && ( range.second < 0 || uid <= range.second ) ;
		}
		inline void check( const std::string & spec )
		{
    		if( !spec.empty() )
    		{
        		auto r = range( spec ) ; // throws on error -- see G::Str::toUInt()
        		if( r.first != -1 && r.second < r.first ) // eg. "1000-900"
            		throw G::Exception( "not a valid numeric range" ) ;
    		}
		}
	}
}

#endif
