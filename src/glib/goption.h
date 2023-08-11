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
/// \file goption.h
///

#ifndef G_OPTION_H
#define G_OPTION_H

#include "gdef.h"
#include "gstringarray.h"
#include <string>
#include <utility>

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
	unsigned int main_tag ; // principal category
	unsigned int tag_bits ; // all categories

	Option( char c , const std::string & name , const std::string & description ,
		const std::string & description_extra , Multiplicity value_multiplicity ,
		const std::string & vd , unsigned int level ) ;
			///< Constructor taking strings.

	Option( char c , const char * name , const char * description ,
		const char * description_extra , Multiplicity value_multiplicity ,
		const char * vd , unsigned int level ,
		unsigned int main_tag , unsigned int tag_bits ) ;
			///< Constructor taking c-strings and tags.

	static Multiplicity decode( const std::string & ) ;
		///< Decodes a multiplicity string into its enumeration.
		///< Returns 'error' on error.

	bool valued() const ;
	bool defaulting() const ;
	bool multivalued() const ;
	bool visible() const ;
	bool visible( std::pair<unsigned,unsigned> level_range , unsigned int main_tag = 0U , unsigned int tag_bits = 0U ) const ;
} ;

inline bool G::Option::valued() const { return value_multiplicity != Multiplicity::zero ; }
inline bool G::Option::defaulting() const { return value_multiplicity == Option::Multiplicity::zero_or_one ; }
inline bool G::Option::multivalued() const { return value_multiplicity == Option::Multiplicity::many ; }
inline bool G::Option::visible() const { return visible( {1U,99U} ) ; }
inline bool G::Option::visible( std::pair<unsigned,unsigned> level_range , unsigned int main_tag_in , unsigned int tag_bits_in ) const
{
	return
		!hidden &&
		level >= level_range.first &&
		level <= level_range.second &&
		( main_tag_in == 0U || main_tag_in == main_tag ) &&
		( tag_bits_in == 0U || ( tag_bits_in & tag_bits ) != 0U ) ;
}

#endif
