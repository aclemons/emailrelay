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
///
/// \file glinestore.h
///

#ifndef G_LINE_STORE__H
#define G_LINE_STORE__H

#include "gdef.h"
#include "gassert.h"
#include <string>
#include <algorithm>
#include <iterator>

namespace GNet
{
	class LineStore ;
	class LineStoreIterator ;
}

/// \class GNet::LineStore
/// A pair of character buffers, one kept by value and the other being
/// an ephemoral extension. An iterator class can iterate over the
/// combined data.
/// \see GNet::LineBuffer
///
class GNet::LineStore
{
public:
	typedef LineStoreIterator const_iterator ;

	LineStore() ;
		///< Default constructor.

	void append( const std::string & ) ;
		///< Appends to the store (by copying). Any existing
		///< extension is first consolidate()d.

	void append( const char * , size_t ) ;
		///< Appends to the store (by copying). Any existing
		///< extension is first consolidate()d.

	void extend( const char * , size_t ) ;
		///< Sets the extension. Any existing extension is
		///< consolidated(). Use consolidate(), discard() or
		///< clear() before the extension pointer becomes
		///< invalid.

	void discard( size_t n ) ;
		///< Discards the first 'n' bytes and consolidates the residue.

	void consolidate() ;
		///< Consolidates the extension into the store.

	void clear() ;
		///< Clears all data.

	size_t size() const ;
		///< Returns the overall size.

	size_t find( char c , size_t startpos = 0U ) const ;
		///< Finds the given character. Returns npos if
		///< not found.

	size_t find( const std::string & s , size_t startpos = 0U ) const ;
		///< Finds the given string. Returns npos if not
		///< found.

	template <typename T>
	size_t find( T begin , T end , size_t startpos = 0U ) const ;
		///< Finds the given character range. Returns npos if not
		///< found.

	size_t findSubStringAtEnd( const std::string & s , size_t startpos = 0U ) const ;
		///< Finds a non-empty leading substring of 's' that
		///< appears at the end of the data. Returns npos
		///< if not found.

	const char * data( size_t pos , size_t size ) const ;
		///< Returns a pointer for the data at the given position
		///< that is contiguous for the given size. Data is
		///< shuffled around as required, which means that
		///< previous pointers are invalidated.

	char at( size_t ) const ;
		///< Returns the n'th character.

	std::string str() const ;
		///< Returns the complete string.

	std::string str( LineStoreIterator begin , LineStoreIterator end ) const ;
		///< Returns the string between the two iterators.

	LineStoreIterator begin() const ;
		///< Returns a begin const-iterator.

	LineStoreIterator end() const ;
		///< Returns an off-the-end const-iterator.

private:
	LineStore( const LineStore & ) ;
	void operator=( const LineStore & ) ;
	const char * dataimp( size_t pos , size_t size ) ;

private:
	std::string m_store ;
	const char * m_extra_data ;
	size_t m_extra_size ;
} ;

/// \class GNet::LineStoreIterator
/// A const-iterator class for GNet::LineStore.
///
class GNet::LineStoreIterator : public std::iterator<std::bidirectional_iterator_tag,LineStoreIterator,ptrdiff_t,LineStoreIterator*,LineStoreIterator&>
{
public:
	LineStoreIterator() ;
	explicit LineStoreIterator( const LineStore & , bool end = false ) ;

	LineStoreIterator( const LineStoreIterator & ) ;
	LineStoreIterator & operator=( const LineStoreIterator & ) ;

	char operator*() const ;
	char operator[]( size_t ) const ;

	void operator+=( ptrdiff_t n ) ;
	void operator-=( ptrdiff_t n ) ;
	bool operator==( const LineStoreIterator & ) const ;
	bool operator!=( const LineStoreIterator & ) const ;
	bool operator<( const LineStoreIterator & ) const ;
	bool operator<=( const LineStoreIterator & ) const ;
	bool operator>( const LineStoreIterator & ) const ;
	bool operator>=( const LineStoreIterator & ) const ;

	LineStoreIterator & operator++() ;
	LineStoreIterator operator++( int ) ;
	LineStoreIterator & operator--() ;
	LineStoreIterator operator--( int ) ;
	void swap( LineStoreIterator & ) ;
	ptrdiff_t distanceTo( const LineStoreIterator & ) const ;
	size_t pos() const ;

private:
	const LineStore * m_p ;
	size_t m_pos ;
} ;

// ==

inline
char GNet::LineStore::at( size_t n ) const
{
	G_ASSERT( n < size() ) ;
	return n < m_store.size() ? m_store[n] : m_extra_data[n-m_store.size()] ;
}

inline
size_t GNet::LineStore::size() const
{
	return m_store.size() + m_extra_size ;
}

