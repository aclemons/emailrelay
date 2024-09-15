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
/// \file commandline.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "gpop.h"
#include "options.h"
#include "goptionparser.h"
#include "goptionreader.h"
#include "goptionsusage.h"
#include "gaddress.h"
#include "gprocess.h"
#include "gpath.h"
#include "gfile.h"
#include "gformat.h"
#include "ggettext.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"
#include <algorithm>

namespace Main
{
	class Show ;
}

//| \class Main::Show
/// Used by Main::CommandLine to generate user feedback via
/// the Main::Output interface.
///
class Main::Show
{
public:
	Show( Main::Output & , bool e , bool v ) ;
	std::ostream & s() ;
	~Show() ;

public:
	Show( const Show & ) = delete ;
	Show( Show && ) = delete ;
	Show & operator=( const Show & ) = delete ;
	Show & operator=( Show && ) = delete ;

private:
	static Show * m_this ;
	std::ostringstream m_ss ;
	Main::Output & m_output ;
	bool m_e ;
	bool m_v ;
} ;

// ==

Main::CommandLine::CommandLine( Output & output , const G::Arg & args_in ,
	const G::Options & options_spec ,
	const std::string & version ) :
		m_output(output) ,
		m_options_spec(options_spec) ,
		m_version(version) ,
		m_arg_prefix(args_in.prefix())
{
	const bool multiconfig = !G::Test::enabled("main-commandline-simple") ;
	if( multiconfig )
	{
		// basic parse just to see if there is a config file
		std::string config_file ;
		{
			G::StringArray errors ;
			G::OptionMap option_map ;
			G::StringArray args = G::OptionParser::parse( args_in.array() ,
				options_spec , option_map , &errors , 1U , 0U ,
					[options_spec](const std::string & s_,bool){
						auto option_p = parserFind( options_spec , s_ ) ;
						return option_p ? ("-"+option_p->name) : s_ ;
					} ) ;
			if( errors.empty() && !args.empty() )
				config_file = args[0] ;
		}

		// assemble all args, including from any config file
		G::StringArray args = args_in.array( 1U ) ;
		if( !config_file.empty() )
		{
			args.pop_back() ;
			G::OptionReader::add( args , configFile(config_file) ) ;
		}

		// parse again looking for config names
		G::StringArray config_names ;
		{
			G::StringArray errors ;
			G::OptionMap option_map ;
			G::OptionParser::parse( args ,
				options_spec , option_map , &errors , 0U , 0U ,
					[&config_names,options_spec](const std::string & s_,bool){
						auto option_p = parserFind( options_spec , s_ ) ;
						if( option_p && option_p->name == "spool-dir" )
							config_names.push_back( s_.substr(0U,s_.size()-9U) ) ;
						return option_p ? ("-"+option_p->name) : s_ ;
					} ) ;
			if( !errors.empty() )
				config_names.clear() ;
		}
		if( config_names.empty() )
			config_names.emplace_back() ;

		// parse each config name
		for( std::size_t i = 0U ; i < config_names.size() ; i++ )
		{
			m_config_names.push_back( config_names[i] ) ;
			m_option_maps.emplace_back() ;

			G::StringArray new_args = G::OptionParser::parse( args ,
				options_spec , m_option_maps.back() , &m_errors , 0U , 0U ,
				[&config_names,options_spec,i](const std::string & s_,bool){
					std::string prefix ;
					auto option_p = parserFind( options_spec , s_ , &prefix ) ;
					if( option_p == nullptr )
						return s_ ;
					else if( s_ == option_p->name && i == 0U )
						return option_p->name ; // no prefix, first config
					else if( s_ == option_p->name )
						return "-"+option_p->name ; // no prefix, not first config
					else if( s_ == (config_names[i]+option_p->name) )
						return option_p->name ; // our prefix
					else if( std::find(config_names.begin(),config_names.end(),prefix+"-") != config_names.end() )
						return "-"+option_p->name ; // valid prefix but not ours
					else
						return s_ ; // invalid, fail as normal
				} ) ;

			if( !new_args.empty() )
				m_argc_error = true ;
			if( m_option_maps.back().contains("verbose") )
				m_verbose = true ;
		}

		std::for_each( m_config_names.begin() , m_config_names.end() , [](std::string &s_){G::Str::trimRight(s_,"-",1U);} ) ;
		std::sort( m_errors.begin() , m_errors.end() ) ;
		m_errors.erase( std::unique( m_errors.begin() , m_errors.end() ) , m_errors.end() ) ;
	}
	else
	{
		m_config_names.emplace_back() ;
		m_option_maps.emplace_back() ;
		G::StringArray args = G::OptionParser::parse( args_in.array() , m_options_spec , m_option_maps[0] , &m_errors ) ;
		if( m_errors.empty() && !args.empty() )
			G::OptionParser::parse( G::OptionReader::read(configFile(args[0])) , m_options_spec , m_option_maps[0] , &m_errors , 0U ) ;
	}
	if( G::Test::enabled("main-commandline-dump") )
		dump() ;
}

