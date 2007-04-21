//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// guimain.cpp
//

#include "gdef.h"
#include "qt.h"
#include "gunpack.h"
#include "gdialog.h"
#include "gfile.h"
#include "dir.h"
#include "pages.h"
#include "glogoutput.h"
#include "ggetopt.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include <string>
#include <iostream>
#include <stdexcept>

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
			"d/debug/show debug messages if compiled-in/0//1|"
			//"p/prefix/target directory prefix/1/path/0|"
			"P/page/single page test/1/page-name/0|"
			"f/file/write configuration to file/1/file/0|"
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
		std::string cfg_test_page = getopt.contains("page") ? getopt.value("page") : std::string() ;
		//std::string cfg_prefix = getopt.contains("prefix") ? getopt.value("prefix") : std::string() ;
		G::Path cfg_dump_file( getopt.contains("file") ? getopt.value("file") : std::string() ) ;

		try
		{
			bool is_setup = G::Unpack::isPacked(args.v(0)) ; // are we "setup" or "gui"?
			bool is_installed = !is_setup ;
			Dir dir( args.v(0) , is_installed ) ;
			if( is_installed )
			{
				// read base directories from the state file, typically written by "make install"
				std::ifstream dir_state( G::Path(G::Path(args.v(0)).dirname(),"emailrelay-gui.state").str().c_str() ) ;
				dir.read( dir_state ) ;
			}

			G_DEBUG( "Dir::install: " << dir.install() ) ;
			G_DEBUG( "Dir::spool: " << dir.spool() ) ;
			G_DEBUG( "Dir::config: " << dir.config() ) ;
			G_DEBUG( "Dir::startup: " << dir.startup() ) ;
			G_DEBUG( "Dir::pid: " << dir.pid() ) ;
			G_DEBUG( "Dir::cwd: " << dir.cwd() ) ;
			G_DEBUG( "Dir::thisdir: " << dir.thisdir() ) ;

			// default translator
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			// application translator
			QTranslator translator;
			translator.load(QString("emailrelay_install_")+QLocale::system().name());
			app.installTranslator(&translator);

			// initialise GPage
			if( ! cfg_test_page.empty() || test_mode ) 
				GPage::setTestMode() ;

			// create the dialog and all its pages
			GDialog d ;
			d.add( new TitlePage(d,"title","license","",false,false) , cfg_test_page ) ;
			d.add( new LicensePage(d,"license","directory","",false,false) , cfg_test_page ) ;
			d.add( new DirectoryPage(d,"directory","dowhat","",false,false,dir,is_setup) , cfg_test_page ) ;
			d.add( new DoWhatPage(d,"dowhat","pop","smtpserver",false,false) , cfg_test_page ) ;
			d.add( new PopPage(d,"pop","popaccount","popaccounts",false,false) , cfg_test_page ) ;
			d.add( new PopAccountPage(d,"popaccount","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new PopAccountsPage(d,"popaccounts","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new SmtpServerPage(d,"smtpserver","smtpclient","",false,false) , cfg_test_page ) ;
			d.add( new SmtpClientPage(d,"smtpclient","logging","",false,false) , cfg_test_page ) ;
			d.add( new LoggingPage(d,"logging","listening","",false,false) , cfg_test_page ) ;
			d.add( new ListeningPage(d,"listening","startup","",false,false) , cfg_test_page ) ;
			d.add( new StartupPage(d,"startup","ready","",false,false) , cfg_test_page ) ;
			d.add( new ReadyPage(d,"ready","progress","",true,false,is_setup) , cfg_test_page ) ;
			d.add( new ProgressPage(d,"progress","","",true,true,args.v(0),cfg_dump_file) , cfg_test_page ) ;
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
			std::cerr << "exception: " << e.what() << std::endl ;
			error( e.what() ) ;
			std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
			qCritical( "exception: %s" , message.c_str() ) ;
		}
		catch(...)
		{
			std::cerr << "unknown exception" << std::endl ;
			error( "unknown exception" ) ;
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

/// \file guimain.cpp
