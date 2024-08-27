//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsecrets.h
///

#ifndef G_AUTH_SECRETS_H
#define G_AUTH_SECRETS_H

#include "gdef.h"
#include "gsaslserversecrets.h"
#include "gsaslclientsecrets.h"
#include "gsecretsfile.h"
#include "gexception.h"
#include "gpath.h"
#include "gstringview.h"
#include <memory>
#include <utility>
#include <string>

namespace GAuth
{
	class Secrets ;
	class SecretsFileClient ;
	class SecretsFileServer ;
}

//| \class GAuth::Secrets
/// Provides factory functions for client and server secrets objects.
/// The implementation is based on GAuth::SecretsFile.
///
class GAuth::Secrets
{
public:
	G_EXCEPTION( ClientAccountError , tx("invalid client account details") )

	static void check( const std::string & client , const std::string & server , const std::string & pop ) ;
		///< Checks the given secret sources. Logs warnings and throws
		///< an exception if there are any fatal errors.

	static std::unique_ptr<SaslServerSecrets> newServerSecrets( const std::string & spec ,
		const std::string & log_name ) ;
			///< Factory function for server secrets. The spec is empty or
			///< a secrets file path or "/pam" or "pam:". The 'log-name' is
			///< used in log and error messages. Returns an in-valid() object
			///< if the spec is empty. Throws on error.

	static std::unique_ptr<SaslClientSecrets> newClientSecrets( const std::string & spec ,
		const std::string & log_name ) ;
			///< Factory function for client secrets. The spec is empty or a
			///< secrets file path or "plain:<base64-user-id>:<base64-pwd>".
			///< The 'log-name' is used in log and error messages. Returns
			///< an in-valid() object if the spec is empty. Throws on error.

public:
	Secrets() = delete ;
} ;

//| \class GAuth::SecretsFileClient
/// A thin adapter between GAuth::SecretsFile and GAuth::SaslClientSecrets
/// returned by GAuth::Secrets::newClientSecrets().
///
class GAuth::SecretsFileClient : public GAuth::SaslClientSecrets
{
public:
	SecretsFileClient( const std::string & path_spec , const std::string & log_name ) ;
		///< Constructor. See GAuth::Secrets::newClientSecrets().

	~SecretsFileClient() override ;
		///< Destructor.

public:
	SecretsFileClient( const SecretsFileClient & ) = delete ;
	SecretsFileClient( SecretsFileClient && ) = delete ;
	SecretsFileClient & operator=( const SecretsFileClient & ) = delete ;
	SecretsFileClient & operator=( SecretsFileClient && ) = delete ;

private: // overrides
	bool validSelector( std::string_view selector ) const override ;
	bool mustAuthenticate( std::string_view selector ) const override ;
	Secret clientSecret( std::string_view type , std::string_view selector ) const override ;

private:
	bool m_id_pwd ; // first
	std::string m_id ;
	std::string m_pwd ;
	SecretsFile m_file ;
} ;

//| \class GAuth::SecretsFileServer
/// A thin adapter between GAuth::SecretsFile and GAuth::SaslServerSecrets
/// returned by GAuth::Secrets::newServerSecrets().
///
class GAuth::SecretsFileServer : public GAuth::SaslServerSecrets
{
public:
	SecretsFileServer( const std::string & path , const std::string & log_name ) ;
		///< Constructor. See GAuth::Secrets::newServerSecrets().

	~SecretsFileServer() override ;
		///< Destructor.

public:
	SecretsFileServer( const SecretsFileServer & ) = delete ;
	SecretsFileServer( SecretsFileServer && ) = delete ;
	SecretsFileServer & operator=( const SecretsFileServer & ) = delete ;
	SecretsFileServer & operator=( SecretsFileServer && ) = delete ;

private: // overrides
	bool valid() const override ;
	Secret serverSecret( std::string_view type , std::string_view id ) const override ;
	std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const override ;
	std::string source() const override ;
	bool contains( std::string_view type , std::string_view id ) const override ;

private:
	bool m_pam ; // first
	SecretsFile m_file ;
} ;

#endif
