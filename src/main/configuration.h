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
#include "glogoutput.h"
#include <string>

namespace Main
{
	class Configuration ;
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
	Configuration( const std::vector<G::Option> & , const G::OptionMap & ,
		const G::Path & app_dir , const G::Path & base_dir ) ;
			///< Constructor. The app-dir path is used as a substitution
			///< value, and the base-dir path is used to turn relative paths
			///< into absolute ones when daemon() is true.

	std::string semanticError() const ;
		///< Returns a non-empty string if there is a fatal semantic conflict
		///< in the configuration.

	G::StringArray semanticWarnings() const ;
		///< Returns a non-empty array if there are non-fatal semantic conflicts
		///< in the configuration.

	G::StringArray display() const ;
		///< Returns a table of all configuration options for display. The
		///< list of strings are paired as key-then-value.

	unsigned int port() const ;
		///< Returns the main listening port number.

	std::pair<int,int> socketLinger() const ;
		///< Returns the socket linger option for smtp connections.

	G::StringArray listeningAddresses( const std::string & protocol = {} ) const ;
		///< Returns the listening addresses.

	std::string clientBindAddress() const ;
		///< Returns the sending address.

	bool closeStderr() const ;
		///< Returns true if stderr should be closed.

	bool log() const ;
		///< Returns true if doing logging.

	G::Path logFile() const ;
		///< Returns the path of a stderr replacement for logging.

	bool verbose() const ;
		///< Returns true if doing verbose logging.

	bool debug() const ;
		///< Returns true if doing debug-level logging.

	bool hidden() const ;
		///< Returns true if explicitly hidden, but see also show('hidden').

	bool useSyslog() const ;
		///< Returns true if generating syslog events.

	G::LogOutput::SyslogFacility syslogFacility() const ;
		///< Returns the syslog facility enum.

	std::string validSyslogFacilities() const ;
		///< Returns help text for the valid syslog facility names.

	bool logTimestamp() const ;
		///< Returns true if logging output should be timestamped.

	bool logAddress() const ;
		///< Returns true if logging output should have remote addresses.

	bool daemon() const ;
		///< Returns true if running in the background.

	bool nodaemon() const ;
		///< Returns true if running in the foreground.

	bool forwardOnStartup() const ;
		///< Returns true if running as a client.

	bool forwardToSome() const ;
		///< Returns true if some envelope addressees can be rejected.

	bool doServing() const ;
		///< Returns true if running as a server (smtp, pop, admin).

	bool dontServe() const ;
		///< Returns !doServing().

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

	unsigned int adminPort() const ;
		///< Returns the admin port number.

	std::pair<int,int> adminSocketLinger() const ;
		///< Returns the socket linger option for admin connections.

	unsigned int popPort() const ;
		///< Returns the pop port number.

	std::pair<int,int> popSocketLinger() const ;
		///< Returns the socket linger option for pop connections.

	bool allowRemoteClients() const ;
		///< Returns true if allowing remote clients to connect.

	G::Path spoolDir() const ;
		///< Returns the spool directory.

	std::string serverAddress() const ;
		///< Returns the downstream server's address string.

	bool usePidFile() const ;
		///< Returns true if writing a pid file.

	G::Path pidFile() const ;
		///< Returns the pid file's path.

	bool useFilter() const ;
		///< Returns true if pre-processing.

	G::Path filter() const ;
		///< Returns the path to a server-side pre-processor.

	G::Path clientFilter() const ;
		///< Returns the path to a client-side pre-processor.

	unsigned int filterTimeout() const ;
		///< Returns the timeout for executing an ansynchronous
		///< filter() or clientFilter() program.

	unsigned int responseTimeout() const ;
		///< Returns the client-side protocol timeout value.

	unsigned int idleTimeout() const ;
		///< Returns the server-side idle timeout value.

	unsigned int connectionTimeout() const ;
		///< Returns the client-side connection timeout value.

	unsigned int secureConnectionTimeout() const ;
		///< Returns the timeout for establishing a secure connection.

	unsigned int promptTimeout() const ;
		///< Returns the timeout for getting a prompt from the SMTP server.

	G::Path clientSecretsFile() const ;
		///< Returns the client-side autentication secrets (password) file.
		///< Returns the empty string if none.

