//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "goptions.h"
#include "legal.h"
#include "configuration.h"
#include "commandline.h"
#include "gmessagestore.h"
#include "ggetopt.h"
#include "gfile.h"
#include "gtest.h"
#include "gstr.h"
#include "glog.h"

namespace Main
{
	class Show ;
}

/// \class Main::Show
/// A private implementation class used by Main::CommandLine
/// to generate user feedback.
///
class Main::Show
{
public:
	Show( Main::Output & , bool e ) ;
	std::ostream & s() ;
	G::Options::Layout layout() const ;
	~Show() ;

private:
	Show( const Show & ) g__eq_delete ;
	void operator=( const Show & ) g__eq_delete ;

private:
	static Show * m_this ;
	std::ostringstream m_ss ;
	Main::Output & m_output ;
	bool m_e ;
} ;

// ==

Main::CommandLine::CommandLine( Output & output , const G::Arg & arg , const std::string & spec ,
	const std::string & version ) :
		m_output(output) ,
		m_version(version) ,
		m_arg(arg) ,
		m_getopt(m_arg,spec)
{
	if( !m_getopt.hasErrors() && m_getopt.args().c() == 2U )
	{
		std::string config_file = m_getopt.args().v(1U) ;
		if( sanityCheck( config_file ) )
			m_getopt.addOptionsFromFile( 1U ) ;
	}
	m_getopt.collapse( "pid-file" ) ; // allow multiple pidfiles but only if all the same
}

Main::CommandLine::~CommandLine()
{
}

const G::OptionMap & Main::CommandLine::map() const
{
	return m_getopt.map() ;
}

const G::Options & Main::CommandLine::options() const
{
	return m_getopt.options() ;
}

G::Arg::size_type Main::CommandLine::argc() const
{
	return m_getopt.args().c() ;
}

bool Main::CommandLine::sanityCheck( const G::Path & path )
{
	// a simple check to reject pem files since 'server-tls' no longer takes a value
	std::ifstream file ;
	if( path.extension() == "pem" )
		m_insanity = "invalid filename extension for config file: [" + path.str() + "]" ;
	G::File::open( file , path ) ;
	if( file.good() && G::Str::readLineFrom(file).find("---") == 0U )
		m_insanity = "invalid file format for config file: [" + path.str() + "]" ;
	return m_insanity.empty() ;
}

bool Main::CommandLine::hasUsageErrors() const
{
	return !m_insanity.empty() || m_getopt.hasErrors() ;
}

void Main::CommandLine::showUsage( bool e ) const
{
	Show show( m_output , e ) ;

	G::Options::Level level = G::Options::Level(2U) ;
	std::string introducer = G::Options::introducerDefault() ;
	if( m_getopt.contains("verbose") )
		level = G::Options::levelDefault() ;
	else
		introducer = std::string("abbreviated ") + introducer ;

	bool extra = m_getopt.contains("verbose") ;
	m_getopt.options().showUsage( show.s() , m_arg.prefix() , "[<config-file>]" ,
		introducer , level , show.layout() , extra ) ;
}

void Main::CommandLine::showUsageErrors( bool e ) const
{
	Show show( m_output , e ) ;
	if( !m_insanity.empty() )
		show.s() << m_arg.prefix() << ": error: " << m_insanity << std::endl ;
	else
		m_getopt.showErrors( show.s() ) ;
	showShortHelp( e ) ;
}

void Main::CommandLine::showArgcError( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": usage error: too many non-option arguments" << std::endl ;
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

void Main::CommandLine::showNothingToSend( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": no messages to send" << std::endl ;
}

void Main::CommandLine::showNothingToDo( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": nothing to do" << std::endl ;
}

void Main::CommandLine::showFinished( bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": finished" << std::endl ;
}

void Main::CommandLine::showError( const std::string & reason , bool e ) const
{
	Show show( m_output , e ) ;
	show.s() << m_arg.prefix() << ": " << reason << std::endl ;
}

void Main::CommandLine::showBanner( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s()
		<< "E-MailRelay V" << m_version << std::endl << eot ;
}

void Main::CommandLine::showCopyright( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::copyright() << std::endl << eot ;
}

void Main::CommandLine::showWarranty( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s() << Legal::warranty("","\n") << eot ;
}

void Main::CommandLine::showSslCredit( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s() << GSsl::Library::credit("","\n",eot) ;
}

void Main::CommandLine::showSslVersion( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s() << "TLS library: " << GSsl::Library::ids() << std::endl << eot ;
}

void Main::CommandLine::showThreading( bool e , const std::string & eot ) const
{
	Show show( m_output , e ) ;
	show.s() << "Multi-threading: " << (G::threading::works()?"enabled":"disabled") << std::endl << eot ;
}

void Main::CommandLine::showVersion( bool e ) const
{
	Show show( m_output , e ) ;
	showBanner( e , "\n" ) ;
	showCopyright( e , "\n" ) ;
	if( m_getopt.contains("verbose") )
	{
		showThreading( e , "\n" ) ;
		showSslVersion( e , "\n" ) ;
	}
	showSslCredit( e , "\n" ) ;
	showWarranty( e ) ;
}

void Main::CommandLine::showSemanticError( const std::string & error ) const
{
	Show show( m_output , true ) ;
	show.s() << m_arg.prefix() << ": usage error: " << error << std::endl ;
}

void Main::CommandLine::showSemanticWarnings( const G::StringArray & warnings ) const
{
	if( !warnings.empty() )
	{
		Show show( m_output , true ) ;
		show.s() << m_arg.prefix() << ": warning: " << G::Str::join("\n"+m_arg.prefix()+": warning: ",warnings) << std::endl ;
	}
}

void Main::CommandLine::logSemanticWarnings( const G::StringArray & warnings ) const
{
	for( G::StringArray::const_iterator p = warnings.begin() ; p != warnings.end() ; ++p )
		G_WARNING( "CommandLine::logSemanticWarnings: " << (*p) ) ;
}

// ===

Main::Show * Main::Show::m_this = nullptr ;

Main::Show::Show( Output & output , bool e ) :
	m_output(output) ,
	m_e(e)
{
	if( m_this == nullptr )
		m_this = this ;
}

std::ostream & Main::Show::s()
{
	return m_this->m_ss ;
}

G::Options::Layout Main::Show::layout() const
{
	return m_output.layout() ;
}

Main::Show::~Show()
{
	if( m_this == this )
	{
		m_this = nullptr ;
		m_output.output( m_ss.str() , m_e ) ;
	}
}

