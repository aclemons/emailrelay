//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsecrets.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gsecrets.h"
#include "groot.h"
#include "gxtext.h"
#include "gstr.h"
#include <fstream>
#include <utility> // std::pair

// Class: GSmtp::SecretsImp
// Description: A private pimple-pattern implemtation class used by GSmtp::Secrets.
//
class GSmtp::SecretsImp 
{
public:
	SecretsImp( const G::Path & path , bool auto_ , const std::string & name , const std::string & type ) ;
	~SecretsImp() ;
	bool valid() const ;
	std::string id( const std::string & mechanism ) const ;
	std::string secret( const std::string & mechanism ) const ;
	std::string secret(  const std::string & mechanism , const std::string & id ) const ;

private:
	void process( std::string mechanism , std::string side , std::string id , std::string secret ) ;
	void read( std::istream & ) ;

private:
	typedef std::map<std::string,std::string> Map ;
	G::Path m_path ;
	bool m_auto ;
	std::string m_debug_name ;
	std::string m_server_type ;
	bool m_valid ;
	Map m_map ;
} ;

// ===

GSmtp::Secrets::Secrets( const std::string & path , const std::string & name , const std::string & type ) :
	m_imp(NULL)
{
	const bool is_auto = true ;
	m_imp =  new SecretsImp( path , is_auto , name , type ) ;
}

GSmtp::Secrets::~Secrets()
{
	delete m_imp ;
}

bool GSmtp::Secrets::valid() const
{
	return m_imp->valid() ;
}

std::string GSmtp::Secrets::id( const std::string & mechanism ) const
{
	return m_imp->id( mechanism ) ;
}

std::string GSmtp::Secrets::secret( const std::string & mechanism ) const
{
	return m_imp->secret( mechanism ) ;
}

std::string GSmtp::Secrets::secret(  const std::string & mechanism , const std::string & id ) const
{
	return m_imp->secret( mechanism , id ) ;
}

// ===

GSmtp::SecretsImp::SecretsImp( const G::Path & path , bool auto_ , const std::string & debug_name ,
	const std::string & server_type ) :
		m_path(path) ,
		m_auto(auto_) ,
		m_debug_name(debug_name) ,
		m_server_type(server_type)
{
	if( m_server_type.empty() ) 
		m_server_type = "server" ;

	G_DEBUG( "GSmtp::Secrets: " << m_debug_name << ": \"" << path << "\"" ) ;

	if( path.str().empty() )
	{
		m_valid = false ;
	}
	else
	{
		G::Root claim_root ;
		std::ifstream file( path.str().c_str() ) ;
		if( !file.good() )
			throw Secrets::OpenError( path.str() ) ;

		read( file ) ;
		m_valid = m_map.size() != 0U ;
	}

	const bool debug = false ; // security hole if true
	if( debug )
	{
		for( Map::iterator p = m_map.begin() ; p != m_map.end() ; ++p )
			G_DEBUG( "GSmtp::Secrets::ctor: \"" << (*p).first << "\", \"" << (*p).second << "\"" ) ;
	}
}

void GSmtp::SecretsImp::read( std::istream & file )
{
	while( file.good() )
	{
		std::string line = G::Str::readLineFrom( file ) ;
		const std::string ws = " \t" ;
		G::Str::trim( line , ws ) ;
		if( !line.empty() && line.at(0U) != '#' )
		{
			G::StringArray word_array ;
			G::Str::splitIntoTokens( line , word_array , ws ) ;
			if( word_array.size() == 4U )
				process( word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
		}
	}
}

void GSmtp::SecretsImp::process( std::string mechanism , std::string side , std::string id , std::string secret )
{
	G::Str::toUpper( mechanism ) ;
	if( side.at(0U) == m_server_type.at(0U) )
	{
		// server-side
		std::string key = mechanism + ":" + id ;
		std::string value = secret ;
		m_map.insert( std::make_pair(key,value) ) ;
	}
	else if( side.at(0U) == 'c' || side.at(0U) == 'C' )
	{
		// client-side -- no userid in the key since only one secret
		std::string key = mechanism + " client" ;
		std::string value = id + " " + secret ;
		m_map.insert( std::make_pair(key,value) ) ;
	}
}

GSmtp::SecretsImp::~SecretsImp()
{
}

bool GSmtp::SecretsImp::valid() const
{
	return m_valid ;
}

std::string GSmtp::SecretsImp::id( const std::string & mechanism ) const
{
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	std::string result ;
	if( p != m_map.end() && (*p).second.find(" ") != std::string::npos )
		result = G::Xtext::decode( (*p).second.substr(0U,(*p).second.find(" ")) ) ;
	G_DEBUG( "GSmtp::Secrets::id: " << m_debug_name << ": \"" << mechanism << "\"" ) ;
	return result ;
}

std::string GSmtp::SecretsImp::secret( const std::string & mechanism ) const
{
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	if( p == m_map.end() || (*p).second.find(" ") == std::string::npos )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second.substr((*p).second.find(" ")+1U) ) ;
}

std::string GSmtp::SecretsImp::secret(  const std::string & mechanism , const std::string & id ) const
{
	Map::const_iterator p = m_map.find( mechanism+":"+G::Xtext::encode(id) ) ;
	if( p == m_map.end() )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second ) ;
}

