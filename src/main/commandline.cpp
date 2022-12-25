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
/// \file commandline.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "options.h"
#include "gmessagestore.h"
#include "ggetopt.h"
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
/// A private implementation class used by Main::CommandLine
/// to generate user feedback.
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
	// look for special config names
	G::StringArray special_names ;
	{
		for( std::size_t i = 1U ; i < args_in.c() ; i++ )
		{
			std::string arg = args_in.v( i ) ;
			if( G::Str::headMatch(arg,"--spool-dir-") )
				special_names.push_back( G::Str::head(arg.substr(12U),"=",false) ) ;
		}
	}
	if( !special_names.empty() )
	{
		for( std::size_t i = 0U ; i < special_names.size() ; i++ )
		{
			m_option_maps.emplace_back() ;
			G::OptionParser parser( options_spec , m_option_maps.back() , &m_errors ) ;
			G::StringArray args = parser.parse( args_in.array() , 1U , 0U ,
				[&special_names,i]( const std::string & name_ ){ return onParse(special_names,i,name_) ; } ) ;
			if( !args.empty() && m_errors.empty() )
				m_errors.push_back( "config files cannot be used with a multi-dimensional command-line" ) ;
		}
		m_errors.erase( std::unique( m_errors.begin() , m_errors.end() ) , m_errors.end() ) ;
	}
	else
	{
		// normal command-line parsing
		m_option_maps.emplace_back() ;
		G::OptionParser parser0( options_spec , m_option_maps.back() , &m_errors ) ;
		G::StringArray args = parser0.parse( args_in.array() ) ;
		m_verbose = m_option_maps.back().contains( "verbose" ) ;

		// any non-option arguments are treated as config files - each
		// config file after the first creates a new configuration
		if( m_errors.empty() )
		{
			std::string app_dir = G::Path(G::Process::exe()).dirname().str() ;
			for( std::size_t i = 0U ; i < args.size() ; i++ )
			{
				std::string config_file = args[i] ;
				if( config_file.find("@app") == 0U && !app_dir.empty() )
					G::Str::replace( config_file , "@app" , app_dir ) ;

				if( i != 0U ) m_option_maps.push_back( G::OptionMap() ) ;
				G::OptionParser parser( options_spec , m_option_maps.back() , &m_errors ) ;
				parser.parse( G::OptionReader::read(config_file) , 0U ) ;
			}
		}
	}
}

Main::CommandLine::~CommandLine()
= default ;

std::string Main::CommandLine::onParse( const G::StringArray & special_names , std::size_t i , const std::string & name )
{
	// we want names like "port-in" where "in" is the current (i'th) special-name to be
	// parsed as if "port", but we want to ignore "port-out" where "out" is some
	// other (non-i'th) special name

	std::string _special = std::string(1U,'-').append(special_names.at(i)) ; // "-in"
	std::string result = name ;
	if( G::Str::tailMatch( name , _special ) )
	{
		result = name.substr( 0U , name.size()-_special.size() ) ; // "port"
	}
	else
	{
		for( const auto & special : special_names )
		{
			_special = std::string(1U,'-').append(special) ; // "-out"
			if( G::Str::tailMatch( name , _special ) ) // "port-out"
			{
				char discard = '-' ; // see OptionParser::parse()
				result = std::string(1U,discard).append( name.substr(0U,name.size()-_special.size()) ) ; // "-port"
				break ;
			}
		}
	}
	return result ;
}

std::size_t Main::CommandLine::configurations() const
{
	return m_option_maps.size() ;
}

const G::OptionMap & Main::CommandLine::options( std::size_t i ) const
{
	return m_option_maps.at( i ) ;
}

bool Main::CommandLine::argcError() const
{
	return false ; // we now accept any number of arguments
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
	if( !G::is_windows() || G::is_wine() )
	{
		bool enabled = GNet::Address::supports( GNet::Address::Family::local ) ;
		Show show( m_output , e , m_verbose ) ;
		show.s() << "Unix domain sockets: " << (enabled?"enabled":"disabled") << eot ;
	}
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
		showSslVersion( e , "\n" ) ;
	}
	showSslCredit( e , "\n" ) ;
	showWarranty( e ) ;
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

