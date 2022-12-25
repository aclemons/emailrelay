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
#include <fstream>
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
		G::Str::readLine( file , line ) ;
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
				processLine( contents , line_number , w1 , w2 , w3 , w4 ) ;
			else
				addWarning( contents , line_number , "too few fields"_sv ) ;
		}
	}
	return contents ;
}

void GAuth::SecretsFile::processLine( Contents & contents , unsigned int line_number ,
	G::string_view side , G::string_view type_in , G::string_view id , G::string_view secret )
{
	G::string_view type = canonicalView( G::Str::headView( type_in , ":" , false ) ) ;
	G::string_view type_decoration = G::Str::tailView( type_in , ":" ) ;
	bool is_server_side = G::Str::imatch( side , "server"_sv ) ;
	bool is_client_side = G::Str::imatch( side , "client"_sv ) ;

	if( is_server_side && G::Str::imatch( type , "none"_sv ) )
	{
		G::string_view ip_range = id ;
		G::string_view keyword = secret ;
		bool inserted = contents.m_trust_map.insert( {G::sv_to_string(ip_range),{G::sv_to_string(keyword),line_number}} ).second ;
		if( !inserted )
			addWarning( contents , line_number , "duplicate server trust address"_sv ) ;
	}
	else
	{
		G::string_view id_encoding ;
		G::string_view secret_encoding ;
		G::string_view hash_function ;
		if( G::Str::imatch( type.substr(0U,5U) , "plain" ) )
		{
			id_encoding = G::Str::imatch( type_decoration , "b" ) ? "base64"_sv : "xtext"_sv ; // should also allow plain:xb etc
			secret_encoding = id_encoding ;
			//hash_function = "" ;
		}
		else if( G::Str::imatch( type , "md5"_sv ) && Secret::isDotted(secret) )
		{
			id_encoding = "xtext"_sv ;
			secret_encoding = "dotted"_sv ;
			hash_function = "md5"_sv ;
		}
		else
		{
			id_encoding = "xtext"_sv ;
			secret_encoding = "base64"_sv ;
			hash_function = type ;
		}

		if( is_server_side )
		{
			std::string key = serverKey( type , Secret::decode({id,id_encoding}) ) ;
			Secret secret_obj( {id,id_encoding} , {secret,secret_encoding} , hash_function , lineContext(line_number) ) ;
			bool inserted = contents.m_map.insert( {key,secret_obj} ).second ;
			if( inserted )
				contents.m_types.insert( G::Str::lower(type) ) ;
			else
				addWarning( contents , line_number , "duplicate server secret"_sv ) ;
		}
		else if( is_client_side )
		{
			std::string key = clientKey( type ) ;
			Secret secret_obj( {id,id_encoding} , {secret,secret_encoding} , hash_function , lineContext(line_number) ) ;
			bool inserted = contents.m_map.insert( {key,secret_obj} ).second ;
			if( !inserted )
				addWarning( contents , line_number , "duplicate client secret"_sv ) ;
		}
		else
		{
			addWarning( contents , line_number , "invalid value in first field"_sv , side ) ;
		}
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
		G_WARNING( "GAuth::SecretsFile::read: problems reading" << (debug_name.empty()?"":" ") << debug_name << " "
			"secrets file [" << path.str() << "]..." ) ;

		std::string prefix = path.basename() ;
		for( const auto & warning : warnings )
		{
			G_WARNING( "GAuth::SecretsFile::read: " << prefix << "(" << warning.first << "): " << warning.second ) ;
		}
	}
}

G::string_view GAuth::SecretsFile::canonicalView( G::string_view type )
{
	// (for backwards compatibility -- new code exects plain, md5, sha1, sha512 etc)
	if( G::Str::imatch( type , "cram-md5"_sv ) ) return "md5" ;
	if( G::Str::imatch( type , "apop"_sv ) ) return "md5" ;
	if( G::Str::imatch( type , "login"_sv ) ) return "plain" ;
	return type ;
}

std::string GAuth::SecretsFile::serverKey( const std::string & type , const std::string & id_decoded )
{
	return std::string("server ",7U).append(G::Str::lower(type)).append(1U,' ').append(id_decoded) ;
}

std::string GAuth::SecretsFile::serverKey( G::string_view type , G::string_view id_decoded )
{
	return serverKey( G::sv_to_string(type) , G::sv_to_string(id_decoded) ) ;
}

std::string GAuth::SecretsFile::clientKey( G::string_view type )
{
	return std::string("client ",7U).append(G::Str::lower(type)) ;
}

GAuth::Secret GAuth::SecretsFile::clientSecret( G::string_view type ) const
{
	reread() ;

	auto p = m_contents.m_map.find( clientKey(type) ) ;
	if( p == m_contents.m_map.end() )
		return Secret::none() ;
	else
		return (*p).second ;
}

GAuth::Secret GAuth::SecretsFile::serverSecret( G::string_view type , G::string_view id ) const
{
	if( id.empty() )
		return Secret::none() ;

	reread() ;

	auto p = m_contents.m_map.find( serverKey(type,id) ) ;
	if( p == m_contents.m_map.end() )
		return Secret::none() ;
	else
		return (*p).second ;
}

std::pair<std::string,std::string> GAuth::SecretsFile::serverTrust( const std::string & address_range ) const
{
	reread() ;

	std::pair<std::string,std::string> result ;
	auto p = m_contents.m_trust_map.find( address_range ) ;
	if( p != m_contents.m_trust_map.end() )
	{
		result.first = (*p).second.first ;
		result.second = lineContext( (*p).second.second ) ;
	}
	return result ;
}

#ifndef G_LIB_SMALL
std::string GAuth::SecretsFile::path() const
{
	return m_path.str() ;
}
#endif

bool GAuth::SecretsFile::contains( G::string_view type , G::string_view id_decoded ) const
{
	return id_decoded.empty() ?
		m_contents.m_types.find( G::Str::lower(type) ) != m_contents.m_types.end() :
		m_contents.m_map.find( serverKey(type,id_decoded) ) != m_contents.m_map.end() ;
}

std::string GAuth::SecretsFile::lineContext( unsigned int line_number )
{
	return std::string("line ",5U).append(G::Str::fromUInt(line_number)) ;
}

