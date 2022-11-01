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
/// \file configuration.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gstringfield.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "gfactoryparser.h"
#include "gaddress.h"
#include "gprocess.h"
#include "gformat.h"
#include "ggettext.h"
#include "glog.h"
#include <array>

Main::Configuration::Configuration( const std::vector<G::Option> & options , const G::OptionMap & map ,
	const G::Path & app_dir , const G::Path & base_dir ) :
		m_options(options) ,
		m_map(map) ,
		m_app_dir(app_dir) ,
		m_base_dir(base_dir)
{
}

bool Main::Configuration::log() const
{
	return
		m_map.contains( "log" ) ||
		m_map.contains( "as-client" ) ||
		m_map.contains( "as-proxy" ) ||
		m_map.contains( "as-server" ) ;
}

bool Main::Configuration::verbose() const
{
	return m_map.contains( "verbose" ) ;
}

bool Main::Configuration::debug() const
{
	return m_map.contains( "debug" ) ;
}

bool Main::Configuration::useSyslog() const
{
	bool basic = !m_map.contains("no-syslog") && !m_map.contains("as-client") ;
	bool use_syslog_override = m_map.contains( "syslog" ) ;
	return use_syslog_override || basic ;
}

bool Main::Configuration::validSyslogFacility() const
{
	std::string s = m_map.value( "syslog" ) ;
	return s.empty() || s == G::Str::positive() ||
		s == "mail" || s == "user" || s == "daemon" ||
		( s.length() == 6U && s.find("local") == 0U &&
			( s.at(5U) >= '0' && s.at(5U) <= '7' ) ) ;
}

std::string Main::Configuration::validSyslogFacilities() const
{
	return "mail, user, daemon, local0..7" ;
}

G::LogOutput::SyslogFacility Main::Configuration::syslogFacility() const
{
	std::string s = m_map.value( "syslog" ) ;
	if( s == "user" ) return G::LogOutput::SyslogFacility::User ;
	if( s == "daemon" ) return G::LogOutput::SyslogFacility::Daemon ;
	if( s == "local0" ) return G::LogOutput::SyslogFacility::Local0 ;
	if( s == "local1" ) return G::LogOutput::SyslogFacility::Local1 ;
	if( s == "local2" ) return G::LogOutput::SyslogFacility::Local2 ;
	if( s == "local3" ) return G::LogOutput::SyslogFacility::Local3 ;
	if( s == "local4" ) return G::LogOutput::SyslogFacility::Local4 ;
	if( s == "local5" ) return G::LogOutput::SyslogFacility::Local5 ;
	if( s == "local6" ) return G::LogOutput::SyslogFacility::Local6 ;
	if( s == "local7" ) return G::LogOutput::SyslogFacility::Local7 ;
	return G::LogOutput::SyslogFacility::Mail ;
}

bool Main::Configuration::logTimestamp() const
{
	return m_map.contains( "log-time" ) ;
}

bool Main::Configuration::logAddress() const
{
	return m_map.contains( "log-address" ) ;
}

G::Path Main::Configuration::logFile() const
{
	return m_map.contains("log-file") ? pathValue("log-file") : G::Path() ;
}

unsigned int Main::Configuration::port() const
{
	return G::Str::toUInt( m_map.value( "port" , "25" ) ) ;
}

bool Main::Configuration::closeFiles() const
{
	return daemon() && m_map.value("interface").find("fd#") == std::string::npos ;
}

G::StringArray Main::Configuration::listeningNames( const std::string & protocol ) const
{
	// allow eg. "127.0.0.1,smtp=192.168.1.1,admin=10.0.0.1,pop=fd#4"

	// prepare the naive result by splitting the user's string by commas
	G::StringArray result = G::Str::splitIntoFields( m_map.value("interface") , ',' ) ;

	// then weed out the ones that have an explicit protocol name that doesnt match the
	// required protocol, removing the "protocol=" prefix at the same time to leave just
	// the required list of addresses
	for( auto p = result.begin() ; p != result.end() ; )
	{
		if( protocol.empty() || protocol == G::Str::head( *p , (*p).find('=') , protocol ) )
		{
			*p = G::Str::tail( *p , (*p).find('=') , *p ) ;
			++p ;
		}
		else
		{
			p = result.erase( p ) ;
		}
	}

	return result ;
}

