//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gfilterfactory.h"
#include "gverifierfactory.h"
#include "gfilestore.h"
#include "gtest.h"
#include "gpop.h"
#include "gaddress.h"
#include "gidn.h"
#include "gprocess.h"
#include "gassert.h"
#include "gformat.h"
#include "ggettext.h"
#include "glog.h"
#include <array>
#include <utility>
#include <algorithm>
#include <iterator>
#include <iostream>

Main::Configuration::Configuration( const G::OptionMap & map , const std::string & name ,
	const G::Path & app_dir , const G::Path & base_dir ) :
		m_map(map) ,
		m_name(name) ,
		m_app_dir(app_dir) ,
		m_base_dir(base_dir)
{
	G_ASSERT( m_base_dir.isAbsolute() ) ;
	if( m_map.count("pid-file") > 1U )
	{
		// let the start/stop script override the config file
		m_pid_file_warning = true ;
		m_map.replace( "pid-file" , (*m_map.find("pid-file")).second.value() ) ;
	}
	if( G::Test::enabled("configuration-dump") )
	{
		for( const auto & p : map )
			std::cout << "config: [" << name << "] " << p.first << "=[" << p.second.value() << "]\n" ;
	}
}

void Main::Configuration::merge( const Configuration & )
{
	// this does nothing, as per the documentation
}

std::string Main::Configuration::name() const
{
	return m_name ;
}

bool Main::Configuration::contains( const char * key ) const noexcept
{
	static_assert( noexcept(m_map.contains(key)) , "" ) ;
	return m_map.contains( key ) ;
}

bool Main::Configuration::useSyslog() const
{
	bool basic = !contains("no-syslog") && !contains("as-client") ;
	bool use_syslog_override = contains( "syslog" ) ;
	return use_syslog_override || basic ;
}

