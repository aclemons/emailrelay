//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// ggetopt.cpp
//	

#include "gdef.h"
#include "glog.h"
#include "gstrings.h"
#include "gstr.h"
#include "ggetopt.h"
#include "gassert.h"
#include "gdebug.h"
#include <sstream>

G::GetOpt::GetOpt( const Arg & args_in , const std::string & spec , 
	char sep_major , char sep_minor , char escape ) :
		m_args(args_in)
{
	parseSpec( spec , sep_major , sep_minor , escape ) ;
	size_t n = parseArgs( args_in ) ;
	remove( n ) ;
}

void G::GetOpt::parseSpec( const std::string & spec , char sep_major , char sep_minor , char escape )
{
	Strings outer ;
	std::string ws_major( 1U , sep_major ) ;
	G::Str::splitIntoFields( spec , outer , ws_major , escape , false ) ;

	for( Strings::iterator p = outer.begin() ; p != outer.end() ; ++p )
	{
		if( (*p).empty() ) continue ;
		StringArray inner ;
		std::string ws_minor( 1U , sep_minor ) ;
		G::Str::splitIntoFields( *p , inner , ws_minor , escape ) ;
		if( inner.size() != 6U )
		{
			std::ostringstream ss ;
			ss << "\"" << *p << "\" (" << ws_minor << ")" ;
			throw InvalidSpecification( ss.str() ) ;
		}
		bool is_valued = G::Str::toUInt( inner[3U] ) != 0U ;
		unsigned int level = G::Str::toUInt( inner[5U] ) ;
		addSpec( inner[1U] , inner[0U].at(0U) , inner[1U] , inner[2U] , is_valued , inner[4U] , level ) ;
	}
}

void G::GetOpt::addSpec( const std::string & sort_key , char c , const std::string & name , 
	const std::string & description , bool is_valued , const std::string & value_description ,
	unsigned int level )
{
	if( c == '\0' )
		throw InvalidSpecification() ;

	const bool debug = true ;
	if( debug )
	{
		std::ostringstream ss ;
		ss
			<< "G::GetOpt::addSpec: "
			<< "sort-key=" << sort_key << ": "
			<< "char=" << c << ": "
			<< "name=" << name << ": " 
			<< "description=" << description << ": " 
			<< "valued=" << (is_valued?"true":"false") << ": " 
			<< "value-description=" << value_description << ": "
			<< "level=" << level ;
		G_DEBUG( ss.str() ) ;
	}

	std::pair<SwitchSpecMap::iterator,bool> rc = 
		m_spec_map.insert( std::make_pair( sort_key , 
			SwitchSpec(c,name,description,is_valued,value_description,level) ) ) ;

	if( ! rc.second )
		throw InvalidSpecification("duplication") ;
}

bool G::GetOpt::valued( const std::string & name ) const
{
	return valued(key(name)) ;
}

bool G::GetOpt::valued( char c ) const
{
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( (*p).second.c == c )
			return (*p).second.valued ;
	}
	return false ;
}

char G::GetOpt::key( const std::string & name ) const
{
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( (*p).second.name == name )
		{
			return (*p).second.c ;
		}
	}
	G_DEBUG( "G::GetOpt::key: " << name << " not found" ) ;
	return '\0' ;
}

//static
size_t G::GetOpt::wrapDefault()
{
	return 79U ;
}

//static
size_t G::GetOpt::tabDefault()
{
	return 30U ;
}

//static
std::string G::GetOpt::introducerDefault()
{
	return "usage: " ;
}

//static
G::GetOpt::Level G::GetOpt::levelDefault()
{
	return Level(99U) ;
}

//static
size_t G::GetOpt::widthLimit( size_t w )
{
	return (w != 0U && w < 50U) ? 50U : w ;
}

void G::GetOpt::showUsage( std::ostream & stream , const std::string & args , bool verbose ) const
{
	showUsage( stream , m_args.prefix() , args , introducerDefault() , verbose ? levelDefault() : Level(1U) ) ;
}

void G::GetOpt::showUsage( std::ostream & stream , const std::string & exe , const std::string & args , 
	const std::string & introducer , Level level , size_t tab_stop , size_t width ) const
{
	stream 
		<< usageSummary(exe,args,introducer,level,width) << std::endl 
		<< usageHelp(level,tab_stop,width,false) ;
}

std::string G::GetOpt::usageSummary( const std::string & exe , const std::string & args , 
	const std::string & introducer , Level level , size_t width ) const
{
	std::string s = introducer + exe + " " + usageSummarySwitches(level) + args ;
	if( width != 0U )
	{
		return G::Str::wrap( s , "" , "  " , widthLimit(width) ) ;
	}
	else
	{
		return s ;
	}
}

std::string G::GetOpt::usageSummarySwitches( Level level ) const
{
	return usageSummaryPartOne(level) + usageSummaryPartTwo(level) ;
}

