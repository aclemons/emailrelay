//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// legal.cpp
//

#include "gdef.h"
#include "legal.h"
#include <string>
#include <sstream>

//static
std::string Main::Legal::copyright()
{
	return "Copyright (C) 2001-2005 Graeme Walker" ;
}

//static
std::string Main::Legal::warranty( const std::string & prefix , const std::string & eol )
{
	std::ostringstream ss ;
	ss
		<< prefix << "This software is provided without warranty of any kind." << eol
		<< prefix << "You may redistribure copies of this program under " << eol
		<< prefix << "the terms of the GNU General Public License." << eol
		<< prefix << "For more information refer to the file named COPYING." << eol ;
	return ss.str() ;
}

