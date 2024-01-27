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
/// \file configuration.h
///

#ifndef G_MAIN_CONFIGURATION_H
#define G_MAIN_CONFIGURATION_H

#include "gdef.h"
#include "garg.h"
#include "goptions.h"
#include "goptionmap.h"
#include "gpath.h"
#include "gstringarray.h"
#include "gserver.h"
#include "gserverpeer.h"
#include "gsmtpserver.h"
#include "gadminserver.h"
#include "gsmtpclient.h"
#include "gfilestore.h"
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "gpopserver.h"
#include "gsaslserversecrets.h"
#include "glocal.h"
#include "glogoutput.h"
#include <string>
#include <functional>

namespace Main
{
	class Configuration ;
	class ConfigurationImp ;
	class CommandLine ;
}

//| \class Main::Configuration
/// An interface for returning application configuration information. In
/// practice this is a thin wrapper around the command-line options
/// passed in to the constructor.
///
class Main::Configuration
{
public:
	Configuration( const G::OptionMap & , const std::string & name ,
		const G::Path & app_dir , const G::Path & base_dir ) ;
			///< Constructor. The app-dir path is used as a substitution
			///< value, and the base-dir path is used to turn relative paths
			///< into absolute ones when daemon() is true.

	std::string name() const ;
		///< Returns the configuration name, or the empty string for the
		///< default configuration.

	std::string semanticError() const ;
		///< Returns a non-empty string if there is a fatal semantic conflict
		///< in the configuration.

	G::StringArray semanticWarnings() const ;
		///< Returns a non-empty array if there are non-fatal semantic conflicts
		///< in the configuration.

	G::StringArray display( const G::Options & options_spec ) const ;
		///< Returns a table of all configuration options for display. The
		///< list of strings are paired as key-then-value.

	bool closeFiles() const ;
		///< Returns true if the daemon should start by closing files.

	std::string clientBindAddress() const ;
		///< Returns the sending address.

	bool closeStderr() const ;
		///< Returns true if stderr should be closed.

	bool log() const ;
		///< Returns true if doing logging.

	std::string logFile() const ;
		///< Returns the path of a stderr replacement for logging.

	bool debug() const ;
		///< Returns true if doing debug-level logging.

	bool hidden() const ;
		///< Returns true if explicitly hidden, but see also show('hidden').

	bool useSyslog() const ;
		///< Returns true if generating syslog events.

	bool daemon() const ;
		///< Returns true if running in the background.

	bool forwardOnStartup() const ;
		///< Returns true if running as a client.

	bool doServing() const ;
		///< Returns true if running as a server (smtp, pop, admin).

	bool doSmtp() const ;
		///< Returns true if listening for smtp connections.

	bool doPop() const ;
		///< Returns true if listening for pop connections.

	bool popByName() const ;
		///< Returns true if the pop spool directory is
		///< modified according to the client name.

	bool popNoDelete() const ;
		///< Returns true if pop deletion is to be disabled.

	bool doAdmin() const ;
		///< Returns true if listening for admin connections.

	G::Path spoolDir() const ;
		///< Returns the spool directory.

	std::string serverAddress() const ;
		///< Returns the downstream server's address string.

	bool usePidFile() const ;
		///< Returns true if writing a pid file.

	G::Path pidFile() const ;
		///< Returns the pid file's path.

	G::Path clientSecretsFile() const ;
		///< Returns the client-side autentication secrets (password) file.
		///< Returns the empty string if none.

	G::Path serverSecretsFile() const ;
		///< Returns the server-side autentication secrets (password) file.
		///< Returns the empty string if none.

	G::Path popSecretsFile() const ;
		///< Returns the pop-server autentication secrets (password) file.
		///< Returns the empty string if not defined.

	std::string domain( std::function<std::string()> default_fn ) const ;
		///< Returns an override for local host's canonical network name.

	std::string user() const ;
		///< Returns the name of an unprivileged user. This is only
		///< used if running with a real user-id of root.

	bool doPolling() const ;
		///< Returns true if doing poll-based forwarding.

