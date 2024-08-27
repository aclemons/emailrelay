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
/// \file gmapfile.cpp
///

#include "gdef.h"
#include "gmapfile.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "gpath.h"
#include "gfile.h"
#include "gcodepage.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm> // std::find
#include <iterator>
#include <stdexcept>
#include <array>

G::MapFile::MapFile()
= default;

G::MapFile::MapFile( const Path & path , std::string_view kind ) :
	m_path(path) ,
	m_kind(sv_to_string(kind))
{
	if( !m_path.empty() )
		readFromFile( path , kind , true ) ;
}

G::MapFile::MapFile( const Path & path , std::string_view kind , std::nothrow_t ) :
	m_path(path) ,
	m_kind(sv_to_string(kind))
{
	if( !m_path.empty() )
		readFromFile( path , kind , false ) ;
}

G::MapFile::MapFile( std::istream & stream )
{
	readFromStream( stream ) ;
}

G::MapFile::MapFile( const StringMap & map ) :
	m_map(map)
{
	if( !m_map.empty() )
		m_keys.reserve( m_map.size() ) ;
	for( auto & p : m_map )
		m_keys.push_back( p.first ) ;
}

G::MapFile::MapFile( const OptionMap & map , std::string_view yes )
{
	for( auto p = map.begin() ; p != map.end() ; )
	{
		const std::string & key = (*p).first ;
		if( !(*p).second.isOff() )
		{
			std::string value = (*p).second.isOn() ? sv_to_string(yes) : map.value(key) ;
			add( key , value ) ;
		}
		while( p != map.end() && (*p).first == key ) // since we used OptionMap::value() to get them all
			++p ;
	}
}

void G::MapFile::readFromFile( const Path & path , std::string_view kind , bool do_throw )
{
	std::ifstream stream ;
	File::open( stream , path , File::Text() ) ;
	if( !stream.good() )
	{
		if( !do_throw ) return ;
		throw readError( path , kind ) ;
	}

	readFromStream( stream ) ;

	if( stream.bad() && do_throw ) // eg. EISDIR
		throw readError( path , sv_to_string(kind) ) ;
}

G::MapFile::List G::MapFile::readLines( const Path & path , std::string_view kind , bool do_throw ) const
{
	std::ifstream stream ;
	File::open( stream , path , File::Text() ) ;
	if( !stream.good() )
	{
		if( !do_throw ) return {} ;
		throw readError( path , kind ) ;
	}

	List line_list ; // all lines including blanks and comments
	std::string line ;
	while( stream.good() && Str::readLine(stream,line) )
		line_list.push_back( line ) ;
	return line_list ;
}

void G::MapFile::readFromStream( std::istream & stream )
{
	std::string line ;
	while( stream.good() )
	{
		Str::readLine( stream , line ) ;
		Str::trimRight( line , std::string_view("\r",1U) ) ;
		if( line.empty() )
			continue ;
		if( !stream )
			break ;

		if( !valued(line) )
			continue ;

		auto pair = split( line ) ;
		if( !pair.first.empty() )
			add( pair.first , pair.second ) ;
	}
}

std::pair<std::string_view,std::string_view> G::MapFile::split( std::string_view line )
{
	StringTokenView t( line , std::string_view(" =\t",3U) ) ;
	if( !t.valid() )
		return {{},{}} ;

	std::string_view key = t() ;
	auto pos = line.find( key.data() , 0U , key.size() ) + key.size() ;
	std::string_view value = Str::tailView( line , pos ) ;
	value = Str::trimLeftView( value , std::string_view(" =\t",3U) ) ;
	value = Str::trimRightView( value , Str::ws() ) ;

	// strip simple quotes -- no escaping
	if( value.size() >= 2U && value.at(0U) == '"' && value.at(value.size()-1U) == '"' )
		value = value.substr( 1U , value.length() - 2U ) ;

	return {key,value} ;
}

std::string G::MapFile::join( std::string_view key , std::string_view value )
{
	std::string line = sv_to_string(key).append(1U,' ').append(quote(sv_to_string(value))) ;
	return Str::trimmed( line , Str::ws() ) ;
}

std::string G::MapFile::quote( const std::string & s )
{
	return s.find_first_of(" \t") == std::string::npos ? s : ("\""+s+"\"") ;
}

bool G::MapFile::valued( const std::string & line )
{
	auto pos_letter = line.find_first_not_of( " \t\r#" ) ;
	auto pos_hash = line.find( '#' ) ;
	if( pos_letter == std::string::npos )
		return false ; // no value if just # and ws
	else if( pos_hash == std::string::npos )
		return true ; // value if only letters
	else
		return pos_hash >= pos_letter ; // value if comment comes later
}

bool G::MapFile::commentedOut( const std::string & line )
{
	auto pos_letter = line.find_first_not_of( " \t\r#" ) ;
	auto pos_hash = line.find( '#' ) ;
	if( pos_letter == std::string::npos )
		return false ; // not commented-out if just # and ws
	else if( pos_hash == std::string::npos )
		return false ; // not commented-out if only letters
	else
		return pos_hash == 0U && pos_letter == 1U ; // commented-out if hash then letter
}