std::string Main::Configuration::clientBindAddress() const
{
	return m_map.value( "client-interface" ) ;
}

unsigned int Main::Configuration::adminPort() const
{
	return G::Str::toUInt( m_map.value( "admin" , "0" ) ) ;
}

bool Main::Configuration::closeStderr() const
{
	return
		m_map.contains("close-stderr") ||
		m_map.contains("as-proxy") ||
		m_map.contains("as-server") ;
}

bool Main::Configuration::nodaemon() const
{
	return m_map.contains("no-daemon") || m_map.contains("as-client") ;
}

bool Main::Configuration::daemon() const
{
	return !m_map.contains("no-daemon") && !m_map.contains("as-client") ;
}

std::string Main::Configuration::dnsbl() const
{
	return m_map.value( "dnsbl" ) ;
}

std::string Main::Configuration::show() const
{
	return m_map.value( "show" ) ;
}

bool Main::Configuration::show( const std::string & key ) const
{
	return (","+show()+",").find(","+key+",") != std::string::npos ;
}

bool Main::Configuration::hidden() const
{
	return m_map.contains( "hidden" ) ; // also test for show("hidden")
}

G::Path Main::Configuration::spoolDir() const
{
	return m_map.contains("spool-dir") ? pathValue("spool-dir") : GSmtp::MessageStore::defaultDirectory() ;
}

std::string Main::Configuration::serverAddress() const
{
	const char * key = "forward-to" ;
	if( m_map.contains("as-client") )
		key = "as-client" ;
	else if( m_map.contains("as-proxy") )
		key = "as-proxy" ;
	return m_map.value( key ) ;
}

bool Main::Configuration::forwardOnStartup() const
{
	return m_map.contains( "forward" ) || m_map.contains( "as-client" ) ;
}

bool Main::Configuration::forwardToSome() const
{
	return m_map.contains( "forward-to-some" ) ;
}

bool Main::Configuration::dontServe() const
{
	return m_map.contains( "dont-serve" ) || m_map.contains( "as-client" ) ;
}

bool Main::Configuration::doServing() const
{
	return !m_map.contains( "dont-serve" ) && !m_map.contains( "as-client" ) ;
}

bool Main::Configuration::immediate() const
{
	return m_map.contains( "immediate" ) ;
}

bool Main::Configuration::doPolling() const
{
	return m_map.contains( "poll" ) ;
}

G::TimeInterval Main::Configuration::pollingTimeout() const
{
	std::string value = m_map.value( "poll" , "0" ) ;
	if( G::Str::tailMatch(value,"ms"_sv) )
		return G::TimeInterval( 0U , 1000U * G::Str::toUInt( G::Str::head(value,"ms"_sv) ) ) ;
	else
		return G::TimeInterval( G::Str::toUInt( value ) ) ;
}

bool Main::Configuration::pollingLog() const
{
	// dont log if polling very frequently
	return doPolling() && pollingTimeout() > G::TimeInterval(60U) ;
}

bool Main::Configuration::forwardOnDisconnect() const
{
	return
		m_map.contains( "forward-on-disconnect" ) ||
		m_map.contains( "as-proxy" ) ;
}

unsigned int Main::Configuration::promptTimeout() const
{
	return G::Str::toUInt( m_map.value( "prompt-timeout" , "20" ) ) ;
}

bool Main::Configuration::doSmtp() const
{
	return ! m_map.contains( "no-smtp" ) ;
}

bool Main::Configuration::doPop() const
{
	return m_map.contains( "pop" ) ;
}

bool Main::Configuration::popByName() const
{
	return m_map.contains( "pop-by-name" ) ;
}

bool Main::Configuration::popNoDelete() const
{
	return m_map.contains( "pop-no-delete" ) ;
}

unsigned int Main::Configuration::popPort() const
{
	return G::Str::toUInt( m_map.value( "pop-port" , "110" ) ) ;
}

bool Main::Configuration::allowRemoteClients() const
{
	return m_map.contains( "remote-clients" ) ;
}

bool Main::Configuration::doAdmin() const
{
	return m_map.contains( "admin" ) ;
}

bool Main::Configuration::usePidFile() const
{
	return m_map.contains( "pid-file" ) ;
}

