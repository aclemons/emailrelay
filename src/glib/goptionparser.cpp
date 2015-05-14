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
//
// goptionparser.cpp
//	

#include "gdef.h"
#include "goptionparser.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <map>
#include <stdexcept>

G::OptionParser::OptionParser( const Options & spec , OptionValueMap & values_out , StringArray & errors_out ) :
	m_spec(spec) ,
	m_map(values_out) ,
	m_errors(errors_out)
{
}

G::MapFile G::OptionParser::parse( const std::string & spec , const StringArray & args , size_t start , bool no_throw )
{
	typedef std::map<std::string,OptionValue> OptionValueMap ;
	Options options( spec ) ;
	OptionValueMap option_value_map ;
	StringArray parse_errors ;
	OptionParser parser( options , option_value_map , parse_errors ) ;
	parser.parse( args , start ) ;
	if( !parse_errors.empty() )
	{
		if( no_throw )
			G_WARNING( "G::OptionParser::parse: " << parse_errors.at(0U) ) ;
		else
			throw std::runtime_error( parse_errors.at(0U) ) ;
	}
	return MapFile( option_value_map , G::Str::positive() ) ;
}

size_t G::OptionParser::parse( const StringMap & map )
{
	StringArray list ;
	for( StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
	{
		std::string key = (*p).first ;
		std::string value = (*p).second ;
		if( m_spec.valued(key) )
		{
			list.push_back( "--" + key ) ;
			list.push_back( value ) ;
		}
		else
		{
			list.push_back( "--" + key + "=" + value ) ;
		}
	}
	return parse( list ) ;
}

size_t G::OptionParser::parse( const StringArray & args_in , size_t start )
{
	size_t i = start ;
	for( ; i < args_in.size() ; i++ )
	{
		const std::string & arg = args_in.at(i) ;

		if( arg == "--" ) // special "end-of-switches" switch
		{
			i++ ;
			break ;
		}

		if( isAOptionSet(arg) ) // eg. "-ltv"
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
				processOption( key , eqValue(name,pos_eq) ) ; 
			else if( m_spec.valued(name) && (i+1U) >= args_in.size() ) // "foo bar"
				errorNoValue( name ) ;
			else if( m_spec.valued(name) )
				processOption( name , args_in.at(++i) ) ;
			else
				processOptionOn( name ) ;
		}
		else
		{
			break ;
		}
	}
	i-- ;
	return i ;
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

bool G::OptionParser::isAOptionSet( const std::string & arg )
{
	return isOldOption(arg) && arg.length() > 2U ;
}

void G::OptionParser::errorOptionLikeValue( const std::string & name , const std::string & value )
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

void G::OptionParser::errorExtraValue( char c )
{
	m_errors.push_back( std::string("cannot give a value with \"-") + std::string(1U,c) + "\"" ) ;
}

void G::OptionParser::errorExtraValue( const std::string & name )
{
	m_errors.push_back( std::string("cannot give a value with \"--") + name + "\"" ) ;
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

std::string G::OptionParser::join( const OptionValueMap & map , const std::string & key , const std::string & new_value )
{
	OptionValueMap::const_iterator p = map.find( key ) ;
	std::string old_value ;
	if( p != map.end() )
		old_value = (*p).second.value() ;
	return join( old_value , new_value ) ;
}

std::string G::OptionParser::join( const std::string & s1 , const std::string & s2 )
{
	if( s1.empty() )
		return s2 ;
	else if( s2.empty() )
		return s1 ;
	else
		return s1 + "," + s2 ;
}

bool G::OptionParser::contains( const std::string & name ) const
{
	OptionValueMap::const_iterator p = m_map.find( name ) ;
	return p != m_map.end() && !(*p).second.is_off() ;
}

void G::OptionParser::processOptionOff( const std::string & name )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( m_spec.valued(name) )
		errorNoValue( name ) ;
	else if( contains(name) )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::off()) ) ;
}

void G::OptionParser::processOptionOn( const std::string & name )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( m_spec.valued(name) )
		errorNoValue( name ) ;
	else if( m_map.find(name) != m_map.end() && (*m_map.find(name)).second.is_off() )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::on()) ) ;
}

void G::OptionParser::processOption( const std::string & name , const std::string & value )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( !value.empty() && value[0] == '-' )
		errorOptionLikeValue( name , value ) ;
	else if( !m_spec.valued(name) )
		errorExtraValue( name ) ;
	else if( haveSeen(name) && !m_spec.multivalued(name) )
		errorDuplicate( name ) ;
	else if( m_spec.multivalued(name) )
		m_map[name] = OptionValue(join(m_map,name,value)) ;
	else
		m_map.insert( OptionValueMap::value_type(name,OptionValue(value)) ) ;
}

void G::OptionParser::processOptionOn( char c )
{
	std::string name = m_spec.lookup( c ) ;
	if( !m_spec.valid(name) )
		errorUnknownOption( c ) ;
	else if( m_spec.valued(name) )
		errorNoValue( c ) ;
	else if( m_map.find(name) != m_map.end() && (*m_map.find(name)).second.is_off() )
		errorConflict( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::on()) ) ;
}

void G::OptionParser::processOption( char c , const std::string & value )
{
	std::string name = m_spec.lookup( c ) ;
	if( !m_spec.valid(name) )
		errorUnknownOption( c ) ;
	else if( !m_spec.valued(name) )
		errorExtraValue( name ) ;
	else if( haveSeen(name) && !m_spec.multivalued(c) )
		errorDuplicate( c ) ;
	else if( m_spec.multivalued(c) )
		m_map[name] = OptionValue(join(m_map,name,value)) ;
	else
		m_map.insert( OptionValueMap::value_type(name,OptionValue(value)) ) ;
}

bool G::OptionParser::haveSeen( const std::string & name ) const
{
	return m_map.find(name) != m_map.end() ;
}

/// \file goptionparser.cpp
