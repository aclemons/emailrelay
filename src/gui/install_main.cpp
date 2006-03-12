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
// install.cpp
//

#include "qt.h"
#include "gdialog.h"
#include "gfile.h"
#include "gsystem.h"
#include "pages.h"
#include "glogoutput.h"
#include "garg.h"
#include "gpath.h"
#include <string>
#include <iostream>
#include <stdexcept>

#if QT_VERSION < 0x400000
#define qCritical(a,b)
#endif

namespace
{
	int width() { return 500 ; }
	int height() { return 500 ; }
}

void error( const std::string & what )
{
	QString title(QMessageBox::tr("E-MailRelay installation")) ;
	QMessageBox::critical( NULL , title , 
		QMessageBox::tr("E-MailRelay installation failed with the following exception: %1").arg(what.c_str()) ,
		QMessageBox::Abort , QMessageBox::NoButton , QMessageBox::NoButton ) ;
}

int main( int argc , char * argv [] )
{
	try
	{
		QApplication app( argc , argv ) ;
		G::Arg args( argc , argv ) ;
		G::LogOutput log_ouptut( args.contains("--debug") ) ;

		// parse the commandline
		bool test_mode = args.contains("--test") ;
		std::string test_page = args.index("--page",1U) ? args.v(args.index("--page",1U)+1U) : std::string() ;
		std::string tool = args.index("--tool",1U) ? args.v(args.index("--tool",1U)+1U) : std::string() ;

		try
		{
			// default translator
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			// application translator
			QTranslator translator;
			translator.load(QString("emailrelay_install_")+QLocale::system().name());
			app.installTranslator(&translator);

			// prepare an absolute path to the non-gui tool
			if( tool.empty() )
			{
				G::Path this_dir = G::Path(args.v(0)).dirname() ; 
				G::Path tool_dir = this_dir ;
				if( this_dir.isRelative() && !this_dir.hasDriveLetter() )
				{
					tool_dir = GSystem::cwd() ;
					tool_dir.pathAppend( this_dir.str() ) ;
				}
				G::Path tool_path = tool_dir ; 
				tool_path.pathAppend("install-tool") ;
				tool = tool_path.str() ;
			}

			// check the tool path is valid
std::cout << "[" << tool << "]" << std::endl ;
			if( ! G::File::exists(G::Path(tool)) )
				throw std::runtime_error(std::string()+"invalid install tool path: \""+tool+"\"") ;

			// initialise GPage
			GPage::setTool( tool ) ;
			if( ! test_page.empty() || test_mode ) 
				GPage::setTestMode() ;

			// create the dialog and all its pages
			GDialog d ;
			d.add( new TitlePage(d,"title","license") , test_page ) ;
			d.add( new LicensePage(d,"license","directory") , test_page ) ;
			d.add( new DirectoryPage(d,"directory","dowhat") , test_page ) ;
			d.add( new DoWhatPage(d,"dowhat","pop","smtpserver") , test_page ) ;
			d.add( new PopPage(d,"pop","popaccount","popaccounts") , test_page ) ;
			d.add( new PopAccountPage(d,"popaccount","smtpserver","startup") , test_page ) ;
			d.add( new PopAccountsPage(d,"popaccounts","smtpserver","startup") , test_page ) ;
			d.add( new SmtpServerPage(d,"smtpserver","smtpclient") , test_page ) ;
			d.add( new SmtpClientPage(d,"smtpclient","startup") , test_page ) ;
			d.add( new StartupPage(d,"startup","todo") , test_page ) ;
			d.add( new ToDoPage(d,"todo","progress") , test_page ) ;
			d.add( new ProgressPage(d,"progress","final") , test_page ) ;
			d.add( new FinalPage(d,"final") , test_page ) ;

			// check the test_page value
			if( d.empty() )
				throw std::runtime_error(std::string()+"invalid page name: \""+test_page+"\"") ;

			// set the dialog dimensions
			QSize s = d.size() ;
			if( s.width() < width() ) s.setWidth(width()) ;
			if( s.height() < height() ) s.setHeight(height()) ;
			d.resize( s ) ;

			// run the dialog
			return d.exec() ;
		}
		catch( std::exception & e )
		{
			error( e.what() ) ;
			std::cerr << "exception: " << e.what() << std::endl ;
			qCritical( "exception: %s" , e.what() ) ;
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
		qCritical( "exception: %s" , e.what() ) ;
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl ;
		qCritical( "%s" , "unknown exception" ) ;
	}
	return 1 ;
}

