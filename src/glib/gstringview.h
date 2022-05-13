//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <algorithm>
#include <stdexcept>
#include <ostream>
#include <cstring>

namespace G
{
	template <typename Tchar> class basic_string_view ;
	using string_view = basic_string_view<char> ;
	using wstring_view = basic_string_view<wchar_t> ;

	namespace StringViewImp /// An implementation namespace for G::basic_string_view.
	{
		constexpr unsigned int strlen_constexpr( const char * p ) noexcept
		{
			return *p ? (strlen_constexpr(p+1)+1U) : 0U ;
		}
		constexpr unsigned int strlen_constexpr( const wchar_t * p ) noexcept
		{
			return *p ? (strlen_constexpr(p+1)+1U) : 0U ;
		}
		inline std::size_t strlen( const char * p ) noexcept
		{
			return std::strlen( p ) ;
		}
		inline std::size_t strlen( const wchar_t * p ) noexcept
		{
			return std::wcslen( p ) ;
		}
		constexpr bool same( std::size_t n , const char * p1 , const char * p2 ) noexcept
		{
			return n == 0U ? true : ( *p1 == *p2 && same( n-1U , p1+1 , p2+1 ) ) ;
		}
	}
}

//| \class G::basic_string_view
/// A class template like c++17's std::basic_string_view.
///
/// Compared to std::basic_string_view there is an extra free function
/// for conversion to std::basic_string (G::sv_to_string()) and an
/// extra constructor overload for constinit initialisation:
/// \code
///  static constinit G::string_view sv( "foo" , nullptr ) ;
/// \endcode
///
template <typename Tchar>
class G::basic_string_view
{
public:
	using traits = std::char_traits<Tchar> ;
	using iterator = const Tchar * ;
	using const_iterator = const Tchar * ;
	using size_type = std::size_t ;
	using difference_type = std::ptrdiff_t ;
	using value_type = Tchar ;
	static constexpr std::size_t npos = std::size_t(-1) ;
	basic_string_view() noexcept = default ;
	constexpr basic_string_view( const Tchar * p , std::size_t n ) noexcept :
		m_p(p) ,
		m_n(n)
	{
	}
	basic_string_view( const Tchar * p ) noexcept /*implicit*/ :
		m_p(p) ,
		m_n(p?StringViewImp::strlen(p):0U)
	{
	}
	constexpr basic_string_view( const Tchar * p , std::nullptr_t ) noexcept :
		m_p(p) ,
		m_n(p?StringViewImp::strlen_constexpr(p):0U)
	{
	}
	constexpr static bool same( basic_string_view a , basic_string_view b ) noexcept
	{
		return a.size() == b.size() && ( a.empty() || StringViewImp::same( a.size() , a.data() , b.data() ) ) ;
	}
	constexpr std::size_t size() const noexcept { return m_n ; }
	constexpr std::size_t length() const noexcept { return m_n ; }
	constexpr const Tchar * data() const noexcept { return m_p ; }
	constexpr bool empty() const noexcept { return m_n == 0U ; }
	void swap( basic_string_view<Tchar> & other ) noexcept { std::swap(m_p,other.m_p) ; std::swap(m_n,other.m_n) ; }
	constexpr const Tchar & operator[]( std::size_t i ) const { return m_p[i] ; }
	const Tchar & at( std::size_t i ) const { if( i >= m_n ) throw std::out_of_range("string_view") ; return m_p[i] ; }
	const Tchar * begin() const noexcept { return empty() ? nullptr : m_p ; }
	const Tchar * cbegin() const noexcept { return empty() ? nullptr : m_p ; }
	const Tchar * end() const noexcept { return empty() ? nullptr : (m_p+m_n) ; }
	const Tchar * cend() const noexcept { return empty() ? nullptr : (m_p+m_n) ; }
	bool operator==( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) == 0 ; }
	bool operator!=( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) != 0 ; }
	bool operator<( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) < 0 ; }
	bool operator<=( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) <= 0 ; }
	bool operator>( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) > 0 ; }
	bool operator>=( const basic_string_view<Tchar> & other ) const noexcept { return compare(other) >= 0 ; }
	int compare( const basic_string_view<Tchar> & other ) const noexcept
	{
		int rc = ( empty() || other.empty() ) ? 0 : std::char_traits<Tchar>::compare( m_p , other.m_p , std::min(m_n,other.m_n) ) ;
		return rc == 0 ? ( m_n < other.m_n ? -1 : (m_n==other.m_n?0:1) ) : rc ;
	}
	string_view substr( std::size_t pos , std::size_t count = npos ) const
	{
		if( empty() || pos > m_n ) throw std::out_of_range( "string_view" ) ;
		if( pos == m_n ) return string_view( m_p , std::size_t(0U) ) ; // (more than the standard requires)
		return string_view( m_p + pos , std::min(m_n-pos,count) ) ;
	}
	std::size_t find( Tchar c , std::size_t pos = 0U ) const noexcept
	{
		if( empty() || pos >= m_n ) return std::string::npos ;
		const Tchar * p = m_p + pos ;
		std::size_t n = m_n - pos ;
		for( ; n ; p++ , n-- , pos++ )
		{
			if( *p == c )
				return pos ;
		}
		return std::string::npos ;
	}
	std::size_t find( const Tchar * substr_p , std::size_t pos , std::size_t substr_n ) const
	{
		return find( basic_string_view<Tchar>(substr_p,substr_n) , pos ) ;
	}
	std::size_t find( basic_string_view<Tchar> substr , std::size_t pos = 0U ) const
	{
		if( empty() || pos >= m_n ) return std::string::npos ;
		if( substr.empty() ) return pos ;
		auto const end = m_p + m_n ;
		auto p = std::search( m_p+pos , end , substr.m_p , substr.m_p+substr.m_n ) ;
		return p == end ? std::string::npos : std::distance(m_p,p) ;
	}
	std::size_t find_first_of( const Tchar * chars , std::size_t pos , std::size_t chars_size ) const noexcept
	{
		return find_first_of( basic_string_view<Tchar>(chars,chars_size) , pos ) ;
	}
	std::size_t find_first_of( basic_string_view<Tchar> chars , std::size_t pos = 0U ) const noexcept
	{
		if( empty() || pos >= m_n || chars.empty() ) return std::string::npos ;
		const Tchar * p = m_p + pos ;
		std::size_t n = m_n - pos ;
		for( ; n ; p++ , n-- , pos++ )
		{
			const std::size_t i_end = chars.size() ;
			for( std::size_t i = 0U ; i < i_end ; i++ )
			{
				if( *p == chars[i] )
					return pos ;
			}
		}
		return std::string::npos ;
	}
	std::size_t find_first_not_of( const Tchar * chars , std::size_t pos , std::size_t chars_size ) const noexcept
	{
		return find_first_not_of( basic_string_view<Tchar>(chars,chars_size) , pos ) ;
	}
	std::size_t find_first_not_of( basic_string_view<Tchar> chars , std::size_t pos = 0U ) const noexcept
	{
		if( empty() || pos >= m_n || chars.empty() ) return std::string::npos ;
		const Tchar * p = m_p + pos ;
		std::size_t n = m_n - pos ;
		for( ; n ; p++ , n-- , pos++ )
		{
			bool match = false ;
			const std::size_t i_end = chars.size() ;
			for( std::size_t i = 0U ; !match && i < i_end ; i++ )
			{
				if( *p == chars[i] )
					match = true ;
			}
			if( !match )
				return pos ;
		}
		return std::string::npos ;
	}
	std::basic_string<Tchar> sv_to_string_imp() const
	{
		return empty() ? std::basic_string<Tchar>() : std::basic_string<Tchar>( m_p , m_n ) ;
	}

