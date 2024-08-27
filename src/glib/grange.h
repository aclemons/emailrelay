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
/// \file grange.h
///

#ifndef G_RANGE_H
#define G_RANGE_H

#include "gdef.h"
#include "gstringview.h"
#include "gstr.h"
#include "gexception.h"

namespace G
{
	namespace Range /// Utility functions for pair-of-integer ranges.
	{
		inline std::pair<int,int> range( std::string_view spec_part )
		{
			if( spec_part.empty() )
			{
				return { -1 , -1 } ;
			}
			else if( spec_part.find('-') == std::string::npos )
			{
				int value = static_cast<int>( Str::toUInt( spec_part ) ) ;
				return { value , value } ;
			}
			else
			{
				return {
					static_cast<int>(Str::toUInt(Str::headView(spec_part,"-",false))) ,
					static_cast<int>(Str::toUInt(Str::tailView(spec_part,"-",true)))
				} ;
			}
		}
		inline std::pair<int,int> range( int a , int b ) { return { a , b } ; }
		inline std::pair<int,int> from( int n ) { return { n , -1 } ; }
		inline std::pair<int,int> none() { return { -1 , -1 } ; }
		inline std::pair<int,int> all() { return { 0 , -1 } ; }
		inline std::string str( std::pair<int,int> range , int big = 9999 )
		{
			return Str::fromInt(range.first).append(1U,'-').append(Str::fromInt(range.second<0?big:range.second)) ;
		}
		inline bool within( std::pair<int,int> range , int n )
		{
			return n >= 0 && n >= range.first && ( range.second < 0 || n <= range.second ) ;
		}
		inline void check( std::string_view spec )
		{
    		if( !spec.empty() )
    		{
        		auto r = range( spec ) ; // throws on error -- see G::Str::toUInt()
        		if( r.first != -1 && r.second < r.first ) // eg. "1000-900"
            		throw Exception( "not a valid numeric range" ) ;
    		}
		}
	}
}

#endif