Main::CommandLine::~CommandLine()
= default ;

G::Path Main::CommandLine::configFile( const std::string & arg )
{
	G::Path path( arg ) ;
	path.replace( "@app" , G::Process::exe().dirname().str() ) ;
	return path ;
}

void Main::CommandLine::dump() const
{
	for( std::size_t i = 0U ; i < m_config_names.size() ; i++ )
	{
		std::cout << m_config_names.at(i) << "...\n" ;
		for( auto p = m_option_maps.at(i).begin() ; p != m_option_maps.at(i).end() ; ++p ) // NOLINT modernize-loop-convert
		{
			std::cout << "  " << (*p).first << "=[" << (*p).second.value() << "] (" << (*p).second.count() << ")\n" ;
		}
	}
}

const G::Option * Main::CommandLine::parserFind( const G::Options & options_spec ,
	const std::string & parser_name , std::string * prefix_p )
{
	// exact match
	const G::Option * option_p = options_spec.find( parser_name ) ;
	if( option_p )
		return option_p ;

	// tail match
	std::string tail = G::Str::tail( parser_name , "-" ) ;
	if( tail.empty() )
		return nullptr ;
	if( prefix_p )
		*prefix_p = G::Str::head( parser_name , "-" ) ;
	return options_spec.find( tail ) ;
}

std::size_t Main::CommandLine::configurations() const
{
	return m_option_maps.size() ;
}

const G::OptionMap & Main::CommandLine::configurationOptionMap( std::size_t i ) const
{
	return m_option_maps.at( i ) ;
}

std::string Main::CommandLine::configurationName( std::size_t i ) const
{
	return m_config_names.at( i ) ;
}

bool Main::CommandLine::argcError() const
{
	return m_argc_error ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return !m_errors.empty() ;
}

void Main::CommandLine::showUsage( bool is_error ) const
{
	std::string args_help = " [<config-file>]" ;
	auto layout = m_output.outputLayout( m_verbose ) ;
	layout.set_column( m_verbose ? 42U : 30U ) ;
	layout.set_extra( m_verbose ) ;
	layout.set_alt_usage( !m_verbose ) ;
	layout.set_level_max( m_verbose ? 99U : 20U ) ;

	G::OptionsUsage usage( m_options_spec.list() ) ;
	if( m_verbose )
	{
		// show help in sections...
		bool help_state = false ;
		usage.help( layout , &help_state ) ;
		Show show( m_output , is_error , m_verbose ) ;
		show.s() << usage.summary(layout,m_arg_prefix,args_help) << "\n" ;
		auto tags = Options::tags() ;
		for( const auto & tag : tags )
		{
			layout.set_main_tag( tag.first ) ;
			show.s() << "\n" << tag.second << "\n" << usage.help(layout,&help_state) ;
		}
	}
	else
	{
		Show show( m_output , is_error , m_verbose ) ;
		usage.output( layout , show.s() , m_arg_prefix , args_help ) ;
	}
}

void Main::CommandLine::showUsageErrors( bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	for( const auto & error : m_errors )
	{
		show.s() << m_arg_prefix << ": error: " << error << "\n" ;
	}
	showShortHelp( e ) ;
}