//static
bool G::GetOpt::visible( SwitchSpecMap::const_iterator p , Level level , bool exact )
{
	return 
		exact ?
			( !(*p).second.hidden && (*p).second.level == level.level ) :
			( !(*p).second.hidden && (*p).second.level <= level.level ) ;
}

std::string G::GetOpt::usageSummaryPartOne( Level level ) const
{
	// summarise the single-character switches, excluding those which take a value
	std::ostringstream ss ;
	bool first = true ;
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( !(*p).second.valued && visible(p,level,false) )
		{
			if( first )
				ss << "[-" ;
			first = false ;
			ss << (*p).second.c ;
		}
	}

	std::string s = ss.str() ;
	if( s.length() ) s.append( "] " ) ;
	return s ;
}

std::string G::GetOpt::usageSummaryPartTwo( Level level ) const
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( visible(p,level,false) )
		{
			ss << sep << "[" ;
			if( (*p).second.name.length() )
			{
				ss << "--" << (*p).second.name ;
			}
			else
			{
				ss << "-" << (*p).second.c ;
			}
			if( (*p).second.valued )
			{
				std::string vd = (*p).second.value_description ;
				if( vd.empty() ) vd = "value" ;
				ss << " <" << vd << ">" ;
			}
			ss << "]" ;
			sep = " " ;
		}
	}
	return ss.str() ;
}

std::string G::GetOpt::usageHelp( Level level , size_t tab_stop , size_t width , bool exact ) const
{
	return usageHelpCore( "  " , level , tab_stop , widthLimit(width) , exact ) ;
}

std::string G::GetOpt::usageHelpCore( const std::string & prefix , Level level ,
	size_t tab_stop , size_t width , bool exact ) const
{
	std::string result ;
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( visible(p,level,exact) )
		{
			std::string line( prefix ) ;
			line.append( "-" ) ;
			line.append( 1U , (*p).second.c ) ;

			if( (*p).second.name.length() )
			{
				line.append( ", --" ) ;
				line.append( (*p).second.name ) ;
			}

			if( (*p).second.valued )
			{
				std::string vd = (*p).second.value_description ;
				if( vd.empty() ) vd = "value" ;
				line.append( "=<" ) ;
				line.append( vd ) ;
				line.append( ">" ) ;
			}
			line.append( 1U , ' ' ) ;

			if( line.length() < tab_stop )
				line.append( tab_stop-line.length() , ' ' ) ;

			line.append( (*p).second.description ) ;

			if( width )
			{
				std::string indent( tab_stop , ' ' ) ;
				line = G::Str::wrap( line , "" , indent , width ) ;
			}
			else
			{
				line.append( 1U , '\n' ) ;
			}

			result.append( line ) ;
		}
	}
	return result ;
}

size_t G::GetOpt::parseArgs( const Arg & args_in )
{
	size_t i = 1U ;
	for( ; i < args_in.c() ; i++ )
	{
		const std::string & arg = args_in.v(i) ;

		if( arg == "--" ) // special "end-of-switches" switch
		{
			i++ ;
			break ;
		}

		if( isSwitchSet(arg) ) // eg. "-lt"
		{
			for( size_t n = 1U ; n < arg.length() ; n++ )
				processSwitch( arg.at(n) ) ;
		}
		else if( isOldSwitch(arg) ) // eg. "-v"
		{
			char c = arg.at(1U) ;
			if( valued(c) && (i+1U) >= args_in.c() )
				errorNoValue( c ) ;
			else if( valued(c) )
				processSwitch( c , args_in.v(++i) ) ;
			else
				processSwitch( c ) ;
		}
		else if( isNewSwitch(arg) ) // eg. "--foo"
		{
			std::string name = arg.substr( 2U ) ;
			size_t pos_eq = eqPos(name) ;
			bool has_eq = pos_eq != std::string::npos ;
			std::string eq_value = eqValue(name,pos_eq) ;
			if( has_eq ) name = name.substr(0U,pos_eq) ;

			if( valued(name) && !has_eq && (i+1U) >= args_in.c() )
				errorNoValue( name ) ;
			else if( valued(name) && !has_eq )
				processSwitch( name , args_in.v(++i) ) ;
			else if( valued(name) )
				processSwitch( name , eq_value ) ;
			else
				processSwitch( name ) ;
		}
		else
		{
			break ;
		}
	}
	i-- ;
	G_DEBUG( "G::GetOpt::parseArgs: removing " << i << " switch args" ) ;
	return i ;
}

size_t G::GetOpt::eqPos( const std::string & s )
{
	size_t p = s.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_") ;
	return p != std::string::npos && s.at(p) == '=' ? p : std::string::npos ;
}

std::string G::GetOpt::eqValue( const std::string & s , size_t pos )
{
	return (pos+1U) == s.length() ? std::string() : s.substr(pos+1U) ;
}

bool G::GetOpt::isOldSwitch( const std::string & arg ) const
{
	return 
		( arg.length() > 1U && arg.at(0U) == '-' ) &&
		! isNewSwitch( arg ) ;
}

bool G::GetOpt::isNewSwitch( const std::string & arg ) const
{
	return arg.length() > 2U && arg.at(0U) == '-' && arg.at(1U) == '-' ;
}

