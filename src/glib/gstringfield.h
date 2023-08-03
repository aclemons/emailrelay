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
/// \file gstringfield.h
///

#ifndef G_STRING_FIELD_H
#define G_STRING_FIELD_H

#include "gdef.h"
#include "gstringview.h"
#include "gassert.h"
#include <string>
#include <algorithm>
#include <iterator>
#include <cstddef> // std::nullptr_t

namespace G
{
	template <typename T> class StringFieldT ;
	template <typename T> class StringFieldIteratorT ;
	using StringField = StringFieldT<std::string> ;
	using StringFieldView = StringFieldT<string_view> ;
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

	StringFieldT( const T & s , const char_type * sep , std::size_t sepn ) noexcept ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.
		///<
		///< The rvalue overload is deleted to avoid passing a
		///< temporary T that has been implicitly constructed from
		///< something else. Temporary string_views constructed
		///< from a string would be safe, but might be unsafe for
		///< other types.

	StringFieldT( const T & s , char_type sep ) noexcept ;
		///< Constructor. The parameters must stay valid
		///< for the object lifefime.

	explicit operator bool() const noexcept ;
		///< Returns true if a valid field.

	bool valid() const noexcept ;
		///< Returns true if a valid field.

	static constexpr bool deref_operator_noexcept = std::is_same<T,string_view>::value ;

	T operator()() const noexcept(deref_operator_noexcept) ;
		///< Returns the current field substring. Prefer data()
		///< and size() to avoid copying.

	StringFieldT<T> & operator++() noexcept ;
		///< Moves to the next field.

	const char_type * data() const noexcept ;
		///< Returns the current field pointer.

	std::size_t size() const noexcept ;
		///< Returns the current field size.

	bool first() const noexcept ;
		///< Returns true if the current field is the first.

	bool last() const noexcept ;
		///< Returns true if the current field is the last.

	std::size_t count() const noexcept ;
		///< Returns the number of fields.

public:
	~StringFieldT() = default ;
	StringFieldT( T && s , const char * , std::size_t ) = delete ;
	StringFieldT( T && s , char ) = delete ;
	StringFieldT( const StringFieldT<T> & ) = delete ;
	StringFieldT( StringFieldT<T> && ) = delete ;
	StringFieldT<T> & operator=( const StringFieldT<T> & ) = delete ;
	StringFieldT<T> & operator=( StringFieldT<T> && ) = delete ;

private:
	const T & m_s ;
	char m_c ;
	const char_type * m_sep ;
	std::size_t m_sepn ;
	std::size_t m_fpos ;
	std::size_t m_fendpos ;
} ;

//| \class G::StringFieldIteratorT
/// A standard forward iterator for G::StringFieldT:
/// \code
/// StringFieldView f( "foo,bar"_sv , "," , 1U ) ;
/// std::copy( begin(f) , end(f) , std::back_inserter(list) ) ; // or...
/// for( string_view sv : f ) list.push_back( sv ) ;
/// \endcode
///
template <typename T>
class G::StringFieldIteratorT
{
public:
	using iterator_category = std::forward_iterator_tag ;
	using value_type = T ;
	using difference_type = int ;
	using pointer = T* ;
	using reference = T& ;
	explicit StringFieldIteratorT( StringFieldT<T> & ) noexcept ;
	explicit StringFieldIteratorT( std::nullptr_t ) noexcept ;
	StringFieldIteratorT & operator++() noexcept ;
	T operator*() const noexcept(StringFieldT<T>::deref_operator_noexcept) ;
	bool operator==( StringFieldIteratorT other ) const noexcept ;
	bool operator!=( StringFieldIteratorT other ) const noexcept ;
private:
	StringFieldT<T> * f ;
} ;

namespace G
{
	namespace StringFieldImp /// An implementation namespace for G::StringField.
	{
		template <typename T> inline T substr( const T & s ,
			std::size_t pos , std::size_t len ) noexcept(std::is_same<T,string_view>::value)
		{
			return s.substr( pos , len ) ;
		}
		template <> string_view inline substr<string_view>( const string_view & s ,
			std::size_t pos , std::size_t len ) noexcept
		{
			return sv_substr( s , pos , len ) ;
		}
		static_assert( !noexcept(std::string().substr(0,0)) , "" ) ;
		static_assert( noexcept(sv_substr(string_view(),0,0)) , "" ) ;
	}
}

