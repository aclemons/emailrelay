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
/// \file goption.h
///

#ifndef G_OPTION_H
#define G_OPTION_H

#include "gdef.h"
#include "gstringarray.h"
#include <string>

namespace G
{
	struct Option ;
}

//| \class G::Option
/// A structure representing a G::Options command-line option.
///
struct G::Option
{
	enum class Multiplicity { zero , zero_or_one , one , many , error } ;
	char c ;
	std::string name ;
	std::string description ;
	std::string description_extra ;
	Multiplicity value_multiplicity ;
	bool hidden ;
	std::string value_description ;
	unsigned int level ;
	StringArray tags ;

	Option( char c , const std::string & name , const std::string & description ,
		const std::string & description_extra , Multiplicity value_multiplicity ,
		const std::string & vd , unsigned int level , const StringArray & tags ) ;
			///< Constructor taking strings.

	Option( char c , const char * name , const char * description ,
		const char * description_extra , Multiplicity value_multiplicity ,
		const char * vd , unsigned int level , unsigned int tags = 0U ) ;
			///< Constructor taking c-strings and tags as a bit-mask.

	static Multiplicity decode( const std::string & ) ;
		///< Decodes a multiplicity string into its enumeration.
		///< Returns 'error' on error.

	bool valued() const ;
	bool defaulting() const ;
	bool multivalued() const ;
	bool visible( unsigned int level , bool level_exact ) const ;
	bool visible() const ;
} ;

inline bool G::Option::valued() const { return value_multiplicity != Multiplicity::zero ; }
inline bool G::Option::defaulting() const { return value_multiplicity == Option::Multiplicity::zero_or_one ; }
inline bool G::Option::multivalued() const { return value_multiplicity == Option::Multiplicity::many ; }
inline bool G::Option::visible() const { return visible( 99U , false ) ; }
inline bool G::Option::visible( unsigned int level_in , bool level_exact ) const
{
	return level_exact ? ( !hidden && level == level_in ) : ( !hidden && level <= level_in ) ;
}

#endif
