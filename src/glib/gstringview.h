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
/// \file gstringview.h
///

#ifndef G_STRING_VIEW_H
#define G_STRING_VIEW_H

#include "gdef.h"
#include <string>
#include <cstring>
#include <new>

#if GCONFIG_HAVE_CXX_STRING_VIEW

#include <string_view>

namespace G
{
	std::string_view sv_substr_noexcept( std::string_view sv , std::size_t pos , std::size_t count = std::string::npos ) noexcept ;
	bool sv_imatch( std::string_view , std::string_view ) noexcept ;
	inline std::string sv_to_string( std::string_view sv ) { return std::string(sv) ; }
}

constexpr std::string_view operator "" _sv( const char * p , std::size_t n ) noexcept
{
	return {p,n} ;
}

#else

#include <stdexcept>
#include <ostream>
#include <algorithm> // std::min()

namespace G
{
	class string_view ;
	G::string_view sv_substr_noexcept( G::string_view sv , std::size_t pos , std::size_t count = std::string::npos ) noexcept ;
	bool sv_imatch( G::string_view , G::string_view ) noexcept ;
}

namespace std /// NOLINT
{
	using string_view = G::string_view ;
}

//| \class G::string_view
/// A class like c++17's std::string_view.
///
/// There is an implicit conversion constructor from std::string
/// since std::string has its convertion operator "operator sv()".
///
class G::string_view
{
public:
	using traits = std::char_traits<char> ;
	using iterator = const char * ;
	using const_iterator = const char * ;
	using size_type = std::size_t ;
	using difference_type = std::ptrdiff_t ;
	using value_type = char ;
	static constexpr std::size_t npos = std::size_t(-1) ;
	string_view() noexcept = default ;
	string_view( std::nullptr_t ) = delete ;
	static constexpr /*consteval*/ std::size_t strlen_imp( const char * p , std::size_t n = 0U ) noexcept { return *p ? strlen_imp(p+1,n+1U) : n ; }
	constexpr string_view( const char * p , std::size_t n ) noexcept : m_p(p) , m_n(n) {}
	constexpr /*consteval*/ string_view( const char * p , std::nothrow_t ) noexcept : m_p(p) , m_n(p?strlen_imp(p):0U) {}
	string_view( const char * p ) noexcept /*implicit*/ : m_p(p) , m_n(p?std::strlen(p):0U) {}
	string_view( const std::string & s ) noexcept /* implicit */ : m_p(s.data()) , m_n(s.size()) {}
	constexpr std::size_t size() const noexcept { return m_n ; }
	constexpr std::size_t length() const noexcept { return m_n ; }
	constexpr const char * data() const noexcept { return m_p ; }
	constexpr bool empty() const noexcept { return m_n == 0U ; }
	void swap( string_view & other ) noexcept { std::swap(m_p,other.m_p) ; std::swap(m_n,other.m_n) ; }
	constexpr const char & operator[]( std::size_t i ) const noexcept { return m_p[i] ; }
	const char & at( std::size_t i ) const { if( i >= m_n ) throw std::out_of_range("string_view::at") ; return m_p[i] ; }
	const char * begin() const noexcept { return empty() ? nullptr : m_p ; }
	const char * cbegin() const noexcept { return empty() ? nullptr : m_p ; }
	const char * end() const noexcept { return empty() ? nullptr : (m_p+m_n) ; }
	const char * cend() const noexcept { return empty() ? nullptr : (m_p+m_n) ; }
	int compare( const string_view & other ) const noexcept {
    	int rc = ( empty() || other.empty() ) ? 0 : std::char_traits<char>::compare( m_p , other.m_p , std::min(m_n,other.m_n) ) ;
    	return rc == 0 ? ( m_n < other.m_n ? -1 : (m_n==other.m_n?0:1) ) : rc ;
	}
	int compare( std::size_t pos , std::size_t count , string_view other ) const noexcept { return substr(pos,count).compare(other) ; }
	bool operator==( const string_view & other ) const noexcept { return compare(other) == 0 ; }
	bool operator!=( const string_view & other ) const noexcept { return compare(other) != 0 ; }
	bool operator<( const string_view & other ) const noexcept { return compare(other) < 0 ; }
	bool operator<=( const string_view & other ) const noexcept { return compare(other) <= 0 ; }
	bool operator>( const string_view & other ) const noexcept { return compare(other) > 0 ; }
	bool operator>=( const string_view & other ) const noexcept { return compare(other) >= 0 ; }

	string_view substr( std::size_t pos , std::size_t count = npos ) const ;
	std::size_t find( char c , std::size_t pos = 0U ) const noexcept ;
	std::size_t find( const char * substr_p , std::size_t pos , std::size_t substr_n ) const ;
	std::size_t find( string_view substr , std::size_t pos = 0U ) const ;
	std::size_t find_first_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept ;
	std::size_t find_first_of( string_view chars , std::size_t pos = 0U ) const noexcept ;
	std::size_t find_first_not_of( char c , std::size_t pos = 0U ) const noexcept ;
	std::size_t find_first_not_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept ;
	std::size_t find_first_not_of( string_view chars , std::size_t pos = 0U ) const noexcept ;
	std::size_t find_last_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept ;
	std::size_t find_last_of( string_view chars , std::size_t pos = std::string::npos ) const noexcept ;
	std::size_t find_last_not_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept ;
	std::size_t find_last_not_of( string_view chars , std::size_t pos = std::string::npos ) const noexcept ;
	std::size_t rfind( char c , std::size_t pos = std::string::npos ) const noexcept ;

private:
	const char * m_p {nullptr} ;
	std::size_t m_n {0U} ;
} ;

namespace G
{
	inline std::ostream & operator<<( std::ostream & stream , const string_view & sv )
	{
		if( !sv.empty() )
			stream.write( sv.data() , sv.size() ) ; // NOLINT narrowing
		return stream ;
	}
	inline void swap( string_view & a , string_view b ) noexcept
	{
		a.swap( b ) ;
	}
	inline bool operator==( const std::string & s , string_view sv )
	{
		return sv.empty() ? s.empty() : ( 0 == s.compare( 0 , s.size() , sv.data() , sv.size() ) ) ;
	}
	inline bool operator!=( const std::string & s , string_view sv )
	{
		return !(s == sv) ;
	}
	inline std::string sv_to_string( string_view sv )
	{
		return sv.empty() ? std::string() : std::string( sv.data() , sv.size() ) ;
	}
}

namespace std /// NOLINT
{
	inline bool operator<( const string & s , G::string_view sv ) // NOLINT
	{
		return s.compare( 0 , s.size() , sv.data() , sv.size() ) < 0 ;
	}
}

constexpr G::string_view operator "" _sv( const char * p , std::size_t n ) noexcept
{
	return {p,n} ;
}

#endif

#endif
