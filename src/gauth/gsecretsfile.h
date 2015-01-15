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
///
/// \file gsecretsfile.h
///

#ifndef G_AUTH_SECRETS_FILE_H
#define G_AUTH_SECRETS_FILE_H

#include "gdef.h"
#include "gauth.h"
#include "gpath.h"
#include "gdatetime.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

/// \namespace GAuth
namespace GAuth
{
	class SecretsFile ;
}

/// \class GAuth::SecretsFile
/// A class to read authentication secrets from file, used by GAuth::Secrets.
///
class GAuth::SecretsFile
{
public:
	SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name , const std::string & server_type = std::string() ) ;
		///< Constructor to read "client" and "<server-type>" records from the 
		///< named file. The server type defaults to "server". The filename path 
		///< is optional; see valid().

	~SecretsFile() ;
		///< Destructor.

	bool valid() const ;
		///< Returns true if the file path was supplied in the ctor.

	std::string id( const std::string & mechanism ) const ;
		///< Returns the client id for the given mechanism.
		///< Returns the empty string if none.

	std::string secret( const std::string & mechanism ) const ;
		///< Returns the client secret for the given mechanism.
		///< Returns the empty string if none.

	std::string secret( const std::string & mechanism , const std::string & id ) const ;
		///< Returns the server secret for the given id and mechanism.
		///< Returns the empty string if none.

	std::string path() const ;
		///< Returns the file path, as supplied to the ctor.

	bool contains( const std::string & mechanism ) const ;
		///< Returns true if the given mechanism is represented.

private:
	std::string process( std::string side , std::string mechanism , std::string id , std::string secret ) ;
	static std::string serverKey( const std::string & , const std::string & ) ;
	static std::string clientKey( const std::string & ) ;
	void read( const G::Path & ) ;
	void read( std::istream & ) ;
	void reread() const ;
	void reread(int) ;
	static G::DateTime::EpochTime readFileTime( const G::Path & ) ;

private:
	typedef std::map<std::string,std::string> Map ;
	typedef std::set<std::string> Set ;
	typedef std::pair<unsigned long,std::string> Warning ;
	typedef std::vector<Warning> Warnings ;
	G::Path m_path ;
	bool m_auto ;
	std::string m_debug_name ;
	std::string m_server_type ;
	bool m_valid ;
	Map m_map ;
	Set m_set ;
	Warnings m_warnings ;
	G::DateTime::EpochTime m_file_time ;
	G::DateTime::EpochTime m_check_time ;
} ;

#endif
