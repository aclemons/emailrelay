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

#include "gpage.h"
#include "gdialog.h"

GPage::GPage( GDialog & dialog , const std::string & name , const std::string & next_1 , const std::string & next_2 ) : 
	QWidget(&dialog) , 
	m_dialog(dialog) ,
	m_name(name) ,
	m_next_1(next_1) ,
	m_next_2(next_2)
{
	hide();
}

GDialog & GPage::dialog()
{
	return m_dialog ;
}

const GDialog & GPage::dialog() const
{
	return m_dialog ;
}

void GPage::reset()
{
}

bool GPage::isComplete()
{
	return true;
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
	QString open( "<center><font color=\"blue\" size=\"5\"><b><i>" ) ;
	QString close( "</i></b></font></center>" ) ;
	QLabel * label = new QLabel( open + s + close ) ;
	QSizePolicy p = label->sizePolicy() ;
	p.setVerticalPolicy( QSizePolicy::Fixed ) ;
	label->setSizePolicy( p ) ;
	return label ;
}