void G::MapFile::log( const std::string & prefix_in ) const
{
	std::string prefix = prefix_in.empty() ? std::string() : ( prefix_in + ": " ) ;
	for( const auto & key : m_keys )
	{
		auto p = find( key ) ;
		if( p == m_map.end() ) continue ;
		std::string value = (*p).second ;
		G_LOG( "MapFile::item: " << prefix << key << "=[" <<
			( Str::ifind(key,"password") == std::string::npos ?
				Str::printable(value) :
				std::string("<not-logged>")
			) << "]" ) ;
	}
}

void G::MapFile::writeItem( std::ostream & stream , std::string_view key ) const
{
	auto p = find( key ) ;
	if( p == m_map.end() )
		writeItem( stream , key , {} ) ;
	else
		writeItem( stream , key , (*p).second ) ;
}

void G::MapFile::writeItem( std::ostream & stream , std::string_view key , std::string_view value )
{
	const char * qq = value.find(' ') == std::string::npos ? "" : "\"" ;
	stream << key << "=" << qq << value << qq << "\n" ;
}

G::Path G::MapFile::editInto( const Path & path , bool make_backup , bool do_throw ) const
{
	// read the file
	List lines = readLines( path , m_kind , do_throw ) ;
	List old_lines ;
	if( make_backup )
		old_lines = lines ;

	// mark lines that are values that we can change, including lines that
	// might be commented-out values
	std::for_each( lines.begin() , lines.end() , [](std::string & line_){
			if( valued(line_) ) line_.insert(0U,1U,'\0') ;
			else if( commentedOut(line_) ) line_.at(0U) = '\0';
		} ) ;

	// re-write lines that match each item in the map
	for( const auto & map_item : m_map )
	{
		bool found = false ;
		for( auto & line : lines )
		{
			if( !line.empty() && line[0] == '\0' )
			{
				auto pair = split( std::string_view(line).substr(1U) ) ;
				if( !pair.first.empty() && pair.first == map_item.first )
				{
					line = join( pair.first , map_item.second ) ;
					found = true ;
					break ;
				}
			}
		}
		if( !found )
		{
			lines.push_back( join(map_item.first,map_item.second) ) ;
		}
	}

	// comment-out lines we could have re-written but didn't
	std::for_each( lines.begin() , lines.end() , [](std::string & line_){
		if( !line_.empty() && line_[0U] == '\0' ) line_[0] = '#';} ) ;

	// optionally make a backup if there have been changes
	Path backup_path ;
	if( make_backup && lines != old_lines )
		backup_path = File::backup( path , std::nothrow ) ;

	// write the lines back to the file
	std::ofstream file_out ;
	File::open( file_out , path , File::Text() ) ;
	std::copy( lines.begin() , lines.end() , std::ostream_iterator<std::string>(file_out,"\n") ) ;
	file_out.close() ;
	if( file_out.fail() && do_throw )
		throw writeError( path ) ;

	return backup_path ;
}

bool G::MapFile::booleanValue( std::string_view key , bool default_ ) const
{
	auto p = find( key ) ;
	if( p == m_map.end() )
	{
		return default_ ;
	}
	else if( (*p).second.empty() )
	{
		return true ;
	}
	else
	{
		return Str::isPositive( (*p).second ) ;
	}
}

std::string G::MapFile::value( std::string_view key , std::string_view default_ ) const
{
	auto p = find( key ) ;
	return ( p == m_map.end() || (*p).second.empty() ) ? sv_to_string(default_) : (*p).second ;
}

bool G::MapFile::valueContains( std::string_view key , std::string_view token , std::string_view default_ ) const
{
	const std::string s = value( key , default_ ) ;
	const std::string_view sv = s ;
	for( StringTokenView t(sv,",",1U) ; t ; ++t )
	{
		if( t() == token )
			return true ;
	}
	return false ;
}

std::string G::MapFile::mandatoryValue( std::string_view key ) const
{
	if( find(key) == m_map.end() )
		throw missingValueError( m_path , m_kind , sv_to_string(key) ) ;
	return value( key ) ;
}

G::Path G::MapFile::expandedPathValue( std::string_view key , const Path & default_ ) const
{
	return toPath( expand(value(key,default_.str())) ) ;
}

G::Path G::MapFile::expandedPathValue( std::string_view key ) const
{
	return toPath( expand(mandatoryValue(key)) ) ;
}

G::Path G::MapFile::pathValue( std::string_view key , const Path & default_ ) const
{
	return toPath( value(key,default_.str()) ) ;
}

G::Path G::MapFile::pathValue( std::string_view key ) const
{
	return toPath( mandatoryValue(key) ) ;
}

