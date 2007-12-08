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
// commandline.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gsmtp.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "gpopsecrets.h"
#include "gstr.h"
#include "gdebug.h"

std::string Main::CommandLine::switchSpec( bool is_windows )
{
	std::string dir = GSmtp::MessageStore::defaultDirectory().str() ;
	std::string pop_auth = GPop::Secrets::defaultPath() ;
	std::ostringstream ss ;
	ss <<
		(is_windows?switchSpec_windows():switchSpec_unix()) << "|"
		"q!as-client!runs as a client, forwarding spooled mail to <host>: "
			"equivalent to \"--log --no-syslog --no-daemon --dont-serve --forward --forward-to\"!" 
			"1!host:port!1|"
		"d!as-server!runs as a server: equivalent to \"--log --close-stderr\"!0!!1|"
		"y!as-proxy!runs as a proxy: equivalent to \"--log --close-stderr --immediate --forward-to\"!"
			"1!host:port!1|"
		"v!verbose!generates more verbose output (works with --help and --log)!0!!1|"
		"h!help!displays help text and exits!0!!1|"
		""
		"p!port!specifies the smtp listening port number!1!port!2|"
		"r!remote-clients!allows remote clients to connect!0!!2|"
		"s!spool-dir!specifies the spool directory (default is \"" << dir << "\")!1!dir!2|"
		"V!version!displays version information and exits!0!!2|"
		""
		"j!client-tls!enables tls/ssl layer for smtp client (if openssl built in)!0!!3|"
		"K!server-tls!enables tls/ssl layer for smtp server using the given openssl certificate file (if openssl built in)!1!pem-file!3|"
		"g!debug!generates debug-level logging if compiled-in!0!!3|"
		"C!client-auth!enables smtp authentication with the remote server, using the given secrets file!1!file!3|"
		"L!log-time!adds a timestamp to the logging output!0!!3|"
		"S!server-auth!enables authentication of remote clients, using the given secrets file!1!file!3|"
		"e!close-stderr!closes the standard error stream soon after start-up!0!!3|"
		"a!admin!enables the administration interface and specifies its listening port number!"
			"1!admin-port!3|"
		"x!dont-serve!disables acting as a server on any port (part of --as-client and usually used with --forward)!0!!3|"
		"X!no-smtp!disables listening for smtp connections (usually used with --admin or --pop)!0!!3|"
		"z!filter!specifies an external program to process messages as they are stored!1!program!3|"
		"W!filter-timeout!sets the timeout (in seconds) for running the --filter processor (default is 300)!1!time!3|"
		"w!prompt-timeout!sets the timeout (in seconds) for getting an initial prompt from the server!1!time!3|"
		"D!domain!sets an override for the host's fully qualified domain name!1!fqdn!3|"
		"f!forward!forwards stored mail on startup (requires --forward-to)!0!!3|"
		"o!forward-to!specifies the remote smtp server (required by --forward, --poll, --immediate and --admin)!1!host:port!3|"
		"T!response-timeout!sets the response timeout (in seconds) when talking to a remote server "
			"(default is 1800)!1!time!3|"
		"U!connection-timeout!sets the timeout (in seconds) when connecting to a remote server "
			"(default is 40)!1!time!3|"
		"m!immediate!enables immediate forwarding of messages as soon as they are received (requires --forward-to)!0!!3|"
		"I!interface!defines the listening interface for incoming connections!1!ip-address!3|"
		"i!pid-file!defines a file for storing the daemon process-id!1!pid-file!3|"
		"O!poll!enables polling of the spool directory for messages to be forwarded with the specified period (requires --forward-to)!1!period!3|"
		"P!postmaster!!0!!0|"
		"Z!verifier!specifies an external program for address verification!1!program!3|"
		"Y!client-filter!specifies an external program to process messages when they are forwarded!1!program!3|"
		"Q!admin-terminate!enables the terminate command on the admin interface!0!!3|"
		"A!anonymous!disables the smtp vrfy command and sends less verbose smtp responses!0!!3|"
		"B!pop!enables the pop server!0!!3|"
		"E!pop-port!specifies the pop listening port number (requires --pop)!1!port!3|"
		"F!pop-auth!defines the pop server secrets file (default is \"" << pop_auth << "\")!1!file!3|"
		"G!pop-no-delete!disables message deletion via pop (requires --pop)!0!!3|"
		"J!pop-by-name!modifies the pop spool directory according to the user name (requires --pop)!0!!3|"
		;
	return ss.str() ;
}