G::LogOutput::SyslogFacility Main::Configuration::_syslogFacility() const
{
	std::string s = stringValue( "syslog" ) ;
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

bool Main::Configuration::validSyslogFacility() const
{
	std::string s = stringValue( "syslog" ) ;
	if( s.empty() ) return true ;
	if( s == G::Str::positive() ) return true ;
	if( s == "mail" ) return true ;
	if( s == "user" ) return true ;
	if( s == "daemon" ) return true ;
	if( s == "local0" ) return true ;
	if( s == "local1" ) return true ;
	if( s == "local2" ) return true ;
	if( s == "local3" ) return true ;
	if( s == "local4" ) return true ;
	if( s == "local5" ) return true ;
	if( s == "local6" ) return true ;
	if( s == "local7" ) return true ;
	return false ;
}

bool Main::Configuration::logFormatContains( std::string_view token ) const
{
	std::string s = stringValue( "log-format" ) ;
	std::string_view sv = s ;
	for( G::StringFieldView f(sv,",",1U) ; f ; ++f )
	{
		if( f() == token )
			return true ;
	}
	return false ;
}

G::StringArray Main::Configuration::listeningNames( std::string_view protocol ) const
{
	// allow eg. "127.0.0.1,smtp=192.168.1.1,admin=10.0.0.1,pop=fd#4"

	// prepare the naive result by splitting the user's string by commas
	G::StringArray result = G::Str::splitIntoFields( stringValue("interface") , ',' ) ;

	// then weed out the ones that have an explicit protocol name that doesnt match the
	// required protocol, removing the "protocol=" prefix at the same time to leave just
	// the required list of addresses
	for( auto p = result.begin() ; p != result.end() ; )
	{
		std::size_t eqpos = (*p).find( '=' ) ;
		if( protocol.empty() || protocol == G::Str::headView( *p , eqpos , protocol ) )
		{
			if( eqpos != std::string::npos )
				(*p).erase( 0U , eqpos+1U ) ;
			++p ;
		}
		else
		{
			p = result.erase( p ) ;
		}
	}

	return result ;
}

bool Main::Configuration::show( const std::string & key ) const
{
	return (","+_show()+",").find(","+key+",") != std::string::npos ;
}

G::Path Main::Configuration::spoolDir() const
{
	return contains( "spool-dir" ) ? pathValue( "spool-dir" ) : GStore::FileStore::defaultDirectory() ;
}

std::string Main::Configuration::serverAddress() const
{
	const char * key = "forward-to" ;
	if( contains("as-client") )
		key = "as-client" ;
	else if( contains("as-proxy") )
		key = "as-proxy" ;
	return stringValue( key ) ;
}

bool Main::Configuration::closeFiles() const
{
	return daemon() && stringValue("interface").find("fd#") == std::string::npos ;
}

G::Path Main::Configuration::keyFile( const std::string & option_name ) const
{
    std::string value = stringValue( option_name ) ;
    return value.empty() ? G::Path() : pathValueImp( G::Str::head(value,",",false) ) ;
}

G::Path Main::Configuration::certificateFile( const std::string & option_name ) const
{
    std::string value = stringValue( option_name ) ;
    return value.empty() ? G::Path() : pathValueImp( G::Str::tail(value,",",false) ) ;
}

bool Main::Configuration::anonymous( std::string_view type ) const
{
	if( !contains( "anonymous" ) )
		return false ;

	std::string value = stringValue( "anonymous" ) ;
	if( value.empty() )
		return true ;

	std::string_view sv = value ;
	for( G::StringFieldView f(sv,",",1U) ; f ; ++f )
	{
		if( f() == type )
			return true ;
	}
	return false ;
}

std::string Main::Configuration::semanticError() const
{
	const char * error1 = semanticError1() ;
	return error1 ? std::string(error1) : semanticError2() ;
}

const char * Main::Configuration::semanticError1() const
{
	using G::tx ;

	if( contains("syslog") && !validSyslogFacility() )
	{
		return tx("invalid --syslog facility: use 'mail', 'user', 'daemon', or 'local0' to 'local7'") ;
	}

	const bool contains_poll = contains( "poll" ) ;
	if( contains_poll && numberValue("poll",0U) == 0U )
	{
		return tx("invalid --poll period: try --forward-on-disconnect") ;
	}

	const bool contains_pop = contains( "pop" ) ;
	if( contains_pop && !GPop::enabled() )
	{
		return tx("pop has been disabled in this build") ;
	}

	const bool contains_pop_auth = contains( "pop-auth" ) ;
	if( !contains_pop && (
		contains("pop-port") ||
		contains_pop_auth ||
		contains("pop-by-name") ||
		contains("pop-no-delete") ) )
	{
		return tx("pop options require --pop") ;
	}

	if( contains_pop && !contains_pop_auth )
	{
		return tx("the --pop option requires --pop-auth") ;
	}

	const bool contains_admin = contains( "admin" ) ;
	if( contains_admin && !GSmtp::AdminServer::enabled() )
	{
		return tx("admin interface has been disabled in this build") ;
	}

	if( contains("admin-terminate") && !contains_admin )
	{
		return tx("the --admin-terminate option requires --admin") ;
	}

	const bool contains_as_proxy = contains( "as-proxy" ) ;
	const bool contains_as_client = contains( "as-client" ) ;
	if( contains("forward-to") && ( contains_as_proxy || contains_as_client ) )
	{
		return tx("--forward-to cannot be used with --as-client or --as-proxy") ;
	}

	const bool have_forward_to =
		contains_as_proxy || // => forward-to
		contains_as_client || // => forward-to
		contains( "forward-to" ) ;

	if( !have_forward_to )
	{
		if( contains("forward") ) return tx("--forward requires --forward-to") ;
		if( contains("forward-on-disconnect") ) return tx("--forward-on-disconnect requires --forward-to") ;
		if( contains("client-filter") ) return tx("--client-filter requires --forward-to") ;
	}

	//forwarding := "admin" "forward" "forward-on-disconnect" "immediate" "poll"
	//if( !forwarding && contains("forward-to") )
	//{
		// ignore this -- we want the configuration gui to be able to set the
		// forwarding address even if it's not used and we might need it for
		// filter exit code 103 (rescan)
	//}

	const bool not_serving = contains("dont-serve") || contains_as_client ;

	const bool contains_filter = contains("filter") ;
	const bool contains_port = contains("port") ;
	const bool contains_server_auth = contains("server-auth") ;

	if( not_serving ) // ie. if not serving admin, smtp or pop
	{
		if( contains_filter )
			return tx("the --filter option cannot be used with --as-client or --dont-serve") ;

		if( contains_port )
			return tx("the --port option cannot be used with --as-client or --dont-serve") ;

		if( contains_server_auth )
			return tx("the --server-auth option cannot be used with --as-client or --dont-serve") ;

		if( contains_pop )
			return tx("the --pop option cannot be used with --as-client or --dont-serve") ;

		if( contains_admin )
			return tx("the --admin option cannot be used with --as-client or --dont-serve") ;
	}

	if( contains("no-smtp") ) // ie. if not serving smtp
	{
		if( contains_filter )
			return tx("the --filter option cannot be used with --no-smtp") ;

		if( contains_port )
			return tx("the --port option cannot be used with --no-smtp") ;

		if( contains_server_auth )
			return tx("the --server-auth option cannot be used with --no-smtp") ;
	}

	if( contains("server-auth-config") && !contains("server-auth") )
	{
		return tx("the --server-auth-config option requires --server-auth") ;
	}

	if( contains("client-auth-config") && !contains("client-auth") )
	{
		return tx("the --client-auth-config option requires --client-auth") ;
	}

	const bool contains_log = log() ;

	if( contains("verbose") && ! ( contains("help") || contains_log ) )
	{
		return tx("the --verbose option must be used with --log, --help, --as-client, --as-server or --as-proxy") ;
	}

	if( contains("debug") && !contains_log )
	{
		return tx("the --debug option requires --log, --as-client, --as-server or --as-proxy") ;
	}

	const bool contains_client_tls = contains("client-tls") ;
	const bool contains_client_tls_connection = contains("client-tls-connection") ;
	if( contains_client_tls && contains_client_tls_connection )
	{
		return tx("the --client-tls and --client-tls-connection options cannot be used together") ;
	}

	const bool contains_server_tls = contains("server-tls") ;
	if( contains_server_tls && contains("server-tls-connection") )
	{
		return tx("the --server-tls and --server-tls-connection options cannot be used together") ;
	}

	const bool contains_server_tls_certificate = contains("server-tls-certificate") ;
	if( contains_server_tls && !contains_server_tls_certificate )
	{
		return tx("the --server-tls option requires --server-tls-certificate") ;
	}

	const bool contains_server_tls_connection = contains("server-tls-connection") ;
	if( contains_server_tls_connection && !contains_server_tls_certificate )
	{
		return tx("the --server-tls-connection option requires --server-tls-certificate") ;
	}

	if( ( contains_server_tls_certificate || contains("server-tls-verify") ) &&
		!( contains_server_tls || contains_server_tls_connection ) )
	{
		return tx("the --server-tls options require either --server-tls or --server-tls-connection") ;
	}

	const bool contains_client_tls_verify = contains("client-tls-verify") ;
	if( ( contains("client-tls-certificate") || contains_client_tls_verify || contains("client-tls-required") ) &&
		!( contains_client_tls || contains_client_tls_connection ) )
	{
		return tx("the --client-tls- options require --client-tls or --client-tls-connection") ;
	}

	if( m_map.count("server-tls-certificate") > 2U )
	{
		return tx("the --server-tls-certificate option cannot be used more than twice")  ;
	}

	if( m_map.count("client-tls-certificate") > 2U )
	{
		return tx("the --client-tls-certificate option cannot be used more than twice")  ;
	}

	if( contains("client-tls-verify-name") && !contains_client_tls_verify )
	{
		return tx("the --client-tls-verify-name options requires --client-tls-verify") ;
	}

	if( contains("server-tls-required") &&
		!( contains_server_tls || contains_server_tls_connection ) )
	{
		return tx("--server-tls-required requires --server-tls or --server-tls-connection") ;
	}

	if( contains("interface") && stringValue("interface").find("client=") != std::string::npos )
	{
		return tx("using --interface with client= is no longer supported: use --client-interface instead") ;
	}

	if( contains("dnsbl") && !contains("remote-clients") )
	{
		return tx("--dnsbl requires --remote-clients or -r") ;
	}

	if( contains("client-interface") && GNet::Address::isFamilyLocal(serverAddress()) )
	{
		return tx("the --client-interface option cannot be used with a unix-domain forwarding address") ;
	}

	if( contains("domain") && stringValue("domain").empty() )
	{
		return tx("the --domain value must not be empty") ;
	}

	return nullptr ;
}

std::string Main::Configuration::semanticError2() const
{
	using G::txt ;
	using G::format ;

	GSmtp::FilterFactoryBase::Spec f ;
	if( (f=_filter()).first.empty() ) // NOLINT assignment
		return str( format( txt("invalid filter specification: %1%") ) % f.second ) ;
	if( (f=_clientFilter()).first.empty() ) // NOLINT assignment
		return str( format( txt("invalid client filter specification: %1%") ) % f.second ) ;

	GSmtp::VerifierFactoryBase::Spec v ;
	if( (v=_verifier()).first.empty() ) // NOLINT assignment
		return str( format( txt("invalid verifier specification: %1%") ) % v.second ) ;

	return {} ;
}

G::StringArray Main::Configuration::semanticWarnings() const
{
	using G::format ;
	using G::txt ;
	G::StringArray warnings ;

	if( m_pid_file_warning )
		warnings.emplace_back( txt("more than one --pid-file: using the first") ) ;

	const bool no_syslog =
		contains( "no-syslog" ) ||
		contains( "as-client") ;

	const bool syslog =
		! ( no_syslog && ! contains("syslog") ) ;

	const bool close_stderr =
		contains( "close-stderr" ) ||
		contains( "as-server" ) ||
		contains( "as-proxy" ) ;

	const bool contains_log = log() ;

	if( contains_log && close_stderr && !contains("log-file") && !syslog ) // ie. logging to nowhere
	{
		std::string close_stderr_option =
			( contains("close-stderr") ? "--close-stderr" :
			( contains("as-server") ? "--as-server" :
			"--as-proxy" ) ) ;

		warnings.push_back( str( format(
			txt("logging will stop because %1% closes the standard error stream soon after startup")) %
				close_stderr_option ) ) ;
	}

	const bool no_daemon =
		contains( "as-client" ) || // => no-daemon
		contains( "no-daemon" ) ;

	if( contains("show") && no_daemon ) // (windows)
	{
		warnings.emplace_back(
			txt("the --show option is ignored when using --no-daemon or --as-client") ) ; // or --hidden
	}

	if( stringValue("server-auth") == "/pam" || stringValue("pop-auth") == "/pam" )
	{
		warnings.emplace_back(
			txt("pam authentication should be enabled with pam: rather than /pam") ) ;
	}

	std::string domain = stringValue( "domain" ) ;
	if( !domain.empty() && !G::Str::isPrintableAscii(domain) )
	{
		warnings.push_back( str( format(
			txt("the domain should be printable ascii: try using \"%1%\"")) %
				G::Idn::encode(domain) ) ) ;
	}

	filterValue( "filter" , &warnings ) ;
	filterValue( "client-filter" , &warnings ) ;
	verifierValue( "address-verifier" , &warnings ) ;

	return warnings ;
}

GSmtp::FilterFactoryBase::Spec Main::Configuration::filterValue( std::string_view option_name ,
	G::StringArray * warnings_p ) const
{
	std::string value = stringValue( option_name ) ;
	return GFilters::FilterFactory::parse( value , m_base_dir.str() , m_app_dir.str() , warnings_p ) ;
}

GSmtp::VerifierFactoryBase::Spec Main::Configuration::verifierValue( std::string_view option_name ,
	G::StringArray * warnings_p ) const
{
	std::string value = stringValue( option_name ) ;
	return GVerifiers::VerifierFactory::parse( value , m_base_dir.str() , m_app_dir.str() , warnings_p ) ;
}

G::Path Main::Configuration::pathValue( std::string_view option_name ) const
{
	std::string value = stringValue( option_name ) ;
	if( tlsVerifyType(option_name) && specialTlsVerifyString(value) )
	{
		// dont mess with eg. "--tls-client-verify=<none>"
		return value ;
	}
	else if( ( option_name == "server-auth" || option_name == "pop-auth" ) &&
		( value == "pam:" || value == "/pam" ) )
	{
		// dont mess with "--server-auth=/pam" etc.
		return value ;
	}
	else if( option_name == "client-auth" && G::Str::headMatch(value,"plain:") )
	{
		// dont mess with "--client-auth=plain:user:pwd"
		return value ;
	}
	else
	{
		return pathValueImp( value ) ;
	}
}

G::Path Main::Configuration::pathValueImp( const std::string & value ) const
{
	G::Path path( value ) ;
	if( !m_app_dir.empty() )
		path.replace( "@app" , m_app_dir.str() ) ;
	if( path.isAbsolute() || !daemon() )
		return path ;
	else
		return G::Path::join( m_base_dir , path ) ;
}

#ifdef G_WINDOWS
bool Main::Configuration::pathlike( std::string_view option_name )
{
	return
		option_name == "log-file" ||
		option_name == "spool-dir" ||
		option_name == "local-delivery-dir" ||
		option_name == "pid-file" ||
		option_name == "server-tls-certificate" ||
		option_name == "server-tls-verify" || // tlsVerifyType()
		option_name == "client-tls-certificate" ||
		option_name == "client-tls-verify" || // tlsVerifyType()
		option_name == "client-auth" ||
		option_name == "server-auth" ||
		option_name == "pop-auth" ||
		false ; // NOLINT readability-simplify-boolean-expr
}
#endif

bool Main::Configuration::tlsVerifyType( std::string_view option_name )
{
	return
		option_name == "server-tls-verify" ||
		option_name == "client-tls-verify" ;
}

bool Main::Configuration::specialTlsVerifyString( std::string_view value )
{
	return !value.empty() && value.at(0U) == '<' && value.at(value.size()-1U) == '>' ;
}

#ifdef G_WINDOWS
G::StringArray Main::Configuration::display( const G::Options & options ) const
{
	const std::string yes = "yes" ;
	const std::string no = "no" ;
	G::StringArray result ;
	result.reserve( 70U ) ;
	for( const auto & option : options.list() )
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
			else if( option.name == "delivery-dir" )
				result.push_back( deliveryDir().str() ) ;
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
				result.push_back( (doSmtp()&&doServing()) ? G::Str::fromUInt(_port()) : std::string() ) ;
			else if( option.name == "pop-port" )
				result.push_back( (doPop()&&doServing()) ? G::Str::fromUInt(_popPort()) : std::string() ) ;
			else if( option.multivalued() )
				result.push_back( stringValue(option.name) ) ;
			else if( option.valued() && pathlike(option.name) )
				result.push_back( contains(option.name.c_str()) ? pathValue(option.name).str() : std::string() ) ;
			else if( option.valued() )
				result.push_back( stringValue(option.name) ) ;
			else
				result.push_back( contains(option.name.c_str()) ? yes : no ) ;
		}
	}
	return result ;
}
#endif