G::Path Main::Configuration::pidFile() const
{
	return pathValue( "pid-file" ) ;
}

bool Main::Configuration::useFilter() const
{
	return m_map.contains( "filter" ) ;
}

GSmtp::FactoryParser::Result Main::Configuration::filter() const
{
	return specValue( "filter" , true ) ;
}

GSmtp::FactoryParser::Result Main::Configuration::clientFilter() const
{
	return specValue( "client-filter" , true ) ;
}

G::Path Main::Configuration::clientSecretsFile() const
{
	return m_map.contains("client-auth") ? pathValue("client-auth") : G::Path() ;
}

std::string Main::Configuration::smtpSaslClientConfig() const
{
	return m_map.value( "client-auth-config" ) ;
}

std::string Main::Configuration::smtpSaslServerConfig() const
{
	return m_map.value( "server-auth-config" ) ;
}

std::string Main::Configuration::popSaslServerConfig() const
{
	return m_map.value( "server-auth-config" ) ; // moot
}

bool Main::Configuration::clientTls() const
{
	return m_map.contains( "client-tls" ) ;
}

bool Main::Configuration::clientTlsRequired() const
{
	return m_map.contains( "client-tls-required" ) ;
}

bool Main::Configuration::clientOverTls() const
{
	return m_map.contains( "client-tls-connection" ) ;
}

bool Main::Configuration::serverTls() const
{
	return m_map.contains( "server-tls" ) ;
}

bool Main::Configuration::serverTlsConnection() const
{
	return m_map.contains( "server-tls-connection" ) ;
}

bool Main::Configuration::serverTlsRequired() const
{
	return m_map.contains( "server-tls-required" ) ;
}

std::string Main::Configuration::tlsConfig() const
{
	return m_map.value( "tls-config" ) ;
}

G::Path Main::Configuration::serverTlsPrivateKey() const
{
	return keyFile( "server-tls-certificate" ) ;
}

G::Path Main::Configuration::serverTlsCertificate() const
{
	return certificateFile( "server-tls-certificate" ) ;
}

G::Path Main::Configuration::serverTlsCaList() const
{
	return m_map.contains("server-tls-verify") ? pathValue("server-tls-verify") : G::Path() ;
}

std::string Main::Configuration::clientTlsPeerCertificateName() const
{
	return m_map.value( "client-tls-verify-name" ) ;
}

std::string Main::Configuration::clientTlsPeerHostName() const
{
	return m_map.value( "client-tls-server-name" ) ;
}

G::Path Main::Configuration::clientTlsPrivateKey() const
{
	return keyFile( "client-tls-certificate" ) ;
}

G::Path Main::Configuration::clientTlsCertificate() const
{
	return certificateFile( "client-tls-certificate" ) ;
}

G::Path Main::Configuration::keyFile( const std::string & option_name ) const
{
	std::string value = m_map.value( option_name ) ;
	return value.empty() ? G::Path() : pathValueImp( G::Str::head(value,",",false) ) ;
}

G::Path Main::Configuration::certificateFile( const std::string & option_name ) const
{
	std::string value = m_map.value( option_name ) ;
	return value.empty() ? G::Path() : pathValueImp( G::Str::tail(value,",",false) ) ;
}

G::Path Main::Configuration::clientTlsCaList() const
{
	return m_map.contains("client-tls-verify") ? pathValue("client-tls-verify") : G::Path() ;
}

G::Path Main::Configuration::popSecretsFile() const
{
	return m_map.contains("pop-auth") ? pathValue("pop-auth") : G::Path() ;
}

G::Path Main::Configuration::serverSecretsFile() const
{
	return m_map.contains("server-auth") ? pathValue("server-auth") : G::Path() ;
}

unsigned int Main::Configuration::responseTimeout() const
{
	return G::Str::toUInt( m_map.value( "response-timeout" , "1800" ) ) ;
}

unsigned int Main::Configuration::idleTimeout() const
{
	return G::Str::toUInt( m_map.value( "idle-timeout" , "1800" ) ) ;
}

unsigned int Main::Configuration::connectionTimeout() const
{
	return G::Str::toUInt( m_map.value( "connection-timeout" , "40" ) ) ;
}

