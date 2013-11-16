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
//
// configuration.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstr.h"
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

std::string Main::Configuration::logFile() const
{
	return contains("log-file") ? value("log-file") : std::string() ;
}

unsigned int Main::Configuration::port() const
{
	return value( "port" , 25U ) ;
}

G::Strings Main::Configuration::listeningInterfaces( const std::string & protocol ) const
{
	// allow eg. "127.0.0.1,smtp=192.168.1.1,admin=10.0.0.1"

	// prepare the naive result by splitting the user's string by comma or slash separators 
	G::Strings result = m_cl.value( "interface" , ",/" ) ;

	// then weed out the ones that have an explicit protocol name that doesnt match the 
	// required protocol, removing the "protocol=" prefix at the same time to leave just 
	// the required list of addresses
	for( G::Strings::iterator p = result.begin() ; p != result.end() ; )
	{
		if( protocol.empty() || protocol == G::Str::head( *p , (*p).find('=') , protocol ) )
			*p++ = G::Str::tail( *p , (*p).find('=') , *p ) ;
		else
			p = result.erase( p ) ;
	}

	return result ;
}

std::string Main::Configuration::clientInterface() const
{
	G::Strings result = m_cl.value( "interface" , ",/" ) ;
	// look for first "client=" value
	{
		for( G::Strings::iterator p = result.begin() ; p != result.end() ; ++p )
		{
			if( (*p).find("client=") == 0U )
				return G::Str::tail( *p , (*p).find('=') , *p ) ;
		}
	}
	// or use the first unqalified value
	{
		for( G::Strings::iterator p = result.begin() ; p != result.end() ; ++p )
		{
			if( (*p).find('=') == std::string::npos )
				return *p ;
		}
	}
	return std::string() ;
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

bool Main::Configuration::doForwardingOnStartup() const
{
	return contains("forward") || contains("as-client") ;
}

bool Main::Configuration::doServing() const
{
	return !contains("dont-serve") && !contains("as-client") ;
}

bool Main::Configuration::immediate() const
{
	return contains("immediate") ;
}

bool Main::Configuration::doPolling() const
{
	return contains("poll") && pollingTimeout() > 0U ;
}

unsigned int Main::Configuration::pollingTimeout() const
{
	return value( "poll" , 0U ) ;
}

bool Main::Configuration::pollingLog() const
{
	// dont log if polling very frequently
	return doPolling() && pollingTimeout() > 60U ;
}

bool Main::Configuration::forwardingOnStore() const
{
	// this is not settable for now -- the 103 exit
	// code has a similar effect -- see also comments below
	return false ;
}

bool Main::Configuration::forwardingOnDisconnect() const
{
	// TODO -- it not completely logical to tie forwarding-on-disconnect
	// in with polling, but it avoids the situation where messages can
	// get missed if a polling run is already in progress when a
	// forwarding-on-disconnect event occurs -- the scan of the spool 
	// directory is done at the start of the polling run -- the fix 
	// could be to allow clients to run in parallel in the Run class, 
	// or have a method to force the client to rescan the directory 
	// before finishing

	return 
		( contains("poll") && pollingTimeout() == 0U ) ||
		contains("as-proxy") ;
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
	return 0U ; // no longer used
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

bool Main::Configuration::clientOverTls() const
{
	return contains( "client-tls-connection" ) ;
}

unsigned int Main::Configuration::tlsConfig() const
{
	return value( "tls-config" , 0U ) ;
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

unsigned int Main::Configuration::secureConnectionTimeout() const
{
	return connectionTimeout() ;
}

std::string Main::Configuration::fqdn( std::string default_ ) const
{
	return contains("domain") ? value("domain") : default_ ;
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

bool Main::Configuration::peerLookup() const 
{
	return contains( "peer-lookup" ) ;
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
