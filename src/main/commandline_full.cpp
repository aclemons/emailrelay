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
// commandline_full.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gsmtp.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "gpopsecrets.h"
#include "ggetopt.h"
#include "gtest.h"
#include "gstr.h"
#include "gdebug.h"

namespace Main
{
	class Show ;
}

/// \class Main::CommandLineImp
/// A private implementation class used by Main::CommandLine.
/// 
class Main::CommandLineImp 
{
public:
	CommandLineImp( Main::Output & , const G::Arg & arg , const std::string & spec , 
		const std::string & version , const std::string & capabilities ) ;
	bool contains( const std::string & switch_ ) const ;
	std::string value( const std::string & switch_ ) const ;
	G::Arg::size_type argc() const ;
	bool hasUsageErrors() const ;
	bool hasSemanticError( const Configuration & ) const ;
	void showHelp( bool error_stream ) const ;
	void showUsageErrors( bool error_stream ) const ;
	void showSemanticError( const Configuration & cfg , bool error_stream ) const ;
	void logSemanticWarnings( const Configuration & cfg ) const ;
	void showArgcError( bool error_stream ) const ;
	void showNoop( bool error_stream = false ) const ;
	void showError( std::string reason , bool error_stream = true ) const ;
	void showVersion( bool error_stream = false ) const ;
	void showBanner( bool error_stream = false , const std::string & = std::string() ) const ;
	void showCopyright( bool error_stream = false , const std::string & = std::string() ) const ;
	void showCapabilities( bool error_stream = false , const std::string & = std::string() ) const ;
	static std::string switchSpec( bool is_windows ) ;

public:
	void showWarranty( bool error_stream = false , const std::string & = std::string() ) const ;
	void showCredit( bool error_stream = false , const std::string & = std::string() ) const ;
	void showTestFeatures( bool error_stream = false , const std::string & = std::string() ) const ;
	void showShortHelp( bool error_stream ) const ;
	std::string semanticError( const Configuration & , bool & ) const ;
	void showUsage( bool e ) const ;
	void showExtraHelp( bool error_stream ) const ;
	static std::string switchSpec_unix() ;
	static std::string switchSpec_windows() ;

private:
	Output & m_output ;
	std::string m_version ;
	std::string m_capabilities ;
	G::Arg m_arg ;
	G::GetOpt m_getopt ;
} ;

/// \class Main::Show
/// A private implementation class used by Main::CommandLineImp.
/// 
class Main::Show  
{
public:
	Show( Main::Output & , bool e ) ;
	std::ostream & s() ;
	~Show() ;

private:
	static Show * m_this ;
	Show( const Show & ) ; // not implemented
	void operator=( const Show & ) ; // not implemented
	std::ostringstream m_ss ;
	Main::Output & m_output ;
	bool m_e ;
} ;

// ==

