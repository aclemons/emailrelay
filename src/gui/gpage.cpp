//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gpage.cpp
///

#include "gdef.h"
#include "gqt.h"
#include "gstr.h"
#include "gpage.h"
#include "gdialog.h"
#include "gmapfile.h"
#include "glog.h"

#include "moc_gpage.cpp"

int GPage::m_test_mode = 0 ;

GPage::GPage( GDialog & dialog , const std::string & name , const std::string & next_1 ,
	const std::string & next_2 ) :
		QWidget(&dialog) ,
		m_dialog(dialog) ,
		m_name(name) ,
		m_next_1(next_1) ,
		m_next_2(next_2)
{
	hide() ;
	addHelpAction() ;
}

GDialog & GPage::dialog()
{
	return m_dialog ;
}

const GDialog & GPage::dialog() const
{
	return m_dialog ;
}

bool GPage::isReadyToFinishPage() const
{
	return false ;
}

bool GPage::isFinishPage() const
{
	return false ;
}

bool GPage::isComplete()
{
	return true ;
}

bool GPage::isFinishing()
{
	return false ;
}

bool GPage::canLaunch()
{
	return false ;
}

std::string GPage::name() const
{
	return m_name ;
}

std::string GPage::next1() const
{
	return m_next_1 ;
}

std::string GPage::next2() const
{
	return m_next_2 ;
}

QLabel * GPage::newTitle( QString s )
{
	QString open( "<center><font size=\"5\"><b>" ) ;
	QString close( "</b></font></center>" ) ;
	QLabel * label = new QLabel( open + s + close ) ;
	QSizePolicy p = label->sizePolicy() ;
	p.setVerticalPolicy( QSizePolicy::Fixed ) ;
	label->setSizePolicy( p ) ;
	return label ;
}

void GPage::dump( std::ostream & stream , bool ) const
{
	G_DEBUG( "GPage::dump: page: " << name() ) ;
	stream << "# page: " << name() << "\n" ;
}

void GPage::dumpItem( std::ostream & stream , bool for_install , const std::string & key , const G::Path & value ) const
{
	dumpItem( stream , for_install , key , value.str() ) ;
}

void GPage::dumpItem( std::ostream & stream , bool , const std::string & key , const std::string & value ) const
{
	G::MapFile::writeItem( stream , key , value ) ;
}

std::string GPage::value( bool b )
{
	return b ? "y" : "n" ;
}

std::string GPage::value( const QAbstractButton * p )
{
	return p && p->isChecked() ? "y" : "n" ;
}

std::string GPage::stdstr( const QString & s )
{
	// (config files and batch scripts are in the local 8bit code page)
	return GQt::stdstr( s ) ;
}

std::string GPage::stdstr_utf8( const QString & s )
{
	// (userids and passwords are in utf8 (RFC-4954) and then either xtext-ed or hashed)
	return GQt::stdstr( s , GQt::Utf8 ) ;
}

QString GPage::qstr( const std::string & s )
{
	return GQt::qstr( s ) ;
}

std::string GPage::value_utf8( const QLineEdit * p )
{
	return stdstr_utf8( p->text().trimmed() ) ;
}

std::string GPage::value( const QLineEdit * p )
{
	return p ? stdstr( p->text().trimmed() ) : std::string() ;
}

std::string GPage::value( const QComboBox * p )
{
	return p ? stdstr( p->currentText().trimmed() ) : std::string() ;
}

void GPage::setTestMode( int test_mode )
{
	m_test_mode = test_mode ;
}

bool GPage::testMode() const
{
	return m_test_mode != 0 ;
}

int GPage::testModeValue() const
{
	return m_test_mode ;
}

void GPage::onShow( bool )
{
	// no-op
}

void GPage::onLaunch()
{
	// no-op
}

void GPage::tip( QWidget * w , const QString & s )
{
	if( !s.isEmpty() )
		w->setToolTip( s ) ; // see also QWidget::setWhatsThis()
}

void GPage::tip( QWidget * w , const char * s )
{
	if( s && s[0] )
		w->setToolTip( s ) ;
}

void GPage::tip( QWidget * w , NameTip )
{
	//: used as a tool-tip for edit boxes containing an authentication username
	w->setToolTip( tr("Username to be added to the secrets file") ) ;
}

void GPage::tip( QWidget * w , PasswordTip )
{
	//: used as a tool-tip for edit boxes containing an authentication password
	w->setToolTip( tr("Password to be added to the secrets file") ) ;
}

void GPage::addHelpAction()
{
	auto action = new QAction( this ) ;
	action->setShortcut( QKeySequence::HelpContents ) ;
	connect( action , SIGNAL(triggered()) , this , SLOT(helpKeyTriggered()) ) ;
	addAction( action ) ;
}

void GPage::helpKeyTriggered()
{
	std::string language = GQt::stdstr( QLocale().bcp47Name() ) ;
	std::string url = helpUrl( language.empty() || language == "C" ? std::string("en") : G::Str::head(language,"-",false) ) ;
	QDesktopServices::openUrl( QString(url.c_str()) ) ;
}

std::string GPage::helpUrl( const std::string & language ) const
{
	return "http://emailrelay.sourceforge.net/help/" + G::Str::lower(m_name) + "#" + language ;
}

