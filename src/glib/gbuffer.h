//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gbuffer.h
///

#ifndef G_BUFFER_H
#define G_BUFFER_H

#include "gdef.h"
#include "gassert.h"
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <cstring>
#include <memory>
#include <new>

namespace G
{
	//| \class G::Buffer
	/// A substitute for std::vector<char> that has more useful alignment
	/// guarantees and explicitly avoids default initialisation of each
	/// element. The alignment is that of std::malloc(), ie. std::max_align_t.
	/// (See also posix_memalign().)
	///
	/// The buffer_cast() free function can be used to return a pointer
	/// to the start of the buffer for some aggregate type, throwing if
	/// the buffer is too small for a complete object:
	///
	/// \code
	/// Buffer<char> buffer( 100 ) ;
	/// auto n = fill( buffer ) ;
	/// buffer.resize( n ) ;
	/// Foo * foo_p = buffer_cast<Foo*>( buffer ) ;
	/// ...
	/// foo_p->~Foo() ;
	/// \endcode
	///
	/// The implementation of buffer_cast uses placement-new in order to
	/// avoid the undefined behaviour of a pointer cast. If the buffer_cast
	/// type has a non-trivial destructor then the destructor should be
	/// called explicitly through the pointer before the G::Buffer object
	/// disappears.
	///
	template <typename T>
	struct Buffer
	{
		static_assert( sizeof(T) == 1 , "sizeof t is one" ) ;
		using value_type = T ;
		using iterator = T * ;
		using const_iterator = const T * ;
		Buffer() noexcept = default ;
		explicit Buffer( std::size_t n ) : m_n(n) , m_c(n)
		{
			m_p = static_cast<T*>( std::malloc(n) ) ; // NOLINT cppcoreguidelines-no-malloc
			checkalloc() ;
		}
		~Buffer()
		{
			std::free( m_p ) ; // NOLINT cppcoreguidelines-no-malloc
		}
		void reserve( std::size_t n )
		{
			if( n > m_c )
			{
				T * new_p = static_cast<T*>( std::realloc(m_p,n) ) ; // NOLINT cppcoreguidelines-no-malloc
				m_p = checkalloc( new_p ) ;
				m_c = n ;
			}
		}
		void resize( std::size_t n ) { reserve( n ) ; m_n = n ; }
		const T & operator[]( std::size_t i ) const noexcept { return *(m_p+i) ; }
		T & operator[]( std::size_t i ) noexcept { return *(m_p+i) ; }
		const T & at( std::size_t i ) const { checkindex( i ) ; return *(m_p+i) ; }
		T & at( std::size_t i ) { checkindex( i ) ; return *(m_p+i) ; }
		std::size_t size() const noexcept { return m_n ; }
		std::size_t capacity() const noexcept { return m_c ; }
		bool empty() const noexcept { return m_n == 0U ; }
		void clear() noexcept { m_n = 0 ; }
		void shrink_to_fit() noexcept { if( empty() && m_p ) { std::free( m_p ) ; m_p = nullptr ; m_c = 0U ; } } // NOLINT cppcoreguidelines-no-malloc
		iterator begin() noexcept { return m_p ? m_p : &m_c0 ; }
		iterator end() noexcept { return m_p ? (m_p+m_n) : &m_c0 ; }
		const value_type * data() const noexcept { return m_p ? m_p : &m_c0 ; }
		value_type * data() noexcept { return m_p ? m_p : &m_c0 ; }
		const_iterator begin() const noexcept { return m_p ? m_p : &m_c0 ; }
		const_iterator end() const noexcept { return m_p ? (m_p+m_n) : &m_c0 ; }
		const_iterator cbegin() const noexcept { return m_p ? m_p : &m_c0 ; }
		const_iterator cend() const noexcept { return m_p ? (m_p+m_n) : &m_c0 ; }
		iterator erase( iterator range_begin , iterator range_end ) noexcept
		{
			if( range_end == end() )
			{
				m_n = std::distance( begin() , range_begin ) ;
			}
			else if( range_begin != range_end )
			{
				std::size_t range = std::distance( range_begin , range_end ) ;
				std::size_t end_offset = std::distance( begin() , range_end ) ;
				std::memmove( range_begin , range_end , m_n-end_offset ) ;
				m_n -= range ;
			}
			return range_begin ;
		}
		template <typename U> void insert( iterator p , U range_begin , U range_end )
		{
			std::size_t range = std::distance( range_begin , range_end ) ;
			if( range )
			{
				p = makespace( p , range ) ;
				std::copy( range_begin , range_end , p ) ;
			}
		}
		template <typename U> void insert( iterator p , U * range_begin , U * range_end )
		{
			std::size_t range = std::distance( range_begin , range_end ) ;
			if( range )
			{
				p = makespace( p , range ) ;
				std::memcpy( p , range_begin , range ) ;
			}
		}
		void swap( Buffer<T> & other ) noexcept
		{
			std::swap( m_n , other.m_n ) ;
			std::swap( m_p , other.m_p ) ;
			std::swap( m_c , other.m_c ) ;
		}
		Buffer( const Buffer<T> & other )
		{
			if( other.m_n )
			{
				resize( other.m_n ) ;
				std::memcpy( m_p , other.m_p , m_n ) ;
			}
		}
		Buffer( Buffer<T> && other ) noexcept
		{
			swap( other ) ;
		}
		Buffer<T> & operator=( const Buffer<T> & other )
		{
			Buffer<T>(other).swap(*this) ;
			return *this ;
		}
		Buffer<T> & operator=( Buffer<T> && other ) noexcept
		{
			Buffer<T>(std::move(other)).swap(*this) ;
			return *this ;
		}
		template <typename U> T * aligned()
		{
			if( m_n == 0U || m_p == nullptr ) return nullptr ;
			void * vp = m_p ;
			std::size_t space = m_n ;
			void * result = std::align( alignof(U) , sizeof(U) , vp , space ) ;
			return static_cast<T*>(result) ;
		}
		private:
		iterator makespace( iterator p , std::size_t range )
		{
			G_ASSERT( p >= begin() && p <= end() ) ;
			G_ASSERT( range != 0U ) ;
			std::size_t head = std::distance( begin() , p ) ;
			std::size_t tail = std::distance( p , end() ) ;
			resize( m_n + range ) ;
			p = m_p + head ; // since p invalidated by resize()
			std::memmove( p+range , p , tail ) ;
			return p ;
		}
		void checkalloc() { checkalloc(m_p) ; }
		T * checkalloc( T * p ) { if( p == nullptr ) throw std::bad_alloc() ; return p ; }
		void checkindex( std::size_t i ) const { if( i >= m_n ) throw std::out_of_range("G::Buffer") ; }
		char * m_p {nullptr} ;
		std::size_t m_n {0U} ;
		std::size_t m_c {0U} ;
		value_type m_c0 {'\0'} ;
	} ;

