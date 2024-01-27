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
/// \file goptionvalue.h
///

#ifndef G_OPTION_VALUE_H
#define G_OPTION_VALUE_H

#include "gdef.h"
#include "gstr.h"
#include <string>

namespace G
{
	class OptionValue ;
}

//| \class G::OptionValue
/// A simple structure encapsulating the value of a command-line option.
/// Unvalued options (eg. "--debug") can be be explicitly on (eg. "--debug=yes")
/// or off ("--debug=no"); the latter are typically ignored.
///
class G::OptionValue
{
public:
	OptionValue() ;
		///< Default constructor for a valueless value.

	explicit OptionValue( const std::string & s , std::size_t count = 1U ) ;
		///< Constructor for a valued value.
		///< Precondition: !s.empty()

	static OptionValue on() ;
		///< A factory function for an unvalued option-enabled option.

	static OptionValue off() ;
		///< A factory function for an unvalued option-disabled option.

	bool isOn() const noexcept ;
		///< Returns true if on().

	bool isOff() const noexcept ;
		///< Returns true if off().

	std::string value() const ;
		///< Returns the value as a string.

	bool numeric() const noexcept ;
		///< Returns true if value() is an unsigned integer.

	unsigned int number( unsigned int default_ = 0U ) const ;
		///< Returns value() as an unsigned integer.
		///< Returns the default if not numeric().

	size_t count() const noexcept ;
		///< Returns an instance count that is one by default.

	void increment() noexcept ;
		///< Increments the instance count().

private:
	bool m_on_off{false} ;
	std::size_t m_count{1U} ;
	std::string m_value ;
} ;

inline
G::OptionValue::OptionValue() :
	m_on_off(true) ,
	m_value(G::Str::positive())
{
}

inline
G::OptionValue::OptionValue( const std::string & s , std::size_t count ) :
	m_count(count) ,
	m_value(s)
{
}

inline
G::OptionValue G::OptionValue::on()
{
	return {} ;
}

inline
G::OptionValue G::OptionValue::off()
{
	OptionValue v ;
	v.m_value = G::Str::negative() ;
	return v ;
}

inline
bool G::OptionValue::isOn() const noexcept
{
	return m_on_off && G::Str::isPositive(m_value) ;
}

inline
bool G::OptionValue::isOff() const noexcept
{
	return m_on_off && G::Str::isNegative(m_value) ;
}

inline
std::string G::OptionValue::value() const
{
	return m_value ;
}

inline
bool G::OptionValue::numeric() const noexcept
{
	return !m_on_off && G::Str::isUInt(m_value) ;
}

inline
unsigned int G::OptionValue::number( unsigned int default_ ) const
{
	return numeric() ? G::Str::toUInt(m_value) : default_ ;
}

inline
std::size_t G::OptionValue::count() const noexcept
{
	return m_count ;
}

inline
void G::OptionValue::increment() noexcept
{
	m_count++ ;
}

#endif
