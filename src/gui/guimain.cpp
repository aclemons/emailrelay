//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// This GUI program is primarily intended to help with configuration of an
// initial installation ("--as-install"), but it can also be used to reconfigure
// an existing installation ("--as-configure").
//
// The program determines whether to run in install mode or configure mode by
// looking for packed files appended to the end of the executable, although the
// "--as-(whatever)" command-line switches can be used as an override for this
// test.
//
// In a unix-like installation the install steps are normally done from the
// command-line using "make install", so the GUI is only expected to
// run in configuration mode.
//
// Pointer variables
// -----------------
// In install mode the target directory paths can be set from within the
// GUI and the packed files are extracted into those directories. In
// configuration mode the assumption is that installation is complete so the
// target directory paths are greyed out in the GUI. However, the code still
// needs to know what the installation directories were (especially for the
// main config file) and it tries to obtain these from a special read-only
// "pointer" file located in the same directory as the executable.
//
// The name of the pointer file is the name of the GUI executable without any
// extension; or if the GUI executable had no extension to begin with then it is
// the name of the GUI executable with ".cfg" appended.
//
// In a unix-like installation it is the "make install" step that creates the
// pointer file containing variables pointing to the installation directories,
// but it also cunningly formats it as a shell script so that running the
// pointer file ("emailrelay-gui") actually runs the GUI ("emailrelay-gui.real").
//
// State variables
// ---------------
// The GUI uses "state" variables to record the state of its widgets etc. so
// that it can be easily re-run. Rather than have a separate file for these
// state variables they are just stored in the main configuration file with
// a "gui-" prefix.
//
// The GUI state variables are read initially at startup and then written out
// as part of the final stage of the GUI workflow.
//
// Install variables
// -----------------
// The Installer class operates according to a set of install variables so that
// it is largely independent of the GUI. The install variables are emited by
// GUI pages into a stringstream, and the contents of the stringstream are
// interpreted by the Installer.
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
#include "gunpack.h"
#include "gdialog.h"
#include "gfile.h"
#include "mapfile.h"
#include "state.h"
#include "pointer.h"
#include "gdirectory.h"
#include "dir.h"
#include "pages.h"
#include "boot.h"
#include "glogoutput.h"
#include "ggetopt.h"
#include "garg.h"
#include "gpath.h"
#include "gstr.h"
#include <string>
#include <iostream>
#include <stdexcept>

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
	// (a compile-time test is problably better than run-time here)
 #ifdef G_MAC
	return true ;
 #else
	return false ;
 #endif
}

static void debug( const std::string & prefix , const G::StringMap & map )
{
	G_IGNORE_PARAMETER(std::string,prefix) ;
	if( map.empty() )
	{
		G_DEBUG( prefix << ": (empty)" ) ;
	}
	else
	{
		for( G::StringMap::const_iterator p = map.begin() ; p != map.end() ; ++p )
		{
			G_DEBUG( prefix << ": " << (*p).first << "=[" << (*p).second << "]" ) ;
		}
	}
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
		std::cerr << "exception: " << e.what() << std::endl ;
		error( e.what() ) ;
		std::string message = G::Str::wrap( e.what() , "" , "" , 40 ) ;
		qCritical( "exception: %s" , message.c_str() ) ;
		exit( 3 ) ;
	}
	return false ;
}