unsigned int Main::Configuration::secureConnectionTimeout() const
{
	return connectionTimeout() ;
}

std::string Main::Configuration::networkName( const std::string & default_ ) const
{
	return m_map.value( "domain" , default_ ) ;
}

std::string Main::Configuration::user() const
{
	return m_map.value( "user" , "daemon" ) ;
}

GSmtp::FactoryParser::Result Main::Configuration::verifier() const
{
	return specValue( "address-verifier" , false ) ;
}

bool Main::Configuration::withTerminate() const
{
	return m_map.contains( "admin-terminate" ) ;
}

unsigned int Main::Configuration::maxSize() const
{
	return G::Str::toUInt( m_map.value( "size" , "0" ) ) ;
}

bool Main::Configuration::eightBitTest() const
{
	return true ;
}

std::string Main::Configuration::scannerAddress() const
{
	return m_map.value( "scanner" ) ;
}

unsigned int Main::Configuration::scannerConnectionTimeout() const
{
	return 10U ; // for now
}

unsigned int Main::Configuration::scannerResponseTimeout() const
{
	return 90U ; // for now
}

bool Main::Configuration::anonymous( G::string_view type ) const
{
	if( !m_map.contains("anonymous") )
		return false ;

	std::string value = m_map.value( "anonymous" ) ;
	if( value.empty() )
		return true ;

	for( G::StringField f(value,",",1U) ; f ; ++f )
	{
		if( f() == type )
			return true ;
	}
	return false ;
}

bool Main::Configuration::anonymousServerVrfy() const
{
	return anonymous( "vrfy" ) ;
}

bool Main::Configuration::anonymousServerSmtp() const
{
	return anonymous( "server" ) ;
}

bool Main::Configuration::anonymousContent() const
{
	return anonymous( "content" ) ;
}

bool Main::Configuration::anonymousClientSmtp() const
{
	return anonymous( "client" ) ;
}

bool Main::Configuration::smtpPipelining() const
{
	// allow broken clients by default
	return m_map.value( "test" ) != "smtp-no-pipelining" ;
}

unsigned int Main::Configuration::filterTimeout() const
{
	return G::Str::toUInt( m_map.value( "filter-timeout" , "300" ) ) ;
}

std::string Main::Configuration::semanticError() const
{
	// (code size optimisation)
	std::string s ;
	const char * p = semanticErrorImp( s ) ;
	return (p && *p) ? std::string(p) : ( p ? s : std::string() ) ;
}