G::Path G::MapFile::toPath( std::string_view path_in )
{
	// (temporary backwards compatibility in case the file is ansi-encoded)
	Path path1( path_in ) ;
	Path path2( CodePage::fromCodePageAnsi(path_in) ) ;
	if( is_windows() && !File::isDirectory(path1,std::nothrow) && File::isDirectory(path2,std::nothrow) )
		return path2 ;
	return path1 ;
}

unsigned int G::MapFile::numericValue( std::string_view key , unsigned int default_ ) const
{
	return Str::toUInt( value(key,{}) , default_ ) ;
}

bool G::MapFile::remove( std::string_view key )
{
	auto p = find( key ) ;
	if( p != m_map.end() )
	{
		m_map.erase( p ) ;
		G_ASSERT( std::find(m_keys.begin(),m_keys.end(),key) != m_keys.end() ) ;
		m_keys.erase( std::find(m_keys.begin(),m_keys.end(),key) ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

std::string G::MapFile::expand( std::string_view value_in ) const
{
	std::string value = sv_to_string(value_in) ;
	expand_( value ) ;
	return value ;
}

namespace G
{
	namespace MapFileImp
	{
		std::size_t find_single( std::string & s , char c , std::size_t start_pos )
		{
			std::array<char,2U> cc {{ c , '\0' }} ;
			std::size_t pos = start_pos ;
			for(;;)
			{
				pos = s.find( &cc[0] , pos ) ;
				if( pos == std::string::npos )
				{
					break ; // not found
				}
				else if( (pos+1U) < s.length() && s.at(pos+1U) == c )
				{
					s.erase( pos , 1U ) ;
					if( (pos+1U) == s.length() )
					{
						pos = std::string::npos ;
						break ;
					}
					pos++ ;
				}
				else
				{
					break ; // found
				}
			}
			return pos ;
		}
	}
}

bool G::MapFile::expand_( std::string & value ) const
{
	bool changed = false ;
	std::size_t start = 0U ;
	std::size_t end = 0U ;
	std::size_t const npos = std::string::npos ;
	while( end < value.length() )
	{
		start = MapFileImp::find_single( value , '%' , end ) ;
		if( start == npos ) break ;
		end = value.find( '%' , start+1U ) ;
		if( end == npos ) break ;
		end++ ;
		std::string key = value.substr( start+1U , end-start-2U ) ;
		auto p = find( key ) ;
		if( p != m_map.end() )
		{
			std::size_t old = end - start ;
			std::size_t new_ = (*p).second.length() ;
			value.replace( start , old , (*p).second ) ;
			end += new_ ;
			end -= old ;
			changed = true ;
		}
	}
	return changed ;
}

void G::MapFile::add( std::string_view key , std::string_view value , bool clear )
{
	auto p = find( key ) ;
	if( p == m_map.end() )
	{
		m_keys.push_back( sv_to_string(key) ) ;
		m_map[sv_to_string(key)] = sv_to_string(value) ;
	}
	else if( clear )
	{
		m_map[sv_to_string(key)] = sv_to_string(value) ;
	}
	else
	{
		(*p).second.append(1U,',').append( value.data() , value.size() ) ;
	}
}

bool G::MapFile::update( std::string_view key , std::string_view value )
{
	auto p = find( key ) ;
	if( p == m_map.end() )
	{
		return false ;
	}
	else
	{
		(*p).second = sv_to_string(value) ;
		return true ;
	}
}

G::StringMap::iterator G::MapFile::find( std::string_view key )
{
	return m_map.find( sv_to_string(key) ) ; // or c++14 'generic associative lookup' of std::string_view
}

G::StringMap::const_iterator G::MapFile::find( std::string_view key ) const
{
	return m_map.find( sv_to_string(key) ) ; // or c++14 'generic associative lookup' of std::string_view
}

bool G::MapFile::contains( std::string_view key ) const
{
	return find( key ) != m_map.end() ;
}

const G::StringMap & G::MapFile::map() const
{
	return m_map ;
}

const G::StringArray & G::MapFile::keys() const
{
	return m_keys ;
}

std::string G::MapFile::strkind( std::string_view kind )
{
	return kind.empty() ? std::string("map") : sv_to_string(kind) ;
}

std::string G::MapFile::strpath( const Path & path_in )
{
	return path_in.empty() ? std::string() : (" ["+path_in.str()+"]") ;
}

G::MapFile::Error G::MapFile::readError( const Path & path , std::string_view kind )
{
	std::string description = "cannot read " + strkind(kind) + " file" + strpath(path) ;
	return Error( description ) ;
}

G::MapFile::Error G::MapFile::writeError( const Path & path , std::string_view kind )
{
	return Error( std::string("cannot create ").append(strkind(kind)).append(" file ").append(strpath(path)) ) ;
}

G::MapFile::Error G::MapFile::missingValueError( const Path & path , const std::string & kind ,
	const std::string & key )
{
	return Error( std::string("no item [").append(key).append("] in ").append(strkind(kind)).append(" file ").append(strpath(path)) ) ;
}

