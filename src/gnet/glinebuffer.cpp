//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "glinebuffer.h"
#include "gtest.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

GNet::LineBuffer::LineBuffer( LineBufferConfig config ) :
	m_auto(config.eol().empty()) ,
	m_eol(config.eol()) ,
	m_warn_limit(config.warn()) ,
	m_fmin(config.fmin()) ,
	m_expect(config.expect()) ,
	m_warned(false) ,
	m_pos(0U)
{
}

void GNet::LineBuffer::clear()
{
	m_in.clear() ;
	m_out = Output() ;
	m_pos = 0U ;
	if( !transparent() )
		m_expect = 0U ;

	G_ASSERT( m_in.size() == 0U ) ;
	G_ASSERT( !more(true) ) ;
}

void GNet::LineBuffer::add( const char * data , size_t size )
{
	m_in.append( data , size ) ;
}

void GNet::LineBuffer::add( const std::string & s )
{
	m_in.append( s ) ;
}

void GNet::LineBuffer::extensionStart( const char * data , size_t size )
{
	m_in.extend( data , size ) ;
}

void GNet::LineBuffer::extensionEnd()
{
	m_in.discard( m_pos ) ;
	m_pos = 0U ;
}

bool GNet::LineBuffer::more( bool fragments )
{
	G_ASSERT( m_pos <= m_in.size() ) ;
	const size_t npos = std::string::npos ;
	size_t pos = 0U ;

	if( m_pos == m_in.size() )
	{
		// finished iterating, no residue
		//
		m_in.clear() ;
		m_pos = 0U ;
		return false ;
	}
	else if( m_expect != 0U )
	{
		if( !transparent() && (m_pos+m_expect) <= m_in.size() )
		{
			// got all expected
			//
			output( m_expect , 0U , true ) ;
			m_expect = 0U ;
			return true ;
		}
		else if( fragments && !trivial(m_in.size()) )
		{
			// not all expected, return the available fragment
			//
			G_ASSERT( m_in.size() > m_pos ) ;
			size_t n = m_in.size() - m_pos ;
			output( n , 0U ) ;
			if( !transparent() ) m_expect -= n ;
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
		// no eol-style determined yet
		//
		return false ;
	}
	else if( (pos=m_in.find(m_eol,m_pos)) != npos )
	{
		// complete line available
		//
		output( pos-m_pos , m_eol.size() ) ;
		return true ;
	}
	else if( fragments && (pos=m_in.findSubStringAtEnd(m_eol,m_pos)) != m_pos && !trivial(pos) )
	{
		// finished iterating, return the residual fragment
		//
		pos = pos == npos ? m_in.size() : pos ;
		output( pos-m_pos , 0U ) ;
		return true ;
	}
	else
	{
		// finished iterating
		//
		return false ;
	}
}

bool GNet::LineBuffer::trivial( size_t pos ) const
{
	pos = pos == std::string::npos ? m_in.size() : pos ;
	return ( pos - m_pos ) < m_fmin ;
}

bool GNet::LineBuffer::detect()
{
	const size_t npos = std::string::npos ;
	if( m_auto )
	{
		size_t pos = m_in.find( '\n' ) ;
		if( pos != npos )
		{
			if( pos > 0U && m_in.at(pos-1U) == '\r' )
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

bool GNet::LineBuffer::transparent() const
{
	return ( m_expect + 1U ) == 0U ;
}

std::string GNet::LineBuffer::eol() const
{
	return m_eol ;
}

void GNet::LineBuffer::output( size_t size , size_t eolsize , bool force_next_is_start_of_line )
{
	G_ASSERT( (size+eolsize) != 0U ) ;
	m_pos += m_out.set( m_in , m_pos , size , eolsize ) ;
	if( force_next_is_start_of_line )
		m_out.m_first = true ;
	check( m_out ) ;
}

void GNet::LineBuffer::check( const Output & out )
{
	if( !m_warned && m_warn_limit != 0U && out.m_size > m_warn_limit )
	{
		G_WARNING( "GNet::LineBuffer::check: very long line detected: " << out.m_size << " > " << m_warn_limit ) ;
		m_warned = true ;
	}
}

const char * GNet::LineBuffer::data() const
{
	G_ASSERT( m_out.m_data != nullptr ) ;
	return m_out.m_data ;
}

GNet::LineBufferState GNet::LineBuffer::state() const
{
	return LineBufferState( *this ) ;
}

// ==

GNet::LineBuffer::Output::Output() :
	m_first(true) ,
	m_data(nullptr) ,
	m_size(0U) ,
	m_eolsize(0U) ,
	m_linesize(0U) ,
	m_c0('\0')
{
}

size_t GNet::LineBuffer::Output::set( LineStore & in , size_t pos , size_t size , size_t eolsize )
{
	bool start = m_first || m_eolsize != 0U ; // ie. wrt previous line's eolsize
	m_first = false ;

	m_size = size ;
	m_eolsize = eolsize ;
	if( start ) m_linesize = 0U ;
	m_linesize += size ;
	m_data = in.data( pos , size+eolsize ) ;
	if( start ) m_c0 = size == 0U ? '\0' : m_data[0] ;
	return size + eolsize ;
}

// ==

GNet::LineBufferConfig::LineBufferConfig( const std::string & eol , size_t warn , size_t fmin , size_t expect ) :
	m_eol(eol) ,
	m_warn(warn) ,
	m_fmin(fmin) ,
	m_expect(expect)
{
}

GNet::LineBufferConfig GNet::LineBufferConfig::transparent()
{
	const size_t inf = ~(size_t(0)) ;
	//G_ASSERT( (inf+1U) == 0U ) ;
	return LineBufferConfig( std::string(1U,'\n') , 0U , 0U , inf ) ;
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
	return LineBufferConfig( std::string("\r\n",2U) , 998U + 2U , /*fmin=*/2U ) ; // RFC-2822
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
