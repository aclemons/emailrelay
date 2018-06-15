//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// around for a payload.
//
// Payload
// -------
// Originally the payload was a single packed file, but it is now a directory
// tree called "payload", containing a "payload.cfg" configuration file that
// describes how and where the other payload files are to be installed.
//
// Pointer variables
// -----------------
// In install mode the user supplies the main directory paths via the GUI.
// In configure mode the main directory paths (in particular the config-file
// directory) come from a 'pointer' file.
//
// If there is no payload and no pointer file then all bets are off.
//
// The name of the pointer file is the name of the GUI executable without any
// extension; or if the GUI executable had no extension to begin with then it
// is the name of the GUI executable with ".cfg" appended; or on a mac it is
// just "dir.cfg" somewhere inside the bundle.
//
// The main pointer variables are "dir-config", "dir-install" and "dir-run".
// On unix these might be "/etc", "/usr/bin", and "/var/run", respectively.
//
// The pointer file on unix is cunningly formatted as an executable script that
// runs the real GUI program.
//
// Page variables
// --------------
// The GUI implementation uses a set of dialog-box "pages" with forward and back
// buttons.
//
// Each page initialises its widgets from a set of page variables ("pvalues").
// In configure mode the initial set of pvalues are read in from the server
// configuration file by the ServerConfiguration class, plus a copy of the
// pointer variables.
//
// Once the page interactions are complete each page writes its pvalues out into
// a text stream that is passed to a separate installer class.
//
// The installer class assembles a set of installation actions (in the Command
// pattern) which are then executed to effect the installation.
//
// Note that widget states are not explicitly preserved across separate runs of
// the gui program, but these mechanisms have largely the same effect.
//
// Install variables
// -----------------
// The installer class operates according to the set of pvalues dumped out by
// the individual GUI pages, and its own "installer" variables (ivalues). A
// handful of substitution variables allow the pvalues to be defined in terms
// of ivalues.
//
// The core file-copying stage of the install is driven from the payload
// configuration file, "payload/payload.cfg" (see "bin/make-setup.sh"). Each
// line specifies a file-or-directory-tree copy, or a chown/chmod. For each file
// the source path is relative to the payload root, and the destination
// typically starts with a base-directory substitution variable (eg.
// "%dir-install%").
//
// End-result
// ----------
// After installation the install directory will contain the gui executable, the
// server executable, and the pointer file (which on unix is a shell script that
// re-runs the gui).
//
// The configuration directory will contain the server configuration file (which
// is a startup batch file on windows), and the authentication secrets file.
//
// On unix the server configuration file is in a format that can be read by the
// System V init script to assemble the server's full command-line.
//
// The install directory, and to a lesser extent the configuration directory,
// will contain additional files copied straight from the payload, such as
// shared libraries, documentation, and utilities.
//

#include "gdef.h"
#include "qt.h"
#include "gdialog.h"
#include "gfile.h"
#include "gbatchfile.h"
#include "gmapfile.h"
#include "gdirectory.h"
#include "goptionparser.h"
#include "installer.h"
#include "dir.h"
#include "glink.h"
#include "pages.h"
#include "boot.h"
#include "serverconfiguration.h"
#include "glogoutput.h"
#include "gassert.h"
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
	// this should be big enough that the buttons dont move
	// when going from one page to another, but not so big
	// that there is wasted space at the bottom of every
	// page, and not so big that the dialog box does not fit
	// on the screen (which may be 640x480)
	return 490 ;
}

static void error( const std::string & what )
{
	QString title(QMessageBox::tr("E-MailRelay")) ;
	QMessageBox::critical( nullptr , title ,
		QMessageBox::tr("Failed with the following exception: %1").arg(what.c_str()) ,
		QMessageBox::Abort , QMessageBox::NoButton , QMessageBox::NoButton ) ;
}