std::string Main::CommandLineImp::switchSpec( bool is_windows )
{
	// single-character options unused: 012345678
	std::string dir = GSmtp::MessageStore::defaultDirectory().str() ;
	std::string pop_auth = GPop::Secrets::defaultPath() ;
	std::ostringstream ss ;
	ss <<
		(is_windows?switchSpec_windows():switchSpec_unix()) << "|"
		"q!as-client!runs as a client, forwarding all spooled mail to <host>!: "
			"equivalent to \"--log --no-syslog --no-daemon --dont-serve --forward --forward-to\"!" 
			"1!host:port!1|"
		"d!as-server!runs as a server, storing mail in the spool directory!: equivalent to \"--log --close-stderr\"!0!!1|"
		"y!as-proxy!runs as a proxy server, forwarding each mail immediately to <host>!"
			": equivalent to \"--log --close-stderr --poll=0 --forward-to\"!"
			"1!host:port!1|"
		"v!verbose!generates more verbose output! (works with --help and --log)!0!!1|"
		"h!help!displays help text and exits!!0!!1|"
		""
		"p!port!specifies the smtp listening port number (default is 25)!!1!port!2|"
		"r!remote-clients!allows remote clients to connect!!0!!2|"
		"s!spool-dir!specifies the spool directory (default is \"" << dir << "\")!!1!dir!2|"
		"V!version!displays version information and exits!!0!!2|"
		""
		"j!client-tls!enables negotiated tls/ssl for smtp client! (if openssl built in)!0!!3|"
		"b!client-tls-connection!enables smtp over tls/ssl for smtp client! (if openssl built in)!0!!3|"
		"K!server-tls!enables negotiated tls/ssl for smtp server using the given openssl certificate file! (which must be in the directory trusted by openssl)!1!pem-file!3|"
		"9!tls-config!sets tls configuration flags! (eg. 2 for SSLv2/3 support)!1!flags!3|"
		"g!debug!generates debug-level logging if built in!!0!!3|"
		"C!client-auth!enables smtp authentication with the remote server, using the given secrets file!!1!file!3|"
		"L!log-time!adds a timestamp to the logging output!!0!!3|"
		"N!log-file!log to file instead of stderr! (%d replaced by the date)!1!file!3|"
		"S!server-auth!enables authentication of remote clients, using the given secrets file!!1!file!3|"
		"e!close-stderr!closes the standard error stream soon after start-up!!0!!3|"
		"a!admin!enables the administration interface and specifies its listening port number!!"
			"1!admin-port!3|"
		"x!dont-serve!disables acting as a server on any port! (part of --as-client and usually used with --forward)!0!!3|"
		"X!no-smtp!disables listening for smtp connections! (usually used with --admin or --pop)!0!!3|"
		"z!filter!specifies an external program to process messages as they are stored!!1!program!3|"
		"W!filter-timeout!sets the timeout (in seconds) for running the --filter processor (default is 300)!!1!time!3|"
		"w!prompt-timeout!sets the timeout (in seconds) for getting an initial prompt from the server (default is 20)!!1!time!3|"
		"D!domain!sets an override for the host's fully qualified domain name!!1!fqdn!3|"
		"f!forward!forwards stored mail on startup! (requires --forward-to)!0!!3|"
		"o!forward-to!specifies the remote smtp server! (required by --forward, --poll, --immediate and --admin)!1!host:port!3|"
		"T!response-timeout!sets the response timeout (in seconds) when talking to a remote server "
			"(default is 1800)!!1!time!3|"
		"U!connection-timeout!sets the timeout (in seconds) when connecting to a remote server "
			"(default is 40)!!1!time!3|"
		"m!immediate!enables immediate forwarding of messages as soon as they are received! (requires --forward-to)!0!!3|"
		"I!interface!defines the listening interface(s) for incoming connections! (comma-separated list with optional smtp=,pop=,admin= qualifiers)!1!ip-list!3|"
		"i!pid-file!defines a file for storing the daemon process-id!!1!pid-file!3|"
		"O!poll!enables polling of the spool directory for messages to be forwarded with the specified period (zero means on client disconnection)! (requires --forward-to)!1!period!3|"
		"P!postmaster!!!0!!0|"
		"Z!verifier!specifies an external program for address verification!!1!program!3|"
		"Y!client-filter!specifies an external program to process messages when they are forwarded!!1!program!3|"
		"Q!admin-terminate!enables the terminate command on the admin interface!!0!!3|"
		"A!anonymous!disables the smtp vrfy command and sends less verbose smtp responses!!0!!3|"
		"B!pop!enables the pop server!!0!!3|"
		"E!pop-port!specifies the pop listening port number (default is 110)! (requires --pop)!1!port!3|"
		"F!pop-auth!defines the pop server secrets file (default is \"" << pop_auth << "\")!!1!file!3|"
		"G!pop-no-delete!disables message deletion via pop! (requires --pop)!0!!3|"
		"J!pop-by-name!modifies the pop spool directory according to the pop user name! (requires --pop)!0!!3|"
		"M!size!limits the size of submitted messages!!1!bytes!3|"
		;
	return ss.str() ;
}

