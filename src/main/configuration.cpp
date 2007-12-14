//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
		m_cl.contains( "log" ) || 
		m_cl.contains( "as-client" ) || 
		m_cl.contains( "as-proxy" ) || 
		m_cl.contains( "as-server" ) ;
}

bool Main::Configuration::verbose() const
{
	return m_cl.contains( "verbose" ) ;
}

bool Main::Configuration::debug() const
{
	return m_cl.contains( "debug" ) ;
}

bool Main::Configuration::useSyslog() const
{
	bool basic = !m_cl.contains("no-syslog") && !m_cl.contains("as-client") ;
	bool override = m_cl.contains( "syslog" ) ;
	return override || basic ;
}

bool Main::Configuration::logTimestamp() const
{
	return m_cl.contains( "log-time" ) ;
}

unsigned int Main::Configuration::port() const
{
	return m_cl.value( "port" , 25U ) ;
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
	return m_cl.value( "admin" , 0U ) ;
}

bool Main::Configuration::closeStderr() const
{
	return 
		m_cl.contains("close-stderr") || 
		m_cl.contains("as-proxy") ||
		m_cl.contains("as-server") ;
}

bool Main::Configuration::immediate() const
{
	return
		m_cl.contains("immediate") ||
		m_cl.contains("as-proxy") ;
}

bool Main::Configuration::daemon() const
{
	return !m_cl.contains("no-daemon") && !m_cl.contains("as-client") ;
}

G::Path Main::Configuration::spoolDir() const
{
	return m_cl.contains("spool-dir") ?
		G::Path(m_cl.value("spool-dir")) : 
		GSmtp::MessageStore::defaultDirectory() ;
}

std::string Main::Configuration::serverAddress() const
{
	const char * key = "forward-to" ;
	if( m_cl.contains("as-client") )
		key = "as-client" ;
	else if( m_cl.contains("as-proxy") )
		key = "as-proxy" ;
	return m_cl.contains(key) ? m_cl.value(key) : std::string() ;
}

bool Main::Configuration::doForwarding() const
{
	return m_cl.contains("forward") || m_cl.contains("as-client") ;
}

bool Main::Configuration::doServing() const
{
	return !m_cl.contains("dont-serve") && !m_cl.contains("as-client") ;
}

bool Main::Configuration::doPolling() const
{
	return m_cl.contains("poll") ;
}

unsigned int Main::Configuration::pollingTimeout() const
{
	return m_cl.value( "poll" , 0U ) ;
}

unsigned int Main::Configuration::promptTimeout() const
{
	return m_cl.value( "prompt-timeout" , 20U ) ;
}

bool Main::Configuration::doSmtp() const
{
	return ! m_cl.contains( "no-smtp" ) ;
}

bool Main::Configuration::doPop() const
{
	return m_cl.contains( "pop" ) ;
}

bool Main::Configuration::popByName() const
{
	return m_cl.contains( "pop-by-name" ) ;
}

bool Main::Configuration::popNoDelete() const
{
	return m_cl.contains( "pop-no-delete" ) ;
}

unsigned int Main::Configuration::popPort() const
{
	return m_cl.value( "pop-port" , 110U ) ;
}

bool Main::Configuration::allowRemoteClients() const
{
	return m_cl.contains( "remote-clients" ) ;
}

bool Main::Configuration::doAdmin() const
{
	return m_cl.contains( "admin" ) ;
}

bool Main::Configuration::usePidFile() const
{
	return m_cl.contains( "pid-file" ) ;
}

std::string Main::Configuration::pidFile() const
{
	return m_cl.value( "pid-file" ) ;
}

bool Main::Configuration::useFilter() const
{
	return m_cl.contains( "filter" ) ;
}

std::string Main::Configuration::filter() const
{
	return m_cl.contains("filter") ? m_cl.value("filter") : std::string() ;
}

std::string Main::Configuration::clientFilter() const
{
	return m_cl.contains("client-filter") ? m_cl.value("client-filter") : std::string() ;
}

unsigned int Main::Configuration::icon() const
{
	unsigned int n = m_cl.value( "icon" , 0U ) ;
	return n % 4U ;
}

bool Main::Configuration::hidden() const
{
	return m_cl.contains( "hidden" ) ;
}

std::string Main::Configuration::clientSecretsFile() const
{
	return m_cl.contains("client-auth") ? m_cl.value("client-auth") : std::string() ;
}

bool Main::Configuration::clientTls() const
{
	return m_cl.contains( "client-tls" ) ;
}

std::string Main::Configuration::serverTlsFile() const
{
	return m_cl.contains("server-tls") ? m_cl.value("server-tls") : std::string() ;
}

std::string Main::Configuration::popSecretsFile() const
{
	return m_cl.contains("pop-auth") ? m_cl.value("pop-auth") : GPop::Secrets::defaultPath() ;
}

std::string Main::Configuration::serverSecretsFile() const
{
	return m_cl.contains("server-auth") ? m_cl.value("server-auth") : std::string() ;
}

unsigned int Main::Configuration::responseTimeout() const
{
	return m_cl.value( "response-timeout" , 30U * 60U ) ;
}

unsigned int Main::Configuration::connectionTimeout() const
{
	return m_cl.value( "connection-timeout" , 40U ) ;
}

std::string Main::Configuration::fqdn() const
{
	return m_cl.contains("domain") ? m_cl.value("domain") : std::string() ;
}

std::string Main::Configuration::nobody() const
{
	return m_cl.contains("user") ? m_cl.value("user") : std::string("daemon") ;
}

std::string Main::Configuration::verifier() const
{
	return m_cl.contains("verifier") ? m_cl.value("verifier") : std::string() ;
}

bool Main::Configuration::withTerminate() const
{
	return m_cl.contains( "admin-terminate" ) ;
}

std::string Main::Configuration::scannerAddress() const
{
	return m_cl.contains("scanner") ? m_cl.value("scanner") : std::string() ;
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
	return m_cl.contains( "anonymous" ) ;
}

unsigned int Main::Configuration::filterTimeout() const
{
	return m_cl.value( "filter-timeout" , 5U * 60U ) ;
}

/// \file configuration.cpp
