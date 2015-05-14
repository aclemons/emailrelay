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
// guimain.cpp
//
// Modes
// -----
// This GUI program is primarily intended to do a new installation, but it 
// can also be used to reconfigure an existing installation.
//
// As far as possible the program should run without command-line arguments;
// it determines whether to run in install mode or configure mode by looking 
// around for a payload file. 
// 
// Pointer variables
// -----------------
// In install mode the user supplies the main directory paths via the GUI. 
// In configure mode the main directory paths (in particular the config-file
// directory) come from a 'pointer' file. The name of the pointer file is the 
// name of the GUI executable without any extension; or if the GUI executable 
// had no extension to begin with then it is the name of the GUI executable with 
// ".cfg" appended. The pointer file is cunningly formatted as an executable 
// script that runs the real GUI program.
//
// Widget state variables
// ----------------------
// Widget states are _not_ currently preserved across separate runs of the 
// program, however many widget states are derived from the pointer file 
// and/or the server configuration file.
//
// Install variables
// -----------------
// The Installer class operates according to a set of install variables that
// are dumped into a stringstream by the GUI pages. This allows the installer 
// to be tested independently of the GUI.
//
// Implementation
// --------------
// The implementation of the GUI uses a set of dialog-box "pages" with forward
// and back buttons. Each page writes its state as install variables into a
// text stream. After the last page has been filled in the resulting install
// text is passed to the Installer class. This class interprets assembles a set
// of installation actions (in the Command pattern) which are then executed to
// effect the installation.
//

#include "gdef.h"
#include "qt.h"
#include "gdialog.h"
#include "gfile.h"
#include "gbatchfile.h"
#include "gmapfile.h"
#include "gdirectory.h"
#include "goptionparser.h"
#include "dir.h"
#include "pages.h"
#include "boot.h"
#include "serverconfiguration.h"
#include "glogoutput.h"
#include "ggetopt.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <fstream>

#ifdef G_WINDOWS
#if defined(QT_VERSION) && QT_VERSION >= 0x050000
#ifdef G_QT_STATIC
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
#endif
#endif

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

static void info( const std::string & text )
{
	QString title(QMessageBox::tr("E-MailRelay")) ;
	QMessageBox::information( NULL , title , QString::fromLatin1(text.c_str()) ,
		QMessageBox::Ok , QMessageBox::NoButton , QMessageBox::NoButton ) ;
}

static bool isMac()
{
 #if defined(G_MAC) || defined(G_AS_IF_MAC)
	return true ;
 #else
	return false ;
 #endif
}

static bool isWindows()
{
 #if defined(G_WINDOWS) || defined(G_AS_IF_WINDOWS)
	return true ;
 #else
	return false ;
 #endif
}

class Application : public QApplication
{
public:
	Application( int & argc , char * argv [] ) ;
	virtual bool notify( QObject * p1 , QEvent * p2 ) ;
} ;
Application::Application( int & argc , char * argv [] ) :
	QApplication(argc,argv)
{
}
bool Application::notify( QObject * p1 , QEvent * p2 )
{
	try
	{
		return QApplication::notify( p1 , p2 ) ;
	}
	catch( std::exception & e )
	{
		G_ERROR( "exception: " << e.what() ) ;
		error( e.what() ) ;
		std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
		qCritical( "exception: %s" , message.c_str() ) ;
		exit( 3 ) ;
	}
	return false ;
}

static G::Path search( G::Path argv0 , std::string p1 , std::string p2 , std::string p3 )
{
	if( !p1.empty() && G::File::exists( argv0.dirname() + p1 ) ) return argv0.dirname() + p1 ;
	if( !p2.empty() && G::File::exists( argv0.dirname() + p2 ) ) return argv0.dirname() + p2 ;
	if( !p3.empty() && G::File::exists( argv0.dirname() + p3 ) ) return argv0.dirname() + p3 ;
	return G::Path() ;
}

static G::Path pointerFile( G::Path argv0 )
{
	std::string ext = argv0.basename().find('.') == std::string::npos || isWindows() ? ".cfg" : "" ;
	argv0.removeExtension() ;
	std::string name = argv0.basename() ;
	name.append( ext ) ;
	return isMac() ?
		argv0.dirname() + ".." + "Resources" + name :
		argv0.dirname() + name ;
}

