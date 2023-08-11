//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gconvert.cpp
///

#include "gdef.h"
#include "gconvert.h"

void G::Convert::convert( G::Convert::utf8 & out_ , const G::Convert::utf8 & s )
{
	out_ = s ; // utf8 -> utf8
}

void G::Convert::convert( std::string & out_ , const std::string & s )
{
	out_ = s ; // 8bit -> 8bit
}

void G::Convert::convert( std::string & out_ , const std::string & s , const ThrowOnError & )
{
	out_ = s ; // degenerate
}

void G::Convert::convert( std::wstring & out_ , const std::wstring & s )
{
	out_ = s ; // degenerate
}

void G::Convert::convert( G::Convert::utf8 & out_ , const std::string & s )
{
	out_ = utf8( narrow(widen(s,false),true) ) ; // 8bit -> utf16 -> utf8
}

void G::Convert::convert( G::Convert::utf8 & out_ , const std::wstring & s )
{
	out_ = utf8( narrow(s,true) ) ; // utf16 -> utf8
}

void G::Convert::convert( std::string & out_ , const G::Convert::utf8 & s , const ThrowOnError & e )
{
	out_ = narrow( widen(s.s,true) , false , e.context ) ; // utf8 -> utf16 -> 8bit
}

void G::Convert::convert( std::string & out_ , const std::wstring & s , const ThrowOnError & e )
{
	out_ = narrow( s , false , e.context ) ;
}

void G::Convert::convert( std::wstring & out_ , const std::string & s )
{
	out_ = widen( s , false ) ;
}

void G::Convert::convert( std::wstring & out_ , const G::Convert::utf8 & s )
{
	out_ = widen( s.s , true ) ;
}

