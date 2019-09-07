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
///
/// \file glinestore.h
///

#ifndef G_NET_LINE_STORE__H
#define G_NET_LINE_STORE__H

#include "gdef.h"
#include "gassert.h"
#include <string>
#include <algorithm> // std::swap()
#include <utility> // std::swap()

namespace GNet
{
	class LineStore ;
}

/// \class GNet::LineStore
/// A pair of character buffers, one kept by value and the other being
/// an ephemeral extension. An iterator class can iterate over the
/// combined data. Used in the implementation of GNet::LineBuffer.
///
class GNet::LineStore
{
public:
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
		///< Discards the first 'n' bytes and consolidates
		///< the residue.

	void consolidate() ;
		///< Consolidates the extension into the store.

	void clear() ;
		///< Clears all data.

	size_t size() const ;
		///< Returns the overall size.

	bool empty() const ;
		///< Returns true if size() is zero.

	size_t find( char c , size_t startpos = 0U ) const ;
		///< Finds the given character. Returns npos if
		///< not found.

	size_t find( const std::string & s , size_t startpos = 0U ) const ;
		///< Finds the given sub-string. Returns npos if not
		///< found.

	size_t findSubStringAtEnd( const std::string & s , size_t startpos = 0U ) const ;
		///< Finds a non-empty leading substring of 's' that
		///< appears at the end of the data. Returns npos
		///< if not found.

	const char * data( size_t pos , size_t size ) const ;
		///< Returns a pointer for the data at the given
		///< position that is contiguous for the given size.
		///< Data is shuffled around as required, which
		///< means that previous pointers are invalidated.

	char at( size_t n ) const ;
		///< Returns the n'th character.

	std::string str() const ;
		///< Returns the complete string.

private:
	LineStore( const LineStore & ) g__eq_delete ;
	void operator=( const LineStore & ) g__eq_delete ;
	const char * dataimp( size_t pos , size_t size ) ;
	size_t search( std::string::const_iterator , std::string::const_iterator , size_t ) const ;

private:
	std::string m_store ;
	const char * m_extra_data ;
	size_t m_extra_size ;
} ;

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
bool GNet::LineStore::empty() const
{
	return m_store.empty() && m_extra_size == 0U ;
}

#endif