std::string Main::CommandLineImp::switchSpec_unix()
{
	return
		"l!log!writes log information on standard error and syslog! (but see --close-stderr and --no-syslog)!0!!2|"
		"t!no-daemon!does not detach from the terminal!!0!!3|"
		"u!user!names the effective user to switch to if started as root "
			"(default is \"daemon\")!!1!username!3|"
		"k!syslog!forces syslog output if logging is enabled (overrides --no-syslog)!!0!!3|"
		"n!no-syslog!disables syslog output (always overridden by --syslog)!!0!!3" ;
}

std::string Main::CommandLineImp::switchSpec_windows()
{
	return
		"l!log!writes log information on stderr and to the event log! (but see --close-stderr and --no-syslog)!0!!2|"
		"t!no-daemon!uses an ordinary window, not the system tray!!0!!3|"
		"k!syslog!forces system event log output if logging is enabled (overrides --no-syslog)!!0!!3|"
		"n!no-syslog!disables use of the system event log!!0!!3|"
		"c!icon!does nothing!!1!0^|1^|2^|3!0|"
		"H!hidden!hides the application window and suppresses message boxes (requires --no-daemon)!!0!!3|"
		"R!peer-lookup!lookup the account names of local peers! to put in the envelope files!0!!3" ;
}

Main::CommandLineImp::CommandLineImp( Output & output , const G::Arg & arg , const std::string & spec ,
	const std::string & version , const std::string & capabilities ) :
		m_output(output) ,
		m_version(version) ,
		m_capabilities(capabilities) ,
		m_arg(arg) ,
		m_getopt(m_arg,spec,'|','!','^')
{
}

G::Arg::size_type Main::CommandLineImp::argc() const
{
	return m_getopt.args().c() ;
}

bool Main::CommandLineImp::hasUsageErrors() const
{
	return m_getopt.hasErrors() ;
}

void Main::CommandLineImp::showUsage( bool e ) const
{
	Show show( m_output , e ) ;

	G::GetOpt::Level level = G::GetOpt::Level(2U) ;
	std::string introducer = G::GetOpt::introducerDefault() ;
	if( m_getopt.contains("verbose") )
		level = G::GetOpt::levelDefault() ;
	else
		introducer = std::string("abbreviated ") + introducer ;

	std::string::size_type tab_stop = 34U ;
	bool extra = m_getopt.contains("verbose") ;
	m_getopt.showUsage( show.s() , m_arg.prefix() , "" , introducer , level , tab_stop , 
		G::GetOpt::wrapDefault() , extra ) ;
}

bool Main::CommandLineImp::contains( const std::string & name ) const
{
	return m_getopt.contains( name ) ;
}

std::string Main::CommandLineImp::value( const std::string & name ) const
{
	return m_getopt.value( name ) ;
}

