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

/// \class G::OptionValue
/// A simple structure encapsulating the value of a command-line option.
/// Unvalued options (eg. "--debug") can be be explicitly on (eg. "--debug=yes")
/// or off ("--debug=no"); the latter are typically ignored.
///
class G::OptionValue
{
public:
	OptionValue() ;
		///< Default constructor for a valueless value.

	explicit OptionValue( const std::string & s ) ;
		///< Constructor for a valued value.
		///< Precondition: !s.empty()

	static OptionValue on() ;
		///< A factory function for an unvalued option-enabled option.

	static OptionValue off() ;
		///< A factory function for an unvalued option-disabled option.

	bool isOn() const ;
		///< Returns true if an unvalued enabled option.

	bool isOff() const ;
		///< Returns true if an unvalued disabled option.

	bool valued() const ;
		///< Returns true if constructed with the string overload,
		///< or false if constructed by on() or off() or the default
		///< constructor. Note that a non-valued() value can still
		///< have a non-empty value().

	std::string value() const ;
		///< Returns the value as a string.

	bool numeric() const ;
		///< Returns true if value() is an unsigned integer.

	unsigned int number( unsigned int default_ = 0U ) const ;
		///< Returns value() as an unsigned integer.
		///< Returns the default if not numeric().

private:
	bool m_valued ; // ie. valued
	std::string m_value ;
} ;

inline
G::OptionValue::OptionValue() :
	m_valued(false) ,
	m_value(G::Str::positive())
{
}

inline
G::OptionValue::OptionValue( const std::string & s ) :
	m_valued(true) ,
	m_value(s)
{
}

inline
G::OptionValue G::OptionValue::on()
{
	OptionValue v ;
	v.m_value = G::Str::positive() ;
	return v ;
}

inline
G::OptionValue G::OptionValue::off()
{
	OptionValue v ;
	v.m_value = G::Str::negative() ;
	return v ;
}

inline
bool G::OptionValue::isOn() const
{
	return !m_valued && G::Str::isPositive(m_value) ;
}

inline
bool G::OptionValue::isOff() const
{
	return !m_valued && G::Str::isNegative(m_value) ;
}

inline
bool G::OptionValue::valued() const
{
	return m_valued ;
}

inline
std::string G::OptionValue::value() const
{
	return m_value ;
}

inline
bool G::OptionValue::numeric() const
{
	return valued() && G::Str::isUInt(m_value) ;
}

inline
unsigned int G::OptionValue::number( unsigned int default_ ) const
{
	return numeric() ? G::Str::toUInt(m_value) : default_ ;
}

#endif
