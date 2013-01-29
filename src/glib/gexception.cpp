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
// gexception.cpp
//

#include "gdef.h"
#include "gexception.h"

G::Exception::Exception()
{
}

G::Exception::Exception( const char * what ) :
	m_what(what?what:"")
{
}

G::Exception::Exception( const std::string & what ) :
	m_what(what)
{
}

G::Exception::Exception( const char * what , const std::string & more ) :
	m_what(what)
{
	append( more ) ;
}

G::Exception::Exception( const std::string & what , const std::string & more ) :
	m_what(what)
{
	append( more ) ;
}

G::Exception::Exception( const std::string & what , const std::string & more1 , const std::string & more2 ) :
	m_what(what)
{
	append( more1 ) ;
	append( more2 ) ;
}


G::Exception::~Exception() throw()
{
}

const char * G::Exception::what() const throw()
{
	return m_what.c_str() ;
}

void G::Exception::append( const char * more )
{
	if( more != NULL && *more != '\0' )
	{
		m_what += std::string(": ") ;
		m_what += std::string(more) ;
	}
}

void G::Exception::append( const std::string & more )
{
	if( !more.empty() )
	{
		m_what += std::string(": ") ;
		m_what += std::string(more) ;
	}
}

void G::Exception::prepend( const char * context )
{
	if( context != NULL && *context != '\0' )
	{
		m_what = std::string(context) + ": " + m_what ;
	}
}

/// \file gexception.cpp