std::string Main::CommandLineImp::semanticError( const Configuration & cfg , bool & fatal ) const
{
	fatal = true ;

	if( 
		( cfg.doAdmin() && cfg.adminPort() == cfg.port() ) ||
		( cfg.doPop() && cfg.popPort() == cfg.port() ) ||
		( cfg.doPop() && cfg.doAdmin() && cfg.popPort() == cfg.adminPort() ) )
	{
		return "the listening ports must be different" ;
	}

	if( ! m_getopt.contains("pop") && (
		m_getopt.contains("pop-port") ||
		m_getopt.contains("pop-auth") ||
		m_getopt.contains("pop-by-name") ||
		m_getopt.contains("pop-no-delete") ) )
	{
		return "pop options require --pop" ;
	}

	if( cfg.withTerminate() && !cfg.doAdmin() )
	{
		return "the --admin-terminate option requires --admin" ;
	}

	if( cfg.daemon() && cfg.spoolDir().isRelative() )
	{
		return "in daemon mode the spool-dir must be an absolute path" ;
	}

	if( cfg.daemon() && (
		( !cfg.clientSecretsFile().empty() && G::Path(cfg.clientSecretsFile()).isRelative() ) ||
		( !cfg.serverSecretsFile().empty() && G::Path(cfg.serverSecretsFile()).isRelative() ) ||
		( !cfg.popSecretsFile().empty() && G::Path(cfg.popSecretsFile()).isRelative() ) ) )
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
		return "the --forward, --immediate and --poll options require --forward-to" ;
	}

	const bool forwarding =
		m_getopt.contains("as-proxy") || // => poll
		m_getopt.contains("as-client") || // => forward
		m_getopt.contains("forward") || 
		m_getopt.contains("immediate") || 
		m_getopt.contains("poll") ;

	if( m_getopt.contains("client-filter") && ! forwarding )
	{
		return "the --client-filter option requires --as-proxy, --as-client, --poll, --immediate or --forward" ;
	}

	const bool not_serving = m_getopt.contains("dont-serve") || m_getopt.contains("as-client") ;

	if( not_serving ) // ie. if not serving admin, smtp or pop
	{
		if( m_getopt.contains("filter") )
			return "the --filter option cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("port") )
			return "the --port option cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("server-auth") )
			return "the --server-auth option cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("pop") )
			return "the --pop option cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("admin") )
			return "the --admin option cannot be used with --as-client or --dont-serve" ;

		if( m_getopt.contains("poll") )
			return "the --poll option cannot be used with --as-client or --dont-serve" ;
	}

	if( m_getopt.contains("no-smtp") ) // ie. if not serving smtp
	{
		if( m_getopt.contains("filter") )
			return "the --filter option cannot be used with --no-smtp" ;

		if( m_getopt.contains("port") )
			return "the --port option cannot be used with --no-smtp" ;

		if( m_getopt.contains("server-auth") )
			return "the --server-auth option cannot be used with --no-smtp" ;
	}

	const bool log =
		m_getopt.contains("log") ||
		m_getopt.contains("as-server") || // => log
		m_getopt.contains("as-client") || // => log
		m_getopt.contains("as-proxy") ; // => log

	if( m_getopt.contains("verbose") && ! ( m_getopt.contains("help") || log ) )
	{
		return "the --verbose option must be used with --log, --help, --as-client, --as-server or --as-proxy" ;
	}

	if( m_getopt.contains("debug") && !log )
	{
		return "the --debug option requires --log, --as-client, --as-server or --as-proxy" ;
	}

	const bool no_daemon =
		m_getopt.contains("as-client") || // => no-daemon
		m_getopt.contains("no-daemon") ;

	if( m_getopt.contains("hidden") && ! no_daemon ) // (win32)
	{
		return "the --hidden option requires --no-daemon or --as-client" ;
	}

	if( m_getopt.contains("client-tls") && m_getopt.contains("client-tls-connection") )
	{
		return "the --client-tls and --client-tls-connection options cannot be used together" ;
	}

	if( m_getopt.contains("server-auth") && m_getopt.value("server-auth") == "/pam" &&
		!m_getopt.contains("server-tls" ) )
	{
		return "--server-auth using pam requires --server-tls" ;
	}

	if( m_getopt.contains("pop-auth") && m_getopt.value("pop-auth") == "/pam" &&
		!m_getopt.contains("server-tls" ) )
	{
		return "--pop-auth using pam requires --server-tls" ;
	}

	// warnings...

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

		std::string warning = "logging is enabled but it has nowhere to go because " +
			close_stderr_switch + " closes the standard error stream soon after startup and " +
			"output to the system log is disabled" ;

		if( m_getopt.contains("as-server") && !m_getopt.contains("log") )
			warning = warning + ": replace --as-server with --log" ;
		else if( m_getopt.contains("as-server") )
			warning = warning + ": remove --as-server" ;
		else if( m_getopt.contains("as-proxy" ) )
			warning = warning + ": replace --as-proxy with --log --poll 0 --forward-to" ;

		fatal = false ;
		return warning ;
	}

	fatal = false ;
	return std::string() ;
}