	bool pollingLog() const ;
		///< Returns true if polling activity should be logged.

	unsigned int pollingTimeout() const ;
		///< Returns the timeout for periodic polling.

	bool immediate() const ;
		///< Returns true if forwarding should occur as soon as each
		///< message body is received and before receipt is
		///< acknowledged.

	bool forwardOnDisconnect() const ;
		///< Returns true if forwarding should occur when the
		///< submitter's network connection disconnects.

	bool clientTls() const ;
		///< Returns true if the client protocol should take
		///< account of the server's TLS capability.

	bool clientOverTls() const ;
		///< Returns true if using the SMTP over a TLS tunnel
		///< (as opposed to using STARTTLS).

	bool serverTls() const ;
		///< Returns true if the server protocol should support negotiated TLS.

	bool serverTlsConnection() const ;
		///< Returns true if the server protocol should support implicit TLS.

	std::string tlsConfig() const ;
		///< Returns low-level TLS configuration options.

	G::Path serverTlsPrivateKey() const ;
		///< Returns the server-side TLS private key file.

	G::Path serverTlsCertificate() const ;
		///< Returns the server-side TLS certificate file.

	G::Path serverTlsCaList() const ;
		///< Returns the server-side TLS CA file-or-directory.

	G::Path clientTlsPrivateKey() const ;
		 ///< Returns the client-side TLS private key file.

	G::Path clientTlsCertificate() const ;
		///< Returns the client-side TLS certificate file.

	G::Path clientTlsCaList() const ;
		///< Returns the client-side TLS CA file-or-directory.

	std::string clientTlsPeerCertificateName() const ;
		///< Returns the client-side's requirement for the subject-CNAME
		///< in the server's certificate.

	std::string clientTlsPeerHostName() const ;
		///< Returns the client-side's target server hostname (SNI).

	std::string dnsbl() const ;
		///< Returns a DNSBL configuration string including a list servers.

	std::string show() const ;
		///< Returns the requested Windows user-interface style,
		///< such as "popup" or "tray".

	bool show( const std::string & key ) const ;
		///< Returns true if the show() string contains the given
		///< sub-string.

	GSmtp::Client::Config smtpClientConfig( const std::string & client_tls_profile ,
		const std::string & domain ) const ;
			///< Returns the smtp client configuration structure.

	GSmtp::Server::Config smtpServerConfig( const std::string & smtp_ident ,
		bool server_secrets_valid , const std::string & server_tls_profile ,
		const std::string & domain ) const ;
			///< Returns the smtp server configuration structure.

	GPop::Server::Config popServerConfig( const std::string & server_tls_profile ,
		const std::string & domain ) const ;
			///< Returns the pop server configuration structure.

	G::LogOutput::Config logOutputConfig( bool has_gui ) const ;
		///< Returns the log-output configuration structure.

	GStore::FileStore::Config fileStoreConfig() const ;
		///< Returns the file-store configuration structure.

	GSmtp::AdminServer::Config adminServerConfig( const G::StringMap & info_map ,
		const std::string & client_tls_profile_for_flush ,
		const std::string & domain ) const ;
			///< Returns the admin server configuration structure.

	G::StringArray listeningNames( G::string_view protocol = {} ) const ;
		///< Returns the listening addresses, interfaces and file descriptors.

	GSmtp::FilterFactoryBase::Spec filter() const ;
		///< Returns the filter spec.

	unsigned int filterTimeout() const ;
		///< Returns the filter timeout.

