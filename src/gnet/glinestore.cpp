//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glinestore.cpp
///

#include "gdef.h"
#include "glinestore.h"
#include "gexception.h"
#include "gstr.h"
#include "gassert.h"
#include <iterator>
#include <cstddef>

namespace GNet
{
	class LineStoreIterator ;
}

//| \class GNet::LineStoreIterator
/// An iterator class for GNet::LineStore.
///
class GNet::LineStoreIterator
{
public:
	G_EXCEPTION( Error , tx("line buffer internal error") ) ;
	using iterator_category = std::bidirectional_iterator_tag ;
	using value_type = char ;
	using difference_type = std::ptrdiff_t ;
	using pointer = char* ;
	using reference = char ;
	LineStoreIterator() = default;
	~LineStoreIterator() = default;
	explicit LineStoreIterator( const LineStore & line_store , bool end = false ) :
		m_p(&line_store) ,
		m_pos(end?line_store.size():0U)
	{
	}
	LineStoreIterator( const LineStoreIterator & ) = default ;
	LineStoreIterator( LineStoreIterator && ) noexcept = default ;
	LineStoreIterator & operator=( const LineStoreIterator & ) = default ;
	LineStoreIterator & operator=( LineStoreIterator && ) noexcept = default ;
	LineStoreIterator & operator++()
	{
		m_pos++ ;
		return *this ;
	}
	LineStoreIterator & operator--()
	{
		m_pos-- ;
		return *this ;
	}
	bool operator==( const LineStoreIterator & other ) const
	{
		return m_pos == other.m_pos ;
	}
	bool operator!=( const LineStoreIterator & other ) const
	{
		return m_pos != other.m_pos ;
	}
	bool operator<( const LineStoreIterator & other ) const
	{
		return m_pos < other.m_pos ;
	}
	bool operator<=( const LineStoreIterator & other ) const
	{
		return m_pos <= other.m_pos ;
	}
	bool operator>( const LineStoreIterator & other ) const
	{
		return m_pos > other.m_pos ;
	}
	bool operator>=( const LineStoreIterator & other ) const
	{
		return m_pos >= other.m_pos ;
	}
	char operator*() const
	{
		G_ASSERT( m_p != nullptr ) ;
		return m_p ? m_p->at( m_pos ) : '\0' ;
	}
	char operator[]( std::size_t n ) const
	{
		G_ASSERT( m_p != nullptr ) ;
		return m_p ? m_p->at( m_pos + n ) : '\0' ;
	}
	void operator+=( std::ptrdiff_t n )
	{
		if( n < 0 )
			m_pos -= static_cast<std::size_t>(-n) ;
		else
			m_pos += static_cast<std::size_t>(n) ;
	}
	void operator-=( std::ptrdiff_t n )
	{
		if( n < 0 )
			m_pos += static_cast<std::size_t>(-n) ;
		else
			m_pos -= static_cast<std::size_t>(n) ;
	}
	std::ptrdiff_t distanceTo( const LineStoreIterator & other ) const
	{
		if( other.m_pos >= m_pos )
			return static_cast<std::ptrdiff_t>(other.m_pos-m_pos) ;
		else
			return -static_cast<std::ptrdiff_t>(m_pos-other.m_pos) ;
	}
	std::size_t pos() const
	{
		return ( m_p == nullptr || m_pos >= m_p->size() ) ? std::string::npos : m_pos ;
	}

private:
	const LineStore * m_p {nullptr} ;
	std::size_t m_pos {0U} ;
} ;

namespace GNet
{
	inline LineStoreIterator operator+( const LineStoreIterator & in , std::ptrdiff_t n )
	{
		LineStoreIterator result( in ) ;
		result += n ;
		return result ;
	}
	inline LineStoreIterator operator-( const LineStoreIterator & in , std::ptrdiff_t n )
	{
		LineStoreIterator result( in ) ;
		result -= n ;
		return result ;
	}
	inline LineStoreIterator operator+( std::ptrdiff_t n , const LineStoreIterator & in )
	{
		LineStoreIterator result( in ) ;
		result += n ;
		return result ;
	}
	inline std::ptrdiff_t operator-( const LineStoreIterator & a , const LineStoreIterator & b )
	{
		return a.distanceTo( b ) ;
	}
}

namespace GNet
{
	namespace LineStoreImp
	{
		template <typename T1, typename T2> bool std_equal( T1 p1 , T1 end1 , T2 p2 , T2 end2 )
		{
			// (std::equal with four iterators is c++14 or later)
			for( ; p1 != end1 && p2 != end2 ; ++p1 , ++p2 )
			{
				if( !(*p1 == *p2) )
					return false ;
			}
			return p1 == end1 && p2 == end2 ;
		}
	}
}

// ==

GNet::LineStore::LineStore()
= default;

