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
/// \file guidialog.cpp
///

#include "gdef.h"
#include "gqt.h"
#include "guidialog.h"
#include "gfile.h"
#include "glog.h"
#include <algorithm>
#include <stdexcept>

#ifndef G_NO_MOC_INCLUDE
#include "moc_guidialog.cpp"
#endif

Gui::Dialog::Dialog( const G::Path & virgin_flag_file , bool with_launch ) :
	QDialog(nullptr) ,
	m_first(true) ,
	m_with_launch(with_launch) ,
	m_next_is_launch(false) ,
	m_help_button(nullptr) ,
	m_cancel_button(nullptr) ,
	m_back_button(nullptr) ,
	m_next_button(nullptr) ,
	m_finish_button(nullptr) ,
	m_button_layout(nullptr) ,
	m_main_layout(nullptr) ,
	m_finishing(false) ,
	m_back_state(false) ,
	m_next_state(false) ,
	m_finish_state(false) ,
	m_virgin_flag_file(virgin_flag_file)
{
	m_cancel_button = new QPushButton(tr("Cancel")) ;
	m_back_button = new QPushButton(tr("&< Back")) ;
	m_next_button = new QPushButton(tr("Next &>")) ;
	m_finish_button = new QPushButton(tr("&Finish")) ;

	connect( m_back_button , SIGNAL(clicked()) , this, SLOT(backButtonClicked()) ) ;
	connect( m_next_button , SIGNAL(clicked()) , this, SLOT(nextButtonClicked()) ) ;
	connect( m_cancel_button , SIGNAL(clicked()) , this, SLOT(reject()) ) ;
	connect( m_finish_button , SIGNAL(clicked()) , this , SLOT(finishButtonClicked()) ) ;

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

void Gui::Dialog::add( Page * page , const std::string & conditional_name )
{
	if( conditional_name.empty() || page->name() == conditional_name )
		add( page ) ;
}

void Gui::Dialog::add( Page * page )
{
	m_map.insert( Map::value_type(page->name(),page) ) ;
	if( m_first )
		setFirstPage( *page ) ;
	m_first = false ;
}

void Gui::Dialog::add()
{
	if( !empty() )
		pageUpdated() ;
}

bool Gui::Dialog::empty() const
{
	return m_map.empty() ;
}

Gui::Page & Gui::Dialog::page( const std::string & name )
{
	if( m_map.find(name) == m_map.end() )
		throw std::runtime_error( std::string() + "internal error: no such page: " + name ) ;
	return *m_map[name] ;
}

const Gui::Page & Gui::Dialog::page( const std::string & name ) const
{
	auto p = m_map.find( name ) ;
	if( p == m_map.end() )
		throw std::runtime_error( std::string() + "internal error: no such page: " + name ) ;
	return *(*p).second ;
}

void Gui::Dialog::setFirstPage( Page & page )
{
	m_history.push_back(page.name()) ;
	switchPage( m_history.back() ) ;
}

void Gui::Dialog::backButtonClicked()
{
	if( m_history.size() < 2U )
		throw std::runtime_error( std::string() + "internal error: cannot go back" ) ;
	std::string old_page_name = m_history.back();
	m_history.pop_back() ;
	switchPage( m_history.back() , old_page_name , true ) ;
}

void Gui::Dialog::nextButtonClicked()
{
	if( m_next_is_launch )
	{
		m_next_button->setEnabled( false ) ;
		page(m_history.back()).onLaunch() ;
	}
	else
	{
		std::string old_page_name = m_history.back() ;
		std::string new_page_name = page(old_page_name).nextPage() ;
		m_history.push_back( new_page_name ) ;
		switchPage( m_history.back() , old_page_name ) ;
	}
}

void Gui::Dialog::finishButtonClicked()
{
	if( page(m_history.back()).isFinishPage() ) // ie. "close" button clicked
	{
		accept() ; // QDialog::accept() terminates the modal dialog box
	}
	else
	{
		// next -- ie. ready-to-finish -> finish page
		std::string old_page_name = m_history.back() ;
		std::string new_page_name = page(old_page_name).nextPage() ;
		m_history.push_back( new_page_name ) ;
		switchPage( m_history.back() , old_page_name ) ;
	}
}

void Gui::Dialog::pageUpdated()
{
	std::string current_page_name = m_history.back() ;
	G_DEBUG( "Gui::Dialog::pageUpdated: \"" << current_page_name << "\" page updated" ) ;
	Page & current_page = page( current_page_name ) ;

	if( current_page.isReadyToFinishPage() )
	{
		// ready to finish -- no next button
		m_cancel_button->setEnabled( true ) ;
		m_back_button->setEnabled( m_history.size() != 1U ) ;
		m_next_button->setEnabled( false ) ;
		m_finish_button->setEnabled( true ) ;
	}
	else if( current_page.isFinishPage() && current_page.isFinishing() )
	{
		// finishing -- all disabled
		m_cancel_button->setEnabled( false ) ; // moot
		m_back_button->setEnabled( false ) ;
		m_next_button->setEnabled( false ) ;
		m_finish_button->setEnabled( false ) ;
	}
	else if( current_page.isFinishPage() && !current_page.isComplete() )
	{
		// finishing failed -- can go back
		m_cancel_button->setEnabled( true ) ;
		m_back_button->setEnabled( m_history.size() != 1U ) ;
		m_next_button->setEnabled( false ) ;
		m_finish_button->setEnabled( false ) ;
	}
	else if( current_page.isFinishPage() && current_page.isComplete() )
	{
		// finished ok -- close or launch
		m_cancel_button->setEnabled( false ) ;
		m_back_button->setEnabled( false ) ;
		m_next_button->setEnabled( m_next_is_launch = m_with_launch && current_page.canLaunch() ) ;
		m_finish_button->setText( tr("Close") ) ;
		m_finish_button->setEnabled( true ) ;
		if( !m_virgin_flag_file.empty() )
			G::File::remove( m_virgin_flag_file , std::nothrow ) ;
		if( m_next_is_launch )
			m_next_button->setText( tr("Launch") ) ;
	}
	else
	{
		m_cancel_button->setEnabled( true ) ;
		m_back_button->setEnabled( m_history.size() != 1U ) ;
		m_next_button->setEnabled( current_page.isComplete() ) ;
		m_finish_button->setEnabled( false ) ;
	}
}

void Gui::Dialog::switchPage( const std::string & new_page_name , const std::string & old_page_name , bool back )
{
	// hide and disconnect the old page
	//
	if( ! old_page_name.empty() )
	{
		Page & oldPage = page(old_page_name) ;
		oldPage.hide();
		m_main_layout->removeWidget(&oldPage);
		disconnect( &oldPage , SIGNAL(pageUpdateSignal()) , this , SLOT(pageUpdated()) );
	}

	// show and connect the new page
	//
	Page & new_page = page( new_page_name ) ;
	m_main_layout->insertWidget( 0 , &new_page ) ;
	new_page.onShow( back ) ; // Page method
	new_page.show() ;
	new_page.setFocus() ;
	connect( &new_page , SIGNAL(pageUpdateSignal()) , this , SLOT(pageUpdated()) ) ;

	// modify the next and finish buttons according to the page state
	//
	pageUpdated() ;
}

bool Gui::Dialog::historyContains( const std::string & name ) const
{
	return std::find( m_history.begin() , m_history.end() , name ) != m_history.end() ;
}

std::string Gui::Dialog::currentPageName() const
{
	return m_history.empty() ? std::string() : m_history.back() ;
}

Gui::Page & Gui::Dialog::previousPage( unsigned int distance )
{
	if( m_history.size() < (std::size_t(distance)+1U) ) throw std::runtime_error("internal error") ;
	auto p = m_history.rbegin() ;
	while( distance-- ) ++p ;
	G_DEBUG( "Gui::Dialog::previousPage: " << currentPageName() << " -> " << *p ) ;
	return page(*p) ;
}

void Gui::Dialog::dumpStateVariables( std::ostream & stream ) const
{
	dump( stream , false ) ;
}

void Gui::Dialog::dumpInstallVariables( std::ostream & stream ) const
{
	dump( stream , true ) ;
}

void Gui::Dialog::dump( std::ostream & stream , bool for_install ) const
{
	for( const auto & pname : m_history )
		page(pname).dump( stream , for_install ) ;
}

