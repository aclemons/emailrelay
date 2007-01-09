//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// main.cpp
//

#include "gdef.h"
#include "qt.h"
#include "gdialog.h"
#include "gfile.h"
#include "dir.h"
#include "pages.h"
#include "glogoutput.h"
#include "ggetopt.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include "unpack.h"
#include <string>
#include <iostream>
#include <stdexcept>

static G::Path prepare_tool( const std::string & ) ;
static void unpack( const std::string & from_exe , const std::string & to_dir , const std::string & name ) ;

static int width() 
{ 
	return 500 ; 
}

static int height() 
{ 
	return 500 ; 
}

static void error( const std::string & what )
{
	QString title(QMessageBox::tr("E-MailRelay")) ;
	QMessageBox::critical( NULL , title , 
		QMessageBox::tr("Failed with the following exception: %1").arg(what.c_str()) ,
		QMessageBox::Abort , QMessageBox::NoButton , QMessageBox::NoButton ) ;
}

int main( int argc , char * argv [] )
{
	try
	{
		QApplication app( argc , argv ) ;
		G::Arg args( argc , argv ) ;
		G::GetOpt getopt( args , 
			"h/help/show this help text and exit/0//1|"
			"c/configure/do configuration steps only/0//1|"
			"x/tool/text-mode install tool/1/path/1|"
			"X/tool-arg/text-mode install tool argument/1/arg/1|"
			"d/debug/show debug messages if compiled-in/0//1|"
			"p/prefix/target directory prefix/1/path/0|"
			"P/page/single page test/1/page-name/0|"
			"t/test/test-mode/0//0" ) ;
		if( getopt.hasErrors() )
		{
			getopt.showErrors( std::cerr ) ;
			return 2 ;
		}
		if( getopt.contains("help") )
		{
			getopt.showUsage( std::cout , " [<qt4-switches>]" , false ) ;
			return 0 ;
		}
		G::LogOutput log_ouptut( getopt.contains("debug") ) ;

		// parse the commandline
		bool test_mode = getopt.contains("test") ;
		bool install = ! getopt.contains("configure") ;
		std::string cfg_test_page = getopt.contains("page") ? getopt.value("page") : std::string() ;
		std::string cfg_tool = getopt.contains("tool") ? getopt.value("tool") : std::string() ;
		std::string cfg_tool_arg = getopt.contains("tool-arg") ? getopt.value("tool-arg") : std::string() ;
		std::string cfg_prefix = getopt.contains("prefix") ? getopt.value("prefix") : std::string() ;

		try
		{
			Dir dir( args.v(0) , cfg_prefix ) ;
			G_DEBUG( "Dir::install: " << dir.install() ) ;
			G_DEBUG( "Dir::spool: " << dir.spool() ) ;
			G_DEBUG( "Dir::config: " << dir.config() ) ;
			G_DEBUG( "Dir::startup: " << dir.startup() ) ;
			G_DEBUG( "Dir::pid: " << dir.pid() ) ;
			G_DEBUG( "Dir::cwd: " << dir.cwd() ) ;
			G_DEBUG( "Dir::tooldir: " << dir.tooldir() ) ;
			G_DEBUG( "Dir::thisdir: " << dir.thisdir() ) ;

			// default translator
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			// application translator
			QTranslator translator;
			translator.load(QString("emailrelay_install_")+QLocale::system().name());
			app.installTranslator(&translator);

			// prepare the tool and store its path in GPage 
			G::Path tool = prepare_tool( cfg_tool ) ;
			GPage::setTool( tool.str() , cfg_tool_arg ) ;

			// initialise GPage
			if( ! cfg_test_page.empty() || test_mode ) 
				GPage::setTestMode() ;

			// create the dialog and all its pages
			GDialog d ;
			if( install )
			{
				d.add( new TitlePage(d,"title","license","",false,false) , cfg_test_page ) ;
				d.add( new LicensePage(d,"license","directory","",false,false) , cfg_test_page ) ;
				d.add( new DirectoryPage(d,"directory","dowhat","",false,false) , cfg_test_page ) ;
			}
			else
			{
				d.add( new DirectoryPage(d,"directory","dowhat","",false,false) , cfg_test_page ) ;
			}
			d.add( new DoWhatPage(d,"dowhat","pop","smtpserver",false,false) , cfg_test_page ) ;
			d.add( new PopPage(d,"pop","popaccount","popaccounts",false,false) , cfg_test_page ) ;
			d.add( new PopAccountPage(d,"popaccount","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new PopAccountsPage(d,"popaccounts","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new SmtpServerPage(d,"smtpserver","smtpclient","",false,false) , cfg_test_page ) ;
			d.add( new SmtpClientPage(d,"smtpclient","logging","",false,false) , cfg_test_page ) ;
			d.add( new LoggingPage(d,"logging","listening","",false,false) , cfg_test_page ) ;
			if( install )
			{
				d.add( new ListeningPage(d,"listening","startup","",false,false) , cfg_test_page ) ;
				d.add( new StartupPage(d,"startup","configuration","",false,false) , cfg_test_page ) ;
				d.add( new ConfigurationPage(d,"configuration","progress","",true,false) , cfg_test_page ) ;
				d.add( new ProgressPage(d,"progress","","",false,true) , cfg_test_page ) ;
			}
			else
			{
				d.add( new ListeningPage(d,"listening","end","",false,false) , cfg_test_page ) ;
				d.add( new EndPage_(d,"end") , cfg_test_page ) ;
			}
			d.add() ;

			// check the test-page value
			if( d.empty() )
				throw std::runtime_error(std::string()+"invalid page name: \""+cfg_test_page+"\"") ;

			// set the dialog dimensions
			QSize s = d.size() ;
			if( s.width() < width() ) s.setWidth(width()) ;
			if( s.height() < height() ) s.setHeight(height()) ;
			d.resize( s ) ;

			// run the dialog
			d.exec() ;
			return 0 ;
		}
		catch( std::exception & e )
		{
			error( e.what() ) ;
			std::cerr << "exception: " << e.what() << std::endl ;
			std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
			qCritical( "exception: %s" , message.c_str() ) ;
		}
		catch(...)
		{
			error( "unknown exception" ) ;
			std::cerr << "unknown exception" << std::endl ;
			qCritical( "%s" , "unknown exception" ) ;
		}
		return 1 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
		std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
		qCritical( "exception: %s" , message.c_str() ) ;
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl ;
		qCritical( "%s" , "unknown exception" ) ;
	}
	return 1 ;
}

static G::Path prepare_tool( const std::string & cfg_tool )
{
	// if the path is specified on the command-line just
	// make sure it exists
	//
	if( ! cfg_tool.empty() )
	{
		if( ! G::File::exists(G::Path(cfg_tool)) )
			throw std::runtime_error(std::string()+"invalid install tool: \""+cfg_tool+"\"") ;
		return G::Path(cfg_tool) ;
	}

	// if not specified then look in this exe's directory
	//
	std::string tool_name = std::string() + "emailrelay-install-tool" + Dir::dotexe() ;
	G::Path tool_path( Dir::thisdir() , tool_name ) ;
	if( G::File::exists(G::Path(tool_path)) )
		return G::Path(tool_path) ;

	// if it's not there then try unpacking it from this exe
	//
	G::Path unpack_dir = Dir::tmp() ;
	G::Path unpack_path = G::Path(unpack_dir,tool_name) ;
	G_DEBUG( "extracting " << tool_name << " from " << Dir::thisexe().basename() << " to \"" << unpack_path << "\"" );
	unpack( Dir::thisexe().str() , Dir::tmp().str() , tool_name ) ;
	G::File::chmodx( unpack_path ) ;
	return unpack_path ;
}

static void unpack( const std::string & from_exe , const std::string & to_dir , const std::string & name )
{
	Unpack * unpack = unpack_new(from_exe.c_str(),0) ;
	bool ok = !! unpack_file( unpack , to_dir.c_str() , name.c_str() ) ;
	unpack_delete( unpack ) ;
	if( !ok )
		throw std::runtime_error( std::string() + "failed to unpack " + name + " from " + from_exe + " into " + to_dir);
}

