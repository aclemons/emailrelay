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
// glinebuffer.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gnet.h"
#include "glinebuffer.h"
#include "gdebug.h"
#include "gassert.h"

unsigned long GNet::LineBuffer::m_limit = G::limits::net_line_limit ; // denial-of-service line-length limit

GNet::LineBuffer::LineBuffer( const std::string & eol , bool do_throw ) :
	m_eol(eol) ,
	m_eol_length(eol.length()) ,
	m_p(std::string::npos) ,
	m_current_valid(false) ,
	m_throw(do_throw) ,
	m_locked(false)
{
	G_ASSERT( !eol.empty() ) ;
}

void GNet::LineBuffer::lock()
{
	m_locked = true ;
}

void GNet::LineBuffer::unlock( std::string::size_type n )
{
	m_locked = false ;
	m_current_valid = false ;
	m_store.erase( 0U , n ) ;
	if( m_p != std::string::npos )
		m_p -= n ;
}

void GNet::LineBuffer::add( const char * p , std::string::size_type n )
{
	G_ASSERT( p != NULL ) ;
	check( n ) ;
	m_store.append( p , n ) ;
	fix( n ) ;
}

void GNet::LineBuffer::add( const std::string & s )
{
	check( s.length() ) ;
	m_store.append( s ) ;
	fix( s.length() ) ;
}

void GNet::LineBuffer::check( std::string::size_type n )
{
	G_ASSERT( !m_locked ) ;
	if( (m_store.length()+n) > m_limit )
	{
		std::string::size_type total = m_store.size() + m_current.size() + n ;

		m_p = std::string::npos ;
		m_current.erase() ;
		m_current_valid = false ;
		m_store.erase() ;

		if( m_throw )
			throw Overflow() ;
		else
			G_ERROR( "GNet::LineBuffer::check: line too long: discarding " << total << " bytes" ) ;
	}
}

void GNet::LineBuffer::fix( std::string::size_type n )
{
	if( m_p == std::string::npos )
	{
		std::string::size_type start = m_store.length() - n ;
		start = (start+1U) > m_eol_length ? (start+1U-m_eol_length) : 0U ;
		m_p = m_store.find( m_eol , start ) ;
	}
}

bool GNet::LineBuffer::more() const
{
	return m_p != std::string::npos ;
}

const std::string & GNet::LineBuffer::current() const
{
	G_ASSERT( m_p != std::string::npos ) ;
	G_ASSERT( !m_locked ) ;

	if( ! m_current_valid )
	{
		LineBuffer * this_ = const_cast<LineBuffer*>(this) ;
		this_->m_current = std::string( m_store , 0U , m_p ) ;
		this_->m_current_valid = true ;
	}
	return m_current ;
}

void GNet::LineBuffer::discard()
{
	G_ASSERT( m_p != std::string::npos ) ;
	G_ASSERT( !m_locked ) ;

	m_current.erase() ;
	m_current_valid = false ;
	m_store.erase( 0U , m_p + m_eol_length ) ;
	m_p = m_store.find( m_eol ) ;
}

std::string GNet::LineBuffer::line()
{
	std::string s( current() ) ;
	discard() ;
	return s ;
}

// ==

bool GNet::LineBufferIterator::more() const
{
	return m_b.m_p != std::string::npos ;
}

const std::string & GNet::LineBufferIterator::line()
{
	// this is optimised code -- the optimisation applies in high-throughput 
	// situations when the network code dumps in more than one line of
	// text from each network event -- the discard() above is effectively
	// done once per add() rather than once per line
	const std::string::size_type n = m_b.m_p - m_n ;
	m_b.m_current.resize( n ) ;
	m_b.m_current.replace( 0U , n , m_b.m_store.data() + m_n , n ) ;
	m_n = m_b.m_p + m_b.m_eol_length ;
	m_b.m_p = m_n == m_store_length ? std::string::npos : m_b.m_store.find(m_b.m_eol,m_n) ;
	return m_b.m_current ;
}

/// \file glinebuffer.cpp
