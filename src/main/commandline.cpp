//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// commandline.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gaddress.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "gstr.h"
#include "gdebug.h"

//static
std::string Main::CommandLine::switchSpec( bool is_windows )
{
	std::string dir = GSmtp::MessageStore::defaultDirectory().str() ;
	std::ostringstream ss ;
	ss
		<< (is_windows?switchSpec_windows():switchSpec_unix()) << "|"
		<< "q!as-client!runs as a client, forwarding spooled mail to <host>: "
			<< "equivalent to \"--log --no-syslog --no-daemon --dont-serve --forward --forward-to\"!" 
			<< "1!host:port!1|"
		<< "d!as-server!runs as a server: equivalent to \"--log --close-stderr --postmaster\"!0!!1|"
		<< "y!as-proxy!runs as a proxy: equivalent to \"--log --close-stderr --immediate --forward-to\"!"
			<< "1!host:port!1|"
		<< "v!verbose!generates more verbose output (works with --help and --log)!0!!1|"
		<< "h!help!displays help text and exits!0!!1|"
		<< ""
		<< "p!port!specifies the smtp listening port number!1!port!2|"
		<< "r!remote-clients!allows remote clients to connect!0!!2|"
		<< "s!spool-dir!specifies the spool directory (default is \"" << dir << "\")!1!dir!2|"
		<< "V!version!displays version information and exits!0!!2|"
		<< ""
		<< "g!debug!generates debug-level logging (if compiled-in)!0!!3|"
		<< "C!client-auth!enables authentication with remote server, using the given secrets file!1!file!3|"
		<< "L!log-time!adds a timestamp to the logging output!0!!3|"
		<< "S!server-auth!enables authentication of remote clients, using the given secrets file!1!file!3|"
		<< "e!close-stderr!closes the standard error stream after start-up!0!!3|"
		<< "a!admin!enables the administration interface and specifies its listening port number!"
			<< "1!admin-port!3|"
		<< "x!dont-serve!dont act as a server (usually used with --forward)!0!!3|"
		<< "X!dont-listen!dont listen for smtp connections (usually used with --admin)!0!!3|"
		<< "z!filter!defines a mail processor program!1!program!3|"
		<< "D!domain!sets an override for the host's fully qualified domain name!1!fqdn!3|"
		<< "f!forward!forwards stored mail on startup (requires --forward-to)!0!!3|"
		<< "o!forward-to!specifies the remote smtp server (required by --forward and --admin)!1!host:port!3|"
		<< "T!response-timeout!sets the response timeout (in seconds) when talking to a remote server "
			<< "(default is 1800)!1!time!3|"
		<< "U!connection-timeout!sets the timeout (in seconds) when connecting to a remote server "
			<< "(default is 40)!1!time!3|"
		<< "m!immediate!forwards each message as soon as it is received (requires --forward-to)!0!!3|"
		<< "I!interface!listen on a specific interface!1!ip-address!3|"
		<< "i!pid-file!records the daemon process-id in the given file!1!pid-file!3|"
		<< "O!poll!enables polling with the specified period (requires --forward-to)!1!period!3|"
		<< "P!postmaster!deliver to postmaster and reject all other local mailbox addresses!0!!3|"
		<< "Z!verifier!defines an external address verifier program!1!program!3|"
		<< "Q!admin-terminate!!0!!0|"
		<< "R!scanner!!1!host:port!0|"
		;
	return ss.str() ;
}

//static
std::string Main::CommandLine::switchSpec_unix()
{
	std::ostringstream ss ;
	ss
		<< "l!log!writes log information on standard error and syslog!0!!2|"
		<< "t!no-daemon!does not detach from the terminal!0!!3|"
		<< "u!user!names the effective user to switch to when started as root "
			<< "(default is \"daemon\")!1!username!3|"
		<< "n!no-syslog!disables syslog output!0!!3"
		;
	return ss.str() ;
}