// --

G::LogOutput::Config Main::Configuration::logOutputConfig( bool has_gui ) const
{
	return
		G::LogOutput::Config()
			.set_output_enabled( log() )
			.set_summary_info( log() )
			.set_verbose_info( contains("verbose") )
			.set_more_verbose_info( m_map.count("verbose") > 1U )
			.set_debug( debug() )
			.set_with_level( true )
			.set_with_timestamp( contains("log-time") || logFormatContains("time") )
			.set_with_context( contains("log-address") || contains("log-format") )
			.set_strip( !debug() )
			.set_use_syslog( useSyslog() )
			.set_allow_bad_syslog( !(has_gui && logFile().empty()) )
			.set_umask( G::Process::Umask::Mode::Tighter )
			.set_facility( _syslogFacility() ) ;
}

GStore::FileStore::Config Main::Configuration::fileStoreConfig() const
{
	return
		GStore::FileStore::Config()
			.set_max_size( _maxSize() ) ; // see also ServerProtocol::Config
}

std::pair<int,int> Main::Configuration::_smtpServerSocketLinger() const
{
	Switches switches( stringValue("server-smtp-config") , false ) ;
	bool nolinger = !switches( "linger" , true ) ;
	return nolinger ? std::make_pair(1,0) : std::make_pair(-1,-1) ;
}

