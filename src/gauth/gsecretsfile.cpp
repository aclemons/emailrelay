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
#include "gstringtoken.h"
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
	std::string line ;
	for( unsigned int line_number = 1U ; file.good() ; ++line_number )
	{
		G::Str::readLineFrom( file , "\n"_sv , line ) ;
		if( !file )
			break ;

		G::Str::trim( line , G::Str::ws() ) ;
		if( !line.empty() && line.at(0U) != '#' )
		{
			G::string_view line_sv( line ) ;
			G::StringTokenView t( line_sv , " \t"_sv ) ;
			G::string_view w1 = t() ;
			G::string_view w2 = (++t)() ;
			G::string_view w3 = (++t)() ;
			G::string_view w4 = (++t)() ;
			if( t.valid() )
			{
				// 0=<client-server> 1=<encoding-type> 2=<userid-or-ipaddress> 3=<secret-or-verifier-hint>
				processLine( contents , line_number , w1 , w2 , w3 , w4 ) ;
			}
			else
			{
				addWarning( contents , line_number , "too few fields"_sv ) ;
			}
		}
	}
	return contents ;
}

void GAuth::SecretsFile::processLine( Contents & contents ,
	unsigned int line_number , G::string_view side , G::string_view type ,
	G::string_view id_or_ip , G::string_view secret )
{
	if( G::Str::imatch( type , "plain:b"_sv ) )
	{
		// for now just re-encode to xtext -- TODO rework secrets-file encodings
        bool valid_id = G::Base64::valid( id_or_ip ) ;
		bool valid_secret = G::Base64::valid( secret ) ;
		if( !valid_id )
            addWarning( contents , line_number , "invalid base64 encoding in third field"_sv , id_or_ip ) ;
        if( !valid_secret )
            addWarning( contents , line_number , "invalid base64 encoding in fourth field"_sv ) ;
		if( !valid_id || !valid_secret )
			return ;
        std::string xtext_id_or_ip = G::Xtext::encode( G::Base64::decode(id_or_ip) ) ;
        std::string xtext_secret = G::Xtext::encode( G::Base64::decode(secret) ) ;
		processLineImp( contents , line_number , side , "plain"_sv , xtext_id_or_ip , xtext_secret ) ;
	}
	else
	{
		processLineImp( contents , line_number , side , type , id_or_ip , secret ) ;
	}
}

void GAuth::SecretsFile::processLineImp( Contents & contents ,
	unsigned int line_number , G::string_view side , G::string_view type ,
	G::string_view id_or_ip , G::string_view secret )
{
	G_ASSERT( !side.empty() && !type.empty() && !id_or_ip.empty() && !secret.empty() ) ;

	if( type == "plain"_sv )
	{
		if( !G::Xtext::valid(id_or_ip) ) // (ip address ranges are valid xtext)
			addWarning( contents , line_number , "invalid xtext encoding in third field"_sv , id_or_ip ) ;
		if( !G::Xtext::valid(secret) )
			addWarning( contents , line_number , "invalid xtext encoding in fourth field"_sv ) ;
	}

	if( G::Str::imatch( side , "server"_sv ) )
	{
		// server-side
		Value server_value( secret , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(serverKey(type,id_or_ip),server_value)).second ;
		if( inserted )
			contents.m_types.insert( canonical(type) ) ;
		else
			addWarning( contents , line_number , "duplicate server secret"_sv , serverKey(type,id_or_ip) ) ;
	}
	else if( G::Str::imatch( side , "client"_sv ) )
	{
		// client-side
		Value client_value( clientValue(id_or_ip,secret) , line_number ) ;
		bool inserted = contents.m_map.insert(std::make_pair(clientKey(type),client_value)).second ;
		if( !inserted )
			addWarning( contents , line_number , "too many client secrets"_sv , clientKey(type) ) ;
	}
	else
	{
		addWarning( contents , line_number , "invalid value in first field"_sv , side ) ;
	}
}

void GAuth::SecretsFile::addWarning( Contents & contents , unsigned int line_number , G::string_view message_in , G::string_view more )
{
	std::string message( message_in.data() , message_in.size() ) ;
	if( !more.empty() )
		message.append(": [",3U).append(G::Str::printable(more)).append(1U,']') ;
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

std::string GAuth::SecretsFile::canonical( G::string_view type )
{
	// (for backwards compatibility -- new code exects plain, md5, sha1, sha512 etc)
	if( G::Str::imatch( type , "cram-md5"_sv ) ) return "md5" ;
	if( G::Str::imatch( type , "apop"_sv ) ) return "md5" ;
	if( G::Str::imatch( type , "login"_sv ) ) return "plain" ;

	return G::Str::lower( type ) ;
}

std::string GAuth::SecretsFile::serverKey( G::string_view type , G::string_view id_xtext )
{
	// eg. key -> value...
	// "server plain bob" -> "e+3Dmc2"
	// "server md5 bob" -> "xbase64x=="
	// "server none 192.168.0.0/24" -> "trustee"
	// "server none ::1/128" -> "trustee"
	return std::string("server ",7U).append(canonical(type)).append(1U,' ').append(id_xtext.data(),id_xtext.size()) ;
}

std::string GAuth::SecretsFile::clientKey( G::string_view type )
{
	// eg. key -> value...
	// "client plain" -> "alice secret+21"
	// "client md5" -> "alice xbase64x=="
	return std::string("client ",7U).append(canonical(type)) ;
}

std::string GAuth::SecretsFile::clientValue( G::string_view id , G::string_view secret )
{
	return G::sv_to_string(id).append(1U,' ').append(secret.data(),secret.size()) ;
}

GAuth::Secret GAuth::SecretsFile::clientSecret( G::string_view type ) const
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

GAuth::Secret GAuth::SecretsFile::serverSecret( G::string_view type , G::string_view id ) const
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
	reread() ;

	std::pair<std::string,std::string> result ;
	G::string_view type = "none"_sv ;
	const std::string & id = address_range ; // the address-range lives in the id field
	std::string xid = G::Xtext::encode( id ) ;
	auto p = m_contents.m_map.find( serverKey(type,{xid.data(),xid.size()}) ) ;
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

bool GAuth::SecretsFile::contains( G::string_view type , G::string_view id ) const
{
	return id.empty() ?
		m_contents.m_types.find( canonical(type) ) != m_contents.m_types.end() :
		m_contents.m_map.find( serverKey(type,G::Xtext::encode(id)) ) != m_contents.m_map.end() ;
}

std::string GAuth::SecretsFile::line( unsigned int line_number )
{
	return "line " + G::Str::fromUInt(line_number) ;
}
