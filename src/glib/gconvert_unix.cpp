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
// gconvert_unix.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include <cstdlib> // mbstowcs(), wcstomcs()

#if 0
std::wstring G::Convert::widen( const std::string & s , bool utf8 )
{
	// TODO -- implement with mbstowcs() and libiconv
	return std::wstring() ;
}
std::string G::Convert::narrow( const std::wstring & s , bool utf8 )
{
	// TODO -- implement with wcstomcs() and libiconv
	return std::string() ;
}
#endif
/// \file gconvert_unix.cpp
