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
// configuration.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "gpopsecrets.h"
#include "gstrings.h"
#include "gdebug.h"

Main::Configuration::Configuration( const CommandLine & cl ) :
	m_cl(cl)
{
}

bool Main::Configuration::log() const
{
	return 
		contains( "log" ) || 
		contains( "as-client" ) || 
		contains( "as-proxy" ) || 
		contains( "as-server" ) ;
}

bool Main::Configuration::verbose() const
{
	return contains( "verbose" ) ;
}

bool Main::Configuration::debug() const
{
	return contains( "debug" ) ;
}

bool Main::Configuration::useSyslog() const
{
	bool basic = !contains("no-syslog") && !contains("as-client") ;
	bool override = contains( "syslog" ) ;
	return override || basic ;
}

bool Main::Configuration::logTimestamp() const
{
	return contains( "log-time" ) ;
}

unsigned int Main::Configuration::port() const
{
	return value( "port" , 25U ) ;
}

G::Strings Main::Configuration::listeningInterfaces() const
{
	G::Strings result = m_cl.value( "interface" , ",/" ) ;
	if( result.empty() )
	{
		result.push_back( std::string() ) ;
	}
	return result ;
}

std::string Main::Configuration::firstListeningInterface() const
{
	G::Strings s = listeningInterfaces() ;
	return s.size() ? s.front() : std::string() ;
}

std::string Main::Configuration::clientInterface() const
{
	// TODO -- separate switch ?
	return firstListeningInterface() ;
}

unsigned int Main::Configuration::adminPort() const
{
	return value( "admin" , 0U ) ;
}

bool Main::Configuration::closeStderr() const
{
	return 
		contains("close-stderr") || 
		contains("as-proxy") ||
		contains("as-server") ;
}

bool Main::Configuration::immediate() const
{
	return
		contains("immediate") ||
		contains("as-proxy") ;
}

bool Main::Configuration::daemon() const
{
	return !contains("no-daemon") && !contains("as-client") ;
}

G::Path Main::Configuration::spoolDir() const
{
	return contains("spool-dir") ?
		G::Path(value("spool-dir")) : 
		GSmtp::MessageStore::defaultDirectory() ;
}

std::string Main::Configuration::serverAddress() const
{
	const char * key = "forward-to" ;
	if( contains("as-client") )
		key = "as-client" ;
	else if( contains("as-proxy") )
		key = "as-proxy" ;
	return contains(key) ? value(key) : std::string() ;
}

bool Main::Configuration::doForwarding() const
{
	return contains("forward") || contains("as-client") ;
}

bool Main::Configuration::doServing() const
{
	return !contains("dont-serve") && !contains("as-client") ;
}

bool Main::Configuration::doPolling() const
{
	return contains( "poll" ) ;
}

unsigned int Main::Configuration::pollingTimeout() const
{
	return value( "poll" , 0U ) ;
}

unsigned int Main::Configuration::promptTimeout() const
{
	return value( "prompt-timeout" , 20U ) ;
}

bool Main::Configuration::doSmtp() const
{
	return ! contains( "no-smtp" ) ;
}

bool Main::Configuration::doPop() const
{
	return contains( "pop" ) ;
}

bool Main::Configuration::popByName() const
{
	return contains( "pop-by-name" ) ;
}

bool Main::Configuration::popNoDelete() const
{
	return contains( "pop-no-delete" ) ;
}

unsigned int Main::Configuration::popPort() const
{
	return value( "pop-port" , 110U ) ;
}

bool Main::Configuration::allowRemoteClients() const
{
	return contains( "remote-clients" ) ;
}

bool Main::Configuration::doAdmin() const
{
	return contains( "admin" ) ;
}

bool Main::Configuration::usePidFile() const
{
	return contains( "pid-file" ) ;
}

std::string Main::Configuration::pidFile() const
{
	return value( "pid-file" ) ;
}

bool Main::Configuration::useFilter() const
{
	return contains( "filter" ) ;
}

std::string Main::Configuration::filter() const
{
	return contains("filter") ? value("filter") : std::string() ;
}

std::string Main::Configuration::clientFilter() const
{
	return contains("client-filter") ? value("client-filter") : std::string() ;
}

unsigned int Main::Configuration::icon() const
{
	unsigned int n = value( "icon" , 0U ) ;
	return n % 4U ;
}

bool Main::Configuration::hidden() const
{
	return contains( "hidden" ) ;
}

std::string Main::Configuration::clientSecretsFile() const
{
	return contains("client-auth") ? value("client-auth") : std::string() ;
}

bool Main::Configuration::clientTls() const
{
	return contains( "client-tls" ) ;
}

std::string Main::Configuration::serverTlsFile() const
{
	return contains("server-tls") ? value("server-tls") : std::string() ;
}

std::string Main::Configuration::popSecretsFile() const
{
	return contains("pop-auth") ? value("pop-auth") : GPop::Secrets::defaultPath() ;
}

std::string Main::Configuration::serverSecretsFile() const
{
	return contains("server-auth") ? value("server-auth") : std::string() ;
}

unsigned int Main::Configuration::responseTimeout() const
{
	return value( "response-timeout" , 30U * 60U ) ;
}

unsigned int Main::Configuration::connectionTimeout() const
{
	return value( "connection-timeout" , 40U ) ;
}

std::string Main::Configuration::fqdn() const
{
	return contains("domain") ? value("domain") : std::string() ;
}

std::string Main::Configuration::nobody() const
{
	return contains("user") ? value("user") : std::string("daemon") ;
}

std::string Main::Configuration::verifier() const
{
	return contains("verifier") ? value("verifier") : std::string() ;
}

bool Main::Configuration::withTerminate() const
{
	return contains( "admin-terminate" ) ;
}

unsigned int Main::Configuration::maxSize() const
{
	return value( "size" , 0U ) ;
}

std::string Main::Configuration::scannerAddress() const
{
	return contains("scanner") ? value("scanner") : std::string() ;
}

unsigned int Main::Configuration::scannerConnectionTimeout() const
{
	return 10U ; // for now
}

unsigned int Main::Configuration::scannerResponseTimeout() const
{
	return 90U ; // for now
}

bool Main::Configuration::anonymous() const
{
	return contains( "anonymous" ) ;
}

unsigned int Main::Configuration::filterTimeout() const
{
	return value( "filter-timeout" , 5U * 60U ) ;
}

bool Main::Configuration::contains( const char * s ) const
{
	return m_cl.contains( s ) ;
}

std::string Main::Configuration::value( const char * s ) const
{
	return m_cl.value( s ) ;
}

unsigned int Main::Configuration::value( const char * s , unsigned int d ) const
{
	return m_cl.value( s , d ) ;
}

/// \file configuration.cpp
