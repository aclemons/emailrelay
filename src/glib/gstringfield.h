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
/// \file gstringfield.h
///

#ifndef G_STRING_FIELD_H
#define G_STRING_FIELD_H

#include "gdef.h"
#include "gassert.h"
#include <string>
#include <algorithm>

namespace G
{
	template <typename T> class StringFieldT ;
	using StringField = StringFieldT<std::string> ;
}

//| \class G::StringFieldT
/// A zero-copy string field iterator where the field separators
/// are short fixed strings.
///
/// \code
/// for( StringField f(s,"\r\n",2U) ; f ; ++f )
///   std::cout << std::string_view(f.data(),f.size()) << "\n" ;
/// \endcode
///
/// \see G::Str::splitIntoFields(), G::SubString
///
template <typename T>
class G::StringFieldT
{
public:
	using char_type = typename T::value_type ;

	StringFieldT( const T & s , const char_type * sep , std::size_t sepn ) ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.

	explicit operator bool() const ;
		///< Returns true if a valid field.

	T operator()() const ;
		///< Returns the current field substring. Prefer data()
		///< and size() to avoid copying.

	StringFieldT<T> & operator++() ;
		///< Moves to the next field.

	const char_type * data() const ;
		///< Returns the current field pointer.

	std::size_t size() const ;
		///< Returns the current field size.

	bool first() const ;
		///< Returns true if the current field is the first.

	bool last() const ;
		///< Returns true if the current field is the last.

public:
	~StringFieldT() = default ;
	StringFieldT( T && s , const char * , std::size_t ) = delete ;
	StringFieldT( const StringFieldT<T> & ) = delete ;
	StringFieldT( StringFieldT<T> && ) = delete ;
	void operator=( const StringFieldT<T> & ) = delete ;
	void operator=( StringFieldT<T> && ) = delete ;

private:
	const T * const m_p ;
	const char_type * m_sep ;
	std::size_t m_sepn ;
	std::size_t m_fpos ;
	std::size_t m_fendpos ;
} ;

template <typename T>
G::StringFieldT<T>::StringFieldT( const T & s , const char_type * sep_p , std::size_t sep_n ) :
	m_p(&s) ,
	m_sep(sep_p) ,
	m_sepn(sep_n) ,
	m_fpos(s.empty()?std::string::npos:0U) ,
	m_fendpos(s.find(m_sep,0U,m_sepn))
{
}

template <typename T>
const typename T::value_type * G::StringFieldT<T>::data() const
{
	G_ASSERT( m_fpos != std::string::npos ) ;
	return m_p->data() + m_fpos ;
}

template <typename T>
std::size_t G::StringFieldT<T>::size() const
{
	G_ASSERT( m_fpos != std::string::npos ) ;
	return (m_fendpos==std::string::npos?m_p->size():m_fendpos) - m_fpos ;
}

template <typename T>
G::StringFieldT<T>::operator bool() const
{
	return m_fpos != std::string::npos ;
}

template <typename T>
T G::StringFieldT<T>::operator()() const
{
	if( m_fpos == std::string::npos ) return {} ;
	return m_p->substr( m_fpos , size() ) ;
}

template <typename T>
G::StringFieldT<T> & G::StringFieldT<T>::operator++()
{
	m_fpos = m_fendpos ;
	if( m_fpos != std::string::npos )
	{
		m_fpos = std::min( m_p->size() , m_fpos + m_sepn ) ;
		m_fendpos = m_p->find( m_sep , m_fpos , m_sepn ) ;
	}
	return *this ;
}

template <typename T>
bool G::StringFieldT<T>::first() const
{
	return m_fpos == 0U ;
}

template <typename T>
bool G::StringFieldT<T>::last() const
{
	return m_fendpos == std::string::npos ;
}

#endif
