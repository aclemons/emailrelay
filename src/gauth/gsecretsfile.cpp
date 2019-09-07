//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <algorithm> // std::swap()
#include <utility> // std::swap(), std::pair
#include <sstream>

GAuth::SecretsFile::SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ,
	const std::string & server_type ) :
		m_path(path) ,
		m_auto(auto_reread) ,
		m_debug_name(debug_name) ,
		m_server_type(G::Str::lower(server_type)) ,
		m_file_time(0) ,
		m_check_time(G::DateTime::now())
{
	m_server_type = m_server_type.empty() ? std::string("server") : m_server_type ;
	m_valid = ! path.str().empty() ;
	if( m_valid )
		read( path , false ) ;
}

void GAuth::SecretsFile::check( const std::string & path )
{
	checkImp( path , true , "server" ) ; // allow only 'client' or 'server' lines
}

void GAuth::SecretsFile::checkImp( const std::string & path , bool strict_side , const std::string & server_type )
{
	if( !path.empty() )
	{
		Contents contents = readContents( path , server_type , strict_side ) ;
		showWarnings( contents.m_warnings , path ) ;
		if( !contents.m_warnings.empty() )
			throw Error() ;
	}
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
				read( m_path , false ) ;
			}
		}
	}
}

void GAuth::SecretsFile::read( const G::Path & path , bool strict_side )
{
	m_file_time = readFileTime( path ) ;
	m_contents = readContents( path , m_server_type , strict_side ) ;
	showWarnings( m_contents.m_warnings , path , m_debug_name ) ;
}

G::EpochTime GAuth::SecretsFile::readFileTime( const G::Path & path )
{
	G::Root claim_root ;
	return G::File::time( path ) ;
}

GAuth::SecretsFile::Contents GAuth::SecretsFile::readContents( const G::Path & path , const std::string & server_type , bool strict_side )
{
	unique_ptr<std::ifstream> file ;
	{
		G::Root claim_root ;
		file.reset( new std::ifstream( path.str().c_str() ) ) ;
	}
	if( !file->good() )
	{
		throw Secrets::OpenError( path.str() ) ;
	}

	return readContents( *file.get() , server_type , strict_side ) ;
}

GAuth::SecretsFile::Contents GAuth::SecretsFile::readContents( std::istream & file , const std::string & server_type , bool strict_side )
{
	Contents contents ;
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
				processLine( contents , server_type , line_number , word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] , strict_side ) ;
			}
			else
			{
				addWarning( contents , line_number , "too few fields" , "" ) ;
			}
		}
	}
	return contents ;
}

void GAuth::SecretsFile::processLine( Contents & contents , const std::string & server_type ,
	unsigned int line_number , const std::string & side_in , const std::string & encoding_type_in ,
	const std::string & id_xtext_or_ip , const std::string & secret , bool strict_side )
{
	G_ASSERT( !side_in.empty() && !encoding_type_in.empty() && !id_xtext_or_ip.empty() && !secret.empty() ) ;
	std::string encoding_type = G::Str::lower( encoding_type_in ) ;
	std::string side = G::Str::lower( side_in ) ;

	if( !G::Xtext::valid(id_xtext_or_ip) ) // (ip address ranges are valid xtext)
		addWarning( contents , line_number , "invalid xtext encoding in third field" , id_xtext_or_ip ) ;

	if( encoding_type == "client" || encoding_type == "server" || encoding_type == server_type ) // old-style field order, eg. "cram-md5 server ..."
	{
		const bool plain = side == "plain" || side == "login" ;
		addWarning( contents , line_number , "incorrect field order: use \"" + encoding_type + " " + (plain?"plain":"md5") + " <id> <" + (plain?"pwd":"hash") + ">\"" , "" ) ;
	}
	else if( side == server_type )
	{
		// server-side
		std::string key = serverKey( encoding_type , id_xtext_or_ip ) ;
		Value value( secret , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(key,value)).second ;
		if( inserted )
			contents.m_types.insert( canonical(encoding_type) ) ;
		else
			addWarning( contents , line_number , "duplicate server secret" , key ) ;
	}
	else if( side == "client" )
	{
		// client-side
		const std::string & id = id_xtext_or_ip ;
		std::string key = clientKey( encoding_type ) ; // not including user id
		Value value( id + " " + secret , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(key,value)).second ;
		if( !inserted )
			addWarning( contents , line_number , "too many client secrets" , key ) ;
	}
	else if( strict_side )
	{
		addWarning( contents , line_number , "invalid value in first field: expecting 'client' or '" + server_type + "'" , side ) ;
	}
	else
	{
		G_DEBUG( "GAuth::SecretsFile::processLine: ignoring line number " << line_number << ": not 'client' or '" << server_type << "'" ) ;
	}
}

