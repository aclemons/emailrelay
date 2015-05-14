//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// serverconfiguration.cpp
//

#include "gdef.h"
#include "serverconfiguration.h"
#include "gbatchfile.h"
#include "gfile.h"
#include "goptionparser.h"
#include "gdebug.h"

ServerConfiguration::ServerConfiguration()
{
}

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
		// read the batch file and parse the command-line
		G::BatchFile batch_file( config_file , G::BatchFile::NoThrow() ) ;
		G::StringArray const args = batch_file.args() ;
		if( args.size() != 0U )
			config = G::OptionParser::parse( spec() , args , 1U , true ) ;
	}
	else
	{
		// TODO maybe push through OptionParser to eliminate cruft
		config = G::MapFile( config_file ) ;
	}

	// normalise
	std::string const yes = "yes" ;
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
		config.add( "poll" , "0" ) ; // TODO "forward-on-disconnect"
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
		config.add( "no-syslog" , "no" ) ;
	}

	return config ;
}

std::string ServerConfiguration::quote( std::string s , bool escape_spaces )
{
	if( escape_spaces )
		G::Str::replaceAll( s , " " , "\\ " ) ;
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

std::string ServerConfiguration::spec()
{
	// TODO refactor wrt. src/main
	return
		"l!log!...!!0!!2|"
		"t!no-daemon!...!!0!!3|"
		"k!syslog!...!!0!!3|"
		"n!no-syslog!...!!0!!3|"
		"c!icon!...!!1!0123!0|"
		"H!hidden!...!!0!!3|"
		"R!peer-lookup!...!!0!!3|"
		"q!as-client!...!!1!host:port!1|"
		"d!as-server!...!!0!!1|"
		"y!as-proxy!...!!1!host:port!1|"
		"v!verbose!...!!0!!1|"
		"h!help!...!!0!!1|"
		"p!port!...!!1!port!2|"
		"r!remote-clients!...!!0!!2|"
		"s!spool-dir!...!!1!dir!2|"
		"V!version!...!!0!!2|"
		"j!client-tls!...!!0!!3|"
		"b!client-tls-connection!...!!0!!3|"
		"K!server-tls!...!!1!pem-file!3|"
		"9!tls-config!...!!1!flags!3|"
		"g!debug!...!!0!!3|"
		"C!client-auth!...!!1!file!3|"
		"L!log-time!...!!0!!3|"
		"N!log-file!...!!1!file!3|"
		"S!server-auth!...!!1!file!3|"
		"e!close-stderr!...!!0!!3|"
		"a!admin!...!!1!admin-port!3|"
		"x!dont-serve!...!!0!!3|"
		"X!no-smtp!...!!0!!3|"
		"z!filter!...!!1!program!3|"
		"W!filter-timeout!...!!1!time!3|"
		"w!prompt-timeout!...!!1!time!3|"
		"D!domain!...!!1!fqdn!3|"
		"f!forward!...!!0!!3|"
		"o!forward-to!...!!1!host:port!3|"
		"T!response-timeout!...!!1!time!3|"
		"U!connection-timeout!...!!1!time!3|"
		"m!immediate!...!!0!!3|"
		"I!interface!...!!2!ip-list!3|"
		"6!client-interface!...!!1!ip!3|"
		"i!pid-file!...!!1!pid-file!3|"
		"O!poll!...!!1!period!3|"
		"P!postmaster!...!!0!!0|"
		"Z!verifier!...!!1!program!3|"
		"Y!client-filter!...!!1!program!3|"
		"Q!admin-terminate!...!!0!!3|"
		"A!anonymous!...!!0!!3|"
		"B!pop!...!!0!!3|"
		"E!pop-port!...!!1!port!3|"
		"F!pop-auth!...!!1!file!3|"
		"G!pop-no-delete!...!!0!!3|"
		"J!pop-by-name!...!!0!!3|"
		"M!size!...!!1!bytes!3"
	;
}

std::string ServerConfiguration::exe( const G::Path & config_file )
{
	return
		G::File::exists(config_file) && 
		config_file.extension() == "bat" && 
		!G::BatchFile(config_file,G::BatchFile::NoThrow()).args().empty() ?
			G::BatchFile(config_file,G::BatchFile::NoThrow()).args().at(0U) :
			std::string() ;
}

G::StringArray ServerConfiguration::args( bool no_close_stderr ) const
{
	G::StringArray result ;
	for( G::StringMap::const_iterator p = m_config.map().begin() ; p != m_config.map().end() ; ++p )
	{
		std::string option = (*p).first ;
		std::string option_arg = (*p).second ;

		if( no_close_stderr && option == "close-stderr" )
			continue ;

		result.push_back( "--" + option ) ;
		if( ! option_arg.empty() )
		{
			bool option_arg_is_commandline =
				result.back() == "--filter" || result.back() == "-z" ||
				result.back() == "--client-filter" || result.back() == "-Y" ||
				result.back() == "--verifier" || result.back() == "-Z" ;
			result.push_back( quote(option_arg,option_arg_is_commandline) ) ;
		}
	}
	return result ;
}

ServerConfiguration ServerConfiguration::fromPages( const G::MapFile & pages , const G::Path & copy_filter )
{
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
			out["poll"] = "0" ; // TODO "forward-on-disconnect"
		}
		if( pages.booleanValue("forward-poll",true) )
		{
			if( pages.value("forward-poll-period") == "minute" )
				out["poll"] = "60" ;
			else if( pages.value("forward-poll-period") == "second" )
				out["poll"] = "1" ;
			else
				out["poll"] = "3600" ;
		}
		if( pages.value("smtp-server-port") != "25" )
		{
			out["port"] = pages.value("smtp-server-port") ;
		}
		if( pages.booleanValue("smtp-server-auth",true) )
		{
			out["server-auth"] = auth ;
		}
		out["forward-to"] = pages.value("smtp-client-host") + ":" + pages.value("smtp-client-port") ;
		if( pages.booleanValue("smtp-client-tls",true) )
		{
			out["client-tls"] ;
		}
		if( pages.booleanValue("smtp-client-auth",true) )
		{
			out["client-auth"] = auth ;
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
		if( pages.booleanValue("pop-by-name-auto-copy",true) )
		{
			out["filter"] = copy_filter.str() ;
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
	if( pages.booleanValue("listening-remote",true) )
	{
		out["remote-clients"] ;
	}
	if( !pages.booleanValue("listening-all",true) && !pages.value("listening-interface").empty() )
	{
		out["interface"] = pages.value("listening-interface") ;
		out["client-interface"] = "0.0.0.0" ; // TODO ipv6 & separate widget
	}

	ServerConfiguration result ;
	result.m_config = G::MapFile( out ) ;
	return result ;
}

const G::MapFile & ServerConfiguration::map() const
{
	return m_config ;
}

/// \file serverconfiguration.cpp
