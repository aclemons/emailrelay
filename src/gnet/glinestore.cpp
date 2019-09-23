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
// glinestore.cpp
//

#include "gdef.h"
#include "glinestore.h"
#include "gexception.h"
#include "gassert.h"
#include <iterator>

namespace GNet
{
	class LineStoreIterator ;
}

/// \class GNet::LineStoreIterator
/// An iterator class for GNet::LineStore.
///
class GNet::LineStoreIterator : public std::iterator<std::bidirectional_iterator_tag,char,ptrdiff_t>
{
public:
	G_EXCEPTION( Error , "line buffer internal error" ) ;
	LineStoreIterator() : m_p(nullptr) , m_pos(0U) {}
	LineStoreIterator( const LineStore & line_store , bool end = false ) : m_p(&line_store) , m_pos(end?line_store.size():0U) {}
	LineStoreIterator( const LineStoreIterator & other ) : m_p(other.m_p) , m_pos(other.m_pos) {}
	void swap( LineStoreIterator & other ) g__noexcept { using std::swap ; swap( m_p , other.m_p ) ; swap( m_pos , other.m_pos ) ; }
	LineStoreIterator & operator=( const LineStoreIterator & other ) { LineStoreIterator tmp( other ) ; swap( tmp ) ; return *this ; }
	LineStoreIterator & operator++() { m_pos++ ; return *this ; }
	LineStoreIterator & operator--() { m_pos-- ; return *this ; }
	bool operator==( const LineStoreIterator & other ) const { return m_pos == other.m_pos ; }
	bool operator!=( const LineStoreIterator & other ) const { return m_pos != other.m_pos ; }
	bool operator<( const LineStoreIterator & other ) const { return m_pos < other.m_pos ; }
	bool operator<=( const LineStoreIterator & other ) const { return m_pos <= other.m_pos ; }
	bool operator>( const LineStoreIterator & other ) const { return m_pos > other.m_pos ; }
	bool operator>=( const LineStoreIterator & other ) const { return m_pos >= other.m_pos ; }
	char operator*() const { G_ASSERT( m_p != nullptr ) ; if( m_p == nullptr ) throw Error() ; return m_p->at( m_pos ) ; }
	char operator[]( size_t n ) const { G_ASSERT( m_p != nullptr ) ; if( m_p == nullptr ) throw Error() ; return m_p->at( m_pos + n ) ; }
	void operator+=( ptrdiff_t n ) { if( n < 0 ) m_pos -= static_cast<size_t>(-n) ; else m_pos += static_cast<size_t>(n) ; }
	void operator-=( ptrdiff_t n ) { if( n < 0 ) m_pos += static_cast<size_t>(-n) ; else m_pos -= static_cast<size_t>(n) ; }
	ptrdiff_t distanceTo( const LineStoreIterator & other ) const { if( other.m_pos >= m_pos ) return static_cast<ptrdiff_t>(other.m_pos-m_pos) ; else return -static_cast<ptrdiff_t>(m_pos-other.m_pos) ; }
	size_t pos() const { return ( m_p == nullptr || m_pos >= m_p->size() ) ? std::string::npos : m_pos ; }

private:
	const LineStore * m_p ;
	size_t m_pos ;
} ;

namespace GNet
{
	inline LineStoreIterator operator+( const LineStoreIterator & in , ptrdiff_t n )
	{
		LineStoreIterator result( in ) ;
		result += n ;
		return result ;
	}
	inline LineStoreIterator operator-( const LineStoreIterator & in , ptrdiff_t n )
	{
		LineStoreIterator result( in ) ;
		result -= n ;
		return result ;
	}
	inline LineStoreIterator operator+( ptrdiff_t n , const LineStoreIterator & in )
	{
		LineStoreIterator result( in ) ;
		result += n ;
		return result ;
	}
	inline ptrdiff_t operator-( const LineStoreIterator & a , const LineStoreIterator & b )
	{
		return a.distanceTo( b ) ;
	}
	inline void swap( LineStoreIterator & a , LineStoreIterator & b ) g__noexcept
	{
		a.swap( b ) ;
	}
}

namespace
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

// ==

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
	if( m_extra_size )
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
	size_t result = std::string::npos ;
	const size_t store_size = m_store.size() ;
	if( startpos < store_size )
	{
		result = m_store.find( c , startpos ) ;
	}
	if( result == std::string::npos && m_extra_size != 0U )
	{
		const size_t offset = startpos > store_size ? (startpos-store_size) : 0U ;
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

size_t GNet::LineStore::find( const std::string & s , size_t startpos ) const
{
	const size_t npos = std::string::npos ;
	size_t result = npos ;
	if( s.size() == 2U )
	{
		const char c0 = s[0] ;
		const char c1 = s[1] ;
		const size_t end = size() ;
		for( size_t pos = startpos ; pos != npos ; ++pos )
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

size_t GNet::LineStore::search( std::string::const_iterator begin , std::string::const_iterator end , size_t startpos ) const
{
	return std::search( LineStoreIterator(*this)+startpos , LineStoreIterator(*this,true) , begin , end ).pos() ;
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
		// for progressivley shorter leading substrings...
		for( --s_size , --s_end ; s_start != s_end ; --s_size , --s_end )
		{
			if( (size()-startpos) >= s_size ) // if we have enough
			{
				// compare leading substring with the end of the store
				const LineStoreIterator end( *this , true ) ;
				LineStoreIterator p = end - s_size ;
				if( ::std_equal(s_start,s_end,p,end) )
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