void Main::CommandLine::showArgcError( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg_prefix << ": " << txt("usage error: too many non-option arguments") << std::endl ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showShortHelp( bool e ) const
{
	using G::format ;
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	const std::string & exe = m_arg_prefix ;
	show.s()
		<< std::string(exe.length()+2U,' ')
		<< str(format(txt("try \"%1%\" for more information"))%(exe+" --help --verbose")) << std::endl ;
}

void Main::CommandLine::showHelp( bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	showBanner( e ) ;
	show.s() << std::endl ;
	showUsage( e ) ;
	showExtraHelp( e ) ;
	showCopyright( e ) ;
}

void Main::CommandLine::showExtraHelp( bool e ) const
{
	using G::format ;
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	const std::string & exe = m_arg_prefix ;

	show.s() << "\n" ;
	if( m_verbose )
	{
		show.s()
			<< txt("To start a 'storage' daemon in background...") << "\n"
			<< "   " << exe << " --as-server\n\n"
			//
			<< txt("To forward stored mail to \"mail.myisp.net\"...") << "\n"
			<< "   " << exe << " --as-client mail.myisp.net:smtp\n\n"
			//
			<< txt("To run as a proxy (on port 10025) to a local server (on port 25)...") << "\n"
			<< "   " << exe << " --port 10025 --as-proxy localhost:25\n"
			<< std::endl ;
	}
	else
	{
		show.s()
			<< format(txt("For complete usage information run \"%1%\"")) % (exe+" --help --verbose") << "\n"
			<< std::endl ;
	}
}

void Main::CommandLine::showNothingToSend( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg_prefix << ": " << txt("no messages to send") << std::endl ;
}

void Main::CommandLine::showNothingToDo( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg_prefix << ": " << txt("nothing to do") << std::endl ;
}

void Main::CommandLine::showFinished( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg_prefix << ": " << txt("finished") << std::endl ;
}

void Main::CommandLine::showBanner( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s()
		<< "E-MailRelay V" << m_version << std::endl << eot ;
}

void Main::CommandLine::showCopyright( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << Legal::copyright() << std::endl << eot ;
}

void Main::CommandLine::showWarranty( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << Legal::warranty("","\n") << eot ;
}

void Main::CommandLine::showSslCredit( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << GSsl::Library::credit("","\n",eot) ;
}

void Main::CommandLine::showSslVersion( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << "TLS library: " << GSsl::Library::ids() << std::endl << eot ;
}

void Main::CommandLine::showThreading( bool e , const std::string & eot ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << "Multi-threading: " << (G::threading::works()?"enabled":"disabled") << eot ;
}

void Main::CommandLine::showUds( bool e , const std::string & eot ) const
{
	if( !G::is_windows() )
	{
		bool enabled = GNet::Address::supports( GNet::Address::Family::local ) ;
		Show show( m_output , e , m_verbose ) ;
		show.s() << "Unix domain sockets: " << (enabled?"enabled":"disabled") << eot ;
	}
}

void Main::CommandLine::showPop( bool e , const std::string & eot ) const
{
	bool enabled = GPop::enabled() ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << "POP server: " << (enabled?"enabled":"disabled") << eot ;
}

void Main::CommandLine::showVersion( bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	showBanner( e , "\n" ) ;
	showCopyright( e , "\n" ) ;
	if( m_verbose )
	{
		showThreading( e , "\n" ) ;
		showUds( e , "\n" ) ;
		showPop( e , "\n" ) ;
		showAdmin( e , "\n" ) ;
		showSslVersion( e , "\n" ) ;
	}
	showSslCredit( e , "\n" ) ;
	showWarranty( e ) ;
}

void Main::CommandLine::showAdmin( bool e , const std::string & eot ) const
{
	bool enabled = GSmtp::AdminServer::enabled() ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << "Admin server: " << (enabled?"enabled":"disabled") << eot ;
}

void Main::CommandLine::showSemanticError( const std::string & error ) const
{
	using G::txt ;
	Show show( m_output , true , m_verbose ) ;
	show.s() << m_arg_prefix << ": " << txt("usage error: ") << error << std::endl ;
}

void Main::CommandLine::showSemanticWarnings( const G::StringArray & warnings ) const
{
	using G::txt ;
	if( !warnings.empty() )
	{
		Show show( m_output , true , m_verbose ) ;
		const char * warning = txt( "warning" ) ;
		std::string sep = std::string(1U,'\n').append(m_arg_prefix).append(": ",2U).append(warning).append(": ",2U) ;
		show.s() << m_arg_prefix << ": " << warning << ": "
			<< G::Str::join(sep,warnings) << std::endl ;
	}
}

void Main::CommandLine::logSemanticWarnings( const G::StringArray & warnings ) const
{
	for( const auto & warning : warnings )
		G_WARNING( "CommandLine::logSemanticWarnings: " << warning ) ;
}

// ===

Main::Show * Main::Show::m_this = nullptr ;

Main::Show::Show( Output & output , bool e , bool v ) :
	m_output(output) ,
	m_e(e) ,
	m_v(v)
{
	if( m_this == nullptr )
		m_this = this ;
}

std::ostream & Main::Show::s()
{
	return m_this->m_ss ;
}

Main::Show::~Show()
{
	try
	{
		if( m_this == this )
		{
			m_this = nullptr ;
			m_output.output( m_ss.str() , m_e , m_v ) ;
		}
	}
	catch(...) // dtor
	{
	}
}

