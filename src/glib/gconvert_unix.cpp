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
/// \file gconvert_none.cpp
///

#include "gdef.h"
#include "gconvert.h"
#include "glog.h"

std::wstring G::Convert::widen( const std::string & s , bool utf8 , const std::string & /*context*/ )
{
	if( utf8 ) G_WARNING_ONCE( "G::Convert::widen: utf8 character-set conversions not supported" ) ;
	std::wstring result ;
	for( char c : s )
		result.append( 1U , static_cast<wchar_t>(c) ) ;
	return result ;
}

std::string G::Convert::narrow( const std::wstring & s , bool utf8 , const std::string & /*context*/ )
{
	if( utf8 ) G_WARNING_ONCE( "G::Convert::narrow: utf8 character-set conversions not supported" ) ;
	std::string result ;
	for( wchar_t wc : s )
		result.append( 1U , static_cast<char>(wc) ) ;
	return result ;
}

