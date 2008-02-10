//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsecrets_full.cpp
//

#include "gdef.h"
#include "gsmtp.h"
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
#include <utility> // std::pair
#include <sstream>

/// \class GSmtp::SecretsImp
/// A private pimple-pattern implentation class used by GSmtp::Secrets.
/// 
class GSmtp::SecretsImp 
{
public:
	SecretsImp( const G::Path & path , bool auto_ , const std::string & name , const std::string & type ) ;
	~SecretsImp() ;
	bool valid() const ;
	std::string id( const std::string & mechanism ) const ;
	std::string secret( const std::string & mechanism ) const ;
	std::string secret(  const std::string & mechanism , const std::string & id ) const ;
	std::string path() const ;
	bool contains( const std::string & mechanism ) const ;

private:
	void process( std::string mechanism , std::string side , std::string id , std::string secret ) ;
	void read( const G::Path & ) ;
	unsigned int read( std::istream & ) ;
	void reread() const ;
	void reread(int) ;
	static G::DateTime::EpochTime readFileTime( const G::Path & ) ;

private:
	typedef std::map<std::string,std::string> Map ;
	typedef std::set<std::string> Set ;
	G::Path m_path ;
	bool m_auto ;
	std::string m_debug_name ;
	std::string m_server_type ;
	bool m_valid ;
	Map m_map ;
	Set m_set ;
	G::DateTime::EpochTime m_file_time ;
	G::DateTime::EpochTime m_check_time ;
} ;

// ===

GSmtp::Secrets::Secrets( const std::string & path , const std::string & name , const std::string & type ) :
	m_imp(new SecretsImp(path,true,name,type))
{
}

GSmtp::Secrets::Secrets() :
	m_imp(new SecretsImp(std::string(),true,std::string(),std::string()))
{
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

bool GSmtp::Secrets::contains( const std::string & mechanism ) const
{
	return m_imp->contains( mechanism ) ;
}

// ===

GSmtp::SecretsImp::SecretsImp( const G::Path & path , bool auto_ , const std::string & debug_name ,
	const std::string & server_type ) :
		m_path(path) ,
		m_auto(auto_) ,
		m_debug_name(debug_name) ,
		m_server_type(server_type) ,
		m_file_time(0) ,
		m_check_time(G::DateTime::now())
{
	m_server_type = m_server_type.empty() ? std::string("server") : m_server_type ;
	G_DEBUG( "GSmtp::Secrets: " << m_debug_name << ": \"" << path << "\"" ) ;
	m_valid = ! path.str().empty() ;
	if( m_valid )
		read( path ) ;
}

bool GSmtp::SecretsImp::valid() const
{
	return m_valid ;
}

void GSmtp::SecretsImp::reread() const
{
	(const_cast<SecretsImp*>(this))->reread(0) ;
}

void GSmtp::SecretsImp::reread( int )
{
	G_DEBUG( "GSmtp::SecretsImp::reread" ) ;
	if( m_auto )
	{
		G::DateTime::EpochTime now = G::DateTime::now() ;
		G_DEBUG( "GSmtp::SecretsImp::reread: file time checked at " << m_check_time << ": now " << now ) ;
		if( now != m_check_time ) // at most once a second
		{
			m_check_time = now ;
			G::DateTime::EpochTime t = readFileTime( m_path ) ;
			G_DEBUG( "GSmtp::SecretsImp::reread: current file time " << t << ": saved file time " << m_file_time ) ;
			if( t != m_file_time )
			{
				G_LOG_S( "GSmtp::Secrets: re-reading secrets file: " << m_path ) ;
				(void) read( m_path ) ;
			}
		}
	}
}

void GSmtp::SecretsImp::read( const G::Path & path )
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
	G_DEBUG( "GSmtp::SecretsImp::read: processed " << (count-1) << " records" ) ;
}

G::DateTime::EpochTime GSmtp::SecretsImp::readFileTime( const G::Path & path )
{
	G::Root claim_root ;
	return G::File::time( path ) ;
}

unsigned int GSmtp::SecretsImp::read( std::istream & file )
{
	unsigned int count = 0U ;
	for( unsigned int line_number = 1U ; file.good() ; ++line_number )
	{
		std::string line = G::Str::readLineFrom( file ) ;
		if( !file ) 
			break ;

		const std::string ws = " \t" ;
		G::Str::trim( line , ws ) ;
		if( !line.empty() && line.at(0U) != '#' )
		{
			G::StringArray word_array ;
			G::Str::splitIntoTokens( line , word_array , ws ) ;
			if( word_array.size() > 4U )
			{
				G_WARNING( "GSmtp::SecretsImp::read: ignoring extra fields on line " 
					<< line_number << " of secrets file" ) ;
			}
			if( word_array.size() >= 4U )
			{
				process( word_array[0U] , word_array[1U] , word_array[2U] , word_array[3U] ) ;
				count++ ;
			}
			else
			{
				G_WARNING( "GSmtp::SecretsImp::read: ignoring line " 
					<< line_number << " of secrets file: too few fields" ) ;
			}
		}
	}
	return count ;
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
		m_set.insert( mechanism ) ;
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

std::string GSmtp::SecretsImp::id( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	std::string result ;
	if( p != m_map.end() && (*p).second.find(" ") != std::string::npos )
		result = G::Xtext::decode( (*p).second.substr(0U,(*p).second.find(" ")) ) ;
	G_DEBUG( "GSmtp::Secrets::id: " << m_debug_name << ": \"" << mechanism << "\"" ) ;
	return result ;
}

std::string GSmtp::SecretsImp::secret( const std::string & mechanism ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+" client" ) ;
	if( p == m_map.end() || (*p).second.find(" ") == std::string::npos )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second.substr((*p).second.find(" ")+1U) ) ;
}

std::string GSmtp::SecretsImp::secret(  const std::string & mechanism , const std::string & id ) const
{
	reread() ;
	Map::const_iterator p = m_map.find( mechanism+":"+G::Xtext::encode(id) ) ;
	if( p == m_map.end() )
		return std::string() ;
	else
		return G::Xtext::decode( (*p).second ) ;
}

std::string GSmtp::SecretsImp::path() const
{
	return m_path.str() ;
}

bool GSmtp::SecretsImp::contains( const std::string & mechanism ) const
{
	return m_set.find(mechanism) != m_set.end() ;
}