std::string Main::CommandLine::switchSpec_unix()
{
	return
		"l!log!writes log information on standard error and syslog!0!!2|"
		"t!no-daemon!does not detach from the terminal!0!!3|"
		"u!user!names the effective user to switch to if started as root "
			"(default is \"daemon\")!1!username!3|"
		"k!syslog!forces syslog output if logging is enabled (overrides --no-syslog)!0!!3|"
		"n!no-syslog!disables syslog output (always overridden by --syslog)!0!!3" ;
}

std::string Main::CommandLine::switchSpec_windows()
{
	return
		"l!log!writes log information on standard error and event log!0!!2|"
		"t!no-daemon!uses an ordinary window, not the system tray!0!!3|"
		"k!syslog!forces system event log output if logging is enabled (overrides --no-syslog)!0!!3|"
		"n!no-syslog!disables use of the system event log!0!!3|"
		"c!icon!selects the application icon!1!0^|1^|2^|3!3|"
		"H!hidden!hides the application window and suppresses message boxes (requires --no-daemon)!0!!3" ;
}

Main::CommandLine::CommandLine( Output & output , const G::Arg & arg , const std::string & spec ,
	const std::string & version ) :
		m_output(output) ,
		m_version(version) ,
		m_arg(arg) ,
		m_getopt(m_arg,spec,'|','!','^')
{
}

unsigned int Main::CommandLine::argc() const
{
	return m_getopt.args().c() ;
}

Main::Configuration Main::CommandLine::cfg() const
{
	return Configuration( *this ) ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return m_getopt.hasErrors() ;
}

void Main::CommandLine::showUsage( bool e ) const
{
	Show show( m_output , e ) ;

	G::GetOpt::Level level = G::GetOpt::Level(2U) ;
	std::string introducer = G::GetOpt::introducerDefault() ;
	if( m_getopt.contains("verbose") )
		level = G::GetOpt::levelDefault() ;
	else
		introducer = std::string("abbreviated ") + introducer ;

	std::string::size_type tab_stop = 34U ;
	m_getopt.showUsage( show.s() , m_arg.prefix() , "" , introducer , level , tab_stop ) ;
}

bool Main::CommandLine::contains( const std::string & name ) const
{
	return m_getopt.contains( name ) ;
}

std::string Main::CommandLine::value( const std::string & name ) const
{
	return m_getopt.value( name ) ;
}

