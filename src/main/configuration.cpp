//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gprocess.h"
#include "gstrings.h"
#include "gdebug.h"

namespace
{
	bool fixed_up = false ;
}

G::Arg Main::Configuration::backwardsCompatibilityFixup( const G::Arg & arg )
{
	// TODO remove backwards compatibility
	bool has_server_tls = false ;
	bool has_server_tls_certificate = false ;
	for( size_t i = 1U ; i < arg.c() ; i++ )
	{
		if( arg.v(i) == "--server-tls" || arg.v(i) == "-K" || arg.v(i).find("--server-tls=") == 0U )
			has_server_tls = true ;
		if( arg.v(i) == "--server-tls-certificate" || arg.v(i).find("--server-tls-certificate=") == 0U )
			has_server_tls_certificate = true ;
	}
	if( has_server_tls && !has_server_tls_certificate )
	{
		for( size_t i = 1U ; i < arg.c() ; i++ )
		{
			if( ( ( arg.v(i) == "-K" && (i+1U) < arg.c() ) ||
				( arg.v(i) == "--server-tls" && (i+1U) < arg.c() ) ) &&
				( !arg.v(i+1U).empty() && arg.v(i+1U).at(0U) != '-' ) )
			{
				G::StringArray args = arg.array() ;
				args.insert( args.begin()+i+1 , "--server-tls-certificate" ) ;
				fixed_up = true ;
				return G::Arg( args ) ;
			}
			else if( arg.v(i).find("--server-tls=") == 0U && (i+1U) < arg.c() )
			{
				G::StringArray args = arg.array() ;
				args.insert( args.begin()+i , "--server-tls" ) ;
				G::Str::replace( args[i+1U] , "server-tls" , "server-tls-certificate" ) ;
				fixed_up = true ;
				return G::Arg( args ) ;
			}
		}
	}
	return arg ;
}

