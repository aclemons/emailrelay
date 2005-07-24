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
// glinebuffer.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "glinebuffer.h"
#include "gdebug.h"
#include "gassert.h"

unsigned long GNet::LineBuffer::m_limit = 1000000UL ; // 1Mb denial-of-service line-length limit

GNet::LineBuffer::LineBuffer( const std::string & eol , bool do_throw ) :
	m_eol(eol) ,
	m_more(false) ,
	m_throw(do_throw)
{
}

void GNet::LineBuffer::add( const char * p , size_t n )
{
	G_ASSERT( p != NULL ) ;
	check( n ) ;
	m_store.append( p , n ) ;
	load() ;
}

void GNet::LineBuffer::add( const std::string & s )
{
	check( s.length() ) ;
	m_store.append( s ) ;
	load() ;
}

void GNet::LineBuffer::check( size_t n )
{
	if( (m_store.length()+n) > m_limit )
	{
		size_t total = m_store.size() + m_current.size() + n ;
		m_store.erase() ;
		m_current.erase() ;
		m_more = false ;
		if( m_throw )
			throw Overflow() ;
		else
			G_ERROR( "GNet::LineBuffer::check: line too long: discarding " << total << " bytes" ) ;
	}
}

void GNet::LineBuffer::load()
{
	if( ! m_more )
	{
		size_t pos = m_store.find(m_eol) ; // optimisation opportunity here using find(pos,s) overload
		if( pos != std::string::npos )
		{
			m_more = true ;
			m_current = m_store.substr(0U,pos) ;
			m_store.erase( 0U , pos + m_eol.length() ) ;
		}
	}
}

bool GNet::LineBuffer::more() const
{
	return m_more ;
}

const std::string & GNet::LineBuffer::current() const
{
	return m_current ;
}

void GNet::LineBuffer::discard()
{
	m_current.erase() ;
	m_more = false ;
	load() ;
}

std::string GNet::LineBuffer::line()
{
	std::string s( current() ) ;
	discard() ;
	return s ;
}

