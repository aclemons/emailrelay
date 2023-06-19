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
/// \file goptions.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "ggettext.h"
#include "goptions.h"
#include <algorithm>

G::Options::Options()
= default;

G::Options::Options( const std::string & spec , char sep_major , char sep_minor , char escape )
{
	parseSpec( spec , sep_major , sep_minor , escape ) ;
}

void G::Options::parseSpec( const std::string & spec , char sep_major , char sep_minor , char escape )
{
	// split into separate options
	StringArray spec_items ;
	spec_items.reserve( 40U ) ;
	Str::splitIntoFields( spec , spec_items , sep_major , escape , false ) ;

	// for each option
	for( auto & spec_item : spec_items )
	{
		// split into separate fields
		if( spec_item.empty() ) continue ;
		StringArray inner_parts ;
		inner_parts.reserve( 10U ) ;
		Str::splitIntoFields( spec_item , inner_parts , sep_minor , escape ) ;

		if( inner_parts.size() < 7U )
			throw InvalidSpecification( std::string(1U,'[').append(G::Str::join(",",inner_parts)).append(1U,']') ) ;

		std::string short_form = inner_parts[0] ;
		char c = short_form.empty() ? '\0' : short_form[0U] ;
		unsigned int level = Str::toUInt( inner_parts[6U] ) ;

		Option::Multiplicity multiplicity = Option::decode( inner_parts[4U] ) ;
		if( multiplicity == Option::Multiplicity::error )
			throw InvalidSpecification( std::string(1U,'[').append(G::Str::join(",",inner_parts)).append(1U,']') ) ;

		Option opt( c , inner_parts[1U] , inner_parts[2U] , inner_parts[3U] ,
			multiplicity , inner_parts[5U] , level ) ;

		addOption( opt , sep_minor , escape ) ;
	}
}

void G::Options::add( Options & options , char c , const char * name , const char * text ,
	const char * more , Option::Multiplicity m , const char * argname ,
	unsigned int level , unsigned int main_tag , unsigned int tag_bits )
{
	options.add( Option(c,name,G::gettext(text),more,m,argname,level,main_tag,main_tag|tag_bits) ) ;
}

void G::Options::add( const Option & opt , char sep , char escape )
{
	addOption( opt , sep , escape ) ;
}

void G::Options::addOption( Option opt , char sep , char escape )
{
	if( sep )
	{
		// if the description is in two parts separated by 'sep' and the
		// extra-description is empty then take the first half as the
		// description and the second part as the extra-description -- this
		// allows the description to be translatable as a single string
		//
		StringArray sub_parts ;
		sub_parts.reserve( 2U ) ;
		Str::splitIntoFields( opt.description , sub_parts , sep , escape ) ;
		if( sub_parts.size() > 2U || ( sub_parts.size()==2U && !opt.description_extra.empty() ) )
			throw InvalidSpecification() ;
		if( sub_parts.size() == 2U )
		{
			opt.description = sub_parts[0] ;
			opt.description_extra = sub_parts[1] ;
		}
	}

	auto range = std::equal_range( m_list.begin() , m_list.end() , opt ,
		[](const Option & a, const Option & b){ return a.name < b.name ; } ) ;

	if( range.first != range.second )
		throw InvalidSpecification( "duplication" ) ;

	m_list.insert( range.first , opt ) ;
}

bool G::Options::defaulting( char c ) const
{
	return defaulting( lookup(c) ) ;
}

bool G::Options::defaulting( const std::string & name ) const
{
	const Option * p = find( name ) ;
	return p ? p->defaulting() : false ;
}

bool G::Options::valued( char c ) const
{
	return valued( lookup(c) ) ;
}

bool G::Options::valued( const std::string & name ) const
{
	const Option * p = find( name ) ;
	return p ? p->valued() : false ;
}

bool G::Options::unvalued( const std::string & name ) const
{
	return valid(name) && !valued(name) ;
}

bool G::Options::multivalued( char c ) const
{
	return multivalued( lookup(c) ) ;
}

bool G::Options::multivalued( const std::string & name ) const
{
	const Option * p = find( name ) ;
	return p ? p->multivalued() : false ;
}

bool G::Options::visible( const std::string & name , unsigned int level , bool level_exact ) const
{
	const Option * p = find( name ) ;
	return p ? p->visible({level_exact?level:1U,level}) : false ;
}

#ifndef G_LIB_SMALL
bool G::Options::visible( const std::string & name ) const
{
	return visible( name , 99U , false ) ;
}
#endif

bool G::Options::valid( const std::string & name ) const
{
	return find(name) != nullptr ;
}

std::string G::Options::lookup( char c ) const
{
	for( auto p = m_list.begin() ; c != '\0' && p != m_list.end() ; ++p )
	{
		if( (*p).c == c )
			return (*p).name ;
	}
	return {} ;
}

const G::Option * G::Options::find( const std::string & name ) const
{
	for( auto & option : m_list )
	{
		if( option.name == name )
			return &option ;
	}
	return nullptr ;
}

const std::vector<G::Option> & G::Options::list() const
{
	return m_list ;
}

