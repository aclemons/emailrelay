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
#include "gtest.h"
#include "gdebug.h"
#include "gassert.h"
#include <algorithm>

static const size_t c_line_limit = G::limits::net_line_limit ;

GNet::LineBuffer::LineBuffer( LineBufferConfig config ) :
	m_auto(config.eol().empty()) ,
	m_eol(config.eol()) ,
	m_warn_limit(config.warn()) ,
	m_fail_limit(config.fail()?config.fail():c_line_limit) ,
	m_extra_data(nullptr) ,
	m_extra_size(0U) ,
	m_expect(0U) ,
	m_warned(false) ,
	m_pos(0U) ,
	m_line_data(nullptr) ,
	m_line_size(0U) ,
	m_eol_size(0U)
{
}

void GNet::LineBuffer::add( const char * data , size_t size )
{
	if( size )
	{
		precheck( size ) ;
		m_store.append( data , size ) ;
	}
}

void GNet::LineBuffer::add( const std::string & s )
{
	if( !s.empty() )
	{
		precheck( s.size() ) ;
		m_store.append( s ) ;
	}
}

void GNet::LineBuffer::addextra( const char * data , size_t size )
{
	if( G::Test::enabled("line-buffer-simple") )
	{
		add( data , size ) ; // no zero-copy optimisation
	}
	else if( size )
	{
		precheck( size ) ;
		m_extra_data = data ; G_ASSERT( m_extra_size == 0U ) ;
		m_extra_size = size ;

		// make sure no eol spans store and extra
		while( !m_store.empty() && m_extra_size != 0U && !m_eol.empty() &&
			m_eol.find(m_store.at(m_store.size()-1U)) != std::string::npos &&
			m_eol.find(*m_extra_data) != std::string::npos )
		{
			m_store.append( 1U , *m_extra_data++ ) ;
			m_extra_size-- ;
		}
	}
}

void GNet::LineBuffer::precheck( size_t n )
{
	G_ASSERT( n != 0U ) ;
	bool arithmetic_overflow = (m_store.size() + n) < m_store.size() ;
	bool fail_limit_overflow = (m_store.size() + n) > m_fail_limit ;
	if( arithmetic_overflow || fail_limit_overflow )
		throw ErrorOverflow() ;
}

bool GNet::LineBuffer::more()
{
	const size_t npos = std::string::npos ;
	size_t pos = npos ;
	const char * p = nullptr ;
	m_line_data = nullptr ;

	G_ASSERT( m_pos <= (m_store.size()+m_extra_size) ) ;
	if( m_pos == (m_store.size()+m_extra_size) )
	{
		// finished iterating, no residue
		//
		m_store.clear() ;
		m_extra_size = 0U ;
		m_pos = 0U ;
		return false ;
	}
	else if( m_expect != 0U )
	{
		consolidate() ;
		if( (m_pos+m_expect) <= m_store.size() )
		{
			// got all expected
			//
			m_eol_size = 0U ;
			m_line_data = m_store.data() + m_pos ;
			m_line_size = m_expect ;
			m_pos += m_expect ;
			m_expect = 0U ;
			return true ;
		}
		else
		{
			// expecting more
			//
			return false ;
		}
	}
	else if( !detect() )
	{
		// no eol detected
		//
		consolidate() ;
		return false ;
	}
	else if( m_pos < m_store.size() && (pos=m_store.find(m_eol,m_pos)) != npos )
	{
		// complete line in store
		//
		m_eol_size = m_eol.size() ;
		m_line_data = m_store.data() + m_pos ;
		m_line_size = pos - m_pos ;
		linecheck( m_line_size ) ;
		m_pos = pos + m_eol_size ;
		return true ;
	}
	else if( m_pos < m_store.size() && m_extra_size == 0U )
	{
		// finished iterating, leave the residue
		//
		if( m_pos ) m_store.erase( 0U , m_pos ) ;
		m_pos = 0U ;
		return false ;
	}
	else if( (p=extraeol()) != nullptr )
	{
		G_ASSERT( m_extra_size != 0U ) ;
		if( m_pos < m_store.size() )
		{
			// line spans store and extra
			//
			size_t n_lhs = m_store.size() - m_pos ;
			size_t n_rhs = std::distance(m_extra_data,p) + m_eol.size() ;
			m_store.append( m_extra_data , n_rhs ) ;
			m_extra_data += n_rhs ;
			m_extra_size -= n_rhs ;
			m_eol_size = m_eol.size() ;
			m_line_data = m_store.data() + m_pos ;
			m_line_size = n_lhs + n_rhs - m_eol_size ;
			linecheck( m_line_size ) ;
			m_pos += ( n_lhs + n_rhs ) ;
			return true ;
		}
		else
		{
			// complete line in extra
			//
			m_eol_size = m_eol.size() ;
			m_line_data = m_extra_data + m_pos - m_store.size() ;
			m_line_size = std::distance( m_line_data , p ) ;
			linecheck( m_line_size ) ;
			m_pos += ( m_line_size + m_eol_size ) ;
			return true ;
		}
	}
	else if( m_pos < m_store.size() )
	{
		// finished iterating, residue in store and extra
		//
		G_ASSERT( m_extra_size != 0U ) ;
		consolidate() ;
		if( m_pos ) m_store.erase( 0U , m_pos ) ;
		m_pos = 0U ;
		return false ;
	}
	else
	{
		// finished iterating, residue in extra
		//
		G_ASSERT( m_extra_size != 0U ) ;
		size_t n = m_pos - m_store.size() ;
		m_store.assign( m_extra_data + n , m_extra_size - n ) ;
		m_extra_size = 0U ;
		m_pos = 0U ;
		return false ;
	}
}

