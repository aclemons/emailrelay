//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glinebuffer.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "glinebuffer.h"
#include "gdebug.h"
#include "gassert.h"

unsigned long GNet::LineBuffer::m_limit = 10000000UL ; // 10Mb denial-of-service limit

GNet::LineBuffer::LineBuffer( const std::string & eol ) :
	m_eol(eol)
{
}

unsigned long GNet::LineBuffer::count() const
{
	unsigned long result = 0UL ;
	for( G::Strings::const_iterator p = m_lines.begin() ; p != m_lines.end() ; ++p )
		result += (*p).length() ;
	return result ;
}

void GNet::LineBuffer::add( const std::string & segment )
{
	if( m_limit == 0UL || count() < m_limit )
	{
		size_t n = segment.size() ;
		for( size_t i = 0U ; i < n ; i++ )
		{
			if( m_lines.empty() )
				m_lines.push_back( std::string() ) ;

			char c = segment.at(i) ;
			m_lines.back().append( 1U , c ) ;
			if( terminated() )
			{
				m_lines.push_back( std::string() ) ;
			}
		}
	}
}

bool GNet::LineBuffer::terminated() const
{
	G_ASSERT( ! m_lines.empty() ) ;
	const std::string & s = m_lines.back() ;
	if( s.length() < m_eol.length() )
	{
		return false ;
	}
	else
	{
		size_t pos = s.length() - m_eol.length() ;
		return s.find(m_eol,pos) == pos ;
	}
}

bool GNet::LineBuffer::more() const
{
	return 
		( m_lines.size() == 1U && terminated() ) || 
		m_lines.size() > 1U ;
}

std::string GNet::LineBuffer::line()
{
	G_ASSERT( ! m_lines.empty() ) ;
	std::string result = m_lines.front() ;
	size_t n = result.length() ;
	G_ASSERT( n >= m_eol.length() ) ;
	n -= m_eol.length() ;
	G_ASSERT( result.find(m_eol,n) == n ) ;
	result.erase( n , m_eol.length() ) ;
	m_lines.pop_front() ;
	return result ;
}

