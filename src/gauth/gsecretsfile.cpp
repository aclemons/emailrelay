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
/// \file gsecretsfile.cpp
///

#include "gdef.h"
#include "gsecretsfile.h"
#include "gsecrets.h"
#include "groot.h"
#include "gxtext.h"
#include "gbase64.h"
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

GAuth::SecretsFile::SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ) :
	m_path(path) ,
	m_auto(auto_reread) ,
	m_debug_name(debug_name) ,
	m_file_time(0) ,
	m_check_time(G::SystemTime::now())
{
	m_valid = ! path.str().empty() ;
	if( m_valid )
		read( path ) ;
}

void GAuth::SecretsFile::check( const std::string & path )
{
	if( !path.empty() )
	{
		Contents contents = readContents( path ) ;
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
		G::SystemTime now = G::SystemTime::now() ;
		G_DEBUG( "GAuth::SecretsFile::reread: file time checked at " << m_check_time << ": now " << now ) ;
		if( !now.sameSecond(m_check_time) ) // at most once a second
		{
			m_check_time = now ;
			G::SystemTime t = readFileTime( m_path ) ;
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
	m_file_time = readFileTime( path ) ;
	m_contents = readContents( path ) ;
	showWarnings( m_contents.m_warnings , path , m_debug_name ) ;
}

G::SystemTime GAuth::SecretsFile::readFileTime( const G::Path & path )
{
	G::Root claim_root ;
	return G::File::time( path ) ;
}

GAuth::SecretsFile::Contents GAuth::SecretsFile::readContents( const G::Path & path )
{
	std::unique_ptr<std::ifstream> file ;
	{
		G::Root claim_root ;
		file = std::make_unique<std::ifstream>( path.cstr() ) ;
	}
	if( !file->good() )
	{
		throw Secrets::OpenError( path.str() ) ;
	}

	return readContents( *file ) ;
}

GAuth::SecretsFile::Contents GAuth::SecretsFile::readContents( std::istream & file )
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
				processLine( contents , line_number , word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
			}
			else
			{
				addWarning( contents , line_number , "too few fields" ) ;
			}
		}
	}
	return contents ;
}

void GAuth::SecretsFile::processLine( Contents & contents ,
	unsigned int line_number , const std::string & side , const std::string & type_in ,
	const std::string & id_or_ip_in , const std::string & secret_in )
{
	std::string type = G::Str::lower( type_in ) ;
	std::string id_or_ip = id_or_ip_in ;
	std::string secret = secret_in ;
	if( type == "plain:b" )
	{
		// for now just re-encode to xtext -- TODO rework secrets-file encodings
        bool valid_id = G::Base64::valid( id_or_ip ) ;
		bool valid_secret = G::Base64::valid( secret ) ;
		if( !valid_id )
            addWarning( contents , line_number , "invalid base64 encoding in third field" , id_or_ip ) ;
        if( !valid_secret )
            addWarning( contents , line_number , "invalid base64 encoding in fourth field" ) ;
		if( !valid_id || !valid_secret )
			return ;
		type = "plain" ;
        id_or_ip = G::Xtext::encode( G::Base64::decode(id_or_ip) ) ;
        secret = G::Xtext::encode( G::Base64::decode(secret) ) ;
	}
	processLineImp( contents , line_number , G::Str::lower(side) , type , id_or_ip , secret ) ;
}

void GAuth::SecretsFile::processLineImp( Contents & contents ,
	unsigned int line_number , const std::string & side , const std::string & type ,
	const std::string & id_or_ip , const std::string & secret )
{
	G_ASSERT( !side.empty() && !type.empty() && !id_or_ip.empty() && !secret.empty() ) ;

	if( type == "plain" )
	{
		if( !G::Xtext::valid(id_or_ip) ) // (ip address ranges are valid xtext)
			addWarning( contents , line_number , "invalid xtext encoding in third field" , id_or_ip ) ;
		if( !G::Xtext::valid(secret) )
			addWarning( contents , line_number , "invalid xtext encoding in fourth field" ) ; // (new)
	}

	if( side == "server" )
	{
		// server-side
		std::string key = serverKey( type , id_or_ip ) ;
		Value value( secret , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(key,value)).second ;
		if( inserted )
			contents.m_types.insert( canonical(type) ) ;
		else
			addWarning( contents , line_number , "duplicate server secret" , key ) ;
	}
	else if( side == "client" )
	{
		// client-side
		const std::string & id = id_or_ip ;
		std::string key = clientKey( type ) ; // not including user id
		Value value( id + " " + secret , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(key,value)).second ;
		if( !inserted )
			addWarning( contents , line_number , "too many client secrets" , key ) ;
	}
	else
	{
		addWarning( contents , line_number , "invalid value in first field" , side ) ;
	}
}

