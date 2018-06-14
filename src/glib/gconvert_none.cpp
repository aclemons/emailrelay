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
// gconvert_none.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include "glog.h"

namespace
{
	void warn_if( bool b )
	{
		static bool warned = false ;
		if( b && !warned )
		{
			G_WARNING( "G::Convert::convert: no iconv library built in: character set conversions will be incorrect" ) ;
			warned = true ;
		}
	}
}

std::wstring G::Convert::widen( const std::string & s , bool utf8 , const std::string & /*context*/ )
{
	warn_if( utf8 ) ;
	std::wstring result ;
	for( std::string::const_iterator p = s.begin() ; p != s.end() ; ++p )
		result.append( 1U , static_cast<wchar_t>(*p) ) ;
	return result ;
}

std::string G::Convert::narrow( const std::wstring & s , bool utf8 , const std::string & /*context*/ )
{
	warn_if( utf8 ) ;
	std::string result ;
	for( std::wstring::const_iterator p = s.begin() ; p != s.end() ; ++p )
		result.append( 1U , static_cast<char>(*p) ) ;
	return result ;
}

/// \file gconvert_none.cpp
