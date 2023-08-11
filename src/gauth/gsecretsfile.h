//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <tuple>

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
	G_EXCEPTION( OpenError , tx("cannot read secrets file") ) ;

	static void check( const std::string & path , bool with_warnings ) ;
		///< Checks the given file. Logs errors and optionally warnings and then
		///< throws an exception if there were any errors.

	SecretsFile( const G::Path & path , bool auto_reread , const std::string & debug_name ) ;
		///< Constructor to read "client" and "server" records from
		///< the named file. The path is optional; see valid().

	bool valid() const ;
		///< Returns true if the file path was supplied in the ctor.

	bool containsClientSelector( G::string_view selector ) const ;
		///< Returns true if the given client account selector is valid.
		///< A special "plain:b = = <selector>" line can make the
		///< selector valid without creating a client secret.

	bool containsClientSecret( G::string_view selector ) const ;
		///< Returns true if a client secret is available with
		///< the given account selector.

	Secret clientSecret( G::string_view type , G::string_view selector = {} ) const ;
		///< Returns the client id and secret for the given type.
		///< Returns an in-valid() Secret if no matching client
		///< secret having a non-empty id.

	bool containsServerSecret( G::string_view type , G::string_view id = {} ) const ;
		///< Returns true if a server secret of the given type
		///< is available for the particular user or for any user
		///< if defaulted.

	Secret serverSecret( G::string_view type , G::string_view id ) const ;
		///< Returns the server secret for the given id and type.
		///< Returns an in-valid() Secret if no matching server
		///< secret.

	std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const ;
		///< Returns a non-empty trustee name if the server trusts remote
		///< clients in the given address range, together with context
		///< information.

	std::string path() const ;
		///< Returns the file path, as supplied to the ctor.

private:
	using MapOfSecrets = std::map<std::string,Secret> ;
	using MapOfInt = std::map<std::string,unsigned int> ;
	using SetOfStrings = std::set<std::string> ;
	using Diagnostic = std::tuple<bool,unsigned long,std::string> ; // is-error,line-number,text
	using Diagnostics = std::vector<Diagnostic> ;
	using TrustMap = std::map<std::string,std::pair<std::string,int>> ;
	struct Contents
	{
		MapOfSecrets m_map ;
		SetOfStrings m_types ; // server
		MapOfInt m_selectors ; // client -- zero integer if only an empty id
		TrustMap m_trust_map ;
		Diagnostics m_diagnostics ;
		std::size_t m_errors {0U} ;
	} ;

private:
	void read( const G::Path & ) ;
	void reread() const ;
	void reread( int ) ;
	bool containsClientSecretImp( G::string_view , bool ) const ;
	static Contents readContents( const G::Path & ) ;
	static Contents readContents( std::istream & ) ;
	static void processLine( Contents & ,
		unsigned int , G::string_view side , G::string_view , G::string_view ,
		G::string_view , G::string_view ) ;
	static void showDiagnostics( const Contents & c , const G::Path & , const std::string & debug_name , bool with_warnings ) ;
	static void addWarning( Contents & , unsigned int , G::string_view , G::string_view = {} ) ;
	static void addError( Contents & , unsigned int , G::string_view , G::string_view = {} ) ;
	static std::string join( G::string_view , G::string_view ) ;
	static G::string_view canonicalView( G::string_view encoding_type ) ;
	static std::string serverKey( const std::string & , const std::string & ) ;
	static std::string serverKey( G::string_view , G::string_view ) ;
	static std::string clientKey( G::string_view , G::string_view ) ;
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
