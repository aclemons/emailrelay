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

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QMessageBox>
#include "gdialog.h"
#include "pages.h"
#include "glogoutput.h"
#include "garg.h"
#include <string>
#include <iostream>
#include <stdexcept>

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

		try
		{
			QTranslator qt_translator;
			qt_translator.load(QString("qt_")+QLocale::system().name());
			app.installTranslator(&qt_translator);

			QTranslator translator;
			translator.load(QString("emailrelay_install_")+QLocale::system().name());
			app.installTranslator(&translator);

			GDialog d ;
			d.add( new TitlePage(d,"title","license") ) ;
			d.add( new LicensePage(d,"license","directory") ) ;
			d.add( new DirectoryPage(d,"directory","dowhat") ) ;
			d.add( new DoWhatPage(d,"dowhat","pop","smtpserver") ) ;
			d.add( new PopPage(d,"pop","popaccount","popaccounts") ) ;
			d.add( new PopAccountPage(d,"popaccount","smtpserver","startup") ) ;
			d.add( new PopAccountsPage(d,"popaccounts","smtpserver","startup") ) ;
			d.add( new SmtpServerPage(d,"smtpserver","smtpclient") ) ;
			d.add( new SmtpClientPage(d,"smtpclient","startup") ) ;
			d.add( new StartupPage(d,"startup","final") ) ;
			d.add( new FinalPage(d,"final") ) ;
			QSize s = d.size() ;
			if( s.width() < 500 ) s.setWidth(500) ;
			if( s.height() < 500 ) s.setHeight(500) ;
			d.resize( s ) ;

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

