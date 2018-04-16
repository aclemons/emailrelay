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
// gsecretsfile.cpp
//

#include "gdef.h"
#include "gsecretsfile.h"
#include "gsecrets.h"
#include "groot.h"
#include "gxtext.h"
#include "gstr.h"
#include "gdatetime.h"
#include "gfile.h"
#include "gassert.h"
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

GAuth::SecretsFile::~SecretsFile()
{
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
		G::EpochTime now = G::DateTime::now() ;
		G_DEBUG( "GAuth::SecretsFile::reread: file time checked at " << m_check_time << ": now " << now ) ;
		if( now != m_check_time ) // at most once a second
		{
			m_check_time = now ;
			G::EpochTime t = readFileTime( m_path ) ;
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
	unique_ptr<std::ifstream> file ;
	{
		G::Root claim_root ;
		file.reset( new std::ifstream( path.str().c_str() ) ) ;
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
	m_types.clear() ;
	m_warnings.clear() ;
	read( *file.get() ) ;

	// show warnings
	G_DEBUG( "GAuth::SecretsFile::read: processed " << m_map.size() << " records" ) ;
	if( m_warnings.size() )
	{
		std::string prefix = path.basename() ;
		G_WARNING( "GAuth::SecretsFile::read: problems reading " << m_debug_name
			<< " secrets file [" << path.str() << "]..." ) ;
		for( Warnings::iterator p = m_warnings.begin() ; p != m_warnings.end() ; ++p )
		{
			G_WARNING( "GAuth::SecretsFile::read: " << prefix << "(" << (*p).first << "): " << (*p).second ) ;
		}
	}
}

G::EpochTime GAuth::SecretsFile::readFileTime( const G::Path & path )
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
			if( word_array.size() >= 4U )
			{
				// 0=<client-server> 1=<encoding-type> 2=<userid-or-ipaddress> 3=<secret-or-verifier-hint>
				process( line_number , word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
			}
			else
			{
				warning( line_number , "line ignored: too few fields" , "" ) ;
			}
		}
	}
}

void GAuth::SecretsFile::process( unsigned int line_number , std::string side ,
	std::string encoding_type , std::string id , std::string secret )
{
	G_ASSERT( !side.empty() && !encoding_type.empty() && !id.empty() && !secret.empty() ) ;
	G::Str::toLower( encoding_type ) ;
	G::Str::toLower( side ) ;

	// allow columns 1 and 2 to switch around - TODO remove backwards compatibility
	if( encoding_type == G::Str::lower(m_server_type) || encoding_type == "client" )
		std::swap( encoding_type , side ) ;

	if( side == G::Str::lower(m_server_type) )
	{
		// server-side
		std::string key = serverKey( encoding_type , id ) ;
		Value value( secret , line_number ) ;
		bool inserted = m_map.insert(std::make_pair(key,value)).second ;
		if( inserted )
			m_types.insert( canonical(encoding_type) ) ;
		else
			warning( line_number , "line ignored: duplicate key" , key ) ;
	}
	else if( side == "client" )
	{
		// client-side
		std::string key = clientKey( encoding_type ) ; // not including user id
		Value value( id + " " + secret , line_number ) ;
		bool inserted = m_map.insert(std::make_pair(key,value)).second ;
		if( !inserted )
			warning( line_number , "line ignored: duplicate key" , key ) ;
	}
	else
	{
		G_DEBUG( "GAuth::SecretsFile::process: ignoring line number " << line_number << ": not 'client' or '" << m_server_type << "'" ) ;
	}
}

void GAuth::SecretsFile::warning( unsigned int line_number , const std::string & message , const std::string & more )
{
	m_warnings.push_back( Warnings::value_type(line_number,message+(more.empty()?std::string():": ")+more) ) ;
}

std::string GAuth::SecretsFile::canonical( const std::string & encoding_type )
{
	std::string type = G::Str::lower( encoding_type ) ;
	if( type == "cram-md5" ) type = "md5" ;
	if( type == "apop" ) type = "md5" ;
	if( type == "login" ) type = "plain" ;
	return type ;
}

std::string GAuth::SecretsFile::serverKey( std::string encoding_type , const std::string & id )
{
	// eg. key -> value
	// "server plain bob" -> "e+3Dmc2"
	// "server md5 bob" -> "xbase64x=="
	// "server none 192.168.0.0/24" -> "trustee"
	// "server none ::1/128" -> "trustee"
	return "server " + canonical(encoding_type) + " " + id ;
}

std::string GAuth::SecretsFile::clientKey( std::string encoding_type )
{
	// eg. key -> value...
	// "client plain" -> "alice secret+21"
	// "client md5" -> "alice xbase64x=="
	return "client " + canonical(encoding_type) ;
}

GAuth::Secret GAuth::SecretsFile::clientSecret( const std::string & encoding_type ) const
{
	reread() ;

	Map::const_iterator p = m_map.find( clientKey(encoding_type) ) ;
	if( p == m_map.end() )
	{
		return Secret::none() ;
	}
	else
	{
		std::string id = G::Xtext::decode( G::Str::head( (*p).second.s , " " ) ) ;
		std::string str_secret = G::Str::tail( (*p).second.s , " " ) ;
		return Secret( str_secret , canonical(encoding_type) , id , context((*p).second.n) ) ;
	}
}

GAuth::Secret GAuth::SecretsFile::serverSecret( const std::string & encoding_type , const std::string & id ) const
{
	if( id.empty() )
		return Secret::none() ;

	reread() ;

	Map::const_iterator p = m_map.find( serverKey(encoding_type,G::Xtext::encode(id)) ) ;
	if( p == m_map.end() )
	{
		return Secret::none( id ) ;
	}
	else
	{
		return Secret( (*p).second.s , canonical(encoding_type) , id , context((*p).second.n) ) ;
	}
}

std::pair<std::string,std::string> GAuth::SecretsFile::serverTrust( const std::string & address_range ) const
{
	std::pair<std::string,std::string> result ;
	std::string encoding_type = "none" ;
	std::string id = address_range ; // the address-range lives in the id field
	Map::const_iterator p = m_map.find( serverKey(encoding_type,G::Xtext::encode(id)) ) ;
	if( p != m_map.end() )
	{
		result.first = (*p).second.s ; // the trustee name lives in the shared-secret field
		result.second = context( (*p).second.n ) ;
	}
	return result ;
}

std::string GAuth::SecretsFile::path() const
{
	return m_path.str() ;
}

bool GAuth::SecretsFile::contains( const std::string & encoding_type ) const
{
	return m_types.find(canonical(encoding_type)) != m_types.end() ;
}

std::string GAuth::SecretsFile::context( unsigned int line_number )
{
	return " from line " + G::Str::fromUInt(line_number) ;
}
/// \file gsecretsfile.cpp
