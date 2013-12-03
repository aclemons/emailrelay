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
// gdialog.cpp
//

#include "gdef.h"
#include "qt.h"
#include "gdialog.h"
#include "gdebug.h"
#include <algorithm>
#include <stdexcept>

GDialog::GDialog( bool with_help ) :
	QDialog(NULL) ,
	m_first(true) ,
	m_help_button(NULL) ,
	m_cancel_button(NULL) ,
	m_back_button(NULL) ,
	m_next_button(NULL) ,
	m_finish_button(NULL) ,
	m_waiting(false)
{
	init( with_help ) ;
}

GDialog::GDialog( QWidget *parent ) :
	QDialog(parent) ,
	m_first(true) ,
	m_help_button(NULL) ,
	m_cancel_button(NULL) ,
	m_back_button(NULL) ,
	m_next_button(NULL) ,
	m_finish_button(NULL) ,
	m_waiting(false)
{
	init( false ) ;
}

void GDialog::init( bool with_help )
{
	if( with_help )
	{
		m_help_button = new QPushButton(tr("&Help")) ;
		connect( m_help_button, SIGNAL(clicked()) , this , SLOT(helpButtonClicked()) ) ;
	}

	m_cancel_button = new QPushButton(tr("Cancel")) ;
	m_back_button = new QPushButton(tr("< &Back")) ;
	m_next_button = new QPushButton(tr("Next >")) ;
	m_finish_button = new QPushButton(tr("&Finish")) ;

	connect( m_back_button, SIGNAL(clicked()), this, SLOT(backButtonClicked()) ) ;
	connect( m_next_button, SIGNAL(clicked()), this, SLOT(nextButtonClicked()) ) ;
	connect( m_cancel_button, SIGNAL(clicked()), this, SLOT(reject()) ) ;
	connect( m_finish_button, SIGNAL(clicked()) , this , SLOT(finishButtonClicked()) ) ;

	m_button_layout = new QHBoxLayout ;
	if( m_help_button != NULL )
		m_button_layout->addWidget( m_help_button ) ;
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

void GDialog::add()
{
	if( !empty() )
		pageUpdated() ;
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
	m_history.push_back(page.name()) ;
	switchPage( m_history.back() ) ;
}

void GDialog::helpButtonClicked()
{
	std::string base = "http://emailrelay.sourceforge.net/help/" ;
	std::string url = base + page(currentPageName()).helpName() + ".html" ;
	QDesktopServices::openUrl( QString(url.c_str()) ) ;
}

void GDialog::backButtonClicked()
{
	std::string old_page_name = m_history.back();
	m_history.pop_back() ;
	switchPage( m_history.back() , old_page_name , true ) ;
}

void GDialog::nextButtonClicked()
{
	std::string old_page_name = m_history.back();
	std::string new_page_name = page(old_page_name).nextPage();
	m_history.push_back(new_page_name);
	switchPage( m_history.back() , old_page_name ) ;
}

void GDialog::finishButtonClicked()
{
	std::string old_page_name = m_history.back();
	std::string new_page_name = page(old_page_name).nextPage();
	if( new_page_name.empty() )
	{
		accept() ;
	}
	else
	{
		m_history.push_back(new_page_name);
		switchPage( m_history.back() , old_page_name ) ;
	}
}

void GDialog::pageUpdated()
{
	std::string current_page_name = m_history.back() ;
	G_DEBUG( "GDialog::pageUpdated: \"" << current_page_name << "\" page updated" ) ;

	GPage & current_page = page(current_page_name) ;
	if( m_waiting )
	{
		; // no-op
	}
	else if( current_page.closeButton() )
	{
		m_cancel_button->setEnabled(false) ;
		m_back_button->setEnabled(false) ;
		m_next_button->setEnabled(false) ;
		m_finish_button->setText(tr("Close")) ;
		m_finish_button->setEnabled(true) ;
		if( m_help_button != NULL )
			m_help_button->setEnabled(!current_page.helpName().empty()) ;
	}
	else
	{
		m_cancel_button->setEnabled(true) ;
		m_back_button->setEnabled( m_history.size() != 1U ) ;

		// enable either 'next' or 'finish'...
		m_finish_button->setText(tr("&Finish")) ;
		bool finish_button = current_page.useFinishButton() ;
		QPushButton * active_button = finish_button ? m_finish_button : m_next_button ;
		QPushButton * inactive_button = finish_button ? m_next_button : m_finish_button ;

		// ...or neither
		bool active_state = current_page.isComplete() ;
		if( active_button == m_next_button && current_page.nextPage().empty() )
			active_state = false ;

		active_button->setEnabled( active_state ) ;
		inactive_button->setEnabled( false ) ;

		if( m_help_button != NULL )
			m_help_button->setEnabled(!current_page.helpName().empty()) ;
	}
}

void GDialog::switchPage( std::string new_page_name , std::string old_page_name , bool back )
{
	// hide and disconnect the old page
	//
	if( ! old_page_name.empty() )
	{
		GPage & oldPage = page(old_page_name) ;
		oldPage.hide();
		m_main_layout->removeWidget(&oldPage);
		disconnect(&oldPage, SIGNAL(pageUpdateSignal()), this, SLOT(pageUpdated()));
	}

	// show and connect the new page
	//
	GPage & newPage = page(new_page_name) ;
	m_main_layout->insertWidget(0, &newPage);
	newPage.onShow( back ) ; // GPage method
	newPage.show() ;
	newPage.setFocus() ;
	connect(&newPage, SIGNAL(pageUpdateSignal()), this, SLOT(pageUpdated())) ;

	// modify the next and finish buttons according to the page state
	//
	pageUpdated() ;
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

void GDialog::dumpStateVariables( std::ostream & stream ) const
{
	dump( stream , false ) ;
}

void GDialog::dumpInstallVariables( std::ostream & stream ) const
{
	dump( stream , true ) ;
}

void GDialog::dump( std::ostream & stream , bool for_install ) const
{
	for( History::const_iterator p = m_history.begin() ; p != m_history.end() ; ++p )
		page(*p).dump( stream , for_install ) ;
}

void GDialog::wait( bool wait_on )
{
	if( wait_on && !m_waiting )
	{
		m_waiting = wait_on ;

		m_back_state = m_back_button->isEnabled() ;
		m_next_state = m_next_button->isEnabled() ;
		m_finish_state = m_finish_button->isEnabled() ;

		// all off
		m_cancel_button->setEnabled(false);
		m_back_button->setEnabled(false);
		m_next_button->setEnabled(false);
		m_finish_button->setEnabled(false);
	}
	else if( m_waiting && !wait_on )
	{
		m_waiting = wait_on ;
		pageUpdated() ;
	}
}

/// \file gdialog.cpp
