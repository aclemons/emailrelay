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
///
/// \file gsecretsfile.h
///

#ifndef G_AUTH_SECRETS_FILE_H
#define G_AUTH_SECRETS_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gdatetime.h"
#include "gsecret.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <utility>

namespace GAuth
{
	class SecretsFile ;
}

/// \class GAuth::SecretsFile
/// A class to read authentication secrets from file, used by GAuth::Secrets.
/// Updates to the file are detected automatically.
///
class GAuth::SecretsFile
{
public:
	G_EXCEPTION( Error , "invalid secrets file" ) ;

	static void check( const std::string & path ) ;
		///< Checks the given file. Logs warnings and throws an exception
		///< if there are any fatal errors.

	SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ,
		const std::string & server_type = std::string() ) ;
			///< Constructor to read "client" and "<server-type>" records
			///< from the named file. The server type defaults to "server".
			///< The filename path is optional; see valid().

	bool valid() const ;
		///< Returns true if the file path was supplied in the ctor.

	Secret clientSecret( const std::string & encoding_type ) const ;
		///< Returns the client id and secret for the given encoding-type.
		///< Returns the empty string if none.

	Secret serverSecret( const std::string & encoding_type , const std::string & id ) const ;
		///< Returns the server secret for the given id and encoding-type.
		///< Returns the empty string if none.

	std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const ;
		///< Returns a non-empty trustee name if the server trusts clients
		///< in the given address range, together with context information.

	std::string path() const ;
		///< Returns the file path, as supplied to the ctor.

	bool contains( const std::string & server_encoding_type ) const ;
		///< Returns true if the given server encoding-type is represented.

private:
	struct Value
	{
		Value(const std::string &s_,unsigned int n_):s(s_),n(n_) {}
		std::string s ;
		unsigned int n ;
	} ;
	typedef std::map<std::string,Value> Map ;
	typedef std::set<std::string> Set ;
	typedef std::pair<unsigned long,std::string> Warning ;
	typedef std::vector<Warning> Warnings ;
	struct Contents
	{
		Map m_map ;
		Set m_types ;
		Warnings m_warnings ;
	} ;

private:
	void read( const G::Path & , bool ) ;
	void reread() const ;
	void reread( int ) ;
	static void checkImp( const std::string & path , bool strict , const std::string & server_type ) ;
	static Contents readContents( const G::Path & , const std::string & , bool ) ;
	static Contents readContents( std::istream & , const std::string & , bool ) ;
	static void processLine( Contents & , const std::string & server_type ,
		unsigned int , const std::string & side , const std::string & , const std::string & , const std::string & , bool ) ;
	static void showWarnings( const Warnings & warnings , const G::Path & path , const std::string & debug_name = std::string() ) ;
	static void addWarning( Contents & , unsigned int , const std::string & , const std::string & ) ;
	static std::string canonical( const std::string & encoding_type ) ;
	static std::string serverKey( const std::string & , const std::string & ) ;
	static std::string clientKey( const std::string & ) ;
	static G::EpochTime readFileTime( const G::Path & ) ;
	static std::string line( unsigned int ) ;

private:
	G::Path m_path ;
	bool m_auto ;
	std::string m_debug_name ;
	std::string m_server_type ;
	bool m_valid ;
	Contents m_contents ;
	G::EpochTime m_file_time ;
	G::EpochTime m_check_time ;
} ;

#endif