bool G::GetOpt::isSwitchSet( const std::string & arg ) const
{
	return isOldSwitch(arg) && arg.length() > 2U ;
}

void G::GetOpt::errorNoValue( char c )
{
	std::string e("no value supplied for -") ;
	e.append( 1U , c ) ;
	m_errors.push_back( e ) ;
}

void G::GetOpt::errorNoValue( const std::string & name )
{
	std::string e("no value supplied for --") ;
	e.append( name ) ;
	m_errors.push_back( e ) ;
}

void G::GetOpt::errorUnknownSwitch( char c )
{
	std::string e("invalid switch: -") ;
	e.append( 1U , c ) ;
	m_errors.push_back( e ) ;
}

void G::GetOpt::errorUnknownSwitch( const std::string & name )
{
	std::string e("invalid switch: --") ;
	e.append( name ) ;
	m_errors.push_back( e ) ;
}

void G::GetOpt::processSwitch( const std::string & name )
{
	if( !valid(name) )
	{
		errorUnknownSwitch( name ) ;
		return ;
	}

	char c = key(name) ;
	if( valued(c) )
	{
		errorNoValue( name ) ;
		return ;
	}

	m_map.insert( std::make_pair(c,std::make_pair(false,std::string())) ) ;
}

void G::GetOpt::processSwitch( const std::string & name , const std::string & value )
{
	if( !valid(name) )
	{
		errorUnknownSwitch( name ) ;
		return ;
	}

	char c = key(name) ;
	m_map.insert( std::make_pair(c,std::make_pair(true,value)) ) ;
}

void G::GetOpt::processSwitch( char c )
{
	if( !valid(c) )
	{
		errorUnknownSwitch( c ) ;
		return ;
	}

	if( valued(c) )
	{
		errorNoValue( c ) ;
		return ;
	}

	m_map.insert( std::make_pair(c,std::make_pair(false,std::string())) ) ;
}

void G::GetOpt::processSwitch( char c , const std::string & value )
{
	if( !valid(c) )
		errorUnknownSwitch( c ) ;

	m_map.insert( std::make_pair(c,std::make_pair(true,value)) ) ;
}

G::Strings G::GetOpt::errorList() const
{
	return m_errors ;
}

bool G::GetOpt::contains( char c ) const
{
	SwitchMap::const_iterator p = m_map.find( c ) ;
	return p != m_map.end() ;
}

bool G::GetOpt::contains( const std::string & name ) const
{
	char c = key(name) ;
	SwitchMap::const_iterator p = m_map.find( c ) ;
	return p != m_map.end() ;
}

std::string G::GetOpt::value( char c ) const
{
	G_ASSERT( contains(c) ) ;
	SwitchMap::const_iterator p = m_map.find( c ) ;
	Value value_pair = (*p).second ;
	return value_pair.second ;
}

std::string G::GetOpt::value( const std::string & name ) const
{
	G_ASSERT( contains(name) ) ;
	return value( key(name) ) ;
}

G::Arg G::GetOpt::args() const
{
	return m_args ;
}

void G::GetOpt::show( std::ostream &stream , std::string prefix ) const
{
	for( SwitchMap::const_iterator p = m_map.begin() ; p != m_map.end() ; ++p )
	{
		char c = (*p).first ;
		Value v = (*p).second ;
		bool valued = v.first ;
		std::string value = v.second ;

		std::string name ;
		for( SwitchSpecMap::const_iterator q = m_spec_map.begin() ; q != m_spec_map.end() ; ++q )
		{
			if( (*q).second.c == c )
			{
				name = (*q).second.name ;
				break ;
			}
		}

		stream << prefix << "-" << c ;
		if( !name.empty() )
			stream << ",--" << name ;
		if( valued )
			stream << " = \"" << value << "\"" ;
		stream << std::endl ;
	}
}

bool G::GetOpt::hasErrors() const
{
	return m_errors.size() != 0U ;
}

void G::GetOpt::showErrors( std::ostream & stream ) const
{
	showErrors( stream , m_args.prefix() ) ;
}

void G::GetOpt::showErrors( std::ostream & stream , std::string prefix_1 , 
	std::string prefix_2 ) const
{
	if( m_errors.size() != 0U )
	{
		for( Strings::const_iterator p = m_errors.begin() ;
			p != m_errors.end() ; ++p )
		{
			stream << prefix_1 << prefix_2 << *p << std::endl ;
		}
	}
}

void G::GetOpt::remove( size_t n )
{
	if( n != 0U )
	{
		m_args.removeAt(1U,n-1U) ;
	}
}

bool G::GetOpt::valid( const std::string & name ) const
{
	return valid( key(name) ) ;
}

bool G::GetOpt::valid( char c ) const
{
	for( SwitchSpecMap::const_iterator p = m_spec_map.begin() ; p != m_spec_map.end() ; ++p )
	{
		if( (*p).second.c == c )
			return true ;
	}
	return false ;
}


