//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glinebuffer.h"
#include "gdebug.h"
#include "gassert.h"

static const unsigned long line_limit = G::limits::net_line_limit ;

GNet::LineBuffer::LineBuffer() :
	m_iterator(nullptr) ,
	m_auto(true) ,
	m_eol("\n") ,
	m_throw_on_overflow(false) ,
	m_expect(0U)
{
}

GNet::LineBuffer::LineBuffer( const std::string & eol , bool do_throw ) :
	m_iterator(nullptr) ,
	m_auto(false) ,
	m_eol(eol) ,
	m_throw_on_overflow(do_throw) ,
	m_expect(0U)
{
	G_ASSERT( !eol.empty() ) ;
}

size_t GNet::LineBuffer::lock( LineBufferIterator * iterator )
{
	if( m_iterator != nullptr ) throw Error("double lock") ;
	m_iterator = iterator ;

	detect() ;
	size_t expect = m_expect ;
	m_expect = 0U ;
	return expect ;
}

void GNet::LineBuffer::unlock( LineBufferIterator * iterator , size_t discard , size_t expect )
{
	if( m_iterator == nullptr || iterator != m_iterator ) throw Error("double unlock") ;
	m_iterator = nullptr ;
	m_expect = expect ;

	G_ASSERT( discard <= m_store.size() ) ;
	if( discard == m_store.size() )
		m_store.clear() ;
	else if( discard != 0U )
		m_store.erase( 0U , discard ) ;
}

void GNet::LineBuffer::add( const char * p , std::string::size_type n )
{
	if( check( n ) )
		m_store.append( p , n ) ;
}

void GNet::LineBuffer::add( const std::string & s )
{
	add( s.data() , s.size() ) ;
}

void GNet::LineBuffer::expect( size_t n )
{
	// pass it on, either now or at lock() time
	if( m_iterator == nullptr )
	{
		m_expect = n ;
	}
	else
	{
		m_expect = 0U ;
		m_iterator->expect( n ) ;
	}
}

void GNet::LineBuffer::detect()
{
	std::string::size_type pos = m_auto ? m_store.find('\n') : std::string::npos ;
	if( m_auto && pos != std::string::npos )
	{
		if( pos > 0U && m_store.at(pos-1U) == '\r' )
			m_eol = std::string( "\r\n" ) ;
		m_auto = false ;
	}
}

const std::string & GNet::LineBuffer::eol() const
{
	return m_eol ;
}

bool GNet::LineBuffer::check( size_t n ) const
{
	if( n == 0U ) return false ;
	if( m_iterator != nullptr ) throw Error( "locked" ) ;
	bool ok = (m_store.size()+n) <= line_limit ;
	if( !ok )
	{
		if( m_throw_on_overflow )
			throw Error( "overflow" ) ;
		else
			G_ERROR( "GNet::LineBuffer::check: line too long: end-of-line expected: "
				"have " << m_store.size() << " characters, and adding " << n << " more" ) ;
	}
	return ok ;
}

// ==

GNet::LineBufferIterator::LineBufferIterator( LineBuffer & buffer ) :
	m_buffer(buffer) ,
	m_expect(0U) ,
	m_pos(0U) ,
	m_eol_size(buffer.m_eol.size()) ,
	m_line_begin(buffer.m_store.begin()) ,
	m_line_end(m_line_begin) ,
	m_line_valid(false)
{
	m_expect = m_buffer.lock( this ) ;
}

GNet::LineBufferIterator::~LineBufferIterator()
{
	m_buffer.unlock( this , m_pos , m_expect ) ;
}

void GNet::LineBufferIterator::expect( size_t n )
{
	m_expect = n ;
}

bool GNet::LineBufferIterator::more()
{
	m_line_valid = false ;

	const size_t npos = std::string::npos ;
	std::string & store = m_buffer.m_store ;
	const std::string & eol = m_buffer.m_eol ;
	size_t eol_size = eol.size() ;

	G_ASSERT( m_pos <= store.size() ) ;
	if( m_pos == store.size() )
	{
		m_line.clear() ;
		return false ;
	}

	size_t pos = npos ;
	if( m_expect != 0U )
	{
		if( (m_pos+m_expect) <= store.size() )
		{
			pos = m_pos + m_expect ;
			m_expect = 0U ;
			eol_size = 0U ;
		}
	}
	else
	{
		pos = store.find( eol , m_pos ) ;
	}

	if( pos == std::string::npos )
	{
		m_line.clear() ;
		return false ;
	}
	else
	{
		G_ASSERT( pos >= m_pos ) ;
		const std::string & store = m_buffer.m_store ;
		m_line_begin = store.begin() + m_pos ;
		m_line_end = store.begin() + pos ;
		m_pos = pos + eol_size ;
		return true ;
	}
}

const std::string & GNet::LineBufferIterator::line() const
{
	if( !m_line_valid )
	{
		m_line.assign( m_line_begin , m_line_end ) ;
		m_line_valid = true ;
	}
	return m_line ;
}

std::string::const_iterator GNet::LineBufferIterator::begin() const
{
	return m_line_begin ;
}

std::string::const_iterator GNet::LineBufferIterator::end() const
{
	return m_line_end ;
}

/// \file glinebuffer.cpp