bool Main::CommandLineImp::hasSemanticError( const Configuration & cfg ) const
{
	bool fatal = false ;
	bool error = ! semanticError(cfg,fatal).empty() ;
	return error && fatal ;
}

void Main::CommandLineImp::showSemanticError( const Configuration & cfg , bool e ) const
{
	Show show( m_output , e ) ;
	bool fatal = false ;
	show.s() << m_arg.prefix() << ": usage error: " << semanticError(cfg,fatal) << std::endl ;
}

void Main::CommandLineImp::logSemanticWarnings( const Configuration & cfg ) const
{
	bool fatal = false ;
	std::string warning = semanticError( cfg , fatal ) ;
	if( !warning.empty() && !fatal )
		G_WARNING( "CommandLine::logSemanticWarnings: " << warning ) ;
}

void Main::CommandLineImp::showUsageErrors( bool e ) const
{
	Show show( m_output , e ) ;
	m_getopt.showErrors( show.s() , m_arg.prefix() ) ;
	showShortHelp( e ) ;
}

void Main::CommandLineImp::showArgcError( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": usage error: too many non-switch arguments" << std::endl ;
	showShortHelp( e ) ;
}

void Main::CommandLineImp::showShortHelp( bool e ) const
{
	Show show( m_output , e ) ;
	const std::string & exe = m_arg.prefix() ;
	show.s() 
		<< std::string(exe.length()+2U,' ') 
		<< "try \"" << exe << " --help --verbose\" for more information" << std::endl ;
}

void Main::CommandLineImp::showHelp( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e ) ;
	show.s() << std::endl ;
	showUsage( e ) ;
	showExtraHelp( e ) ;
	showCopyright( e ) ;
}

void Main::CommandLineImp::showExtraHelp( bool e ) const
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
			<< "To forward stored mail to \"mail.myisp.net\"..." << std::endl
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

void Main::CommandLineImp::showNoop( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": no messages to send" << std::endl ;
}

void Main::CommandLineImp::showError( std::string reason , bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": " << reason << std::endl ;
}

void Main::CommandLineImp::showBanner( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() 
		<< "E-MailRelay V" << m_version << std::endl << final ;
}

void Main::CommandLineImp::showCopyright( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::copyright() << std::endl << final ;
}

void Main::CommandLineImp::showCapabilities( bool e , const std::string & final ) const
{
	if( !m_capabilities.empty() )
	{
		Show show( m_output , e ) ;
		show.s() << "Build configuration [" << m_capabilities << "]" << std::endl << final ;
	}
}

void Main::CommandLineImp::showWarranty( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::warranty("","\n") << final ;
}

void Main::CommandLineImp::showCredit( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << GSsl::Library::credit("","\n",final) ;
}

void Main::CommandLineImp::showTestFeatures( bool e , const std::string & final ) const
{
	Show show( m_output , e ) ;
	show.s() << "Test features " << (G::Test::enabled()?"enabled":"disabled") << std::endl << final ;
}

void Main::CommandLineImp::showVersion( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e , "\n" ) ;
	showCopyright( e , "\n" ) ;
	if( contains("verbose") )
	{
		showCapabilities( e , "\n" ) ;
		showTestFeatures( e , "\n" ) ;
	}
	showCredit( e , "\n" ) ;
	showWarranty( e ) ;
}

// ===

Main::Show * Main::Show::m_this = NULL ;

Main::Show::Show( Output & output , bool e ) :
	m_output(output) ,
	m_e(e)
{
	if( m_this == NULL )
		m_this = this ;
}