	std::string smtpSaslClientConfig() const ;
		///< Returns the SMTP client-side SASL configuration string.

	G::Path serverSecretsFile() const ;
		///< Returns the server-side autentication secrets (password) file.
		///< Returns the empty string if none.

	std::string smtpSaslServerConfig() const ;
		///< Returns the SMTP server-side SASL configuration string.

	G::Path popSecretsFile() const ;
		///< Returns the pop-server autentication secrets (password) file.
		///< Returns the empty string if not defined.

	std::string popSaslServerConfig() const ;
		///< Returns the POP SASL configuration string.

	std::string networkName( const std::string & default_ = {} ) const ;
		///< Returns an override for local host's canonical network name.

	std::string user() const ;
		///< Returns the name of an unprivileged user. This is only
		///< used if running with a real user-id of root.

	G::Path verifier() const ;
		///< Returns the path of an external address verifier program.

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

	bool withTerminate() const ;
		///< Returns true if the admin interface should support the
		///< terminate command.

	std::string scannerAddress() const ;
		///< Returns the address of a scanner process.

	unsigned int scannerConnectionTimeout() const ;
		///< Returns a timeout for connecting to the scanner process.

	unsigned int scannerResponseTimeout() const ;
		///< Returns a timeout for talking to the scanner process.

	bool anonymous() const ;
		///< Returns true if the server protocol should be
		///< slightly more anonymous.

	bool clientTls() const ;
		///< Returns true if the client protocol should take
		///< account of the server's TLS capability.

	bool clientTlsRequired() const ;
		///< Returns true if the SMTP client connections must use TLS
		///< (either tunnelled or STARTTLS).

	bool clientOverTls() const ;
		///< Returns true if using the SMTP over a TLS tunnel
		///< (as opposed to using STARTTLS).

	bool serverTls() const ;
		///< Returns true if the server protocol should support negotiated TLS.

	bool serverTlsConnection() const ;
		///< Returns true if the server protocol should support implicit TLS.

	bool serverTlsRequired() const ;
		///< Returns true if the SMTP server requires TLS before authentication.

	std::string tlsConfig() const ;
		///< Returns low-level TLS configuration options.

	G::Path serverTlsCertificate() const ;
		///< Returns the server-side TLS certificate file.

	G::Path serverTlsCaList() const ;
		///< Returns the server-side TLS CA file-or-directory.

	G::Path clientTlsCertificate() const ;
		///< Returns the client-side TLS certificate file.

	G::Path clientTlsCaList() const ;
		///< Returns the client-side TLS CA file-or-directory.

	std::string clientTlsPeerCertificateName() const ;
		///< Returns the client-side's requirement for the subject-CNAME
		///< in the server's certificate.

	std::string clientTlsPeerHostName() const ;
		///< Returns the client-side's target server hostname (SNI).

	unsigned int maxSize() const ;
		///< Returns the maximum size of submitted messages, or zero.

	int shutdownHowOnQuit() const ;
		///< Returns the socket shutdown parameter when replying
		///< to SMTP QUIT command (1 for the default behaviour
		///< or -1 for no-op).

	bool utf8Test() const ;
		///< Returns true if new messages should be tested as to
		///< whether they have 8bit mailbox names.

	std::string dnsbl() const ;
		///< Returns a DNSBL configuration string including a list servers.

	std::string show() const ;
		///< Returns the requested Windows user-interface style,
		///< such as "popup" or "tray".

	bool show( const std::string & key ) const ;
		///< Returns true if the show() string contains the given
		///< sub-string.

private:
	G::Path pathValue( const std::string & ) const ;
	G::Path pathValue( const char * ) const ;
	std::string semanticError( bool & ) const ;
	bool pathlike( const std::string & ) const ;
	bool filterType( const std::string & ) const ;
	bool specialFilterValue( const std::string & ) const ;
	bool verifyType( const std::string & ) const ;
	bool specialVerifyValue( const std::string & ) const ;
	G::StringArray semantics( bool ) const ;
	bool validSyslogFacility() const ;

private:
	std::vector<G::Option> m_options ;
	G::OptionMap m_map ;
	G::Path m_app_dir ;
	G::Path m_base_dir ;
} ;

#endif