const char * GNet::LineBuffer::extraeol() const
{
	G_ASSERT( m_store.find(m_eol,m_pos) == std::string::npos ) ;
	G_ASSERT( m_extra_size != 0U ) ;

	const char * end = m_extra_data + m_extra_size ;
	const char * start = m_extra_data ;
	if( m_pos > m_store.size() )
		start += ( m_pos-m_store.size() ) ;

	const char * p = std::search( start , end , m_eol.data() , m_eol.data()+m_eol.size() ) ;
	return p == end ? nullptr : p ;
}

void GNet::LineBuffer::consolidate()
{
	if( m_extra_size )
	{
		m_store.append( m_extra_data , m_extra_size ) ;
		m_extra_size = 0U ;
	}
}

bool GNet::LineBuffer::detect()
{
	const size_t npos = std::string::npos ;
	if( m_auto )
	{
		consolidate() ; // could do better
		size_t pos = m_store.find( '\n' ) ;
		if( pos != npos )
		{
			if( pos > 0U && m_store.at(pos-1U) == '\r' )
				m_eol = std::string( "\r\n" , 2U ) ;
			else
				m_eol = std::string( 1U , '\n' ) ;
			m_auto = false ;
		}
	}
	return !m_eol.empty() ;
}

void GNet::LineBuffer::expect( size_t n )
{
	m_expect = n ;
}

std::string GNet::LineBuffer::eol() const
{
	return m_eol ;
}

void GNet::LineBuffer::linecheck( size_t n )
{
	if( !m_warned && m_warn_limit != 0U && n > m_warn_limit )
	{
		G_WARNING( "GNet::LineBuffer::check: very long line detected: " << n << " > " << m_warn_limit ) ;
		m_warned = true ;
	}
}

size_t GNet::LineBuffer::eolSize() const
{
	return m_eol_size ;
}

size_t GNet::LineBuffer::lineSize() const
{
	return m_line_size ;
}

const char * GNet::LineBuffer::lineData() const
{
	G_ASSERT( m_line_data != nullptr ) ;
	return m_line_data ;
}

// ==

GNet::LineBufferConfig::LineBufferConfig( const std::string & eol , size_t warn , size_t fail ) :
	m_eol(eol) ,
	m_warn(warn) ,
	m_fail(fail)
{
}

GNet::LineBufferConfig GNet::LineBufferConfig::newline()
{
	return LineBufferConfig( std::string(1U,'\n') ) ;
}

GNet::LineBufferConfig GNet::LineBufferConfig::autodetect()
{
	return LineBufferConfig( std::string() ) ;
}

GNet::LineBufferConfig GNet::LineBufferConfig::crlf()
{
	return LineBufferConfig( std::string("\r\n",2U) ) ;
}

GNet::LineBufferConfig GNet::LineBufferConfig::smtp()
{
	return LineBufferConfig( std::string("\r\n",2U) , 998U + 2U ) ; // RFC-2822
}

GNet::LineBufferConfig GNet::LineBufferConfig::pop()
{
	return crlf() ;
}

GNet::LineBufferConfig GNet::LineBufferConfig::http()
{
	return crlf() ;
}

/// \file glinebuffer.cpp