std::string Main::CommandLine::semanticError( bool & fatal ) const
{
	fatal = true ;
	std::string warning ;

	if( 
		( cfg().doAdmin() && cfg().adminPort() == cfg().port() ) ||
		( cfg().doPop() && cfg().popPort() == cfg().port() ) ||
		( cfg().doPop() && cfg().doAdmin() && cfg().popPort() == cfg().adminPort() ) )
	{
		return "the listening ports must be different" ;
	}

	if( ! m_getopt.contains("pop") && (
		m_getopt.contains("pop-port") ||
		m_getopt.contains("pop-auth") ||
		m_getopt.contains("pop-by-name") ||
		m_getopt.contains("pop-no-delete") ) )
	{
		return "pop switches require --pop" ;
	}

	if( cfg().withTerminate() && !cfg().doAdmin() )
	{
		return "the --admin-terminate switch requires --admin" ;
	}

	if( cfg().daemon() && cfg().spoolDir().isRelative() )
	{
		return "in daemon mode the spool-dir must be an absolute path" ;
	}

	if( cfg().daemon() && (
		( !cfg().clientSecretsFile().empty() && G::Path(cfg().clientSecretsFile()).isRelative() ) ||
		( !cfg().serverSecretsFile().empty() && G::Path(cfg().serverSecretsFile()).isRelative() ) ||
		( !cfg().popSecretsFile().empty() && G::Path(cfg().popSecretsFile()).isRelative() ) ) )
	{
		return "in daemon mode the authorisation secrets file(s) must be absolute paths" ;
	}

	const bool forward_to = 
		m_getopt.contains("as-proxy") || // => forward-to
		m_getopt.contains("as-client") || // => forward-to
		m_getopt.contains("forward-to") ;

	if( ! forward_to && (
		m_getopt.contains("forward") || 
		m_getopt.contains("poll") || 
		m_getopt.contains("immediate") ) )
	{
		return "the --forward, --immediate and --poll switches require --forward-to" ;
	}

	const bool forwarding =
		m_getopt.contains("as-proxy") ||  // => immediate
		m_getopt.contains("as-client") ||  // => forward
		m_getopt.contains("forward") || 
		m_getopt.contains("immediate") || 
		m_getopt.contains("poll") ;

	if( m_getopt.contains("client-filter") && ! forwarding )
	{
		return "the --client-filter switch requires --as-proxy, --as-client, --poll, --immediate or --forward" ;
	}

	const bool not_serving = m_getopt.contains("dont-serve") || m_getopt.contains("as-client") ;

	if( not_serving ) // ie. if not serving admin, smtp or pop
	{
		if( m_getopt.contains("filter") )
			return "the --filter switch cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("port") )
			return "the --port switch cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("server-auth") )
			return "the --server-auth switch cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("pop") )
			return "the --pop switch cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("admin") )
			return "the --admin switch cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("poll") )
			return "the --poll switch cannot be used with --as-client or --dont-serve" ;
	}

	if( m_getopt.contains("no-smtp") ) // ie. if not serving smtp
	{
		if( m_getopt.contains("filter") )
			return "the --filter switch cannot be used with --no-smtp" ;

		if( m_getopt.contains("port") )
			return "the --port switch cannot be used with --no-smtp" ;

		if( m_getopt.contains("server-auth") )
			return "the --server-auth switch cannot be used with --no-smtp" ;
	}

	const bool log =
		m_getopt.contains("log") ||
		m_getopt.contains("as-server") || // => log
		m_getopt.contains("as-client") || // => log
		m_getopt.contains("as-proxy") ; // => log

	if( m_getopt.contains("verbose") && ! ( m_getopt.contains("help") || log ) )
	{
		return "the --verbose switch must be used with --log, --help, --as-client, --as-server or --as-proxy" ;
	}

	if( m_getopt.contains("debug") && !log )
	{
		return "the --debug switch requires --log, --as-client, --as-server or --as-proxy" ;
	}

	const bool no_daemon =
		m_getopt.contains("as-client") || // => no-daemon
		m_getopt.contains("no-daemon") ;

	if( m_getopt.contains("hidden") && ! no_daemon ) // (win32)
	{
		return "the --hidden switch requires --no-daemon or --as-client" ;
	}

	const bool no_syslog = 
		m_getopt.contains("no-syslog") || 
		m_getopt.contains("as-client") ;

	const bool syslog =
		! ( no_syslog && ! m_getopt.contains("syslog") ) ;

	const bool close_stderr =
		m_getopt.contains("close-stderr") ||
		m_getopt.contains("as-server") ||
		m_getopt.contains("as-proxy") ;

	if( log && close_stderr && !syslog ) // ie. logging to nowhere
	{
		std::string close_stderr_switch =
			( m_getopt.contains("close-stderr") ? "--close-stderr" : 
			( m_getopt.contains("as-server") ? "--as-server" :
			"--as-proxy" ) ) ;

		warning = "logging is enabled but it has nowhere to go because " +
			close_stderr_switch + " closes the standard error stream soon after startup and " +
			"output to the system log is disabled" ;

		if( m_getopt.contains("as-server") && !m_getopt.contains("log") )
			warning = warning + ": replace --as-server with --log" ;
		else if( m_getopt.contains("as-server") )
			warning = warning + ": remove --as-server" ;
		else if( m_getopt.contains("as-proxy" ) )
			warning = warning + ": replace --as-proxy with --log --immediate --forward-to" ;
	}

	fatal = false ;
	return warning ;
}

