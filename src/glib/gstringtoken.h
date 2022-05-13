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
/// \file gstringtoken.h
///

#ifndef G_STRING_TOKEN_H
#define G_STRING_TOKEN_H

#include "gdef.h"
#include "gassert.h"
#include <string>
#include <algorithm>

namespace G
{
	template <typename T> class StringTokenT ;
	using StringToken = StringTokenT<std::string> ;
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

	StringTokenT( const T & s , const char_type * ws , std::size_t wsn ) ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.

	template <typename Tsv> StringTokenT( const T & s , Tsv ws ) ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.

	explicit operator bool() const ;
		///< Returns true if a valid token.

	const char_type * data() const ;
		///< Returns the current token pointer.

	std::size_t size() const ;
		///< Returns the current token size.

	T operator()() const ;
		///< Returns the current token substring.

	StringTokenT<T> & operator++() ;
		///< Moves to the next token.

	StringTokenT<T> & next() ;
		///< Moves to the next token.

public:
	~StringTokenT() = default ;
	StringTokenT( T && s , const char_type * , std::size_t ) = delete ;
	StringTokenT( const StringTokenT<T> & ) = delete ;
	StringTokenT( StringTokenT<T> && ) = delete ;
	void operator=( const StringTokenT<T> & ) = delete ;
	void operator=( StringTokenT<T> && ) = delete ;

private:
	static constexpr std::size_t npos = T::npos ;
	const T * m_p ;
	const char_type * m_ws ;
	std::size_t m_wsn ;
	std::size_t m_pos ;
	std::size_t m_endpos ;
} ;

template <typename T>
G::StringTokenT<T>::StringTokenT( const T & s , const char_type * ws , std::size_t wsn ) :
	m_p(&s) ,
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
template <typename Tsv>
G::StringTokenT<T>::StringTokenT( const T & s , Tsv ws ) :
	m_p(&s) ,
	m_ws(ws.data()) ,
	m_wsn(ws.size()) ,
	m_pos(s.empty()?npos:s.find_first_not_of(m_ws,0U,m_wsn)) ,
	m_endpos(s.find_first_of(m_ws,m_pos,m_wsn))
{
}

template <typename T>
const typename T::value_type * G::StringTokenT<T>::data() const
{
	static_assert( __cplusplus >= 201100 , "" ) ;
	return m_p->data() + (m_pos==npos?0U:m_pos) ;
}

template <typename T>
std::size_t G::StringTokenT<T>::size() const
{
	return (m_endpos==npos?m_p->size():m_endpos) - m_pos ;
}

template <typename T>
G::StringTokenT<T>::operator bool() const
{
	return m_pos != npos ;
}

template <typename T>
T G::StringTokenT<T>::operator()() const
{
	using string_type = T ;
	return m_pos == npos ? string_type{} : m_p->substr( m_pos , size() ) ;
}

template <typename T>
G::StringTokenT<T> & G::StringTokenT<T>::operator++()
{
	m_pos = m_p->find_first_not_of( m_ws , m_endpos , m_wsn ) ;
	m_endpos = m_p->find_first_of( m_ws , m_pos , m_wsn ) ;
	G_ASSERT( !(m_p->empty()) || ( m_pos == npos && m_endpos == npos ) ) ;
	G_ASSERT( !(!m_p->empty() && m_wsn==0U) || ( m_pos == npos && m_endpos == npos ) ) ;
	G_ASSERT( !(m_pos == npos) || m_endpos == npos ) ;
	return *this ;
}

template <typename T>
G::StringTokenT<T> & G::StringTokenT<T>::next()
{
	return ++(*this) ;
}

#endif
