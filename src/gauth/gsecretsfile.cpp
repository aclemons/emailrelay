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
// gsecretsfile.cpp
//

#include "gdef.h"
#include "gauth.h"
#include "gsecretsfile.h"
#include "gsecrets.h"
#include "groot.h"
#include "gxtext.h"
#include "gstr.h"
#include "gdatetime.h"
#include "gfile.h"
#include "gassert.h"
#include "gmemory.h"
#include <map>
#include <set>
#include <fstream>
#include <string>
#include <algorithm> // std::swap
#include <utility> // std::swap, std::pair
#include <sstream>

GAuth::SecretsFile::SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ,
	const std::string & server_type ) :
		m_path(path) ,
		m_auto(auto_reread) ,
		m_debug_name(debug_name) ,
		m_server_type(server_type) ,
		m_file_time(0) ,
		m_check_time(G::DateTime::now())
{
	m_server_type = m_server_type.empty() ? std::string("server") : m_server_type ;
	G_DEBUG( "GAuth::Secrets: " << m_debug_name << ": \"" << path << "\"" ) ;
	m_valid = ! path.str().empty() ;
	if( m_valid )
		read( path ) ;
}

bool GAuth::SecretsFile::valid() const
{
	return m_valid ;
}

void GAuth::SecretsFile::reread() const
{
	(const_cast<SecretsFile*>(this))->reread(0) ;
}

void GAuth::SecretsFile::reread( int )
{
	if( m_auto )
	{
		G::DateTime::EpochTime now = G::DateTime::now() ;
		G_DEBUG( "GAuth::SecretsFile::reread: file time checked at " << m_check_time << ": now " << now ) ;
		if( now != m_check_time ) // at most once a second
		{
			m_check_time = now ;
			G::DateTime::EpochTime t = readFileTime( m_path ) ;
			G_DEBUG( "GAuth::SecretsFile::reread: current file time " << t << ": saved file time " << m_file_time ) ;
			if( t != m_file_time )
			{
				G_LOG_S( "GAuth::Secrets: re-reading secrets file: " << m_path ) ;
				read( m_path ) ;
			}
		}
	}
}

void GAuth::SecretsFile::read( const G::Path & path )
{
	// open the file
	std::auto_ptr<std::ifstream> file ;
	{
		G::Root claim_root ;
		file <<= new std::ifstream( path.str().c_str() ) ;
	}
	if( !file->good() )
	{
		std::ostringstream ss ;
		ss << "reading \"" << path << "\" for " << m_debug_name << " secrets" ;
		throw Secrets::OpenError( ss.str() ) ;
	}
	m_file_time = readFileTime( path ) ;

	// read the file
	m_map.clear() ;
	m_set.clear() ;
	m_warnings.clear() ;
	read( *file.get() ) ;

	// show the results
	for( Map::iterator p = m_map.begin() ; p != m_map.end() ; ++p )
	{
		G_DEBUG( "GAuth::SecretsFile::read: key=[" << (*p).first << "] value=[" << (*p).second << "]" ) ;
	}

	// show warnings
	G_DEBUG( "GAuth::SecretsFile::read: processed " << m_map.size() << " records" ) ;
	if( m_warnings.size() )
	{
		std::string prefix = path.basename() ;
		G_WARNING( "GAuth::SecretsFile::read: problems reading client & " << m_server_type << " entries from " 
			<< m_debug_name << " secrets file [" << path.str() << "]..." ) ;
		for( Warnings::iterator p = m_warnings.begin() ; p != m_warnings.end() ; ++p )
		{
			G_WARNING( "GAuth::SecretsFile::read: " << prefix << "(" << (*p).first << "): " << (*p).second ) ;
		}
	}
}

G::DateTime::EpochTime GAuth::SecretsFile::readFileTime( const G::Path & path )
{
	G::Root claim_root ;
	return G::File::time( path ) ;
}

void GAuth::SecretsFile::read( std::istream & file )
{
	for( unsigned int line_number = 1U ; file.good() ; ++line_number )
	{
		std::string line = G::Str::readLineFrom( file ) ;
		if( !file ) 
			break ;

		G::Str::trim( line , G::Str::ws() ) ;
		if( !line.empty() && line.at(0U) != '#' )
		{
			G::StringArray word_array ;
			G::Str::splitIntoTokens( line , word_array , " \t" ) ;
			if( word_array.size() > 4U )
			{
				m_warnings.push_back( Warnings::value_type(line_number,"ignored extra fields") ) ;
			}
			if( word_array.size() >= 4U )
			{
				std::string dupe_key = process( word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
				if( !dupe_key.empty() )
					m_warnings.push_back( Warnings::value_type(line_number,"line ignored: duplicate key [" + dupe_key + "]") ) ;
			}
			else
			{
				m_warnings.push_back( Warnings::value_type(line_number,"line ignored: too few fields") ) ;
			}
		}
	}
}

std::string GAuth::SecretsFile::process( std::string side , std::string mechanism , std::string id , std::string secret )
{
	G::Str::toLower( mechanism ) ;
	G::Str::toLower( side ) ;

	// allow columns 1 and 2 to switch around
	if( mechanism == G::Str::lower(m_server_type) || mechanism == "client" )
		std::swap( mechanism , side ) ;

	G_DEBUG( "GAuth::SecretsFile::process: side=[" << side << "] mechanism=[" << mechanism << "] id=[" << id << "] secret=[" << secret << "]" ) ;

	if( side == G::Str::lower(m_server_type) )
	{
		// server-side
		std::string key = serverKey( id , mechanism ) ;
		std::string value = secret ;
		m_set.insert( G::Str::upper(mechanism) ) ;
		return m_map.insert(std::make_pair(key,value)).second ? std::string() : key ;
	}
	else if( side == "client" )
	{
		// client-side -- no userid in the key since only one secret
		std::string key = clientKey( mechanism ) ;
		std::string value = id + " " + secret ;
		return m_map.insert(std::make_pair(key,value)).second ? std::string() : key ;
	}
	else
	{
		return std::string() ; // not what we're looking for
	}
}

GAuth::SecretsFile::~SecretsFile()
{
}

std::string GAuth::SecretsFile::serverKey( const std::string & id , const std::string & mechanism )
{
	return "server:" + G::Str::lower(mechanism) + ":" + id ;
}

std::string GAuth::SecretsFile::clientKey( const std::string & mechanism )
{
	return "client:" + G::Str::lower(mechanism) ;
}

std::string GAuth::SecretsFile::id( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( clientKey(mechanism) ) ;
	if( p == m_map.end() || (*p).second.find(" ") == std::string::npos )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second.substr(0U,(*p).second.find(" ")) ) ; // head
}

std::string GAuth::SecretsFile::secret( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( clientKey(mechanism) ) ;
	if( p == m_map.end() || (*p).second.find(" ") == std::string::npos )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second.substr((*p).second.find(" ")+1U) ) ; // tail
}

std::string GAuth::SecretsFile::secret( const std::string & mechanism , const std::string & raw_id ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( serverKey(G::Xtext::encode(raw_id),mechanism) ) ;
	if( p == m_map.end() )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second ) ;
}

std::string GAuth::SecretsFile::path() const
{
	return m_path.str() ;
}

bool GAuth::SecretsFile::contains( const std::string & mechanism ) const
{
	return m_set.find(mechanism) != m_set.end() ;
}

/// \file gsecretsfile.cpp
