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
/// \file gsecretsfile.h
///

#ifndef G_AUTH_SECRETS_FILE_H
#define G_AUTH_SECRETS_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gdatetime.h"
#include "gstringview.h"
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

//| \class GAuth::SecretsFile
/// A class to read authentication secrets from file, used by GAuth::Secrets.
/// Updates to the file are detected automatically.
///
class GAuth::SecretsFile
{
public:
	G_EXCEPTION( Error , tx("invalid secrets file") ) ;

	static void check( const std::string & path ) ;
		///< Checks the given file. Logs warnings and throws an exception
		///< if there are any fatal errors.

	SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ) ;
		///< Constructor to read "client" and "server" records from
		///< the named file. The path is optional; see valid().

	bool valid() const ;
		///< Returns true if the file path was supplied in the ctor.

	Secret clientSecret( G::string_view type ) const ;
		///< Returns the client id and secret for the given type.
		///< Returns the empty string if none.

	Secret serverSecret( G::string_view type , G::string_view id ) const ;
		///< Returns the server secret for the given id and type.
		///< Returns the empty string if none.

	std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const ;
		///< Returns a non-empty trustee name if the server trusts remote
		///< clients in the given address range, together with context
		///< information.

	std::string path() const ;
		///< Returns the file path, as supplied to the ctor.

	bool contains( G::string_view type , G::string_view id = {} ) const ;
		///< Returns true if a server secret of the given type
		///< is available for the particular user or for any user
		///< if defaulted.

private:
	using Map = std::map<std::string,Secret> ;
	using Set = std::set<std::string> ;
	using Warning = std::pair<unsigned long,std::string> ;
	using Warnings = std::vector<Warning> ;
	using TrustMap = std::map<std::string,std::pair<std::string,int>> ;
	struct Contents
	{
		Map m_map ;
		Set m_types ;
		TrustMap m_trust_map ;
		Warnings m_warnings ;
	} ;

private:
	void read( const G::Path & ) ;
	void reread() const ;
	void reread( int ) ;
	static Contents readContents( const G::Path & ) ;
	static Contents readContents( std::istream & ) ;
	static void processLine( Contents & ,
		unsigned int , G::string_view side , G::string_view , G::string_view , G::string_view ) ;
	static void showWarnings( const Warnings & warnings , const G::Path & path , const std::string & debug_name = {} ) ;
	static void addWarning( Contents & , unsigned int , G::string_view , G::string_view = {} ) ;
	static G::string_view canonicalView( G::string_view encoding_type ) ;
	static std::string serverKey( const std::string & , const std::string & ) ;
	static std::string serverKey( G::string_view , G::string_view ) ;
	static std::string clientKey( G::string_view ) ;
	static G::SystemTime readFileTime( const G::Path & ) ;
	static std::string lineContext( unsigned int ) ;

private:
	G::Path m_path ;
	bool m_auto ;
	std::string m_debug_name ;
	bool m_valid ;
	Contents m_contents ;
	G::SystemTime m_file_time ;
	G::SystemTime m_check_time ;
} ;

#endif
