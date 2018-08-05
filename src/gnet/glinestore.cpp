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
// glinestore.cpp
//

#include "gdef.h"
#include "glinestore.h"
#include "gassert.h"

GNet::LineStore::LineStore() :
	m_extra_data(nullptr) ,
	m_extra_size(0U)
{
}

void GNet::LineStore::append( const std::string & s )
{
	consolidate() ;
	m_store.append( s ) ;
}

void GNet::LineStore::append( const char * data , size_t size )
{
	consolidate() ;
	m_store.append( data , size ) ;
}

void GNet::LineStore::extend( const char * data , size_t size )
{
	consolidate() ;
	m_extra_data = data ;
	m_extra_size = size ;
}

void GNet::LineStore::clear()
{
	m_store.clear() ;
	m_extra_size = 0U ;
}

void GNet::LineStore::consolidate()
{
	m_store.append( m_extra_data , m_extra_size ) ;
	m_extra_size = 0U ;
}

void GNet::LineStore::discard( size_t n )
{
	if( n == 0U )
	{
		if( m_extra_size )
		{
			m_store.append( m_extra_data , m_extra_size ) ;
			m_extra_size = 0U ;
		}
	}
	else if( n < m_store.size() )
	{
		m_store.erase( 0U , n ) ;
		if( m_extra_size )
		{
			m_store.append( m_extra_data , m_extra_size ) ;
			m_extra_size = 0U ;
		}
	}
	else if( n == m_store.size() )
	{
		m_store.clear() ;
		if( m_extra_size )
		{
			m_store.assign( m_extra_data , m_extra_size ) ;
			m_extra_size = 0U ;
		}
	}
	else if( n < size() )
	{
		size_t offset = n - m_store.size() ;
		m_store.clear() ;
		if( m_extra_size )
		{
			G_ASSERT( m_extra_size >= offset ) ;
			m_store.assign( m_extra_data+offset , m_extra_size-offset ) ;
			m_extra_size = 0U ;
		}
	}
	else
	{
		clear() ;
	}
}

size_t GNet::LineStore::find( char c , size_t startpos ) const
{
	G_ASSERT( startpos <= size() ) ;
	return std::find( begin()+startpos , end() , c ).pos() ;
}

size_t GNet::LineStore::findSubStringAtEnd( const std::string & s , size_t startpos ) const
{
	if( s.empty() )
	{
		return 0U ;
	}
	else
	{
		size_t result = std::string::npos ;
		size_t s_size = s.size() ;
		std::string::const_iterator s_start = s.begin() ;
		std::string::const_iterator s_end = s.end() ;
		for( --s_size , --s_end ; s_start != s_end ; --s_size , --s_end )
		{
			if( (size()-startpos) >= s_size )
			{
				LineStoreIterator p = end() - s_size ;
				if( std::equal(s_start,s_end,p) )
				{
					result = p.pos() ;
					break ;
				}
			}
		}
		return result ;
	}
}

const char * GNet::LineStore::data( size_t pos , size_t n ) const
{
	return (const_cast<LineStore*>(this))->dataimp( pos , n ) ;
}

const char * GNet::LineStore::dataimp( size_t pos , size_t n )
{
	G_ASSERT( (n==0U && size()==0U) || (pos+n) <= size() ) ;
	if( n == 0U && size() == 0U )
	{
		return "" ;
	}
	else if( n == 0U && pos == size() )
	{
		return "" ;
	}
	else if( (pos+n) <= m_store.size() )
	{
		return m_store.data() + pos ;
	}
	else if( pos >= m_store.size() )
	{
		size_t offset = pos - m_store.size() ;
		return m_extra_data + offset ;
	}
	else
	{
		size_t nmove = pos + n - m_store.size() ;
		m_store.append( m_extra_data , nmove ) ;
		m_extra_data += nmove ;
		m_extra_size -= nmove ;
		return m_store.data() + pos ;
	}
}

std::string GNet::LineStore::str() const
{
	std::string result( m_store ) ;
	if( m_extra_size )
		result.append( m_extra_data , m_extra_size ) ;
	return result ;
}

std::string GNet::LineStore::str( LineStoreIterator p , LineStoreIterator end ) const
{
	std::string result ;
	result.reserve( std::distance(p,end) ) ;
	for( ; p != end ; ++p )
		result.append( 1U , *p ) ;
	return result ;
}

/// \file glinestore.cpp