void GAuth::SecretsFile::addWarning( Contents & contents , unsigned int line_number , const std::string & message_in , const std::string & more )
{
	std::string message = more.empty() ? message_in : ( message_in + ": [" + G::Str::printable(more) + "]" ) ;
	contents.m_warnings.push_back( Warnings::value_type(line_number,message) ) ;
}

void GAuth::SecretsFile::showWarnings( const Warnings & warnings , const G::Path & path , const std::string & debug_name )
{
	if( warnings.size() )
	{
		std::string prefix = path.basename() ;
		G_WARNING( "GAuth::SecretsFile::read: problems reading" << (debug_name.empty()?"":" ") << debug_name << " secrets file [" << path.str() << "]..." ) ;
		for( Warnings::const_iterator p = warnings.begin() ; p != warnings.end() ; ++p )
		{
			G_WARNING( "GAuth::SecretsFile::read: " << prefix << "(" << (*p).first << "): " << (*p).second ) ;
		}
	}
}

std::string GAuth::SecretsFile::canonical( const std::string & encoding_type )
{
	std::string type = G::Str::lower( encoding_type ) ;
	if( type == "cram-md5" ) type = "md5" ;
	if( type == "apop" ) type = "md5" ;
	if( type == "login" ) type = "plain" ;
	return type ;
}

std::string GAuth::SecretsFile::serverKey( const std::string & encoding_type , const std::string & id_xtext )
{
	// eg. key -> value
	// "server plain bob" -> "e+3Dmc2"
	// "server md5 bob" -> "xbase64x=="
	// "server none 192.168.0.0/24" -> "trustee"
	// "server none ::1/128" -> "trustee"
	return "server " + canonical(encoding_type) + " " + id_xtext ;
}

std::string GAuth::SecretsFile::clientKey( const std::string & encoding_type )
{
	// eg. key -> value...
	// "client plain" -> "alice secret+21"
	// "client md5" -> "alice xbase64x=="
	return "client " + canonical(encoding_type) ;
}

GAuth::Secret GAuth::SecretsFile::clientSecret( const std::string & encoding_type ) const
{
	reread() ;

	Map::const_iterator p = m_contents.m_map.find( clientKey(encoding_type) ) ;
	if( p == m_contents.m_map.end() )
	{
		return Secret::none() ;
	}
	else
	{
		std::string id_xtext = G::Str::head( (*p).second.s , " " ) ;
		std::string secret_encoded = G::Str::tail( (*p).second.s , " " ) ;
		return Secret( secret_encoded , canonical(encoding_type) , id_xtext , true , line((*p).second.n) ) ;
	}
}

GAuth::Secret GAuth::SecretsFile::serverSecret( const std::string & encoding_type , const std::string & id ) const
{
	if( id.empty() )
		return Secret::none() ;

	reread() ;

	Map::const_iterator p = m_contents.m_map.find( serverKey(encoding_type,G::Xtext::encode(id)) ) ;
	if( p == m_contents.m_map.end() )
	{
		return Secret::none( id ) ;
	}
	else
	{
		return Secret( (*p).second.s , canonical(encoding_type) , id , false , line((*p).second.n) ) ;
	}
}

std::pair<std::string,std::string> GAuth::SecretsFile::serverTrust( const std::string & address_range ) const
{
	std::pair<std::string,std::string> result ;
	std::string encoding_type = "none" ;
	std::string id = address_range ; // the address-range lives in the id field
	Map::const_iterator p = m_contents.m_map.find( serverKey(encoding_type,G::Xtext::encode(id)) ) ;
	if( p != m_contents.m_map.end() )
	{
		result.first = (*p).second.s ; // the trustee name lives in the shared-secret field
		result.second = line( (*p).second.n ) ;
	}
	return result ;
}

std::string GAuth::SecretsFile::path() const
{
	return m_path.str() ;
}

bool GAuth::SecretsFile::contains( const std::string & encoding_type ) const
{
	return m_contents.m_types.find(canonical(encoding_type)) != m_contents.m_types.end() ;
}

std::string GAuth::SecretsFile::line( unsigned int line_number )
{
	return "line " + G::Str::fromUInt(line_number) ;
}
/// \file gsecretsfile.cpp
