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
/// \file gformat.cpp
///

#include "gdef.h"
#include "gformat.h"
#include "gstr.h"

#ifndef G_LIB_SMALL
G::format::format( const std::string & fmt ) :
	m_fmt(fmt)
{
}
#endif

G::format::format( const char * fmt ) :
	m_fmt(fmt)
{
}

#ifndef G_LIB_SMALL
G::format & G::format::parse( const std::string & fmt )
{
	m_fmt = fmt ;
	m_i = 0U ;
	m_values.clear() ;
	return *this ;
}
#endif

#ifndef G_LIB_SMALL
G::format & G::format::parse( const char * fmt )
{
	m_fmt = fmt ;
	m_i = 0U ;
	m_values.clear() ;
	return *this ;
}
#endif

bool G::format::isdigit( char c ) noexcept
{
	// std::isdigit( static_cast<unsigned char>(c) )
	return c >= '0' && c <= '9' ;
}

std::string G::format::str() const
{
	std::string s = m_fmt ;
	const std::size_t npos = std::string::npos ;
	for( std::size_t p = s.find('%') ; p != npos && (p+2U) < s.size() ; )
	{
		std::size_t q = s.find( '%' , p+1 ) ;
		if( q != npos && q == (p+2U) && isdigit(s.at(p+1U)) ) // kiss 1..9 only
		{
			auto n = G::Str::toUInt( s.substr(p+1,1U) ) ;
			if( n && n <= m_values.size() )
			{
				s.replace( p , 3U , m_values.at(n-1U) ) ;
				p += m_values.at(n-1U).size() ;
			}
			else
			{
				s.erase( p , 3U ) ;
			}
		}
		else
		{
			p++ ;
		}
		p = p < s.size() ? s.find('%',p) : npos ;
	}
	return s ;
}

#ifndef G_LIB_SMALL
std::size_t G::format::size() const
{
	return str().size() ;
}
#endif

void G::format::apply( const std::string & value )
{
	m_values.push_back( value ) ;
}

std::ostream & G::operator<<( std::ostream & stream , const format & f )
{
	return stream << f.str() ;
}

