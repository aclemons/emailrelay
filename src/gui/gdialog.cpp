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
// gdialog.cpp
//

#include "qt.h"
#include "gdialog.h"
#include "gdebug.h"
#include <algorithm>
#include <stdexcept>

GDialog::GDialog( QWidget *parent ) : 
	QDialog(parent) ,
	m_first(true)
{
	m_cancel_button = new QPushButton(tr("Cancel")) ;
	m_back_button = new QPushButton(tr("< &Back")) ;
	m_next_button = new QPushButton(tr("Next >")) ;
	m_finish_button = new QPushButton(tr("&Finish")) ;

	connect( m_back_button, SIGNAL(clicked()), this, SLOT(backButtonClicked()) ) ;
	connect( m_next_button, SIGNAL(clicked()), this, SLOT(nextButtonClicked()) ) ;
	connect( m_cancel_button, SIGNAL(clicked()), this, SLOT(reject()) ) ;
	connect( m_finish_button, SIGNAL(clicked()) , this , SLOT(accept()) ) ;

	m_button_layout = new QHBoxLayout ;
	m_button_layout->addStretch( 1 ) ;
	m_button_layout->addWidget( m_cancel_button ) ;
	m_button_layout->addWidget( m_back_button ) ;
	m_button_layout->addWidget( m_next_button ) ;
	m_button_layout->addWidget( m_finish_button ) ;

	m_main_layout = new QVBoxLayout ;
	m_main_layout->addLayout( m_button_layout ) ;
	setLayout( m_main_layout ) ;
}

void GDialog::add( GPage * page , const std::string & conditional_name )
{
	if( conditional_name.empty() || page->name() == conditional_name )
		add( page ) ;
}

void GDialog::add( GPage * page )
{
	m_map.insert( Map::value_type(page->name(),page) ) ;
	if( m_first )
		setFirstPage( *page ) ;
	m_first = false ;
}

bool GDialog::empty() const
{
	return m_map.empty() ;
}

GPage & GDialog::page( const std::string & name )
{
	if( m_map.find(name) == m_map.end() )
		throw std::runtime_error( std::string() + "internal error: no such page: " + name ) ;
	return *m_map[name] ;
}

const GPage & GDialog::page( const std::string & name ) const
{
	Map::const_iterator p = m_map.find(name) ;
	if( p == m_map.end() )
		throw std::runtime_error( std::string() + "internal error: no such page: " + name ) ;
	return *(*p).second ;
}

void GDialog::setFirstPage( GPage & page )
{
	page.reset() ;
	m_history.push_back(page.name()) ;
	switchPage( m_history.back() ) ;
}

void GDialog::backButtonClicked()
{
	std::string oldPageName = m_history.back();
	m_history.pop_back() ;
	switchPage( m_history.back() , oldPageName , true ) ;
}

void GDialog::nextButtonClicked()
{
	std::string oldPageName = m_history.back();
	std::string newPageName = page(oldPageName).nextPage();
	m_history.push_back(newPageName);
	switchPage( m_history.back() , oldPageName ) ;
}

void GDialog::pageUpdated()
{
	std::string currentPageName = m_history.back() ;
	G_DEBUG( "GDialog::pageUpdated: " << currentPageName ) ;
	GPage & currentPage = page(currentPageName) ;
	if( currentPage.nextPage().empty() || m_map.size() == 1U )
		m_finish_button->setEnabled( currentPage.isComplete() ) ;
	else
		m_next_button->setEnabled( currentPage.isComplete() );
}

void GDialog::switchPage( std::string newPageName , std::string oldPageName , bool back )
{
	// hide and disconnect the old page
	//
	if( ! oldPageName.empty() )
	{
		GPage & oldPage = page(oldPageName) ;
		oldPage.hide();
		m_main_layout->removeWidget(&oldPage);
		disconnect(&oldPage, SIGNAL(onUpdate()), this, SLOT(pageUpdated()));
	}

	// show and connect the new page
	//
	GPage & newPage = page(newPageName) ;
	m_main_layout->insertWidget(0, &newPage);
	newPage.onShow( back ) ; // GPage method
	newPage.show() ;
	newPage.setFocus() ;
	connect(&newPage, SIGNAL(onUpdate()), this, SLOT(pageUpdated()));

	// set the default state of the next and finish buttons
	//
	m_back_button->setEnabled( m_history.size() != 1U ) ;
	if( newPage.nextPage().empty() ) // || m_map.size() == 1U )
	{
		m_next_button->setEnabled(false);
		m_finish_button->setDefault(true);
	} 
	else 
	{
		m_next_button->setDefault(true);
		m_finish_button->setEnabled(false);
	}

	// modify the next and finish buttons according to the page state
	//
	pageUpdated();
}

bool GDialog::historyContains( const std::string & name ) const
{
	return std::find( m_history.begin() , m_history.end() , name ) != m_history.end() ;
}

std::string GDialog::currentPageName() const
{
	return m_history.empty() ? std::string() : m_history.back() ;
}

GPage & GDialog::previousPage( unsigned int distance )
{
	if( m_history.size() < (distance+1U) ) throw std::runtime_error("internal error") ;
	History::reverse_iterator p = m_history.rbegin() ;
	while( distance-- ) ++p ;
	G_DEBUG( "GDialog::previousPage: " << currentPageName() << " -> " << *p ) ;
	return page(*p) ;
}

void GDialog::dump( std::ostream & stream , const std::string & prefix , const std::string & eol ) const
{
	for( History::const_iterator p = m_history.begin() ; p != m_history.end() ; ++p )
		page(*p).dump( stream , prefix , eol ) ;
}

