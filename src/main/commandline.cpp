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
#include "goptionsusage.h"
#include "gprocess.h"
#include "gpath.h"
#include "gfile.h"
#include "gformat.h"
#include "ggettext.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"

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

Main::CommandLine::CommandLine( Output & output , const G::Arg & arg ,
	const G::Options & spec ,
	const std::string & version ) :
		m_output(output) ,
		m_version(version) ,
		m_arg(arg) ,
		m_getopt(m_arg,spec) ,
		m_verbose(m_getopt.contains("verbose"))
{
	if( !m_getopt.hasErrors() && m_getopt.args().c() == 2U )
	{
		m_getopt.addOptionsFromFile( 1U , "@app" , G::Path(G::Process::exe()).dirname().str() ) ;
	}
}

Main::CommandLine::~CommandLine()
= default;

const G::OptionMap & Main::CommandLine::map() const
{
	return m_getopt.map() ;
}

const std::vector<G::Option> & Main::CommandLine::options() const
{
	return m_getopt.options() ;
}

std::size_t Main::CommandLine::argc() const
{
	return m_getopt.args().c() ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return m_getopt.hasErrors() ;
}

void Main::CommandLine::showUsage( bool is_error ) const
{
	std::string args = " [<config-file>]" ;
	auto layout = m_output.outputLayout( m_verbose ) ;
	layout.set_column( m_verbose ? 42U : 30U ) ;
	layout.set_extra( m_verbose ) ;
	layout.set_alt_usage( !m_verbose ) ;
	layout.set_level_max( m_verbose ? 99U : 20U ) ;

	G::OptionsUsage usage( m_getopt.options() ) ;
	if( m_verbose )
	{
		// show help in sections...
		bool help_state = false ;
		usage.help( layout , &help_state ) ;
		Show show( m_output , is_error , m_verbose ) ;
		show.s() << usage.summary(layout,m_arg.prefix(),args) << "\n" ;
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
		usage.output( layout , show.s() , m_arg.prefix() , args ) ;
	}
}

void Main::CommandLine::showUsageErrors( bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	m_getopt.showErrors( show.s() ) ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showArgcError( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg.prefix() << ": " << txt("usage error: too many non-option arguments") << std::endl ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showShortHelp( bool e ) const
{
	using G::format ;
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	const std::string & exe = m_arg.prefix() ;
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
	const std::string & exe = m_arg.prefix() ;

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
	show.s() << m_arg.prefix() << ": " << txt("no messages to send") << std::endl ;
}

void Main::CommandLine::showNothingToDo( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg.prefix() << ": " << txt("nothing to do") << std::endl ;
}

void Main::CommandLine::showFinished( bool e ) const
{
	using G::txt ;
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg.prefix() << ": " << txt("finished") << std::endl ;
}

void Main::CommandLine::showError( const std::string & reason , bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	show.s() << m_arg.prefix() << ": " << reason << std::endl ;
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
	show.s() << "Multi-threading: " << (G::threading::works()?"enabled":"disabled") << std::endl << eot ;
}

void Main::CommandLine::showVersion( bool e ) const
{
	Show show( m_output , e , m_verbose ) ;
	showBanner( e , "\n" ) ;
	showCopyright( e , "\n" ) ;
	if( m_verbose )
	{
		showThreading( e , "\n" ) ;
		showSslVersion( e , "\n" ) ;
	}
	showSslCredit( e , "\n" ) ;
	showWarranty( e ) ;
}

void Main::CommandLine::showSemanticError( const std::string & error ) const
{
	using G::txt ;
	Show show( m_output , true , m_verbose ) ;
	show.s() << m_arg.prefix() << ": " << txt("usage error: ") << error << std::endl ;
}

void Main::CommandLine::showSemanticWarnings( const G::StringArray & warnings ) const
{
	using G::txt ;
	if( !warnings.empty() )
	{
		Show show( m_output , true , m_verbose ) ;
		const char * warning = txt( "warning" ) ;
		std::string sep = std::string(1U,'\n').append(m_arg.prefix()).append(": ",2U).append(warning).append(": ",2U) ;
		show.s() << m_arg.prefix() << ": " << warning << ": "
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

