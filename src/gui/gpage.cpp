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
// gpage.h
//

#include "qt.h"
#include "gstr.h"
#include "gpage.h"
#include "gdialog.h"

bool GPage::m_test_mode = false ;
std::string GPage::m_tool ;
std::string GPage::m_tool_arg ;

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

void GPage::dump( std::ostream & stream , const std::string & prefix , const std::string & eol ) const
{
	stream << prefix << "# " << name() << eol ;
}

std::string GPage::value( const QAbstractButton * p )
{
	return p->isChecked() ? "y" : "n" ;
}

std::string GPage::value( const QLineEdit * p )
{
	return p->text().toStdString() ;
}

std::string GPage::value( const QComboBox * p )
{
	return p->currentText().toStdString() ;
}

void GPage::setTestMode()
{
	m_test_mode = true ;
}

bool GPage::testMode() const
{
	return m_test_mode ;
}

void GPage::onShow( bool )
{
	// no-op
}

void GPage::setTool( const std::string & tool , const std::string & arg )
{
	m_tool = tool ;
	m_tool_arg = arg ;
}

std::string GPage::tool()
{
	return m_tool ;
}

G::Strings GPage::toolArgs( const std::string & prefix )
{
	G::Strings result ;
	G::Str::splitIntoTokens( m_tool_arg , result , G::Str::ws() ) ;
	if( ! prefix.empty() ) result.push_front(prefix) ;
	return result ;
}

void GPage::mechanismUpdateSlot( const QString & m )
{
	static bool first = true ;
	if( first && m != "CRAM-MD5" )
	{
		QString title(QMessageBox::tr("E-MailRelay")) ;
		QMessageBox::warning( NULL , title , 
			QMessageBox::tr("Passwords will be written to a text file in the clear if not using CRAM-MD5.\nIf security is important then use dummy passwords now and edit the secrets file \"emailrelay.auth\" later on.") ,
			QMessageBox::Ok , QMessageBox::NoButton , QMessageBox::NoButton ) ;
		first = false ;
	}
}