void GAuth::SecretsFile::addWarning( Contents & contents , unsigned int line_number , const std::string & message_in , const std::string & more )
{
	std::string message = more.empty() ? message_in : ( message_in + ": [" + G::Str::printable(more) + "]" ) ;
	contents.m_warnings.push_back( Warnings::value_type(line_number,message) ) ;
}

void GAuth::SecretsFile::showWarnings( const Warnings & warnings , const G::Path & path , const std::string & debug_name )
{
	if( !warnings.empty() )
	{
		std::string prefix = path.basename() ;
		G_WARNING( "GAuth::SecretsFile::read: problems reading" << (debug_name.empty()?"":" ") << debug_name << " secrets file [" << path.str() << "]..." ) ;
		for( const auto & warning : warnings )
		{
			G_WARNING( "GAuth::SecretsFile::read: " << prefix << "(" << warning.first << "): " << warning.second ) ;
		}
	}
}

std::string GAuth::SecretsFile::canonical( const std::string & type_in )
{
	// (cram-md5, apop and login are for backwards compatibility -- new
	// code exects plain, md5, sha1, sha512 etc)
	std::string type = G::Str::lower( type_in ) ;
	if( type == "cram-md5" ) type = "md5" ;
	if( type == "apop" ) type = "md5" ;
	if( type == "login" ) type = "plain" ;
	return type ;
}

std::string GAuth::SecretsFile::serverKey( const std::string & type , const std::string & id_xtext )
{
	// eg. key -> value...
	// "server plain bob" -> "e+3Dmc2"
	// "server md5 bob" -> "xbase64x=="
	// "server none 192.168.0.0/24" -> "trustee"
	// "server none ::1/128" -> "trustee"
	return "server " + canonical(type) + " " + id_xtext ;
}

std::string GAuth::SecretsFile::clientKey( const std::string & type )
{
	// eg. key -> value...
	// "client plain" -> "alice secret+21"
	// "client md5" -> "alice xbase64x=="
	return "client " + canonical(type) ;
}

GAuth::Secret GAuth::SecretsFile::clientSecret( const std::string & type ) const
{
	reread() ;

	auto p = m_contents.m_map.find( clientKey(type) ) ;
	if( p == m_contents.m_map.end() )
	{
		return Secret::none() ;
	}
	else
	{
		std::string id_xtext = G::Str::head( (*p).second.s , " " ) ;
		std::string secret_encoded = G::Str::tail( (*p).second.s , " " ) ;
		return Secret( secret_encoded , canonical(type) , id_xtext , true , line((*p).second.n) ) ;
	}
}

GAuth::Secret GAuth::SecretsFile::serverSecret( const std::string & type , const std::string & id ) const
{
	if( id.empty() )
		return Secret::none() ;

	reread() ;

	auto p = m_contents.m_map.find( serverKey(type,G::Xtext::encode(id)) ) ;
	if( p == m_contents.m_map.end() )
	{
		return Secret::none( id ) ;
	}
	else
	{
		return Secret( (*p).second.s , canonical(type) , id , false , line((*p).second.n) ) ;
	}
}

std::pair<std::string,std::string> GAuth::SecretsFile::serverTrust( const std::string & address_range ) const
{
	reread() ; // (new)

	std::pair<std::string,std::string> result ;
	std::string type = "none" ;
	const std::string & id = address_range ; // the address-range lives in the id field
	auto p = m_contents.m_map.find( serverKey(type,G::Xtext::encode(id)) ) ;
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

bool GAuth::SecretsFile::contains( const std::string & type , const std::string & id ) const
{
	return id.empty() ?
		m_contents.m_types.find( canonical(type) ) != m_contents.m_types.end() :
		m_contents.m_map.find( serverKey(type,G::Xtext::encode(id)) ) != m_contents.m_map.end() ;
}

std::string GAuth::SecretsFile::line( unsigned int line_number )
{
	return "line " + G::Str::fromUInt(line_number) ;
}
