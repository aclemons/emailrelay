//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexception.cpp
//

#include "gdef.h"
#include "gexception.h"

namespace G
{
	namespace ExceptionImp
	{
		std::string join( std::string s1 , const std::string & s2 )
		{
			return s1.append(": ").append(s2) ;
		}
		std::string join( std::string s1 , const std::string & s2 , const std::string & s3 )
		{
			return s1.append(": ").append(s2).append(": ").append(s3) ;
		}
		std::string join( std::string s1 , const std::string & s2 , const std::string & s3 , const std::string & s4 )
		{
			return s1.append(": ").append(s2).append(": ").append(s3).append(": ").append(s4) ;
		}
		std::string join( std::string s1 , const std::string & s2 , const std::string & s3 , const std::string & s4 , const std::string & s5 )
		{
			return s1.append(": ").append(s2).append(": ").append(s3).append(": ").append(s4).append(": ").append(s5) ;
		}
	}
}

G::Exception::Exception( const char * what ) :
	std::runtime_error(what?what:"")
{
}

G::Exception::Exception( const std::string & what ) :
	std::runtime_error(what)
{
}

G::Exception::Exception( const char * what , const std::string & more ) :
	std::runtime_error(ExceptionImp::join(what,more))
{
}

G::Exception::Exception( const std::string & what , const std::string & more ) :
	std::runtime_error(ExceptionImp::join(what,more))
{
}

G::Exception::Exception( const char * what , const std::string & more1 , const std::string & more2 ) :
	std::runtime_error(ExceptionImp::join(what,more1,more2))
{
}

G::Exception::Exception( const std::string & what , const std::string & more1 , const std::string & more2 ) :
	std::runtime_error(ExceptionImp::join(what,more1,more2))
{
}

G::Exception::Exception( const char * what , const std::string & more1 , const std::string & more2 , const std::string & more3 ) :
	std::runtime_error(ExceptionImp::join(what,more1,more2,more3))
{
}

G::Exception::Exception( const std::string & what , const std::string & more1 , const std::string & more2 , const std::string & more3 ) :
	std::runtime_error(ExceptionImp::join(what,more1,more2,more3))
{
}
/// \file gexception.cpp
