//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file guimain.cpp
///
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
// The installation payload is a directory tree called "payload", containing
// a "payload.cfg" configuration file that describes how and where the other
// payload files are to be installed.
//
// The core file-copying stage of the install is driven from the payload
// configuration file. Each line specifies a file-or-directory-tree copy, or
// a chown/chmod. For each file-copy the source path is relative to the payload
// root, and the destination typically starts with a base-directory substitution
// variable (eg. "%dir-install%").
//
// The payload is assembled by the "make-setup.sh" script on unix or
// "winbuild.pl" on windows.
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
// extension; or if the GUI executable had no extension to begin with then
// it is the name of the GUI executable with ".cfg" appended; or on a mac
// it is just "dir.cfg" somewhere inside the bundle.
//
// The main pointer variables are "dir-config", "dir-install" and "dir-run".
// On unix these might be "/etc", "/usr/bin", and "/run", respectively.
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
// System V init script to assemble the server's full command-line (although
// it is no longer done that way).
//
// The install directory, and to a lesser extent the configuration directory,
// will contain additional files copied straight from the payload, such as
// shared libraries, documentation, and utilities.
//

#include "gdef.h"
#include "gqt.h"
#include "guidialog.h"
#include "gfile.h"
#include "gbatchfile.h"
#include "gmapfile.h"
#include "gdirectory.h"
#include "goptionparser.h"
#include "installer.h"
#include "guidir.h"
#include "guilink.h"
#include "guipages.h"
#include "guiboot.h"
#include "serverconfiguration.h"
#include "glogoutput.h"
#include "ggetopt.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include "gstringwrap.h"
#include "gformat.h"
#include <string>
#include <iostream>
#include <stdexcept>
#include <fstream>

#ifdef G_WINDOWS
#ifdef G_QT_STATIC
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
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

static void errorBox( const std::string & what )
{
	QString title( GQt::qstr("E-MailRelay") ) ;
	QString qwhat = GQt::qstr( what ) ;
	QMessageBox::critical( nullptr , title ,
		QMessageBox::tr("Failed with the following exception: %1").arg(qwhat) ,
		QMessageBox::Abort ) ;
}

static QString tr( const char * text )
{
	return QCoreApplication::translate( "main" , text ) ;
}

static void infoBox( const std::string & text )
{
	QString title( GQt::qstr("E-MailRelay") ) ;
	QString qtext = GQt::qstr( text ) ;
	QMessageBox::information( nullptr , title , qtext ,
		QMessageBox::Ok ) ;
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
	Application( int & argc , char * argv [] ) ; // NOLINT std::array
	bool notify( QObject * p1 , QEvent * p2 ) override ;
} ;
Application::Application( int & argc , char * argv [] ) : // NOLINT std::array
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
		errorBox( e.what() ) ;
		std::string message = G::StringWrap::wrap( e.what() , "" , "" , 40U ) ;
		qCritical( message.find(' ')==std::string::npos ? "exception: %s" : "%s" , message.c_str() ) ;
		exit( 3 ) ;
	}
	return false ;
}

static G::Path search( const G::Path & base , const std::string & filename ,
	const std::string & d1 , const std::string & d2 = std::string() , const std::string & d3 = std::string() )
{
	if( !d1.empty() && G::File::exists( base + d1 + filename ) ) return base + d1 + filename ;
	if( !d2.empty() && G::File::exists( base + d2 + filename ) ) return base + d2 + filename ;
	if( !d3.empty() && G::File::exists( base + d3 + filename ) ) return base + d3 + filename ;
	return G::Path() ;
}

static std::string pointerFilename( const G::Path & argv0 )
{
	std::string filename = argv0.withoutExtension().basename() ;
	std::string ext = argv0.basename().find('.') == std::string::npos || isWindows() ? ".cfg" : "" ;
	filename.append( ext ) ;
	return filename ;
}

static G::Path configFile( const G::Path & dir_config )
{
	return isWindows() ?
		(dir_config+"emailrelay-start.bat") :
		(dir_config+"emailrelay.conf") ;
}

static std::string value( const G::Arg & args , const std::string & option )
{
	std::size_t pos = args.index( option , 1U ) ;
	std::size_t pos_eq = args.match( option+"=" ) ;
	if( pos )
		return args.v( pos + 1U ) ;
	else if( pos_eq )
		return G::Str::tail( args.v(pos_eq) , "=" ) ;
	else
		return std::string() ;
}

