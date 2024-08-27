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
/// \file legal.cpp
///

#include "gdef.h"
#include "legal.h"
#include <string>
#include <sstream>

std::string Main::Legal::copyright()
{
	return "Copyright (C) 2001-2024 Graeme Walker" ;
}

std::string Main::Legal::warranty( const std::string & prefix , const std::string & eol )
{
	std::ostringstream ss ;
	ss
		<< prefix << "This program comes with ABSOLUTELY NO WARRANTY." << eol
		<< prefix << "This is free software, and you are welcome to " << eol
		<< prefix << "redistribute it under certain conditions. For " << eol
		<< prefix << "more information refer to the file named COPYING." << eol ;
	return ss.str() ;
}