	template <typename Uptr, typename T = char>
	Uptr buffer_cast( Buffer<T> & buffer )
	{
		using U = typename std::remove_pointer<Uptr>::type ;
		T * p = buffer.template aligned<U>() ;
		G_ASSERT( p == nullptr || p == buffer.data() ) ; // assert malloc is behaving
		if( p != buffer.data() )
			throw std::bad_cast() ; // buffer too small for a U
		return new(p) U ;
	}

	template <typename Uptr, typename T = char>
	Uptr buffer_cast( Buffer<T> & buffer , std::nothrow_t )
	{
		using U = typename std::remove_pointer<Uptr>::type ;
		T * p = buffer.template aligned<U>() ;
		G_ASSERT( p == nullptr || p == buffer.data() ) ; // assert malloc is behaving
		if( p != buffer.data() )
			return nullptr ; // buffer too small for a U
		return new(p) U ;
	}

	template <typename Uptr, typename T = char>
	Uptr buffer_cast( const Buffer<T> & buffer )
	{
		using U = typename std::remove_pointer<Uptr>::type ;
		return const_cast<Uptr>( buffer_cast<U*>( const_cast<Buffer<T>&>(buffer) ) ) ;
	}

	template <typename T> void swap( Buffer<T> & a , Buffer<T> & b ) noexcept
	{
		a.swap( b ) ;
	}
}

#endif