int main( int argc , char * argv [] )
{
	try
	{
		const G::Arg args( argc , argv ) ;
		Application app( argc , argv ) ;
		if( argc > 1 && std::string(argv[1]) == "--message" ) // message-box helper esp. for mac
		{
			std::ostringstream ss ;
			const char * sep = "" ;
			for( int i = 2 ; i < argc ; i++ , sep = " " )
				ss << sep << argv[i] ;
			infoBox( ss.str() ) ;
			return 1 ;
		}
		G::LogOutput log_ouptut( "" ,
			G::LogOutput::Config()
				.set_output_enabled(true)
				.set_summary_info(true)
				.set_verbose_info(args.contains("-v"))
				.set_debug(args.count("-v")>2U)
				.set_strip(args.count("-v")<2U)
				.set_with_level(true)
				.set_use_syslog(false) ) ;
		G_LOG( "main: start: " << argv[0] ) ;

		try
		{
			G::Path argv0 = G::Arg::exe().empty() ? args.v(0U) : G::Arg::exe() ;
			bool is_mac = isMac() || args.contains("--as-mac") ;

			// load translations from 'translations/emailrelay*.qm' files according
			// to the 'LANG' environment variable or an explicit '--qm' option
			QTranslator translator ;
			{
				G_LOG( "main: locale: " << GQt::stdstr(QLocale::system().name()) ) ; // eg. "en_GB"
				bool loaded = false ;
				G::Path qmfile = value( args , "--qm" ) ;
				G::Path qmdir = value( args , "--qmdir" ) ;
				if( qmdir.empty() )
					qmdir = argv0.dirname() + "translations" ;
				if( !qmfile.empty() )
					loaded = translator.load( GQt::qstr(qmfile) ) ;
				if( !loaded )
					loaded = translator.load( QLocale() , GQt::qstr("emailrelay") , GQt::qstr(".") ,
						GQt::qstr(qmdir.str()) , GQt::qstr(".qm") ) ;
				if( loaded )
					QCoreApplication::installTranslator( &translator ) ;
				else
					G_LOG( "main: no translations loaded" ) ;
			}

			// load an icon
			if( !isWindows() && !isMac() && !args.contains("-qwindowicon") )
			{
				G::Path icon_png_path = search( argv0.dirname() , "emailrelay-icon.png" , "." , "icon" , "resources" ) ;
				if( !icon_png_path.empty() )
					app.setWindowIcon( QIcon(GQt::qstr(icon_png_path)) ) ;
			}

			// test-mode -- create a minimal payload, make it easier to
			// click through, and write install variables to installer.txt
			if( args.contains("--test") )
			{
				G::Path pdir = argv0.dirname() + "payload" ;
				G::File::mkdir( pdir , std::nothrow ) ;
				std::ofstream f ;
				G::File::open( f , pdir+"payload.cfg" , G::File::Text() ) ;
				if( isWindows() )
				{
					G::File::copyInto( "emailrelay-gui.exe" , pdir , std::nothrow ) ;
					f << "emailrelay-gui.exe= %dir-install%/emailrelay-gui.exe +x" << std::endl ;
				}
				else
				{
					G::File::mkdirs( pdir+"usr/sbin" , std::nothrow ) ;
					G::File::copyInto( argv0 , pdir + "usr/sbin" , std::nothrow ) ;
					G::File::copyInto( "/usr/sbin/emailrelay" , pdir + "usr/sbin" , std::nothrow ) ;
					f << "usr/sbin/=%dir-install%/sbin/" << std::endl ;
				}
				Gui::Page::setTestMode() ;
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
			if( pointer_file.empty() )
				pointer_file = search( argv0.dirname() , pointerFilename(argv0) , "." ) ;

			// choose install or configure mode
			bool configure_mode = is_mac || payload_path.empty() ;
			G_LOG_S( "main: pointer file [" << pointer_file << "]" ) ;
			G_LOG_S( "main: payload [" << payload_path << "]" ) ;
			G_LOG_S( "main: running in " << (configure_mode?"configure":"install") << " mode" ) ;

			// fail if no payload and no pointer file
			if( configure_mode && pointer_file.empty() )
			{
				QString message_format = tr( "cannot find a 'payload' directory for installation or a "
					"'%1' pointer file to allow reconfiguration: "
					"this program has probably been moved away from its original location: "
					"please configure the emailrelay server manually" ) ;
				QString message = message_format.arg( GQt::qstr( pointerFilename(argv0) , GQt::Path ) ) ;
				throw std::runtime_error( GQt::stdstr( message , GQt::Utf8 ) ) ;
			}

			// load the pointer file
			G::MapFile pointer_map ;
			if( configure_mode )
			{
				if( G::File::exists(pointer_file) )
				{
					G_LOG_S( "main: reading directories from [" << pointer_file << "]" ) ;
					pointer_map = G::MapFile( pointer_file , "pointer" ) ;
				}
				else
				{
					G_LOG_S( "main: cannot read install directories from file [" << pointer_file << "]: "
						"file not found: using built-in defaults" ) ;
				}
			}

			// add some handy substitution variables
			if( is_mac )
			{
				pointer_map.add( "dir-contents" , (argv0.dirname()+"..").collapsed().str() ) ;
				pointer_map.add( "dir-bundle" , (argv0.dirname()+".."+"..").collapsed().str() ) ;
			}

			// load the existing server configuration
			G::Path dir_config = pointer_map.expandedPathValue( "dir-config" , Gui::Dir::config() ) ;
			G::Path dir_run = pointer_map.expandedPathValue( "dir-run" , Gui::Dir::pid(dir_config) ) ;
			G::Path server_config_file = configFile( dir_config ) ; // config-file or batch-file
			G::MapFile server_config_map ;
			if( configure_mode )
			{
				G_LOG_S( "main: editing configuration file [" << server_config_file << "]" ) ;
				server_config_map = ServerConfiguration(server_config_file).map() ;
				if( server_config_map.contains("pid-file") )
					dir_run = server_config_map.pathValue("pid-file").dirname() ;
			}

			// determine the install root
			G::Path dir_install = Gui::Dir::install() ;
			if( configure_mode )
			{
				// configure-mode dir-install is relative to the server exe or this gui exe, with a pointer-file override
				if( isWindows() )
				{
					// read the server exe referred to in startup batch file
					G::Path server_exe = ServerConfiguration::exe( server_config_file ) ;
					dir_install = !server_exe.empty() && !server_exe.dirname().empty() ?
						server_exe.dirname() : argv0.dirname() ;
				}
				else
				{
					dir_install = argv0.dirname() ;
				}
				dir_install = pointer_map.expandedPathValue( "dir-install" , dir_install ) ; // override
			}

			// log the directory paths
			G_LOG_S( "main: dir-config=" << dir_config ) ;
			G_LOG_S( "main: dir-install=" << dir_install ) ;
			G_LOG_S( "main: dir-run=" << dir_run ) ;

			// set up the gui pages' config map
			G::MapFile pages_config = server_config_map ;
			pages_config.add( "=dir-config" , dir_config.str() ) ;
			pages_config.add( "=dir-install" , dir_install.str() ) ;
			pages_config.add( "=dir-run" , dir_run.str() ) ;
			pages_config.add( "=dir-boot-enabled" , Gui::Boot::installable() ? "y" : "n" ) ;
			pages_config.add( "=dir-autostart-enabled" , !Gui::Dir::autostart().str().empty() ? "y" : "n" ) ;
			pages_config.add( "=dir-menu-enabled" , !Gui::Dir::menu().str().empty() ? "y" : "n" ) ;
			pages_config.add( "=dir-desktop-enabled" , !Gui::Dir::desktop().str().empty() ? "y" : "n" ) ;
			if( !pages_config.contains("spool-dir") )
				pages_config.add( "spool-dir" , Gui::Dir::spool().str() ) ;

			// set widget states
			if( configure_mode )
			{
				// ... based on the current file-system state
				std::string fname = Gui::Link::filename( "E-MailRelay" ) ;
				pages_config.add( "start-on-boot" , Gui::Boot::installed("emailrelay") ? "y" : "n" ) ;
				pages_config.add( "start-at-login" , Gui::Link::exists(Gui::Dir::autostart(),fname) ? "y" : "n" ) ;
				pages_config.add( "start-link-menu" , Gui::Link::exists(Gui::Dir::menu(),fname) ? "y" : "n" ) ;
				pages_config.add( "start-link-desktop" , Gui::Link::exists(Gui::Dir::desktop(),fname) ? "y" : "n" ) ;
			}
			else
			{
				pages_config.add( "start-on-boot" , Gui::Boot::installable() ? "y" : "n" ) ;
				pages_config.add( "start-at-login" , "n" ) ;
				pages_config.add( "start-link-menu" , !Gui::Dir::menu().str().empty() ? "y" : "n" ) ;
				pages_config.add( "start-link-desktop" , "n" ) ;
			}

			// check the config file (or windows batch file) will be writeable
			if( configure_mode )
			{
				if( G::File::exists(server_config_file) )
				{
					std::ofstream f ;
					G::File::open( f , server_config_file , G::File::Append() ) ;
					if( !f.good() )
					{
						QString message_format = tr( "cannot write [%1]: check file permissions%2" ) ;
						QString more_help = isWindows() ? tr( " or run as administrator" ) : QString() ;
						QString message = message_format.arg(GQt::qstr(server_config_file.str())).arg(more_help) ;
						throw std::runtime_error( GQt::stdstr( message , GQt::Utf8 ) ) ;
					}
				}
				else if( !G::Directory(server_config_file.dirname()).valid(true) )
				{
					bool exists = pointer_map.contains( "dir-config" ) ;
					QString message_format = exists ?
						tr( "cannot create files in [%1]: try changing the \"dir-config\" entry in the configuration file [%2]" ) :
						tr( "cannot create files in [%1]: try adding a \"dir-config\" entry to the configuration file [%2]" ) ;
					QString message = message_format.arg(GQt::qstr(server_config_file.dirname())).arg(GQt::qstr(pointer_file)) ;
					throw std::runtime_error( GQt::stdstr( message , GQt::Utf8 ) ) ;
				}
			}

			// create the installer - the ProgressPage extracts dumps from all the
			// dialog pages and passes the result to the installer
			Installer installer( !configure_mode , isWindows() , isMac() , payload_path ) ;
			bool can_generate = installer.canGenerateKey() ;

			// test whether we have run before -- this is normally the same
			// as the configure-mode flag because the first run is always in
			// install mode -- but for a mac bundle we are always in configure
			// mode, so we add a flag file into the distribution bundle that gets
			// deleted by a successful first configuration (see Gui::Dialog)
			bool run_before = configure_mode ;
			G::Path virgin_flag_file ;
			if( is_mac )
			{
				virgin_flag_file = dir_run + ".new" ;
				G_LOG_S( "main: virgin-file=" << virgin_flag_file ) ;
				run_before = !G::File::exists( virgin_flag_file ) ;
			}

			// create the dialog and all its pages
			const bool licence_accepted = run_before ;
			const bool with_launch = !configure_mode ;
			const bool skip_startup_page = configure_mode && !isWindows() ;
			Gui::Dialog d( virgin_flag_file , with_launch ) ;
			d.add( new TitlePage(d,pages_config,"title","license","") ) ;
			d.add( new LicensePage(d,pages_config,"license","directory","",licence_accepted) ) ;
			d.add( new DirectoryPage(d,pages_config,"directory","dowhat","",!configure_mode,isWindows(),is_mac) ) ;
			d.add( new DoWhatPage(d,pages_config,"dowhat","pop","smtpserver") ) ;
			d.add( new PopPage(d,pages_config,"pop","smtpserver","logging",configure_mode) ) ;
			d.add( new SmtpServerPage(d,pages_config,"smtpserver","smtpclient","",configure_mode,can_generate&&!configure_mode) ) ;
			d.add( new SmtpClientPage(d,pages_config,"smtpclient","filter","",configure_mode) ) ;
			d.add( new FilterPage(d,pages_config,"filter","logging","",!configure_mode,isWindows()) ) ;
			d.add( new LoggingPage(d,pages_config,"logging","listening","") ) ;
			d.add( new ListeningPage(d,pages_config,"listening","startup","ready",skip_startup_page) ) ;
			d.add( new StartupPage(d,pages_config,"startup","ready","",is_mac) ) ;
			d.add( new ReadyPage(d,pages_config,"ready","progress","",!configure_mode) ) ;
			d.add( new ProgressPage(d,pages_config,"progress","","",installer,!configure_mode) ) ;
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
			errorBox( e.what() ) ;
			std::string message = G::StringWrap::wrap( e.what() , "" , "" , 40 ) ;
			qCritical( "%s" , message.c_str() ) ;
		}
		catch(...)
		{
			errorBox( "unknown exception" ) ;
			qCritical( "%s" , "unknown exception" ) ;
		}
		return 1 ;
	}
	catch( std::exception & e )
	{
		std::string message = G::StringWrap::wrap( e.what() , "" , "" , 40U ) ;
		qCritical( "%s" , message.c_str() ) ;
	}
	catch(...)
	{
		qCritical( "%s" , "unknown exception" ) ;
	}
	return 1 ;
}

