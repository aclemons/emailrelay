//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gpath.h"
#include "gexception.h"
#include "gsaslserversecrets.h"
#include "gsaslclientsecrets.h"

namespace GAuth
{
	class Secrets ;
	class SecretsFile ;
}

//| \class GAuth::Secrets
/// A simple interface for a store of secrets used in authentication.
/// The default implementation uses a flat file.
///
class GAuth::Secrets : public SaslClientSecrets , public SaslServerSecrets
{
public:
	G_EXCEPTION( OpenError , "cannot read secrets file" ) ;

	static void check( const std::string & , const std::string & , const std::string & ) ;
		///< Checks the given secret sources. Logs warnings and throws
		///< an exception if there are any fatal errors.

	Secrets( const std::string & source_storage_path , const std::string & debug_name ,
		const std::string & server_type = std::string() ) ;
			///< Constructor. The connection string is a secrets file path
			///< or "/pam".
			///<
			///< The 'debug-name' is used in log and error messages to
			///< identify the repository.
			///<
			///< The 'server-type' parameter can be used to select
			///< a different set of server-side authentication records
			///< that may be stored in the same repository such as
			///< "smtp" or "pop". The default is "server".
			///<
			///< Throws on error, although an empty path is not
			///< considered an error: see valid().

	Secrets() ;
		///< Default constructor for an in-valid(), empty-path object.

	bool valid() const override ;
		///< Override from GAuth::Valid virtual base.

	Secret serverSecret( const std::string & mechanism , const std::string & id ) const override ;
		///< Override from GAuth::SaslServerSecrets.

	bool contains( const std::string & mechanism ) const override ;
		///< Override from GAuth::SaslServerSecrets.

public:
	~Secrets() override ;
	Secrets( const Secrets & ) = delete ;
	Secrets( Secrets && ) = delete ;
	void operator=( const Secrets & ) = delete ;
	void operator=( Secrets && ) = delete ;

private: // overrides
	std::string source() const override ; // Override from GAuth::SaslServerSecrets.
	Secret clientSecret( const std::string & mechanism ) const override ; // Override from GAuth::SaslClientSecrets.
	std::pair<std::string,std::string> serverTrust( const std::string & address_range ) const override ; // Override from GAuth::SaslServerSecrets.

private:
	std::string m_source ;
	std::unique_ptr<SecretsFile> m_imp ;
} ;

#endif