	G::Path deliveryDir() const ;
		///< Returns a local-delivery base directory.

private:
	bool contains( const char * ) const ;
	unsigned int numberValue( const char * key , unsigned int default_ ) const ;
	std::string stringValue( G::string_view ) const ;
	std::string stringValue( const char * ) const ;
	std::string stringValue( const char * , const std::string & ) const ;
	std::string stringValue( const char * , std::function<std::string()> ) const ;
	std::string stringValue( const std::string & ) const ;
	G::Path pathValue( G::string_view ) const ;
	G::Path pathValueImp( std::string && ) const ;
	GSmtp::FilterFactoryBase::Spec filterValue( G::string_view , G::StringArray * = nullptr ) const ;
	GSmtp::VerifierFactoryBase::Spec verifierValue( G::string_view , G::StringArray * = nullptr ) const ;
	static bool pathlike( G::string_view ) ;
	//
	const char * semanticError1() const ;
	std::string semanticError2() const ;
	//
	G::Path certificateFile( const std::string & option ) const ;
	G::Path keyFile( const std::string & option ) const ;
	bool validSyslogFacility() const ;
	bool anonymous( G::string_view ) const ;
	static bool tlsVerifyType( G::string_view ) ;
	static bool specialTlsVerifyString( G::string_view ) ;
	//
	GNet::Server::Config _netServerConfig( std::pair<int,int> linger ) const ;
	GNet::StreamSocket::Config _netSocketConfig( std::pair<int,int> linger ) const ;
	GSmtp::ServerProtocol::Config _smtpServerProtocolConfig( bool server_secrets_valid , const std::string & domain ) const ;
	GNet::SocketProtocol::Config _socketProtocolConfig( const std::string & server_tls_profile ) const ;
	//
	unsigned int _adminPort() const ;
	std::pair<int,int> _adminServerSocketLinger() const ;
	bool _allowRemoteClients() const ;
	GSmtp::FilterFactoryBase::Spec _clientFilter() const ;
	std::pair<int,int> _clientSocketLinger() const ;
	unsigned int _connectionTimeout() const ;
	unsigned int _idleTimeout() const ;
	unsigned int _maxSize() const ;
	bool _nodaemon() const ;
	unsigned int _popPort() const ;
	std::string _popSaslServerConfig() const ;
	std::pair<int,int> _popServerSocketLinger() const ;
	unsigned int _port() const ;
	unsigned int _promptTimeout() const ;
	unsigned int _responseTimeout() const ;
	unsigned int _secureConnectionTimeout() const ;
	bool _serverTlsRequired() const ;
	std::string _show() const ;
	int _shutdownHowOnQuit() const ;
	std::string _smtpSaslClientConfig() const ;
	std::string _smtpSaslServerConfig() const ;
	std::pair<int,int> _smtpServerSocketLinger() const ;
	G::LogOutput::SyslogFacility _syslogFacility() const ;
	GSmtp::VerifierFactoryBase::Spec _verifier() const ;

private:
	struct Switches /// Sub-option keywords that can be on or off.
	{
		explicit Switches( const std::string & ) ;
		~Switches() ;
		Switches( const Switches & ) = default ;
		Switches( Switches && ) noexcept = default ;
		Switches & operator=( const Switches & ) = default ;
		Switches & operator=( Switches && ) noexcept = default ;
		bool operator()( G::string_view item , bool default_ ) ;
		G::StringArray m_items ;
		static std::string str( char , G::string_view ) ;
		static std::string str( G::string_view , G::string_view ) ;
		bool remove( G::string_view ) ;
		bool remove( const std::string & ) ;
	} ;

private:
	G::OptionMap m_map ;
	std::string m_name ;
	G::Path m_app_dir ;
	G::Path m_base_dir ;
	bool m_pid_file_warning {false} ;
} ;

inline
unsigned int Main::Configuration::numberValue( const char * key , unsigned int default_ ) const
{
	return m_map.number( key , default_ ) ;
}

inline
std::string Main::Configuration::stringValue( const char * key ) const
{
	return m_map.value( key ) ;
}

inline
std::string Main::Configuration::stringValue( const char * key , const std::string & default_ ) const
{
	return m_map.value( key , default_ ) ;
}

inline
std::string Main::Configuration::stringValue( const char * key , std::function<std::string()> default_fn ) const
{
	return m_map.contains(key) ? m_map.value( key ) : m_map.value( key , default_fn() ) ;
}

inline
std::string Main::Configuration::stringValue( G::string_view key ) const
{
	return m_map.value( key ) ;
}

inline
std::string Main::Configuration::stringValue( const std::string & key ) const
{
	return m_map.value( key ) ;
}

#endif