const char * Main::Configuration::semanticErrorImp( std::string & s ) const
{
	using G::txt ;
	using G::format ;

	if( m_map.contains("syslog") && !validSyslogFacility() )
	{
		s.assign( str( format(txt("invalid --syslog facility [%1%]: use %2%")) % m_map.value("syslog") % validSyslogFacilities() ) ) ;
		return "" ;
	}

	if( m_map.contains("poll") && pollingTimeout() == G::TimeInterval(0U) )
	{
		return txt("invalid --poll period: try --forward-on-disconnect") ;
	}

	if( ! m_map.contains("pop") && (
		m_map.contains("pop-port") ||
		m_map.contains("pop-auth") ||
		m_map.contains("pop-by-name") ||
		m_map.contains("pop-no-delete") ) )
	{
		return txt("pop options require --pop") ;
	}

	if( m_map.contains("pop") && !m_map.contains("pop-auth") )
	{
		return txt("the --pop option requires --pop-auth") ;
	}

	if( m_map.contains("admin-terminate") && !m_map.contains("admin") )
	{
		return txt("the --admin-terminate option requires --admin") ;
	}

	if( daemon() && spoolDir().isRelative() )
	{
		// (in case m_base_dir is relative)
		return txt("in daemon mode the spool-dir must be an absolute path") ;
	}

	if( daemon() && (
		( m_map.contains("client-auth") && ( clientSecretsFile().empty() || clientSecretsFile().isRelative() ) ) ||
		( m_map.contains("server-auth") && ( serverSecretsFile().empty() || serverSecretsFile().isRelative() ) ) ||
		( m_map.contains("pop-auth") && ( popSecretsFile().empty() || popSecretsFile().isRelative() ) ) ) )
	{
		// (in case m_base_dir is relative)
		return txt("in daemon mode the authorisation secrets file(s) must be absolute paths") ;
	}

	if( m_map.contains("forward-to") && ( m_map.contains("as-proxy") || m_map.contains("as-client") ) )
	{
		return txt("--forward-to cannot be used with --as-client or --as-proxy") ;
	}

	const bool have_forward_to =
		m_map.contains("as-proxy") || // => forward-to
		m_map.contains("as-client") || // => forward-to
		m_map.contains("forward-to") ;

	{
		std::array<const char*,5U> need_forward_to {{
			"forward" ,
			"poll" ,
			"forward-on-disconnect" ,
			"immediate" ,
			"client-filter" }} ;

		for( const char * p : need_forward_to )
		{
			if( m_map.contains(p) && !have_forward_to )
			{
				s.assign( str( format(txt("%1% requires --forward-to")) % ("--"+std::string(p)) ) ) ;
				return "" ;
			}
		}
	}

	const bool forwarding =
		m_map.contains("admin") ||
		m_map.contains("forward") ||
		m_map.contains("forward-on-disconnect") ||
		m_map.contains("immediate") ||
		m_map.contains("poll") ;

	if( m_map.contains("forward-to") && !forwarding )
	{
		// ignore this -- we want the configuration gui to be able to set the
		// forwarding address even if it's not used and we might need it for
		// filter exit code 103 (rescan)
	}

	const bool not_serving = m_map.contains("dont-serve") || m_map.contains("as-client") ;

	if( not_serving ) // ie. if not serving admin, smtp or pop
	{
		if( m_map.contains("filter") )
			return txt("the --filter option cannot be used with --as-client or --dont-serve") ;

		if( m_map.contains("port") )
			return txt("the --port option cannot be used with --as-client or --dont-serve") ;

		if( m_map.contains("server-auth") )
			return txt("the --server-auth option cannot be used with --as-client or --dont-serve") ;

		if( m_map.contains("pop") )
			return txt("the --pop option cannot be used with --as-client or --dont-serve") ;

		if( m_map.contains("admin") )
			return txt("the --admin option cannot be used with --as-client or --dont-serve") ;

		if( m_map.contains("poll") )
			return txt("the --poll option cannot be used with --as-client or --dont-serve") ;
	}

	if( m_map.contains("no-smtp") ) // ie. if not serving smtp
	{
		if( m_map.contains("filter") )
			return txt("the --filter option cannot be used with --no-smtp") ;

		if( m_map.contains("port") )
			return txt("the --port option cannot be used with --no-smtp") ;

		if( m_map.contains("server-auth") )
			return txt("the --server-auth option cannot be used with --no-smtp") ;
	}

	if( m_map.contains("server-auth-config") && !m_map.contains("server-auth") )
	{
		return txt("--server-auth-config requires --server-auth") ;
	}

	if( m_map.contains("client-auth-config") && !m_map.contains("client-auth") )
	{
		return txt("--client-auth-config requires --client-auth") ;
	}

	const bool contains_log =
		m_map.contains("log") ||
		m_map.contains("as-server") || // => log
		m_map.contains("as-client") || // => log
		m_map.contains("as-proxy") ; // => log

	if( m_map.contains("verbose") && ! ( m_map.contains("help") || contains_log ) )
	{
		return txt("the --verbose option must be used with --log, --help, --as-client, --as-server or --as-proxy") ;
	}

	if( m_map.contains("debug") && !contains_log )
	{
		return txt("the --debug option requires --log, --as-client, --as-server or --as-proxy") ;
	}

	if( m_map.contains("client-tls") && m_map.contains("client-tls-connection") )
	{
		return txt("the --client-tls and --client-tls-connection options cannot be used together") ;
	}

	if( m_map.contains("server-tls") && m_map.contains("server-tls-connection") )
	{
		return txt("the --server-tls and --server-tls-connection options cannot be used together") ;
	}

	if( m_map.contains("server-tls") && !m_map.contains("server-tls-certificate") )
	{
		return txt("the --server-tls option requires --server-tls-certificate") ;
	}

	if( m_map.contains("server-tls-connection") && !m_map.contains("server-tls-certificate") )
	{
		return txt("the --server-tls-connection option requires --server-tls-certificate") ;
	}

	if( ( m_map.contains("server-tls-certificate") || m_map.contains("server-tls-verify") ) &&
		!( m_map.contains("server-tls") || m_map.contains("server-tls-connection") ) )
	{
		return txt("the --server-tls options require either --server-tls or --server-tls-connection") ;
	}

	if( ( m_map.contains("client-tls-certificate") || m_map.contains("client-tls-verify") ||
		m_map.contains("client-tls-required") ) &&
		!( m_map.contains("client-tls") || m_map.contains("client-tls-connection") ) )
	{
		return txt("the --client-tls- options require --client-tls or --client-tls-connection") ;
	}

	if( m_map.count("server-tls-certificate") > 2U )
	{
		return txt("the --server-tls-certificate option cannot be used more than twice") ;
	}

	if( m_map.count("client-tls-certificate") > 2U )
	{
		return txt("the --client-tls-certificate option cannot be used more than twice") ;
	}

	if( m_map.contains("client-tls-verify-name") && !m_map.contains("client-tls-verify") )
	{
		return txt("the --client-tls-verify-name options requires --client-tls-verify") ;
	}

	if( m_map.contains("server-tls-required") &&
		!( m_map.contains("server-tls" ) || m_map.contains("server-tls-connection") ) )
	{
		return txt("--server-tls-required requires --server-tls or --server-tls-connection") ;
	}

	if( m_map.contains("interface") && m_map.value("interface").find("client=") != std::string::npos )
	{
		return txt("using --interface with client= is no longer supported: use --client-interface instead") ;
	}

	if( m_map.contains("dnsbl") && !m_map.contains("remote-clients") )
	{
		return txt("--dnsbl requires --remote-clients or -r") ;
	}

	std::string forward_to = serverAddress() ;

	if( m_map.contains("client-interface") && GNet::Address::isFamilyLocal(forward_to) )
	{
		return txt("cannot use --client-interface with a unix-domain forwarding address") ;
	}

	{
		GSmtp::FactoryParser::Result r ;
		if( (r=filter()).first.empty() )
		{
			s.assign( str( format(txt("invalid filter specification: %1%")) % r.second ) ) ;
			return "" ;
		}
		if( (r=clientFilter()).first.empty() )
		{
			s.assign( str( format(txt("invalid client filter specification: %1%")) % r.second ) ) ;
			return "" ;
		}
		if( (r=verifier()).first.empty() )
		{
			s.assign( str( format(txt("invalid address verifier specification: %1%")) % r.second ) ) ;
			return "" ;
		}
	}

	return nullptr ;
}

