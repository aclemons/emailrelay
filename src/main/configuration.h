//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gsmtp.h"
#include "gpath.h"
#include "gstrings.h"
#include <string>

/// \namespace Main
namespace Main
{
	class Configuration ;
	class CommandLine ;
}

/// \class Main::Configuration
/// An interface for returning application configuration
/// information. This implementation is minimaly dependent on the
/// command line in order to simplify moving to (eg) the windows registry
/// or a configuration file in the future.
///
/// \see CommandLine
///
class Main::Configuration 
{
public:
	explicit Configuration( const CommandLine & cl ) ;
		///< Constructor. The reference is kept.

	unsigned int port() const ;
		///< Returns the main listening port number.

	G::Strings listeningInterfaces( const std::string & protocol = std::string() ) const ;
		///< Returns the listening interface(s).

	std::string clientInterface() const ;
		///< Returns the sending interface.

	bool closeStderr() const ;
		///< Returns true if stderr should be closed.

	bool log() const ;
		///< Returns true if doing logging.

	std::string logFile() const ;
		///< Returns the path of a stderr replacement for logging.

	bool verbose() const ;
		///< Returns true if doing verbose logging.

	bool debug() const ;
		///< Returns true if doing debug-level logging.

	bool useSyslog() const ;
		///< Returns true if generating syslog events.

	bool logTimestamp() const ;
		///< Returns true if logging output should be timestamped.

	bool daemon() const ;
		///< Returns true if running as a daemon.

	bool doForwardingOnStartup() const ;
		///< Returns true if running as a client.

	bool doServing() const ;
		///< Returns true if running as a server (SMTP, POP, admin or COM).

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

	unsigned int popPort() const ;
		///< Returns the pop port number.

	bool allowRemoteClients() const ;
		///< Returns true if allowing remote clients to connect.

	G::Path spoolDir() const ;
		///< Returns the spool directory.

	std::string serverAddress() const ;
		///< Returns the downstream server's address string.

	bool usePidFile() const ;
		///< Returns true if writing a pid file.

	std::string pidFile() const ;
		///< Returns the pid file's path.

	bool useFilter() const ;
		///< Returns true if pre-processing.

	std::string filter() const ;
		///< Returns the path to a server-side pre-processor.

	std::string clientFilter() const ;
		///< Returns the path to a client-side pre-processor.

	unsigned int filterTimeout() const ;
		///< Returns the timeout for executing an ansynchronous
		///< filter() or clientFilter() program.

	unsigned int icon() const ;
		///< Returns the icon selector (win32).

	bool hidden() const ;
		///< Returns true if the main window is hidden (win32).

	unsigned int responseTimeout() const ;
		///< Returns the client-side protocol timeout value.

	unsigned int connectionTimeout() const ;
		///< Returns the client-side connection timeout value.

	unsigned int secureConnectionTimeout() const ;
		///< Returns the timeout for establishing a secure connection.

	unsigned int promptTimeout() const ;
		///< Returns the timeout for getting a prompt from the SMTP server.

	std::string clientSecretsFile() const ;
		///< Returns the client-side autentication secrets (password) file.
		///< Returns the empty string if none.

	std::string serverSecretsFile() const ;
		///< Returns the server-side autentication secrets (password) file.
		///< Returns the empty string if none.

	std::string popSecretsFile() const ;
		///< Returns the pop-server autentication secrets (password) file.
		///< Returns the empty string if not defined.

	std::string fqdn( std::string default_ = std::string() ) const ;
		///< Returns the fully-qualified-domain-name override.

	std::string nobody() const ;
		///< Returns the name of an unprivileged user. This is only
		///< used if running with a real user-id of root.

	std::string verifier() const ;
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

	bool forwardingOnStore() const ;
		///< Returns true if forwarding should occur as each message
		///< is stored, after it is acknowledged. (This will result
		///< in a complete client session per message.)

	bool forwardingOnDisconnect() const ;
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
		///< account of the server's tls capability.

	bool clientOverTls() const ;
		///< Returns true if using the SMTP over TLS (vs. negotiated 
		///< TLS using STARTTLS).

	unsigned int tlsConfig() const ;
		///< Returns TLS configuration flags.

	std::string serverTlsFile() const ;
		///< Returns the tls certificate file if the server
		///< should support tls.

	unsigned int maxSize() const ;
		///< Returns the maximum size of submitted messages, or zero.

	bool peerLookup() const ;
		///< Returns true if there should be some attempt to 
		///< look up the userid of SMTP peers connected from the
		///< the local machine.

private:
	bool contains( const char * ) const ;
	std::string value( const char * ) const ;
	unsigned int value( const char * , unsigned int ) const ;

private:
	const CommandLine & m_cl ;
} ;

#endif

