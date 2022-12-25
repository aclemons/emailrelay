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
/// \file goptional.h
///

#ifndef G_OPTIONAL_H
#define G_OPTIONAL_H

#include "gdef.h"
#include <utility>
#include <stdexcept>

namespace G
{
	template <typename T> class optional ;
}

//| \class G::optional
/// A class template like a simplified c++17 std::optional.
///
template <typename T>
class G::optional
{
public:
	optional() noexcept(noexcept(T())) ;
		///< Default constructor for no value.

	explicit optional( const T & ) ;
		///< Constructor for a defined value.

	optional( bool has_value , const T & value ) ;
		///< Constructor. Not in std::optional.

	void clear() ;
		///< Clears the value. Not in std::optional.

	bool has_value() const noexcept ;
		///< Returns true if a defined value.

	explicit operator bool() const noexcept ;
		///< Returns true if a defined value.

	const T & value() const ;
		///< Returns the value.

	T value_or( const T & ) const ;
		///< Returns the value or a default.

	optional<T> & operator=( const T & ) ;
		///< Assignment for a defined value.

public:
	~optional() = default ;
	optional( const optional & ) = default ;
	optional( optional && ) noexcept = default ;
	optional & operator=( const optional & ) = default ;
	optional & operator=( optional && ) noexcept = default ;

private:
	void doThrow() const ;

private:
	T m_value {} ;
	bool m_has_value {false} ;
} ;

template <typename T>
G::optional<T>::optional() noexcept(noexcept(T()))
= default ;

template <typename T>
G::optional<T>::optional( const T & t ) :
	m_value(t) ,
	m_has_value(true)
{
}

template <typename T>
G::optional<T>::optional( bool has_value , const T & value ) :
	m_value(value) ,
	m_has_value(has_value)
{
}

template <typename T>
void G::optional<T>::clear()
{
	m_has_value = false ;
}

template <typename T>
bool G::optional<T>::has_value() const noexcept
{
	return m_has_value ;
}

template <typename T>
G::optional<T>::operator bool() const noexcept
{
	return m_has_value ;
}

template <typename T>
const T & G::optional<T>::value() const
{
	return m_has_value ? m_value : ( doThrow() , m_value ) ;
}

template <typename T>
void G::optional<T>::doThrow() const
{
	throw std::runtime_error( "bad optional access" ) ;
}

template <typename T>
T G::optional<T>::value_or( const T & default_ ) const
{
	return m_has_value ? m_value : default_ ;
}

template <typename T>
G::optional<T> & G::optional<T>::operator=( const T & t )
{
	m_value = t ;
	m_has_value = true ;
	return *this ;
}

#endif
