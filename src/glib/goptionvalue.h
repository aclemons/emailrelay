//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_OPTION_VALUE_H__
#define G_OPTION_VALUE_H__

#include "gdef.h"
#include "gstr.h"
#include <string>

/// \namespace G
namespace G
{
	class OptionValue ;
}

/// \class G::OptionValue
/// A simple structure encapsulating the value of a command-line option.
/// Unvalued options (eg. "--debug") can be be explicitly on (eg. "--debug=yes") or 
/// off ("--debug=no"); the latter are typically ignored.
///
class G::OptionValue
{
public:

	OptionValue() ;
		///< Default constructor for a valueless value.

	explicit OptionValue( std::string s ) ;
		///< Default constructor for a valued value.

	static OptionValue on() ;
		///< A factory function for an unvalued option-enabled option.

	static OptionValue off() ;
		///< A factory function for an unvalued option-disabled option.

	bool is_on() const ;
		///< Returns true if an unvalued enabled option.

	bool is_off() const ;
		///< Returns true if an unvalued disabled option.

	bool valued() const ;
		///< Returns true if valued.

	std::string value() const ;
		///< Returns the value as a string.

private:
	bool first ; // ie. valued
	std::string second ;
} ;

inline
G::OptionValue::OptionValue() :
	first(false) ,
	second(G::Str::positive())
{
}

inline
G::OptionValue::OptionValue( std::string s ) :
	first(true) ,
	second(s)
{
}

inline
G::OptionValue G::OptionValue::on()
{
	OptionValue v ;
	v.second = G::Str::positive() ;
	return v ;
}

inline
G::OptionValue G::OptionValue::off()
{
	OptionValue v ;
	v.second = G::Str::negative() ;
	return v ;
}

inline
bool G::OptionValue::is_on() const
{
	return !first && G::Str::isPositive(second) ;
}

inline
bool G::OptionValue::is_off() const
{
	return !first && G::Str::isNegative(second) ;
}

inline
bool G::OptionValue::valued() const
{
	return first ;
}

inline
std::string G::OptionValue::value() const
{
	return second ;
}

#endif