GSmtp::ServerProtocol::Config Main::Configuration::_smtpServerProtocolConfig( bool server_secrets_valid ,
	const std::string & domain ) const
{
	Switches switches( stringValue("server-smtp-config") ) ;
	bool nostrictparsing = !switches( "strictparsing" , true ) ;
	return
		GSmtp::ServerProtocol::Config()
			.set_with_vrfy( !anonymous("vrfy") )
			.set_max_size( _maxSize() ) // see also FileStore::Config
			.set_shutdown_how_on_quit( _shutdownHowOnQuit() )
			.set_mail_requires_authentication( server_secrets_valid )
			.set_mail_requires_encryption( _serverTlsRequired() )
			.set_sasl_server_config( _smtpSaslServerConfig() )
			.set_sasl_server_challenge_hostname( domain )
			.set_tls_starttls( serverTls() )
			.set_tls_connection( serverTlsConnection() )
			.set_with_pipelining( switches("pipelining",true) )
			.set_with_chunking( switches("chunking",false) )
			.set_with_smtputf8( switches("smtputf8",false) )
			.set_smtputf8_strict( switches("smtputf8strict",false) )
			.set_parser_config(
				GSmtp::ServerParser::Config()
					.set_allow_spaces() // or (nostrictparsing) in the future
					//.set_allow_spaces_help( nostrictparsing?"":"invalid whitespace after the colon: use --server-smtp-config=nostrictparsing for forwards compatibility" ) // moot
					.set_allow_nobrackets( nostrictparsing )
					.set_allow_nobrackets_help( nostrictparsing?"":"use --server-smtp-config=nostrictparsing to allow" )
					.set_alabels( switches("alabels",true) ) ) ;
}

