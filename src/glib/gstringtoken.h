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
/// \file gstringtoken.h
///

#ifndef G_STRING_TOKEN_H
#define G_STRING_TOKEN_H

#include "gdef.h"
#include "gassert.h"
#include "gstringview.h"
#include <string>
#include <type_traits>
#include <algorithm>

namespace G
{
	template <typename T> class StringTokenT ;
	using StringToken = StringTokenT<std::string> ;
	using StringTokenView = StringTokenT<string_view> ;
}

//| \class G::StringTokenT
/// A zero-copy string token iterator where the token separators
/// are runs of whitespace characters, with no support for escape
/// characters. Leading and trailing whitespace are not significant.
/// Empty whitespace yields a single token. Stepping beyond the
/// last token is allowed.
///
/// \code
/// for( StringToken t(s," \t",2U) ; t ; ++t )
///   std::cout << t() << "\n" ;
/// \endcode
///
/// \see G::Str::splitIntoTokens()
///
template <typename T>
class G::StringTokenT
{
public:
	using char_type = typename T::value_type ;

	StringTokenT( const T & s , const char_type * ws , std::size_t wsn ) noexcept ;
		///< Constructor. The parameters must stay valid for
		///< the object lifefime.
		///<
		///< The rvalue overload is deleted to avoid passing a
		///< temporary T that has been implicitly constructed from
		///< something else. Temporary string_views constructed
		///< from a string would be safe, but might be unsafe for
		///< other types.

	StringTokenT( const T & s , string_view ws ) noexcept ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.

	bool valid() const noexcept ;
		///< Returns true if a valid token position.

	explicit operator bool() const noexcept ;
		///< Returns true if a valid token.

	const char_type * data() const noexcept ;
		///< Returns the current token pointer.

	std::size_t size() const noexcept ;
		///< Returns the current token size.

	std::size_t pos() const noexcept ;
		///< Returns the offset of data().

	T operator()() const noexcept(std::is_same<T,string_view>::value) ;
		///< Returns the current token substring or T() if
		///< not valid().

	StringTokenT<T> & operator++() noexcept ;
		///< Moves to the next token.

	StringTokenT<T> & next() noexcept ;
		///< Moves to the next token.

public:
	~StringTokenT() = default ;
	StringTokenT( T && s , const char_type * , std::size_t ) = delete ;
	StringTokenT( T && s , string_view ) = delete ;
	StringTokenT( const StringTokenT<T> & ) = delete ;
	StringTokenT( StringTokenT<T> && ) = delete ;
	StringTokenT<T> & operator=( const StringTokenT<T> & ) = delete ;
	StringTokenT<T> & operator=( StringTokenT<T> && ) = delete ;

private:
	static constexpr std::size_t npos = T::npos ;
	const T & m_s ;
	const char_type * m_ws ;
	std::size_t m_wsn ;
	std::size_t m_pos ;
	std::size_t m_endpos ;
} ;

namespace G
{
	namespace StringTokenImp /// An implementation namespace for G::StringToken.
	{
		template <typename T> inline T substr( const T & s ,
			std::size_t pos , std::size_t len ) noexcept(std::is_same<T,string_view>::value)
		{
			return s.substr( pos , len ) ;
		}
		template <> inline string_view substr<string_view>( const string_view & s ,
			std::size_t pos , std::size_t len ) noexcept
		{
			return sv_substr( s , pos , len ) ;
		}
		static_assert( !noexcept(std::string().substr(0,0)) , "" ) ;
		static_assert( noexcept(sv_substr(string_view(),0,0)) , "" ) ;
	}
}

template <typename T>
G::StringTokenT<T>::StringTokenT( const T & s , const char_type * ws , std::size_t wsn ) noexcept :
	m_s(s) ,
	m_ws(ws) ,
	m_wsn(wsn) ,
	m_pos(s.empty()?npos:s.find_first_not_of(m_ws,0U,m_wsn)) ,
	m_endpos(s.find_first_of(m_ws,m_pos,m_wsn))
{
	G_ASSERT( !(s.empty()) || ( m_pos == npos && m_endpos == npos ) ) ;
	G_ASSERT( !(!s.empty() && wsn==0U) || ( m_pos == 0U && m_endpos == npos ) ) ;
	G_ASSERT( !(m_pos == npos) || m_endpos == npos ) ;
}

template <typename T>
G::StringTokenT<T>::StringTokenT( const T & s , string_view ws ) noexcept :
	m_s(s) ,
	m_ws(ws.data()) ,
	m_wsn(ws.size()) ,
	m_pos(s.empty()?npos:s.find_first_not_of(m_ws,0U,m_wsn)) ,
	m_endpos(s.find_first_of(m_ws,m_pos,m_wsn))
{
}

template <typename T>
const typename T::value_type * G::StringTokenT<T>::data() const noexcept
{
	//static_assert( __cplusplus >= 201100 , "" ) ; // broken on msvc
	return m_s.data() + (m_pos==npos?0U:m_pos) ;
}

template <typename T>
std::size_t G::StringTokenT<T>::size() const noexcept
{
	return (m_endpos==npos?m_s.size():m_endpos) - m_pos ;
}

template <typename T>
std::size_t G::StringTokenT<T>::pos() const noexcept
{
	return m_pos == npos ? 0U : m_pos ;
}

template <typename T>
G::StringTokenT<T>::operator bool() const noexcept
{
	return m_pos != npos ;
}

template <typename T>
bool G::StringTokenT<T>::valid() const noexcept
{
	return m_pos != npos ;
}

template <typename T>
T G::StringTokenT<T>::operator()() const noexcept(std::is_same<T,string_view>::value)
{
	using string_type = T ;
	return m_pos == npos ? string_type{} : StringTokenImp::substr( m_s , m_pos , size() ) ;
}

template <typename T>
G::StringTokenT<T> & G::StringTokenT<T>::operator++() noexcept
{
	m_pos = m_s.find_first_not_of( m_ws , m_endpos , m_wsn ) ;
	m_endpos = m_s.find_first_of( m_ws , m_pos , m_wsn ) ;
	G_ASSERT( !(m_s.empty()) || ( m_pos == npos && m_endpos == npos ) ) ;
	G_ASSERT( !(!m_s.empty() && m_wsn==0U) || ( m_pos == npos && m_endpos == npos ) ) ;
	G_ASSERT( !(m_pos == npos) || m_endpos == npos ) ;
	return *this ;
}

template <typename T>
G::StringTokenT<T> & G::StringTokenT<T>::next() noexcept
{
	return ++(*this) ;
}

#endif
