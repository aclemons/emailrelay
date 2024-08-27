//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file goption.cpp
///

#include "gdef.h"
#include "goption.h"

G::Option::Option( char c_in , const std::string & name_in , const std::string & description_in ,
	const std::string & description_extra_in , Multiplicity value_multiplicity_in ,
	const std::string & vd_in , unsigned int level_in ) :
		c(c_in) ,
		name(name_in) ,
		description(description_in) ,
		description_extra(description_extra_in) ,
		value_multiplicity(value_multiplicity_in) ,
		hidden(description_in.empty()||level_in==0U) ,
		value_description(vd_in) ,
		level(level_in) ,
		main_tag(0U) ,
		tag_bits(0U)
{
}

G::Option::Option( char c_in , const char * name_in , const char * description_in ,
	const char * description_extra_in , Multiplicity value_multiplicity_in ,
	const char * vd_in , unsigned int level_in , unsigned int main_tag_in ,
	unsigned int tag_bits_in ) :
		c(c_in) ,
		name(name_in) ,
		description(description_in) ,
		description_extra(description_extra_in) ,
		value_multiplicity(value_multiplicity_in) ,
		hidden(*description_in=='\0'||level_in==0U) ,
		value_description(vd_in) ,
		level(level_in) ,
		main_tag(main_tag_in) ,
		tag_bits(main_tag_in|tag_bits_in)
{
}

G::Option::Multiplicity G::Option::decode( const std::string & s )
{
	if( s == "0" )
		return Multiplicity::zero ;
	else if( s == "01" )
		return Multiplicity::zero_or_one ;
	else if( s == "1" )
		return Multiplicity::one ;
	else if( s == "2" )
		return Multiplicity::many ;
	else
		return Multiplicity::error ;
}