std::ostream & Main::Show::s()
{
	return m_this->m_ss ;
}

Main::Show::~Show()
{
	if( m_this == this )
	{
		m_this = NULL ;
		m_output.output( m_ss.str() , m_e ) ;
	}
}

// ==

std::string Main::CommandLine::switchSpec( bool is_windows )
{
	return CommandLineImp::switchSpec( is_windows ) ;
}

Main::CommandLine::CommandLine( Main::Output & output , const G::Arg & arg , const std::string & spec , 
	const std::string & version , const std::string & capabilities ) :
		m_imp(new CommandLineImp(output,arg,spec,version,capabilities))
{
}

Main::CommandLine::~CommandLine()
{
	delete m_imp ;
}

Main::Configuration Main::CommandLine::cfg() const
{
	return Configuration( *this ) ;
}

bool Main::CommandLine::contains( const std::string & switch_ ) const
{
	return m_imp->contains( switch_ ) ;
}

std::string Main::CommandLine::value( const std::string & switch_ ) const
{
	return m_imp->value( switch_ ) ;
}

G::Arg::size_type Main::CommandLine::argc() const
{
	return m_imp->argc() ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return m_imp->hasUsageErrors() ;
}

bool Main::CommandLine::hasSemanticError() const
{
	return m_imp->hasSemanticError( cfg() ) ;
}

void Main::CommandLine::showHelp( bool error_stream ) const
{
	m_imp->showHelp( error_stream ) ;
}

void Main::CommandLine::showUsageErrors( bool error_stream ) const
{
	m_imp->showUsageErrors( error_stream ) ;
}

void Main::CommandLine::showSemanticError( bool error_stream ) const
{
	m_imp->showSemanticError( cfg() , error_stream ) ;
}

void Main::CommandLine::logSemanticWarnings() const
{
	m_imp->logSemanticWarnings( cfg() ) ;
}

void Main::CommandLine::showArgcError( bool error_stream ) const
{
	m_imp->showArgcError( error_stream ) ;
}

void Main::CommandLine::showNoop( bool error_stream ) const
{
	m_imp->showNoop( error_stream ) ;
}

void Main::CommandLine::showError( const std::string & reason , bool error_stream ) const
{
	m_imp->showError( reason , error_stream ) ;
}

void Main::CommandLine::showVersion( bool error_stream ) const
{
	m_imp->showVersion( error_stream ) ;
}

void Main::CommandLine::showBanner( bool error_stream , const std::string & s ) const
{
	m_imp->showBanner( error_stream , s ) ;
}

void Main::CommandLine::showCopyright( bool error_stream , const std::string & s ) const
{
	m_imp->showCopyright( error_stream , s ) ;
}

void Main::CommandLine::showCapabilities( bool error_stream , const std::string & s ) const
{
	m_imp->showCapabilities( error_stream , s ) ;
}

unsigned int Main::CommandLine::value( const std::string & switch_ , unsigned int default_ ) const
{
	return m_imp->contains(switch_) ? G::Str::toUInt(value(switch_)) : default_ ;
}

G::Strings Main::CommandLine::value( const std::string & switch_ , const std::string & separators ) const
{
	G::Strings result ;
	if( contains(switch_) )
	{
		G::Str::splitIntoFields( value(switch_) , result , separators ) ;
	}
	return result ;
}

bool Main::CommandLine::contains( const char * switch_ ) const
{
	return contains( std::string(switch_) ) ;
}

std::string Main::CommandLine::value( const char * switch_ ) const
{
	return value( std::string(switch_) ) ;
}

unsigned int Main::CommandLine::value( const char * switch_ , unsigned int default_ ) const
{
	return value( std::string(switch_) , default_ ) ;
}

G::Strings Main::CommandLine::value( const char * switch_ , const char * separators ) const
{
	return value( std::string(switch_) , std::string(separators) ) ;
}