G::StringArray Main::Configuration::semanticWarnings() const
{
	using G::txt ;
	using G::format ;

	G::StringArray warnings ;

	const bool no_syslog =
		m_map.contains("no-syslog") ||
		m_map.contains("as-client") ;

	const bool syslog =
		! ( no_syslog && ! m_map.contains("syslog") ) ;

	const bool close_stderr =
		m_map.contains("close-stderr") ||
		m_map.contains("as-server") ||
		m_map.contains("as-proxy") ;

	const bool contains_log =
		m_map.contains("log") ||
		m_map.contains("as-server") || // => log
		m_map.contains("as-client") || // => log
		m_map.contains("as-proxy") ; // => log

	if( contains_log && close_stderr && !m_map.contains("log-file") && !syslog ) // ie. logging to nowhere
	{
		std::string close_stderr_option =
			( m_map.contains("close-stderr") ? "--close-stderr" :
			( m_map.contains("as-server") ? "--as-server" :
			"--as-proxy" ) ) ;

		warnings.push_back( str( format(
			txt("logging will stop because %1% closes the standard error stream soon after startup")) %
				close_stderr_option ) ) ;
	}

	const bool no_daemon =
		m_map.contains("as-client") || // => no-daemon
		m_map.contains("no-daemon") ;

	if( m_map.contains("show") && ( no_daemon || m_map.contains("hidden") ) ) // (windows)
	{
		warnings.push_back(
			txt("the --show option is ignored when using --no-daemon, --as-client or --hidden") ) ;
	}

	specValue( "filter" , true , &warnings ) ;
	specValue( "client-filter" , true , &warnings ) ;
	specValue( "address-verifier" , false , &warnings ) ;

	return warnings ;
}

