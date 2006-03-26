//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
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
#include "gstr.h"
#include "gdebug.h"
#include <sstream>

Main::Configuration::Configuration( const CommandLine & cl ) :
	m_cl(cl)
{
}

//static
std::string Main::Configuration::yn( bool b )
{
	return b ? std::string("yes") : std::string("no") ;
}

std::string Main::Configuration::na() const
{
	return "<none>" ;
}

std::string Main::Configuration::na( const std::string & s ) const
{
	return s.empty() ? na() : s ;
}

//static
std::string Main::Configuration::any( const std::string & s )
{
	return s.empty() ? std::string("<any>") : s ;
}

std::string Main::Configuration::str( const std::string & p , const std::string & eol ) const
{
	std::ostringstream ss ;
	ss
		<< p << "allow remote clients? " << yn(allowRemoteClients()) << eol
		<< p << "listening interface: " << (doServing()&&doSmtp()?any(listeningInterface()):na()) << eol
		<< p << "smtp listening port: " << (doServing()&&doSmtp()?G::Str::fromUInt(port()):na()) << eol
		<< p << "pop listening port: " << (doServing()&&doPop()?G::Str::fromUInt(popPort()):na()) << eol
		<< p << "admin listening port: " << (doAdmin()?G::Str::fromUInt(adminPort()):na()) << eol
		<< p << "next smtp server address: " << (serverAddress().length()?serverAddress():na()) << eol
		<< p << "spool directory: " << spoolDir() << eol
		<< p << "smtp client secrets file: " << na(clientSecretsFile()) << eol
		<< p << "smtp server secrets file: " << na(serverSecretsFile()) << eol
		<< p << "pop server secrets file: " << na(popSecretsFile()) << eol
		<< p << "pid file: " << (usePidFile()?pidFile():na()) << eol
		<< p << "immediate forwarding? " << yn(immediate()) << eol
		<< p << "mail processor: " << (useFilter()?filter():na()) << eol
		<< p << "address verifier: " << na(verifier()) << eol
		<< p << "run as daemon? " << yn(daemon()) << eol
		<< p << "verbose logging? " << yn(verbose()) << eol
		<< p << "debug logging? " << yn(debug()) << eol
		<< p << "log to stderr/syslog? " << yn(log()) << eol
		<< p << "use syslog? " << yn(syslog()) << eol
		<< p << "close stderr? " << yn(closeStderr()) << eol
		<< p << "connect timeout: " << connectionTimeout() << "s" << eol
		<< p << "response timeout: " << responseTimeout() << "s" << eol
		<< p << "domain override: " << na(fqdn()) << eol
		<< p << "polling period: " << (pollingTimeout()?(G::Str::fromUInt(pollingTimeout())+"s"):na()) << eol
		;
	return ss.str() ;
}

bool Main::Configuration::log() const
{
	return 
		m_cl.contains("log") || 
		m_cl.contains("as-client") || 
		m_cl.contains("as-proxy") || 
		m_cl.contains("as-server") ;
}

bool Main::Configuration::verbose() const
{
	return m_cl.contains("verbose") ;
}

bool Main::Configuration::debug() const
{
	return m_cl.contains("debug") ;
}

bool Main::Configuration::syslog() const
{
	bool basic = !m_cl.contains("no-syslog") && !m_cl.contains("as-client") ;
	bool override = m_cl.contains("syslog") ;
	return override || basic ;
}

bool Main::Configuration::logTimestamp() const
{
	return m_cl.contains("log-time") ;
}

unsigned int Main::Configuration::port() const
{
	return m_cl.contains("port") ?
		G::Str::toUInt(m_cl.value("port")) : 25U ;
}

std::string Main::Configuration::listeningInterface() const
{
	return m_cl.contains("interface") ? m_cl.value("interface") : std::string() ;
}

std::string Main::Configuration::clientInterface() const
{
	return listeningInterface() ; // or a separate switch?
}

G::Path Main::Configuration::adminAddressFile() const
{
	if( ! m_cl.contains("admin") ) 
		return G::Path() ;

	const std::string s = m_cl.value("admin") ;
	if( s.find("tcp://") == 0U && s.length() > 6U && s.find("/",6U) != std::string::npos )
	{
		return G::Path(s.substr(s.find("/",6U))) ;
	}
	else
	{
		return G::Path() ;
	}
}

unsigned int Main::Configuration::adminPort() const
{
	std::string s = m_cl.contains("admin") ? m_cl.value("admin") : std::string() ;
	if( s.find("tcp://") == 0U )
	{
		s = 6U >= s.length() ? std::string() : s.substr(6U) ;

		size_t p = s.find("/") ;
		if( p != std::string::npos ) 
			s = s.substr(0U,p) ;

		p = s.find_last_of( ':' ) ;
		if( p != std::string::npos )
			s = (p+1U) >= s.length() ? std::string() : s.substr(p+1U) ;
	}
	return s.empty() ? 0U : G::Str::toUInt(s) ;
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
	return m_cl.contains("poll") ? G::Str::toUInt(m_cl.value("poll")) : 0U ;
}

bool Main::Configuration::doSmtp() const
{
	return !m_cl.contains("no-smtp") ;
}

bool Main::Configuration::doPop() const
{
	return m_cl.contains("pop") ;
}

bool Main::Configuration::popByName() const
{
	return m_cl.contains("pop-by-name") ;
}

bool Main::Configuration::popNoDelete() const
{
	return m_cl.contains("pop-no-delete") ;
}

unsigned int Main::Configuration::popPort() const
{
	return m_cl.contains("pop-port") ?
		G::Str::toUInt(m_cl.value("pop-port")) : 110U ;
}

bool Main::Configuration::allowRemoteClients() const
{
	return m_cl.contains("remote-clients") ;
}

bool Main::Configuration::doAdmin() const
{
	return m_cl.contains("admin") ;
}

bool Main::Configuration::usePidFile() const
{
	return m_cl.contains("pid-file") ;
}

std::string Main::Configuration::pidFile() const
{
	return m_cl.value("pid-file") ;
}

bool Main::Configuration::useFilter() const
{
	return m_cl.contains("filter") ;
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
	unsigned int n = 0U ;
	if( m_cl.contains("icon") )
	{
		n = G::Str::toUInt(m_cl.value("icon")) ;
		n %= 4U ;
	}
	return n ;
}

bool Main::Configuration::hidden() const
{
	return m_cl.contains("hidden") ;
}

std::string Main::Configuration::clientSecretsFile() const
{
	return m_cl.contains("client-auth") ? m_cl.value("client-auth") : std::string() ;
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
	const unsigned int default_timeout = 30U * 60U ;
	return m_cl.contains("response-timeout") ?
		G::Str::toUInt(m_cl.value("response-timeout")) : default_timeout ;
}

unsigned int Main::Configuration::connectionTimeout() const
{
	const unsigned int default_timeout = 40U ;
	return m_cl.contains("connection-timeout") ?
		G::Str::toUInt(m_cl.value("connection-timeout")) : default_timeout ;
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
	return m_cl.contains("admin-terminate") ;
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
	return m_cl.contains("anonymous") ;
}

unsigned int Main::Configuration::filterTimeout() const
{
	return 120U ; // for now
}