inline
size_t GNet::LineStore::find( const std::string & s , size_t startpos ) const
{
	return find( s.begin() , s.end() , startpos ) ;
}

inline
GNet::LineStoreIterator GNet::LineStore::begin() const
{
	return LineStoreIterator( *this ) ;
}

inline
GNet::LineStoreIterator GNet::LineStore::end() const
{
	return LineStoreIterator( *this , true ) ;
}

// ==

inline
GNet::LineStoreIterator::LineStoreIterator() :
	m_p(nullptr) ,
	m_pos(0U)
{
}

inline
GNet::LineStoreIterator::LineStoreIterator( const LineStore & line_store , bool end ) :
	m_p(&line_store) ,
	m_pos(end?line_store.size():0U)
{
}

inline
GNet::LineStoreIterator::LineStoreIterator( const LineStoreIterator & other ) :
	m_p(other.m_p) ,
	m_pos(other.m_pos)
{
}

inline
void GNet::LineStoreIterator::swap( LineStoreIterator & other )
{
	using std::swap ;
	swap( m_p , other.m_p ) ;
	swap( m_pos , other.m_pos ) ;
}

inline
GNet::LineStoreIterator & GNet::LineStoreIterator::operator=( const LineStoreIterator & other )
{
	LineStoreIterator tmp( other ) ;
	swap( tmp ) ;
	return *this ;
}

inline
GNet::LineStoreIterator & GNet::LineStoreIterator::operator++()
{
	G_ASSERT( m_p != nullptr && m_pos < m_p->size() ) ;
	m_pos++ ;
	return *this ;
}

inline
GNet::LineStoreIterator & GNet::LineStoreIterator::operator--()
{
	G_ASSERT( m_p != nullptr && m_pos > 0U ) ;
	m_pos-- ;
	return *this ;
}

inline
bool GNet::LineStoreIterator::operator==( const LineStoreIterator & other ) const
{
	return m_pos == other.m_pos ;
}

inline
bool GNet::LineStoreIterator::operator!=( const LineStoreIterator & other ) const
{
	return m_pos != other.m_pos ;
}

inline
bool GNet::LineStoreIterator::operator<( const LineStoreIterator & other ) const
{
	return m_pos < other.m_pos ;
}

inline
bool GNet::LineStoreIterator::operator<=( const LineStoreIterator & other ) const
{
	return m_pos <= other.m_pos ;
}

inline
bool GNet::LineStoreIterator::operator>( const LineStoreIterator & other ) const
{
	return m_pos > other.m_pos ;
}

inline
bool GNet::LineStoreIterator::operator>=( const LineStoreIterator & other ) const
{
	return m_pos >= other.m_pos ;
}

inline
char GNet::LineStoreIterator::operator*() const
{
	G_ASSERT( m_p != nullptr ) ;
	return m_p->at( m_pos ) ;
}

inline
char GNet::LineStoreIterator::operator[]( size_t n ) const
{
	G_ASSERT( m_p != nullptr ) ;
	return m_p->at( m_pos + n ) ;
}

inline
void GNet::LineStoreIterator::operator+=( ptrdiff_t n )
{
	if( n < 0 )
		m_pos -= static_cast<size_t>(-n) ;
	else
		m_pos += static_cast<size_t>(n) ;
	G_ASSERT( n == 0 || ( m_p != nullptr && m_pos <= m_p->size() ) ) ;
}

inline
void GNet::LineStoreIterator::operator-=( ptrdiff_t n )
{
	if( n < 0 )
		m_pos += static_cast<size_t>(-n) ;
	else
		m_pos -= static_cast<size_t>(n) ;
	G_ASSERT( n == 0 || ( m_p != nullptr && m_pos <= m_p->size() ) ) ;
}

inline
ptrdiff_t GNet::LineStoreIterator::distanceTo( const LineStoreIterator & other ) const
{
	G_ASSERT( m_p == other.m_p ) ;
	if( other.m_pos >= m_pos )
		return static_cast<ptrdiff_t>(other.m_pos-m_pos) ;
	else
		return -static_cast<ptrdiff_t>(m_pos-other.m_pos) ;
}

inline
size_t GNet::LineStoreIterator::pos() const
{
	G_ASSERT( m_p == nullptr || m_pos <= m_p->size() ) ;
	return ( m_p == nullptr || m_pos == m_p->size() ) ? std::string::npos : m_pos ;
}

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
	inline void swap( LineStoreIterator & a , LineStoreIterator & b )
	{
		a.swap( b ) ;
	}
}

// ==

template <typename T>
size_t GNet::LineStore::find( T begin , T end , size_t startpos ) const
{
	return std::search( LineStoreIterator(*this)+startpos , LineStoreIterator(*this,true) , begin , end ).pos() ;
}

#endif