GSmtp::FactoryParser::Result Main::Configuration::specValue( const std::string & option_name , bool is_filter ,
	G::StringArray * warnings_p ) const
{
	std::string value = m_map.value( option_name ) ;
	return GSmtp::FactoryParser::parse( value , is_filter ,
		m_base_dir.str() , m_app_dir.str() , warnings_p ) ;
}

G::Path Main::Configuration::pathValue( const std::string & option_name ) const
{
 	std::string value = m_map.value( option_name ) ;
	if( verifyType(option_name) && specialVerifyValue(value) ) // dont mess "<none>" etc. in "--tls-client-verify=<none>"
		return value ;
	else
		return pathValueImp( value ) ;
}

G::Path Main::Configuration::pathValueImp( const std::string & value_in ) const
{
	// (see also GSmtp::FactoryParser::parse())
	std::string value = value_in ;
	if( value.find("@app") == 0U && !m_app_dir.empty() )
		G::Str::replace( value , "@app" , m_app_dir.str() ) ;
	return G::Path(value).isAbsolute() ? G::Path(value) : ( daemon() ? (m_base_dir+value) : value ) ;
}

bool Main::Configuration::verifyType( const std::string & option_name )
{
	return
		option_name == "server-tls-verify" ||
		option_name == "client-tls-verify" ;
}

bool Main::Configuration::specialVerifyValue( const std::string & value )
{
	return !value.empty() && value.at(0U) == '<' && value.at(value.length()-1U) == '>' ;
}

#ifdef G_WINDOWS
bool Main::Configuration::pathlike( const std::string & option_name )
{
	return
		option_name == "log-file" ||
		option_name == "spool-dir" ||
		option_name == "pid-file" ||
		option_name == "client-auth" ||
		option_name == "server-tls-certificate" ||
		option_name == "server-tls-verify" || // verifyType()
		option_name == "client-tls-certificate" ||
		option_name == "client-tls-verify" || // verifyType()
		option_name == "pop-auth" ||
		option_name == "server-auth" ||
		false ;
}
#endif

#ifdef G_WINDOWS
G::StringArray Main::Configuration::display() const
{
	const std::string yes = "yes" ;
	const std::string no = "no" ;
	G::StringArray result ;
	result.reserve( 70U ) ;
	for( const auto & option : m_options )
	{
		if( option.visible() &&
			option.name != "version" &&
			option.name != "help" &&
			option.name != "no-syslog" &&
			option.name.find("as-") != 0U &&
			true )
		{
			result.push_back( option.name ) ;
			if( option.name == "log" )
				result.push_back( log()?yes:no ) ;
			else if( option.name == "syslog" )
				result.push_back( useSyslog()?yes:no ) ;
			else if( option.name == "spool-dir" )
				result.push_back( spoolDir().str() ) ;
			else if( option.name == "close-stderr" )
				result.push_back( closeStderr()?yes:no ) ;
			else if( option.name == "no-daemon" )
				result.back() = "daemon" , result.push_back( daemon()?yes:no ) ;
			else if( option.name == "dont-serve" )
				result.back() = "serve" , result.push_back( doServing()?yes:no ) ;
			else if( option.name == "forward" )
				result.push_back( forwardOnStartup()?yes:no ) ;
			else if( option.name == "forward-on-disconnect" )
				result.push_back( forwardOnDisconnect()?yes:no ) ;
			else if( option.name == "forward-to" )
				result.push_back( serverAddress() ) ;
			else if( option.name == "port" )
				result.push_back( (doSmtp()&&doServing()) ? G::Str::fromUInt(port()) : std::string() ) ;
			else if( option.name == "pop-port" )
				result.push_back( (doPop()&&doServing()) ? G::Str::fromUInt(popPort()) : std::string() ) ;
			else if( option.multivalued() )
				result.push_back( m_map.value(option.name) ) ;
			else if( option.valued() && pathlike(option.name) )
				result.push_back( m_map.contains(option.name) ? pathValue(option.name).str() : std::string() ) ;
			else if( option.valued() )
				result.push_back( m_map.value(option.name) ) ;
			else
				result.push_back( m_map.contains(option.name) ? yes : no ) ;
		}
	}
	return result ;
}
#endif