Main::Configuration::Configuration( const G::Options & options , const G::OptionMap & map ,
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

bool Main::Configuration::logTimestamp() const
{
	return m_map.contains( "log-time" ) ;
}

G::Path Main::Configuration::logFile() const
{
	return m_map.contains("log-file") ? pathValue("log-file") : G::Path() ;
}

unsigned int Main::Configuration::port() const
{
	return G::Str::toUInt( m_map.value( "port" , "25" ) ) ;
}

G::StringArray Main::Configuration::listeningAddresses( const std::string & protocol ) const
{
	// allow eg. "127.0.0.1,smtp=192.168.1.1,admin=10.0.0.1"

	// prepare the naive result by splitting the user's string by commas
	G::StringArray result = G::Str::splitIntoFields( m_map.value("interface") , "," ) ;

	// then weed out the ones that have an explicit protocol name that doesnt match the
	// required protocol, removing the "protocol=" prefix at the same time to leave just
	// the required list of addresses
	for( G::StringArray::iterator p = result.begin() ; p != result.end() ; )
	{
		if( protocol.empty() || protocol == G::Str::head( *p , (*p).find('=') , protocol ) )
		{
			*p = G::Str::tail( *p , (*p).find('=') , *p ) ;
			p++ ;
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
	// "--interface client=..." no longer supported, just "--client-interface"
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

bool Main::Configuration::daemon() const
{
	return !m_map.contains("no-daemon") && !m_map.contains("as-client") ;
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
	return m_map.contains("forward") || m_map.contains("as-client") ;
}

bool Main::Configuration::dontServe() const
{
	return m_map.contains("dont-serve") || m_map.contains("as-client") ;
}

bool Main::Configuration::doServing() const
{
	return !m_map.contains("dont-serve") && !m_map.contains("as-client") ;
}

bool Main::Configuration::immediate() const
{
	return m_map.contains("immediate") ;
}

bool Main::Configuration::doPolling() const
{
	return m_map.contains("poll") && pollingTimeout() > 0U ;
}

unsigned int Main::Configuration::pollingTimeout() const
{
	return G::Str::toUInt( m_map.value( "poll" , "0" ) ) ;
}

bool Main::Configuration::pollingLog() const
{
	// dont log if polling very frequently
	return doPolling() && pollingTimeout() > 60U ;
}

bool Main::Configuration::forwardOnDisconnect() const
{
	return
		( m_map.contains("poll") && pollingTimeout() == 0U ) || // TODO remove backwards compatibility
		m_map.contains("forward-on-disconnect") ||
		m_map.contains("as-proxy") ;
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

G::Path Main::Configuration::filter() const
{
	return m_map.contains("filter") ? pathValue("filter") : G::Path() ;
}

G::Path Main::Configuration::clientFilter() const
{
	return m_map.contains("client-filter") ? pathValue("client-filter") : G::Path() ;
}

bool Main::Configuration::hidden() const
{
	return m_map.contains( "hidden" ) ;
}

G::Path Main::Configuration::clientSecretsFile() const
{
	return m_map.contains("client-auth") ? pathValue("client-auth") : G::Path() ;
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

bool Main::Configuration::serverTlsRequired() const
{
	return m_map.contains( "server-tls-required" ) ;
}

std::string Main::Configuration::tlsConfig() const
{
	return m_map.value( "tls-config" ) ;
}

G::Path Main::Configuration::serverTlsCertificate() const
{
	return m_map.contains("server-tls-certificate") ? pathValue("server-tls-certificate") : G::Path() ;
}

G::Path Main::Configuration::serverTlsCaList() const
{
	return m_map.contains("server-tls-verify") ? pathValue("server-tls-verify") : G::Path() ;
}

std::string Main::Configuration::clientTlsPeerCertificateName() const
{
	return m_map.value("client-tls-verify-name") ;
}

std::string Main::Configuration::clientTlsPeerHostName() const
{
	return m_map.value("client-tls-server-name") ;
}

G::Path Main::Configuration::clientTlsCertificate() const
{
	return m_map.contains("client-tls-certificate") ? pathValue("client-tls-certificate") : G::Path() ;
}

G::Path Main::Configuration::clientTlsCaList() const
{
	return m_map.contains("client-tls-verify") ? pathValue("client-tls-verify") : G::Path() ;
}

G::Path Main::Configuration::popSecretsFile() const
{
	return m_map.contains("pop-auth") ? pathValue("pop-auth") : G::Path(GPop::Secrets::defaultPath()) ;
}

G::Path Main::Configuration::serverSecretsFile() const
{
	return m_map.contains("server-auth") ? pathValue("server-auth") : G::Path() ;
}

unsigned int Main::Configuration::responseTimeout() const
{
	return G::Str::toUInt( m_map.value( "response-timeout" , "1800" ) ) ;
}

unsigned int Main::Configuration::connectionTimeout() const
{
	return G::Str::toUInt( m_map.value( "connection-timeout" , "40" ) ) ;
}

unsigned int Main::Configuration::secureConnectionTimeout() const
{
	return connectionTimeout() ;
}

std::string Main::Configuration::networkName( std::string default_ ) const
{
	return m_map.value( "domain" , default_ ) ;
}

std::string Main::Configuration::nobody() const
{
	return m_map.value( "user" , "daemon" ) ;
}

G::Path Main::Configuration::verifier() const
{
	return
		m_map.contains("address-verifier") ?
			pathValue( "address-verifier" ) :
			( m_map.contains("verifier") ? pathValue("verifier") : G::Path() ) ;
}

bool Main::Configuration::verifierCompatibility() const
{
	return m_map.contains( "verifier" ) ;
}

bool Main::Configuration::withTerminate() const
{
	return m_map.contains( "admin-terminate" ) ;
}

unsigned int Main::Configuration::maxSize() const
{
	return G::Str::toUInt( m_map.value( "size" , "0" ) ) ;
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

bool Main::Configuration::anonymous() const
{
	return m_map.contains( "anonymous" ) ;
}

unsigned int Main::Configuration::filterTimeout() const
{
	return G::Str::toUInt( m_map.value( "filter-timeout" , "300" ) ) ;
}

G::StringArray Main::Configuration::semanticWarnings() const
{
	return semantics( false ) ;
}

std::string Main::Configuration::semanticError() const
{
	G::StringArray errors = semantics( true ) ;
	return errors.empty() ? std::string() : errors.at(0U) ;
}

G::StringArray Main::Configuration::semantics( bool want_errors ) const
{
	G::StringArray errors ;
	G::StringArray warnings ;

	if(
		( m_map.contains("admin") && adminPort() == port() ) ||
		( m_map.contains("pop") && popPort() == port() ) ||
		( m_map.contains("pop") && m_map.contains("admin") && popPort() == adminPort() ) )
	{
		errors.push_back( "the listening ports must be different" ) ;
	}

	if( ! m_map.contains("pop") && (
		m_map.contains("pop-port") ||
		m_map.contains("pop-auth") ||
		m_map.contains("pop-by-name") ||
		m_map.contains("pop-no-delete") ) )
	{
		errors.push_back( "pop options require --pop" ) ;
	}

	if( m_map.contains("admin-terminate") && !m_map.contains("admin") )
	{
		errors.push_back( "the --admin-terminate option requires --admin" ) ;
	}

	if( daemon() && spoolDir().isRelative() )
	{
		errors.push_back( "in daemon mode the spool-dir must be an absolute path" ) ;
	}

	if( daemon() && (
		( m_map.contains("client-auth") && ( clientSecretsFile() == G::Path() || clientSecretsFile().isRelative() ) ) ||
		( m_map.contains("server-auth") && ( serverSecretsFile() == G::Path() || serverSecretsFile().isRelative() ) ) ||
		( m_map.contains("pop-auth") && ( popSecretsFile() == G::Path() || popSecretsFile().isRelative() ) ) ) )
	{
		// (never gets here)
		errors.push_back( "in daemon mode the authorisation secrets file(s) must be absolute paths" ) ;
	}

	if( m_map.contains("forward-to") && ( m_map.contains("as-proxy") || m_map.contains("as-client") ) )
	{
		errors.push_back( "--forward-to cannot be used with --as-client or --as-proxy" ) ;
	}

	const bool have_forward_to =
		m_map.contains("as-proxy") || // => forward-to
		m_map.contains("as-client") || // => forward-to
		m_map.contains("forward-to") ;

	{
		const char * need_forward_to [] = {
			"forward" ,
			"poll" ,
			"forward-on-disconnect" ,
			"immediate" ,
			"client-filter" ,
			nullptr } ;

		for( const char ** p = need_forward_to ; *p ; p++ )
		{
			if( m_map.contains(*p) && !have_forward_to )
				errors.push_back( "--" + std::string(*p) + " requires --forward-to" ) ;
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
			errors.push_back( "the --filter option cannot be used with --as-client or --dont-serve" ) ;

		if( m_map.contains("port") )
			errors.push_back( "the --port option cannot be used with --as-client or --dont-serve" ) ;

		if( m_map.contains("server-auth") )
			errors.push_back( "the --server-auth option cannot be used with --as-client or --dont-serve" ) ;

		if( m_map.contains("pop") )
			errors.push_back( "the --pop option cannot be used with --as-client or --dont-serve" ) ;

		if( m_map.contains("admin") )
			errors.push_back( "the --admin option cannot be used with --as-client or --dont-serve" ) ;

		if( m_map.contains("poll") )
			errors.push_back( "the --poll option cannot be used with --as-client or --dont-serve" ) ;
	}

	if( m_map.contains("no-smtp") ) // ie. if not serving smtp
	{
		if( m_map.contains("filter") )
			errors.push_back( "the --filter option cannot be used with --no-smtp" ) ;

		if( m_map.contains("port") )
			errors.push_back( "the --port option cannot be used with --no-smtp" ) ;

		if( m_map.contains("server-auth") )
			errors.push_back( "the --server-auth option cannot be used with --no-smtp" ) ;
	}

	const bool contains_log =
		m_map.contains("log") ||
		m_map.contains("as-server") || // => log
		m_map.contains("as-client") || // => log
		m_map.contains("as-proxy") ; // => log

	if( m_map.contains("verbose") && ! ( m_map.contains("help") || contains_log ) )
	{
		errors.push_back( "the --verbose option must be used with --log, --help, "
			"--as-client, --as-server or --as-proxy" ) ;
	}

	if( m_map.contains("debug") && !contains_log )
	{
		errors.push_back( "the --debug option requires --log, --as-client, --as-server or --as-proxy" ) ;
	}

	const bool no_daemon =
		m_map.contains("as-client") || // => no-daemon
		m_map.contains("no-daemon") ;

	if( m_map.contains("hidden") && ! no_daemon ) // (win32)
	{
		errors.push_back( "the --hidden option requires --no-daemon or --as-client" ) ;
	}

	if( m_map.contains("client-tls") && m_map.contains("client-tls-connection") )
	{
		errors.push_back( "the --client-tls and --client-tls-connection options cannot be used together" ) ;
	}

	if( m_map.contains("server-tls") && !m_map.contains("server-tls-certificate") )
	{
		errors.push_back( "the --server-tls option requires --server-tls-certificate" ) ;
	}

	if( ( m_map.contains("server-tls-certificate") || m_map.contains("server-tls-verify") ) &&
		!m_map.contains("server-tls") )
	{
		errors.push_back( "the --server-tls-... options require --server-tls" ) ;
	}

	if( ( m_map.contains("client-tls-certificate") || m_map.contains("client-tls-verify") ||
		m_map.contains("client-tls-required") ) &&
		!( m_map.contains("client-tls") || m_map.contains("client-tls-connection") ) )
	{
		errors.push_back( "the --client-tls- options require --client-tls or --client-tls-connection" ) ;
	}

	if( m_map.contains("client-tls-verify-name") && !m_map.contains("client-tls-verify") )
	{
		errors.push_back( "the --client-tls-verify-name options requires --client-tls-verify" ) ;
	}

	if( m_map.contains("server-auth") && m_map.value("server-auth") == "/pam" && !m_map.contains("server-tls" ) )
	{
		errors.push_back( "--server-auth using pam requires --server-tls" ) ;
	}

	if( m_map.contains("server-tls-required") && !m_map.contains("server-tls") )
	{
		errors.push_back( "--server-tls-required requires --server-tls" ) ;
	}

	if( m_map.contains("pop-auth") && m_map.value("pop-auth") == "/pam" && !m_map.contains("server-tls" ) )
	{
		errors.push_back( "--pop-auth using pam requires --server-tls" ) ;
	}

	if( m_map.contains("interface") && m_map.value("interface").find("client=") != std::string::npos )
	{
		errors.push_back( "using --interface with client= is no longer supported: use --client-interface instead" ) ;
	}

	if( m_map.contains("verifier") && m_map.contains("address-verifier") )
	{
		errors.push_back( "use --address-verifier, not --verifier" ) ;
	}

	// warnings...

	const bool no_syslog =
		m_map.contains("no-syslog") ||
		m_map.contains("as-client") ;

	const bool syslog =
		! ( no_syslog && ! m_map.contains("syslog") ) ;

	const bool close_stderr =
		m_map.contains("close-stderr") ||
		m_map.contains("as-server") ||
		m_map.contains("as-proxy") ;

	if( contains_log && close_stderr && !m_map.contains("log-file") && !syslog ) // ie. logging to nowhere
	{
		std::string close_stderr_option =
			( m_map.contains("close-stderr") ? "--close-stderr" :
			( m_map.contains("as-server") ? "--as-server" :
			"--as-proxy" ) ) ;

		std::string warning = "logging will stop because " + close_stderr_option +
			" closes the standard error stream soon after startup" ;

		const bool help = false ; // moot
		if( help )
		{
			if( m_map.contains("as-server") && !m_map.contains("log") )
				warning = warning + ": replace --as-server with --log" ;
			else if( m_map.contains("as-server") )
				warning = warning + ": remove --as-server" ;
			else if( m_map.contains("as-proxy" ) )
				warning = warning + ": replace --as-proxy with --log --forward-on-disconnect --forward-to" ;
		}

		warnings.push_back( warning ) ;
	}

	if( m_map.contains("poll") && G::Str::toUInt(m_map.value("poll","1")) == 0U )
	{
		warnings.push_back( "use of --poll=0 is deprecated; replace with --forward-on-disconnect" ) ;
	}

	if( m_map.contains("verifier") ) // backwards compatibility
	{
		warnings.push_back( "use of --verifier is deprecated; replace with --address-verifier" ) ;
	}

	if( fixed_up )
	{
		warnings.push_back( "use of --server-tls with a filename is deprecated; use --server-tls-certificate" ) ;
	}

	return want_errors ? errors : warnings ;
}

G::Path Main::Configuration::pathValue( const std::string & option_name ) const
{
	std::string value = m_map.value( option_name ) ;
	if( compoundy(option_name) && compound(value) ) // dont mess with eg. "net:localhost:999"
    {
        return value ;
    }
    else
    {
		// for backwards compatibility only replace backslash-space with space
		// on windows -- old installer code used to put in the backslash when
		// writing the startup batch file, but it's not needed when properly
		// quoted
		if( G::is_windows() )
			G::Str::replaceAll( value , "\\ " , " " ) ; // TODO remove backwards compatibility

    	if( value.find("@app") == 0U && m_app_dir != G::Path() )
        	G::Str::replace( value , "@app" , m_app_dir.str() ) ;

        return G::Path(value).isAbsolute() ? G::Path(value) : ( m_base_dir + value ) ;
    }
}

bool Main::Configuration::pathlike( const std::string & option_name ) const
{
	return
		option_name == "log-file" ||
		option_name == "spool-dir" ||
		option_name == "pid-file" ||
		option_name == "filter" ||
		option_name == "client-filter" ||
		option_name == "client-auth" ||
        option_name == "server-tls-certificate" ||
        option_name == "server-tls-verify" ||
        option_name == "client-tls-certificate" ||
        option_name == "client-tls-verify" ||
        option_name == "pop-auth" ||
        option_name == "server-auth" ||
        option_name == "verifier" ||
        option_name == "address-verifier" ||
		false ;
}

bool Main::Configuration::compoundy( const std::string & option_name ) const
{
	return
		option_name == "filter" ||
		option_name == "client-filter" ||
		option_name == "verifier" ||
		option_name == "address-verifier" ;
}

bool Main::Configuration::compound( const std::string & value ) const
{
	return
		value.find(':') != std::string::npos &&
		value.find(':') >= 3U ;
}

G::StringArray Main::Configuration::display() const
{
	const std::string yes = "yes" ;
	const std::string no = "no" ;
	G::StringArray result ;
	result.reserve( 70U ) ;
	G::StringArray names = m_options.names() ;
	for( G::StringArray::iterator name_p = names.begin() ; name_p != names.end() ; ++name_p )
	{
		const std::string & name = *name_p ;
		if( m_options.visible(name,G::OptionsLevel(99),false) &&
			name != "version" &&
			name != "help" &&
			name != "no-syslog" &&
			name.find("as-") != 0U &&
			true )
		{
			result.push_back( name ) ;
			if( name == "log" )
				result.push_back( log()?yes:no ) ;
			else if( name == "syslog" )
				result.push_back( useSyslog()?yes:no ) ;
			else if( name == "spool-dir" )
				result.push_back( spoolDir().str() ) ;
			else if( name == "close-stderr" )
				result.push_back( closeStderr()?yes:no ) ;
			else if( name == "no-daemon" )
				result.back() = "daemon" , result.push_back( daemon()?yes:no ) ;
			else if( name == "dont-serve" )
				result.back() = "serve" , result.push_back( doServing()?yes:no ) ;
			else if( name == "forward" )
				result.push_back( forwardOnStartup()?yes:no ) ;
			else if( name == "forward-on-disconnect" )
				result.push_back( forwardOnDisconnect()?yes:no ) ;
			else if( name == "forward-to" )
				result.push_back( serverAddress() ) ;
			else if( name == "port" )
				result.push_back( (doSmtp()&&doServing()) ? G::Str::fromUInt(port()) : std::string() ) ;
			else if( name == "pop-port" )
				result.push_back( (doPop()&&doServing()) ? G::Str::fromUInt(popPort()) : std::string() ) ;
			else if( name == "address-verifier" ) // TODO remove backwards compatibility
				result.push_back( verifier().str() ) ;
			else if( m_options.multivalued(name) )
				result.push_back( m_map.value(name) ) ;
			else if( m_options.valued(name) && pathlike(name) )
				result.push_back( m_map.contains(name) ? pathValue(name).str() : std::string() ) ;
			else if( m_options.valued(name) )
				result.push_back( m_map.value(name) ) ;
			else
				result.push_back( m_map.contains(name) ? yes : no ) ;
		}
	}
	return result ;
}

/// \file configuration.cpp