static void info( const std::string & text )
{
	QString title(QMessageBox::tr("E-MailRelay")) ;
	QMessageBox::information( nullptr , title , QString::fromLatin1(text.c_str()) ,
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

static G::Path search( G::Path base , std::string filename , std::string d1 , std::string d2 = std::string() , std::string d3 = std::string() )
{
	if( !d1.empty() && G::File::exists( base + d1 + filename ) ) return base + d1 + filename ;
	if( !d2.empty() && G::File::exists( base + d2 + filename ) ) return base + d2 + filename ;
	if( !d3.empty() && G::File::exists( base + d3 + filename ) ) return base + d3 + filename ;
	return G::Path() ;
}

static std::string pointerFilename( const G::Path & argv0 )
{
	std::string ext = argv0.basename().find('.') == std::string::npos || isWindows() ? ".cfg" : "" ;
	std::string filename = argv0.withoutExtension().basename() ;
	filename.append( ext ) ;
	return filename ;
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
			G::Path argv0 = G::Arg::exe().empty() ? args.v(0U) : G::Arg::exe() ;
			bool is_mac = isMac() || args.contains("--as-mac") ;

			// test-mode -- create a minimal payload
			if( args.contains("--test") )
			{
				G::Path pdir = argv0.dirname() + "payload" ;
				G::File::mkdir( pdir , G::File::NoThrow() ) ;
				G::File::copy( argv0 , pdir + argv0.basename() , G::File::NoThrow() ) ;
				std::ofstream f( (pdir+"payload.cfg").str().c_str() ) ;
				std::string sdir = isWindows() ? "" : "sbin/" ; // see "-gui" in installer
				std::string exe = isWindows() ? "emailrelay-gui.exe" : "emailrelay-gui.real" ;
				f << argv0.basename() << "= %dir-install%/" << sdir << exe << " +x" << std::endl ;
			}

			// look for the payload (for install mode)
			G::Path payload_path =
				is_mac ?
					search( argv0.dirname() , "payload" , ".." , "." ) :
					search( argv0.dirname() , "payload" , "." ) ;

			// look for for the pointer file (for configure mode)
			G::Path pointer_file ;
			if( is_mac )
				pointer_file = search( argv0.dirname() , "dir.cfg" , ".." ) ;
			if( pointer_file == G::Path() )
				pointer_file = search( argv0.dirname() , pointerFilename(argv0) , "." ) ;

			// choose install or configure mode
			bool configure_mode = is_mac || payload_path == G::Path() ;
			G_LOG_S( "pointer file [" << pointer_file << "]" ) ;
			G_LOG_S( "payload [" << payload_path << "]" ) ;
			G_LOG_S( "running in " << (configure_mode?"configure":"install") << " mode" ) ;

			// fail if no payload and no pointer file
			if( configure_mode && pointer_file == G::Path() )
				throw std::runtime_error( "cannot find a payload for installation or a pointer file to allow reconfiguration: "
					"this program has probably been moved away from its original location: "
					"please configure the emailrelay server manually" ) ;

			// load the pointer file
			G::MapFile pointer_map ;
			if( configure_mode )
			{
				if( G::File::exists(pointer_file) )
				{
					G_LOG_S( "reading directories from [" << pointer_file << "]" ) ;
					pointer_map = G::MapFile( pointer_file ) ;
				}
				else
				{
					G_LOG_S( "cannot read install directories from file [" << pointer_file << "]: "
						"file not found: using built-in defaults" ) ;
				}
			}

			if( is_mac )
			{
				// add some handy substitution variables
				pointer_map.add( "dir-contents" , (argv0.dirname()+"..").collapsed().str() ) ;
				pointer_map.add( "dir-bundle" , (argv0.dirname()+".."+"..").collapsed().str() ) ;
			}

			// load the existing server configuration
			G::Path dir_config = pointer_map.expandedPathValue( "dir-config" , Dir::config() ) ;
			G::Path dir_run = pointer_map.expandedPathValue( "dir-run" , Dir::pid(dir_config) ) ;
			G::Path server_config_file = configFile( dir_config ) ; // config-file or batch-file
			G::MapFile server_config_map ;
			if( configure_mode )
			{
				G_LOG_S( "editing configuration file [" << server_config_file << "]" ) ;
				server_config_map = ServerConfiguration(server_config_file).map() ;
				if( server_config_map.contains("pid-file") )
					dir_run = server_config_map.pathValue("pid-file").dirname() ;
			}

			// determine the install root
			G::Path dir_install = Dir::install() ;
			if( configure_mode )
			{
				// configure-mode dir-install is relative to the server exe or this gui exe, with a pointer-file override
				if( isWindows() )
				{
					// read the server exe referred to in startup batch file
					G::Path server_exe = ServerConfiguration::exe( server_config_file ) ;
					dir_install = server_exe != G::Path() && server_exe.dirname() != G::Path() ?
						server_exe.dirname() : argv0.dirname() ;
				}
				else
				{
					dir_install = argv0.dirname() ;
				}
				dir_install = pointer_map.expandedPathValue( "dir-install" , dir_install ) ; // override
			}

			// log the directory paths
			G_LOG_S( "dir-config=" << dir_config ) ;
			G_LOG_S( "dir-install=" << dir_install ) ;
			G_LOG_S( "dir-run=" << dir_run ) ;

			// set up the gui pages' config map
			G::MapFile pages_config = server_config_map ;
			if( !pages_config.contains("spool-dir") ) pages_config.add( "spool-dir" , Dir::spool().str() ) ;
			pages_config.add( "=dir-config" , dir_config.str() ) ;
			pages_config.add( "=dir-install" , dir_install.str() ) ;
			pages_config.add( "=dir-run" , dir_run.str() ) ;

			// set widget states based on the current file-system state
			if( configure_mode )
			{
				std::string fname = GLink::filename( "E-MailRelay" ) ;
				pages_config.add( "start-on-boot" , Boot::installed(Dir::boot(),"emailrelay") ? "y" : "n" ) ;
				pages_config.add( "start-link-desktop" , GLink::exists(Dir::desktop()+fname) ? "y" : "n" ) ;
				pages_config.add( "start-link-menu" , GLink::exists(Dir::menu()+fname) ? "y" : "n" ) ;
				pages_config.add( "start-at-login" , GLink::exists(Dir::login()+fname) ? "y" : "n" ) ;
			}

			// check the config file (or windows batch file) will be writeable
			if( configure_mode )
			{
				if( G::File::exists(server_config_file) )
				{
					std::string more_help = isWindows() ? " or run as administrator" : "" ;
					std::ofstream f( server_config_file.str().c_str() , std::ios_base::app ) ; // app for no-truncate
					if( !f.good() )
						throw std::runtime_error( "cannot write [" + server_config_file.str() + "]"
							": check file permissions" + more_help ) ;
				}
				else if( !G::Directory(server_config_file.dirname()).valid(true) )
				{
					std::string gerund_article = pointer_map.contains("dir-config") ? "changing the" : "adding a" ;
					std::string preposition = pointer_map.contains("dir-config") ? "in" : "to" ;
					throw std::runtime_error( "cannot create files in [" + server_config_file.dirname().str() + "]: "
						"try " + gerund_article + " \"dir-config\" entry " + preposition + " the configuration file "
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

			// create the installer - the ProgressPage extracts dumps from all the
			// dialog pages and passes the result to the installer
			Installer installer( !configure_mode , isWindows() , isMac() , payload_path ) ;

			// test whether we have run before -- this is normally the same
			// as the configure-mode flag because the first run is always in
			// install mode -- but for a mac bundle we are always in configure
			// mode, so we add a flag file into the distribution bundle that gets
			// deleted by a successful first configuration (see GDialog)
			G::Path virgin_flag_file = is_mac ? (dir_run+".new") : G::Path() ;
			if( virgin_flag_file != G::Path() ) G_LOG_S( "virgin-file=" << virgin_flag_file ) ;
			bool run_before = virgin_flag_file == G::Path() ? configure_mode : !G::File::exists(virgin_flag_file) ;

			// create the dialog and all its pages
			const bool licence_accepted = run_before ;
			GDialog d( false , virgin_flag_file ) ;
			d.add( new TitlePage(d,pages_config,"title","license","",false,false) ) ;
			d.add( new LicensePage(d,pages_config,"license","directory","",false,false,licence_accepted) ) ;
			d.add( new DirectoryPage(d,pages_config,"directory","dowhat","",false,false,!configure_mode,is_mac) ) ;
			d.add( new DoWhatPage(d,pages_config,"dowhat","pop","smtpserver",false,false) ) ;
			d.add( new PopPage(d,pages_config,"pop","smtpserver","logging",false,false,configure_mode) ) ;
			d.add( new SmtpServerPage(d,pages_config,"smtpserver","smtpclient","",false,false,configure_mode) ) ;
			d.add( new SmtpClientPage(d,pages_config,"smtpclient","filter","",false,false,configure_mode) ) ;
			d.add( new FilterPage(d,pages_config,"filter","logging","",false,false,isWindows()) ) ;
			d.add( new LoggingPage(d,pages_config,"logging","listening","",false,false) ) ;
			d.add( new ListeningPage(d,pages_config,"listening","startup","",false,false) ) ;
			d.add( new StartupPage(d,pages_config,"startup","ready","",false,false,Boot::able(Dir::boot()),is_mac) ) ;
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

/// \file guimain.cpp