void GNet::LineStore::append( const std::string & s )
{
	consolidate() ;
	m_store.append( s ) ;
}

void GNet::LineStore::append( const char * data , std::size_t size )
{
	consolidate() ;
	m_store.append( data , size ) ;
}

void GNet::LineStore::extend( const char * data , std::size_t size )
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
	if( m_extra_size )
		m_store.append( m_extra_data , m_extra_size ) ;
	m_extra_size = 0U ;
}

void GNet::LineStore::discard( std::size_t n )
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
		std::size_t offset = n - m_store.size() ;
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

std::size_t GNet::LineStore::find( char c , std::size_t startpos ) const
{
	G_ASSERT( startpos <= size() ) ;
	std::size_t result = std::string::npos ;
	const std::size_t store_size = m_store.size() ;
	if( startpos < store_size )
	{
		result = m_store.find( c , startpos ) ;
	}
	if( result == std::string::npos && m_extra_size != 0U )
	{
		const std::size_t offset = startpos > store_size ? (startpos-store_size) : 0U ;
		const char * const begin = m_extra_data + offset ;
		const char * const end = m_extra_data + m_extra_size ;
		G_ASSERT( begin >= m_extra_data && begin <= end ) ;
		const char * p = std::find( begin , end , c ) ;
		if( p != end )
			result = store_size + std::distance(m_extra_data,p) ;
	}
	G_ASSERT( result == std::find(LineStoreIterator(*this)+startpos,LineStoreIterator(*this,true),c).pos() ) ;
	return result ;
}

std::size_t GNet::LineStore::find( const std::string & s , std::size_t startpos ) const
{
	const std::size_t npos = std::string::npos ;
	std::size_t result = npos ;
	if( s.size() == 2U )
	{
		const char c0 = s[0] ;
		const char c1 = s[1] ;
		const std::size_t end = size() ;
		for( std::size_t pos = startpos ; pos != npos ; ++pos )
		{
			pos = find( c0 , pos ) ;
			if( pos == npos ) break ;
			if( (pos+1U) != end && at(pos+1U) == c1 )
			{
				result = pos ;
				break ;
			}
		}
		G_ASSERT( result == search(s.begin(),s.end(),startpos) ) ;
	}
	else if( s.size() == 1U )
	{
		result = find( s[0] , startpos ) ;
		G_ASSERT( result == search(s.begin(),s.end(),startpos) ) ;
	}
	else
	{
        result = search( s.begin() , s.end() , startpos ) ;
	}
	return result ;
}

std::size_t GNet::LineStore::search( std::string::const_iterator begin , std::string::const_iterator end ,
	std::size_t startpos ) const
{
	return std::search( LineStoreIterator(*this)+startpos , LineStoreIterator(*this,true) , begin , end ).pos() ; // NOLINT narrowing
}

std::size_t GNet::LineStore::findSubStringAtEnd( const std::string & s , std::size_t startpos ) const
{
	namespace imp = LineStoreImp ;
	if( s.empty() )
	{
		return 0U ;
	}
	else
	{
		std::size_t result = std::string::npos ;
		std::size_t s_size = s.size() ;
		std::string::const_iterator s_start = s.begin() ;
		std::string::const_iterator s_end = s.end() ;
		// for progressivley shorter leading substrings...
		for( --s_size , --s_end ; s_start != s_end ; --s_size , --s_end )
		{
			if( (size()-startpos) >= s_size ) // if we have enough
			{
				// compare leading substring with the end of the store
				const LineStoreIterator end( *this , true ) ;
				LineStoreIterator p = end - s_size ; // NOLINT narrowing
				if( imp::std_equal(s_start,s_end,p,end) )
				{
					result = p.pos() ;
					break ;
				}
			}
		}
		return result ;
	}
}

const char * GNet::LineStore::data( std::size_t pos , std::size_t n ) const
{
	return (const_cast<LineStore*>(this))->dataimp( pos , n ) ;
}

const char * GNet::LineStore::dataimp( std::size_t pos , std::size_t n )
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
		std::size_t offset = pos - m_store.size() ;
		return m_extra_data + offset ;
	}
	else
	{
		std::size_t nmove = pos + n - m_store.size() ;
		m_store.append( m_extra_data , nmove ) ;
		m_extra_data += nmove ;
		m_extra_size -= nmove ;
		return m_store.data() + pos ;
	}
}

#ifndef G_LIB_SMALL
std::string GNet::LineStore::str() const
{
	std::string result( m_store ) ;
	if( m_extra_size )
		result.append( m_extra_data , m_extra_size ) ;
	return result ;
}
#endif

std::string GNet::LineStore::head( std::size_t n ) const
{
	std::string result = G::Str::head( m_store , n ) ;
	if( result.size() < n && m_extra_size )
		result.append( m_extra_data , std::min(n-result.size(),m_extra_size) ) ;
	return result ;
}

