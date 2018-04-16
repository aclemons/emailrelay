//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// goptionparser.cpp
//

#include "gdef.h"
#include "goptionparser.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <algorithm>
#include <map>
#include <stdexcept>

G::OptionParser::OptionParser( const Options & spec , OptionMap & values_out , StringArray & errors_out ) :
	m_spec(spec) ,
	m_map(values_out) ,
	m_errors(errors_out)
{
}

G::OptionParser::OptionParser( const Options & spec , OptionMap & values_out ) :
	m_spec(spec) ,
	m_map(values_out) ,
	m_errors(m_errors_ignored)
{
}

size_t G::OptionParser::parse( const StringArray & args_in , size_t start )
{
	size_t i = start ;
	for( ; i < args_in.size() ; i++ )
	{
		const std::string & arg = args_in.at(i) ;

		if( arg == "--" ) // end-of-options marker
		{
			i++ ;
			break ;
		}

		if( isAnOptionSet(arg) ) // eg. "-ltv"
		{
			for( size_t n = 1U ; n < arg.length() ; n++ )
				processOptionOn( arg.at(n) ) ;
		}
		else if( isOldOption(arg) ) // eg. "-v"
		{
			char c = arg.at(1U) ;
			if( m_spec.valued(c) && (i+1U) >= args_in.size() )
				errorNoValue( c ) ;
			else if( m_spec.valued(c) )
				processOption( c , args_in.at(++i) ) ;
			else
				processOptionOn( c ) ;
		}
		else if( isNewOption(arg) ) // eg. "--foo"
		{
			std::string name = arg.substr( 2U ) ; // eg. "foo" or "foo=..."
			std::string::size_type pos_eq = eqPos(name) ;
			bool has_eq = pos_eq != std::string::npos ;
			std::string key = has_eq ? name.substr(0U,pos_eq) : name ;
			if( has_eq && m_spec.unvalued(key) && G::Str::isPositive(eqValue(name,pos_eq)) ) // "foo=yes"
				processOptionOn( key ) ;
			else if( has_eq && m_spec.unvalued(key) && G::Str::isNegative(eqValue(name,pos_eq)) ) // "foo=no"
				processOptionOff( key ) ;
			else if( has_eq ) // "foo=bar"
				processOption( key , eqValue(name,pos_eq) , false ) ;
			else if( m_spec.valued(name) && (i+1U) >= args_in.size() ) // "foo bar"
				errorNoValue( name ) ;
			else if( m_spec.valued(name) )
				processOption( name , args_in.at(++i) , true ) ;
			else
				processOptionOn( name ) ;
		}
		else
		{
			break ;
		}
	}
	return i ;
}

void G::OptionParser::processOptionOn( const std::string & name )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( m_spec.valued(name) )
		errorNoValue( name ) ;
	else if( haveSeenOff(name) )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::on()) ) ;
}

void G::OptionParser::processOptionOff( const std::string & name )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( m_spec.valued(name) )
		errorNoValue( name ) ;
	else if( haveSeenOn(name) )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::off()) ) ;
}

void G::OptionParser::processOption( const std::string & name , const std::string & value , bool fail_if_dubious_value )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( !value.empty() && value[0] == '-' && fail_if_dubious_value )
		errorDubiousValue( name , value ) ;
	else if( !m_spec.valued(name) && !value.empty() )
		errorExtraValue( name , value ) ;
	else if( haveSeen(name) && !m_spec.multivalued(name) )
		errorDuplicate( name ) ;
	else
		m_map.insert( OptionMap::value_type(name,OptionValue(value)) ) ;
}

void G::OptionParser::processOptionOn( char c )
{
	std::string name = m_spec.lookup( c ) ;
	if( !m_spec.valid(name) )
		errorUnknownOption( c ) ;
	else if( m_spec.valued(name) )
		errorNoValue( c ) ;
	else if( haveSeenOff(name) )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::on()) ) ;
}

void G::OptionParser::processOption( char c , const std::string & value )
{
	std::string name = m_spec.lookup( c ) ;
	if( !m_spec.valid(name) )
		errorUnknownOption( c ) ;
	else if( !m_spec.valued(name) && !value.empty() )
		errorExtraValue( name , value ) ;
	else if( haveSeen(name) && !m_spec.multivalued(c) )
		errorDuplicate( c ) ;
	else
		m_map.insert( OptionMap::value_type(name,OptionValue(value)) ) ;
}

std::string::size_type G::OptionParser::eqPos( const std::string & s )
{
	std::string::size_type p = s.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_") ;
	return p != std::string::npos && s.at(p) == '=' ? p : std::string::npos ;
}

std::string G::OptionParser::eqValue( const std::string & s , std::string::size_type pos )
{
	return (pos+1U) == s.length() ? std::string() : s.substr(pos+1U) ;
}

bool G::OptionParser::isOldOption( const std::string & arg )
{
	return
		( arg.length() > 1U && arg.at(0U) == '-' ) &&
		! isNewOption( arg ) ;
}

bool G::OptionParser::isNewOption( const std::string & arg )
{
	return arg.length() > 2U && arg.at(0U) == '-' && arg.at(1U) == '-' ;
}

bool G::OptionParser::isAnOptionSet( const std::string & arg )
{
	return isOldOption(arg) && arg.length() > 2U ;
}

void G::OptionParser::errorDubiousValue( const std::string & name , const std::string & value )
{
	m_errors.push_back( std::string("use of \"--")+name+" "+value+"\" is probably a mistake, or try \"--"+name+"="+value+"\" instead" ) ;
}

void G::OptionParser::errorDuplicate( char c )
{
	m_errors.push_back( std::string("duplicate use of \"-") + std::string(1U,c) + "\"" ) ;
}

void G::OptionParser::errorDuplicate( const std::string & name )
{
	m_errors.push_back( std::string("duplicate use of \"--") + name + "\"" ) ;
}

void G::OptionParser::errorExtraValue( char c , const std::string & )
{
	m_errors.push_back( std::string("cannot give a value with \"-") + std::string(1U,c) + "\"" ) ;
}

void G::OptionParser::errorExtraValue( const std::string & name , const std::string & value )
{
	m_errors.push_back( std::string("cannot give a value with \"--") + name + "\" (" + value + ")" ) ;
}

void G::OptionParser::errorNoValue( char c )
{
	m_errors.push_back( std::string("no value supplied for -") + std::string(1U,c) ) ;
}

void G::OptionParser::errorNoValue( const std::string & name )
{
	m_errors.push_back( std::string("no value supplied for \"--") + name + "\"" ) ;
}

void G::OptionParser::errorUnknownOption( char c )
{
	m_errors.push_back( std::string("invalid option: \"-") + std::string(1U,c) + "\"" ) ;
}

void G::OptionParser::errorUnknownOption( const std::string & name )
{
	m_errors.push_back( std::string("invalid option: \"--") + name + "\"" ) ;
}

void G::OptionParser::errorConflict( const std::string & name )
{
	m_errors.push_back( std::string("conflicting values: \"--") + name + "\"" ) ;
}

bool G::OptionParser::haveSeenOn( const std::string & name ) const
{
	OptionMap::const_iterator p = m_map.find( name ) ;
	return p != m_map.end() && !(*p).second.is_off() ;
}

bool G::OptionParser::haveSeenOff( const std::string & name ) const
{
	OptionMap::const_iterator p = m_map.find( name ) ;
	return p != m_map.end() && (*p).second.is_off() ;
}

bool G::OptionParser::haveSeen( const std::string & name ) const
{
	return m_map.find(name) != m_map.end() ;
}

/// \file goptionparser.cpp
