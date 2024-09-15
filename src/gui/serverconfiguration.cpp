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
/// \file serverconfiguration.cpp
///

#include "gdef.h"
#include "serverconfiguration.h"
#include "options.h"
#include "gbatchfile.h"
#include "gfile.h"
#include "gstringarray.h"
#include "gstringmap.h"
#include "goptionparser.h"
#include "goptionreader.h"
#include "gassert.h"
#include "glog.h"

namespace ServerConfigurationImp
{
	struct Token
	{
		const char * p ;
	} ;
	static std::string & operator<<( std::string & s , Token t )
	{
		s.append(",",s.empty()?0U:1U).append(t.p) ;
		return s ;
	}
}

ServerConfiguration::ServerConfiguration()
= default;

ServerConfiguration::ServerConfiguration( const G::Path & config_file ) :
	m_config(read(config_file))
{
}

G::MapFile ServerConfiguration::read( const G::Path & config_file )
{
	G::MapFile config ;
	if( !G::File::exists(config_file) )
	{
		; // leave it empty
	}
	else if( config_file.extension() == "bat" )
	{
		config = readBatchFile( config_file ) ;
	}
	else
	{
		config = G::MapFile( config_file , "config" ) ;
	}

	normalise( config ) ;
	return config ;
}

G::MapFile ServerConfiguration::readBatchFile( const G::Path & batch_file_path )
{
	// TODO refactor ServerConfiguration::readBatchFile() and Main::CommandLine

	// read the batch file
	const G::BatchFile batch_file( batch_file_path , std::nothrow ) ;
	if( batch_file.args().empty() )
		return {} ;

	// parse once to see if there is a config file
	const G::Options options_spec = Main::Options::spec() ;
	G::Path config_file ;
	{
		G::StringArray errors ;
		G::OptionMap option_map ;
		G::StringArray parsed_args = G::OptionParser::parse( batch_file.args() , options_spec , option_map , &errors ) ;
		if( errors.empty() && parsed_args.size() == 1U )
			config_file = parsed_args[0] ;
	}

	// assemble all args from the batch file combined with any config file that it refers to
	G::StringArray all_args = batch_file.args() ;
	G_ASSERT( config_file.empty() || all_args.size() >= 2U ) ;
	if( !config_file.empty() && all_args.size() >= 2U )
	{
		all_args.pop_back() ;
		std::string app_value = G::Path(all_args[0]).dirname().str() ;
		if( !app_value.empty() )
			config_file.replace( "@app" , app_value ) ;
		G::OptionReader::add( all_args , config_file ) ;
	}

	// parse again
	G::OptionMap option_map ;
	{
		G::StringArray errors ; // ignored -- empty option_map on error
		G::OptionParser::parse( all_args , options_spec , option_map , &errors ) ;
	}

	return G::MapFile( option_map , G::Str::positive() ) ;
}

void ServerConfiguration::normalise( G::MapFile & config )
{
	// normalise by expanding "--as-whatever" etc.
	std::string const yes = G::Str::positive() ;
	if( config.contains("as-client") )
	{
		config.add( "log" , yes ) ;
		config.add( "no-syslog" , yes ) ;
		config.add( "no-daemon" , yes ) ;
		config.add( "dont-serve" , yes ) ;
		config.add( "forward" , yes ) ;
		config.add( "forward-to" , config.value("as-client") ) ;
		config.remove( "as-client" ) ;
	}
	if( config.contains("as-proxy") )
	{
		config.add( "log" , yes ) ;
		config.add( "close-stderr" , yes ) ;
		config.add( "forward-on-disconnect" , yes ) ; // was poll 0
		config.add( "forward-to" , config.value("as-proxy") ) ;
		config.remove( "as-proxy" ) ;
	}
	if( config.contains("as-server") )
	{
		config.add( "log" , yes ) ;
		config.add( "close-stderr" , yes ) ;
		config.remove( "as-server" ) ;
	}
	if( config.booleanValue("syslog",false) )
	{
		config.add( "no-syslog" , G::Str::negative() ) ;
	}
	if( config.contains("log-time") )
	{
		config.add( "log-format" , "time" ) ;
		config.remove( "log-time" ) ;
	}
	if( config.contains("log-address") )
	{
		config.add( "log-format" , "address" ) ;
		config.remove( "log-address" ) ;
	}
}