GNet::Server::Config Main::Configuration::_netServerConfig( std::pair<int,int> linger ) const
{
	bool open_permissions = user().empty() || user() == "root" ;
	return
		GNet::Server::Config()
			.set_stream_socket_config( _netSocketConfig(linger) )
			.set_uds_open_permissions( open_permissions ) ;
}

GNet::StreamSocket::Config Main::Configuration::_netSocketConfig( std::pair<int,int> linger ) const
{
	return
		GNet::StreamSocket::Config()
			.set_create_linger( linger )
			.set_accept_linger( linger )
			.set_bind_reuse( !G::is_windows() )
			.set_bind_exclusive( G::is_windows() )
			.set_last<GNet::StreamSocket::Config>() ;
}

GSmtp::Server::Config Main::Configuration::smtpServerConfig( const std::string & smtp_ident ,
	bool server_secrets_valid , const std::string & server_tls_profile ,
	const std::string & domain ) const
{
	return
		GSmtp::Server::Config()
			.set_allow_remote( _allowRemoteClients() )
			.set_interfaces( listeningNames("smtp") )
			.set_port( _port() )
			.set_ident( smtp_ident )
			.set_anonymous_smtp( anonymous("server") )
			.set_anonymous_content( anonymous("content") )
			.set_filter_config(
				GSmtp::Filter::Config()
					.set_domain( domain )
					.set_timeout( _filterTimeout() ) )
			.set_filter_spec( _filter() )
			.set_verifier_config(
				GSmtp::Verifier::Config()
					.set_domain( domain )
					.set_timeout( _filterTimeout() ) )
			.set_verifier_spec( _verifier() )
			.set_net_server_peer_config(
				GNet::ServerPeer::Config()
					.set_socket_protocol_config( _socketProtocolConfig(server_tls_profile) )
					.set_idle_timeout( _idleTimeout() )
					.set_log_address( contains("log-address") || logFormatContains("address") )
					.set_log_port( logFormatContains("port") ) )
			.set_net_server_config( _netServerConfig(_smtpServerSocketLinger()) )
			.set_protocol_config( _smtpServerProtocolConfig(server_secrets_valid,domain) )
			.set_dnsbl_config( dnsbl() )
			.set_buffer_config( GSmtp::ServerBufferIn::Config() )
			.set_domain( domain ) ;
}