int main( int argc , char * argv [] )
{
	try
	{
		Application app( argc , argv ) ;
		G::Arg args( argc , argv ) ;
		G::GetOpt getopt( args ,
			"h/help/show this help text and exit//0//1|"
			"N/no-help/dont show a help button//0//1|"
			"i/as-install/install mode, as if payload present//0//1|"
			"c/as-configure/configure mode, as if no payload present//0//1|"
			"x/extract/extract files only//0//1|"
			"w/write/configuration file for writing//1/file/1|"
			"r/read/configuration file for reading//1/file/1|"
			"p/pointer/directory pointer file//1/file/1|"
			"l/log/output log messages//0//1|"
			"d/debug/output debug log messages/ if compiled in/0//0|"
			"s/syslog/copy log messages to the system log//0//1|"
			// hidden...
			"P/page/single page test//1/page-name/0|"
			"m/mac/enable some mac-like runtime behaviour//0//0|"
			"t/test/test-mode 1 or 2//1/test-type/0" ) ;
		if( getopt.hasErrors() )
		{
			std::ostringstream ss ;
			getopt.showErrors( ss ) ;
			error( ss.str() ) ;
			return 2 ;
		}
		if( getopt.contains("help") )
		{
			std::ostringstream ss ;
			getopt.showUsage( ss , " [<qt4-options>]" , false ) ;
			info( ss.str() ) ;
			return 0 ;
		}
		G::LogOutput log_ouptut( 
			"emailrelay-gui" ,
			getopt.contains("log") , // output
			getopt.contains("log") , // with-logging
			getopt.contains("log") , // with-verbose-logging
			getopt.contains("debug") , // with-debug
			true , // with-level
			false , // with-timestamp
			false , // strip-context
			getopt.contains("syslog") // use-syslog
		) ;
		G_LOG( "main: start: " << argv[0] ) ;

		// parse the commandline
		int test_mode = G::Str::toInt(getopt.contains("test")?getopt.value("test"):std::string(1U,'0')) ;
		bool with_help = !getopt.contains("no-help") ;
		std::string cfg_test_page = getopt.contains("page") ? getopt.value("page") : std::string() ;
		G::Path cfg_read_file( getopt.contains("read") ? getopt.value("read") : std::string() ) ;
		G::Path cfg_write_file( getopt.contains("write") ? getopt.value("write") : std::string() ) ;
		G::Path cfg_pointer_file( getopt.contains("pointer") ? getopt.value("pointer") : std::string() ) ;
		bool cfg_as_mac = getopt.contains("mac") ;
		bool cfg_install = getopt.contains("as-install") ;
		bool cfg_configure = getopt.contains("as-configure") ;
		bool cfg_extract = getopt.contains("extract") ;

		try
		{
			// find the payload -- packed into the running executable or a separate file called "payload"
			G::Path payload_1 = args.v(0) ;
			G::Path payload_2 = G::Path( G::Path(args.v(0)).dirname() , "payload" ) ;
			G::Path payload_3 = G::Path( G::Path(args.v(0)).dirname() , ".." , "payload" ) ;
			G::Path payload =
				G::Unpack::isPacked(payload_1) ? payload_1 : (
				G::Unpack::isPacked(payload_2) ? payload_2 : (
				payload_3 ) ) ;
			bool is_packed = G::Unpack::isPacked( payload ) ;
			int packed_file_count = G::Unpack::fileCount(payload) ;
			if( is_packed )
				G_LOG( "main: found " << packed_file_count << " packed files in " << payload ) ;
			else
				G_DEBUG( "main: no payload (" << payload_1 << "," << payload_2 << "," << payload_3 << ")" ) ;
			if( packed_file_count == 0 )
				is_packed = false ;

			// unpack only if requested
			if( cfg_extract )
			{
				if( packed_file_count == 0 )
					throw std::runtime_error(std::string()+"there is nothing to extract") ;
				G::Unpack package( payload ) ;
				package.unpack( G::Path(args.v(0)).dirname() ) ;
				return 0 ;
			}

			// are we install-mode or configure-mode?
			if( cfg_install && !is_packed )
				throw std::runtime_error(std::string()+"cannot find a valid payload to install; "
					"try moving a suitable payload file into the same directory as this executable, "
					"or remove --as-install to run in configuration mode" ) ;
			if( cfg_install && cfg_configure )
				throw std::runtime_error(std::string()+"usage error; "
					"you cannot use both --as-install and --as-configure" ) ;
			bool is_installing = ( cfg_install || is_packed ) && !cfg_configure ;
			bool is_installed = !is_installing ; // presumably

			// set up a directory pointer map - start with o/s defaults and then override from the file
			G::StringMap dir_map ;
			dir_map["dir-spool"] = Dir::spool().str() ; // should be overridden by pointer file
			dir_map["dir-config"] = Dir::config().str() ; // should be overridden by pointer file
			dir_map["dir-install"] = Dir::install().str() ;
			dir_map["dir-boot"] = Dir::boot().str() ;
			dir_map["dir-desktop"] = Dir::desktop().str() ;
			dir_map["dir-login"] = Dir::login().str() ;
			dir_map["dir-menu"] = Dir::menu().str() ;
			debug( "default directory" , dir_map ) ;
			{
				G::Path pointer_file_in = cfg_pointer_file == G::Path() ? Pointer::file(args.v(0)) : cfg_pointer_file ;
				G_DEBUG( "main: pointer file: " << pointer_file_in ) ;
				std::ifstream pointer_stream( pointer_file_in.str().c_str() ) ;
				if( !pointer_stream.good() && is_installed )
					throw std::runtime_error(std::string()+"cannot open file \""+pointer_file_in.str()+"\" to read the spool and config directory names; this should have been created at install-time; try re-creating the file with DIR_SPOOL=... and DIR_CONFIG=... lines pointing to the relevant install directories, or use --as-install to re-install" ) ;
				Pointer::read( dir_map , pointer_stream ) ;
				if( !pointer_stream.eof() && is_installed )
					throw std::runtime_error(std::string()+"cannot read pointer file \""+pointer_file_in.str()+"\"") ;
			}
			if( dir_map.find("dir-pid") == dir_map.end() )
				dir_map["dir-pid"] = Dir::pid(dir_map["dir-config"]).str() ;
			debug( "directory" , dir_map ) ;
			bool start_on_boot_able = Boot::able( dir_map["dir-boot"] ) ;

			// prepare paths to the main config file
			G::Path config_file_in = cfg_read_file == G::Path() ?
				G::Path::join( dir_map["dir-config"] , "emailrelay.conf" ) :
				cfg_read_file ;
			G::Path config_file_out = cfg_write_file == G::Path() ?
				G::Path::join( dir_map["dir-config"] , "emailrelay.conf" ) :
				cfg_write_file ;

			// read the installed config file - allow for re-installing with
			// preservation of some of the existing config
			G::StringMap config_file_map ;
			{
				G_DEBUG( "main: read config file: " << config_file_in ) ;
				bool strict = false ; // allow config file to be missing (esp. on windows)
				std::ifstream state_stream( config_file_in.str().c_str() ) ;
				if( !state_stream.good() && is_installed && strict )
					throw std::runtime_error(std::string()+"cannot open config file: \""+config_file_in.str()+"\"") ;
				if( state_stream.good() )
				{
					config_file_map = MapFile::read( state_stream ) ;
					if( !state_stream.eof() && is_installed && strict )
						throw std::runtime_error(std::string()+"cannot read config file: \""+config_file_in.str()+"\"") ;
				}
			}
			debug( "config" , config_file_map ) ;
			State state( config_file_map , dir_map ) ;

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
				GPage::setTestMode( test_mode ) ;

			// check the config file will be writeable by ProgressPage
			if( !config_file_out.str().empty() && !is_installing &&
				!G::Directory(config_file_out.dirname()).valid(true) )
			{
				std::ostringstream ss ;
				ss << "config file \"" << config_file_out.str() << "\" is not writable" ;
				if( !getopt.contains("write") )
					ss
						<< ": try \"" << args.prefix() << " --write <config-file>\" "
						<<  "specifying a writable filesystem path; then add "
						<< "\"--read <config-file>\" when re-running" ;
				throw std::runtime_error( ss.str() ) ;
			}

			// create the dialog and all its pages
			GDialog d( with_help ) ;
			d.add( new TitlePage(d,state,"title","license","",false,false) , cfg_test_page ) ;
			d.add( new LicensePage(d,state,"license","directory","",false,false,is_installed) , cfg_test_page ) ;
			d.add( new DirectoryPage(d,state,"directory","dowhat","",false,false,is_installing) , cfg_test_page ) ;
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
			d.add( new StartupPage(d,state,"startup","ready","",false,false,start_on_boot_able,isMac()||cfg_as_mac) , cfg_test_page ) ;
			d.add( new ReadyPage(d,state,"ready","progress","",true,false,is_installing) , cfg_test_page ) ;
			d.add( new ProgressPage(d,state,"progress","","",true,true,args.v(0),payload,config_file_out,is_installing),
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