std::string ServerConfiguration::quote( const std::string & s )
{
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

std::string ServerConfiguration::exe( const G::Path & config_file )
{
	return
		G::File::exists(config_file) &&
		config_file.extension() == "bat" &&
		!G::BatchFile(config_file,std::nothrow).args().empty() ?
			G::BatchFile(config_file,std::nothrow).args().at(0U) :
			std::string() ;
}

G::StringArray ServerConfiguration::args( bool no_close_stderr ) const
{
	G::StringArray result ;
	for( const auto & map_item : m_config.map() )
	{
		std::string option = map_item.first ;
		std::string option_arg = map_item.second ;

		if( no_close_stderr && option == "close-stderr" )
			continue ;

		result.push_back( "--" + option ) ;
		if( ! option_arg.empty() )
		{
			result.push_back( quote(option_arg) ) ;
		}
	}
	return result ;
}

ServerConfiguration ServerConfiguration::fromPages( const G::MapFile & pages )
{
	using Token = ServerConfigurationImp::Token ;
	G::StringMap out ;

	std::string auth = G::Path(pages.value("dir-config"),"emailrelay.auth").str() ;

	out["spool-dir"] = pages.value("dir-spool") ;
	out["log"] ;
	out["close-stderr"] ;
	out["pid-file"] = G::Path(pages.value("dir-run"),"emailrelay.pid").str() ;
	if( pages.booleanValue("do-smtp",true) )
	{
		if( pages.booleanValue("forward-immediate",true) )
		{
			out["immediate"] ;
		}
		else if( pages.booleanValue("forward-on-disconnect",true) )
		{
			out["forward-on-disconnect"] ; // was poll 0
		}
		if( pages.booleanValue("forward-poll",true) )
		{
			out["poll"] = pages.value("forward-poll-period") ;
		}
		if( pages.value("smtp-server-port") != "25" )
		{
			out["port"] = pages.value("smtp-server-port") ;
		}
		if( pages.booleanValue("smtp-server-auth",true) )
		{
			out["server-auth"] = auth ;
		}
		if( pages.booleanValue("smtp-server-tls",false) )
		{
			out["server-tls"] ;
			out["server-tls-certificate"] = pages.value("smtp-server-tls-certificate") ;
		}
		else if( pages.booleanValue("smtp-server-tls-connection",false) )
		{
			out["server-tls-connection"] ;
			out["server-tls-certificate"] = pages.value("smtp-server-tls-certificate") ;
		}
		out["forward-to"] = pages.value("smtp-client-host") + ":" + pages.value("smtp-client-port") ;
		if( pages.booleanValue("smtp-client-tls",true) )
		{
			out["client-tls"] ;
		}
		if( pages.booleanValue("smtp-client-tls-connection",true) )
		{
			out["client-tls-connection"] ;
		}
		if( pages.booleanValue("smtp-client-auth",true) )
		{
			out["client-auth"] = auth ;
		}
		if( !pages.value("filter-server").empty() )
		{
			out["filter"] = pages.value("filter-server") ;
		}
		if( !pages.value("filter-client").empty() )
		{
			out["client-filter"] = pages.value("filter-client") ;
		}
	}
	else
	{
		out["no-smtp"] ;
	}
	if( pages.booleanValue("do-pop",true) )
	{
		out["pop"] ;
		if( pages.value("pop-port") != "110" )
		{
			out["pop-port"] = pages.value("pop-port") ;
		}
		if( pages.booleanValue("pop-shared-no-delete",true) )
		{
			out["pop-no-delete"] ;
		}
		if( pages.booleanValue("pop-by-name",true) )
		{
			out["pop-by-name"] ;
		}
		out["pop-auth"] = auth ;
	}
	if( pages.booleanValue("logging-verbose",true) )
	{
		out["verbose"] ;
	}
	if( pages.booleanValue("logging-debug",true) )
	{
		out["debug"] ;
	}
	if( !pages.booleanValue("logging-syslog",true) )
	{
		out["no-syslog"] ;
	}
	if( !pages.value("logging-file").empty() )
	{
		out["log-file"] = pages.value("logging-file") ;
	}
	if( pages.booleanValue("logging-time",true) )
	{
		out["log-format"] << Token{"time"} ;
	}
	if( pages.booleanValue("logging-address",true) )
	{
		out["log-format"] << Token{"address"} ;
	}
	if( pages.booleanValue("logging-port",true) )
	{
		out["log-format"] << Token{"port"} ;
	}
	if( pages.booleanValue("logging-msgid",true) )
	{
		out["log-format"] << Token{"msgid"} ;
	}
	if( pages.booleanValue("listening-remote",true) )
	{
		out["remote-clients"] ;
	}
	if( !pages.value("listening-interface").empty() )
	{
		out["interface"] = pages.value("listening-interface") ;
	}

	ServerConfiguration result ;
	result.m_config = G::MapFile( out ) ;
	return result ;
}

const G::MapFile & ServerConfiguration::map() const
{
	return m_config ;
}