GPop::Store::Config Main::Configuration::popStoreConfig() const
{
	return
		GPop::Store::Config()
			.set_by_name( contains("pop-by-name") )
			.set_by_name_mkdir( contains("pop-by-name") )
			.set_allow_delete( !contains("pop-no-delete") ) ;
}

GPop::Server::Config Main::Configuration::popServerConfig( const std::string & server_tls_profile ,
	const std::string & domain ) const
{
	return
		GPop::Server::Config()
			.set_allow_remote( _allowRemoteClients() )
			.set_port( _popPort() )
			.set_addresses( listeningNames("pop") )
			.set_net_server_peer_config(
				GNet::ServerPeer::Config()
					.set_socket_protocol_config( _socketProtocolConfig(server_tls_profile) )
					.set_idle_timeout( _idleTimeout() ) )
			.set_net_server_config( _netServerConfig(_popServerSocketLinger()) )
			.set_protocol_config(
				GPop::ServerProtocol::Config()
					.set_sasl_server_challenge_domain( domain ) )
			.set_sasl_server_config( _popSaslServerConfig() ) ;
}

std::pair<int,int> Main::Configuration::_clientSocketLinger() const
{
	Switches switches( stringValue( "client-smtp-config" ) , false ) ;
	bool nolinger = !switches( "linger" , true ) ;
	return nolinger ? std::make_pair(1,0) : std::make_pair(-1,-1) ;
}

GSmtp::Client::Config Main::Configuration::smtpClientConfig( const std::string & client_tls_profile ,
	const std::string & filter_domain , const std::string & client_domain ) const
{
	std::string local_address_str = clientBindAddress() ;
	GNet::Address local_address = GNet::Address::defaultAddress() ;
	if( !local_address_str.empty() )
	{
		if( GNet::Address::validString(local_address_str,GNet::Address::NotLocal()) )
			local_address = GNet::Address::parse( local_address_str , GNet::Address::NotLocal() ) ;
		else
			local_address = GNet::Address::parse( local_address_str , 0U ) ;
	}

	Switches switches( stringValue( "client-smtp-config" ) ) ;
	return
		GSmtp::Client::Config()
			.set_client_protocol_config(
				GSmtp::ClientProtocol::Config()
					.set_ehlo( client_domain )
					.set_response_timeout( _responseTimeout() )
					.set_ready_timeout( _promptTimeout() )
					.set_use_starttls_if_possible( clientTls() && !clientOverTls() )
					.set_must_use_tls( contains("client-tls-required") && !clientOverTls() )
					.set_authentication_fallthrough( false )
					.set_anonymous( anonymous("client") )
					.set_must_accept_all_recipients( !contains("forward-to-some") )
					.set_pipelining( switches("pipelining",false) )
					.set_smtputf8_strict( switches("smtputf8strict",true) )
					.set_eightbit_strict( switches("eightbitstrict",false) )
					.set_binarymime_strict( switches("binarymimestrict",true) ) )
			.set_net_client_config(
				GNet::Client::Config()
					.set_stream_socket_config( _netSocketConfig(_clientSocketLinger()) )
					.set_line_buffer_config( GNet::LineBuffer::Config::smtp() )
					.set_bind_local_address( !local_address_str.empty() )
					.set_local_address( local_address )
					.set_connection_timeout( _connectionTimeout() )
					.set_socket_protocol_config(
						GNet::SocketProtocol::Config()
							.set_client_tls_profile( client_tls_profile )
							.set_secure_connection_timeout( _secureConnectionTimeout() ) ) )
			.set_filter_config(
				GSmtp::Filter::Config()
					.set_domain( filter_domain )
					.set_timeout( _filterTimeout() ) )
			.set_filter_spec( _clientFilter() )
			.set_secure_tunnel( clientOverTls() )
			.set_sasl_client_config( _smtpSaslClientConfig() )
			.set_fail_if_no_remote_recipients()
			.set_log_msgid( logFormatContains("msgid") ) ;
}

GSmtp::AdminServer::Config Main::Configuration::adminServerConfig( const G::StringMap & info_map ,
	const std::string & client_tls_profile , const std::string & filter_domain ,
	const std::string & client_domain ) const
{
	return
		GSmtp::AdminServer::Config()
			.set_port( _adminPort() )
			.set_with_terminate( contains("admin-terminate") )
			.set_allow_remote( _allowRemoteClients() )
			.set_remote_address( serverAddress() )
			.set_info_commands( info_map )
			.set_smtp_client_config( smtpClientConfig(client_tls_profile,filter_domain,client_domain) )
			.set_net_server_config( _netServerConfig(_adminServerSocketLinger()) )
			.set_net_server_peer_config(
				GNet::ServerPeer::Config()
					.set_socket_protocol_config(
						GNet::SocketProtocol::Config()
							.set_secure_connection_timeout( _connectionTimeout() ) )
					.set_idle_timeout( 0 ) ) ; // not idleTimeout()
}