static G::Path configFile( const G::Path & dir_config )
{
	return isWindows() ?
		(dir_config+"emailrelay-start.bat") : 
		(dir_config+"emailrelay.conf") ;
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg args( argc , argv ) ;
		Application app( argc , argv ) ;
		if( argc > 1 && std::string(argv[1]) == "--message" ) // message-box helper esp. for mac
		{
			std::ostringstream ss ;
			const char * sep = "" ;
			for( int i = 2 ; i < argc ; i++ , sep = " " )
				ss << sep << argv[i] ;
			info( ss.str() ) ;
			return 1 ;
		}
		G::LogOutput log_ouptut( 
			"emailrelay-gui" ,
			true , // output
			true , // with-logging
			args.contains("-v") , // with-verbose-logging
			args.contains("-v") , // with-debug
			true , // with-level
			false , // with-timestamp
			false , // strip-context
			args.contains("-v") // use-syslog
		) ;
		G_LOG( "main: start: " << argv[0] ) ;

		try
		{
			// look for the payload
			G::Path payload_path = search( args.v(0U) , "payload" , "../payload" , "../Resources/payload" ) ;
			bool configure_mode = payload_path == G::Path() ;
			if( configure_mode )
			{
				G_LOG_S( "no payload found so running in configure mode" ) ;
			}
			else
			{
				G_LOG_S( "found payload [" << payload_path << "] so running in install mode" ) ;
			}

			// load the pointer file
			G::Path pointer_file = pointerFile( args.v(0U) ) ;
			G::MapFile pointer_map ;
			if( configure_mode )
			{
				G_LOG_S( "reading directories from [" << pointer_file << "]" ) ;
				if( G::File::exists(pointer_file) )
					pointer_map = G::MapFile( pointer_file ) ;
			}

			// load the existing server configuration
			G::Path dir_config = pointer_map.pathValue( "dir-config" , Dir::config() ) ;
			G::Path dir_run = pointer_map.pathValue( "dir-run" , Dir::pid(dir_config) ) ;
			G::Path server_config_file = configFile( dir_config ) ;
			G::MapFile server_config_map ;
			G::Path server_config_exe ;
			if( configure_mode )
			{
				G_LOG_S( "editing server configuration in [" << server_config_file << "]" ) ;
				server_config_map = ServerConfiguration(server_config_file).map() ;
				server_config_exe = ServerConfiguration::exe( server_config_file ) ; // windows
				if( server_config_map.contains("pid-file") )
					dir_run = server_config_map.pathValue("pid-file").dirname() ;
			}

			// determine the install root
			G::Path dir_install = Dir::install() ;
			if( configure_mode )
			{
				// relative to the server or argv0 exe, but with a pointer-file override
				dir_install = server_config_exe.dirname() != G::Path() ? 
					server_config_exe.dirname() : Dir::absolute(G::Path(args.v(0U)).dirname()+"..") ;
				dir_install = pointer_map.pathValue( "dir-install" , dir_install ) ;
			}

			// set up the gui pages' config map
			G::MapFile pages_config = server_config_map ;
			if( !pages_config.contains("spool-dir") ) pages_config.add( "spool-dir" , Dir::spool().str() ) ;
			pages_config.add( "=dir-config" , dir_config.str() ) ;
			pages_config.add( "=dir-install" , dir_install.str() ) ;
			pages_config.add( "=dir-run" , dir_run.str() ) ;

			// check the config file (or windows batch file) will be writeable
			if( configure_mode ) 
			{
				if( G::File::exists(server_config_file) )
				{
					std::string more_help = isWindows() ? " or run as administrator" : "" ;
					std::ofstream f( server_config_file.str().c_str() , std::ios_base::ate ) ;
					if( !f.good() )
						throw std::runtime_error( "cannot write [" + server_config_file.str() + "]" 
							": check file permissions" + more_help ) ;
				}
				else if( !G::Directory(server_config_file.dirname()).valid(true) )
				{
					std::string gerund = pointer_map.contains("dir-config") ? "changing the" : "adding a" ;
					std::string preposition = pointer_map.contains("dir-config") ? "in" : "to" ;
					throw std::runtime_error( "cannot create files in [" + server_config_file.dirname().str() + "]: "
						"try " + gerund + " \"dir-config\" entry " + preposition + " the configuration file "
						"[" + pointer_file.str() + "]" ) ;
				}
			}

			// default translator
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			// application translator
			QTranslator translator;
			translator.load(QString("emailrelay_")+QLocale::system().name());
			app.installTranslator(&translator);

			// create the installer - see ProgressPage - pass minimal configuration
			// directly to the installer since it takes most of its configuration 
			// from the output of the gui pages
			Installer installer( !configure_mode , isWindows() , payload_path ) ;

			// create the dialog and all its pages
			GDialog d( false ) ;
			d.add( new TitlePage(d,pages_config,"title","license","",false,false) ) ;
			d.add( new LicensePage(d,pages_config,"license","directory","",false,false,configure_mode) ) ;
			d.add( new DirectoryPage(d,pages_config,"directory","dowhat","",false,false,!configure_mode) ) ;
			d.add( new DoWhatPage(d,pages_config,"dowhat","pop","smtpserver",false,false) ) ;
			d.add( new PopPage(d,pages_config,"pop","popaccount","popaccounts",false,false) ) ;
			d.add( new PopAccountPage(d,pages_config,"popaccount","smtpserver","listening",false,false,configure_mode));
			d.add( new PopAccountsPage(d,pages_config,"popaccounts","smtpserver","listening",false,!1,configure_mode));
			d.add( new SmtpServerPage(d,pages_config,"smtpserver","smtpclient","",false,false,configure_mode) ) ;
			d.add( new SmtpClientPage(d,pages_config,"smtpclient","logging","",false,false,configure_mode) ) ;
			d.add( new LoggingPage(d,pages_config,"logging","listening","",false,false) ) ;
			d.add( new ListeningPage(d,pages_config,"listening","startup","",false,false) ) ;
			d.add( new StartupPage(d,pages_config,"startup","ready","",false,false,Boot::able(Dir::boot()),isMac()) ) ;
			d.add( new ReadyPage(d,pages_config,"ready","progress","",true,false,!configure_mode) ) ;
			d.add( new ProgressPage(d,pages_config,"progress","","",true,true,installer) ) ;
			d.add() ;

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
			std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
			qCritical( "exception: %s" , message.c_str() ) ;
		}
		catch(...)
		{
			error( "unknown exception" ) ;
			qCritical( "%s" , "unknown exception" ) ;
		}
		return 1 ;
	}
	catch( std::exception & e )
	{
		std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
		qCritical( "exception: %s" , message.c_str() ) ;
	}
	catch(...)
	{
		qCritical( "%s" , "unknown exception" ) ;
	}
	return 1 ;
}