private:
	const Tchar * m_p{nullptr} ;
	std::size_t m_n{0U} ;
} ;

namespace G
{
	template <typename Tchar> std::basic_string<Tchar> sv_to_string( basic_string_view<Tchar> sv )
	{
		// (greppable name -- remove when using c++17 std::string_view)
		return std::basic_string<Tchar>( sv.sv_to_string_imp() ) ;
	}
	inline std::ostream & operator<<( std::ostream & stream , const string_view & sv )
	{
		if( !sv.empty() )
			stream.write( sv.data() , sv.size() ) ; // NOLINT narrowing
		return stream ;
	}
	inline std::wostream & operator<<( std::wostream & stream , const wstring_view & sv )
	{
		if( !sv.empty() )
			stream.write( sv.data() , sv.size() ) ; // NOLINT narrowing
		return stream ;
	}
	template <typename Tchar> void swap( basic_string_view<Tchar> & a , basic_string_view<Tchar> b ) noexcept
	{
		a.swap( b ) ;
	}
	inline bool operator==( const std::string & s , string_view sv )
	{
		return sv.empty() ? s.empty() : ( 0 == s.compare( 0 , s.size() , sv.data() , sv.size() ) ) ;
	}
	inline bool operator==( string_view sv , const std::string & s )
	{
		return sv.empty() ? s.empty() : ( 0 == s.compare( 0 , s.size() , sv.data() , sv.size() ) ) ;
	}
	inline bool operator!=( const std::string & s , string_view sv )
	{
		return !(s == sv) ;
	}
	inline bool operator!=( string_view sv , const std::string & s )
	{
		return !(sv == s) ;
	}
}

constexpr G::basic_string_view<char> operator "" _sv( const char * p , std::size_t n )
{
	return G::basic_string_view<char>( p , n ) ;
}

#endif