GNet::SocketProtocol::Config Main::Configuration::_socketProtocolConfig( const std::string & server_tls_profile ) const
{
	return
		GNet::SocketProtocol::Config()
			.set_server_tls_profile( server_tls_profile )
			.set_secure_connection_timeout( _connectionTimeout() ) ;
}

// ==

Main::Configuration::Switches::Switches( std::string_view value , bool warn ) :
	m_items(G::Str::splitIntoFields(value,',')) ,
	m_warn(warn)
{
}

Main::Configuration::Switches::~Switches()
{
	try
	{
		for( const auto & item : m_items )
		{
			G_WARNING_IF( m_warn , "Main::Configuration::Switches::dtor: unknown smtp-config item [" << item << "]" ) ;
		}
	}
	catch(...) // dtor
	{
	}
}

bool Main::Configuration::Switches::remove( std::string_view item )
{
	return remove( G::sv_to_string(item) ) ;
}

bool Main::Configuration::Switches::remove( const std::string & item )
{
	auto const end = m_items.end() ;
	auto pos = std::find( m_items.begin() , end , item ) ;
	bool result = pos != end ;
	if( pos != end ) m_items.erase( pos ) ;
	return result ;
}

std::string Main::Configuration::Switches::str( char c , std::string_view item )
{
	return std::string(1U,c).append(item.data(),item.size()) ;
}

std::string Main::Configuration::Switches::str( std::string_view prefix , std::string_view item )
{
	return G::sv_to_string(prefix).append(item.data(),item.size()) ;
}

bool Main::Configuration::Switches::operator()( std::string_view item , bool default_ )
{
	if( remove(item) ) return true ;
	if( remove(str('+',item)) ) return true ;
	if( remove(str("with-",item)) ) return true ;
	if( remove(str('-',item)) ) return false ;
	if( remove(str("no",item)) ) return false ;
	if( remove(str("without-",item)) ) return false ;
	if( remove(str('=',item)) ) return default_ ;
	return default_ ;
}

// ==

unsigned int Main::Configuration::_adminPort() const noexcept { return numberValue( "admin" , 0U ) ; }
std::pair<int,int> Main::Configuration::_adminServerSocketLinger() const noexcept { return std::make_pair( -1 , -1 ) ; }
bool Main::Configuration::_allowRemoteClients() const noexcept { return contains( "remote-clients" ) ; }
GSmtp::FilterFactoryBase::Spec Main::Configuration::_clientFilter() const { return filterValue( "client-filter" ) ; }
unsigned int Main::Configuration::_connectionTimeout() const noexcept { return numberValue( "connection-timeout" , 40U ) ; }
GSmtp::FilterFactoryBase::Spec Main::Configuration::_filter() const { return filterValue( "filter" ) ; }
unsigned int Main::Configuration::_filterTimeout() const noexcept { return numberValue( "filter-timeout" , 60U ) ; }
unsigned int Main::Configuration::_idleTimeout() const noexcept { return numberValue( "idle-timeout" , 1800U ) ; }
unsigned int Main::Configuration::_maxSize() const noexcept { return numberValue( "size" , 0U ) ; }
unsigned int Main::Configuration::_popPort() const noexcept { return numberValue( "pop-port" , 110U ) ; }
std::string Main::Configuration::_popSaslServerConfig() const { return stringValue( "server-auth-config" ) ; }
std::pair<int,int> Main::Configuration::_popServerSocketLinger() const noexcept { return std::make_pair( -1 , -1 ) ; }
unsigned int Main::Configuration::_port() const noexcept { return numberValue( "port" , 25U ) ; }
unsigned int Main::Configuration::_promptTimeout() const noexcept { return numberValue( "prompt-timeout" , 20U ) ; }
unsigned int Main::Configuration::_responseTimeout() const noexcept { return numberValue( "response-timeout" , 1800U ) ; }
unsigned int Main::Configuration::_secureConnectionTimeout() const noexcept { return _connectionTimeout() ; }
bool Main::Configuration::_serverTlsRequired() const noexcept { return contains( "server-tls-required" ) ; }
std::string Main::Configuration::_show() const { return stringValue( "show" ) ; }
int Main::Configuration::_shutdownHowOnQuit() const noexcept { return 1 ; }
std::string Main::Configuration::_smtpSaslClientConfig() const { return stringValue( "client-auth-config" ) ; }
std::string Main::Configuration::_smtpSaslServerConfig() const { return stringValue( "server-auth-config" ) ; }
GSmtp::VerifierFactoryBase::Spec Main::Configuration::_verifier() const { return verifierValue( "address-verifier" ) ; }
bool Main::Configuration::_nodaemon() const noexcept { return contains( "no-daemon" ) || contains( "as-client" ) ; }