template <typename T>
G::StringFieldT<T>::StringFieldT( const T & s , const char_type * sep_p , std::size_t sep_n ) noexcept :
	m_s(s) ,
	m_c('\0') ,
	m_sep(sep_p) ,
	m_sepn(sep_n) ,
	m_fpos(s.empty()?std::string::npos:0U) ,
	m_fendpos(s.find(m_sep,0U,m_sepn))
{
}

template <typename T>
G::StringFieldT<T>::StringFieldT( const T & s , char_type sep ) noexcept :
	m_s(s) ,
	m_c(sep) ,
	m_sep(&m_c) ,
	m_sepn(1U) ,
	m_fpos(s.empty()?std::string::npos:0U) ,
	m_fendpos(s.find(m_sep,0U,m_sepn))
{
}

template <typename T>
const typename T::value_type * G::StringFieldT<T>::data() const noexcept
{
	G_ASSERT( m_fpos != std::string::npos ) ;
	return m_s.data() + m_fpos ;
}

template <typename T>
std::size_t G::StringFieldT<T>::size() const noexcept
{
	G_ASSERT( m_fpos != std::string::npos ) ;
	return (m_fendpos==std::string::npos?m_s.size():m_fendpos) - m_fpos ;
}

template <typename T>
G::StringFieldT<T>::operator bool() const noexcept
{
	return m_fpos != std::string::npos ;
}

template <typename T>
bool G::StringFieldT<T>::valid() const noexcept
{
	return m_fpos != std::string::npos ;
}

template <typename T>
T G::StringFieldT<T>::operator()() const noexcept(deref_operator_noexcept)
{
	if( m_fpos == std::string::npos ) return {} ;
	return StringFieldImp::substr<T>( m_s , m_fpos , size() ) ;
}

template <typename T>
G::StringFieldT<T> & G::StringFieldT<T>::operator++() noexcept
{
	m_fpos = m_fendpos ;
	if( m_fpos != std::string::npos )
	{
		m_fpos = std::min( m_s.size() , m_fpos + m_sepn ) ;
		m_fendpos = m_s.find( m_sep , m_fpos , m_sepn ) ; // documented as non-throwing
	}
	return *this ;
}

template <typename T>
bool G::StringFieldT<T>::first() const noexcept
{
	return m_fpos == 0U ;
}

template <typename T>
bool G::StringFieldT<T>::last() const noexcept
{
	return m_fendpos == std::string::npos ;
}

template <typename T>
std::size_t G::StringFieldT<T>::count() const noexcept
{
	std::size_t n = 0U ;
	for( StringFieldT<T> f( m_s , m_sep , m_sepn ) ; f ; ++f )
		n++ ;
	return n ;
}

// --

template <typename T>
G::StringFieldIteratorT<T>::StringFieldIteratorT( StringFieldT<T> & f_in ) noexcept :
	f(&f_in)
{
}

template <typename T>
G::StringFieldIteratorT<T>::StringFieldIteratorT( std::nullptr_t ) noexcept :
	f(nullptr)
{
}

template <typename T>
G::StringFieldIteratorT<T> & G::StringFieldIteratorT<T>::operator++() noexcept
{
	if( f ) ++(*f) ;
	return *this ;
}

template <typename T>
T G::StringFieldIteratorT<T>::operator*() const noexcept(StringFieldT<T>::deref_operator_noexcept)
{
	return (*f)() ;
}

template <typename T>
bool G::StringFieldIteratorT<T>::operator==( StringFieldIteratorT other ) const noexcept
{
	return (f&&f->valid()?f->data():nullptr) == (other.f&&other.f->valid()?other.f->data():nullptr) ;
}

template <typename T>
bool G::StringFieldIteratorT<T>::operator!=( StringFieldIteratorT other ) const noexcept
{
	return !(*this == other) ;
}

namespace G
{
	template <typename T> StringFieldIteratorT<T> begin( StringFieldT<T> & f ) noexcept
	{
		return StringFieldIteratorT<T>( f ) ;
	}
}

namespace G
{
	template <typename T> StringFieldIteratorT<T> end( StringFieldT<T> & ) noexcept
	{
		return StringFieldIteratorT<T>( nullptr ) ;
	}
}

#endif
