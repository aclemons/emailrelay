//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// guimain.cpp
//
// This GUI program is primarily intended to help with initial 
// installation and configuration, but it can also be used 
// to reconfigure an existing installation (with certain 
// limitations).
//
// The program determines whether it is running as a self-extracting
// installer by looking for packed files appended to the end of 
// the executable. If there are packed files then the target 
// directory paths are obtained from the GUI and the packed files 
// are extracted into those directories. 
//
// If there are no packed files then the assumption is that it
// is being run after a successful installation, so the target 
// directory paths are greyed out in the GUI. However, it still 
// needs to know what the installation directories were and it
// tries to obtain these from a special "state" file located
// in the same directory as the executable.
//
// In a unix-like installation the build and install steps are
// done from the command-line using "make" and "make install". 
// In this case the GUI is only expected to do post-installation 
// configuration so there are no packed files appended to the
// executable and the state file is created by "make install".
//
// In a windows-like installation the state file is created
// by the GUI at the same time as the packed files are extracted
// into the target directories.
//
// The name of the state file is the name of the GUI executable
// without any extension; or if the GUI executable had no extension
// to begin with then it is the name of the GUI executable with
// ".state" appended.
//
// The format of the state file (since v1.8) allows it to be a 
// simple shell script. This means that a unix-like "make install" 
// can install the GUI executable as "emailrelay-gui.real" and the
// state file "emailrelay-gui" can double-up as a wrapper
// shell script.
//
// Note that it is possible to do a windows-like, self-extracting
// installation on unix-like operating systems.
//
// The implementation of the GUI uses a set of dialog-box "pages"
// with forward and back buttons. Each page writes its state as
// "key:value" pairs into an text stream. After the last page has 
// been filled in the resulting configuration text is passed to 
// the Installer class. This class interprets the configuration
// and assembles a set of installation actions which are then
// executed to effect the installation. (For debugging purposes
// the "key:value" pairs can be wriiten to file using "--file".)
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

static bool isMac()
{
	// (a compile-time test is problably better than run-time)
 #ifdef G_MAC
	return true ;
 #else
	return false ;
 #endif
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
			"i/as-install/install mode, as if payload present/0//1|"
			"c/as-configure/configure mode, as if no payload present/0//1|"
			// hidden...
			"P/page/single page test/1/page-name/0|"
			"f/file/write configuration to file/1/file/0|"
			"m/mac/enable some mac-like runtime behaviour/0//0|"
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
		G::Path cfg_dump_file( getopt.contains("file") ? getopt.value("file") : std::string() ) ;
		bool cfg_as_mac = getopt.contains("mac") ;
		bool cfg_install = getopt.contains("as-install") ;
		bool cfg_configure = getopt.contains("as-configure") ;

		try
		{
			// are we "setup" or "gui", ie. install-mode or configure-mode?
			bool is_setup = ( cfg_install || G::Unpack::isPacked(args.v(0)) ) && !cfg_configure ;
			bool is_installed = !is_setup ;
			G_DEBUG( "main: packed files " << (is_setup?"":"not ") << "found" ) ;

			Dir dir( args.v(0) , is_installed ) ;
			if( is_installed )
			{
				// read base directories from the state file, typically written by "make install"
				G::Path argv0( args.v(0) ) ;
				std::string state_extension = argv0.basename().find('.') == std::string::npos ? ".state" : "" ;
				argv0.removeExtension() ;
				std::string state_name = argv0.basename() ;
				state_name.append( state_extension ) ;
				G::Path state_path = G::Path( G::Path(args.v(0)).dirname() , state_name ) ;
				G_DEBUG( "main: state file: " << state_path ) ;
				std::ifstream dir_state( state_path.str().c_str() ) ;
				dir.read( dir_state ) ;
			}

			G_DEBUG( "main: Dir::install: " << dir.install() ) ;
			G_DEBUG( "main: Dir::spool: " << dir.spool() ) ;
			G_DEBUG( "main: Dir::config: " << dir.config() ) ;
			G_DEBUG( "main: Dir::boot: " << dir.boot() ) ;
			G_DEBUG( "main: Dir::pid: " << dir.pid() ) ;
			G_DEBUG( "main: Dir::cwd: " << dir.cwd() ) ;
			G_DEBUG( "main: Dir::thisdir: " << dir.thisdir() ) ;

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
			d.add( new LicensePage(d,"license","directory","",false,false,is_installed) , cfg_test_page ) ;
			d.add( new DirectoryPage(d,"directory","dowhat","",false,false,dir,is_setup) , cfg_test_page ) ;
			d.add( new DoWhatPage(d,"dowhat","pop","smtpserver",false,false) , cfg_test_page ) ;
			d.add( new PopPage(d,"pop","popaccount","popaccounts",false,false) , cfg_test_page ) ;
			d.add( new PopAccountPage(d,"popaccount","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new PopAccountsPage(d,"popaccounts","smtpserver","listening",false,false) , cfg_test_page ) ;
			d.add( new SmtpServerPage(d,"smtpserver","smtpclient","",false,false) , cfg_test_page ) ;
			d.add( new SmtpClientPage(d,"smtpclient","logging","",false,false) , cfg_test_page ) ;
			d.add( new LoggingPage(d,"logging","listening","",false,false) , cfg_test_page ) ;
			d.add( new ListeningPage(d,"listening","startup","",false,false) , cfg_test_page ) ;
			d.add( new StartupPage(d,"startup","ready","",false,false,dir,isMac()||cfg_as_mac) , cfg_test_page ) ;
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