bool Main::CommandLine::hasSemanticError() const
{
	bool fatal = false ;
	bool error = ! semanticError(fatal).empty() ;
	return error && fatal ;
}

void Main::CommandLine::showSemanticError( bool e ) const
{
	Show show( m_output , e ) ;
	bool fatal = false ;
	show.s() << m_arg.prefix() << ": usage error: " << semanticError(fatal) << std::endl ;
}

void Main::CommandLine::logSemanticWarnings() const
{
	bool fatal = false ;
	std::string warning = semanticError( fatal ) ;
	if( !warning.empty() && !fatal )
		G_WARNING( "CommandLine::logSemanticWarnings: " << warning ) ;
}

void Main::CommandLine::showUsageErrors( bool e ) const
{
	Show show( m_output , e ) ;
	m_getopt.showErrors( show.s() , m_arg.prefix() ) ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showArgcError( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": usage error: too many non-switch arguments" << std::endl ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showShortHelp( bool e ) const
{
	Show show( m_output , e ) ;
	const std::string & exe = m_arg.prefix() ;
	show.s() 
		<< std::string(exe.length()+2U,' ') 
		<< "try \"" << exe << " --help --verbose\" for more information" << std::endl ;
}

void Main::CommandLine::showHelp( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e ) ;
	show.s() << std::endl ;
	showUsage( e ) ;
	showExtraHelp( e ) ;
	showCopyright( e ) ;
}

void Main::CommandLine::showExtraHelp( bool e ) const
{
	Show show( m_output , e ) ;
	const std::string & exe = m_arg.prefix() ;

	show.s() << std::endl ;

	if( m_getopt.contains("verbose") )
	{
		show.s() 
			<< "To start a 'storage' daemon in background..." << std::endl
			<< "   " << exe << " --as-server" << std::endl
			<< std::endl ;

		show.s() 
			<< "To forward stored mail to \"mail.myisp.net\"..."  << std::endl
			<< "   " << exe << " --as-client mail.myisp.net:smtp" << std::endl
			<< std::endl ;

		show.s()
			<< "To run as a proxy (on port 10025) to a local server (on port 25)..." << std::endl
			<< "   " << exe << " --port 10025 --as-proxy localhost:25" << std::endl
			<< std::endl ;
	}
	else
	{
		show.s() 
			<< "For complete usage information run \"" << exe 
			<< " --help --verbose\"" << std::endl
			<< std::endl ;
	}
}

void Main::CommandLine::showNoop( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": no messages to send" << std::endl ;
}

void Main::CommandLine::showBanner( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() 
		<< "E-MailRelay V" << m_version << std::endl << final ;
}

void Main::CommandLine::showCopyright( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::copyright() << std::endl << final ;
}

void Main::CommandLine::showWarranty( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::warranty("","\n") << final ;
}

void Main::CommandLine::showCredit( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << GSsl::Library::credit("","\n",final) ;
}

void Main::CommandLine::showVersion( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e , "\n" ) ;
	showCopyright( e , "\n" ) ;
	showCredit( e , "\n" ) ;
	showWarranty( e ) ;
}

// ===

Main::CommandLine::Show * Main::CommandLine::Show::m_this = NULL ;

Main::CommandLine::Show::Show( Output & output , bool e ) :
	m_output(output) ,
	m_e(e)
{
	if( m_this == NULL )
		m_this = this ;
}

std::ostream & Main::CommandLine::Show::s()
{
	return m_this->m_ss ;
}

Main::CommandLine::Show::~Show()
{
	if( m_this == this )
	{
		m_this = NULL ;
		m_output.output( m_ss.str() , m_e ) ;
	}
}

/// \file commandline.cpp
