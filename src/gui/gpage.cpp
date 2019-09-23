//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpage.h
//

#include "gdef.h"
#include "qt.h"
#include "gstr.h"
#include "gpage.h"
#include "gdialog.h"
#include "gmapfile.h"
#include "glog.h"

int GPage::m_test_mode = 0 ;

GPage::GPage( GDialog & dialog , const std::string & name , const std::string & next_1 ,
	const std::string & next_2 , bool finish_button , bool close_button ) :
		QWidget(&dialog) ,
		m_dialog(dialog) ,
		m_name(name) ,
		m_next_1(next_1) ,
		m_next_2(next_2) ,
		m_finish_button(finish_button) ,
		m_close_button(close_button)
{
	hide() ;
}

GDialog & GPage::dialog()
{
	return m_dialog ;
}

const GDialog & GPage::dialog() const
{
	return m_dialog ;
}

std::string GPage::helpName() const
{
	return std::string() ;
}

bool GPage::useFinishButton() const
{
	return m_finish_button ;
}

bool GPage::closeButton() const
{
	return m_close_button ;
}

bool GPage::isComplete()
{
	return true ;
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
	return p->isChecked() ? "y" : "n" ;
}

std::string GPage::stdstr( const QString & s )
{
	// (config files and batch scripts are in the local 8bit code page)
	QByteArray a = s.toLocal8Bit() ; //
	return std::string( a.constData() , a.length() ) ;
}

std::string GPage::stdstr_utf8( const QString & s )
{
	// (userids and passwords are in utf8 (RFC-4954) and then either xtext-ed or hashed)
	QByteArray a = s.toUtf8() ;
	return std::string( a.constData() , a.length() ) ;
}

QString GPage::qstr( const std::string & s )
{
	return QString::fromLocal8Bit( s.data() , static_cast<int>(s.size()) ) ;
}

std::string GPage::value_utf8( const QLineEdit * p )
{
	return stdstr_utf8(p->text()) ;
}

std::string GPage::value( const QLineEdit * p )
{
	return stdstr(p->text()) ;
}

std::string GPage::value( const QComboBox * p )
{
	return stdstr(p->currentText()) ;
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

void GPage::tip( QWidget * w , const std::string & s )
{
	if( !s.empty() )
		w->setToolTip( tip(s.c_str()) ) ;
}

void GPage::tip( QWidget * w , const char * p )
{
	// see also QWidget::setWhatsThis()
	w->setToolTip( tip(p) ) ;
}

void GPage::tip( QWidget * w , NameTip )
{
	w->setToolTip( tip() ) ;
}

void GPage::tip( QWidget * w , PasswordTip )
{
	w->setToolTip( tip() ) ;
}

QString GPage::tip( const char * p )
{
	return QString( p ) ;
}

QString GPage::tip()
{
	return QString( tr("Username or password added to the secrets file") ) ;
}

/// \file gpage.cpp
