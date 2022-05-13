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
/// \file goptionparser.cpp
///

#include "gdef.h"
#include "goptionparser.h"
#include "gformat.h"
#include "ggettext.h"
#include "gstr.h"
#include "glog.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

G::OptionParser::OptionParser( const Options & spec , OptionMap & values_out , StringArray & errors_out ) :
	m_spec(spec) ,
	m_map(values_out) ,
	m_errors(&errors_out)
{
}

G::OptionParser::OptionParser( const Options & spec , OptionMap & values_out , StringArray * errors_out ) :
	m_spec(spec) ,
	m_map(values_out) ,
	m_errors(errors_out)
{
}

G::StringArray G::OptionParser::parse( const StringArray & args_in , const Options & spec ,
	OptionMap & map_out , StringArray * errors_out , std::size_t start_position ,
	std::size_t ignore_non_options )
{
	OptionParser parser( spec , map_out , errors_out ) ;
	return parser.parse( args_in , start_position , ignore_non_options ) ;
}

G::StringArray G::OptionParser::parse( const StringArray & args_in , std::size_t start ,
	std::size_t ignore_non_options )
{
	StringArray args_out ;
	std::size_t i = start ;
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
			for( std::size_t n = 1U ; n < arg.length() ; n++ )
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
			std::string name = arg.substr( 2U ) ; // eg. "--foo" or "--foo=..."
			std::string::size_type pos_eq = eqPos( name ) ;
			bool has_eq = pos_eq != std::string::npos ;
			std::string key = has_eq ? name.substr(0U,pos_eq) : name ;
			if( has_eq && m_spec.unvalued(key) && Str::isPositive(eqValue(name,pos_eq)) ) // "foo=yes"
				processOptionOn( key ) ;
			else if( has_eq && m_spec.unvalued(key) && Str::isNegative(eqValue(name,pos_eq)) ) // "foo=no"
				processOptionOff( key ) ;
			else if( has_eq ) // "foo=bar"
				processOption( key , eqValue(name,pos_eq) , false ) ;
			else if( m_spec.defaulting(name) )
				processOption( key , std::string() , false ) ;
			else if( m_spec.valued(name) && (i+1U) >= args_in.size() )
				errorNoValue( name ) ;
			else if( m_spec.valued(name) )
				processOption( name , args_in.at(++i) , true ) ;
			else
				processOptionOn( name ) ;
		}
		else if( ignore_non_options != 0U )
		{
			--ignore_non_options ;
			args_out.push_back( arg ) ;
		}
		else
		{
			break ;
		}
	}
	for( ; i < args_in.size() ; i++ )
		args_out.push_back( args_in.at(i) ) ;
	return args_out ;
}

void G::OptionParser::processOptionOn( const std::string & name )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( m_spec.valued(name) )
		errorNoValue( name ) ;
	else if( haveSeenOff(name) )
		errorConflict( name ) ;
	else if( haveSeenOn(name) )
		m_map.increment( name ) ;
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
	else if( haveSeenOff(name) )
		m_map.increment( name ) ;
	else
		m_map.insert( std::make_pair(name,OptionValue::off()) ) ;
}

void G::OptionParser::processOption( const std::string & name , const std::string & value ,
	bool fail_if_dubious_value )
{
	if( !m_spec.valid(name) )
		errorUnknownOption( name ) ;
	else if( !value.empty() && value[0] == '-' && fail_if_dubious_value )
		errorDubiousValue( name , value ) ;
	else if( !m_spec.valued(name) && !value.empty() )
		errorExtraValue( name , value ) ;
	else if( m_spec.multivalued(name) )
		m_map.insert( OptionMap::value_type(name,OptionValue(value,valueCount(value))) ) ;
	else if( haveSeen(name) && !haveSeenSame(name,value) )
		errorDuplicate( name ) ;
	else if( haveSeen(name) )
		m_map.increment( name ) ;
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
	else if( haveSeenOn(name) )
		m_map.increment( name ) ;
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
	else if( m_spec.multivalued(c) )
		m_map.insert( OptionMap::value_type(name,OptionValue(value,valueCount(value))) ) ;
	else if( haveSeen(name) && !haveSeenSame(name,value) )
		errorDuplicate( c ) ;
	else if( haveSeen(name) )
		m_map.increment( name ) ;
	else
		m_map.insert( OptionMap::value_type(name,OptionValue(value)) ) ;
}

std::string::size_type G::OptionParser::eqPos( const std::string & s )
{
	std::string::size_type p = s.find_first_not_of( "abcdefghijklmnopqrstuvwxyz0123456789-_" ) ;
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
	std::string s = str(
		format(txt("%1% is probably a mistake, use %2% or %3%")) %
			("\"--"+name+" "+value+"\"") %
			("\"--"+name+"=... "+value+"\"") %
			("\"--"+name+"="+value+"\"") ) ;
	error( s ) ;
}

void G::OptionParser::errorDuplicate( char c )
{
	error( str( format(txt("duplicate use of %1%")) % ("\"-"+std::string(1U,c)+"\"") ) ) ;
}

void G::OptionParser::errorDuplicate( const std::string & name )
{
	error( str( format(txt("duplicate use of %1%")) % ("\"--"+name+"\"") ) ) ;
}

void G::OptionParser::errorExtraValue( char c , const std::string & )
{
	error( str( format(txt("cannot give a value with %1%")) % ("\"-"+std::string(1U,c)+"\"") ) ) ;
}

void G::OptionParser::errorExtraValue( const std::string & name , const std::string & value )
{
	error( str( format(txt("cannot give a value with %1% (%2%)")) % ("\"--"+name+"\"") % value ) ) ;
}

void G::OptionParser::errorNoValue( char c )
{
	error( str( format(txt("no value supplied for %1%")) % ("-"+std::string(1U,c)) ) ) ;
}

void G::OptionParser::errorNoValue( const std::string & name )
{
	error( str( format(txt("no value supplied for %1%")) % ("\"--"+name+"\"") ) ) ;
}

void G::OptionParser::errorUnknownOption( char c )
{
	error( str( format(txt("invalid option: %1%")) % ("\"-"+std::string(1U,c)+"\"") ) ) ;
}

void G::OptionParser::errorUnknownOption( const std::string & name )
{
	error( str( format(txt("invalid option: %1%")) % ("\"--"+name+"\"") ) ) ;
}

void G::OptionParser::errorConflict( const std::string & name )
{
	error( str( format(txt("conflicting values: %1%")) % ("\"--"+name+"\"") ) ) ;
}

void G::OptionParser::error( const std::string & s )
{
	if( m_errors )
		m_errors->push_back( s ) ;
}

bool G::OptionParser::haveSeenOn( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	return p != m_map.end() && !(*p).second.isOff() ;
}

bool G::OptionParser::haveSeenOff( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	return p != m_map.end() && (*p).second.isOff() ;
}

bool G::OptionParser::haveSeen( const std::string & name ) const
{
	return m_map.find(name) != m_map.end() ;
}

bool G::OptionParser::haveSeenSame( const std::string & name , const std::string & value ) const
{
	auto p = m_map.find( name ) ;
	return p != m_map.end() && (*p).second.value() == value ;
}

std::size_t G::OptionParser::valueCount( const std::string & s )
{
	return 1U + std::count( s.begin() , s.end() , ',' ) ;
}