std::string Main::Configuration::clientBindAddress() const { return stringValue( "client-interface" ) ; }
bool Main::Configuration::clientOverTls() const noexcept { return contains( "client-tls-connection" ) ; }
G::Path Main::Configuration::clientSecretsFile() const { return contains( "client-auth" ) ? pathValue( "client-auth" ) : G::Path() ; }
G::Path Main::Configuration::clientTlsCaList() const { return contains( "client-tls-verify" ) ? pathValue( "client-tls-verify" ) : G::Path() ; }
G::Path Main::Configuration::clientTlsCertificate() const { return certificateFile( "client-tls-certificate" ) ; }
bool Main::Configuration::clientTls() const noexcept { return contains( "client-tls" ) ; }
std::string Main::Configuration::clientTlsPeerCertificateName() const { return stringValue( "client-tls-verify-name" ) ; }
std::string Main::Configuration::clientTlsPeerHostName() const { return stringValue( "client-tls-server-name" ) ; }
G::Path Main::Configuration::clientTlsPrivateKey() const { return keyFile( "client-tls-certificate" ) ; }
bool Main::Configuration::closeStderr() const noexcept { return contains( "close-stderr" ) || contains( "as-proxy" ) || contains( "as-server" ) ; }
bool Main::Configuration::daemon() const noexcept { return !_nodaemon() ; }
bool Main::Configuration::debug() const noexcept { return contains( "debug" ) ; }
std::string Main::Configuration::dnsbl() const { return stringValue( "dnsbl" ) ; }
std::string Main::Configuration::domain( std::function<std::string()> default_domain_fn ) const { return stringValue( "domain" , default_domain_fn ) ; }
bool Main::Configuration::doAdmin() const noexcept { return contains( "admin" ) ; }
bool Main::Configuration::doPolling() const noexcept { return contains( "poll" ) && pollingTimeout() > 0U ; }
bool Main::Configuration::doPop() const noexcept { return contains( "pop" ) ; }
bool Main::Configuration::doServing() const noexcept { return !contains( "dont-serve" ) && !contains( "as-client" ) ; }
bool Main::Configuration::doSmtp() const noexcept { return !contains( "no-smtp" ) ; }
bool Main::Configuration::forwardOnDisconnect() const noexcept { return contains( "forward-on-disconnect" ) || contains( "as-proxy" ) ; }
bool Main::Configuration::forwardOnStartup() const noexcept { return contains( "forward" ) || contains( "as-client" ) ; }
bool Main::Configuration::hidden() const noexcept { return contains( "hidden" ) ; }
bool Main::Configuration::immediate() const noexcept { return contains( "immediate" ) ; }
G::Path Main::Configuration::deliveryDir() const { return contains("delivery-dir") ? pathValue("delivery-dir") : spoolDir() ; }
bool Main::Configuration::log() const noexcept { return contains( "log" ) || contains( "as-client" ) || contains( "as-proxy" ) || contains( "as-server" ) ; }
std::string Main::Configuration::logFile() const { return contains("log-file") ? pathValue("log-file").str() : std::string() ; }
G::Path Main::Configuration::pidFile() const { return pathValue( "pid-file" ) ; }
bool Main::Configuration::pollingLog() const noexcept { return doPolling() && pollingTimeout() > 60U ; }
unsigned int Main::Configuration::pollingTimeout() const noexcept { return numberValue( "poll" , 0U ) ; }
G::Path Main::Configuration::popSecretsFile() const { return contains( "pop-auth" ) ? pathValue( "pop-auth" ) : G::Path() ; }
G::Path Main::Configuration::serverSecretsFile() const { return contains( "server-auth" ) ? pathValue( "server-auth" ) : G::Path() ; }
G::Path Main::Configuration::serverTlsCaList() const { return contains( "server-tls-verify" ) ? pathValue( "server-tls-verify" ) : G::Path() ; }
G::Path Main::Configuration::serverTlsCertificate() const { return certificateFile( "server-tls-certificate" ) ; }
bool Main::Configuration::serverTlsConnection() const noexcept { return contains( "server-tls-connection" ) ; }
bool Main::Configuration::serverTls() const noexcept { return contains( "server-tls" ) ; }
G::Path Main::Configuration::serverTlsPrivateKey() const { return keyFile( "server-tls-certificate" ) ; }
std::string Main::Configuration::tlsConfig() const { return stringValue( "tls-config" ) ; }
bool Main::Configuration::usePidFile() const noexcept { return contains( "pid-file" ) ; }
std::string Main::Configuration::user() const { return stringValue( "user" , "daemon" ) ; }

