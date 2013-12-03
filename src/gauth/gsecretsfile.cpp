//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

GAuth::SecretsFile::SecretsFile( const G::Path & path , bool auto_ , const std::string & debug_name ,
	const std::string & server_type ) :
		m_path(path) ,
		m_auto(auto_) ,
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

	m_map.clear() ;
	m_set.clear() ;
	unsigned int count = read( *file.get() ) ;
	count++ ; // avoid 'unused' warning
	G_DEBUG( "GAuth::SecretsFile::read: processed " << (count-1) << " records" ) ;
}

G::DateTime::EpochTime GAuth::SecretsFile::readFileTime( const G::Path & path )
{
	G::Root claim_root ;
	return G::File::time( path ) ;
}

unsigned int GAuth::SecretsFile::read( std::istream & file )
{
	unsigned int count = 0U ;
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
				G_WARNING( "GAuth::SecretsFile::read: ignoring extra fields on line " 
					<< line_number << " of secrets file" ) ;
			}
			if( word_array.size() >= 4U )
			{
				bool processed = process( word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
				G_DEBUG( "GAuth::SecretsFile::read: line #" << line_number << (processed?" used":" ignored") ) ;
				if( processed )
					count++ ;
			}
			else
			{
				G_WARNING( "GAuth::SecretsFile::read: ignoring line " 
					<< line_number << " of secrets file: too few fields" ) ;
			}
		}
	}
	return count ;
}

bool GAuth::SecretsFile::process( std::string side , std::string mechanism , std::string id , std::string secret )
{
	G::Str::toUpper( mechanism ) ;
	G::Str::toUpper( side ) ;

	// allow columns 1 and 2 to switch around
	if( mechanism == G::Str::upper(m_server_type) || mechanism == "CLIENT" )
		std::swap( mechanism , side ) ;

	if( side == G::Str::upper(m_server_type) )
	{
		// server-side
		std::string key = mechanism + ":" + id ;
		std::string value = secret ;
		m_map.insert( std::make_pair(key,value) ) ;
		m_set.insert( mechanism ) ;
		return true ;
	}
	else if( side == "CLIENT" )
	{
		// client-side -- no userid in the key since only one secret
		std::string key = mechanism + " client" ;
		std::string value = id + " " + secret ;
		m_map.insert( std::make_pair(key,value) ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

GAuth::SecretsFile::~SecretsFile()
{
}

std::string GAuth::SecretsFile::id( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	std::string result ;
	if( p != m_map.end() && (*p).second.find(" ") != std::string::npos )
		result = G::Xtext::decode( (*p).second.substr(0U,(*p).second.find(" ")) ) ;
	G_DEBUG( "GAuth::Secrets::id: " << m_debug_name << ": \"" << mechanism << "\"" ) ;
	return result ;
}

std::string GAuth::SecretsFile::secret( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	if( p == m_map.end() || (*p).second.find(" ") == std::string::npos )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second.substr((*p).second.find(" ")+1U) ) ;
}

std::string GAuth::SecretsFile::secret( const std::string & mechanism , const std::string & id ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+":"+G::Xtext::encode(id) ) ;
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