//static
std::string Main::CommandLine::switchSpec_windows()
{
	std::ostringstream ss ;
	ss
		<< "l!log!writes log information on standard error and event log!0!!2|"
		<< "t!no-daemon!use an ordinary window, not the system tray!0!!3|"
		<< "n!no-syslog!dont use the event log!0!!3|"
		<< "c!icon!selects the application icon!1!0^|1^|2^|3!3|"
		<< "H!hidden!hides the application window (requires --no-daemon)!0!!3"
		;
	return ss.str() ;
}

Main::CommandLine::CommandLine( Output & output , const G::Arg & arg , const std::string & spec ,
	const std::string & version ) :
		m_output(output) ,
		m_version(version) ,
		m_arg(arg) ,
		m_getopt( m_arg , spec , '|' ,  '!' , '^' )
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

	size_t tab_stop = 33U ;
	m_getopt.showUsage( show.s() , m_arg.prefix() , "" , 
		introducer , level , tab_stop , m_output.columns() ) ;
}

bool Main::CommandLine::contains( const std::string & name ) const
{
	return m_getopt.contains( name ) ;
}

std::string Main::CommandLine::value( const std::string & name ) const
{
	return m_getopt.value( name ) ;
}

std::string Main::CommandLine::semanticError() const
{
	if( cfg().doAdmin() && cfg().adminPort() == cfg().port() )
	{
		return "the smtp listening port and the "
			"admin listening port must be different" ;
	}

	if( cfg().daemon() && cfg().spoolDir().isRelative() )
	{
		return "in daemon mode the spool-dir must "
			"be an absolute path" ;
	}

	if( cfg().daemon() && (
		( !cfg().clientSecretsFile().empty() && G::Path(cfg().clientSecretsFile()).isRelative() ) ||
		( !cfg().serverSecretsFile().empty() && G::Path(cfg().serverSecretsFile()).isRelative() ) ) )
	{
		return "in daemon mode the authorisation secrets file(s) must "
			"be absolute paths" ;
	}

	const bool forward_to = 
		m_getopt.contains("as-proxy") || 
		m_getopt.contains("as-client") || 
		m_getopt.contains("forward-to") ;

	if( ! forward_to && (
		m_getopt.contains("forward") || 
		m_getopt.contains("poll") || 
		m_getopt.contains("immediate") ) )
	{
		return "the --forward, --immediate and --poll switches require --forward-to" ;
	}

	if( m_getopt.contains("scanner") && ! ( m_getopt.contains("as-proxy") || m_getopt.contains("immediate") ) )
	{
		return "the --scanner switch requires --as-proxy or --immediate" ;
	}

	const bool log =
		m_getopt.contains("log") ||
		m_getopt.contains("as-server") ||
		m_getopt.contains("as-client") ||
		m_getopt.contains("as-proxy") ;

	if( m_getopt.contains("verbose") && ! ( m_getopt.contains("help") || log ) )
	{
		return "the --verbose switch must be used with --log, --help, --as-client, --as-server or --as-proxy" ;
	}

	if( m_getopt.contains("interface") && ( m_getopt.contains("dont-serve") || m_getopt.contains("as-client") ) )
	{
		return "the --interface switch cannot be used with --as-client or --dont-serve" ;
	}

	if( cfg().daemon() && m_getopt.contains("hidden") ) // (win32)
	{
		return "the --hidden switch requires --no-daemon or --as-client" ;
	}

	return std::string() ;
}

bool Main::CommandLine::hasSemanticError() const
{
	return ! semanticError().empty() ;
}

void Main::CommandLine::showSemanticError( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": usage error: " << semanticError() << std::endl ;
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
			<< "To forward stored mail to \"mail.myisp.co.uk\"..."  << std::endl
			<< "   " << exe << " --as-client mail.myisp.co.uk:smtp" << std::endl
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

void Main::CommandLine::showBanner( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() 
		<< "E-MailRelay V" << m_version << std::endl ;
}

void Main::CommandLine::showCopyright( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::copyright() << std::endl ;
}

void Main::CommandLine::showWarranty( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::warranty("","\n") ;
}

void Main::CommandLine::showVersion( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e ) ;
	showWarranty( e ) ;
	showCopyright( e ) ;
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

