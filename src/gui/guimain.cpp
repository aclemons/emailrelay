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
// This GUI program is primarily intended to help with configuration
// of an initial installation ("--as-install"), but it can also be 
// used to reconfigure an existing installation ("--as-configure").
//
// The program determines whether to run in install mode or configure
// mode by looking for packed files appended to the end of the
// executable, although the "--as-(whatever)" command-line switches 
// can be used as an override for this test. In install mode the
// target directory paths can be set from within the GUI and the 
// packed files are extracted into those directories. 
//
// If there are no packed files then the assumption is that we
// are being run after a successful installation, so the target 
// directory paths are greyed out in the GUI. However, the code
// still needs to know what the installation directories were and
// it tries to obtain these from a special "state" file located
// in the same directory as the executable.
//
// The state file contains the base installation directories as a 
// minimum, but it also typically holds the complete state of the
// GUI including the state of the widgets.
//
// The state file is read at startup and written as part of the final 
// stage of the GUI workflow. If running in install mode the state
// file is optional; if running in configuration mode the state
// file is mandatory, but it only needs to contain two directory
// paths (see Dir::read() and "make install").
//
// In a unix-like installation the build and install steps are normally
// done from the command-line using "make" and "make install". In this
// case the GUI is only expected to do post-installation configuration 
// so there are no packed files appended to the executable and the 
// initial state file is created by "make install". (It is possible 
// to do a windows-like, self-extracting installation on unix-like 
// operating systems, but not very likely in practice.)
//
// The name of the state file is the name of the GUI executable
// without any extension; or if the GUI executable had no extension
// to begin with then it is the name of the GUI executable with
// ".state" appended (but refer to the next paragraph to see
// why this is not used in practice).
//
// The format of the state file (since v1.8) allows it to be a 
// simple shell script. This means that a unix-like "make install" 
// can install the GUI executable as "emailrelay-gui.real" and the
// state file called "emailrelay-gui" can double-up as a wrapper shell
// script. Making the state file also a shell script is so that
// the two files in the relevant bin directory are both executable, 
// in line with the FHS and user expectations.
//
// The implementation of the GUI uses a set of dialog-box "pages"
// with forward and back buttons. Each page writes its state as
// "key=value" pairs into a configuration text stream. After the 
// last page has been filled in the resulting configuration text 
// is passed to the Installer class. This class interprets the 
// configuration and assembles a set of installation actions 
// (in the Command pattern) which are then executed to effect the
// installation.
//
// Note that the Installer class only does what the configuration 
// text tells it to do; it does not have any intelligence of its own. 
// In principle this allows for a complete separation of the GUI 
// and the installation process, with a simple text file interface 
// between them. In practice the text file would have to contain 
// plaintext passwords, so this is not a secure approach.
//
// The contents of the state file are also based on the output of
// configuration text from the GUI pages, but it also has additional
// information from the Dir class and it does not contain any 
// account information.
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
	// (a compile-time test is problably better than run-time here)
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
			"H/with-help/show a help button/0//1|"
			"d/debug/show debug messages if compiled-in/0//1|"
			"i/as-install/install mode, as if payload present/0//1|"
			"c/as-configure/configure mode, as if no payload present/0//1|"
			"w/write/state file for writing/1/file/1|"
			"r/read/state file for reading/1/file/1|"
			// hidden...
			"P/page/single page test/1/page-name/0|"
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
		bool with_help = getopt.contains("with-help") ;
		std::string cfg_test_page = getopt.contains("page") ? getopt.value("page") : std::string() ;
		G::Path cfg_write_file( getopt.contains("write") ? getopt.value("write") : std::string() ) ;
		G::Path cfg_read_file( getopt.contains("read") ? getopt.value("read") : std::string() ) ;
		bool cfg_as_mac = getopt.contains("mac") ;
		bool cfg_install = getopt.contains("as-install") ;
		bool cfg_configure = getopt.contains("as-configure") ;
		G::Path state_path_in = cfg_read_file == G::Path() ? State::file(args.v(0)) : cfg_read_file ;
		G::Path state_path_out = cfg_write_file == G::Path() ? State::file(args.v(0)) : cfg_write_file ;

		try
		{
			// find the payload -- normally packed into the running executable
			G::Path payload_1 = args.v(0) ;
			G::Path payload_2 = G::Path( G::Path(args.v(0)).dirname() , "payload" ) ;
			G::Path payload_3 = G::Path( G::Path(args.v(0)).dirname() , ".." , "payload" ) ;
			G::Path payload = 
				G::Unpack::isPacked(payload_1) ? payload_1 : (
				G::Unpack::isPacked(payload_2) ? payload_2 : (
				payload_3 ) ) ;
			G_DEBUG( "main: packed files " << (G::Unpack::isPacked(payload)?"":"not ") 
				<< "found (" << payload << ")" ) ;

			// are we install-mode or configure-mode?
			bool is_installing = ( cfg_install || G::Unpack::isPacked(payload) ) && !cfg_configure ;
			bool is_installed = !is_installing ;

			// read the state file
			State::Map state_map ;
			{
				G_DEBUG( "main: state file: " << state_path_in ) ;
				std::ifstream state_stream( state_path_in.str().c_str() ) ;
				if( !state_stream.good() && is_installed )
					throw std::runtime_error(std::string()+"cannot open state file: \""+state_path_in.str()+"\"") ;
				state_map = State::read( state_stream ) ;
				if( !state_stream.eof() && is_installed )
					throw std::runtime_error(std::string()+"cannot read state file: \""+state_path_in.str()+"\"") ;
			}
			State state( state_map ) ;

			// initialise the directory info object
			Dir dir( args.v(0) ) ;
			dir.read( state ) ;

			G_DEBUG( "main: Dir::install: " << Dir::install() ) ;
			G_DEBUG( "main: Dir::spool: " << dir.spool() ) ;
			G_DEBUG( "main: Dir::config: " << dir.config() ) ;
			G_DEBUG( "main: Dir::boot: " << dir.boot() ) ;
			G_DEBUG( "main: Dir::pid: " << dir.pid(dir.config()) ) ;
			G_DEBUG( "main: Dir::cwd: " << dir.cwd() ) ;
			G_DEBUG( "main: Dir::thisdir: " << dir.thisdir() ) ;

			// default translator
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			// application translator
			QTranslator translator;
			translator.load(QString("emailrelay_")+QLocale::system().name());
			app.installTranslator(&translator);

			// initialise GPage
			if( ! cfg_test_page.empty() || test_mode ) 
				GPage::setTestMode() ;

			// create the dialog and all its pages
			GDialog d( with_help ) ;
			d.add( new TitlePage(d,state,"title","license","",false,false) , cfg_test_page ) ;
			d.add( new LicensePage(d,state,"license","directory","",false,false,is_installed) , cfg_test_page ) ;
			d.add( new DirectoryPage(d,state,"directory","dowhat","",false,false,dir,is_installing) , cfg_test_page ) ;
			d.add( new DoWhatPage(d,state,"dowhat","pop","smtpserver",false,false) , cfg_test_page ) ;
			d.add( new PopPage(d,state,"pop","popaccount","popaccounts",false,false) , cfg_test_page ) ;
			d.add( new PopAccountPage(d,state,"popaccount","smtpserver","listening",false,false,is_installed) , 
				cfg_test_page ) ;
			d.add( new PopAccountsPage(d,state,"popaccounts","smtpserver","listening",false,false,is_installed) , 
				cfg_test_page ) ;
			d.add( new SmtpServerPage(d,state,"smtpserver","smtpclient","",false,false,is_installed) , cfg_test_page ) ;
			d.add( new SmtpClientPage(d,state,"smtpclient","logging","",false,false,is_installed) , cfg_test_page ) ;
			d.add( new LoggingPage(d,state,"logging","listening","",false,false) , cfg_test_page ) ;
			d.add( new ListeningPage(d,state,"listening","startup","",false,false) , cfg_test_page ) ;
			d.add( new StartupPage(d,state,"startup","ready","",false,false,dir,isMac()||cfg_as_mac) , cfg_test_page ) ;
			d.add( new ReadyPage(d,state,"ready","progress","",true,false,is_installing) , cfg_test_page ) ;
			d.add( new ProgressPage(d,state,"progress","","",true,true,args.v(0),payload,state_path_out,is_installing) ,
				cfg_test_page ) ;
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
